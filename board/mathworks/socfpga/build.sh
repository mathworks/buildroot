#!/bin/bash
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

PLATFORM="socfpga"
OPERATING_SYS="linux"
TOOLCHAIN="linaro"
BOARD="alteracycv"
BUILDALL="false"

# List of valid boards
BOARD_LIST=$(cd ${SCRIPT_DIR}/boards; ls -d * | tr -d /)

# List of valid os
OS_LIST="linux"
# List of valid toolchains
TC_LIST="linaro"

checkconfig() {
	# Test for incompatibilities
    return 0
}




