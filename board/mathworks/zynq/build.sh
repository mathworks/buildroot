#!/bin/bash
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

PLATFORM="zynq"
OPERATING_SYS="linux"
TOOLCHAIN="xilinx"
BOARD="zc702"
BUILDALL="false"

# List of valid boards
BOARD_LIST=$(cd ${SCRIPT_DIR}/boards; ls -d * | tr -d /)
# List of valid os
OS_LIST="linux xenomai"
# List of valid toolchains
TC_LIST="xilinx linaro"

checkconfig() {

	# Test for incompatibilities
	if [ ${OPSYS} == "xenomai"  -a ${TOOLCHAIN} == "xilinx" ]; then
		echoerr "Cannot use Xilinx toolchain with Xenomai OS"
		exit 1
	fi
}



