#!/bin/bash
# format: <post-image-script.sh> <image dir> <board name>
# executed out of main buildroot source directory
# available environment variables
#	BUILDROOT_CONFIG: path to .config file
#	HOST_DIR, STAGING_DIR, TARGET_DIR
#	BINARIES_DIR: images dir
#	BASE_DIR: base output directory
IMAGE_DIR=$1
BOARD_NAME=$2
BR_ROOT=$PWD
OUTPUT_DIR=$BASE_DIR
SCRIPT_DIR=$( cd "$( dirname "$0" )" && pwd )
BOARD_DIR=$( cd "$( dirname "${SCRIPT_DIR}" )" && pwd )
SD_DIR=${IMAGE_DIR}/sdcard_${BOARD_NAME}


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
${MKIMAGE_BIN} -A arm -T ramdisk -C gzip -d $CPIO_IMG $UIMAGE

#####################################
# Create the xilinx boot.bin
#####################################
BOOT_DIR=${BOARD_DIR}/boot
UBOOT_BIN=u-boot
UBOOT_ELF=u-boot.elf
BIF_FILE=bootimage.bif
FSBL_SRC=${BOOT_DIR}/${BOARD_NAME}_fsbl.elf
FSBL_DST=zynq_fsbl.elf
BITSTREAM_SRC=${BOOT_DIR}/${BOARD_NAME}.bit
BITSTREAM_DST=zynq.bit
BOOTGEN_BIN=/opt/Xilinx/14.4/SDK/SDK/bin/lin/bootgen
BOOT_BIN=BOOT.BIN
FPGA_BIN=system.bit.bin

# rename to .elf for bootgen
cd ${IMAGE_DIR}
cp ${UBOOT_BIN} ${UBOOT_ELF}
cp ${FSBL_SRC} ${FSBL_DST}
cp ${BITSTREAM_SRC} ${BITSTREAM_DST}

# create BOOT.BIN
rm ${BOOT_BIN} &>/dev/null
create_bif ${BIF_FILE} ${FSBL_DST} ${BITSTREAM_DST}
${BOOTGEN_BIN} -image ${BIF_FILE} -o i ${BOOT_BIN}
mv ${BOOT_BIN} ${SD_DIR}/

# cleanup
rm ${UBOOT_ELF} ${UBOOT_ELF}.bin ${FSBL_DST} ${FSBL_DST}.bin ${BITSTREAM_DST} ${BIF_FILE} ${BOOT_BIN} &>/dev/null

#####################################
# Move the devicetree
#####################################
DEVTREE=zynq-mw-${BOARD_NAME}.dtb
cd ${IMAGE_DIR}
cp ${DEVTREE} ${SD_DIR}/devicetree.dtb

#####################################
# Move the kernel
#####################################
KERNEL=uImage
cd ${IMAGE_DIR}
cp ${KERNEL} ${SD_DIR}/

#####################################
# Copy Over the sdcard files
#####################################
SD_SRC=${BOARD_DIR}/sdcard/*
cp ${SD_SRC} ${SD_DIR}/

#####################################
# Add the version info to the sdcard	
#####################################
${SCRIPT_DIR}/git_verinfo.sh $BUILDROOT_CONFIG ${OUTPUT_DIR}/build $BR_ROOT ${SD_DIR}/BUILDINFO

####################################
# Add the application specific DTBs
####################################
# Video DTB
${SCRIPT_DIR}/gen_dtb.sh $OUTPUT_DIR ${BOARD_NAME} vid
