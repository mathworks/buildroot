#!/bin/bash

PLATFORM="zynq"
OPERATING_SYS="linux"
TOOLCHAIN="xilinx"
BOARD="zc702"
BUILDALL="false"

# List of valid boards
BOARD_LIST="zc702 zc706 zed uzed-7z010 uzed-7z020 mitx-7z045 mitx-7z100 zybo"
# List of valid os
OS_LIST="linux xenomai"
# List of valid toolchains
TC_LIST="linaro xilinx"

checkconfig() {

	# Test for incompatibilities
	if [ ${OPSYS} == "xenomai"  -a ${TOOLCHAIN} == "xilinx" ]; then
		echoerr "Cannot use Xilinx toolchain with Xenomai OS"
		exit 1
	fi
}



