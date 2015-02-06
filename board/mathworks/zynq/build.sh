#!/bin/bash

BR_ROOT=$1
DYNCONFIG=$2
INARG=$3

SCRIPT_DIR=$( cd "$( dirname "$0" )" && pwd )
CONFIG_DIR=${SCRIPT_DIR}/defconfig
COMMON_CONFIG=${CONFIG_DIR}/zynq_common.defconfig

TGT_CONFIG=${DYNCONFIG}_defconfig
TGT_CONFIG_FILE=${BR_ROOT}/configs/${TGT_CONFIG}

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

echoerr() { echo "Error: $@" 1>&2; }

tolower() {
	ARG=${1}
	ARG="`echo $ARG | tr A-Z a-z`"
}

usage() {
	cat >&2 <<EOL

Usage: build.sh <BR_ROOT> <DynConfig> <board>_<os>_<toolchain>
Generate the buildroot configuration
commands:
	BR_ROOT		The Buildroot Root directory
	DynConfig	The name of the dynamically generated config file
	board		set the boardname (${BOARD_LIST})
	os			set the operating system (${OS_LIST})
	toolchain	set the toolchain (${TC_LIST})
EOL
	exit 1
}

listCheck () {
	local lst_elem
	local tst=$1
	shift
	local lst="${@}"
	for lst_elem in ${lst}; do 
		if [[ "${lst_elem}" == "${tst}" ]]; then 
			return 1
		fi
	done
	echoerr "${tst} is not in in the list: {${lst}}"
	usage
}

mkconfig() {

	# Test for incompatibilities
	if [ ${OPSYS} == "xenomai"  -a ${TOOLCHAIN} == "xilinx" ]; then
		echoerr "Cannot use Xilinx toolchain with Xenomai OS"
		exit 1
	fi
	
	BOARD_CONFIG=${CONFIG_DIR}/zynq_${BOARD}.defconfig
	OS_CONFIG=${CONFIG_DIR}/zynq_${OPSYS}.defconfig
	TOOLCHAIN_CONFIG=${CONFIG_DIR}/zynq_${TOOLCHAIN}.defconfig		
	# Generate the defconfig
	
	echo "### Generating Board:[${BOARD}] OS:[${OPSYS}] Toolchain: [${TOOLCHAIN}]"
	cat ${COMMON_CONFIG} > ${TGT_CONFIG_FILE}
	cat ${BOARD_CONFIG} >> ${TGT_CONFIG_FILE}
	cat ${TOOLCHAIN_CONFIG} >> ${TGT_CONFIG_FILE}
	cat ${OS_CONFIG} >> ${TGT_CONFIG_FILE}
}

# Split the argument into its components
CMD=${INARG}
tolower ${CMD%%_*} && BOARD=${ARG}
CMD=${CMD#*_}
tolower ${CMD%%_*} && OPSYS=${ARG}
CMD=${CMD#*_}
tolower ${CMD} && TOOLCHAIN=${ARG}

# Check that each component is in the supported lists
listCheck ${BOARD} ${BOARD_LIST}
listCheck ${OPSYS} ${OS_LIST}
listCheck ${TOOLCHAIN} ${TC_LIST}

mkconfig



