#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "frozen.h"
#include "string.h"
#include "driver/gpio.h"
#include <mqtt_client.h>
#include "appmqtt.h"
#include "appstate.h"
#include "appuart.h"

static const char *TAG = "appuart";

static char* out_topic;
static size_t out_topic_len; 

#define RX_BUF_SIZE  1024
static char rxData[RX_BUF_SIZE];
static int rxLength = 0;

#define TXD_PIN CONFIG_COMMS_PROC_UART_TX_GPIO
#define RXD_PIN CONFIG_COMMS_PROC_UART_RX_GPIO

SemaphoreHandle_t uartMutex; 

int sendUARTData(const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    return txBytes;
}

int sendUARTBytes(const uint8_t* bytes, int len)
{
  const int txBytes = uart_write_bytes(UART_NUM_1, bytes, len);
  return txBytes;
}

void resetUARTRXData(void)
{
  memset(rxData, '\0', RX_BUF_SIZE);
  rxLength = 0;
}

void flushUART(void)
{
  uart_flush(UART_NUM_1);
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    static unsigned char rxByteIn = '\0';
    char *message = NULL;
    char *topic = NULL;
    int jsonStatus = -1;

    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);

    while (1) 
    {
      char *message = NULL;
      char *topic = NULL;
      xSemaphoreTake(uartMutex, portMAX_DELAY);      
      const int rxBytes = uart_read_bytes(UART_NUM_1, &rxByteIn, 1, 10 / portTICK_PERIOD_MS );
      if (rxBytes > 0) 
      {
	//ESP_LOGI("UART", "Received byte: %c %" PRIX8, rxByteIn, rxByteIn);
	if((rxByteIn!=0x0D)&&((rxByteIn<32)||(rxByteIn>126)))
	{
	  //ESP_LOGI("UART", "Ignoring byte");
          xSemaphoreGive(uartMutex);      
	  continue;
	}
	rxData[rxLength] = rxByteIn;
	rxLength++;
	//ESP_LOGI("UART", "Received byte: %c %" PRIX8, rxByteIn, rxByteIn);
	//ESP_LOGI("UART", "Received byte: %" PRIX8, rxByteIn);
	if(rxByteIn == 0x0D)
	{
	  rxData[rxLength] = '\0';
	  ESP_LOGI("UART", "Received 0x0D");
          jsonStatus = json_scanf(rxData, rxLength, "{topic: %Q, message: %Q}", &topic, &message);
	  if(jsonStatus == 2) // Valid format 
	  {
	    if(topic != NULL)
	    {
	      //out_topic_len = asprintf(&out_topic, "/%s/risc_v", getHostname());
	      out_topic_len = asprintf(&out_topic, "/%s/%s", getHostname(), topic);
	      if(isMQTTConnected())
	      {
	        //appmqtt_send_msg_n(out_topic, rxData,rxLength-1);
	        appmqtt_send_msg(out_topic, message);
	      }
	    }
	  }
	  else
	  {
	    ESP_LOGI("UART", "Recieved 0x0D but JSON malformed");
	  }
          resetUARTRXData();
	}
	else if(rxLength==RX_BUF_SIZE)  // Full buffer
	{
	  ESP_LOGI(RX_TASK_TAG, "rxLength == RX_BUF_SIZE Resetting buffer!!!");
          resetUARTRXData();
	}
	// Otherwise, assume JSON string is still forthcoming
      }
      if(message)
      {
        free(message);
	message = NULL;
      }
      if(topic)
      {
        free(topic);
	topic = NULL;
      }
      xSemaphoreGive(uartMutex);      
    }
}

void init_uart(void) 
{
    const uart_config_t uart_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
        .source_clk = UART_SCLK_DEFAULT,
#endif
    };

    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uartMutex = xSemaphoreCreateMutex();
    xTaskCreate(rx_task, "uart_rx_task", 1024*4, NULL, 3, NULL);
}

