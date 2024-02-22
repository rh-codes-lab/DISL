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
docker exec $(docker ps -qf "ancestor=xilinx/finn:xilinx_finn") /bin/bash -c 'source /opt/Xilinx/Vitis_HLS/2022.2/settings64.sh && cd /home/finn_user/finn/tutorials/fpga_flow && python build.py xc7a35tcpg236-1'
docker cp $(docker ps -qf "ancestor=xilinx/finn:xilinx_finn"):/home/finn_user/finn/tutorials/fpga_flow/output_tfc_w1a1_fpga/stitched_ip .
docker cp $(docker ps -qf "ancestor=xilinx/finn:xilinx_finn"):/tmp/finn_dev_$USER /tmp
vivado -nojournal -nolog -mode batch -source ./merge.tcl
docker rm -f $(docker ps -qf "ancestor=xilinx/finn:xilinx_finn")
python testparse.py > test.h

