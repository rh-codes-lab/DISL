from jtag import JTAG
import time
from utils import *
import plotext as plt

bin_file = './edgetestbed_arducam_jtag_uartprog_no_dram/edgetestbed_arducam_jtag_uartprog_no_dram.runs/impl_1/top.bin'
firmware = 'cpu_firmware.hex'
image_file = 'out.jpeg'

print("Initializing FTDI connection")
jtag_ = JTAG(0x0403, 0x6010)
print("Configuring the UART")
jtag_.ftdi_.dev.uart_configure(baudrate=921600)
print("Reconfiguring the FPGA")
jtag_.program(bin_file)
print("Done")
print("Programming the softcore")
program_softcore(jtag_,firmware)
print("Done")
print("Starting image capture")
img_width = 320
img_height = 240
bytes_per_pixel = 2
offset = 0
flip = 0
while 1:
    try:
        img = capture_RAW_image(jtag_)
        #img2 = capture_RAW_image(jtag_)
        try:
            #write_jpeg_image(img, image_file)
            decoded_img = decodeRGB565Image(img, img_height, img_width, flip)
            #decoded_img = decodeBinaryImage(img, img_height, img_width, flip)
            counters = img[-19:-3]
            runtime = counters[12] + ((counters[13]&0xff) << 8) + ((counters[14]&0xff) << 16) + ((counters[15]&0xff) << 24)
            capture_timer = counters[0] + ((counters[1]&0xff) << 8) + ((counters[2]&0xff) << 16) + ((counters[3]&0xff) << 24)
            process_timer = counters[4] + ((counters[5]&0xff) << 8) + ((counters[6]&0xff) << 16) + ((counters[7]&0xff) << 24)
            transmit_timer = counters[8] + ((counters[9]&0xff) << 8) + ((counters[10]&0xff) << 16) + ((counters[11]&0xff) << 24)
            print("Total Runtime: " + str(runtime) + " us");
            print("Capture Timer: " + str(capture_timer) + " us");
            print("Process Timer: " + str(process_timer) + " us");
            print("Transmit Timer: " + str(transmit_timer) + " us");
            s = capture_timer + process_timer + transmit_timer
            setup_time = runtime - s
            setup = [(setup_time*100/runtime)]
            capture = [(capture_timer*100/runtime)]
            process = [(process_timer*100/runtime)]
            transmit = [(transmit_timer*100/runtime)]
            label = ["Overlay w/ process offload"]
            plt.simple_stacked_bar(label, [setup, capture, process, transmit], width = 100, labels = ["Setup", "Capture", "Process", "Transmit"], title = "Percentage of time spent on tasks")
            plt.show()
            print("\n")
            write_numpy_image(decoded_img, image_file)
        except:
            print("Invalid image")
    except:
        break
jtag_.free_dev()
