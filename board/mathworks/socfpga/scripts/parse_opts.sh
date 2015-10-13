#!/bin/bash
# format: <post-*-script.sh> <image dir/target dir> -b <board name> [-c <chip name>] [-a <application> [-a <application>]]
# executed out of main buildroot source directory
# available environment variables
#	BR2_CONFIG: path to .config file
#	HOST_DIR, STAGING_DIR, TARGET_DIR
#	BINARIES_DIR: images dir
#	BASE_DIR: base output directory

INDIR=$1
shift
OPTIND=1 # Reset in case getopts has been used previously in the shell.
APP_LIST=""
BOARD_NAME=""
CHIP_NAME=""

while getopts "a:b:c:" opt; do
    case ${opt} in
        a) 
            APP_LIST="${APP_LIST}${OPTARG} "
            ;;
        b)
            BOARD_NAME=${OPTARG}
            ;;
        c)
            CHIP_NAME="-${OPTARG}"
            ;;
    esac
done
shift $((OPTIND-1))

BR_ROOT=$PWD
OUTPUT_DIR=$BASE_DIR
PLATFORM_DIR=$( cd "$( dirname "${SCRIPT_DIR}" )" && pwd )
COMMON_DIR=$( cd "$( dirname "${PLATFORM_DIR}" )" && pwd )/common
COMMON_SCRIPTS=${COMMON_DIR}/scripts
BOARD_DIR=${PLATFORM_DIR}/boards/${BOARD_NAME}
