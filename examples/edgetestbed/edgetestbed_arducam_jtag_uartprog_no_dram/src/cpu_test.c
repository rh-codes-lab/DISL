#include <stdarg.h> 
#include <stdint.h>
#include <stddef.h>
#include "utils.h"
#include "arducam_ov2640.h"

int main( )   
{ 
  gpio = 0xFFFFFFFC; // turn off the LEDs
  uint8_t threshold = 100;       
  while(1){
    capture_and_transmit(RGB565,OV2640_320x240,200,Auto,Saturation0,Brightness0,Contrast0,Normal);
    //capture_and_transmit(BINARY,OV2640_320x240,220,Auto,Saturation0,Brightness0,Contrast0,Normal);
    //capture_and_transmit(EDGE_SW,OV2640_320x240,threshold,Office,Saturation0,Brightness0,Contrast0,Normal);
    //capture_and_transmit(EDGE_HW,OV2640_320x240,threshold,Office,Saturation0,Brightness0,Contrast0,Normal); 
    //capture_and_transmit(EDGE_HW_BINARY,OV2640_320x240,threshold,Office,Saturation0,Brightness0,Contrast0,Normal); 
  } 
  while(1);  
}    



