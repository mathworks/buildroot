#!/bin/bash

BR_ROOT=$1
DYNCONFIG=$2
BOARD_DIR=$3
INARG=$4

SCRIPT_DIR=$( cd "$( dirname "$0" )" && pwd )
COMPANY_CONFIG=${SCRIPT_DIR}/defconfig/common.defconfig
CONFIG_DIR=${BOARD_DIR}/defconfig
TGT_CONFIG=${DYNCONFIG}_defconfig
TGT_CONFIG_FILE=${BR_ROOT}/configs/${TGT_CONFIG}





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
	BOARD_CONFIG=${CONFIG_DIR}/${PLATFORM}_${BOARD}.defconfig
	OS_CONFIG=${CONFIG_DIR}/${PLATFORM}_${OPSYS}.defconfig
	TOOLCHAIN_CONFIG=${CONFIG_DIR}/${PLATFORM}_${TOOLCHAIN}.defconfig		
	# Generate the defconfig
	
	echo "### Generating Board:[${BOARD}] OS:[${OPSYS}] Toolchain: [${TOOLCHAIN}]"
	cat ${COMPANY_CONFIG} > ${TGT_CONFIG_FILE}
	cat ${COMMON_CONFIG} >> ${TGT_CONFIG_FILE}
	cat ${BOARD_CONFIG} >> ${TGT_CONFIG_FILE}
	cat ${TOOLCHAIN_CONFIG} >> ${TGT_CONFIG_FILE}
	cat ${OS_CONFIG} >> ${TGT_CONFIG_FILE}
}

echo "Sourcing ${BOARD_DIR}/build.sh"
source ${BOARD_DIR}/build.sh

COMMON_CONFIG=${CONFIG_DIR}/${PLATFORM}_common.defconfig

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

checkconfig
mkconfig



