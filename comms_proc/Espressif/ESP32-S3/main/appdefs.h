#pragma once

#define APP_VERSION "v1.01"
#define USE_SSL 1

#define REGISTER_WIFI_PASS ap_password
#define REGISTER_WIFI_CHANNEL 1

#define MAX_STA_CONN 4
#define MAXIMUM_RETRY 5

#define STORAGE_NAMESPACE "ESP32 SB"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define HAS_ADDRESS_BIT    BIT2

#define BITSTREAM_PACKAGE_FILENAME CONFIG_SD_FS_MOUNT_POINT"/package.zip" 

#define CHECK(expr, msg) \
  while ((res = expr) != ESP_OK) { \
      printf(msg "\n", res); \
      vTaskDelay(250 / portTICK_PERIOD_MS); \
  }

typedef struct {
  bool auth;
  char ssid[0];
} ssid_info_t;

extern const uint8_t server_cert_pem_start[] asm("_binary_caroot_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_caroot_pem_end");

