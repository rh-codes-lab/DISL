// The MIT License (MIT)

// Copyright (c) 2015 Lee

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

#ifndef ARDUCAM_OV2640_H
#define ARDUCAM_OV2640_H
#include "utils.h"

typedef enum img_format {
  GRAY = 0x0,
  JPEG = 0x1,
  RGB565 = 0x2,
  BINARY = 0x3,
  EDGE_SW = 0x4,
  EDGE_HW = 0x5,
  EDGE_HW_BINARY = 0x6
} img_format_t;

struct sensor_reg {
  uint16_t reg;
  uint16_t val;
};


#define ARDUCHIP_FRAMES       0x01
#define ARDUCAM_ADDRESS (0x30 << 1)
#define pgm_read_word(x)        ( ((*((unsigned char *)x + 1)) << 8) + (*((unsigned char *)x)))
#define OV2640_160x120    0 //160x120
#define OV2640_176x144    1 //176x144
#define OV2640_320x240    2 //320x240
#define OV2640_352x288    3 //352x288
#define OV2640_640x480    4 //640x480
#define OV2640_800x600    5 //800x600
#define OV2640_1024x768   6 //1024x768
#define OV2640_1280x1024  7 //1280x1024
#define OV2640_1600x1200  8 //1600x1200
//Light Mode
#define Auto                 0
#define Sunny                1
#define Cloudy               2
#define Office               3
#define Home                 4
#define Advanced_AWB         0
#define Simple_AWB           1
#define Manual_day           2
#define Manual_A             3
#define Manual_cwf           4
#define Manual_cloudy        5
//Color Saturation 
#define Saturation4          0
#define Saturation3          1
#define Saturation2          2
#define Saturation1          3
#define Saturation0          4
#define Saturation_1         5
#define Saturation_2         6
#define Saturation_3         7
#define Saturation_4         8
//Brightness
#define Brightness4          0
#define Brightness3          1
#define Brightness2          2
#define Brightness1          3
#define Brightness0          4
#define Brightness_1         5
#define Brightness_2         6
#define Brightness_3         7
#define Brightness_4         8
//Contrast
#define Contrast4            0
#define Contrast3            1
#define Contrast2            2
#define Contrast1            3
#define Contrast0            4
#define Contrast_1           5
#define Contrast_2           6
#define Contrast_3           7
#define Contrast_4           8
#define degree_180            0
#define degree_150            1
#define degree_120            2
#define degree_90             3
#define degree_60             4
#define degree_30             5
#define degree_0              6
#define degree30              7
#define degree60              8
#define degree90              9
#define degree120             10
#define degree150             11
//Special effects
#define Antique                      0
#define Bluish                       1
#define Greenish                     2
#define Reddish                      3
#define BW                           4
#define Negative                     5
#define BWnegative                   6
#define Normal                       7
#define Sepia                        8
#define Overexposure                 9
#define Solarize                     10
#define  Blueish                     11
#define Yellowish                    12
#define Exposure_17_EV                    0
#define Exposure_13_EV                    1
#define Exposure_10_EV                    2
#define Exposure_07_EV                    3
#define Exposure_03_EV                    4
#define Exposure_default                  5
#define Exposure03_EV                     6
#define Exposure07_EV                     7
#define Exposure10_EV                     8
#define Exposure13_EV                     9
#define Exposure17_EV                     10
#define Auto_Sharpness_default              0
#define Auto_Sharpness1                     1
#define Auto_Sharpness2                     2
#define Manual_Sharpnessoff                 3
#define Manual_Sharpness1                   4
#define Manual_Sharpness2                   5
#define Manual_Sharpness3                   6
#define Manual_Sharpness4                   7
#define Manual_Sharpness5                   8
#define Sharpness1                         0
#define Sharpness2                         1
#define Sharpness3                         2
#define Sharpness4                         3
#define Sharpness5                         4
#define Sharpness6                         5
#define Sharpness7                         6
#define Sharpness8                         7
#define Sharpness_auto                       8
#define EV3                                 0
#define EV2                                 1
#define EV1                                 2
#define EV0                                 3
#define EV_1                                4
#define EV_2                                5
#define EV_3                                6
#define MIRROR                              0
#define FLIP                                1
#define MIRROR_FLIP                         2
#define high_quality                         0
#define default_quality                      1
#define low_quality                          2
#define Color_bar                      0
#define Color_square                   1
#define BW_square                      2
#define DLI                            3
#define Night_Mode_On                  0
#define Night_Mode_Off                 1
#define Off                            0
#define Manual_50HZ                    1
#define Manual_60HZ                    2
#define Auto_Detection                 3
#define BURST_FIFO_READ     0x3C  //Burst FIFO read operation
#define SINGLE_FIFO_READ    0x3D  //Single FIFO read operation
#define ARDUCHIP_REV          0x40  //ArduCHIP revision
#define VER_LOW_MASK          0x3F
#define VER_HIGH_MASK         0xC0
#define ARDUCHIP_TRIG         0x41  //Trigger source
#define VSYNC_MASK            0x01
#define SHUTTER_MASK          0x02
#define CAP_DONE_MASK         0x08
#define FIFO_SIZE1        0x42  //Camera write FIFO size[7:0] for burst to read
#define FIFO_SIZE2        0x43  //Camera write FIFO size[15:8]
#define FIFO_SIZE3        0x44  //Camera write FIFO size[18:16]
#define OV2640_CHIPID_HIGH  0x0A
#define OV2640_CHIPID_LOW   0x0B
#define ARDUCHIP_FIFO         0x04  //FIFO and I2C control
#define FIFO_CLEAR_MASK       0x01
#define FIFO_START_MASK       0x02
#define FIFO_RDPTR_RST_MASK     0x10
#define FIFO_WRPTR_RST_MASK     0x20

const struct sensor_reg OV2640_QVGA[]  =
{
  {0xff, 0x0}, 
  {0x2c, 0xff}, 
  {0x2e, 0xdf}, 
  {0xff, 0x1}, 
  {0x3c, 0x32}, 
  {0x11, 0x0}, 
  {0x9, 0x2}, 
  {0x4, 0xa8}, 
  {0x13, 0xe5}, 
  {0x14, 0x48}, 
  {0x2c, 0xc}, 
  {0x33, 0x78}, 
  {0x3a, 0x33}, 
  {0x3b, 0xfb}, 
  {0x3e, 0x0}, 
  {0x43, 0x11}, 
  {0x16, 0x10}, 
  {0x39, 0x2}, 
  {0x35, 0x88}, 

  {0x22, 0xa}, 
  {0x37, 0x40}, 
  {0x23, 0x0}, 
  {0x34, 0xa0}, 
  {0x6, 0x2}, 
  {0x6, 0x88}, 
  {0x7, 0xc0}, 
  {0xd, 0xb7}, 
  {0xe, 0x1}, 
  {0x4c, 0x0}, 
  {0x4a, 0x81}, 
  {0x21, 0x99}, 
  {0x24, 0x40}, 
  {0x25, 0x38}, 
  {0x26, 0x82}, 
  {0x5c, 0x0}, 
  {0x63, 0x0}, 
  {0x46, 0x22}, 
  {0xc, 0x3a}, 
  {0x5d, 0x55}, 
  {0x5e, 0x7d}, 
  {0x5f, 0x7d}, 
  {0x60, 0x55}, 
  {0x61, 0x70}, 
  {0x62, 0x80}, 
  {0x7c, 0x5}, 
  {0x20, 0x80}, 
  {0x28, 0x30}, 
  {0x6c, 0x0}, 
  {0x6d, 0x80}, 
  {0x6e, 0x0}, 
  {0x70, 0x2}, 
  {0x71, 0x94}, 
  {0x73, 0xc1}, 
  {0x3d, 0x34}, 
  {0x12, 0x4}, 
  {0x5a, 0x57}, 
  {0x4f, 0xbb}, 
  {0x50, 0x9c}, 
  {0xff, 0x0}, 
  {0xe5, 0x7f}, 
  {0xf9, 0xc0}, 
  {0x41, 0x24}, 
  {0xe0, 0x14}, 
  {0x76, 0xff}, 
  {0x33, 0xa0}, 
  {0x42, 0x20}, 
  {0x43, 0x18}, 
  {0x4c, 0x0}, 
  {0x87, 0xd0}, 
  {0x88, 0x3f}, 
  {0xd7, 0x3}, 
  {0xd9, 0x10}, 
  {0xd3, 0x82}, 
  {0xc8, 0x8}, 
  {0xc9, 0x80}, 
  {0x7c, 0x0}, 
  {0x7d, 0x0}, 
  {0x7c, 0x3}, 
  {0x7d, 0x48}, 
  {0x7d, 0x48}, 
  {0x7c, 0x8}, 
  {0x7d, 0x20}, 
  {0x7d, 0x10}, 
  {0x7d, 0xe}, 
  {0x90, 0x0}, 
  {0x91, 0xe}, 
  {0x91, 0x1a}, 
  {0x91, 0x31}, 
  {0x91, 0x5a}, 
  {0x91, 0x69}, 
  {0x91, 0x75}, 
  {0x91, 0x7e}, 
  {0x91, 0x88}, 
  {0x91, 0x8f}, 
  {0x91, 0x96}, 
  {0x91, 0xa3}, 
  {0x91, 0xaf}, 
  {0x91, 0xc4}, 
  {0x91, 0xd7}, 
  {0x91, 0xe8}, 
  {0x91, 0x20}, 
  {0x92, 0x0}, 

  {0x93, 0x6}, 
  {0x93, 0xe3}, 
  {0x93, 0x3}, 
  {0x93, 0x3}, 
  {0x93, 0x0}, 
  {0x93, 0x2}, 
  {0x93, 0x0}, 
  {0x93, 0x0}, 
  {0x93, 0x0}, 
  {0x93, 0x0}, 
  {0x93, 0x0}, 
  {0x93, 0x0}, 
  {0x93, 0x0}, 
  {0x96, 0x0}, 
  {0x97, 0x8}, 
  {0x97, 0x19}, 
  {0x97, 0x2}, 
  {0x97, 0xc}, 
  {0x97, 0x24}, 
  {0x97, 0x30}, 
  {0x97, 0x28}, 
  {0x97, 0x26}, 
  {0x97, 0x2}, 
  {0x97, 0x98}, 
  {0x97, 0x80}, 
  {0x97, 0x0}, 
  {0x97, 0x0}, 
  {0xa4, 0x0}, 
  {0xa8, 0x0}, 
  {0xc5, 0x11}, 
  {0xc6, 0x51}, 
  {0xbf, 0x80}, 
  {0xc7, 0x10}, 
  {0xb6, 0x66}, 
  {0xb8, 0xa5}, 
  {0xb7, 0x64}, 
  {0xb9, 0x7c}, 
  {0xb3, 0xaf}, 
  {0xb4, 0x97}, 
  {0xb5, 0xff}, 
  {0xb0, 0xc5}, 
  {0xb1, 0x94}, 
  {0xb2, 0xf}, 
  {0xc4, 0x5c}, 
  {0xa6, 0x0}, 
  {0xa7, 0x20}, 
  {0xa7, 0xd8}, 
  {0xa7, 0x1b}, 
  {0xa7, 0x31}, 
  {0xa7, 0x0}, 
  {0xa7, 0x18}, 
  {0xa7, 0x20}, 
  {0xa7, 0xd8}, 
  {0xa7, 0x19}, 
  {0xa7, 0x31}, 
  {0xa7, 0x0}, 
  {0xa7, 0x18}, 
  {0xa7, 0x20}, 
  {0xa7, 0xd8}, 
  {0xa7, 0x19}, 
  {0xa7, 0x31}, 
  {0xa7, 0x0}, 
  {0xa7, 0x18}, 
  {0x7f, 0x0}, 
  {0xe5, 0x1f}, 
  {0xe1, 0x77}, 
  {0xdd, 0x7f}, 
  {0xc2, 0xe}, 
  
  {0xff, 0x0}, 
  {0xe0, 0x4}, 
  {0xc0, 0xc8}, 
  {0xc1, 0x96}, 
  {0x86, 0x3d}, 
  {0x51, 0x90}, 
  {0x52, 0x2c}, 
  {0x53, 0x0}, 
  {0x54, 0x0}, 
  {0x55, 0x88}, 
  {0x57, 0x0}, 
  
  {0x50, 0x92}, 
  {0x5a, 0x50}, 
  {0x5b, 0x3c}, 
  {0x5c, 0x0}, 
  {0xd3, 0x4}, 
  {0xe0, 0x0}, 
  
  {0xff, 0x0}, 
  {0x5, 0x0}, 
  
  {0xda, 0x8}, 
  {0xd7, 0x3}, 
  {0xe0, 0x0}, 
  
  {0x5, 0x0}, 

  
  {0xff,0xff},
};        

const struct sensor_reg OV2640_JPEG_INIT[]  =
{
  { 0xff, 0x00 },
  { 0x2c, 0xff },
  { 0x2e, 0xdf },
  { 0xff, 0x01 },
  { 0x3c, 0x32 },
  { 0x11, 0x00 }, 
  { 0x09, 0x02 },
  { 0x04, 0x28 },
  { 0x13, 0xe5 },
  { 0x14, 0x48 },
  { 0x2c, 0x0c },
  { 0x33, 0x78 },
  { 0x3a, 0x33 },
  { 0x3b, 0xfB },
  { 0x3e, 0x00 },
  { 0x43, 0x11 },
  { 0x16, 0x10 },
  { 0x39, 0x92 },
  { 0x35, 0xda },
  { 0x22, 0x1a },
  { 0x37, 0xc3 },
  { 0x23, 0x00 },
  { 0x34, 0xc0 },
  { 0x36, 0x1a },
  { 0x06, 0x88 },
  { 0x07, 0xc0 },
  { 0x0d, 0x87 },
  { 0x0e, 0x41 },
  { 0x4c, 0x00 },
  { 0x48, 0x00 },
  { 0x5B, 0x00 },
  { 0x42, 0x03 },
  { 0x4a, 0x81 },
  { 0x21, 0x99 },
  { 0x24, 0x40 },
  { 0x25, 0x38 },
  { 0x26, 0x82 },
  { 0x5c, 0x00 },
  { 0x63, 0x00 },
  { 0x61, 0x70 },
  { 0x62, 0x80 },
  { 0x7c, 0x05 },
  { 0x20, 0x80 },
  { 0x28, 0x30 },
  { 0x6c, 0x00 },
  { 0x6d, 0x80 },
  { 0x6e, 0x00 },
  { 0x70, 0x02 },
  { 0x71, 0x94 },
  { 0x73, 0xc1 },
  { 0x12, 0x40 },
  { 0x17, 0x11 },
  { 0x18, 0x43 },
  { 0x19, 0x00 },
  { 0x1a, 0x4b },
  { 0x32, 0x09 },
  { 0x37, 0xc0 },
  { 0x4f, 0x60 },
  { 0x50, 0xa8 },
  { 0x6d, 0x00 },
  { 0x3d, 0x38 },
  { 0x46, 0x3f },
  { 0x4f, 0x60 },
  { 0x0c, 0x3c },
  { 0xff, 0x00 },
  { 0xe5, 0x7f },
  { 0xf9, 0xc0 },
  { 0x41, 0x24 },
  { 0xe0, 0x14 },
  { 0x76, 0xff },
  { 0x33, 0xa0 },
  { 0x42, 0x20 },
  { 0x43, 0x18 },
  { 0x4c, 0x00 },
  { 0x87, 0xd5 },
  { 0x88, 0x3f },
  { 0xd7, 0x03 },
  { 0xd9, 0x10 },
  { 0xd3, 0x82 },
  { 0xc8, 0x08 },
  { 0xc9, 0x80 },
  { 0x7c, 0x00 },
  { 0x7d, 0x00 },
  { 0x7c, 0x03 },
  { 0x7d, 0x48 },
  { 0x7d, 0x48 },
  { 0x7c, 0x08 },
  { 0x7d, 0x20 },
  { 0x7d, 0x10 },
  { 0x7d, 0x0e },
  { 0x90, 0x00 },
  { 0x91, 0x0e },
  { 0x91, 0x1a },
  { 0x91, 0x31 },
  { 0x91, 0x5a },
  { 0x91, 0x69 },
  { 0x91, 0x75 },
  { 0x91, 0x7e },
  { 0x91, 0x88 },
  { 0x91, 0x8f },
  { 0x91, 0x96 },
  { 0x91, 0xa3 },
  { 0x91, 0xaf },
  { 0x91, 0xc4 },
  { 0x91, 0xd7 },
  { 0x91, 0xe8 },
  { 0x91, 0x20 },
  { 0x92, 0x00 },
  { 0x93, 0x06 },
  { 0x93, 0xe3 },
  { 0x93, 0x05 },
  { 0x93, 0x05 },
  { 0x93, 0x00 },
  { 0x93, 0x04 },
  { 0x93, 0x00 },
  { 0x93, 0x00 },
  { 0x93, 0x00 },
  { 0x93, 0x00 },
  { 0x93, 0x00 },
  { 0x93, 0x00 },
  { 0x93, 0x00 },
  { 0x96, 0x00 },
  { 0x97, 0x08 },
  { 0x97, 0x19 },
  { 0x97, 0x02 },
  { 0x97, 0x0c },
  { 0x97, 0x24 },
  { 0x97, 0x30 },
  { 0x97, 0x28 },
  { 0x97, 0x26 },
  { 0x97, 0x02 },
  { 0x97, 0x98 },
  { 0x97, 0x80 },
  { 0x97, 0x00 },
  { 0x97, 0x00 },
  { 0xc3, 0xed },
  { 0xa4, 0x00 },
  { 0xa8, 0x00 },
  { 0xc5, 0x11 },
  { 0xc6, 0x51 },
  { 0xbf, 0x80 },
  { 0xc7, 0x10 },
  { 0xb6, 0x66 },
  { 0xb8, 0xA5 },
  { 0xb7, 0x64 },
  { 0xb9, 0x7C },
  { 0xb3, 0xaf },
  { 0xb4, 0x97 },
  { 0xb5, 0xFF },
  { 0xb0, 0xC5 },
  { 0xb1, 0x94 },
  { 0xb2, 0x0f },
  { 0xc4, 0x5c },
  { 0xc0, 0x64 },
  { 0xc1, 0x4B },
  { 0x8c, 0x00 },
  { 0x86, 0x3D },
  { 0x50, 0x00 },
  { 0x51, 0xC8 },
  { 0x52, 0x96 },
  { 0x53, 0x00 },
  { 0x54, 0x00 },
  { 0x55, 0x00 },
  { 0x5a, 0xC8 },
  { 0x5b, 0x96 },
  { 0x5c, 0x00 },
  { 0xd3, 0x00 }, //{ 0xd3, 0x7f },
  { 0xc3, 0xed },
  { 0x7f, 0x00 },
  { 0xda, 0x00 },
  { 0xe5, 0x1f },
  { 0xe1, 0x67 },
  { 0xe0, 0x00 },
  { 0xdd, 0x7f },
  { 0x05, 0x00 },
               
  { 0x12, 0x40 },
  { 0xd3, 0x04 }, //{ 0xd3, 0x7f },
  { 0xc0, 0x16 },
  { 0xC1, 0x12 },
  { 0x8c, 0x00 },
  { 0x86, 0x3d },
  { 0x50, 0x00 },
  { 0x51, 0x2C },
  { 0x52, 0x24 },
  { 0x53, 0x00 },
  { 0x54, 0x00 },
  { 0x55, 0x00 },
  { 0x5A, 0x2c },
  { 0x5b, 0x24 },
  { 0x5c, 0x00 },
  { 0xff, 0xff },
};             

const struct sensor_reg OV2640_YUV422[]  =
{
  { 0xFF, 0x00 },
  { 0x05, 0x00 },
  { 0xDA, 0x10 },
  { 0xD7, 0x03 },
  { 0xDF, 0x00 },
  { 0x33, 0x80 },
  { 0x3C, 0x40 },
  { 0xe1, 0x77 },
  { 0x00, 0x00 },
  { 0xff, 0xff },
};

const struct sensor_reg OV2640_JPEG[]  =  
{
  { 0xe0, 0x14 },
  { 0xe1, 0x77 },
  { 0xe5, 0x1f },
  { 0xd7, 0x03 },
  { 0xda, 0x10 },
  { 0xe0, 0x00 },
  { 0xFF, 0x01 },
  { 0x04, 0x08 },
  { 0xff, 0xff },
}; 

/* JPG 160x120 */
const struct sensor_reg OV2640_160x120_JPEG[]  =  
{
  { 0xff, 0x01 },
  { 0x12, 0x40 },
  { 0x17, 0x11 },
  { 0x18, 0x43 },
  { 0x19, 0x00 },
  { 0x1a, 0x4b },
  { 0x32, 0x09 },
  { 0x4f, 0xca },
  { 0x50, 0xa8 },
  { 0x5a, 0x23 },
  { 0x6d, 0x00 },
  { 0x39, 0x12 },
  { 0x35, 0xda },
  { 0x22, 0x1a },
  { 0x37, 0xc3 },
  { 0x23, 0x00 },
  { 0x34, 0xc0 },
  { 0x36, 0x1a },
  { 0x06, 0x88 },
  { 0x07, 0xc0 },
  { 0x0d, 0x87 },
  { 0x0e, 0x41 },
  { 0x4c, 0x00 },
  { 0xff, 0x00 },
  { 0xe0, 0x04 },
  { 0xc0, 0x64 },
  { 0xc1, 0x4b },
  { 0x86, 0x35 },
  { 0x50, 0x92 },
  { 0x51, 0xc8 },
  { 0x52, 0x96 },
  { 0x53, 0x00 },
  { 0x54, 0x00 },
  { 0x55, 0x00 },
  { 0x57, 0x00 },
  { 0x5a, 0x28 },
  { 0x5b, 0x1e },
  { 0x5c, 0x00 },
  { 0xe0, 0x00 },
  { 0xff, 0xff },
};

/* JPG, 0x176x144 */

const struct sensor_reg OV2640_176x144_JPEG[]  =  
{
  { 0xff, 0x01 },
  { 0x12, 0x40 },
  { 0x17, 0x11 },
  { 0x18, 0x43 },
  { 0x19, 0x00 },
  { 0x1a, 0x4b },
  { 0x32, 0x09 },
  { 0x4f, 0xca },
  { 0x50, 0xa8 },
  { 0x5a, 0x23 },
  { 0x6d, 0x00 },
  { 0x39, 0x12 },
  { 0x35, 0xda },
  { 0x22, 0x1a },
  { 0x37, 0xc3 },
  { 0x23, 0x00 },
  { 0x34, 0xc0 },
  { 0x36, 0x1a },
  { 0x06, 0x88 },
  { 0x07, 0xc0 },
  { 0x0d, 0x87 },
  { 0x0e, 0x41 },
  { 0x4c, 0x00 },
  { 0xff, 0x00 },
  { 0xe0, 0x04 },
  { 0xc0, 0x64 },
  { 0xc1, 0x4b },
  { 0x86, 0x35 },
  { 0x50, 0x92 },
  { 0x51, 0xc8 },
  { 0x52, 0x96 },
  { 0x53, 0x00 },
  { 0x54, 0x00 },
  { 0x55, 0x00 },
  { 0x57, 0x00 },
  { 0x5a, 0x2c },
  { 0x5b, 0x24 },
  { 0x5c, 0x00 },
  { 0xe0, 0x00 },
  { 0xff, 0xff },
};

/* JPG 320x240 */

const struct sensor_reg OV2640_320x240_JPEG[]  =  
{
  { 0xff, 0x01 },
  { 0x12, 0x40 },
  { 0x17, 0x11 },
  { 0x18, 0x43 },
  { 0x19, 0x00 },
  { 0x1a, 0x4b },
  { 0x32, 0x09 },
  { 0x4f, 0xca },
  { 0x50, 0xa8 },
  { 0x5a, 0x23 },
  { 0x6d, 0x00 },
  { 0x39, 0x12 },
  { 0x35, 0xda },
  { 0x22, 0x1a },
  { 0x37, 0xc3 },
  { 0x23, 0x00 },
  { 0x34, 0xc0 },
  { 0x36, 0x1a },
  { 0x06, 0x88 },
  { 0x07, 0xc0 },
  { 0x0d, 0x87 },
  { 0x0e, 0x41 },
  { 0x4c, 0x00 },
  { 0xff, 0x00 },
  { 0xe0, 0x04 },
  { 0xc0, 0x64 },
  { 0xc1, 0x4b },
  { 0x86, 0x35 },
  { 0x50, 0x89 },
  { 0x51, 0xc8 },
  { 0x52, 0x96 },
  { 0x53, 0x00 },
  { 0x54, 0x00 },
  { 0x55, 0x00 },
  { 0x57, 0x00 },
  { 0x5a, 0x50 },
  { 0x5b, 0x3c },
  { 0x5c, 0x00 },
  { 0xe0, 0x00 },
  { 0xff, 0xff },
};

/* JPG 352x288 */

const struct sensor_reg OV2640_352x288_JPEG[]  =  

{
  { 0xff, 0x01 },
  { 0x12, 0x40 },
  { 0x17, 0x11 },
  { 0x18, 0x43 },
  { 0x19, 0x00 },
  { 0x1a, 0x4b },
  { 0x32, 0x09 },
  { 0x4f, 0xca },
  { 0x50, 0xa8 },
  { 0x5a, 0x23 },
  { 0x6d, 0x00 },
  { 0x39, 0x12 },
  { 0x35, 0xda },
  { 0x22, 0x1a },
  { 0x37, 0xc3 },
  { 0x23, 0x00 },
  { 0x34, 0xc0 },
  { 0x36, 0x1a },
  { 0x06, 0x88 },
  { 0x07, 0xc0 },
  { 0x0d, 0x87 },
  { 0x0e, 0x41 },
  { 0x4c, 0x00 },
  { 0xff, 0x00 },
  { 0xe0, 0x04 },
  { 0xc0, 0x64 },
  { 0xc1, 0x4b },
  { 0x86, 0x35 },
  { 0x50, 0x89 },
  { 0x51, 0xc8 },
  { 0x52, 0x96 },
  { 0x53, 0x00 },
  { 0x54, 0x00 },
  { 0x55, 0x00 },
  { 0x57, 0x00 },
  { 0x5a, 0x58 },
  { 0x5b, 0x48 },
  { 0x5c, 0x00 },
  { 0xe0, 0x00 },  
  { 0xff, 0xff },
};

/* JPG 640x480 */
const struct sensor_reg OV2640_640x480_JPEG[]  =  
{
  {0xff, 0x01},
  {0x11, 0x01},
  {0x12, 0x00}, // Bit[6:4]: Resolution selection//0x02为彩条
  {0x17, 0x11}, // HREFST[10:3]
  {0x18, 0x75}, // HREFEND[10:3]
  {0x32, 0x36}, // Bit[5:3]: HREFEND[2:0]; Bit[2:0]: HREFST[2:0]
  {0x19, 0x01}, // VSTRT[9:2]
  {0x1a, 0x97}, // VEND[9:2]
  {0x03, 0x0f}, // Bit[3:2]: VEND[1:0]; Bit[1:0]: VSTRT[1:0]
  {0x37, 0x40},
  {0x4f, 0xbb},
  {0x50, 0x9c},
  {0x5a, 0x57},
  {0x6d, 0x80},
  {0x3d, 0x34},
  {0x39, 0x02},
  {0x35, 0x88},
  {0x22, 0x0a},
  {0x37, 0x40},
  {0x34, 0xa0},
  {0x06, 0x02},
  {0x0d, 0xb7},
  {0x0e, 0x01},
  
  {0xff, 0x00},           
  {0xe0, 0x04},       
  {0xc0, 0xc8},       
  {0xc1, 0x96},       
  {0x86, 0x3d},       
  {0x50, 0x89},       
  {0x51, 0x90},       
  {0x52, 0x2c},       
  {0x53, 0x00},       
  {0x54, 0x00},       
  {0x55, 0x88},       
  {0x57, 0x00},       
  {0x5a, 0xa0},       
  {0x5b, 0x78},       
  {0x5c, 0x00},       
  {0xd3, 0x04},       
  {0xe0, 0x00},       
                      
    {0xff, 0xff},
};     
    
/* JPG 800x600 */
const struct sensor_reg OV2640_800x600_JPEG[]  =  
{
  {0xff, 0x01},
  {0x11, 0x01},
  {0x12, 0x00}, // Bit[6:4]: Resolution selection//0x02为彩条
  {0x17, 0x11}, // HREFST[10:3]
  {0x18, 0x75}, // HREFEND[10:3]
  {0x32, 0x36}, // Bit[5:3]: HREFEND[2:0]; Bit[2:0]: HREFST[2:0]
  {0x19, 0x01}, // VSTRT[9:2]
  {0x1a, 0x97}, // VEND[9:2]
  {0x03, 0x0f}, // Bit[3:2]: VEND[1:0]; Bit[1:0]: VSTRT[1:0]
  {0x37, 0x40},
  {0x4f, 0xbb},
  {0x50, 0x9c},
  {0x5a, 0x57},
  {0x6d, 0x80},
  {0x3d, 0x34},
  {0x39, 0x02},
  {0x35, 0x88},
  {0x22, 0x0a},
  {0x37, 0x40},
  {0x34, 0xa0},
  {0x06, 0x02},
  {0x0d, 0xb7},
  {0x0e, 0x01},
  
  {0xff, 0x00},
  {0xe0, 0x04},
  {0xc0, 0xc8},
  {0xc1, 0x96},
  {0x86, 0x35},
  {0x50, 0x89},
  {0x51, 0x90},
  {0x52, 0x2c},
  {0x53, 0x00},
  {0x54, 0x00},
  {0x55, 0x88},
  {0x57, 0x00},
  {0x5a, 0xc8},
  {0x5b, 0x96},
  {0x5c, 0x00},
  {0xd3, 0x02},
  {0xe0, 0x00},
                      
    {0xff, 0xff},
};     
       
/* JPG 1024x768 */
const struct sensor_reg OV2640_1024x768_JPEG[]  =  
{
  {0xff, 0x01},
  {0x11, 0x01},
  {0x12, 0x00}, // Bit[6:4]: Resolution selection//0x02为彩条
  {0x17, 0x11}, // HREFST[10:3]
  {0x18, 0x75}, // HREFEND[10:3]
  {0x32, 0x36}, // Bit[5:3]: HREFEND[2:0]; Bit[2:0]: HREFST[2:0]
  {0x19, 0x01}, // VSTRT[9:2]
  {0x1a, 0x97}, // VEND[9:2]
  {0x03, 0x0f}, // Bit[3:2]: VEND[1:0]; Bit[1:0]: VSTRT[1:0]
  {0x37, 0x40},
  {0x4f, 0xbb},
  {0x50, 0x9c},
  {0x5a, 0x57},
  {0x6d, 0x80},
  {0x3d, 0x34},
  {0x39, 0x02},
  {0x35, 0x88},
  {0x22, 0x0a},
  {0x37, 0x40},
  {0x34, 0xa0},
  {0x06, 0x02},
  {0x0d, 0xb7},
  {0x0e, 0x01},
  
  {0xff, 0x00},     
  {0xc0, 0xC8},          
  {0xc1, 0x96},          
  {0x8c, 0x00},          
  {0x86, 0x3D},          
  {0x50, 0x00},          
  {0x51, 0x90},          
  {0x52, 0x2C},          
  {0x53, 0x00},          
  {0x54, 0x00},          
  {0x55, 0x88},          
  {0x5a, 0x00},          
  {0x5b, 0xC0},          
  {0x5c, 0x01},          
  {0xd3, 0x02},          

                      
  {0xff, 0xff},
};  

   /* JPG 1280x1024 */
const struct sensor_reg OV2640_1280x1024_JPEG[]  =  
{
  {0xff, 0x01},
  {0x11, 0x01},
  {0x12, 0x00}, // Bit[6:4]: Resolution selection//0x02为彩条
  {0x17, 0x11}, // HREFST[10:3]
  {0x18, 0x75}, // HREFEND[10:3]
  {0x32, 0x36}, // Bit[5:3]: HREFEND[2:0]; Bit[2:0]: HREFST[2:0]
  {0x19, 0x01}, // VSTRT[9:2]
  {0x1a, 0x97}, // VEND[9:2]
  {0x03, 0x0f}, // Bit[3:2]: VEND[1:0]; Bit[1:0]: VSTRT[1:0]
  {0x37, 0x40},
  {0x4f, 0xbb},
  {0x50, 0x9c},
  {0x5a, 0x57},
  {0x6d, 0x80},
  {0x3d, 0x34},
  {0x39, 0x02},
  {0x35, 0x88},
  {0x22, 0x0a},
  {0x37, 0x40},
  {0x34, 0xa0},
  {0x06, 0x02},
  {0x0d, 0xb7},
  {0x0e, 0x01},
  
  {0xff, 0x00},               
  {0xe0, 0x04},           
  {0xc0, 0xc8},           
  {0xc1, 0x96},           
  {0x86, 0x3d},           
  {0x50, 0x00},           
  {0x51, 0x90},           
  {0x52, 0x2c},           
  {0x53, 0x00},           
  {0x54, 0x00},           
  {0x55, 0x88},           
  {0x57, 0x00},           
  {0x5a, 0x40},           
  {0x5b, 0xf0},           
  {0x5c, 0x01},           
  {0xd3, 0x02},           
  {0xe0, 0x00},           
                      
    {0xff, 0xff},
};         
       
   /* JPG 1600x1200 */
const struct sensor_reg OV2640_1600x1200_JPEG[]  =  
{
  {0xff, 0x01},
  {0x11, 0x01},
  {0x12, 0x00}, // Bit[6:4]: Resolution selection//0x02为彩条
  {0x17, 0x11}, // HREFST[10:3]
  {0x18, 0x75}, // HREFEND[10:3]
  {0x32, 0x36}, // Bit[5:3]: HREFEND[2:0]; Bit[2:0]: HREFST[2:0]
  {0x19, 0x01}, // VSTRT[9:2]
  {0x1a, 0x97}, // VEND[9:2]
  {0x03, 0x0f}, // Bit[3:2]: VEND[1:0]; Bit[1:0]: VSTRT[1:0]
  {0x37, 0x40},
  {0x4f, 0xbb},
  {0x50, 0x9c},
  {0x5a, 0x57},
  {0x6d, 0x80},
  {0x3d, 0x34},
  {0x39, 0x02},
  {0x35, 0x88},
  {0x22, 0x0a},
  {0x37, 0x40},
  {0x34, 0xa0},
  {0x06, 0x02},
  {0x0d, 0xb7},
  {0x0e, 0x01},
  
  {0xff, 0x00},                                       
  {0xe0, 0x04},                                   
  {0xc0, 0xc8},                                   
  {0xc1, 0x96},                                   
  {0x86, 0x3d},                                   
  {0x50, 0x00},                                   
  {0x51, 0x90},                                   
  {0x52, 0x2c},                                   
  {0x53, 0x00},                                   
  {0x54, 0x00},                                   
  {0x55, 0x88},                                   
  {0x57, 0x00},                                   
  {0x5a, 0x90},                                   
  {0x5b, 0x2C},                                   
  {0x5c, 0x05},              //bit2->1;bit[1:0]->1
  {0xd3, 0x02},                                   
  {0xe0, 0x00},                                   
                      
    {0xff, 0xff},
};  

int detect_arducam_ov2640(){
      int error = 0;
      uint8_t data;
      i2cbus_write(ARDUCAM_ADDRESS,0xFF,0x01);
      data = i2cbus_read(ARDUCAM_ADDRESS, 0x1D);
      if(data != 0xA2)
        error |= 1; 
      spibus_write(0x00, 0x55);
      data = spibus_read(0x00);
      if(data != 0x55)
        error |= 2;
      return error;
}

void write_reg(uint8_t addr, uint8_t data){
  spibus_write(addr, data);
}

uint8_t read_reg(uint8_t addr){
  return (spibus_read(addr));
}

void wrSensorReg8_8(uint8_t addr, uint8_t data){
  i2cbus_write( ARDUCAM_ADDRESS, addr, data); 
}

void wrSensorRegs8_8(const struct sensor_reg reglist[]){
int err = 0;
uint16_t reg_addr = 0;
uint16_t reg_val = 0;
const struct sensor_reg *next = reglist;
while ((reg_addr != 0xff) | (reg_val != 0xff)){
    reg_addr = pgm_read_word(&next->reg);
    reg_val = pgm_read_word(&next->val);
    wrSensorReg8_8(reg_addr, reg_val);
    next++;
  } 
}

uint8_t rdSensorReg8_8(uint8_t addr){
  return (i2cbus_read( ARDUCAM_ADDRESS, addr)); 
}     

void set_JPEG_size(uint8_t size){
  switch(size)
  {
    case OV2640_160x120:
      wrSensorRegs8_8(OV2640_160x120_JPEG);
      break;
    case OV2640_176x144:
      wrSensorRegs8_8(OV2640_176x144_JPEG);
      break;
    case OV2640_320x240:
      wrSensorRegs8_8(OV2640_320x240_JPEG);
      break;
    case OV2640_352x288:
          wrSensorRegs8_8(OV2640_352x288_JPEG);
      break;
    case OV2640_640x480:
      wrSensorRegs8_8(OV2640_640x480_JPEG);
      break;
    case OV2640_800x600:
      wrSensorRegs8_8(OV2640_800x600_JPEG);
      break;
    case OV2640_1024x768:
      wrSensorRegs8_8(OV2640_1024x768_JPEG);
      break;
    case OV2640_1280x1024:
      wrSensorRegs8_8(OV2640_1280x1024_JPEG);
      break;
    case OV2640_1600x1200:
      wrSensorRegs8_8(OV2640_1600x1200_JPEG);
      break;
    default:
      wrSensorRegs8_8(OV2640_320x240_JPEG);
      break;
  }
}


void InitCAM(img_format_t fmt){
  wrSensorReg8_8(0xff, 0x01);
  wrSensorReg8_8(0x12, 0x80);
  delay(100);
  if (fmt == JPEG)
  {
    wrSensorRegs8_8(OV2640_JPEG_INIT);
    wrSensorRegs8_8(OV2640_YUV422);
    wrSensorRegs8_8(OV2640_JPEG);
    wrSensorReg8_8(0xff, 0x01);
    wrSensorReg8_8(0x15, 0x00);
    wrSensorRegs8_8(OV2640_320x240_JPEG);
  }
  else
  {
    wrSensorRegs8_8(OV2640_QVGA);
  }
}

void flush_fifo(void){
  write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

void start_capture(void){
  write_reg(ARDUCHIP_FIFO, FIFO_START_MASK);
}

void clear_fifo_flag(void){
  write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

uint8_t read_fifo(void){
  uint8_t data;
  data = read_reg(SINGLE_FIFO_READ);
  return data;
}

uint32_t read_fifo_length(void){
  uint32_t len1,len2,len3,length=0;
  len1 = read_reg(FIFO_SIZE1);
  len2 = read_reg(FIFO_SIZE2);
  len3 = read_reg(FIFO_SIZE3) & 0x7f;
  length = ((len3 << 16) | (len2 << 8) | len1) & 0x07fffff;
  return length;
}

void set_bit(uint8_t addr, uint8_t bit){
  uint8_t temp;
  temp = read_reg(addr);
  write_reg(addr, temp | bit);
}

void clear_bit(uint8_t addr, uint8_t bit){
  uint8_t temp;
  temp = read_reg(addr);
  write_reg(addr, temp & (~bit));
}

uint8_t get_bit(uint8_t addr, uint8_t bit){
  uint8_t temp;
  temp = read_reg(addr);
  temp = temp & bit;
  return temp;
}


void set_Light_Mode(uint8_t Light_Mode){
 switch(Light_Mode)
 {
  
    case Auto:
    wrSensorReg8_8(0xff, 0x00);
    wrSensorReg8_8(0xc7, 0x00); //AWB on
    break;
    case Sunny:
    wrSensorReg8_8(0xff, 0x00);
    wrSensorReg8_8(0xc7, 0x40); //AWB off
    wrSensorReg8_8(0xcc, 0x5e);
    wrSensorReg8_8(0xcd, 0x41);
    wrSensorReg8_8(0xce, 0x54);
    break;
    case Cloudy:
    wrSensorReg8_8(0xff, 0x00);
    wrSensorReg8_8(0xc7, 0x40); //AWB off
    wrSensorReg8_8(0xcc, 0x65);
    wrSensorReg8_8(0xcd, 0x41);
    wrSensorReg8_8(0xce, 0x4f);  
    break;
    case Office:
    wrSensorReg8_8(0xff, 0x00);
    wrSensorReg8_8(0xc7, 0x40); //AWB off
    wrSensorReg8_8(0xcc, 0x52);
    wrSensorReg8_8(0xcd, 0x41);
    wrSensorReg8_8(0xce, 0x66);
    break;
    case Home:
    wrSensorReg8_8(0xff, 0x00);
    wrSensorReg8_8(0xc7, 0x40); //AWB off
    wrSensorReg8_8(0xcc, 0x42);
    wrSensorReg8_8(0xcd, 0x3f);
    wrSensorReg8_8(0xce, 0x71);
    break;
    default :
    wrSensorReg8_8(0xff, 0x00);
    wrSensorReg8_8(0xc7, 0x00); //AWB on
    break; 
      }
 }
 
void set_Color_Saturation(uint8_t Color_Saturation)
{
  switch(Color_Saturation)
  {
    case Saturation2:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x02);
      wrSensorReg8_8(0x7c, 0x03);
      wrSensorReg8_8(0x7d, 0x68);
      wrSensorReg8_8(0x7d, 0x68);
      break;
    case Saturation1:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x02);
      wrSensorReg8_8(0x7c, 0x03);
      wrSensorReg8_8(0x7d, 0x58);
      wrSensorReg8_8(0x7d, 0x58);
      break;
    case Saturation0:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x02);
      wrSensorReg8_8(0x7c, 0x03);
      wrSensorReg8_8(0x7d, 0x48);
      wrSensorReg8_8(0x7d, 0x48);
      break;
    case Saturation_1:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x02);
      wrSensorReg8_8(0x7c, 0x03);
      wrSensorReg8_8(0x7d, 0x38);
      wrSensorReg8_8(0x7d, 0x38);
      break;
    case Saturation_2:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x02);
      wrSensorReg8_8(0x7c, 0x03);
      wrSensorReg8_8(0x7d, 0x28);
      wrSensorReg8_8(0x7d, 0x28);
      break;  
  }
}    

void set_Brightness(uint8_t Brightness)
{
  switch(Brightness)
  {
    case Brightness2:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x04);
      wrSensorReg8_8(0x7c, 0x09);
      wrSensorReg8_8(0x7d, 0x40);
      wrSensorReg8_8(0x7d, 0x00);
      break;
    case Brightness1:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x04);
      wrSensorReg8_8(0x7c, 0x09);
      wrSensorReg8_8(0x7d, 0x30);
      wrSensorReg8_8(0x7d, 0x00);
      break;  
    case Brightness0:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x04);
      wrSensorReg8_8(0x7c, 0x09);
      wrSensorReg8_8(0x7d, 0x20);
      wrSensorReg8_8(0x7d, 0x00);
      break;
    case Brightness_1:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x04);
      wrSensorReg8_8(0x7c, 0x09);
      wrSensorReg8_8(0x7d, 0x10);
      wrSensorReg8_8(0x7d, 0x00);
      break;
    case Brightness_2:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x04);
      wrSensorReg8_8(0x7c, 0x09);
      wrSensorReg8_8(0x7d, 0x00);
      wrSensorReg8_8(0x7d, 0x00);
      break;  
  }
}

void set_Contrast(uint8_t Contrast)
{ 
switch(Contrast)
  {
    case Contrast2:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x04);
      wrSensorReg8_8(0x7c, 0x07);
      wrSensorReg8_8(0x7d, 0x20);
      wrSensorReg8_8(0x7d, 0x28);
      wrSensorReg8_8(0x7d, 0x0c);
      wrSensorReg8_8(0x7d, 0x06);
      break;
    case Contrast1:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x04);
      wrSensorReg8_8(0x7c, 0x07);
      wrSensorReg8_8(0x7d, 0x20);
      wrSensorReg8_8(0x7d, 0x24);
      wrSensorReg8_8(0x7d, 0x16);
      wrSensorReg8_8(0x7d, 0x06); 
      break;
    case Contrast0:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x04);
      wrSensorReg8_8(0x7c, 0x07);
      wrSensorReg8_8(0x7d, 0x20);
      wrSensorReg8_8(0x7d, 0x20);
      wrSensorReg8_8(0x7d, 0x20);
      wrSensorReg8_8(0x7d, 0x06); 
      break;
    case Contrast_1:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x04);
      wrSensorReg8_8(0x7c, 0x07);
      wrSensorReg8_8(0x7d, 0x20);
      wrSensorReg8_8(0x7d, 0x20);
      wrSensorReg8_8(0x7d, 0x2a);
      wrSensorReg8_8(0x7d, 0x06); 
      break;
    case Contrast_2:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x04);
      wrSensorReg8_8(0x7c, 0x07);
      wrSensorReg8_8(0x7d, 0x20);
      wrSensorReg8_8(0x7d, 0x18);
      wrSensorReg8_8(0x7d, 0x34);
      wrSensorReg8_8(0x7d, 0x06);
      break;
  } 
}

void set_Special_effects(uint8_t Special_effect)
{ 
  switch(Special_effect)
  {
    case Antique:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x18);
      wrSensorReg8_8(0x7c, 0x05);
      wrSensorReg8_8(0x7d, 0x40);
      wrSensorReg8_8(0x7d, 0xa6);
      break;
    case Bluish:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x18);
      wrSensorReg8_8(0x7c, 0x05);
      wrSensorReg8_8(0x7d, 0xa0);
      wrSensorReg8_8(0x7d, 0x40);
      break;
    case Greenish:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x18);
      wrSensorReg8_8(0x7c, 0x05);
      wrSensorReg8_8(0x7d, 0x40);
      wrSensorReg8_8(0x7d, 0x40);
      break;
    case Reddish:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x18);
      wrSensorReg8_8(0x7c, 0x05);
      wrSensorReg8_8(0x7d, 0x40);
      wrSensorReg8_8(0x7d, 0xc0);
      break;
    case BW:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x18);
      wrSensorReg8_8(0x7c, 0x05);
      wrSensorReg8_8(0x7d, 0x80);
      wrSensorReg8_8(0x7d, 0x80);
      break;
    case Negative:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x40);
      wrSensorReg8_8(0x7c, 0x05);
      wrSensorReg8_8(0x7d, 0x80);
      wrSensorReg8_8(0x7d, 0x80);
      break;
    case BWnegative:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x58);
      wrSensorReg8_8(0x7c, 0x05);
      wrSensorReg8_8(0x7d, 0x80);
      wrSensorReg8_8(0x7d, 0x80);
      break;
    case Normal:
      wrSensorReg8_8(0xff, 0x00);
      wrSensorReg8_8(0x7c, 0x00);
      wrSensorReg8_8(0x7d, 0x00);
      wrSensorReg8_8(0x7c, 0x05);
      wrSensorReg8_8(0x7d, 0x80);
      wrSensorReg8_8(0x7d, 0x80);
      break; 
}
}

uint8_t rgb2gray(uint16_t pixel){
  uint16_t R = pixel&0b1111100000000000;
  uint16_t G = pixel&0b0000011111100000;
  uint16_t B = pixel&0b0000000000011111;
  uint8_t byte = ((R>>8) + (G>>3) +(B<<3)) >> 3;
  return byte;
}

void capture_and_transmit(img_format_t fmt, uint8_t size, uint8_t threshold, uint8_t light_mode, uint8_t saturation, uint8_t brightness, uint8_t contrast, uint8_t special_effect){
  uint32_t runtime = timer;
  uint32_t capture_timer = 0;
  uint32_t process_timer = 0;
  uint32_t transmit_timer = 0;
  InitCAM(fmt);
  if (fmt == JPEG){
    set_JPEG_size(size);
  }
  clear_fifo_flag();
  write_reg(ARDUCHIP_FRAMES,0x00);
  set_Light_Mode(light_mode);
  set_Color_Saturation(saturation);
  set_Brightness(brightness);
  set_Contrast(contrast);
  set_Special_effects(special_effect);
  flush_fifo();
  clear_fifo_flag();
  read_reg(ARDUCHIP_FIFO);
  start_capture();
  while (get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK) == 0);
  uint32_t len = read_fifo_length();
  if (fmt == JPEG){
    for (int i = 0; i < len; i++){
      uint32_t byte = read_fifo();
      debug = byte & 0xff;
    }
  }
  else if (fmt == RGB565){
    process_timer = timer;
    uint32_t start = 0;  
    len = len >> 1;
    for (int i = 0; i < len; i++){
      start = timer;
      uint32_t byte1 = read_fifo();
      uint32_t byte2 = read_fifo();
      capture_timer += (timer - start);
      start = timer;
      debug = byte1;
      debug = byte2;
      transmit_timer += (timer - start);
    }
    process_timer = timer - process_timer - capture_timer - transmit_timer;
  }
  else if (fmt == GRAY){
    len = len >> 1;
    for (int i = 0; i < len; i++){
      uint16_t R,G,B;
      uint32_t byte1 = read_fifo();
      uint32_t byte2 = read_fifo();
      uint16_t pixel = (byte1 << 8) + byte2;
      debug = rgb2gray(pixel) & 0xff;
    }
  }
  else if (fmt == BINARY){
    uint8_t bit_counter = 0;
    uint8_t byte2 = 0;
    uint8_t prev_byte = 0;
    len = len >> 1;
    for (int i = 0; i < len; i++){
      uint16_t R,G,B;
      uint32_t byte1 = read_fifo();
      uint32_t byte2 = read_fifo();
      uint16_t pixel = (byte1 << 8) + byte2;
      uint32_t byte = rgb2gray(pixel);
      uint8_t diff;
      if (byte >= prev_byte){
        diff = byte - prev_byte;
      }
      else{
        diff = prev_byte -  byte;
      }
      if (diff < threshold){
        byte2 = (byte2 << 1) | 1;
      }
      else{
        byte2 = byte2 << 1;
      }
      prev_byte = byte;
      bit_counter++;
      if (bit_counter == 8){
        debug = byte2 & 0xff;
        byte2 = 0;
        bit_counter = 0;
      }
    }
  }

  else if (fmt == EDGE_SW){
    process_timer = timer;
    uint32_t start = 0;     
    uint16_t row_0[320];
    uint16_t row_1[320];
    uint16_t row_2[320];

    uint32_t row_counter = 0;
    uint32_t col_counter = 0;
    uint8_t bit_counter = 0;
    uint8_t byte2 = 0;
    uint8_t prev_byte = 0;
    len = len >> 1;

    for (int i=0; i < len; i++){
      uint16_t R,G,B;
      start = timer;
      uint32_t byte1 = read_fifo();
      uint32_t byte2 = read_fifo();
      capture_timer += (timer - start);
      uint16_t pixel = (byte1 << 8) + byte2;
      row_2[col_counter] = pixel;
      col_counter++;
      if (col_counter == 320){
        col_counter = 0;
        row_counter++;
        if (row_counter == 1){
          for (int j=0; j <320; j++){
            row_1[j] = row_2[j];
          }
        } 
        else if (row_counter == 2){
          for (int j=0; j <320; j++){
            start = timer;
            debug = row_1[j]>>8;
            debug = row_1[j]&0x00ff;
            transmit_timer += (timer - start);
            row_0[j] = row_1[j];
            row_1[j] = row_2[j];
          }
        }
        else if (row_counter > 2){
          for (int j=1; j <319; j++){
            int tl = (rgb2gray(row_0[j-1])&0x000000ff);
            int tc = (rgb2gray(row_0[j])&0x000000ff);
            int tr = (rgb2gray(row_0[j+1])&0x000000ff);
            int cl = (rgb2gray(row_1[j-1])&0x000000ff);
            int cc = (rgb2gray(row_1[j])&0x000000ff);
            int cr = (rgb2gray(row_1[j+1])&0x000000ff);
            int bl = (rgb2gray(row_2[j-1])&0x000000ff);
            int bc = (rgb2gray(row_2[j])&0x000000ff);
            int br = (rgb2gray(row_2[j+1])&0x000000ff);
            int dxy = -1*tl -1*tc -1*tr -1*cl +8*cc -1*cr -1*bl -1*bc -1*br;
            if (dxy < 0){
              dxy = dxy*-1;
            }
            if (dxy > threshold){
              row_1[j] = 0b1111100000000000;
            }
          }
          for (int j=0; j <320; j++){
            start = timer;
            debug = row_1[j]>>8;
            debug = row_1[j]&0x00ff;
            transmit_timer += (timer - start);
            row_0[j] = row_1[j];
            row_1[j] = row_2[j];
          }
        }   
      }
    }
    for (int j=0; j <320; j++){
      start = timer;
      debug = row_2[j]>>8;
      debug = row_2[j]&0x00ff;
      transmit_timer += (timer - start);
    }
    process_timer = timer - process_timer - capture_timer - transmit_timer;
  }
  else if (fmt == EDGE_HW){ 
    process_timer = timer;
    uint32_t start = 0;      
    uint32_t ret = 0;
      __asm__ (".insn r 0x2B , 0, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (ret) : [rs1] "r" (ret), [rs2] "r" (ret));
      __asm__ (".insn r 0x2B , 0x1, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (ret) : [rs1] "r" (threshold), [rs2] "r" (threshold));
    len = len >> 1;
    for (int i=0; i < len; i++){
      uint16_t R,G,B;
      start = timer;
      uint32_t byte1 = read_fifo();
      uint32_t byte2 = read_fifo();
      capture_timer += (timer - start);
      uint32_t pixel = (byte1 << 8) + byte2;
      __asm__ (".insn r 0x2B , 0x2, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (ret) : [rs1] "r" (pixel), [rs2] "r" (pixel));
      if ((ret>>16) & 1){
        start = timer;
        debug = (ret>>8)&0x000000ff;
        debug = (ret)&0x000000ff;
        transmit_timer += (timer - start);
      }
    }
    for (int j=0; j <320; j++){
      uint32_t pixel = 0;
      __asm__ (".insn r 0x2B , 0x2, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (ret) : [rs1] "r" (pixel), [rs2] "r" (pixel));
      if ((ret>>16) & 1){
        start = timer;
        debug = (ret>>8)&0x000000ff;
        debug = (ret)&0x000000ff;
        transmit_timer += (timer - start);
      }
    }
    process_timer = timer - process_timer - capture_timer - transmit_timer;
  }
  else if (fmt == EDGE_HW_BINARY){
    process_timer = timer;
    uint32_t start = 0;      
    uint32_t ret = 0;
      __asm__ (".insn r 0x2B , 0, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (ret) : [rs1] "r" (ret), [rs2] "r" (ret));
      __asm__ (".insn r 0x2B , 0x1, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (ret) : [rs1] "r" (threshold), [rs2] "r" (threshold));
    len = len >> 1;
    uint8_t bit_counter = 0;
    uint8_t byte;
    for (int i=0; i < len; i++){
      start = timer;
      uint32_t byte1 = read_fifo();
      uint32_t byte2 = read_fifo();
      capture_timer += (timer - start);
      uint32_t pixel = (byte1 << 8) + byte2;
      __asm__ (".insn r 0x2B , 0x2, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (ret) : [rs1] "r" (pixel), [rs2] "r" (pixel));
      if ((ret>>16) & 1){
        uint8_t edge = (ret&0x000007ff) ? 0 : 1;
        byte = (byte << 1) + edge;
        bit_counter++;
        if (bit_counter == 8){
          start = timer;
          debug = byte;
          transmit_timer += (timer - start);
          bit_counter = 0;
        }
      }
    }
    for (int j=0; j <320; j++){
      uint32_t pixel = 0;
      __asm__ (".insn r 0x2B , 0x2, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (ret) : [rs1] "r" (pixel), [rs2] "r" (pixel));
      if ((ret>>16) & 1){
        uint8_t edge = (ret&0x000007ff) ? 0 : 1;
        byte = (byte << 1) + edge;
        bit_counter++;
        if (bit_counter == 8){
          start = timer;
          debug = byte;
          transmit_timer += (timer - start);
          bit_counter = 0;
        }
      }
    }
    start = timer;
    debug = byte;
    transmit_timer += (timer - start);
    process_timer = timer - process_timer - capture_timer - transmit_timer;
  }
  clear_fifo_flag();
  runtime = timer - runtime;
  if (fmt != JPEG){
    debug = capture_timer;
    debug = capture_timer >> 8;
    debug = capture_timer >> 16;
    debug = capture_timer >> 24;
    debug = process_timer;
    debug = process_timer >> 8;
    debug = process_timer >> 16;
    debug = process_timer >> 24;
    debug = transmit_timer;
    debug = transmit_timer >> 8;
    debug = transmit_timer >> 16;
    debug = transmit_timer >> 24;
    debug = runtime;
    debug = runtime >> 8;
    debug = runtime >> 16;
    debug = runtime >> 24;
    debug = 0x01;
    debug = 0x02;
    debug = 0x04;
    debug = 0x08;
  }
}
#endif