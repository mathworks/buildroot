#!/bin/bash
IMAGE_DIR=$1
SCRIPT_DIR=$( cd "$( dirname "$0" )" && pwd )
BOARD_NAME=zed

source ${SCRIPT_DIR}/postbuild_common.sh
