# DISL Infrastructure Generator

## Requirements
- Vivado (tested with 2020.1)
- Python 3.x (tested with 3.11.4)
- Xilinx 7 series FPGA board with a FT2232H based programmer (Cmod A735t and Arty A735t supported out of box)
- Python libraries given in ```requirements.txt```

## Quick start
To generate, compile and deploy the hardware for an edge testbed example system, run the following command from the repo's root directory 
```console
python configure.py --example_dir ./examples/edgetestbed --example <example> --board <cmoda735t|artya735t> --build_dir build
cd build
source run.sh
make
python test.py
```

If using a precompiled binary:
```console
python configure.py --example_dir ./examples/edgetestbed --example <example> --board <cmoda735t|artya735t> --build_dir build
cd build
mkdir -p <example>/<example>.runs/impl_1
cp ../examples/edgetestbed/precompiled_binaries/<example>.bin <example>/<example>.runs/impl_1/top.bin
make
python test.py
```
## Detailed documentation coming soon
