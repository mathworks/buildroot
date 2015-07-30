#!/bin/bash
# format: <post-build-script.sh> <target dir> -b <board name> [-c <chip name>] [-a <application> [-a <application>]]
# executed out of main buildroot source directory
# available environment variables
#	BR2_CONFIG: path to .config file
#	HOST_DIR, STAGING_DIR, TARGET_DIR
#	BINARIES_DIR: images dir
#	BASE_DIR: base output directory

SCRIPT_DIR=$( cd "$( dirname "$0" )" && pwd )
source ${SCRIPT_DIR}/parse_opts.sh

TARGET_DIR=${INDIR}

###################################
# Update inittab to not use getty
###################################
tempfile=$(mktemp)
sed 's|\(.*\)::respawn.*# GENERIC_SERIAL$|\1::respawn:-/bin/sh # GENERIC_SERIAL|' ${TARGET_DIR}/etc/inittab > $tempfile && \
	cat $tempfile > ${TARGET_DIR}/etc/inittab
	
#####################################
# Add the version info to the rootfs	
#####################################
${SCRIPT_DIR}/git_verinfo.sh $BR2_CONFIG ${OUTPUT_DIR}/build $BR_ROOT ${TARGET_DIR}/etc/buildinfo
