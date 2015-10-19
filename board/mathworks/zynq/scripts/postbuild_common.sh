#!/bin/bash
# format: <post-build-script.sh> <target dir> -b <board name> [-c <chip name>] [-a <application> [-a <application>]]
# executed out of main buildroot source directory
# available environment variables
#	BR2_CONFIG: path to .config file
#	HOST_DIR, STAGING_DIR, TARGET_DIR
#	BINARIES_DIR: images dir
#	BASE_DIR: base output directory

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
PLATFORM_DIR=$( cd "$( dirname "${SCRIPT_DIR}" )" && pwd )
COMMON_DIR=$( cd "$( dirname "${PLATFORM_DIR}" )" && pwd )/common
source ${COMMON_DIR}/scripts/parse_opts.sh


###################################
# Setup some params
###################################
UBOOT_VAR='BR2_TARGET_UBOOT_CUSTOM_REPO_VERSION'
UBOOT_PKG="uboot"

###################################
# Source the common script
###################################
source ${COMMON_SCRIPTS}/postbuild_common.sh
