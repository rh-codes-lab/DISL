#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#include "ssd1306.h"
#include "font8x8_basic.h"

#define SDA_PIN CONFIG_OLED_GPIO_SDA //GPIO_NUM_15
#define SCL_PIN CONFIG_OLED_GPIO_SCL //GPIO_NUM_2

#define tag "SSD1306"

esp_err_t i2c_master_init(void)
{
	i2c_config_t i2c_config; 
	i2c_config.mode = I2C_MODE_MASTER;
	i2c_config.sda_io_num = SDA_PIN;
	i2c_config.scl_io_num = SCL_PIN;
	i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
	i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
	i2c_config.master.clk_speed = 400000;
	i2c_config.clk_flags = 0;
	i2c_param_config(I2C_NUM_0, &i2c_config);
	return(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
}

void ssd1306SendCmd(uint8_t command)
{

	esp_err_t espRc;
	
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd,0x00, true);
	i2c_master_write_byte(cmd,command, true);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	if (espRc != ESP_OK)
	{
		ESP_LOGE(tag, "Cmd send failed code: 0x%.2X", espRc);
	}
	i2c_cmd_link_delete(cmd);
}

void ssd1306SendData(uint8_t *data,uint16_t size)
{
	esp_err_t espRc;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd,0x40, true);
	i2c_master_write(cmd,data,size, true);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	if (espRc != ESP_OK)
	{
		ESP_LOGE(tag, "Data send failed code: 0x%.2X", espRc);
	}
	i2c_cmd_link_delete(cmd);

}

void ssd1306_init(void) 
{

	ssd1306SendCmd(0xAE);	//Turn the OLED panel display OFF.

	ssd1306SendCmd(0x20);	//Set Memory Addressing Mode as
	ssd1306SendCmd(0x00);	//Horizontal Addressing Mode

	ssd1306SendCmd(0xB0);	//Page Start Address for Page Addressing Mode

	ssd1306SendCmd(0xC8);	//COM Output Scan Direction as normal

	ssd1306SendCmd(0x00);	//Lower Column Start Address for Page Addressing Mode
	ssd1306SendCmd(0x10);	//Higher Column Start Address for Page Addressing Mode

	ssd1306SendCmd(0x40);	//Display Start Line 0 - 63

	ssd1306SendCmd(0x81);	//Contrast Control
	ssd1306SendCmd(0xFF);	//256

	ssd1306SendCmd(0xA1);	//Segment Re-map - column address 127 is mapped to SEG0

	ssd1306SendCmd(0xA6);	//Normal display

	ssd1306SendCmd(0xA8);	//Multiplex Ratio
	ssd1306SendCmd(0x3F);	// 64MUX

	ssd1306SendCmd(0xA4);	//Entire Display ON - Resume to RAM content display

	ssd1306SendCmd(0xD3);	//Display Offset
	ssd1306SendCmd(0x00);	//Set vertical shift by COM from 0d~63d

	ssd1306SendCmd(0xD5);	//Display Clock Divide Ratio/Oscillator Frequency
	ssd1306SendCmd(0xF0);

	ssd1306SendCmd(0xD9);	//Pre-charge Period
	ssd1306SendCmd(0x22);


	ssd1306SendCmd(0xDA);	//COM Pins Hardware Configuration
	ssd1306SendCmd(0x12);	//Alternative

	ssd1306SendCmd(0xDB);	//VCOMH Deselect Level
	ssd1306SendCmd(0x20);	//0.77 x VCC

	ssd1306SendCmd(0x8D);	//Charge Pump Setting
	ssd1306SendCmd(0x14);	//Enable charge pump during display on
	ssd1306SendCmd(0xAF);	//Turn the OLED panel display ON.


}

void ssd1306DisplayClear(void)
{
	uint8_t arr[1024];

	for(int i = 0; i < 1024; i++)
	{
		arr[i] = 0x00;
	}

	for(int page = 0; page < 8; page++)
	{
		ssd1306SendCmd(0xB0 + page);
		ssd1306SendCmd(0x00);
		ssd1306SendCmd(0x10);
		ssd1306SendData(&arr[128 * page],128);
	}

}

void ssd1306FillDisplay(void)
{
	uint8_t arr[1024];

	for(int i = 0; i < 1024; i++)
	{
		arr[i] = 0xFF;
	}

	for(int page = 0; page < 8; page++)
	{
		ssd1306SendCmd(0xB0 + page);
		ssd1306SendCmd(0x00);
		ssd1306SendCmd(0x10);
		ssd1306SendData(&arr[128 * page],128);
	}

}

void ssd1306SendChar(char ch)
{
	ssd1306SendData(font8x8_basic_tr[(uint8_t) ch],8);
}


void ssd1306_display_text(char *str)
{

	uint8_t column = 0,page = 0;
	ssd1306SendCmd(0xB0 + page++);
	ssd1306SendCmd(0x00);
	ssd1306SendCmd(0x10);

	for(uint8_t index = 0; index < strlen(str); index++)
	{
		if(column == 16 || (str[index] == '\n' && page != 0))
		{
			ssd1306SendCmd(0xB0 + page++);
			ssd1306SendCmd(0x00);
			ssd1306SendCmd(0x10);
			column = 0;
		}
		if(column < 16 && str[index] != '\n')
		{
			ssd1306SendChar(str[index]);
			column++;
		}
	}
}

