from jtag import JTAG
import json
import time
import numpy as np
from PIL import Image

# reset soc and program softcore
def program_softcore(jtag_, filename):
    jtag_.control_write({"0_31": 3, "32_63": 0, "64_95": 0})
    time.sleep(0.1)
    jtag_.control_write({"0_31": 2, "32_63": 0, "64_95": 0})
    time.sleep(0.1)
    program = []
    address = []
    counter = 0;
    with open(filename) as file:
        for line in file:
            if "@" in line: 
                counter = int(line.split('@')[1],16)
            else:
                nl_rm_line =  line.split('\n')[0];  
                nl_rm_line =  nl_rm_line.split(' ');   
                if len(nl_rm_line) == 0: continue
                words = int(len(nl_rm_line)/4)
                for i in range(words):
                    program.append(nl_rm_line[4*i+3] + nl_rm_line[4*i+2] + nl_rm_line[4*i+1] + nl_rm_line[4*i])
                    address.append(counter)
                    counter = counter + 4;       
    for i in range(len(program)):
        x = address[i]
        x1 = [(x&255)]
        x2 = [((x>>8)&255)]
        x3 = [((x>>16)&255)]
        x4 = [((x>>24)&255)]
        jtag_.ftdi_.dev.uart_write(x1)
        jtag_.ftdi_.dev.uart_write(x2)
        jtag_.ftdi_.dev.uart_write(x3)
        jtag_.ftdi_.dev.uart_write(x4)
        x = int(program[i],16)
        x1 = [(x&255)]
        x2 = [((x>>8)&255)]
        x3 = [((x>>16)&255)]
        x4 = [((x>>24)&255)]
        jtag_.ftdi_.dev.uart_write(x1)
        jtag_.ftdi_.dev.uart_write(x2)
        jtag_.ftdi_.dev.uart_write(x3)
        jtag_.ftdi_.dev.uart_write(x4)
        time.sleep(0.001)
    time.sleep(0.1)
    jtag_.control_write({"0_31": 0, "32_63": 0, "64_95": 0}) 


def capture_RAW_image(jtag_):
    img = []
    prev = -1
    prev_1 = -1
    prev_2 = -1
    while(1):
        sret = jtag_.ftdi_.dev.uart_read(is_str=0)
        for cc in sret:
            c = cc&0x00000000FF
            if (c == 0x08 and prev == 0x04 and prev_1 == 0x02 and prev_2 == 0x01):
                return img
            else:
                img.append(c)
                prev_2 = prev_1
                prev_1 = prev
                prev = c

def capture_JPEG_image(jtag_):   
    img = []
    prev = -1
    count = 0
    while(1):
        sret = jtag_.ftdi_.dev.uart_read(is_str=0)
        for cc in sret:
            c = cc&0x00000000FF
            if ((c & 0xff) == 0xd8) and (prev == 0xff):
                img.append(prev)
                img.append(c)
            elif ((c & 0xff) == 0xd9) and (prev == 0xff):
                img.append(c)
                return img
            elif len(img):
                img.append(c)
            if len(img) > 1000000:
                return []
            prev = c

def decodeRGB565Image(data, row, col, flip):
    px = []
    for num in range(int(len(data)/2)):
        idx = num+num
        byte1 = data[idx]
        byte2 = data[idx+1]
        px.append((byte1 << 8) | byte2)    
    a=[0]*3
    a=[a]*col
    a=[a]*row
    a = np.array(a, dtype=np.uint8)
    for x in range (0,row):
        for y in range (0,col):
            index = y+col*x
            if (flip):
                pixel = px[row*col - index - 1]
            else:
                pixel = px[index]
            R = pixel&0b1111100000000000
            G = pixel&0b0000011111100000
            B = pixel&0b0000000000011111
            a[x,y,0] = R>>8
            a[x,y,1] = G>>3
            a[x,y,2] = B<<3
    return a 

def decodeRGB565toGrayscaleImage(data, row, col, flip):
    px = []
    for num in range(int(len(data)/2)):
        idx = num+num
        byte1 = data[idx]
        byte2 = data[idx+1]
        px.append((byte1 << 8) | byte2)    
    a=[0]*col
    a=[a]*row
    a = np.array(a, dtype=np.uint8)
    for x in range (0,row):
        for y in range (0,col):
            index = y+col*x
            if (flip):
                pixel = px[row*col - index - 1]
            else:
                pixel = px[index]
            R = pixel&0b1111100000000000
            G = pixel&0b0000011111100000
            B = pixel&0b0000000000011111
            a[x,y] = int(0.299*(R>>8)+ 0.587*(G>>3) + 0.114*(B<<3))
    return a 

def decodeGrayscaleImage(data, row, col, flip): 
    px = data
    a=[0]*col
    a=[a]*row
    a = np.array(a, dtype=np.uint8)
    for x in range (0,row):
        for y in range (0,col):
            index = y+col*x
            if (flip):
                pixel = px[row*col - index - 1]
            else:
                pixel = px[index]
            a[x,y] = pixel
    return a 

def decodeBinaryImage(data, row, col, flip): 
    px = []
    for i in range(len(data)):
        px.append(0xff if ((data[i]>>7)&1) else 0x00)
        px.append(0xff if ((data[i]>>6)&1) else 0x00)
        px.append(0xff if ((data[i]>>5)&1) else 0x00)
        px.append(0xff if ((data[i]>>4)&1) else 0x00)
        px.append(0xff if ((data[i]>>3)&1) else 0x00)
        px.append(0xff if ((data[i]>>2)&1) else 0x00)
        px.append(0xff if ((data[i]>>1)&1) else 0x00)
        px.append(0xff if ((data[i]>>0)&1) else 0x00)
    a=[0]*col
    a=[a]*row
    a = np.array(a, dtype=np.uint8)
    for x in range (0,row):
        for y in range (0,col):
            index = y+col*x
            if (flip):
                pixel = px[row*col - index - 1]
            else:
                pixel = px[index]
            a[x,y] = pixel
    return a 



def write_jpeg_image(img, filename):
    with open(filename,'wb') as f:
        f.write(bytearray(img))

def write_numpy_image(img, filename):
    im = Image.fromarray(img)
    im.save(filename)