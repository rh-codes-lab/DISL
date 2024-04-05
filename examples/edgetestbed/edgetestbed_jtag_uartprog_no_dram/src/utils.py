from jtag import JTAG
import time

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