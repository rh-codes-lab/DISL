# Tested with Vivado 2020.1
# Known issue with MIG IP generation in 2021.2

import os
import sys
import toml
import json
import argparse
########################### Get arguments ###########################
parser = argparse.ArgumentParser( prog = 'configure', description='Parse arguments and build project')
parser.add_argument('--example_dir', action='store', help='Specify the target project directory') 
parser.add_argument('--example', action='store', help='Specify the target project') 
parser.add_argument('--build_dir', action='store',help='Specify the build directory') 
parser.add_argument('--board', action='store',help='Target FPGA Board - use the short name (e.g. artya35t)') 
parser.add_argument('-v', action='store_true', help='Print debug information')
args = parser.parse_args()
########################### Initializations ###########################
build_dir= str(args.build_dir) if args.build_dir else ("./build")  
example= str(args.example) if args.example else "edgetestbed"
example_dir = str(args.example_dir) if args.example_dir else "./edgetestbed"
board= str(args.board) if args.board else "artya735t"
verbose= 1 if args.v else 0
########################## Define Logger and error handler ##########################
def logger(msg):
    global verbose
    if verbose:
        print(msg)
def error(msg):
    print("Error! " + msg)
    print ("Exiting")
    exit()
####################### Log build arguments ###########################
if not example:
    error("No example specified")
logger("System: " + example_dir + "/" + example)
logger ("Build Dir: " + build_dir)
logger ("Board: " + board)
logger ("Verbose: " + str(verbose))
####################### Open example###########################
try:
    with open(f"./{example_dir}/{example}/system.tml") as f:
        system = toml.load(f)
except:
    error("Example not found")
####################### Check if board is supported ###################
if board not in system["REQUIREMENTS"]["BOARDS"]:
    error(f"Error: The {board} FPGA board is not supported by {example}\n" +  "Valid boards are: " + str(system["REQUIREMENTS"]["BOARDS"]))
else:
    logger("Board supported - continuing")
####################### Open board###########################
try:
    with open(f"./fpga/boards/{board}/config/board.tml") as f:
        board = toml.load(f)
except:
    error("Board not found")
######################## Create build directory #################
try:
    os.mkdir(build_dir)
except:
    logger("Build directory already exists")
######################## Generate system files #######################
logger("Generating system and copying files")
os.system(f"python ./fpga/system_builder/build.py ./{example_dir}/{example}/system.tml " + board["DESCRIPTION"]["DIRECTORY"] + f" {build_dir}/")
###################### Generate additional tcl scripts ######################
project_tcl = ""
project_tcl += f"create_project -force {example} ./{example}/ -part " + board["DESCRIPTION"]["PART"]["LONG"] + "\n"
project_tcl +=  "\nadd_files -fileset constrs_1 ./constraints.xdc\nadd_files -scan_for_includes .\nupdate_compile_order -fileset sources_1\nset_property top top [current_fileset]\nupdate_compile_order -fileset sources_1\n"
with open(build_dir + "/ip.tcl",'r') as f:
    project_tcl += f.read()
with open(build_dir + "/create_project.tcl",'w') as f:
    f.write(project_tcl)

compile_tcl = ""
compile_tcl += f"open_project  ./{example}/{example}.xpr\n"
compile_tcl += "update_compile_order -fileset sources_1\n"
compile_tcl += "reset_run synth_1\n"
compile_tcl += "launch_runs synth_1 -jobs 24 \n"
compile_tcl += "wait_on_run synth_1\n"
compile_tcl += "set_property STEPS.WRITE_BITSTREAM.ARGS.BIN_FILE true [get_runs impl_1]\n"
compile_tcl += "launch_runs -to_step write_bitstream impl_1 -jobs 24\n"
compile_tcl += "wait_on_run impl_1\n"
with open(build_dir + "/compile_project.tcl",'w') as f:
    f.write(compile_tcl)
###################### Generate bash script ######################
run = "vivado -nojournal -nolog -mode batch -source ./create_project.tcl \n"
run += "vivado -nojournal -nolog -mode batch -source ./compile_project.tcl \n"
with open(build_dir + "/run.sh",'w') as f: 
    f.write(run)
print ("Done")
