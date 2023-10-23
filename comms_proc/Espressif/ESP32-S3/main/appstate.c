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

static char* TAG = "appstate";

extern char ssid[sizeof(((wifi_sta_config_t*) NULL)->ssid)];
extern char pwd[sizeof(((wifi_sta_config_t*) NULL)->password)];

char* hostname;
static char* tz;
static const char default_tz[] = CONFIG_ESP32_FPGA_DEFAULT_TZ;

void setHostname(uint8_t* mac)
{
  if (asprintf(&hostname, "%s-%02X%02X%02X%02X%02X%02X", "ESP32", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) == -1)
  {
    abort();
  }
}

char* getHostname(void)
{
  return hostname;
}

void read_wifi_config(void)
{
  nvs_handle_t handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK)
  {
    return;
  }

  size_t required_size = 0;
  err = nvs_get_blob(handle, "ssid", NULL, &required_size);
  if (err != ESP_OK || required_size == 0 || required_size > sizeof(ssid))
  {
    goto out;
  }

  err = nvs_get_blob(handle, "ssid", ssid, &required_size);
  if (err != ESP_OK) 
  {
    ssid[0] = '\0';
    goto out;
  }
  ssid[sizeof(ssid) - 1] = '\0';

  required_size = 0;
  err = nvs_get_blob(handle, "pwd", NULL, &required_size);
  if (err != ESP_OK || required_size == 0 || required_size > sizeof(pwd)) 
  {
    ssid[0] = '\0';
    goto out;
  }

  err = nvs_get_blob(handle, "pwd", pwd, &required_size);
  if (err != ESP_OK) 
  {
    ssid[0] = '\0';
    pwd[0] = '\0';
    goto out;
  }
  pwd[sizeof(pwd) - 1] = '\0';

  required_size = 0;
  err = nvs_get_blob(handle, "tz", NULL, &required_size);
  if (err != ESP_OK || required_size == 0)
  {
    goto out;
  }

  tz = malloc(required_size);
  if (tz != NULL) 
  {
    err = nvs_get_blob(handle, "tz", tz, &required_size);
    if (err == ESP_OK && required_size != 0)
    {
      // Just in case...
      tz[required_size - 1] = '\0';
    }
    else 
    {
      free(tz);
      tz = NULL;
    }
  }

out:
  nvs_close(handle);
}


void write_wifi_config(void)
{
  nvs_handle_t handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
  if (err != ESP_OK) 
  {
    ESP_LOGI(TAG, "cannot save WiFi config: %s", esp_err_to_name(err));
    return;
  }

  err = nvs_set_blob(handle, "ssid", ssid, strlen(ssid) + 1);
  if (err != ESP_OK)
  {
    goto out;
  }

  err = nvs_set_blob(handle, "pwd", pwd, strlen(pwd) + 1);
  if (err != ESP_OK)
  {
    goto out;
  }

  nvs_commit(handle);

out:
  nvs_close(handle);
  if (err != ESP_OK)
  {
    ESP_LOGI(TAG, "failed to write WiFi config: %s", esp_err_to_name(err));
  }
}

void init_time(void)
{
  ESP_LOGI(TAG, "Entering init_time()");
  if (tz == NULL) 
  {
    tz = (char*) default_tz;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
      return;
    }

    err = nvs_set_blob(handle, "tz", tz, strlen(tz) + 1);
    if (err == ESP_OK)
    {
      nvs_commit(handle);
    }

    nvs_close(handle);
  }

  setenv("TZ", tz, 1);
  tzset();

  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, CONFIG_ESP32_FPGA_NTP_SERVER);
  sntp_init();
}

void setup_mdns(void)
{
  ESP_ERROR_CHECK(mdns_init());
  ESP_ERROR_CHECK(mdns_hostname_set(hostname));
}


