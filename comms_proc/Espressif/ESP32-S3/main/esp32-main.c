#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <esp_event.h>
#include <esp_ota_ops.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <esp_sntp.h>
#include <esp_random.h>
#include <mdns.h>
#include <nvs_flash.h>
#include <mqtt_client.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <esp_err.h>
#include <esp_spiffs.h>
#include "appdefs.h"
#include "appmqtt.h"
#include "appwebserver.h"
#include "appota.h"
#include "appstate.h"
#include "appwifi.h"
#include "appfilesystem.h"
#include "driver/gpio.h"
#include "ssd1306.h"
#include "appusbhost.h"
#include "appuart.h"
#include "ftdi.h"

extern const uint8_t server_cert_pem_start[] asm("_binary_caroot_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_caroot_pem_end");

char ssid[sizeof(((wifi_sta_config_t*) NULL)->ssid)];
char pwd[sizeof(((wifi_sta_config_t*) NULL)->password)];

void app_main(void)
{
  // Otherwise nvs_open will fail.
  assert(strlen(STORAGE_NAMESPACE) <= NVS_KEY_NAME_MAX_SIZE - 1);

  ESP_LOGI("app_main", "Initializing NVS");
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  check_partitions();
  init_usbhost();

#if CONFIG_SD_FS_ENABLE
  init_spi_sd_fs();
  init_external_sd_fs();
#endif
#if CONFIG_OLED_ENABLE
  ESP_ERROR_CHECK(i2c_master_init());
  ssd1306_init();
  ssd1306DisplayClear();
  char dstr[256];
  strcpy(dstr,"ESP32-FPGA");
  ssd1306_display_text(dstr);
#endif
  init_uart();
  init_network();
  ftdi_init();
  read_wifi_config();
  ESP_LOGI("app_main", "saved ssid=%s", ssid);
  ESP_LOGI("app_main", "saved pwd=%s", pwd);
  establish_ssid_and_pw();

  start_webserver(false);

  init_time();

  setup_mdns();

  esp_mqtt_client_handle_t client = NULL;

  while(NULL == client)
  {
    client = setup_mqtt();
    if(NULL == client)
    {
      ESP_LOGI("app_main", "Client handle returned NULL, retrying");
      vTaskDelay(500/portTICK_PERIOD_MS);
    }
  }
  
  while(1)
  {
    vTaskDelay(portMAX_DELAY);
  }
}


