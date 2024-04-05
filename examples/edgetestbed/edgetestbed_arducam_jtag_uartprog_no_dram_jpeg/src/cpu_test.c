#include <stdarg.h> 
#include <stdint.h>
#include <stddef.h>
#include "utils.h"
#include "arducam_ov2640.h"

int main( )   
{ 
  gpio = 0xFFFFFFFC; // turn off the LEDs 
  uint32_t threshold =50;    
  uint32_t motion_threshold = 1200;
  InitCAM(JPEG);
  clear_fifo_flag();
  write_reg(ARDUCHIP_FRAMES,0x00);
  set_Light_Mode(Office);
  set_Color_Saturation(Saturation0); 
  set_Brightness(Brightness0);
  set_Contrast(Contrast0);
  set_Special_effects(Normal);  
  set_JPEG_size(OV2640_320x240); 
  uint32_t check;
  while(1){   
    check = capture_and_transmit(threshold);
  }
while(1); 
}    



