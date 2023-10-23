#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
//#include <esp_event.h>
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
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <esp_err.h>
#include <esp_spiffs.h>
#include "appfilesystem.h"
#include "appdefs.h"
#include "appmqtt.h"
#include "appota.h"
#include "appstate.h"
#include "frozen.h"
#include "ssd1306.h"
#include "appusbhost.h"
#include "appuart.h"
#include "arty_driver.h"
#include "jtag.h"
#include "ftdi.h"

static char* commands[] = 
{
  "GetVersion",
  "ListCommands",
#if CONFIG_SD_FS_ENABLE
  "ListSDCardFiles",
  "GetFileFromURL <url> <filename>",
  "RemoveFile <filename>",
  "JTAGProgramFPGA <filename>",
  "FlashSoftcore <filename>",
#endif
#if CONFIG_OLED_ENABLE    
  "DisplayClear",
  "DisplayString <value>",
  "DisplayHeartbeat <setting 0|1>",
#endif
};

int numCmds = sizeof(commands)/sizeof(char*);

static uint8_t heartbeat_display = 1;

static char out_buffer[1024];

static bool mqtt_running;
static bool mqtt_connected;
static char* update_topic;
static size_t update_topic_len;
static char* out_command_topic;
static size_t out_command_topic_len;
static char* in_command_topic;
static size_t in_command_topic_len;
static char* heartbeat_topic;
static size_t heartbeat_topic_len;

static char* TAG = "appmqtt";

static TimerHandle_t s_tmr = NULL;
static uint32_t cycle = 0;

static esp_mqtt_client_handle_t client = NULL;

extern SemaphoreHandle_t uartMutex; 

static void handle_connect(void);
static void handle_disconnect(void);

static void appmqtt_heartbeat_cb(TimerHandle_t arg);

static void appmqtt_heartbeat_cb(TimerHandle_t arg)
{
  if(mqtt_connected)
  {
    char heartbeat[256];
    ++cycle;
    sprintf(heartbeat, "{\"host\": \"%s\", \"connect seconds\": %" PRIu32 "}", getHostname(), cycle*5);
    appmqtt_send_msg(heartbeat_topic, heartbeat);
#if CONFIG_OLED_ENABLE
    if(heartbeat_display)
    {
      ssd1306DisplayClear();
      sprintf(heartbeat, "Host: %s\nConnect seconds: %" PRIu32 , getHostname(), cycle*5);
      ssd1306_display_text(heartbeat);
    }
#endif
  }
}

void appmqtt_send_msg(char *topic, char* message)
{
  ssize_t n = strlen(message);
  esp_mqtt_client_publish(client, topic, message, n, 0, 0);
}

void appmqtt_send_msg_n(char *topic, char* message, ssize_t n)
{
  esp_mqtt_client_publish(client, topic, message, n, 0, 0);
}

bool isMQTTRunning(void)
{
  return mqtt_running;
}

bool isMQTTConnected(void)
{
  return mqtt_connected;
}

void getFileFromURL(char *str, size_t len)
{
  char* url = NULL;
  char* filename = NULL;
  FILE* fid = NULL;
  const int read_buffer_size = 4096;
  char* read_buffer = NULL;
  esp_http_client_handle_t client = NULL;
  char* command = "GetFileFromURL";

  if((json_scanf(str, len, "{url: %Q}", &url))!=1)
  {
    ESP_LOGE(TAG, "No URL field");
    sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"No url field\"}", command);
    goto cleanup;
  }
  if(strlen(url)==0)
  {
    ESP_LOGE(TAG, "Empty url");
    sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Empty url field\"}", command);
    goto cleanup;
  }
  if((json_scanf(str, len, "{filename: %Q}", &filename))!=1)
  {
    ESP_LOGE(TAG, "No filename field");
    sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"No filename field\"}", command);
    goto cleanup;
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

  client = esp_http_client_init(&config);
  if (client == NULL) 
  {
    ESP_LOGE(TAG, "cannot create HTTP client to download file");
    sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Failed to create HTTP client\"}", command);
    goto cleanup;
  }
  else
  {
    ESP_LOGI(TAG, "Created http client");
  }
  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) 
  {
    ESP_LOGE(TAG, "cannot connect to server to download file: %s", esp_err_to_name(err));
    sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Failed to connect to server\"}", command);
    esp_http_client_cleanup(client);
    goto cleanup;
  }
  else
  {
    ESP_LOGI(TAG, "Opened http client");
  }
  esp_http_client_fetch_headers(client);

  read_buffer = malloc(read_buffer_size);
  fid = fopen(filename, "wb");
  if (fid == NULL) 
  {
      ESP_LOGE(TAG, "Failed to open file %s for writing",filename);
      sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Failed to open file %s for writing\"}", command, filename);
      goto cleanup;
  } 
  else
  {
    ESP_LOGI(TAG, "Opened file %s for writing", filename);
  } 
  // Read the data from the server and store it in the file 
  int binary_file_length = 0;
  int nread;

  ESP_LOGI(TAG, "About to read from client");
  while (true) 
  {
  //  ESP_LOGI(TAG, "About to read from client");
    nread = esp_http_client_read(client, read_buffer, read_buffer_size);
    if (nread < 0) 
    {
      ESP_LOGE(TAG, "cannot read file");
      sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Failed to read file from client\"}", command);
      goto fail;
    }
    //ESP_LOGI(TAG, "Read %d bytes", nread);
    if (nread == 0)
    {
      break;
    }
	
    fwrite(read_buffer, sizeof(char), nread, fid);
    binary_file_length += nread;
    //ESP_LOGI(TAG, "Written image length %d", binary_file_length);
  }
  ESP_LOGI(TAG, "Written image length %d", binary_file_length);
  sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Written image length %d\"}", command, binary_file_length);

cleanup:
  if(fid!=NULL)
    fclose(fid);
  if(client != NULL)
  {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
  }
  if(read_buffer!=NULL)
    free(read_buffer);
  if(filename!=NULL)
    free(filename);
  if(url!=NULL)
    free(url);
  return;
fail:
  if(fid!=NULL)
    fclose(fid);
  if(client != NULL)
  {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
  }
  if(read_buffer!=NULL)
    free(read_buffer);
  if(url!=NULL)
    free(url);
  if(filename!=NULL)
    free(filename);
  ESP_LOGE(TAG, "Exiting task due to fatal error...");
  (void) vTaskDelete(NULL);
  while (true)
    ;
}

static void commandInterpreter(char* str, size_t len)
{
  char *command = NULL;

  if((json_scanf(str, len, "{command: %Q}", &command))==1)
  {
#if 0 // TODO Add in function pointer usage
    for(int i=0; i < numCmds; i++)
    {
      if(strcmp(command, commands[i].name) == 0)
      {
        commands[i].function(str);
	break;
      }
    }
#endif
    if(strcmp(command, "GetVersion")==0)
    {
      ESP_LOGI(TAG, "ESP FPGA Ver: %s", APP_VERSION);
      sprintf(out_buffer, "{\"command\": \"GetVersion\", \"response\":\"%s\"}", APP_VERSION);
    }
    else if(strcmp(command, "ListCommands")==0)
    {
      sprintf(out_buffer, "{\"command\": \"%s\", \"response\": [", command);
      for(int i=0; i<numCmds; i++)
      {
        if(i!=0)
	  strcat(out_buffer,", ");
	strcat(out_buffer, "\"");
        strcat(out_buffer, commands[i]);	      
	strcat(out_buffer, "\"");
      }
      strcat(out_buffer, "]}");
    }
#if CONFIG_SD_FS_ENABLE
    else if(strcmp(command, "GetFileFromURL")==0)
    {
      getFileFromURL(str, len);	    
    }
    else if(strcmp(command, "ListSDCardFiles")==0)
    {
      sprintf(out_buffer, "{\"command\": \"%s\", \"response\": ", command);
      list_dir(CONFIG_SD_FS_MOUNT_POINT, out_buffer);	    
    }
    else if(strcmp(command, "RemoveFile")==0)
    {
      char *filename = NULL;
      if((json_scanf(str, len, "{filename: %Q}", &filename))==1)
      {
        int retVal = remove_file(filename);
	if(retVal == 0)
	{
          sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"File %s removed\"}", command, filename);
	}
	else
	{
          sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Error removing file %s\"}", command, filename);
	}
      }	     
      else
      {
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"No filename field\"}", command);
      } 
      if(filename != NULL)
      {
        free(filename);
      }
    }
    else if(strcmp(command, "JTAGProgramFPGA")==0)
    {
      char *fname = NULL;
      if((json_scanf(str, len, "{filename: %Q}", &fname))==1)
      {
	jtag_program(fname);
	ESP_LOGI(TAG, "JTAG Program FPGA complete");
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"FPGA configured with %s\"}", command, fname);
      }
      else
      {
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"No filename field\"}", command);
      }
    }
    else if(strcmp(command, "JTAGVerifySoftcore")==0)
    {
      char *fname = NULL;
      if((json_scanf(str, len, "{filename: %Q}", &fname))==1)
      {
        jtag_verify_softcore(fname);
        ESP_LOGI(TAG, "JTAG Verify Softcore complete");
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Softcore verified\"}", command);
      }
      else 
      {
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"No filename field\"}", command);
      }
    }
    else if(strcmp(command, "JTAGUARTLoopbackTest")==0)
    {
      jtag_uart_loopback_test();
    }
    else if(strcmp(command, "JTAGUARTTXTest")==0)
    {
      //jtag_uart_tx_test();
      jtag_reset();
      uint8_t rbuf[4] = {0}; uint8_t wbuf[4] = {0}; uint16_t len = 4;
      jtag_drscan_bytes_read(wbuf, rbuf, len);
      ESP_LOGI("MQTT", "Got string back from drscan_bytes_read %s", rbuf);
    }
    else if(strcmp(command, "JTAGUARTProgramSoftcore")==0)
    {
      char *fname = NULL;
      if((json_scanf(str, len, "{filename: %Q}", &fname))==1)
      {
#if 0 
	ftdi_uart_configure(8,115200, XON_XOFF, 10000000);
	jtag_control_write(3,0,0);
	vTaskDelay(10/portTICK_PERIOD_MS);
	jtag_control_write(2,0,0);
	vTaskDelay(10/portTICK_PERIOD_MS);
	jtag_program_softcore(fname);
#endif
	jtag_program_softcore(fname);
	vTaskDelay(10/portTICK_PERIOD_MS);
//	jtag_control_write(0,0,0);
        ESP_LOGI(TAG, "JTAG Program Softcore complete");
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Softcore flashed with %s\"}", command, fname);
      } 
    }
    else if(strcmp(command, "FlashSoftcore")==0)
    {
      char *fname = NULL;

      if((json_scanf(str, len, "{filename: %Q}", &fname))==1)
      {
#if 1
	xSemaphoreTake(uartMutex, portMAX_DELAY);
        jtag_reset();
	vTaskDelay(10/portTICK_PERIOD_MS);	
	jtag_control_write(3,0,0);
	vTaskDelay(10/portTICK_PERIOD_MS);
	jtag_control_write(2,0,0);
	vTaskDelay(10/portTICK_PERIOD_MS);
#endif
	arty_gpio_uart_riscv_flash(fname);
#if 1 
	vTaskDelay(10/portTICK_PERIOD_MS);
	jtag_control_write(0,0,0);
	vTaskDelay(500/portTICK_PERIOD_MS);
	xSemaphoreGive(uartMutex); 
#endif
        ESP_LOGI(TAG, "Program Softcore complete");
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Softcore flashed with %s\"}", command, fname);
      }
      else
      {
        ESP_LOGI(TAG, "No filename provided");
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"No filename provided\"}", command);
      }
      if(fname != NULL)
        free(fname);
    }
    else if(strcmp(command, "FlashFPGA")==0)
    {
      char *fname = NULL;
      if((json_scanf(str, len, "{filename: %Q}", &fname))==1)
      {
	arty_flash(fname);
        ESP_LOGI(TAG, "Finished arty_task");
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"FPGA configured\"}", command);
      }
      else
      {
        ESP_LOGI(TAG, "No filename provided");
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"No filename field\"}", command);
      }
      if(fname != NULL)
      {
        free(fname);
      }
    }
#endif
#if CONFIG_OLED_ENABLE    
    else if(strcmp(command, "DisplayClear")==0)
    {
      ssd1306DisplayClear();
      sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Display cleared\"}", command);
    }
    else if(strcmp(command, "DisplayHeartbeat")==0)
    {
      int set = 0;
      if(json_scanf(str, len, "{setting: %B}",&set)==1)
      {
	heartbeat_display = (uint8_t)set;
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Set to %s\"}", command,(set==0)?("false"):("true"));
      }
      else
      {
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"No setting field\"}", command);
      }
    }
    else if(strcmp(command, "DisplayString")==0)
    {
      char *value = NULL;

      if((json_scanf(str, len, "{value: %Q}", &value))>=1)
      {
        ssd1306DisplayClear();
        ssd1306_display_text(value);	
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"String displayed\"}", command);
      }
      else
      {
        ESP_LOGI(TAG, "No value field");
        sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"No value field\"}", command);
      }
      if(str != NULL)
      {
        free(value);
      }
    }
#endif
    else
    {
      ESP_LOGI(TAG, "Unknown command: %s", command);	    
      sprintf(out_buffer, "{\"command\": \"%s\", \"response\":\"Unknown command\"}", command);
    } 
  }
  else
  {
    ESP_LOGI(TAG, "No command field");	  
    sprintf(out_buffer, "{\"command\": NULL, \"response\":\"No command field\"}");
  }  

  if(command != NULL)
  {
    free(command);
  }
  appmqtt_send_msg(out_command_topic, out_buffer);
}

static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  esp_mqtt_event_handle_t event = event_data;
  switch (event->event_id) {
  case MQTT_EVENT_BEFORE_CONNECT:
    ESP_LOGI(TAG, "MQTT initialized and about to start connecting to broker");
    mqtt_running = true;
    break;
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT connected");
    mqtt_connected = true;
    handle_connect();
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT disconnected");
    mqtt_connected = false;
    handle_disconnect();
    break;
  case MQTT_EVENT_ERROR: 
    ESP_LOGI(TAG, "MQTT error: %d", event->error_handle->connect_return_code);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "topic=%.*s data=%.*s", event->topic_len, event->topic, event->data_len, event->data);
    if (event->topic_len == update_topic_len && strncmp(event->topic, update_topic, update_topic_len) == 0) 
    {
      do_update(event->data, event->data_len);
    } 
    else if (event->topic_len == in_command_topic_len && strncmp(event->topic, in_command_topic, in_command_topic_len) == 0) 
    {
      commandInterpreter(event->data, event->data_len);
    }
    else
      ESP_LOGI(TAG, "unhandled message for topic %.*s", event->topic_len, event->topic);
    break;
  default:
      ESP_LOGI(TAG, "Event received: %d", event->event_id);
    break;
  }
}

esp_mqtt_client_handle_t setup_mqtt(void)
{
  mqtt_running = false;
  mqtt_connected = false;

  mdns_result_t* mdnsres = NULL;
  esp_err_t err = mdns_query_ptr("_mqtt", "_tcp", 3000, 20,  &mdnsres);
  if (err) 
  {
    ESP_LOGI(TAG, "mdns query for _mqtt._tcp failed");
    return NULL;
  }
  if (mdnsres == NULL) 
  {
    ESP_LOGI(TAG, "cannot find mqtt broker via mdns");
    return NULL;
  }

  mdns_result_t* r = mdnsres;
  while (r != NULL) 
  {
    ESP_LOGI(TAG, "mdns result for MQTT points to %s:%d", r->hostname, r->port);

    char buf[256];
    snprintf(buf, sizeof(buf), "mqtt://%s.local:%u", r->hostname, r->port);

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0) 
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = buf,
    };    
#else
    esp_mqtt_client_config_t mqtt_cfg = {
      .uri = buf,
    };
#endif

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client != NULL) 
    {
      err = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, client);
      if(err)
      {
        ESP_LOGI(TAG, "MQTT client register event failed");
      }
      else 
      {
	      err = esp_mqtt_client_start(client);
      	if(err)
      	{
	  ESP_LOGI(TAG, "MQTT client start failed");
      	}
      	else
      	{
	  ESP_LOGI(TAG, "MQTT client start successful");
      	}
      }
      break;
    }

    ESP_LOGI(TAG, "MQTT client startup failed during esp_mqtt_client_init");
    r = r->next;
  }

  mdns_query_results_free(mdnsres);
  if (client == NULL)
  {
    ESP_LOGI(TAG, "mdnsres did not lead to initialized MQTT client");
    return NULL;
  }
  return client;
}

static void handle_disconnect(void)
{
  if(s_tmr != NULL)
  {
    xTimerStop(s_tmr, 0);
  }
}

static void handle_connect(void)
{
  heartbeat_topic_len = asprintf(&heartbeat_topic, "/%s/heartbeat", getHostname()); 
  update_topic_len = asprintf(&update_topic, "/%s/update", getHostname()); 
  esp_mqtt_client_subscribe(client, update_topic, 1);
  ESP_LOGI(TAG, "MQTT channel %s subscribed", update_topic);
  out_command_topic_len = asprintf(&out_command_topic, "/%s/out-command", getHostname()); 
  in_command_topic_len = asprintf(&in_command_topic, "/%s/in-command", getHostname()); 
  esp_mqtt_client_subscribe(client, in_command_topic, 1);
  ESP_LOGI(TAG, "MQTT channel %s subscribed", in_command_topic);
  
  int tmr_id = 1;
  if(s_tmr==NULL)
  {
    s_tmr = xTimerCreate("appmqttHeartbeatTmr", (5000 / portTICK_PERIOD_MS), pdTRUE, (void *) &tmr_id, appmqtt_heartbeat_cb);
  }
  xTimerStart(s_tmr, portMAX_DELAY);
}

