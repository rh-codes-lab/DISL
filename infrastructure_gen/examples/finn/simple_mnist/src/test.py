from jtag import JTAG
import time
from utils import *

#bin_file = './edgetestbed_jtag_uartprog_no_dram/edgetestbed_jtag_uartprog_no_dram.runs/impl_1/top.bin'
bin_file = './top.bin'
firmware = 'cpu_firmware.hex'


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
print("Starting UART monitor")
try:
    while (1):
        sret = jtag_.ftdi_.dev.uart_read()
        if sret:
            print(sret,end='')
        time.sleep(0.01)
except:
    jtag_.free_dev()
