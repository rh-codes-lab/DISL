from jtag import JTAG
import time
from utils import *
import plotext as plt

bin_file = './edgetestbed_arducam_jtag_uartprog_no_dram_fulloffload/edgetestbed_arducam_jtag_uartprog_no_dram_fulloffload.runs/impl_1/top.bin'
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
freq = 12000000
while 1:
    try:
        img = capture_RAW_image(jtag_)
        try:
            decoded_img = decodeBinaryImage(img, img_height, img_width, flip)
            counters = img[-19:-3]
            capture_counter = counters[0] + ((counters[1]&0xff) << 8) + ((counters[2]&0xff) << 16) + ((counters[3]&0xff) << 24)
            process_counter = counters[4] + ((counters[5]&0xff) << 8) + ((counters[6]&0xff) << 16) + ((counters[7]&0xff) << 24)
            transmit_counter = counters[8] + ((counters[9]&0xff) << 8) + ((counters[10]&0xff) << 16) + ((counters[11]&0xff) << 24)
            runtime = counters[12] + ((counters[13]&0xff) << 8) + ((counters[14]&0xff) << 16) + ((counters[15]&0xff) << 24)
            print("Total Runtime: " + str(runtime) + " us");
            capture_time = (capture_counter/freq)*1000000
            transmit_time = (transmit_counter/freq)*1000000
            process_time = (process_counter/freq)*1000000
            print("Capture Timer: " + str(capture_time) + " us");
            print("Process Timer: " + str(process_time) + " us");
            print("Transmit Timer: " + str(transmit_time) + " us");
            s = capture_time + process_time + transmit_time
            setup_time = runtime - s
            setup = [(setup_time*100/runtime)]
            capture = [(capture_time*100/runtime)]
            process = [(process_time*100/runtime)]
            transmit = [(transmit_time*100/runtime)]
            label = ["C.P.T. HW offload"]
            plt.simple_stacked_bar(label, [setup, capture, process, transmit], width = 100, labels = ["Setup", "Capture", "Process", "Transmit"], title = "Percentage of time spent on tasks")
            plt.show()
            print("\n")
            write_numpy_image(decoded_img, image_file)
        except:
            print("Invalid image")
    except:
        break
jtag_.free_dev()
