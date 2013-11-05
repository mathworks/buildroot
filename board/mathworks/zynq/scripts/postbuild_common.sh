#!/bin/bash
IMAGE_DIR=$1
SCRIPT_DIR=$( cd "$( dirname "$0" )" && pwd )
BOARD_DIR=$( cd "$( dirname "${SCRIPT_DIR}" )" && pwd )
# Create the u-boot ramdisk image
CPIO_IMG=${IMAGE_DIR}/rootfs.cpio.gz
UIMAGE=${IMAGE_DIR}/uramdisk.image.gz
mkimage -A arm -T ramdisk -C gzip -d $CPIO_IMG $UIMAGE

#####################################
# Create the xilinx boot.bin
#####################################
UBOOT_BIN=u-boot
UBOOT_ELF=u-boot.elf
BIF_FILE=bootimage.bif
FSBL_SRC=${BOARD_DIR}/${BOARD_NAME}_fsbl.elf
FSBL_DST=zynq_fsbl.elf
BOOTGEN_BIN=/opt/Xilinx/14.4/SDK/SDK/bin/lin/bootgen
BOOT_BIN=BOOT.BIN

# rename to .elf for bootgen
cd ${IMAGE_DIR}
mv ${UBOOT_BIN} ${UBOOT_ELF}
cp ${FSBL_SRC} ${FSBL_DST}

# create bootimage.bif
cat << EOF > ${BIF_FILE}
	the_ROM_image:
	{
		[bootloader]${FSBL_DST}
		u-boot.elf
	}
EOF

# create BOOT.BIN
rm ${BOOT_BIN} &>/dev/null
${BOOTGEN_BIN} -image ${BIF_FILE} -o i ${BOOT_BIN}

#####################################
# Move the devicetree
#####################################
DEVTREE=zynq-mw-${BOARD_NAME}.dtb
cd ${IMAGE_DIR}
cp ${DEVTREE} devicetree.dtb

#####################################
# Copy Over the init.sh
#####################################
INIT_SCR=${BOARD_DIR}/init.sh
cp ${INIT_SCR} ${IMAGE_DIR}/
