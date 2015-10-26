#!/bin/bash
# format: <post-image-script.sh> <image dir> -b <board name> [-c <chip name>] [-a <application> [-a <application>]]
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

IMAGE_DIR=${INDIR}
TGTNAME=${BOARD_NAME}${CHIP_NAME}
SD_DIR=${IMAGE_DIR}/sdcard

function create_bif() {
local BIF=$1
local FSBL=$2
local BITSTREAM=$3
# create bootimage.bif
cat << EOF > ${BIF}
	the_ROM_image:
	{
		[bootloader]${FSBL}
		${BITSTREAM}
		u-boot.elf
	}
EOF
}

rm -rf ${SD_DIR}
mkdir -p ${SD_DIR}

#####################################
# Create the u-boot ramdisk image
#####################################
MKIMAGE_BIN=${OUTPUT_DIR}/host/usr/bin/mkimage
CPIO_IMG=${IMAGE_DIR}/rootfs.cpio.gz
UIMAGE=${SD_DIR}/uramdisk.image.gz
print_msg "Creating uramdisk $UIMAGE"
${MKIMAGE_BIN} -A arm -T ramdisk -C gzip -d $CPIO_IMG $UIMAGE

#####################################
# Create the boot.bin files
#####################################
${SCRIPT_DIR}/gen_boot.sh $OUTPUT_DIR $TGTNAME ${DEFAULT_APP} ${APP_LIST}
print_msg "Setting ${DEFAULT_APP} as default BOOT.BIN"
cp ${SD_DIR}/BOOT_${DEFAULT_APP}.BIN ${SD_DIR}/BOOT.BIN

#####################################
# Move the kernel
#####################################
KERNEL=uImage
cd ${IMAGE_DIR}
cp ${KERNEL} ${SD_DIR}/

#####################################
# Copy Over the sdcard files
#####################################
SD_SRC=${PLATFORM_DIR}/sdcard/*
cp ${SD_SRC} ${SD_DIR}/

#####################################
# Add the version info to the sdcard	
#####################################
gen_verinfo_file ${SD_DIR}/BUILDINFO BR2_TARGET_UBOOT_CUSTOM_REPO_VERSION uboot

####################################
# Add the application specific DTBs
####################################
${COMMON_SCRIPTS}/gen_dtb.sh $OUTPUT_DIR zynq ${BOARD_NAME} ${APP_LIST}

# Boot from the default DTB
print_msg "Setting ${DEFAULT_APP} as default dtb"
cp ${SD_DIR}/devicetree_${DEFAULT_APP}.dtb ${SD_DIR}/devicetree.dtb

####################################
# Zip the SD Card Directory
####################################
IMAGE_DIR=${INDIR}
BUILDDATE=`date +%F`
ZIPNAME=${TGTNAME}_sdcard_${BUILDDATE}.zip
SD_DIR=${IMAGE_DIR}/sdcard
print_msg "Generating ${ZIPNAME}"
pushd ${IMAGE_DIR}
    rm -f ${ZIPNAME}
    pushd sdcard
    zip -r ../${ZIPNAME} *
    popd
popd

