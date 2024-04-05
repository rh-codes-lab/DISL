SOURCE	=  cpu_test.c
OUTPUT_NAME = cpu_firmware
CONFIG = rv32i
ABI = ilp32
LINKER = cpu_linker.ld
CROSS = riscv64-linux-gnu-
CROSSCFLAGS = -O3 -Wno-int-conversion -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  
CROSSLDFLAGS = -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  


.PHONY: all
all:	$(OUTPUT_NAME).hex
	
	
$(OUTPUT_NAME).elf:$(SOURCE)
	$(CROSS)gcc $(CROSSCFLAGS) -c  -o     cpu_reset_handler.o cpu_reset_handler.S
	$(CROSS)gcc $(CROSSLDFLAGS) -Wl,-T $(LINKER) -o $@     $<  muldi3.S div.S
	
$(OUTPUT_NAME).hex:$(OUTPUT_NAME).elf
	$(CROSS)objcopy -O verilog $< /dev/stdout > $@
	$(CROSS)objdump -S $(OUTPUT_NAME).elf | less > $(OUTPUT_NAME).elf.dump
