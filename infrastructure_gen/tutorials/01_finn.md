# Tutorial: Adding the FINN MNIST example to DISL

In this tutorial, we'll walk through one possible approach of how the [FINN MNIST](https://github.com/asanaullah/finn_fedora/tree/dev/tutorials/fpga_flow) example can be added to DISL. Specifically, we will be using a RISC-V core (PicoRV32) to drive the inference block over the PCPI interface, which will allow us to use custom instructions to push test vectors and read/print the results. Our target board will be the `cmoda735t`.

Almost all code and configuration changes will be given within each step. The two exceptions are [test.hex](#testhex) and [run-docker.sh](#run-dockersh), which are large files and are thus given at the end. 

## Steps
- [Step 0: Setup](#step-0-setup)
- [Step 1: Creating a verilog wrapper module for the FINN generated design](#step-1-creating-a-verilog-wrapper-module-for-the-finn-generated-design)
- [Step 2: Adding the module to the DISL configuration files](#step-2-adding-the-module-to-the-disl-configuration-files)
- [Step 3: Copy an existing example to use as the starting point](#step-3-copy-an-existing-example-to-use-as-the-starting-point)
- [Step 4: Modify the system definition file](#step-4-modify-the-system-definition-file)
- [Step 5: Update the softcore source files](#step-5-update-the-softcore-source-files)
	-  [test.hex](#testhex)	
- [Step 6: Update the host python files](#step-6-update-the-host-python-files)
- [Step 7: Patch FINN](#step-7-patch-finn)
	-  [run-docker.sh](#run-dockersh)
- [Step 8: Create a custom Vivado compilation script](#step-8-create-a-custom-vivado-compilation-script)
- [Step 9: Create a custom build script script](#step-9-create-a-custom-build-script)
- [Step 10: Build and run the example](#step-10-build-and-run-the-example)


## Step 0: Setup
Let's start by setting up the required repos, tools, permissions and environment variables. First, we'll clone the DISL repo.
```bash
git clone https://github.com/rh-codes-lab/DISL.git
```

Next, we need to install Vivado. FINN requires 2022.1 or higher - we have used 2022.2 for this tutorial. Remember to source `settings64.sh` either within the terminal, or add it to `~/.bashrc` so that it can be done automatically.
 ```bash
source <installation_path>/Xilinx/Vivado/<Vivado_version>/settings64.sh
 ```

FINN is available in a containerized form to reduce the effort of dealing with dependencies. Assuming docker is already installed, we will create a `docker` group and add `$USER` to it so that docker can be run without root.
```bash
sudo groupadd docker
sudo usermod -aG docker $USER
```

Finally, we set the environment variables needed by FINN. This can again be added to `~/.bashrc` so that we only need to do it once. 
```bash
export FINN_XILINX_PATH=<installation_path>/Xilinx
export FINN_XILINX_VERSION=<Vivado_version>
export FINN_DOCKER_PREBUILT=0
```

## Step 1: Creating a verilog wrapper module for the FINN generated design
In the next step, we will be creating a wrapper module that will allow the PicoRV32 core to interface the generated FINN design using the Pico Co-Processor Interface (PCPI). 


The PCPI interface is explained in detail [here](https://github.com/YosysHQ/picorv32?tab=readme-ov-file#pico-co-processor-interface-pcpi). The top level interface of FINN generated inference blocks is given below. The top module (named `finn_design`) contains: a clock signal (`ap_clk`), an active low reset (`ap_rst_n`) and two streaming handshakes (`m_axis_0` and `s_axis_0`). If the data widths for either handshake (i.e. the `_tdata` signal) is smaller than the width of the actual input/output data to the model, multiple round of data transfers will be needed. 

```verilog
module finn_design
   (ap_clk,              // clock
    ap_rst_n,            // active low reset
    m_axis_0_tdata,      // inference result (output)
    m_axis_0_tready,     // ready signal for the result handshake (input)
    m_axis_0_tvalid,     // valid signal for the result handshake (output)
    s_axis_0_tdata,      // test vector (input) 
    s_axis_0_tready,     // ready signal for the test vector handshake (output)    
    s_axis_0_tvalid);    // valid signal for the test vector handshake (input)
```


The opcode of custom instructions (assigned to FINN) can be specified at compile-time, but we will hardcode the commands corresponding to the `funct3` bits (`[14:12]`) of the instructions. This command mapping is given below.

| Instruction [14:12]      | Command |
| ----------- | ----------- |
| 3'd0      | `cmd_reset`: Reset the finn_design module       |
| 3'd1   | `cmd_poll`: NOP - only triggers the PCPI interface so that the status register can be read         |
| 3'd2      | `cmd_push`: Shift (left) the vector `{pcpi_rs2, pcpi_rs1}` into the signal assigned to `s_axis_0_tdata`  |
| 3'd3   | `cmd_load`: Assert the signal assigned to `s_axis_0_tvalid` - this will load the assembled test vector into the finn_design module        |
| 3'd4      | `cmd_pop`: Assert the signal assigned to `m_axis_0_tready` - this will read the inference result from the finn_design module       |

We hardcode the width of `m_axis_0_tdata` to `8`, but keep the size of `s_axis_0_tdata` as a compile-time variable. The full verilog code is given below. 

```verilog
module finn_rv32_pcpi_wrapper(
input clk,
input        	pcpi_valid,
input [31:0] 	pcpi_insn,
input [31:0] 	pcpi_rs1,
input [31:0] 	pcpi_rs2,
output       	pcpi_wr,
output  [31:0] 	pcpi_rd,
output       	pcpi_wait,
output 	    	pcpi_ready
);

parameter OPCODE = 127;
parameter INPUT_WIDTH_BYTES = 49;

wire [7:0] dout_tdata;
wire dout_tready;
wire dout_tvalid;
reg [(INPUT_WIDTH_BYTES*8)-1:0] din_tdata;
wire din_tready;
reg din_tvalid;

wire [2:0] cmd = pcpi_insn[14:12];
wire cmd_reset = (cmd == 3'd0) ? 1'b1 : 1'b0;
wire cmd_poll = (cmd == 3'd1) ? 1'b1 : 1'b0;
wire cmd_push = (cmd == 3'd2) ? 1'b1 : 1'b0;
wire cmd_load = (cmd == 3'd3) ? 1'b1 : 1'b0;
wire cmd_pop = (cmd == 3'd4) ? 1'b1 : 1'b0;
wire valid_insn  = (pcpi_insn[6:0] == OPCODE[6:0]) ? pcpi_valid : 0;
assign pcpi_wait = 0;
assign pcpi_wr = valid_insn;
assign pcpi_ready = valid_insn;
assign pcpi_rd = {22'd0, din_tready, dout_tvalid, dout_tdata};

always @(posedge clk) begin	
    din_tvalid <= ((valid_insn && cmd_reset) || (din_tvalid && din_tready)) ? 0 : ((valid_insn && cmd_load) ? 1 : din_tvalid);
    din_tdata <= (valid_insn && cmd_push) ? {din_tdata[(INPUT_WIDTH_BYTES*8)-64-1:0],pcpi_rs2, pcpi_rs1}: din_tdata;
end


finn_design_wrapper finn_design_wrapper (
  .ap_clk                (clk               ),//i
  .ap_rst_n              ((valid_insn && cmd_reset) ? 1'b0 : 1'b1),//i

  .m_axis_0_tdata        (dout_tdata           ),//o
  .m_axis_0_tready       (valid_insn && cmd_pop),//i
  .m_axis_0_tvalid       (dout_tvalid          ),//o

  .s_axis_0_tdata        (din_tdata           ),//i
  .s_axis_0_tready       (din_tready		  ),//o
  .s_axis_0_tvalid       (din_tvalid      ) //i
);

endmodule
```

Add the above code to `infrastructure_gen/fpga/common/hdl/riscv_ci.v`.


## Step 2: Adding the module to the DISL configuration files
Once we have added the HDL for our wrapper to DISL, we also need to update configuration files so that the system builder can detect and connect it. Since the system builder does not require functionality of a module to be defined, we only need to add our wrapper to the configuration files (as opposed to all modules generated by FINN). This means that even if we change the underlying model, as long as the interface of the wrapper module is the same, we don't need to update any configuration file. 

There are two configuration files that must be updated:
| File      | Description |
| ----------- | ----------- |
| infrastructure_gen/fpga/common/config/modules.tml      | Contains the interface definitions for supported modules       |
| infrastructure_gen/fpga/common/config/defaults.tml      | Contains the default values of compile-time parameters       |

The values defined in `defaults.tml` can be different from the HDL parameters. If this is the case, we will also need to add a routine to `infrastructure_gen/fpga/system_builder/build.py`. The routine should be named `evaluate_<module name>` and should return a dictionary giving the values to be assigned to the HDL parameters. In this tutorial, we will give the values of the HDL parameters directly in `defaults.tml` - modifications to `build.py` will thus not be needed.

Add the following configuration to `infrastructure_gen/fpga/common/config/modules.tml`. Note that the key name is module name itself, and that we can group all the PCPI signals into a single interface called `pcpi` since we have already defined this interface type in `infrastructure_gen/fpga/common/config/definitions.tml`. Also note that we have specified which HDL file contains the wrapper code, whether that file is a generic HDL or board specific file, and the HDL parameters of our wrapper module. 
The fields `finn_rv32_pcpi_wrapper.TYPES` and `finn_rv32_pcpi_wrapper.REQUIREMENTS.INTERFACES` are specified here, but these can be ignored since they are currently not used by the system builder. The field `finn_rv32_pcpi_wrapper.ENCODINGS` is left empty since we don't need to pass any additional module-specific information to the system builder. 

```toml
[finn_rv32_pcpi_wrapper]
	TYPES = ["INTERCONNECT"]
	PARAMETERS = ["OPCODE", "INPUT_WIDTH_BYTES"]
	[finn_rv32_pcpi_wrapper.REQUIREMENTS]
		INTERFACES = ["clk", "pcpi"]
		[finn_rv32_pcpi_wrapper.REQUIREMENTS.INCLUDES]
			COMMON = ["riscv_ci.v"]
			BOARD = []
	[finn_rv32_pcpi_wrapper.ENCODINGS]
	[finn_rv32_pcpi_wrapper.INTERFACES.clk]
			TYPE = "CLOCK"
			DIRECTION = "SINK"
	[finn_rv32_pcpi_wrapper.INTERFACES.pcpi]
			TYPE = "PCPI"
			DIRECTION = "SINK"
			CLOCK = "clk"
			WORD_WIDTH = 32
```

Add the following configuration to `infrastructure_gen/fpga/common/config/defaults.tml`. We'll reuse the same defaults given in the HDL wrapper i.e. an opcode of `43` and the MNIST specific input width of `49` bytes. 
```toml
	[MODULES.finn_rv32_pcpi_wrapper]
		OPCODE = 43
		INPUT_WIDTH_BYTES = 49
```


## Step 3: Copy an existing example to use as the starting point
Instead of creating and populating an example from scratch, let's copy over a similar example and modify it. In this case, we'll copy the `infrastructure_gen/examples/edgetestbed/edgetestbed_jtag_uartprog_no_dram` example. Run the following code to create a new subdirectory and copy over the example.
```bash
mkdir infrastructure_gen/examples/finn
cp -r infrastructure_gen/examples/edgetestbed/edgetestbed_jtag_uartprog_no_dram infrastructure_gen/examples/finn/simple_mnist
```

## Step 4: Modify the system definition file
The example we copied over was targeting the same board as this tutorial (`cmoda735t`), and has a PicoRV32 based simple SoC already implemented. We'll modify the SoC to remove modules not needed for this tutorial (`gpio_axi`, `i2c_axi`), and add in the our wrapper module. Let's walk through the changes to each key the system definition file (`infrastructure_gen/examples/finn/simple_mnist/system.tml`). 

### DESCRIPTION
Change the project name to `simple_mnist`
```toml
[DESCRIPTION]
NAME = "simple_mnist"
```

### REQUIREMENTS
No change

### EXTERNAL_IO
We can remove the `sw`, `led` and `i2c` connections. The updated dictionary is given below.

```toml
[EXTERNAL_IO]
PORTS = ["clk_i","uart_tx","uart_rx"]
```


### INSTANTIATIONS
We'll remove both the instantiations of `gpio_axi` and `i2c_axi` modules, as well as their addressing in `picorv32_axi`'s memory map. Next, we'll add in the wrapper instantiation and update the memory map since the `gpio_axi` address was freed up. Finally, when instantiating a module, we need to assign it a unique name that we will refer to in the rest of the file. Let's call it `finn`. 

The updated `INSTANTIATIONS` dictionary is given below. 

```toml
[INSTANTIATIONS]
	[INSTANTIATIONS.cache]
		MODULE = "bram"
		PARAMETERS.MEMORY_SIZE = 256
	[INSTANTIATIONS.timer]
		MODULE = "timer_axi"
		PARAMETERS.CLOCK_FREQ_MHZ = 12
	[INSTANTIATIONS.debug]
		MODULE = "uart_axi"
		PARAMETERS.CLOCK_FREQ_MHZ = 12
		PARAMETERS.UART_BAUD_RATE_BPS = 921600
		PARAMETERS.DATA_WIDTH = 32
	[INSTANTIATIONS.chip_manager]
		MODULE = "jtag_chip_manager"
	[INSTANTIATIONS.programmer]
		MODULE = "progloader_axi"
		PARAMETERS.CLOCK_FREQ_MHZ = 12
		PARAMETERS.UART_BAUD_RATE_BPS = 921600
	[INSTANTIATIONS.finn]
		MODULE = "finn_rv32_pcpi_wrapper"
		PARAMETERS.OPCODE = 43
		PARAMETERS.INPUT_WIDTH_BYTES = 49
	[INSTANTIATIONS.cpu]
		MODULE = "picorv32_axi"
		ARCH = "rv32i"
		ABI = "ilp32"
		CROSS = "riscv32-unknown-elf-"
		CROSSCFLAGS = "-O3 -Wno-int-conversion -ffreestanding -nostdlib"
		CROSSLDFLAGS = "-ffreestanding -nostdlib  -Wl,-M"
		LINKER_REQUIREMENTS = ["muldi3.S", "div.S", "riscv-asm.h"]
		MEMORY = "cache"
		PARAMETERS.ENABLE_INTERRUPTS = 0
		PARAMETERS.ENABLE_PCPI = 1
		PARAMETERS.INSTRUCTION_MEMORY_STARTING_ADDRESS = 0
		PARAMETERS.INTERRUPT_HANDLER_STARTING_ADDRESS = 16
		PARAMETERS.INSTRUCTION_AND_DATA_MEMORY_SIZE_BYTES = 32768
		[INSTANTIATIONS.cpu.MAP]
			[INSTANTIATIONS.cpu.MAP.cache]
				ORIGIN = "0x00000000"
				LENGTH = "0x00040000"
			[INSTANTIATIONS.cpu.MAP.debug]
				ORIGIN = "0x00040000"
				LENGTH = "0x00000004"
			[INSTANTIATIONS.cpu.MAP.timer]
				ORIGIN = "0x00040004"
				LENGTH = "0x00000004"
```

### INTRINSICS
We'll need to remove any intrinsics associated with the `gpio` and `i2c` modules. Our wrapper does not need any additional intrinsics (since it is not memory mapped), so we don't need to add anything new. The updated dictionary is given below. 

```toml
[INTRINSICS]
	[[INTRINSICS.ASSIGNMENT]]
		CUSTOM_SIGNAL_WIDTH = 1
		CUSTOM_SIGNAL_NAME = "CUSTOM:jtag_reset"
		INPUT_SIGNAL =  "MODULE:chip_manager:control"
		SIGNAL_BITS = "[0]"
	[[INTRINSICS.ASSIGNMENT]]
		CUSTOM_SIGNAL_WIDTH = 1
		CUSTOM_SIGNAL_NAME = "CUSTOM:reprogram"
		INPUT_SIGNAL =  "MODULE:chip_manager:control"
		SIGNAL_BITS = "[1]"
	[[INTRINSICS.COMBINATIONAL]]
		CUSTOM_SIGNAL_WIDTH = 1
		CUSTOM_SIGNAL_NAME = "CUSTOM:cpu_resetn"
		INPUT_SIGNAL_1 =  "1'd1"
		OPERATION = "^"
		INPUT_SIGNAL_2 =  "CUSTOM:reprogram"
	[[INTRINSICS.COMBINATIONAL]]
		CUSTOM_SIGNAL_WIDTH = 1
		CUSTOM_SIGNAL_NAME = "CUSTOM:cpu_mem_wvalid_and_wready"
		INPUT_SIGNAL_1 =  "MODULE:cpu:mem:axi_wvalid"
		OPERATION = "&"
		INPUT_SIGNAL_2 =  "MODULE:cpu:mem:axi_wready"
	[[INTRINSICS.COMBINATIONAL]]
		CUSTOM_SIGNAL_WIDTH = 1
		CUSTOM_SIGNAL_NAME = "CUSTOM:cpu_mem_wvalid_and_wready_resetn"
		INPUT_SIGNAL_1 =  "CUSTOM:cpu_mem_wvalid_and_wready"
		OPERATION = "&"
		INPUT_SIGNAL_2 =  "CUSTOM:cpu_resetn"
	[[INTRINSICS.COMBINATIONAL]]
		CUSTOM_SIGNAL_WIDTH = "PARAMETER:debug:ADDR_WIDTH"
		CUSTOM_SIGNAL_NAME = "CUSTOM:debug_a_axi_awaddr"
		INPUT_SIGNAL_1 =  "MODULE:debug:a:axi_awaddr"
		OPERATION = "-"
		INPUT_SIGNAL_2 =  "SYSTEM:INSTANTIATIONS.cpu.MAP.debug.ORIGIN"
	[[INTRINSICS.COMBINATIONAL]]
		CUSTOM_SIGNAL_WIDTH = "PARAMETER:debug:ADDR_WIDTH"
		CUSTOM_SIGNAL_NAME = "CUSTOM:debug_a_axi_araddr"
		INPUT_SIGNAL_1 =  "MODULE:debug:a:axi_araddr"
		OPERATION = "-"
		INPUT_SIGNAL_2 =  "SYSTEM:INSTANTIATIONS.cpu.MAP.debug.ORIGIN"
	[[INTRINSICS.COMBINATIONAL]]
		CUSTOM_SIGNAL_WIDTH = "PARAMETER:timer:ADDR_WIDTH"
		CUSTOM_SIGNAL_NAME = "CUSTOM:timer_a_axi_awaddr"
		INPUT_SIGNAL_1 =  "MODULE:timer:a:axi_awaddr"
		OPERATION = "-"
		INPUT_SIGNAL_2 =  "SYSTEM:INSTANTIATIONS.cpu.MAP.timer.ORIGIN"
	[[INTRINSICS.COMBINATIONAL]]
		"CUSTOM_SIGNAL_WIDTH" = "PARAMETER:timer:ADDR_WIDTH"
		CUSTOM_SIGNAL_NAME = "CUSTOM:timer_a_axi_araddr"
		INPUT_SIGNAL_1 =  "MODULE:timer:a:axi_araddr"
		OPERATION = "-"
		INPUT_SIGNAL_2 =  "SYSTEM:INSTANTIATIONS.cpu.MAP.timer.ORIGIN"
	[[INTRINSICS.SEQUENTIAL_HOLD]]
		CUSTOM_SIGNAL_WIDTH = "PARAMETER:cpu:ADDR_WIDTH"
		CUSTOM_SIGNAL_NAME = "CUSTOM:cpu_axi_araddr"
		INTERNAL_SIGNAL_NAME = "INTERNAL:CUSTOM:cpu_axi_araddr"
		CUSTOM_SIGNAL_DEFAULT_VALUE = 0
		CLOCK = "MODULE:cpu:clk"
		TRIGGER = "MODULE:cpu:mem:axi_arvalid"
		HOLD_VALUE = "MODULE:cpu:mem:axi_araddr"
	[[INTRINSICS.SEQUENTIAL_HOLD]]
		CUSTOM_SIGNAL_WIDTH = "PARAMETER:cpu:ADDR_WIDTH"
		CUSTOM_SIGNAL_NAME = "CUSTOM:cpu_axi_awaddr"
		INTERNAL_SIGNAL_NAME = "INTERNAL:CUSTOM:cpu_axi_awaddr"
		CUSTOM_SIGNAL_DEFAULT_VALUE =  0
		CLOCK = "MODULE:cpu:clk"
		TRIGGER = "MODULE:cpu:mem:axi_awvalid"
		HOLD_VALUE = "MODULE:cpu:mem:axi_awaddr"
	[[INTRINSICS.SEQUENTIAL_IFELSEIF]]
		CUSTOM_SIGNAL_WIDTH = 1
		CUSTOM_SIGNAL_NAME = "CUSTOM:cpu_mem_b_valid"
		CUSTOM_SIGNAL_DEFAULT_VALUE =  0
		CLOCK = "MODULE:cpu:clk"
		CONDITION_1 = "CUSTOM:cpu_mem_wvalid_and_wready_resetn"
		ASSIGNMENT_IF_CONDITION_1_TRUE = "1"
		CONDITION_2 = "MODULE:cpu:mem:b_ready"
		ASSIGNMENT_IF_CONDITION_2_TRUE = "0"
```


### INTERCONNECT
The last part of the file we need to update is the interconnect description. Here again we will remove any references to the `gpio` and `i2c` modules, and add in the connections for our wrapper. Since the connection between PicoRV32 and our wrapper is a direct connection over the PCPI interface, we really only need to: 1) add the PCPI connection to the `INTERCONNECT.STATIC` list, that is:

```toml
["MODULE:cpu:pcpi","MODULE:finn:pcpi"]
```
and 2) update the entry in the `INTERCONNECT.STATIC` list corresponding to the clock connections so that
```toml
["BOARD:clk_i", "MODULE:cpu:clk","MODULE:cache:clk","MODULE:chip_manager:clk", "MODULE:debug:clk", "MODULE:timer:clk", "MODULE:gpio:clk", "MODULE:i2cbus:clk","MODULE:programmer:clk"],
```
becomes 
```toml
["BOARD:clk_i", "MODULE:cpu:clk","MODULE:cache:clk","MODULE:chip_manager:clk", "MODULE:debug:clk", "MODULE:timer:clk","MODULE:programmer:clk", "MODULE:finn:clk"],
```
The naming convention using to specify the signal or interface involved in an interconnect is given in the form `MODULE:<module_instantiation_name>:<interface_name>:<optional_interface_signal_name>` if we are connecting a module interface or signal, and in the form `BOARD:<external_io_name>` if we are connecting an external I/O signal. The updated `INTERCONNECT` dictionary is given below. 

```toml
[INTERCONNECT]
	STATIC = [
				["BOARD:clk_i", "MODULE:cpu:clk","MODULE:cache:clk","MODULE:chip_manager:clk", "MODULE:debug:clk", "MODULE:timer:clk","MODULE:programmer:clk", "MODULE:finn:clk"],
				["BOARD:uart_rx" , "MODULE:debug:urx", "MODULE:programmer:urx"],
				["BOARD:uart_tx" , "MODULE:debug:utx"],
				["CUSTOM:cpu_resetn","MODULE:cpu:resetn"],
				["CUSTOM:reprogram", "MODULE:debug:rst", "MODULE:timer:rst", "MODULE:programmer:reprogram"],
				["CUSTOM:jtag_reset" , "MODULE:chip_manager:rst", "MODULE:cache:rst"],
				["MODULE:cpu:pcpi","MODULE:finn:pcpi"]
	]

	OVERRIDES = [ # replace port signal assignments at the end with the overrides
			["MODULE:cpu:mem:b_valid","CUSTOM:cpu_mem_b_valid"],
			["MODULE:cpu:mem:b_response", "0"],
			["MODULE:chip_manager:a:b_response", "0"],
			["MODULE:chip_manager:a:b_valid", "1"],
			["MODULE:programmer:a:b_response", "0"],
			["MODULE:programmer:a:b_valid", "1"],
			["MODULE:cpu:irq", "0"],
			["MODULE:debug:a:axi_awaddr","CUSTOM:debug_a_axi_awaddr"],
			["MODULE:debug:a:axi_araddr","CUSTOM:debug_a_axi_araddr"],
			["MODULE:timer:a:axi_awaddr","CUSTOM:timer_a_axi_awaddr"],
			["MODULE:timer:a:axi_araddr","CUSTOM:timer_a_axi_araddr"],
			["MODULE:chip_manager:a:axi_rvalid","0"],
			["MODULE:chip_manager:a:axi_arready","0"],
			["MODULE:chip_manager:a:axi_awready","0"],
			["MODULE:chip_manager:a:axi_wready","0"],
			["MODULE:chip_manager:a:b_valid","0"]
	]

	[INTERCONNECT.DYNAMIC."MODULE:cpu:mem"]
		GROUP_SELECT = "CUSTOM:reprogram"
		HANDSHAKES = ["WRITE_ADDRESS", "WRITE_DATA", "READ_ADDRESS", "READ_DATA"]
		[[INTERCONNECT.DYNAMIC."MODULE:cpu:mem".GROUPS]]
			INTERCONNECT_TYPE = "ONE_TO_MANY"
			SELECT_VALUE = 0
			ADDRESS_MAP = [
					"SYSTEM:INSTANTIATIONS.cpu.MAP.cache MODULE:cache:cpu",
					"SYSTEM:INSTANTIATIONS.cpu.MAP.debug MODULE:debug:a",
					"SYSTEM:INSTANTIATIONS.cpu.MAP.timer MODULE:timer:a"
			]
			HANDSHAKE_MAP = [
						"WRITE_ADDRESS MODULE:cpu:mem:axi_awaddr",
						"READ_ADDRESS MODULE:cpu:mem:axi_araddr",
						"WRITE_DATA CUSTOM:cpu_axi_awaddr",
						"READ_DATA CUSTOM:cpu_axi_araddr"
			]
		[[INTERCONNECT.DYNAMIC."MODULE:cpu:mem".GROUPS]]
			INTERCONNECT_TYPE = "ONE_TO_MANY"
			SELECT_VALUE = 1
			ADDRESS_MAP = [
					"SYSTEM:INSTANTIATIONS.cpu.MAP.debug MODULE:debug:a",
					"SYSTEM:INSTANTIATIONS.cpu.MAP.timer MODULE:timer:a"
			]
			HANDSHAKE_MAP = [
						"WRITE_ADDRESS MODULE:cpu:mem:axi_awaddr",
						"READ_ADDRESS MODULE:cpu:mem:axi_araddr",
						"WRITE_DATA CUSTOM:cpu_axi_awaddr",
						"READ_DATA CUSTOM:cpu_axi_araddr"
			]
	
		
	[INTERCONNECT.DYNAMIC."MODULE:cache:cpu"]
		GROUP_SELECT = "CUSTOM:reprogram"
		HANDSHAKES = ["WRITE_ADDRESS", "WRITE_DATA", "READ_ADDRESS", "READ_DATA"]
		[[INTERCONNECT.DYNAMIC."MODULE:cache:cpu".GROUPS]]
			SELECT_VALUE = 0
			INTERCONNECT_TYPE = "ONE_TO_ONE"
			INTERFACE = "MODULE:cpu:mem"
		[[INTERCONNECT.DYNAMIC."MODULE:cache:cpu".GROUPS]]
			SELECT_VALUE = 1
			INTERCONNECT_TYPE = "ONE_TO_ONE"
			INTERFACE = "MODULE:programmer:a"


	[INTERCONNECT.DYNAMIC."MODULE:debug:a"]
		GROUP_SELECT = ""
		HANDSHAKES = ["WRITE_ADDRESS", "WRITE_DATA", "READ_ADDRESS", "READ_DATA"]
		[[INTERCONNECT.DYNAMIC."MODULE:debug:a".GROUPS]]
			INTERCONNECT_TYPE = "ONE_TO_ONE"
			INTERFACE = "MODULE:cpu:mem"
			

	[INTERCONNECT.DYNAMIC."MODULE:timer:a"]
		GROUP_SELECT = ""
		HANDSHAKES = ["WRITE_ADDRESS", "WRITE_DATA", "READ_ADDRESS", "READ_DATA"]
		[[INTERCONNECT.DYNAMIC."MODULE:timer:a".GROUPS]]
			INTERCONNECT_TYPE = "ONE_TO_ONE"
			INTERFACE = "MODULE:cpu:mem"
			

	[INTERCONNECT.DYNAMIC."MODULE:programmer:a"]
		GROUP_SELECT = ""
		HANDSHAKES = ["WRITE_ADDRESS", "WRITE_DATA"]
		[[INTERCONNECT.DYNAMIC."MODULE:programmer:a".GROUPS]]
			INTERCONNECT_TYPE = "ONE_TO_ONE"
			INTERFACE = "MODULE:cache:cpu"
```


## Step 5: Update the softcore source files
Remove the following lines from `simple_mnist/src/utils.h`
```c
extern int gpio asm ("GPIO");
```

```c
extern int i2cbus asm ("I2CBUS");
```

```c
void i2cbus_write(uint8_t dev_addr, uint8_t data_addr, uint8_t data){
  i2cbus = (data << 16) | (data_addr << 8) | dev_addr | 0;
}

uint8_t i2cbus_read(uint8_t dev_addr, uint8_t data_addr){
  i2cbus = (data_addr << 8) | dev_addr | 1;
  return(i2cbus & 0xFF);
}
```


Replace the softcore code given in `infrastructure_gen/examples/finn/simple_mnist/src/cpu_test.c` with the following code that reads test vectors from `test.h`, drives the PCPI interface to push these test vectors to the MNIST inference model, and then reads and prints the result on the host machine (over UART). Note that we use `__asm__` to specify our custom instructions with the opcode 0x2B (43 in decimal).  

```c
#include <stdarg.h> 
#include <stdint.h>
#include <stddef.h>
#include "utils.h"
#include "test.h"
int main( )
{
	uint32_t rs1 = 0;
	uint32_t rs2 = 0;
	uint32_t rd;
	uint32_t valid = 0;
	__asm__ (".insn r 0x2B , 0x0, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
	delay(100);
	printf("Detected\tExpected\n\r");
	for (int test = 0; test < NUM_TESTS; test++){
		for (int i = 0; i < 16; i++){
			for (int j = 0; j < 7; j++){
				rs2 = rows[test][i][j*2+0];
				rs1 = rows[test][i][j*2+1];
			 	__asm__ (".insn r 0x2B , 0x2, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
			}
			__asm__ (".insn r 0x2B , 0x3, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
			valid = 0;
			while (!valid){
				__asm__ (".insn r 0x2B , 0x1, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
				valid = rd&512;
			}
		}
		valid = 0;
		while (!valid){
			__asm__ (".insn r 0x2B , 0x1, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
			valid = rd&256;
		}
		__asm__ (".insn r 0x2B , 0x4, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
		printf("%d\t%d\n\r", rd&255,results[test]);
		delay(10);
	}
	while(1);
}
```


Finally, we'll create a parser script that can read the test vectors given as a HEX file (`test.hex`) and create a header file (`test.h`). The contents of `test.hex` are given at the end of the tutorial (these were generated by the FINN MNIST example) - create the file `infrastructure_gen/examples/finn/simple_mnist/src/test.hex` and copy the [test vectors](#testhex) to it. 

Also, create the file `infrastructure_gen/examples/finn/simple_mnist/src/testparse.py` and copy the following code for the test parser to it. 

```python
with open('test.hex') as f:
    hexdata = f.readlines()
hexdigits=['0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F']
hexdata2 = []
results = []
frame = ''
counter = 0
for i in range(len(hexdata)):
    if counter == 28:
        counter = 0
        line = hexdata[i][:-1]
        results.append(line[-1])
    else:
        counter += 1
        line = hexdata[i][:-1]
        for j in range(len(line)):
            idx = len(line)-j-1
            frame = line[idx] + frame
            if len(frame) == 49*2:
                frame = '00000000000000' + frame
                hexdata2.append(frame)
                frame = ''
counter = 0
counter2 = 0
counter3 = 0
print('uint32_t rows[][16][14] = {\n{',end='')
for frame in hexdata2:
    if counter3 == 16:
        print(',\n{',end='')
        counter3 = 0
    print("{",end='')
    for c in frame:
        if counter == 0:
            print('0x',end='')
        print(c,end='')
        counter += 1
        if counter == 8:
            if counter2 < 13:
                print(',',end='')
            counter2 += 1
            counter = 0
    counter = 0
    counter2 = 0
    print("}",end='')
    counter3 += 1
    if counter3 == 16:
        print("}",end='')
    else:
        print(',')
print("};")
print("uint8_t results[]= {" + ",".join(results) + "};")
print("#define NUM_TESTS " + str(len(results)))
```

## Step 6: Update the host python files
Since we'll be using FINN to create the Vivado project, we'll move the resulting compiled FPGA binary to the project build directory (using a tcl script shown later) - this way any changes in the FINN naming convention will not break this file. We'll also change the message being printed on the console. 

To do this we'll modify `infrastructure_gen/examples/finn/simple_mnist/src/test.py` so that:


`bin_file = './edgetestbed_jtag_uartprog_no_dram/edgetestbed_jtag_uartprog_no_dram.runs/impl_1/top.bin'` -> `bin_file = './top.bin'`


and 


`print("Starting temperature capture")` -> `print("Starting UART monitor")`


## Step 7: Patch FINN
Since FINN runs in interactive mode out of the box, we'll modify it to run in a detached mode so that we can integrate it into our overall DISL flow. Create the file `infrastructure_gen/examples/finn/simple_mnist/src/run-docker.sh` and copy the code given [here](#run-dockersh) to it. We'll use the project build script (shown later) to copy this file over to the FINN directory. 

## Step 8: Create a custom Vivado compilation script
While the DISL configuration script (`infrastructure_gen/configure.py`) automatically generates a Vivado compilation script, we'll need a custom one since we'll be adding to the FINN created Vivado project rather than creating a new one. A simple way to do this is to create a custom tcl file and put that in the example's `src` directory. When the system builder is run, all files in this directory are copied over the build directory. Create the file `infrastructure_gen/examples/finn/simple_mnist/src/merge.tcl` and add the following code to it. This script will open the FINN generated project, update the constraints and source files, compile the project and finally copy the `.bin` file to the build directory. 

```tcl
open_project stitched_ip/finn_vivado_stitch_proj.xpr
add_files -fileset constrs_1 ./constraints.xdc
add_files -files [glob *.v *.vh]
update_compile_order -fileset sources_1
set_property top top [current_fileset]
update_compile_order -fileset sources_1
reset_run synth_1
launch_runs synth_1 -jobs 24 
wait_on_run synth_1
set_property STEPS.WRITE_BITSTREAM.ARGS.BIN_FILE true [get_runs impl_1]
launch_runs -to_step write_bitstream impl_1 -jobs 24
wait_on_run impl_1
exec cp stitched_ip/finn_vivado_stitch_proj.runs/impl_1/top.bin .
```

## Step 9: Create a custom build script
The default DISL flow creates a project build script that simply runs the Vivado compilation scripts. However, since we have to run FINN first, we'll create a custom one and add that to the `infrastructure_gen/examples/finn/simple_mnist/src` directory as well. Create the file `infrastructure_gen/examples/finn/simple_mnist/src/finn_run.sh` and add the following code to it. This will clone the FINN repo, patch it, run the container, modify the target part number of the MNIST example, copy over the generated files, run the script from Step 8, remove the container and finally generate the test vectors. At the end of this script, we'll have the `.bin` needed to reconfigure the FPGA. 

```bash
FINN_ROOT="$PWD/finn"
CWD=$PWD
if [ -d "$FINN_ROOT" ]; then
    echo "Directory exists: $FINN_ROOT"
else
    git clone -b dev https://github.com/asanaullah/finn_fedora $FINN_ROOT
fi
cp run-docker.sh finn/run-docker.sh
cd finn && ./run-docker.sh
cd $CWD
docker exec $(docker ps -qf "ancestor=xilinx/finn:xilinx_finn") /bin/bash -c 'source $FINN_XILINX_PATH/Vitis_HLS/$FINN_XILINX_VERSION/settings64.sh && cd /home/finn_user/finn/tutorials/fpga_flow && python build.py xc7a35tcpg236-1'
docker cp $(docker ps -qf "ancestor=xilinx/finn:xilinx_finn"):/home/finn_user/finn/tutorials/fpga_flow/output_tfc_w1a1_fpga/stitched_ip .
docker cp $(docker ps -qf "ancestor=xilinx/finn:xilinx_finn"):/tmp/finn_dev_$USER /tmp
vivado -nojournal -nolog -mode batch -source ./merge.tcl
docker rm -f $(docker ps -qf "ancestor=xilinx/finn:xilinx_finn")
python testparse.py > test.h
```

## Step 10: Build and run the example
At this point, our example is ready to build. To generate the system using DISL and start building, navigate to the `infrastructure_gen` directory and run the following commands.
```bash
mkdir -p build
python configure.py --example_dir ./examples/finn --example simple_mnist --board cmoda735t --build_dir build/simple_mnist
cd build/simple_mnist
source finn_run.sh
```

If this is the first time running the example, it will take some time to build the FINN image. Once the script finishes, compile the softcore code and run the example. 

```bash
make
python test.py
```

If everything was set up correctly, it will print the following:

```bash
Initializing FTDI connection
Configuring the UART
Reconfiguring the FPGA
Done
Programming the softcore
Done
Starting UART capture
Detected	Expected
7	7
2	2
1	1
0	0
4	4
1	1
4	4
9	9
6	5
9	9
0	0
6	6
9	9
0	0
1	1
5	5
9	9
7	7
3	3
4	4
```



___
## run-docker.sh
```bash
#!/bin/bash
# Copyright (c) 2020-2022, Xilinx, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of FINN nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# green echo
gecho () {
  echo -e "${GREEN}$1${NC}"
}

# red echo
recho () {
  echo -e "${RED}$1${NC}"
}

if [ -z "$FINN_XILINX_PATH" ];then
  recho "Please set the FINN_XILINX_PATH environment variable to the path to your Xilinx tools installation directory (e.g. /opt/Xilinx)."
  recho "FINN functionality depending on Vivado, Vitis or HLS will not be available."
fi

if [ -z "$FINN_XILINX_VERSION" ];then
  recho "Please set the FINN_XILINX_VERSION to the version of the Xilinx tools to use (e.g. 2020.1)"
  recho "FINN functionality depending on Vivado, Vitis or HLS will not be available."
fi

if [ -z "$PLATFORM_REPO_PATHS" ];then
  recho "Please set PLATFORM_REPO_PATHS pointing to Vitis platform files (DSAs)."
  recho "This is required to be able to use Alveo PCIe cards."
fi

DOCKER_GID=1001
DOCKER_GNAME=$(id -gn)
DOCKER_UNAME=$(id -un)
DOCKER_UID=1001
DOCKER_PASSWD="finn"
DOCKER_INST_NAME="finn_dev_${DOCKER_UNAME}"
# ensure Docker inst. name is all lowercase
DOCKER_INST_NAME=$(echo "$DOCKER_INST_NAME" | tr '[:upper:]' '[:lower:]')
# Absolute path to this script, e.g. /home/user/bin/foo.sh
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in, thus /home/user/bin
SCRIPTPATH=/home/finn_user/finn

# the settings below will be taken from environment variables if available,
# otherwise the defaults below will be used
: ${JUPYTER_PORT=8888}
: ${JUPYTER_PASSWD_HASH=""}
: ${NETRON_PORT=8081}
: ${LOCALHOST_URL="localhost"}
: ${PYNQ_USERNAME="xilinx"}
: ${PYNQ_PASSWORD="xilinx"}
: ${PYNQ_BOARD="Pynq-Z1"}
: ${PYNQ_TARGET_DIR="/home/xilinx/$DOCKER_INST_NAME"}
: ${NUM_DEFAULT_WORKERS=4}
: ${FINN_SSH_KEY_DIR="$SCRIPTPATH/ssh_keys"}
: ${ALVEO_USERNAME="alveo_user"}
: ${ALVEO_PASSWORD=""}
: ${ALVEO_BOARD="U250"}
: ${ALVEO_TARGET_DIR="/tmp"}
: ${PLATFORM_REPO_PATHS="/opt/xilinx/platforms"}
: ${XRT_DEB_VERSION="xrt_202220.2.14.354_22.04-amd64-xrt"}
: ${FINN_HOST_BUILD_DIR="/tmp/$DOCKER_INST_NAME"}
: ${FINN_DOCKER_TAG="xilinx/finn:xilinx_finn"}
: ${FINN_DOCKER_PREBUILT="0"}
: ${FINN_DOCKER_RUN_AS_ROOT="0"}
: ${FINN_DOCKER_GPU="$(docker info | grep nvidia | wc -m)"}
: ${FINN_DOCKER_EXTRA=""}
: ${FINN_SKIP_DEP_REPOS="0"}
: ${OHMYXILINX="$SCRIPTPATH/deps/oh-my-xilinx"}
: ${NVIDIA_VISIBLE_DEVICES=""}
: ${DOCKER_BUILDKIT="1"}

DOCKER_INTERACTIVE=""

# Catch FINN_DOCKER_EXTRA options being passed in without a trailing space
FINN_DOCKER_EXTRA+=" "

if [ "$1" = "test" ]; then
  gecho "Running test suite (all tests)"
  DOCKER_CMD="pytest"
elif [ "$1" = "quicktest" ]; then
  gecho "Running test suite (non-Vivado, non-slow tests)"
  DOCKER_CMD="quicktest.sh"
elif [ "$1" = "notebook" ]; then
  gecho "Running Jupyter notebook server"
  if [ -z "$JUPYTER_PASSWD_HASH" ]; then
    JUPYTER_PASSWD_ARG=""
  else
    JUPYTER_PASSWD_ARG="--NotebookApp.password='$JUPYTER_PASSWD_HASH'"
  fi
  DOCKER_CMD="jupyter notebook --allow-root --no-browser --ip=0.0.0.0 --port $JUPYTER_PORT $JUPYTER_PASSWD_ARG notebooks"
  FINN_DOCKER_EXTRA+="-e JUPYTER_PORT=$JUPYTER_PORT "
  FINN_DOCKER_EXTRA+="-e NETRON_PORT=$NETRON_PORT "
  FINN_DOCKER_EXTRA+="-p $JUPYTER_PORT:$JUPYTER_PORT "
  FINN_DOCKER_EXTRA+="-p $NETRON_PORT:$NETRON_PORT "
elif [ "$1" = "build_dataflow" ]; then
  BUILD_DATAFLOW_DIR=$(readlink -f "$2")
  FINN_DOCKER_EXTRA+="-v $BUILD_DATAFLOW_DIR:$BUILD_DATAFLOW_DIR "
  DOCKER_INTERACTIVE="-it"
  #FINN_HOST_BUILD_DIR=$BUILD_DATAFLOW_DIR/build
  gecho "Running build_dataflow for folder $BUILD_DATAFLOW_DIR"
  DOCKER_CMD="build_dataflow $BUILD_DATAFLOW_DIR"
elif [ "$1" = "build_custom" ]; then
  BUILD_CUSTOM_DIR=$(readlink -f "$2")
  FLOW_NAME=${3:-build}
  FINN_DOCKER_EXTRA+="-v $BUILD_CUSTOM_DIR:$BUILD_CUSTOM_DIR -w $BUILD_CUSTOM_DIR "
  DOCKER_INTERACTIVE="-it"
  #FINN_HOST_BUILD_DIR=$BUILD_DATAFLOW_DIR/build
  gecho "Running build_custom: $BUILD_CUSTOM_DIR/$FLOW_NAME.py"
  DOCKER_CMD="python -mpdb -cc -cq $FLOW_NAME.py"
elif [ -z "$1" ]; then
   gecho "Running container only"
   DOCKER_CMD=""
   DOCKER_INTERACTIVE="-d -it"
else
  gecho "Running container with passed arguments"
  DOCKER_CMD="$@"
fi


if [ "$FINN_DOCKER_GPU" != 0 ];then
  gecho "nvidia-docker detected, enabling GPUs"
  if [ ! -z "$NVIDIA_VISIBLE_DEVICES" ];then
    FINN_DOCKER_EXTRA+="--runtime nvidia -e NVIDIA_VISIBLE_DEVICES=$NVIDIA_VISIBLE_DEVICES "
  else
    FINN_DOCKER_EXTRA+="--gpus all "
  fi
fi

VIVADO_HLS_LOCAL=$VIVADO_PATH
VIVADO_IP_CACHE=$FINN_HOST_BUILD_DIR/vivado_ip_cache

# ensure build dir exists locally
mkdir -p $FINN_HOST_BUILD_DIR
mkdir -p $FINN_SSH_KEY_DIR

gecho "Docker container is named $DOCKER_INST_NAME"
gecho "Docker tag is named $FINN_DOCKER_TAG"
gecho "Mounting $FINN_HOST_BUILD_DIR into $FINN_HOST_BUILD_DIR"
gecho "Mounting $FINN_XILINX_PATH into $FINN_XILINX_PATH"
gecho "Port-forwarding for Jupyter $JUPYTER_PORT:$JUPYTER_PORT"
gecho "Port-forwarding for Netron $NETRON_PORT:$NETRON_PORT"
gecho "Vivado IP cache dir is at $VIVADO_IP_CACHE"
gecho "Using default PYNQ board $PYNQ_BOARD"


# Check if the FINN Docker image already exists
if [ "$(docker images -q $FINN_DOCKER_TAG 2> /dev/null)" == "" ]; then
  echo "Building the FINN Docker image"
  docker build -f docker/Dockerfile.finn --build-arg XRT_DEB_VERSION=$XRT_DEB_VERSION --tag=$FINN_DOCKER_TAG .
else
  echo "The FINN Docker image with tag $FINN_DOCKER_TAG already exists. Skipping build."
fi

# Launch container with current directory mounted
# important to pass the --init flag here for correct Vivado operation, see:
# https://stackoverflow.com/questions/55733058/vivado-synthesis-hangs-in-docker-container-spawned-by-jenkins
DOCKER_EXEC="docker run -t $DOCKER_INTERACTIVE --tty --init "
DOCKER_EXEC+="--hostname $DOCKER_INST_NAME "
DOCKER_EXEC+="-e SHELL=/bin/bash "
DOCKER_EXEC+="-w $SCRIPTPATH "
#DOCKER_EXEC+="-v $FINN_HOST_BUILD_DIR:$FINN_HOST_BUILD_DIR "
DOCKER_EXEC+="-e FINN_BUILD_DIR=$FINN_HOST_BUILD_DIR "
DOCKER_EXEC+="-e FINN_ROOT="$SCRIPTPATH" "
DOCKER_EXEC+="-e LOCALHOST_URL=$LOCALHOST_URL "
DOCKER_EXEC+="-e VIVADO_IP_CACHE=$VIVADO_IP_CACHE "
DOCKER_EXEC+="-e PYNQ_BOARD=$PYNQ_BOARD "
DOCKER_EXEC+="-e PYNQ_IP=$PYNQ_IP "
DOCKER_EXEC+="-e PYNQ_USERNAME=$PYNQ_USERNAME "
DOCKER_EXEC+="-e PYNQ_PASSWORD=$PYNQ_PASSWORD "
DOCKER_EXEC+="-e PYNQ_TARGET_DIR=$PYNQ_TARGET_DIR "
DOCKER_EXEC+="-e OHMYXILINX=$OHMYXILINX "
DOCKER_EXEC+="-e NUM_DEFAULT_WORKERS=$NUM_DEFAULT_WORKERS "
# Workaround for FlexLM issue, see:
# https://community.flexera.com/t5/InstallAnywhere-Forum/Issues-when-running-Xilinx-tools-or-Other-vendor-tools-in-docker/m-p/245820#M10647
DOCKER_EXEC+="-e LD_PRELOAD=/usr/lib64/libudev.so.1 "
if [ "$FINN_DOCKER_RUN_AS_ROOT" = "0" ];then
  #DOCKER_EXEC+="-v /etc/group:/etc/group:ro "
  #DOCKER_EXEC+="-v /etc/passwd:/etc/passwd:ro "
  #DOCKER_EXEC+="-v /etc/shadow:/etc/shadow:ro "
  #DOCKER_EXEC+="-v /etc/sudoers.d:/etc/sudoers.d:ro "
  DOCKER_EXEC+="-v $FINN_SSH_KEY_DIR:$HOME/.ssh "
  DOCKER_EXEC+="--user $DOCKER_UID:$DOCKER_GID "
else
  DOCKER_EXEC+="-v $FINN_SSH_KEY_DIR:/root/.ssh "
fi
if [ ! -z "$IMAGENET_VAL_PATH" ];then
  DOCKER_EXEC+="-v $IMAGENET_VAL_PATH:$IMAGENET_VAL_PATH "
  DOCKER_EXEC+="-e IMAGENET_VAL_PATH=$IMAGENET_VAL_PATH "
fi
if [ ! -z "$FINN_XILINX_PATH" ];then
  VIVADO_PATH="$FINN_XILINX_PATH/Vivado/$FINN_XILINX_VERSION"
  VITIS_PATH="$FINN_XILINX_PATH/Vitis/$FINN_XILINX_VERSION"
  HLS_PATH="$FINN_XILINX_PATH/Vitis_HLS/$FINN_XILINX_VERSION"
  DOCKER_EXEC+="-v $FINN_XILINX_PATH:$FINN_XILINX_PATH "
  if [ -d "$VIVADO_PATH" ];then
    DOCKER_EXEC+="-e "XILINX_VIVADO=$VIVADO_PATH" "
    DOCKER_EXEC+="-e VIVADO_PATH=$VIVADO_PATH "
  fi
  if [ -d "$HLS_PATH" ];then
    DOCKER_EXEC+="-e HLS_PATH=$HLS_PATH "
  fi
  if [ -d "$VITIS_PATH" ];then
    DOCKER_EXEC+="-e VITIS_PATH=$VITIS_PATH "
  fi
  if [ -d "$PLATFORM_REPO_PATHS" ];then
    DOCKER_EXEC+="-v $PLATFORM_REPO_PATHS:$PLATFORM_REPO_PATHS "
    DOCKER_EXEC+="-e PLATFORM_REPO_PATHS=$PLATFORM_REPO_PATHS "
    DOCKER_EXEC+="-e ALVEO_IP=$ALVEO_IP "
    DOCKER_EXEC+="-e ALVEO_USERNAME=$ALVEO_USERNAME "
    DOCKER_EXEC+="-e ALVEO_PASSWORD=$ALVEO_PASSWORD "
    DOCKER_EXEC+="-e ALVEO_BOARD=$ALVEO_BOARD "
    DOCKER_EXEC+="-e ALVEO_TARGET_DIR=$ALVEO_TARGET_DIR "
  fi
fi
DOCKER_EXEC+="$FINN_DOCKER_EXTRA "
DOCKER_EXEC+="$FINN_DOCKER_TAG $DOCKER_CMD"
$DOCKER_EXEC
```


## test.hex
```hex
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000243C979FB954000000000000
00000000000034AAC6C6C6C6C6C6C6C6F1FEFEFEFEDE000000000000
0000000000008CFEFEE5FAFEFEFEE1FEE3A372487243000000000000
0000000000006AFEEC153B4343430E42110000000000000000000000
00000000000012D1FD53000000000000000000000000000000000000
0000000000000053FFE9160000000000000000000000000000000000
000000000000002CEEFE810000000000000000000000000000000000
00000000000000003EFEF93B00000000000000000000000000000000
000000000000000005BBFE8500000000000000000000000000000000
0000000000000000003AF8CD09000000000000000000000000000000
00000000000000000000B6FE7E000000000000000000000000000000
0000000000000000000039F0FB4B0000000000000000000000000000
0000000000000000000000A6FEDD1300000000000000000000000000
000000000000000000000023DBFECB03000000000000000000000000
0000000000000000000000004DFEFE26000000000000000000000000
0000000000000000000000000173FEE01F0000000000000000000000
0000000000000000000000000034FEFE850000000000000000000000
0000000000000000000000000034FEFEF23D00000000000000000000
0000000000000000000000000028DBFEFE7900000000000000000000
000000000000000000000000000012CFFE7900000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff07
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000005D96FFFFAB7D7400000000000000000000
000000000000000000001EDAFDFDFDFDFDFDA9000000000000000000
000000000000000000007AFDFDB08ED5FDFDFDA90000000000000000
000000000000000000008CFDCE06000C20D2FDFA3400000000000000
0000000000000000000041FDF87A00000019D2FB4D00000000000000
0000000000000000000041FDFDD100000000121F0000000000000000
000000000000000000000AC6FDF77500000000000000000000000000
00000000000000000000003FE7FDF74C000000000000000000000000
00000000000000000000000090FDFD80000000000000000000000000
0000000000000000000000000C9FFDF6B00000000000000000000000
0000000000000000000000000023E9FDEA1900000000000000000000
00000000000000000000000000008DFDFDC600000000000000000000
00000000000000000000000000000CBDFDF84E000000000000000000
0000000000000000000000000000008DFDFDC8130000000000000000
0000000000000000000000000000000CADFDFD860000000000000000
0000000000000000000000000000000019FDFDF80000000000000000
000A93969696251414050005141414142BFDFDF80000000000000000
007BFDFDFDFDFDFDFDA68FA8FDFDFDFDFDFDFDF80000000000000000
00397575A9F7F7F9FDFDFDFDFDFDFDFDFDFDFDAE0000000000000000
00000000000000297B7B9BFDFDFDA67B7B7B76000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff02
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
0000000000000000006DFE2600000000000000000000000000000000
00000000000000000052FC5700000000000000000000000000000000
00000000000000000000F18700000000000000000000000000000000
0000000000000000000096F42D000000000000000000000000000000
000000000000000000003FFE54000000000000000000000000000000
000000000000000000000BDFCA000000000000000000000000000000
0000000000000000000000D8FE200000000000000000000000000000
0000000000000000000000C3FE5F0000000000000000000000000000
00000000000000000000004DFE8C0000000000000000000000000000
000000000000000000000008CDED3900000000000000000000000000
000000000000000000000000A5FF7C00000000000000000000000000
00000000000000000000000051FEAB00000000000000000000000000
00000000000000000000000000D7E818000000000000000000000000
000000000000000000000000009FFE78000000000000000000000000
000000000000000000000000008EFE97000000000000000000000000
0000000000000000000000000042FEE4000000000000000000000000
0000000000000000000000000042FEFB3D0000000000000000000000
0000000000000000000000000003CDFE8D0000000000000000000000
000000000000000000000000000079FED70A00000000000000000000
00000000000000000000000000000AB0C60500000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff01
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000001FCAFD960B000000000000000000000000
00000000000000000000006BFDFBFB25000000000000000000000000
00000000000000000000006BFDFBFBC5150000000000000000000000
0000000000000000003E6DA9FDFBFBFBBE6E00000000000000000000
000000000000000033DCFBFBFDFBFBFBFBFD00000000000000000000
0000000000000000FDFDFDDEEAFDFDFDFDFFB6000000000000000000
0000000000000069FBFB803E4D93FBFBFBFDDD3F0000000000000000
00000000000571F3FBE61F00000A89DCFBFDFBE72000000000000000
000000000023FBFDFB6D000000000014BCFDFBFB2500000000000000
000000000023FBFDC81F0000000000001EC9FBFB2500000000000000
0000000000A4FDFFCA200000000000000000FDFD2500000000000000
000000000023FBFDFB6D0000000000000000FBFB8C00000000000000
00000000001EE6FDFBE73F15000000000000FBFBD900000000000000
0000000000003DDDFBFBFB90000000000000FBFBD900000000000000
00000000000000B4FBFBFBDDB60000000000FBFBD900000000000000
0000000000000000FDFDFDFDFFFDFDE44949FDFDDA00000000000000
000000000000000093FBFBFBFDFBFBFBFBFDFBFB7100000000000000
00000000000000000A23BDE6FDFBFBFBFBFDFBE61F00000000000000
00000000000000000000006BFDFBFBFBFBFD8E3E0000000000000000
00000000000000000000001E4847ADFBAE4800000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff00
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
000000000000001D4600000000000000E03200000000000000000000
00000000000000A89400000000000000E77900000000000000000000
0000000000000BD26000000000000000E7C304000000000000000000
00000000000015FC720000000000000086FC45000000000000000000
00000000000015FCC0000000000000000CD9EC2D0000000000000000
00000000000015FDFF120000000000000035F7A80000000000000000
00000000000005BDFD8D0000000000000000D3F25400000000000000
0000000000000042FAE820000000000000006AFCA900000000000000
0000000000000000D3FC860000000000000000FCE10F000000000000
0000000000000000A7FCA90000000000000000A4FC16000000000000
00000000000000006BFDFD1600000000000012D1CC09000000000000
00000000000000006AFCFCC3A48155555555C7FCA900000000000000
000000000000000009FCFCFBE7E8FCFCFCFCF5AA2900000000000000
000000000000000000FCFCA100005454545431000000000000000000
00000000000000002DFCFC7F00000000000000000000000000000000
000000000000000000FDFD8000000000000000000000000000000000
000000000000000000FCFC7F00000000000000000000000000000000
000000000000000000F4FC8700000000000000000000000000000000
0000000000000000006FECE800000000000000000000000000000000
0000000000000000000042B300000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff04
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
0000000000000000036BFE4D00000000000000000000000000000000
000000000000000009FEFEE313000000000000000000000000000000
000000000000000001A5FEFE51000000000000000000000000000000
00000000000000000049FEFECB070000000000000000000000000000
00000000000000000000FAFEFE350000000000000000000000000000
00000000000000000000B4FEFE860000000000000000000000000000
0000000000000000000030F8FEC40000000000000000000000000000
0000000000000000000000EDFEFE3A00000000000000000000000000
000000000000000000000084FEFE6F00000000000000000000000000
00000000000000000000001CEEFEA300000000000000000000000000
000000000000000000000000DFFEFC3C000000000000000000000000
0000000000000000000000009AFEFE4F000000000000000000000000
00000000000000000000000035EEFEA3000000000000000000000000
00000000000000000000000000D2FEFC1C0000000000000000000000
0000000000000000000000000083FEFE560000000000000000000000
0000000000000000000000000014EAFE690000000000000000000000
0000000000000000000000000005CCFEAF0000000000000000000000
0000000000000000000000000000C4FED30500000000000000000000
0000000000000000000000000000A0FE9E0300000000000000000000
00000000000000000000000000006B9D1A0000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff01
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000054D0F00000000000000002086C016000000000000000000
0000000025F1DC0F0000000000000000A9FAEB110000000000000000
000000000064FD8B00000000000000000093FDBD1400000000000000
00000000000DADFE2B000000000000000015FDFD4600000000000000
0000000000005CFEE72B000000000000000060FD9916000000000000
000000000000009EFE6800000000000000000BCCFFA3000000000000
0000000000000000FDED830900000000000005B2FDA2000000000000
0000000000000000A9FDFDC58546464646AFBFFDFDA2000000000000
000000000000000023DBFDFDFEFDFDFDFDFEFDFDE433000000000000
000000000000000000A1FDFD2C898989E8FE89411100000000000000
00000000000000000015CEFE22000000000000000000000000000000
0000000000000000000045FDA0000000000000000000000000000000
0000000000000000000032F1FE550000000000000000000000000000
0000000000000000000000A5FE9E0000000000000000000000000000
000000000000000000000032F4E70000000000000000000000000000
000000000000000000000000E8FE6800000000000000000000000000
0000000000000000001E0D009DFDD000000000000000000000000000
000000000000000000A1CC5B9AFDD000000000000000000000000000
0000000000000000001D9AFDFEFDD000000000000000000000000000
00000000000000000000061780BE3D00000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff04
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
0000000000000000000000000005C1950E0000000000000000000000
0000000000000000000000000013FDFDE05B00000000000000000000
00000000000000000000000012A6FDFDFEEB1C000000000000000000
000000000000000000000673EEFDFDFDFEFD90000000000000000000
0000000000000000000018E7FDFDFDB9D0FDF11F0000000000000000
00000000000000000012C9FFFEDB620800C1FE4F0000000000000000
0000000000000000000CBFFEFDB600000050FD560000000000000000
0000000000000000000087FEFDEA0000009BFDAF0000000000000000
0000000000000000002AECFEEDFBA65528D0FD560000000000000000
00000000000000000098FDD824B9FDFDFEFDEE120000000000000000
000000000000000023DFFE86000891FEFFF044000000000000000000
0000000000000000A1FDAF0900000C8E9E4400000000000000000000
0000000000000012E2FD580000000000000000000000000000000000
000000000000007EFDA6020000000000000000000000000000000000
00000000000026FDF530000000000000000000000000000000000000
000000000009ACFE7300000000000000000000000000000000000000
00000000002EFEDA1500000000000000000000000000000000000000
0000000000A5FE1E0000000000000000000000000000000000000000
000000002AF4BA000000000000000000000000000000000000000000
000000004EDF0E000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff09
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
0000002F5581102F2F2F110000000000000000000000000000000000
000000FDFDF6D7FDFDFDD9994B000000000000000000000000000000
000000FDFDFDFDFDFDFDFDFDFCF48E23000000000000000000000000
000000AAAAAAAAD5FDFDFDFDFDFDFD3F000000000000000000000000
000000000000000B14457CA8EEE3EE39004884140000000000000000
0000000000000000000000021E002000004EFDCE0B00000000000000
00000000000000000000000000000000000A84FDB106000000000000
0000000000000000000000000000000000000FE9FD850C0000000000
000000000000000000000000000000000000001CDFFD5C0000000000
0000000000000000000000000000000000000000AEFD960000000000
000000000000000000000000000000000000317FF6FDEA0000000000
000000000000000000001C552A2A55795B93FBFDFDFDFF0000000000
000000000000000000A8E8FDFDFDFDFDFDFDFDFDFDFD8B0000000000
00000000000000007CFCFDFDFDFDFDFDFDFDFBDEDA35030000000000
0000000000000000AFFDFDFDFDFDFDFDC84843000000000000000000
0000000000000000AFFDFDA43398F9FD780000000000000000000000
000000000000000094FDFDFCBCFDFDFD320000000000000000000000
00000000000000000BAFFAFDFDFDFDA7090000000000000000000000
0000000000000000000080DDFDE7B417000000000000000000000000
000000000000000000000016955D0000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff05
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
0000000000000000255FC7C989382400000000000000000000000000
00000000000697D3FAFEFEFEFEFEEA982D0000000000000000000000
000000000068E1E5FEC8FB85A6E3FEFEF0992E000000000000000000
0000000015FDDFF6C628BF0000088EBBFEFEEA990000000000000000
0000000015FEFEFE462BD2000000000B80E9FEFD7E08000000000000
0000000005A2FFFEF2E17420030000000036E4FEF348000000000000
000000000026E8FEFEFEE7FBD2A9B2B28A6DDFFEF04B000000000000
00000000000019ABFCFEFEFEFEFEFBFEFEFFFDF4AF09000000000000
000000000000001096FEFEFEFEC89992B0C388100000000000000000
00000000000000000363F1FEFEA20000000000000000000000000000
000000000000000000005AFEFEFA7600000000000000000000000000
0000000000000000000007D3FEFEF264000000000000000000000000
00000000000000000000003BF2FEFEF1360000000000000000000000
00000000000000000000000040F4FEFE830000000000000000000000
0000000000000000000000000098FEFEF90D00000000000000000000
0000000000000000000000000008D0FEFEE40C000000000000000000
000000000000000000000000000042FEFEFF4E000000000000000000
00000000000000000000000000000089FEFED1000000000000000000
00000000000000000000000000000019E9FFE3000000000000000000
000000000000000000000000000000006CFF71000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff09
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
0000000000000000003D7676C1762A033D0000000000000000000000
0000000000000054EBF5FEFEFEFEF2ECF5B30E000000000000000000
0000000000002EF1FEFEB4B2B2C0D5FEFEFE97000000000000000000
000000000011ADFFFC800200000C1C40E2FEEB2B0000000000000000
00000000004BFEFA86000000000000006BFDFE380000000000000000
00000000009DFEDD0000000000000000009EFE3F0000000000000000
0000000000D5FE9600000000000000000067FEC20000000000000000
0000000000D5FE540000000000000000003AEFDC2200000000000000
0000000000D5FE5400000000000000000000ABFE7E00000000000000
0000000000D5FE54000000000000000000003CEFD600000000000000
0000000000D5FE540000000000000000000000C7D600000000000000
0000000000D5FE540000000000000000000000C7DB0B000000000000
0000000000D1FEA20000000000000000000000C7FE62000000000000
00000000004BFEEE3300000000000000000000C7FE62000000000000
000000000004C3FEA533000000000000000000C7FE62000000000000
00000000000037E3FEA7030000000000000000C7F142000000000000
000000000000003FFEFECA982E000000000014D5D600000000000000
000000000000000A9CEAFEFEEBB4B4B4B4B4CCFED600000000000000
00000000000000000078EAFCFEFEFEFEFEFEFECD5100000000000000
00000000000000000000006899FEFEFEFEFED21A0000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff00
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
0000000000000000000000000000B0FDCC0700000000000000000000
00000000000000000000000000007DFCFC9607000000000000000000
00000000000000000000000000000038BAFC75000000000000000000
0000000000000000000000000000000076FC8D000000000000000000
0000000000000000000000000000000032F79A000000000000000000
0000000000000000000000000000000000C4FD1A0000000000000000
0000000000002655553900000000000000C4FD960000000000000000
00000000007DEEFCFCF3E297000000000060FDE10000000000000000
000000001FE4FFE1AFEAFFFDE53604000000E2E50A00000000000000
0000000038FCB200001C86E3FCFC801A000096FC6E00000000000000
0000000038FC8D000000002BBAFCFD96000071FC9F00000000000000
0000000006CA8D00000000000697FDED260071FCB900000000000000
0000000000C59A00000000000000A3FD930072FDC600000000000000
0000000000ABFD1A000000000000BCFCAC0071FCC500000000000000
000000000038F4C800000000137AF7E7130071FCC500000000000000
0000000000007DF9C84C000DC1FCCB19000071FCDE1A000000000000
000000000000007DF4FD9A1D234C0000000AB3FDB900000000000000
000000000000000051D6FDFCC583393952C4FDD11C00000000000000
000000000000000000139CFCFCFCFDFCFCFCD8190000000000000000
0000000000000000000000288B8B8CF08B6710000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff06
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
000000000000094C0B24A9FDFFFDB431000000000000000000000000
0000000000005CFDBDA0FCFCFDFCFCE4440500000000000000000000
00000000000043F7EC5A6445454FE3FCFC3700000000000000000000
0000000000000087FCCB1A00000032B9FCE92B000000000000000000
000000000000003FFCFC460000000025B2FDA8000000000000000000
0000000000000000BEFDBF05000000002AF2FD9B0000000000000000
000000000000000040FCFC880500000000E6FCCF0000000000000000
000000000000000010E3FCFC8A20000000E6FCCF0000000000000000
000000000000000000A0FCFCFDE4CFCFCFF9FCA50000000000000000
00000000000000000038FCA94BFCFCFCFCFDB3090000000000000000
00000000000000000015D7FD95004A74744000000000000000000000
00000000000000000000A2FCFD000000000000000000000000000000
0000000000000000000032F0FD200000000000000000000000000000
0000000000000000000000A4FD9D0000000000000000000000000000
00000000000000000000005CFDF02B00000000000000000000000000
000000000000000000000054FCFD5D00000000000000000000000000
000000000000000000000000D1FC7200000000000000000000000000
00000000000000000000000074FCCF00000000000000000000000000
00000000000000000000000074FCA500000000000000000000000000
0000000000000000000000003FC85D00000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff09
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
0000000000000000178AA9FDFF8A4211000000000000000000000000
00000000000000009EFCFCFCFDFCFCE4780500000000000000000000
0000000000000000FCFCFCFCBEFCFCFCFC6C00000000000000000000
0000000000000000FCFCFC870574FCFCFCE92B000000000000000000
00000000000000D2FCE8360500022BDDFCFDB22B0000000000000000
0000000000009AFFFB88000000000073F9FFFD5D0000000000000000
000000000000CEFDD100000000000000B9FDFCA60000000000000000
000000000000CEFD74000000000000005CFDFCDC1300000000000000
000000000019DFFD740000000000000011C0FCFC4600000000000000
000000000045FCFD7400000000000000003FFCFC7A00000000000000
000000000045FDFF74000000000000000000FDFD8400000000000000
000000000045FCFD74000000000000000000FCFCB800000000000000
000000000032F0FD74000000000000000000FCFCB800000000000000
00000000000070FDD2000000000000000000FCFCB800000000000000
00000000000008E8E600000000000000009EFCE83000000000000000
00000000000000A8FD9B00000000000032F4FD5D0000000000000000
000000000000002AE7EC42000000000071FDA4220000000000000000
000000000000000089FCEA5B26000086F0DE20000000000000000000
000000000000000023B0FCFCE967CFF0B11900000000000000000000
00000000000000000004368989FCB3360F0000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff00
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
000000000000000000000000BFFF8000000000000000000000000000
000000000000000000000040FFFFBF00000000000000000000000000
000000000000000000000080FFFFFF00000000000000000000000000
000000000000000000000000FFFFFF00000000000000000000000000
000000000000000000000000FFFFFF00000000000000000000000000
000000000000000000000080FFFF8000000000000000000000000000
000000000000000000000080FFFFFF00000000000000000000000000
000000000000000000000080FFFFFF00000000000000000000000000
000000000000000000000080FFFFFF00000000000000000000000000
000000000000000000000080FFFFFF00000000000000000000000000
000000000000000000000000FFFFFF00000000000000000000000000
000000000000000000000000FFFFFF00000000000000000000000000
000000000000000000000000FFFFFF00000000000000000000000000
000000000000000000000000FFFFFF00000000000000000000000000
000000000000000000000080FFFFBF00000000000000000000000000
000000000000000000000080FFFFBF00000000000000000000000000
000000000000000000000080FFFFFF40000000000000000000000000
000000000000000000000080FFFFFFBF000000000000000000000000
00000000000000000000000040FFFFFF400000000000000000000000
000000000000000000000000000080FF400000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff01
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
0000000000000029A2CBFDFEFDD68433000000000000000000000000
000000000000000000004697FCFDFCFDCBCB8E660000000000000000
0000000000000000000000000052668ECBF4FDFE0000000000000000
0000000000000000000000000000000000CBFCAC0000000000000000
000000000000000000000000000000001EEADF150000000000000000
0000000000000000000000000000000032FD7A000000000000000000
0000000000000000000000000A3333335BFE7B000000000000000000
000000000000000000000052ACFDFCFDFCFDDF150000000000000000
00000000000000000A33FDFEDFCB6666A2CBFDD61500000000000000
0000000000000029D4FDC070140000000000ABFD3D00000000000000
00000000000033EACB66000000000000000000000000000000000000
000000000052E8D51400000000000000000000000000000000000000
0000000070EACB3E0000000000000000000000000000000000000000
00000000FCD514000000000000000000000000000000000000000000
00000000FD9900000000000000000000000000000000000000000000
00000000D4E929000000000000000000000000000000000000000000
0000000028F4AD1F000000000000000000005C710000000000000000
000000000051D4E9B766661500000000000097FD5200000000000000
000000000000148ED5FEFDFEFDADC19998EAFDFF5200000000000000
00000000000000000A32328397C0D4FDE89797470000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff05
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
000000000000000000000078CDFFE592400000000000000000000000
0000000000000000000059FCEBD8E1FDFCC618000000000000000000
00000000000001265702AECE1D000F46DFFDCD140000000000000000
00000000000009FDFD4C1C230000000006E3FD890000000000000000
00000000000001AEFDEE2A0000000000000CEBFB5800000000000000
00000000000000A1FDEE0E00000000000000C0FD8600000000000000
000000000000004BFDF755000000000000004AFDA900000000000000
000000000000001FF1FDDB060000000000002FFDFA0A000000000000
00000000000000008FFDFD480500000000002FFDFD0A000000000000
00000000000000002FFDFDFD76190000000075FDDD06000000000000
000000000000000012E0FEEFF4DC9F9268BBFEF21C00000000000000
0000000000000000009DFDF7439CD7F8FDFDC94E0000000000000000
0000000000000000004AFDFD26000032383805000000000000000000
00000000000000000012FDFD42000000000000000000000000000000
00000000000000000012FDFD95000000000000000000000000000000
00000000000000000004BFFDEE1E0000000000000000000000000000
0000000000000000000070FDFD420000000000000000000000000000
0000000000000000000003F4FD570000000000000000000000000000
0000000000000000000000C6FDAA0000000000000000000000000000
000000000000000000000095FDFE0000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff09
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
0000000000000000000000003C501988A7A7F5FEFEFEF94A00000000
0000000000000000274771C5FCFEF9FEFEFEFEFEFEFEFE6800000000
0000000000000081F6FEFEFEFEFEE9C0C0C072696987630500000000
0000000000000FF0FEFEFECB72722D00000000000000000000000000
00000000000082FEFE9B230800000000000000000000000000000000
00000000000022F1FE24000000000000000000000000000000000000
00000000000076FEFE73000000000000000000000000000000000000
00000000000011F0FEF3220000000000000000000000000000000000
000000000000008BFEFE6F0000000000000000000000000000000000
0000000000000028F4FEF32500000000000000000000000000000000
000000000000000071FEFEB014000000000000000000000000000000
000000000000000002DCFEFE8C000000000000000000000000000000
0000000000000000002DF3FEFD580000000000000000000000000000
0000000000000000000053FEFEF13F00000000000000000000000000
000000000000000000000593FEFEF310000000000000000000000000
000000000000000000000005CBFEFE6F020000000000000000000000
00000000000000000000000054FEFEFE3A0000000000000000000000
00000000000000000000000004C2FFFEED0E00000000000000000000
000000000000000000000000001BC2FEFE5200000000000000000000
00000000000000000000000000001CC1E62700000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff07
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000309292290C000000000000
00000000000000000000000000000012A3FAFDFDFD810C0000000000
000000000000000000000000000046E5FDFDFDFDFDFD850000000000
0000000000000000000000000A80F7FDED6B6691FCFD650000000000
000000000000000000000005A3FDFDEB3D000000A7FDB50000000000
0000000000000000000004A4FDFDC13A000000002BFDFF0000000000
0000000000000000000056FDFDEC37000000000020FDBB0000000000
000000000000000000007BFDFD93575757BE640020FD920000000000
00000000000000000F54DFFDFDFDFDFDFDFDF8284EFD5E0000000000
0000000000000A59F4FDFDFDFDFDFDFDFDFDF0230C5C0E0000000000
00000000002BD1FDFDFDFDFDFDFDFDFDB3A14B000000000000000000
0000000030B9FDFDF391101026271010030000000000000000000000
00000000B7FDFDD13A00000000000000003A14000000000000000000
00000048F0FDDB0D00000000000000004FF7DD4D0000000000000000
000000BFFDFB35000000000000000039FCFDF75A0000000000000000
00000091FDFC6300000000000000003BFDFD74000000000000000000
00000091FDFDF6D36F00000000269EDDFDBC0E000000000000000000
00000013C8FDFDFDFDF9F9F9F9FBFDF6DD0C00000000000000000000
00000000177CC3FDFDFDFDFDFDE4B75F000000000000000000000000
0000000000000725587E4A8A25180000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff03
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000005BA8000000000000000000000000000000000000
00000000000000007EEA020000000000000000000000000000000000
00000000000000007EFE330000000000000000000000000000000000
000000000000000051FE3300000000001FB226000000000000000000
000000000000000036FE57000000000053FE5E000000000000000000
000000000000000004EEBD000000000038FEA0000000000000000000
000000000000000000ECC2000000000002A8E30D0000000000000000
000000000000000000A7EB10000000000072FE370000000000000000
00000000000000000069FE67000000000032FE730000000000000000
00000000000000000072FEFDFCFCBEB49C4BECDD0700000000000000
00000000000000005EF7FEF6B3B3B3D3FCFEFEFE0F00000000000000
0000000000000000C5F2FEE2000000001675EFD90800000000000000
0000000000000000252ECFF31B0000000000120E0000000000000000
0000000000000000000084FE63000000000000000000000000000000
0000000000000000000043FE74000000000000000000000000000000
000000000000000000003DFE74000000000000000000000000000000
000000000000000000003DFE74000000000000000000000000000000
0000000000000000000064FFAE000000000000000000000000000000
0000000000000000000053FEBB000000000000000000000000000000
000000000000000000000AB073000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000
ffffffffffffffffffffffffffffffffffffffffffffffffffffff04

```








