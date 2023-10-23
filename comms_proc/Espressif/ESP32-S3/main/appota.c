#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include <esp_ota_ops.h>
#include <esp_app_format.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include "esp_log.h"
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

static const esp_partition_t* configured_partition;
static const esp_partition_t* running_partition;
static const esp_partition_t* update_partition;
static char* TAG = "appota";

void do_update(char* str, size_t len)
{
  char* url = strndup(str, len);
  if (url == NULL) 
  {
    ESP_LOGI(TAG, "cannot allocate URL string");
    return;
  }

  esp_http_client_config_t config = {
    .url = url,
    .cert_pem = (char *) server_cert_pem_start,
    .timeout_ms = CONFIG_ESP32_FPGA_OTA_RECV_TIMEOUT,
#ifdef CONFIG_ESP32_FPGA_SKIP_COMMON_NAME_CHECK
    .skip_cert_common_name_check = CONFIG_ESP32_FPGA_SKIP_COMMON_NAME_CHECK,
#else
    .skip_cert_common_name_check = 0,
#endif
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL) 
  {
    ESP_LOGI(TAG, "cannot create HTTP client to download firmware");
    return;
  }

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) 
  {
    ESP_LOGE(TAG, "cannot connect to server to download firmware: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return;
  }
  esp_http_client_fetch_headers(client);

  update_partition = esp_ota_get_next_update_partition(NULL);
  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x", update_partition->subtype, (unsigned int)update_partition->address);
  assert(update_partition != NULL);

  const int read_buffer_size = 1024;
  char* read_buffer = malloc(read_buffer_size);

  /* Set by esp_ota_begin(), must be freed via esp_ota_end() or esp_ota_abort().  */
  esp_ota_handle_t update_handle = 0;

  // Read the data from the server and store it in the next partition.
  int binary_file_length = 0;
  bool image_header_was_checked = false;
  int unstored_bytes = 0;
  int nread;

  while (true) 
  {
    nread = esp_http_client_read(client, read_buffer + unstored_bytes, read_buffer_size - unstored_bytes);
    if (nread < 0) 
    {
      ESP_LOGE(TAG, "cannot read firmware");
      goto fail;
    }

    nread = unstored_bytes;
    unstored_bytes = 0;

    if (nread == 0)
    {
      break;
    }

    if (! image_header_was_checked) 
    {
      if (nread < sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) 
      {
        unstored_bytes = nread;
        continue;
      }

      esp_app_desc_t new_app_info;
      memcpy(&new_app_info, &read_buffer[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
      ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

      esp_app_desc_t running_app_info;
      if (esp_ota_get_partition_description(running_partition, &running_app_info) != ESP_OK) 
      {
        ESP_LOGE(TAG, "cannot get info for running application");
        // XYZ Should we just pass the version check?
        goto fail2;
      }
      ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);

      const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
      if (last_invalid_app != NULL) 
      {
        esp_app_desc_t invalid_app_info;
        if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) 
	{
          ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);

          if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) 
	  {
            ESP_LOGW(TAG, "New version is the same as the last version that failed.");
            ESP_LOGW(TAG, "Terminate update process.");
            goto fail2;
          }
        }
      }

      if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) 
      {
        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
        goto fail2;
      }

      image_header_was_checked = true;

      err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
      if (err != ESP_OK) 
      {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        goto fail;
      }
      ESP_LOGI(TAG, "esp_ota_begin succeeded");
    }

    err = esp_ota_write(update_handle, (const void *) read_buffer, nread);
    if (err != ESP_OK) 
    {
      ESP_LOGE(TAG, "cannot store firmware data");
      goto fail;
    }
    binary_file_length += nread;
    ESP_LOGD(TAG, "Written image length %d", binary_file_length);
  }

  if (! esp_http_client_is_complete_data_received(client)) 
  {
    if (errno == ECONNRESET || errno == ENOTCONN)
    {
      ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
    }
    goto fail;
  }

  ESP_LOGI(TAG, "firmware received");

  err = esp_ota_end(update_handle);
  if (err != ESP_OK) 
  {
    if (err == ESP_ERR_OTA_VALIDATE_FAILED)
    {
      ESP_LOGE(TAG, "Image validation failed, image is corrupted");
    }
    else
    {
      ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
    }
    goto fail;
  }

  err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK) 
  {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
    goto fail2;
  }

  ESP_LOGI(TAG, "firmware updated.  Rebooting now.");
  esp_restart();
  return;

fail:
  esp_ota_abort(update_handle);
fail2:
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  free(read_buffer);
  free(url);
  ESP_LOGE(TAG, "Exiting task due to fatal error...");
  (void) vTaskDelete(NULL);
  while (true)
    ;
}

void check_partitions(void)
{
  configured_partition = esp_ota_get_boot_partition();
  running_partition = esp_ota_get_running_partition();
  if (configured_partition != NULL) 
  {
    if (configured_partition != running_partition) 
    {
      ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x", (unsigned int)configured_partition->address, (unsigned int)running_partition->address);
      ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
  }

#if 0
  if (running_partition != NULL)
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)", running_partition->type, running_partition->subtype, running_partition->address);
  // esp_app_desc_t running_app_info;
  // const char* version = esp_ota_get_partition_description(running_partition, &running_app_info) == ESP_OK ? running_app_info.version : "*UNKNOWN*";
  // ESP_LOGI(TAG, "Running firmware version %s", version);
#endif
}

