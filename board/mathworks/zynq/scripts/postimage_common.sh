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
BR_ROOT=$pwd
OUTPUT_DIR=$BASE_DIR
SCRIPT_DIR=$( cd "$( dirname "$0" )" && pwd )
BOARD_DIR=$( cd "$( dirname "${SCRIPT_DIR}" )" && pwd )
SD_DIR=${IMAGE_DIR}/sdcard_${BOARD_NAME}

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

# rename to .elf for bootgen
cd ${IMAGE_DIR}
cp ${UBOOT_BIN} ${UBOOT_ELF}
cp ${FSBL_SRC} ${FSBL_DST}
cp ${BITSTREAM_SRC} ${BITSTREAM_DST}

# create bootimage.bif
cat << EOF > ${BIF_FILE}
	the_ROM_image:
	{
		[bootloader]${FSBL_DST}
		${BITSTREAM_DST}
		u-boot.elf
	}
EOF

# create BOOT.BIN
rm ${BOOT_BIN} &>/dev/null
${BOOTGEN_BIN} -image ${BIF_FILE} -o i ${BOOT_BIN}
mv ${BOOT_BIN} ${SD_DIR}/

# cleanup
rm ${UBOOT_ELF} ${FSBL_DST} ${BITSTREAM_DST} ${BIF_FILE}

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
# Copy Over the init.sh
#####################################
INIT_SCR=${BOARD_DIR}/init.sh
cp ${INIT_SCR} ${SD_DIR}/

#####################################
# Copy Over the docs
#####################################
DOCS_DIR=${BOARD_DIR}/docs/*
cp ${DOCS_DIR} ${SD_DIR}/

