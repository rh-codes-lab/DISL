/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "appfilesystem.h"
#include <esp_vfs.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <esp_spiffs.h>
#include "esp_log.h"
#include "usb/usb_host.h"
#include "appuart.h"

#define CLIENT_NUM_EVENT_MSG        5

#define ACTION_OPEN_DEV             0x01
#define ACTION_GET_DEV_INFO         0x02
#define ACTION_GET_DEV_DESC         0x04
#define ACTION_GET_CONFIG_DESC      0x08
#define ACTION_GET_STR_DESC         0x10
#define ACTION_CLOSE_DEV            0x20
#define ACTION_EXIT                 0xF0
#define ACTION_TRANSFER             0x80
#define ACTION_CONTROL_TRANSFER     0x40

typedef struct {

        union { // offset   description
                uint8_t bmRequestType; //   0      Bit-map of request type

                struct {
                        uint8_t recipient : 5; //          Recipient of the request
                        uint8_t type : 2; //          Type of request
                        uint8_t direction : 1; //          Direction of data X-fer
                } __attribute__((packed));
        } ReqType_u;
        uint8_t bRequest; //   1      Request

        union {
                uint16_t wValue; //   2      Depends on bRequest

                struct {
                        uint8_t wValueLo;
                        uint8_t wValueHi;
                } __attribute__((packed));
        } wVal_u;
        uint16_t wIndex; //   4      Depends on bRequest
        uint16_t wLength; //   6      Depends on bRequest
} __attribute__((packed)) SETUP_PKT, *PSETUP_PKT;

typedef struct {
    usb_host_client_handle_t client_hdl;
    uint8_t dev_addr;
    usb_device_handle_t dev_hdl;
    uint32_t actions;
} class_driver_t;

static const char *TAG = "arty_driver";
static class_driver_t driver_obj = {0};
usb_transfer_t *transfer;
usb_transfer_t *read_transfer;
static QueueHandle_t transfer_queue;
static QueueHandle_t in_transfer_queue;
static QueueHandle_t control_transfer_queue;
TaskHandle_t arty_receive_task_hdl;

static void client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    class_driver_t *driver_obj = (class_driver_t *)arg;
    switch (event_msg->event) 
    {
        case USB_HOST_CLIENT_EVENT_NEW_DEV:
            if (driver_obj->dev_addr == 0) 
            {
                driver_obj->dev_addr = event_msg->new_dev.address;
                //Open the device next
                driver_obj->actions |= ACTION_OPEN_DEV;
            }
            break;
        case USB_HOST_CLIENT_EVENT_DEV_GONE:
            if (driver_obj->dev_hdl != NULL) 
            {
                ESP_LOGI("class_driver","Received event dev gone");
                //Cancel any other actions and close the device next
                driver_obj->actions = ACTION_CLOSE_DEV;
            }
            break;
        default:
            //Should never occur
            abort();
    }
}

static void action_open_dev(class_driver_t *driver_obj)
{
    assert(driver_obj->dev_addr != 0);
    ESP_LOGI(TAG, "Opening device at address %d", driver_obj->dev_addr);
    ESP_ERROR_CHECK(usb_host_device_open(driver_obj->client_hdl, driver_obj->dev_addr, &driver_obj->dev_hdl));
    usb_host_interface_claim(driver_obj->client_hdl, driver_obj->dev_hdl, 1, 0);
    //Get the device's information next
    driver_obj->actions &= ~ACTION_OPEN_DEV;
    driver_obj->actions |= ACTION_GET_DEV_INFO;
}

static void action_get_info(class_driver_t *driver_obj)
{
    assert(driver_obj->dev_hdl != NULL);
    ESP_LOGI(TAG, "Getting device information");
    usb_device_info_t dev_info;
    ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
    ESP_LOGI(TAG, "\t%s speed", (dev_info.speed == USB_SPEED_LOW) ? "Low" : "Full");
    ESP_LOGI(TAG, "\tbConfigurationValue %d", dev_info.bConfigurationValue);
    //Todo: Print string descriptors

    //Get the device descriptor next
    driver_obj->actions &= ~ACTION_GET_DEV_INFO;
    driver_obj->actions |= ACTION_GET_DEV_DESC;
}

static void action_get_dev_desc(class_driver_t *driver_obj)
{
    assert(driver_obj->dev_hdl != NULL);
    ESP_LOGI(TAG, "Getting device descriptor");
    const usb_device_desc_t *dev_desc;
    ESP_ERROR_CHECK(usb_host_get_device_descriptor(driver_obj->dev_hdl, &dev_desc));
    usb_print_device_descriptor(dev_desc);
    //Get the device's config descriptor next
    driver_obj->actions &= ~ACTION_GET_DEV_DESC;
    driver_obj->actions |= ACTION_GET_CONFIG_DESC;
}

static void action_get_config_desc(class_driver_t *driver_obj)
{
    assert(driver_obj->dev_hdl != NULL);
    ESP_LOGI(TAG, "Getting config descriptor");
    const usb_config_desc_t *config_desc;
    ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(driver_obj->dev_hdl, &config_desc));
    usb_print_config_descriptor(config_desc, NULL);
    //Get the device's string descriptors next
    driver_obj->actions &= ~ACTION_GET_CONFIG_DESC;
    driver_obj->actions |= ACTION_GET_STR_DESC;
}

static void action_get_str_desc(class_driver_t *driver_obj)
{
    assert(driver_obj->dev_hdl != NULL);
    usb_device_info_t dev_info;
    ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
    if (dev_info.str_desc_manufacturer) 
    {
        ESP_LOGI(TAG, "Getting Manufacturer string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_manufacturer);
    }
    if (dev_info.str_desc_product) 
    {
        ESP_LOGI(TAG, "Getting Product string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_product);
    }
    if (dev_info.str_desc_serial_num) 
    {
        ESP_LOGI(TAG, "Getting Serial Number string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_serial_num);
    }
    //Nothing to do until the device disconnects
    driver_obj->actions &= ~ACTION_GET_STR_DESC;
}

static void action_close_dev(class_driver_t *driver_obj)
{
    ESP_ERROR_CHECK(usb_host_device_close(driver_obj->client_hdl, driver_obj->dev_hdl));
    driver_obj->dev_hdl = NULL;
    driver_obj->dev_addr = 0;
    //We need to exit the event handler loop
    driver_obj->actions &= ~ACTION_CLOSE_DEV;
    //driver_obj->actions |= ACTION_EXIT;
}

static void control_transfer_cb(usb_transfer_t *transfer)
{
    uint8_t outByte = 0;

    //class_driver_t *driver_obj = (class_driver_t *)transfer->context;
    //printf("Control transfer status %d, actual number of bytes transferred %d\n", transfer->status, transfer->actual_num_bytes);
    xQueueSend(control_transfer_queue, &outByte, 0);
}

static void transfer_cb(usb_transfer_t *transfer)
{
    uint8_t outByte = 0;

    //This is function is called from within usb_host_client_handle_events(). Don't block and try to keep it short
    //class_driver_t *driver_obj = (class_driver_t *)transfer->context;
    //printf("Transfer status %d, actual number of bytes transferred %d\n", transfer->status, transfer->actual_num_bytes);
    xQueueSend(transfer_queue, &outByte, 0);
}

static void in_transfer_cb(usb_transfer_t *transfer)
{
    uint8_t outByte = 0;

    //This is function is called from within usb_host_client_handle_events(). Don't block and try to keep it short
    //class_driver_t *driver_obj = (class_driver_t *)transfer->context;
    //printf("Transfer status %d, actual number of bytes transferred %d\n", transfer->status, transfer->actual_num_bytes);
    //usb_host_transfer_submit(read_transfer);   
    ESP_LOGI("ARTY_IN_CB","Bytes %" PRIu16 " received", transfer->actual_num_bytes);
    xQueueSend(in_transfer_queue, &outByte, 0);
    usb_host_transfer_submit(read_transfer);   
}


void arty_transfer_control(uint8_t addr, uint8_t ep, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi, uint16_t wInd, uint16_t total)
{
	uint8_t inbyte; 

    SETUP_PKT setup_pkt;
    setup_pkt.ReqType_u.bmRequestType = bmReqType;
    setup_pkt.bRequest = bRequest;
    setup_pkt.wVal_u.wValueLo = wValLo;
    setup_pkt.wVal_u.wValueHi = wValHi;
    setup_pkt.wIndex = wInd;
    setup_pkt.wLength = total;

    transfer->num_bytes = 8;
    transfer->callback = control_transfer_cb;
    transfer->bEndpointAddress = ep;
    memcpy(transfer->data_buffer, (void *) & setup_pkt, 8);
    
    //driver_obj.actions |= ACTION_CONTROL_TRANSFER;
    usb_host_transfer_submit_control(driver_obj.client_hdl, transfer);	
    //ESP_LOGI("class_driver", "Waiting on control transfer queue");
    xQueueReceive(control_transfer_queue,&inbyte,portMAX_DELAY);
}

void arty_transfer_data(uint8_t *data, int size, uint8_t EP)
{
    uint8_t inbyte; 
    
    transfer->num_bytes = size;
    transfer->callback = transfer_cb;
    transfer->bEndpointAddress = EP;
    memcpy(transfer->data_buffer, data, size);
    //driver_obj.actions |= ACTION_TRANSFER;
    usb_host_transfer_submit(transfer);   
    //ESP_LOGI("class_driver", "Waiting on transfer queue");
    xQueueReceive(transfer_queue,&inbyte,portMAX_DELAY);
}

uint16_t arty_receive_data(uint8_t *data, uint16_t size, uint8_t EP)
{
    uint16_t recv = 0; 

#if 0
    uint8_t inbyte; 
    read_transfer->num_bytes = size;
    read_transfer->callback = in_transfer_cb;
    read_transfer->bEndpointAddress = EP;
    //driver_obj.actions |= ACTION_TRANSFER;
    usb_host_transfer_submit(read_transfer);   
    ESP_LOGI("class_driver", "Waiting on transfer queue");
    xQueueReceive(in_transfer_queue,&inbyte,portMAX_DELAY);
    ESP_LOGI("class_driver", "Finished waiting on transfer queue");
    recv = read_transfer->actual_num_bytes;
    for (int i =0; i < recv; i++)
    {
      data[i] = read_transfer->data_buffer[i];
    }
#endif
    return recv;
}

void arty_flash(char *filename)
{
    uint8_t arty_bAddress = 1;
    FILE *f = fopen(filename, "rb");
    size_t ret;
    uint8_t buf[2048];
    uint8_t buffl;
    uint8_t buffh;
    uint16_t lenl;
    uint16_t lenh;
    uint16_t len;
   
    if(f == NULL)
    {
      ESP_LOGE(TAG,"File does not exist!");
      return;
    } 
    arty_transfer_control(arty_bAddress, 0, 0x40, 0, 0, 0, 1, 0);
    arty_transfer_control(arty_bAddress, 0, 0x40, 9, 255, 0, 1, 0);
    arty_transfer_control(arty_bAddress, 0, 0x40, 11, 11, 2, 1, 0);
    arty_transfer_control(arty_bAddress, 0, 0x40, 0, 1, 0, 1, 0);
    arty_transfer_control(arty_bAddress, 0, 0x40, 0, 2, 0, 1, 0);

   while(1)
   {
     ret = fread(&buffh, sizeof(uint8_t), 1, f);
     if(ret != 1) break;
     lenh = 0x00FF & buffh;
     ret = fread(&buffl, sizeof(uint8_t), 1, f);
     if(ret != 1) break;
     lenl = 0x00FF & buffl;
     len = (lenh << 8) + lenl;
     if(len <= 0) break;
     ret = fread(buf, sizeof(uint8_t), len, f);
     if(ret <= 0) break;
     arty_transfer_data(buf, len, 2);
   }
   fclose(f);

}

static uint8_t asciihex2byte(uint8_t *hex)
{
   uint8_t result;

   result = 0x00;
   if((*hex >= 0x30) && (*hex <= 0x39))
     result = (*hex - 0x30) << 4;
   else
     result = (*hex - 0x37) << 4;

   hex++;

   if((*hex >= 0x30) && (*hex <= 0x39))
     result = result | ((*hex - 0x30) & 0x0F);
   else
     result = result | ((*hex - 0x37) & 0x0F);

   return(result);
}

static void uint2bytes(uint32_t val, uint8_t *bytes)
{
  bytes[0] = 0x000000FF & val;
  bytes[1] = 0x000000FF & (val >> 8);
  bytes[2] = 0x000000FF & (val >> 16);
  bytes[3] = 0x000000FF & (val >> 24);
}

void arty_gpio_uart_riscv_flash(char *filename)
{
  FILE*    inFile;
  uint8_t  inLine[256];
  uint8_t  asciiData[16][3];
  uint8_t  hexAddress[4];
  uint8_t  hexData[16];
  uint32_t currAddress = 0;
  uint32_t nextAddress = 0;
  uint32_t currLine = 0;
#if 0
  // Flip SW0 and SW1 to on
  gpio_set_level(GPIO_OUTPUT_FPGA_SW0, 1);
  gpio_set_level(GPIO_OUTPUT_FPGA_SW1, 1);
  // Delay
  vTaskDelay(500 / portTICK_RATE_MS);
  // Flip SW0 to off
  gpio_set_level(GPIO_OUTPUT_FPGA_SW0, 0);
  // Delay
  vTaskDelay(500 / portTICK_RATE_MS);
#endif
  // Open hex file
  flushUART();
  resetUARTRXData();
  inFile = fopen(filename, "r");
  if(inFile == NULL)
    return;
  // fread record byte at a time until \n
  while(fgets((char *)inLine, sizeof(inLine), inFile)!=NULL)
  {
    currLine++;
    // check if address (starts with @)
    if(inLine[0] == '@')
    {
      uint8_t* linePtr = inLine;
      linePtr++;
      // if so, set currAddress
      if(sscanf((char *)linePtr,"%" SCNu32 "\r\n", &nextAddress)==1)
      {
	currAddress = nextAddress;
        ESP_LOGI("ARTY_FLASH_SOFTCORE", "Got line %" PRIu32 " with currAddress of %" PRIu32 " == %" PRIX32, currLine, currAddress, currAddress);
      }
      else
      {
        ESP_LOGI("ARTY_FLASH_SOFTCORE", "Did not scan currAddress");
	goto clean;
      }
    }
    else
    {
      if(sscanf((char *)inLine, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\r\n", \
		      asciiData[0], asciiData[1], asciiData[2], asciiData[3], \
		      asciiData[4], asciiData[5], asciiData[6], asciiData[7], \
		      asciiData[8], asciiData[9], asciiData[10], asciiData[11], \
		      asciiData[12], asciiData[13], asciiData[14], asciiData[15])==16) 
      {
#if 0 
        ESP_LOGI("ARTY_FLASH_SOFTCORE", "Got line %" PRIu32 " with strings %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", \
		      currLine, \
    	 	      asciiData[0], asciiData[1], asciiData[2], asciiData[3], \
		      asciiData[4], asciiData[5], asciiData[6], asciiData[7], \
		      asciiData[8], asciiData[9], asciiData[10], asciiData[11], \
		      asciiData[12], asciiData[13], asciiData[14], asciiData[15]); 
#endif		      
	for(int j=0; j<4; j++)
	{
          for(int i=0; i<4; i++)
          {
            hexData[i] = asciihex2byte(asciiData[i+j*4]);
          }
          // First convert and send currAddress bytes
          uint2bytes(currAddress, hexAddress);
#if 0 
	  // ESP_LOGI("ARTY_FLASH_SOFTCORE", "Sending to address %02X%02X%02X%02X data %02X %02X %02X %02X", 
	  ESP_LOGI("ARTY_FLASH_SOFTCORE", "Sending to address %" PRIX8 " %" PRIX8 " %" PRIX8 " %" PRIX8 " data %" PRIX8 " %" PRIX8 " %" PRIX8 " %" PRIX8, \
			hexAddress[0], hexAddress[1], hexAddress[2], hexAddress[3], \
			hexData[0], hexData[1], hexData[2], hexData[3]); 
#endif
          sendUARTBytes(hexAddress, 4);
          // Then send the data word as bytes
          sendUARTBytes(hexData, 4); 
          // And increment current address by 4
          currAddress += 4; 
	}
      }
      else
      {
        ESP_LOGI("ARTY_FLASH_SOFTCORE", "Got non-data inLine of %s", inLine);
	goto clean;
      }
    }
  }
clean:  fclose(inFile);
}

void arty_receive_task(void *arg)
{
	uint8_t inbyte;
	while(1)
	{
		    	if(xQueueReceive(in_transfer_queue,&inbyte,portMAX_DELAY))
	{
	  ESP_LOGI("ARTY_TASK", "Something arrived in in_transfer_queue!");
	  uint16_t receivedbytes = read_transfer->actual_num_bytes;
	  ESP_LOGI("ARTY_TASK", "Received this many bytes %" PRIu16 " received", receivedbytes);
	  for(int i=0; i < receivedbytes; i++)
	  {
		ESP_LOGI("ARTY_TASK", "Byte: %" PRIu8 " received", read_transfer->data_buffer[i]);
	  }  
	}
	  ESP_LOGI("ARTY_TASK", "Nothing arrived in in_transfer_queue!");
	}
}

void arty_driver_task(void *arg)
{
    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;
    control_transfer_queue = xQueueCreate(1, sizeof(uint8_t));
    transfer_queue = xQueueCreate(1, sizeof(uint8_t));
    in_transfer_queue = xQueueCreate(1, sizeof(uint8_t));
    
    //uint8_t inbyte;

    //Wait until daemon task has installed USB Host Library
    xSemaphoreTake(signaling_sem, portMAX_DELAY);
#if 0
    xTaskCreatePinnedToCore(arty_receive_task,
                            "arty",
                            1024,
       			    NULL,
                            3,
                            &arty_receive_task_hdl,
                            0);
#endif
    ESP_LOGI(TAG, "Registering Client");
    usb_host_client_config_t client_config = {
        .is_synchronous = false,    //Synchronous clients currently not supported. Set this to false
        .max_num_event_msg = CLIENT_NUM_EVENT_MSG,
        .async = {
            .client_event_callback = client_event_cb,
            .callback_arg = (void *)&driver_obj,
        },
    };
    ESP_ERROR_CHECK(usb_host_client_register(&client_config, &driver_obj.client_hdl));

    usb_host_transfer_alloc(2048, 0, &transfer);
    usb_host_transfer_alloc(2048, 0, &read_transfer);
    read_transfer->num_bytes = 256;
    read_transfer->callback = in_transfer_cb;
    read_transfer->bEndpointAddress = 3;

    while (1) 
    {	
        if (driver_obj.actions == 0) 
        {
            usb_host_client_handle_events(driver_obj.client_hdl, portMAX_DELAY);
        } 
        else 
        {
            if (driver_obj.actions & ACTION_TRANSFER) 
            {   
                ESP_LOGI("class_driver", "ACTION_TRANSFER issuing");           
                usb_host_transfer_submit(transfer);   
                driver_obj.actions &= ~ACTION_TRANSFER;
            }
            if (driver_obj.actions & ACTION_CONTROL_TRANSFER) 
            {
                ESP_LOGI("class_driver", "ACTION_CONTROL_TRANSFER issuing");
                usb_host_transfer_submit_control(driver_obj.client_hdl, transfer);	
                driver_obj.actions &= ~ACTION_CONTROL_TRANSFER;
            }
            if (driver_obj.actions & ACTION_OPEN_DEV) 
            {
                action_open_dev(&driver_obj);
            }
            if (driver_obj.actions & ACTION_GET_DEV_INFO) 
            {
                action_get_info(&driver_obj);
            }
            if (driver_obj.actions & ACTION_GET_DEV_DESC) 
            {
                action_get_dev_desc(&driver_obj);
            }
            if (driver_obj.actions & ACTION_GET_CONFIG_DESC) 
            {
                action_get_config_desc(&driver_obj);
            }
            if (driver_obj.actions & ACTION_GET_STR_DESC) 
            {
                action_get_str_desc(&driver_obj);
    		    transfer->device_handle = driver_obj.dev_hdl;
    		    read_transfer->device_handle = driver_obj.dev_hdl;
                usb_host_interface_claim(driver_obj.client_hdl, driver_obj.dev_hdl, 0, 0);
    		//usb_host_transfer_submit(read_transfer);   
                //xSemaphoreGive(signaling_sem);
            }
            if (driver_obj.actions & ACTION_CLOSE_DEV) 
            {
                ESP_LOGI("class_driver","ACTION_CLOSE_DEV");
                action_close_dev(&driver_obj);
            }
            if (driver_obj.actions & ACTION_EXIT) 
            {
                ESP_LOGI("class_driver","ACTION_EXIT");
                break;
            }
        }
    }

    ESP_LOGI(TAG, "Deregistering Client");
    ESP_ERROR_CHECK(usb_host_client_deregister(driver_obj.client_hdl));

    //Wait to be deleted
    xSemaphoreGive(signaling_sem);
    vTaskSuspend(NULL);
}
