#!/bin/bash
# Syntax: atfirmware-xilinx.sh <BINARIES_DIR> <readelf path> <mkimage path> <mkimage arch>
BINARIES_DIR=$1
READELF=$2
MKIMAGE=$3
MKIMAGE_ARCH=$4


# Get the entry point address from the elf.
BL31_BASE_ADDR=$(${READELF} -h ${BINARIES_DIR}/bl31.elf | egrep -m 1 -i "entry point.*?0x" | sed -r 's/.*?(0x.*?)/\1/g')
set -x
${MKIMAGE} -A ${MKIMAGE_ARCH} -O linux -T kernel -C none -a $BL31_BASE_ADDR -e $BL31_BASE_ADDR -d ${BINARIES_DIR}/bl31.bin ${BINARIES_DIR}/atf-uboot.ub


