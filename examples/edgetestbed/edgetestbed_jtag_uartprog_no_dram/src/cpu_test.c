#include <stdarg.h> 
#include <stdint.h>
#include <stddef.h>
#include "utils.h"

#define BME280_STANDBY_TIME_500_MS                (0x04)
#define BME280_CONCAT_BYTES(msb, lsb)             (((uint16_t)msb << 8) | (uint16_t)lsb)
#define BME280_12_BIT_SHIFT                       12
#define BME280_8_BIT_SHIFT                        8
#define BME280_4_BIT_SHIFT                        4
#define BME280_ADDRESS (0x76 << 1)

int main( )
{
  uint8_t data = 0;
  int count = 0;

  uint8_t digT1msb, digT1lsb, digT2msb, digT2lsb, digT3msb, digT3lsb;
  uint16_t digT1;
  int16_t digT2, digT3;

  uint8_t temp[3]; 
  uint32_t uncomp_temp_reading;
  int32_t comp_temp_reading;
  int32_t var1; 
  int32_t var2;
  int32_t temp_comp;

  int32_t tmin = -4000;
  int32_t tmax = 8500;
  int32_t t_fine = 0;

  uint32_t dataXLSB, dataLSB, dataMSB; 
  uint8_t ctrlSettings = 0x00;

  i2cbus = (0 << 16) | (0x88 << 8) | BME280_ADDRESS | 1;
  digT1lsb = i2cbus & 0xFF;
  i2cbus = (0 << 16) | (0x89 << 8) | BME280_ADDRESS | 1; 
  digT1msb = i2cbus & 0xFF;
  digT1 = (int16_t)BME280_CONCAT_BYTES(digT1msb, digT1lsb); 
  
  i2cbus = (0 << 16) | (0x8A << 8) | BME280_ADDRESS | 1;
  digT2lsb = i2cbus & 0xFF;
  i2cbus = (0 << 16) | (0x8B << 8) | BME280_ADDRESS | 1; 
  digT2msb = i2cbus & 0xFF;
  digT2 = (int16_t)BME280_CONCAT_BYTES(digT2msb, digT2lsb); 

  i2cbus = (0 << 16) | (0x8C << 8) | BME280_ADDRESS | 1;
  digT3lsb = i2cbus & 0xFF;
  i2cbus = (0 << 16) | (0x8D << 8) | BME280_ADDRESS | 1; 
  digT3msb = i2cbus & 0xFF;
  digT3 = (int16_t)BME280_CONCAT_BYTES(digT3msb, digT3lsb); 

  i2cbus = (0 << 16) | (0xD0 << 8) | BME280_ADDRESS | 1;
  data = i2cbus & 0xFF;
 
  if(data != 0x60)
  {
   printf("Error, ID %d not equal to BME280 0x60!\r\n", data); 
  }
  i2cbus = (0 << 16) | (0xF4 << 8) | BME280_ADDRESS | 1;
  ctrlSettings = i2cbus & 0xFF;
  ctrlSettings |= 0x23;
  i2cbus = (ctrlSettings << 16) | (0xF4 << 8) | BME280_ADDRESS | 0;
  i2cbus = (0x60 << 16) | (0xF5 << 8) | BME280_ADDRESS | 0;

  while (1)
  {
    i2cbus = (0 << 16) | (0xFA << 8) | BME280_ADDRESS | 1;
    temp[0] = i2cbus & 0xFF; 
    dataMSB = (uint32_t)temp[0] << BME280_12_BIT_SHIFT;
  
    i2cbus = (0 << 16) | (0xFB << 8) | BME280_ADDRESS | 1;
    temp[1] = i2cbus & 0xFF;
    dataLSB = (uint32_t)temp[1] << BME280_4_BIT_SHIFT;
  
    i2cbus = (0 << 16) | (0xFC << 8) | BME280_ADDRESS | 1;
    temp[2] = i2cbus & 0xFF;
    dataXLSB = (uint32_t)temp[2] >> BME280_4_BIT_SHIFT;
  
    uncomp_temp_reading = dataMSB | dataLSB | dataXLSB;
    comp_temp_reading = 0;
  
    var1 = (int32_t)((uncomp_temp_reading / 8) - (digT1 * 2));
    var1 = (var1 * ((int32_t)digT2)) / 2048;
  
    var2 = (int32_t)((uncomp_temp_reading / 16) - ((int32_t)digT1));
    var2 = (((var2 * var2) / 4096) * ((int32_t)digT3)) / 16384;
  
    t_fine = var1 + var2;
    comp_temp_reading = (t_fine * 5 + 128) / 256;
 
    if (comp_temp_reading < tmin)
    {
        comp_temp_reading = tmin;
    }
    else if (comp_temp_reading > tmax)
    {
        comp_temp_reading = tmax;
    }

    printf("Temperature: %d.%dC\n\r",(int)comp_temp_reading/100,comp_temp_reading - ((int)(comp_temp_reading/100)*100 ));
    int start = timer;
    while (timer-start < 500000);
  }
}


