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
BOOT_DIR=${PLATFORM_DIR}/boot

res=''

build_file_list() {
	local dir=$1
    filelist=`ls ${dir}`
    res=''    
    for filename in $filelist; do
        res="${dir}/${filename},${res}"
    done
    # strip the trailing comma
    res=`echo $res | sed -e 's/,$//'`
}

rm -rf ${SD_DIR}
mkdir -p ${SD_DIR}

#####################################
# Copy the kernel
#####################################
KERNEL=zImage
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
gen_verinfo_file ${SD_DIR}/BUILDINFO BR2_PACKAGE_UBOOT_ALTERA_CUSTOM_REPO_VERSION uboot-altera

####################################
# Add the application specific DTBs
####################################
${COMMON_SCRIPTS}/gen_dtb.sh $OUTPUT_DIR socfpga ${BOARD_NAME} ${APP_LIST}

# Boot from the base DTB by default
print_msg "Setting ${DEFAULT_APP} as default dtb"
mv ${SD_DIR}/devicetree_${DEFAULT_APP}.dtb ${SD_DIR}/socfpga.dtb

####################################
# Add the application specific rbfs
####################################
#Todo: do this

# boot from the base rbf by default
cp ${BOARD_DIR}/boot/base.rbf ${SD_DIR}/socfpga.rbf

####################################
# Copy over the u-boot script
####################################
cp ${BOOT_DIR}/u-boot-scr.txt ${IMAGE_DIR}/u-boot-scr.txt 
pushd ${IMAGE_DIR}
${HOST_DIR}/usr/bin/mkimage -A arm -O linux -T script -C none -a 0 -e 0 -n "U-Boot Script" -d u-boot-scr.txt u-boot.scr
popd
mv ${IMAGE_DIR}/u-boot.scr ${SD_DIR}/u-boot.scr 


####################################
# Copy over u-boot (SPL will load u-boot.img)
####################################
cp ${IMAGE_DIR}/u-boot.img ${SD_DIR}/

####################################
# Call the Altera script
####################################

SUDOENV="env PATH=$PATH:${HOST_DIR}/usr/bin:${HOST_DIR}/usr/sbin"
BUILDDATE=`date +%F`
SPL=${IMAGE_DIR}/preloader-mkpimage.bin
#SPL=${BOOT_DIR}/${TGTNAME}-preloader-mkpimage.bin
BOOTLOADER=${IMAGE_DIR}/u-boot.img
build_file_list ${SD_DIR}
FATFILES=$res
IMAGE_SIZE=1000
FAT_SIZE=250
A2_SIZE=10
EXT2_SIZE=$((${IMAGE_SIZE} - ${FAT_SIZE} - ${A2_SIZE} - 10))
IMGFILE=${IMAGE_DIR}/${TGTNAME}_${BUILDDATE}_sdcard.img
ROOTFS=${IMAGE_DIR}/rootfs.tar.gz
TEMPROOTFS=${IMAGE_DIR}/trootfs

rm -f ${IMGFILE} ${IMGFILE}.gz

# untar the rootfs into a temp dir
rm -rf ${TEMPROOTFS}
mkdir ${TEMPROOTFS}
pushd ${TEMPROOTFS}
    sudo tar xzf ${ROOTFS}
popd

pushd ${IMAGE_DIR}
sudo ${SUDOENV} ${SCRIPT_DIR}/make_sdimage.py \
        -f \
        -P ${SPL},${BOOTLOADER},num=3,format=raw,size=${A2_SIZE}M,type=A2  \
        -P ${TEMPROOTFS},num=2,format=ext3,size=${EXT2_SIZE}M  \
        -P ${FATFILES},num=1,format=vfat,size=${FAT_SIZE}M  \
        -s ${IMAGE_SIZE}M  \
        -n ${IMGFILE}

sudo rm -rf ${TEMPROOTFS}

THISUSER=`id -un`
THISGROUP=`id -gn`
sudo chown ${THISUSER}:${THISGROUP} ${IMGFILE}

#compress the SD card image
gzip ${IMGFILE}

popd


