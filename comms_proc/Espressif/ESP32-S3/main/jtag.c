#include <stdio.h>
#include <sys/stat.h>
#include <stdint.h>
#include <inttypes.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "jtag.h"

#define FILE_BUFFER_SIZE 2048
#define ADDRESS_MAX 

void jtag_statemove(tap_state_t state)
{
  struct jtag_command cmd;
  cmd.type = JTAG_STATEMOVE; 
  cmd.end_state = state;
  cmd.in_buffer = NULL;
  cmd.ir_scan = 0;
  cmd.out_buffer = NULL;
  ftdi_execute_command(cmd); 
}

void jtag_reset()
{
  jtag_statemove(TAP_RESET);
}

void jtag_idle()
{
  jtag_statemove(TAP_IDLE);
}
 
void jtag_irscan_bits(uint16_t num_bits, uint8_t val)
{
  uint8_t _val = val;
  struct jtag_command cmd;
  cmd.type = JTAG_SCAN; 
  cmd.ir_scan = 1;
  cmd.num_bits = num_bits;
  cmd.out_buffer = &_val;
  cmd.in_buffer = NULL;
  cmd.end_state = TAP_IDLE;
  ftdi_execute_command(cmd);
}

void jtag_irscan_bits_reset(uint16_t num_bits, uint8_t val)
{
  uint8_t _val = val;
  struct jtag_command cmd;
  cmd.type = JTAG_SCAN; 
  cmd.ir_scan = 1;
  cmd.num_bits = num_bits;
  cmd.out_buffer = &_val;
  cmd.in_buffer = NULL;
  cmd.end_state = TAP_RESET;
  ftdi_execute_command(cmd);
}

void jtag_irscan_bits_irpause(uint16_t num_bits, uint8_t val)
{
  uint8_t _val = val;
  struct jtag_command cmd;
  cmd.type = JTAG_SCAN; 
  cmd.ir_scan = 1;
  cmd.num_bits = num_bits;
  cmd.out_buffer = &_val;
  cmd.in_buffer = NULL;
  cmd.end_state = TAP_IRPAUSE;
  ftdi_execute_command(cmd);
}
      
void jtag_irscan_bits_hold(uint16_t num_bits, uint8_t val)
{
  uint8_t _val = val;
  struct jtag_command cmd;
  cmd.type = JTAG_SCAN; 
  cmd.ir_scan = 1;
  cmd.num_bits = num_bits;
  cmd.out_buffer = &_val;
  cmd.in_buffer = NULL;
  cmd.end_state = TAP_IRSHIFT;
  ftdi_execute_command(cmd);
}

void jtag_drscan_bits(uint16_t num_bits, uint8_t val)
{
  uint8_t _val = val;
  struct jtag_command cmd;
  cmd.type = JTAG_SCAN; 
  cmd.ir_scan = 0;
  cmd.num_bits = num_bits;
  cmd.out_buffer = &_val;
  cmd.in_buffer = NULL;
  cmd.end_state = TAP_IDLE;
  ftdi_execute_command(cmd);
}

void jtag_drscan_bytes(uint8_t* wbuf, uint16_t len)
{
  struct jtag_command cmd;
  cmd.type = JTAG_SCAN; 
  cmd.ir_scan = 0;
  cmd.num_bits = len*8;
  cmd.out_buffer = wbuf;
  cmd.in_buffer = NULL;
  cmd.end_state = TAP_DRPAUSE;
  ftdi_execute_command(cmd);
}
        
void jtag_drscan_bytes_hold(uint8_t* wbuf, uint16_t len)
{
  struct jtag_command cmd;
  cmd.type = JTAG_SCAN; 
  cmd.ir_scan = 0;
  cmd.num_bits = len*8;
  cmd.out_buffer = wbuf;
  cmd.in_buffer = NULL;
  cmd.end_state = TAP_DRSHIFT;
  ftdi_execute_command(cmd);
}

void jtag_drscan_bytes_read(uint8_t* wbuf, uint8_t* rbuf, uint16_t len)
{
  struct jtag_command cmd;
  cmd.type = JTAG_SCAN; 
  cmd.ir_scan = 0;
  cmd.num_bits = len*8;
  cmd.out_buffer = wbuf;
  cmd.in_buffer = rbuf;
  cmd.end_state = TAP_DRPAUSE;
  ESP_LOGI("JTAG_AXI_STATUS", "FTDI Command Execution Starting");
  ftdi_execute_command(cmd);
  ESP_LOGI("JTAG_DRSCAN_BYTES_READ", "FTDI Command Execution Complete");
}

void jtag_program(char* filename)
{
  struct stat st;

  if(stat(filename, &st)!=0)
  {
    return;
  }

  FILE *f = fopen(filename, "rb");
  uint32_t filesize = st.st_size;
  uint32_t bytes_to_read = 0;
  uint8_t buf[128];
  uint8_t CFG_IN = 0x05;
  uint8_t JPROGRAM = 0x0B;
  uint8_t JSTART = 0x0C;
  uint16_t i = 0;
  uint16_t j = 0;
  uint16_t k = 0;
  uint8_t filebuf[FILE_BUFFER_SIZE];

  ftdi_mpsse_open();	
  jtag_reset();
  jtag_idle();
  jtag_irscan_bits_reset(6, JPROGRAM);
  jtag_irscan_bits_irpause(6, CFG_IN);	

  while(filesize > 0)
  {
    bytes_to_read = (filesize >= FILE_BUFFER_SIZE)?(FILE_BUFFER_SIZE):filesize;
    j = fread(&filebuf, sizeof(uint8_t), bytes_to_read, f);
    filesize -= j;     
    i  = 0;

    for (k = 0; k < j; k++)
    {
      uint8_t byte = filebuf[k];
      byte =  (((byte>>0)&1)<<7) | (((byte>>1)&1)<<6) | (((byte>>2)&1)<<5) | (((byte>>3)&1)<<4) | (((byte>>4)&1)<<3) | (((byte>>5)&1)<<2) | (((byte>>6)&1)<<1) | (((byte>>7)&1)<<0);
      buf[i] = byte;
      i++;
      if (i == 32)
      {
        jtag_drscan_bytes_hold(buf,32);
        i = 0;
      }
    }
    if (i > 0)
    {
      jtag_drscan_bytes_hold(buf,i);
    }
  }
  jtag_irscan_bits(6, JSTART);
  jtag_reset();
  jtag_reset();
}  
  
void jtag_axi_write(uint32_t address, uint32_t data)
{
      uint8_t buf[13] = 	{0,0,0,0, \
	      			(uint8_t)(address&255),(uint8_t)((address>>8)&255), \
				(uint8_t)((address>>16)&255),(uint8_t)((address>>24)&255), \
				(uint8_t)(data&255),(uint8_t)((data>>8)&255), \
				(uint8_t)((data>>16)&255),(uint8_t)((data>>24)&255),10};
      jtag_irscan_bits(6,0x23);
      jtag_drscan_bytes(buf,13);
      jtag_idle();
}
      
void jtag_control_write(uint32_t control_0_31, uint32_t control_32_63, uint32_t control_64_95)
{
  	uint8_t buf[13] = {(uint8_t)((control_0_31>>0)&255),(uint8_t)((control_0_31>>8)&255),(uint8_t)((control_0_31>>16)&255), \
		(uint8_t)((control_0_31>>24)&255),(uint8_t)((control_32_63>>0)&255),(uint8_t)((control_32_63>>8)&255), \
		(uint8_t)((control_32_63>>16)&255),(uint8_t)((control_32_63>>24)&255),(uint8_t)((control_64_95>>0)&255), \
		(uint8_t)((control_64_95>>8)&255),(uint8_t)((control_64_95>>16)&255),(uint8_t)((control_64_95>>24)&255),32}; 
      jtag_irscan_bits(6,0x23);
      jtag_drscan_bytes(buf,13);
      jtag_idle();
}

void jtag_axi_write_noirscan(uint32_t address, uint32_t data)
{
      uint8_t buf[13] = {0,0,0,0, \
	      			(uint8_t)(address&255),(uint8_t)((address>>8)&255),(uint8_t)((address>>16)&255), \
				(uint8_t)((address>>24)&255),(uint8_t)(data&255),(uint8_t)((data>>8)&255), \
				(uint8_t)((data>>16)&255),(uint8_t)((data>>24)&255),10};
      jtag_drscan_bytes(buf,13);
      jtag_idle();
}

uint32_t jtag_axi_status()
{
      ESP_LOGI("JTAG_AXI_STATUS", "IR Scan starting");
      jtag_irscan_bits(6,0x23);
      ESP_LOGI("JTAG_AXI_STATUS", "IR Scan complete");
      uint8_t buf[13] = {0,0,0,0,0,0,0,0,0,0,0,0,16};
      uint8_t buf2[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
      ESP_LOGI("JTAG_AXI_STATUS", "DR Scan starting");
      jtag_drscan_bytes(buf, 13);
      ESP_LOGI("JTAG_AXI_STATUS", "DR Scan complete");
      jtag_idle();
      ESP_LOGI("JTAG_AXI_STATUS", "DR Scan bytes starting");
      jtag_drscan_bytes_read(buf2, buf2, 13);
      ESP_LOGI("JTAG_AXI_STATUS", "DR Scan bytes complete");
      jtag_idle();
      return (((buf2[5] & 0xff) << 8) | (buf2[4] & 0xff));
}
      
uint32_t jtag_axi_read(uint32_t address)
{
      uint8_t buf_0[13] = 	{(uint8_t)(address&255),(uint8_t)((address>>8)&255),(uint8_t)((address>>16)&255), \
	      			(uint8_t)((address>>24)&255),0,0,0,0,0,0,0,0,1};
      uint8_t buf_1[13] = {0,0,0,0,0,0,0,0,0,0,0,0,4};
      uint8_t buf_2[13] = {0,0,0,0,0,0,0,0,0,0,0,0,16};
      uint8_t buf_3[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
      jtag_irscan_bits(6,0x23);
      jtag_drscan_bytes(buf_0, 13);
      jtag_idle();
      uint32_t status = 0;
      while (!((status >> 2) & 1)){
	  ESP_LOGI("JTAG_AXI_READ", "Waiting for status");
          status = jtag_axi_status();
      }
      ESP_LOGI("JTAG_AXI_READ", "Status reached");
      jtag_drscan_bytes(buf_1, 13);
      jtag_idle();
      jtag_drscan_bytes(buf_2, 13);
      jtag_idle();
      jtag_drscan_bytes_read(buf_3, buf_3, 13);
      jtag_idle();
      uint32_t data = buf_3[0] + (buf_3[1] << 8) + (buf_3[2] << 16) + (buf_3[3] << 24);

      return (data);
}

bool isHex(uint32_t c)
{
  if (((48 <= c) && (c <= 57)) || ((65 <= c) && (c <= 70)))
    return true;
  else
    return false;
}

uint32_t hex2dec(uint32_t c)
{
  if ((48 <= c) && (c <= 57))
    return c-48;
  else 
    return c-55;
}

void jtag_uart_tx_test()
{
  uint8_t testString[13] = "Loopback19063";
  uint8_t *testStringPtr = testString;

  ftdi_uart_configure(8,1000000, XON_XOFF, 30000000);
  while(1)
  {
    ESP_LOGI("JTAG", "TX test about to write");
    for(int i=0; i<13; i++)
    {
      ftdi_uart_write(testStringPtr, 1);
      testStringPtr++;
    }
    ESP_LOGI("JTAG", "TX test delaying 2 seconds");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  } 
}

void jtag_uart_loopback_test()
{
  uint8_t testString[13] = "Loopback19063";
  uint8_t *testStringPtr = testString;
  uint8_t ibuf[256]; 
  uint16_t recv;

  ftdi_uart_configure(8,1000000, XON_XOFF, 30000000);
  ESP_LOGI("JTAG", "Loopback test about to write");
  for(int i=0; i<13; i++)
  {
    ftdi_uart_write(testStringPtr, 1);
    testStringPtr++;
  } 
  //ESP_LOGI("JTAG", "Loopback test about to read");
  //recv=ftdi_uart_read(ibuf);
  //ESP_LOGI("JTAG","Loopback received bytes %" PRIu16, recv);
}

void jtag_program_softcore(char* filename)
{
  struct stat st;

  if(stat(filename, &st)!=0)
  {
    return;
  }
  FILE *f = fopen(filename, "r");

  if(f == NULL)
  {
    return;
  }
  uint32_t address = 0;
  uint32_t data = 0;
  uint32_t counter = 0;
  int byte_counter = 0;
  int half_byte_counter = 0;
  bool update_address = false;
  size_t ret = 0;
  uint8_t byte = 0;
  ret = fread(&byte, sizeof(uint8_t), 1, f); 

  while(ret == 1)
  {
    if (byte == 64)
    {
      update_address = true;
      address = 0;
    }
    else if (update_address && !isHex(byte))
    {
      update_address = false;
      half_byte_counter = 0;
    }
    if (isHex(byte))
    {
      if (update_address)
      {
        byte = hex2dec(byte);
        address = address + (byte << (28 - (4*half_byte_counter)));
        half_byte_counter++;
      }
      else
      {
        byte = hex2dec(byte);
        if (byte_counter == 0 && half_byte_counter==0)
	{
          data = byte << 4;
          half_byte_counter = 1;
        }
        else
	{
          uint32_t actual_byte = byte;
          if (half_byte_counter == 0)
	  {
            actual_byte = actual_byte << 4;
          }
          for (int i=0;i<byte_counter;i++){
            actual_byte = actual_byte << 8;
          }
          data = data + actual_byte;
          byte_counter+= half_byte_counter;
          half_byte_counter = half_byte_counter ? 0 : 1;
          if (byte_counter == 4)
	  {
            byte_counter = 0;
            uint8_t x1 = address&255;
            uint8_t x2 = (address>>8)&255;
            uint8_t x3 = (address>>16)&255;
            uint8_t x4 = (address>>24)&255;
            uint8_t x5 = data&255;
            uint8_t x6 = (data>>8)&255;
            uint8_t x7 = (data>>16)&255;
            uint8_t x8 = (data>>24)&255;
            ftdi_uart_write(&x1,1);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            ftdi_uart_write(&x2,1);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            ftdi_uart_write(&x3,1);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            ftdi_uart_write(&x4,1);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            ftdi_uart_write(&x5,1);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            ftdi_uart_write(&x6,1);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            ftdi_uart_write(&x7,1);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            ftdi_uart_write(&x8,1);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            address+=4;
            counter++;
            if (counter == 2445)
            {
	      fclose(f);
              return;
            }
          }
        }
      }
    }
    ret = fread(&byte, sizeof(uint8_t), 1, f); 
  }
  fclose(f);
}

void jtag_verify_softcore(char* filename)
{
  struct stat st;
  if(stat(filename, &st)!=0)
  {
    ESP_LOGI("JTAG_VERIFY_SOFTCORE","Stat failed");
    return;
  }
  FILE *f = fopen(filename, "r");
  if(f == NULL)
  {
    ESP_LOGI("JTAG_VERIFY_SOFTCORE","fopen Failed");
    return;
  }
  uint32_t address = 0;
  uint32_t data = 0;
  int byte_counter = 0;
  int half_byte_counter = 0;
  bool update_address = false;
  size_t ret = 0;
  uint8_t byte;  
  ret = fread(&byte, sizeof(uint8_t), 1, f); 
  
  while(ret == 1)
  {  
    if (byte == 64)
    {
      update_address = true;
      address = 0;
    }
    else if (update_address && !isHex(byte))
    {
      update_address = false;
      half_byte_counter = 0;
    }
    if (isHex(byte)){
      if (update_address){
        byte = hex2dec(byte);
        address = address + (byte << (28 - (4*half_byte_counter)));
        half_byte_counter++;
      }
      else{
        byte = hex2dec(byte);
        if (byte_counter == 0 && half_byte_counter==0){
          data = byte << 4;
          half_byte_counter = 1;
        }
        else
	{
          uint32_t actual_byte = byte;
          if (half_byte_counter == 0)
	  {
            actual_byte = actual_byte << 4;
          }
          for (int i=0;i<byte_counter;i++)
	  {
            actual_byte = actual_byte << 8;
          }
          data = data + actual_byte;
          byte_counter+= half_byte_counter;
          half_byte_counter = half_byte_counter ? 0 : 1;
          if (byte_counter == 4)
	  {
            byte_counter = 0;
            uint32_t rdata = jtag_axi_read(address);
            // addback formatting, ESP-IDF 5.0+ complains about format specifiers 
	    ESP_LOGI("JTAG", "%" PRIX32 ": %" PRIX32 " - %" PRIX32,address, data, rdata);
//	    ESP_LOGI("JTAG", "%08X: %08X - %08X",address, data, rdata); 
	    vTaskDelay(1 / portTICK_PERIOD_MS);
            if (address >= 9616)
	    {
	      fclose(f);
              return;
	    }
            address+=4;
          }
        }
      }
    }
    ret = fread(&byte, sizeof(uint8_t), 1, f);     
  }
  fclose(f);
}
