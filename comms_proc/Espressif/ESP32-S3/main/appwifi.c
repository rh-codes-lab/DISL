#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <esp_event.h>
#include <esp_ota_ops.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <esp_https_server.h>
#include <esp_http_client.h>
#include <esp_sntp.h>
#include <esp_random.h>
#include <mdns.h>
#include <nvs_flash.h>
#include <mqtt_client.h>
#include "appdefs.h"
#include "appmqtt.h"
#include "appwebserver.h"
#include "appota.h"
#include "appstate.h"
#include "appwifi.h"


extern char ssid[sizeof(((wifi_sta_config_t*) NULL)->ssid)];
extern char pwd[sizeof(((wifi_sta_config_t*) NULL)->password)];
extern ssid_info_t** aps;

static char* TAG = "appwifi";

static unsigned ap_count;
static esp_netif_t* sta;
static int s_retry_num;
static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */

unsigned getAPCount(void)
{
  return ap_count;
}

EventGroupHandle_t* getWifiEventGroup(void)
{
  return &s_wifi_event_group;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_wifi_connect();
  }
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
  {
    if (s_retry_num < MAXIMUM_RETRY) 
    {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } 
    else
    {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }

    ESP_LOGI(TAG,"connect to the AP failed");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
  {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

esp_err_t connect_station(void)
{
  assert(sta != NULL);

  wifi_config_t wifi_config = {
    .sta = {
      /* Setting a password implies station will connect to all security modes including WEP/WPA.
       * However these modes are deprecated and not advisable to be used. Incase your Access point
       * doesn't support WPA2, these mode can be enabled by commenting below line */
       .threshold.authmode = WIFI_AUTH_WPA2_PSK,

      .pmf_cfg = {
          .capable = true,
          .required = false
      },
    },
  };
  strncpy((char*) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
  strncpy((char*) wifi_config.sta.password, pwd, sizeof(wifi_config.sta.password));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, &instance_got_ip));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(TAG, "WiFi connect to %s finished", ssid);

  s_wifi_event_group = xEventGroupCreate();

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
   * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT) 
  {
    ESP_LOGI(TAG, "WiFi connect successful");
    return ESP_OK;
  }

  vEventGroupDelete(s_wifi_event_group);

  ESP_LOGI(TAG, "WiFi connect failed");
  return ESP_FAIL;
}

void scan(void)
{
  for (unsigned i = 0; i < ap_count; ++i)
  {
    free(aps[i]);
  }
  free(aps);
  aps = NULL;
  ap_count = 0;

  assert(sta != NULL);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
  uint16_t info_count = 64;
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&info_count));
  ESP_LOGI(TAG, "%hu total APs", info_count);
  wifi_ap_record_t* ap_info = calloc(sizeof(wifi_ap_record_t), info_count);
  assert(ap_info != NULL);
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&info_count, ap_info));
  ESP_ERROR_CHECK(esp_wifi_stop());

  // Count unique names.
  unsigned count = 0;
  for (uint16_t i = 0; i < info_count; ++i) 
  {
    uint16_t j = 0;
    while (j < i) 
    {
      if (strcmp((const char*) ap_info[j].ssid, (const char*) ap_info[i].ssid) == 0)
      {
        break;
      }
      ++j;
    }
    if (j == i)
    {
      ++count;
    }
  }

  ESP_LOGI(TAG, "%u unique APs", count);
  if (count != 0) 
  {
    aps = calloc(sizeof(ssid_info_t*), count);
    assert(aps != NULL);
    for (uint16_t i = 0; i < info_count; ++i) 
    {
      uint16_t j = 0;
      while (j < ap_count) 
      {
        if (strcmp(aps[j]->ssid, (const char*) ap_info[i].ssid) == 0)
	{
          break;
	}
        ++j;
      }
      if (j == ap_count) 
      {
        size_t l = strlen((const char*) ap_info[i].ssid) + 1;
        aps[ap_count] = malloc(sizeof(ssid_info_t) + l);
        assert(aps[ap_count] != NULL);
        memcpy(aps[ap_count]->ssid, ap_info[i].ssid, l);
        aps[ap_count]->auth = ap_info[i].authmode != WIFI_AUTH_OPEN;
        ++ap_count;
      }
    }
  }

  free(ap_info);
}

void establish_ssid_and_pw(void)
{
  bool changed = false;

  while (true) 
  {
    while (strlen(ssid) == 0) 
    {
      scan();

      create_ap();
      changed = true;
    }

    if (connect_station() == ESP_OK) 
    {
      ESP_LOGI(TAG, "connected to station");
      if (changed)
      {
        write_wifi_config();
      }
      break;
    }

    ssid[0] = '\0';
    pwd[0] = '\0';
    ESP_LOGI(TAG, "failed to connect to station");
  }
}

void init_network(void)
{
  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  // We in any case need the station first: if we have SSID and password we will connect.  Otherwise we will scan first.
  sta = esp_netif_create_default_wifi_sta();
  assert(sta != NULL);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  setHostname(mac);
}



