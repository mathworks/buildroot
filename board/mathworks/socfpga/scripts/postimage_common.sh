#!/bin/bash
# format: <post-image-script.sh> <image dir> -b <board name> [-c <chip name>] [-a <application> [-a <application>]]
# executed out of main buildroot source directory
# available environment variables
#	BR2_CONFIG: path to .config file
#	HOST_DIR, STAGING_DIR, TARGET_DIR
#	BINARIES_DIR: images dir
#	BASE_DIR: base output directory

SCRIPT_DIR=$( cd "$( dirname "$0" )" && pwd )
source ${SCRIPT_DIR}/parse_opts.sh

IMAGE_DIR=${INDIR}
TGTNAME=${BOARD_NAME}${CHIP_NAME}
SD_DIR=${IMAGE_DIR}/sdcard
BOOT_DIR=${BOARD_DIR}/boot

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
SD_SRC=${BOARD_DIR}/sdcard/*
cp ${SD_SRC} ${SD_DIR}/

#####################################
# Add the version info to the sdcard	
#####################################
${SCRIPT_DIR}/git_verinfo.sh $BR2_CONFIG ${OUTPUT_DIR}/build $BR_ROOT ${SD_DIR}/BUILDINFO

####################################
# Add the application specific DTBs
####################################
# always generate the base app
APP_LIST="base ${APP_LIST}"
${COMMON_SCRIPTS}/gen_dtb.sh $OUTPUT_DIR socfpga ${BOARD_NAME} ${APP_LIST}

# Boot from the base DTB by default
mv ${SD_DIR}/devicetree_base.dtb ${SD_DIR}/socfpga.dtb

####################################
# Copy over the rbf and the u-boot script
####################################
cp ${BOOT_DIR}/u-boot-scr.txt ${IMAGE_DIR}/u-boot-scr.txt 
pushd ${IMAGE_DIR}
${HOST_DIR}/usr/bin/mkimage -A arm -O linux -T script -C none -a 0 -e 0 -n "U-Boot Script" -d u-boot-scr.txt u-boot.scr
popd
mv ${IMAGE_DIR}/u-boot.scr ${SD_DIR}/u-boot.scr 
cp ${BOOT_DIR}/${TGTNAME}.rbf ${SD_DIR}/socfpga.rbf

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
IMAGE_SIZE=1000M
IMGFILE=${IMAGE_DIR}/${TGTNAME}_${BUILDDATE}_sdcard.img
ROOTFS=${IMAGE_DIR}/rootfs.tar.gz

rm -f ${IMGFILE} ${IMGFILE}.gz
#make_sdimage.sh [-h] [-k f1,f2] [[-p preloader |-rp preloader] -b bootloader] [-r rfs_dir [-m merge_dir]] [-o image] [-g size] [-t tool]
pushd ${IMAGE_DIR}

sudo ${SUDOENV} ${SCRIPT_DIR}/make_sdimage.sh -p ${SPL} -b ${BOOTLOADER} -r ${ROOTFS} -k ${FATFILES} -o ${IMGFILE} -g ${IMAGE_SIZE}
THISUSER=`id -un`
THISGROUP=`id -gn`
sudo chown ${THISUSER}:${THISGROUP} ${IMGFILE}

#compress the SD card image
gzip ${IMGFILE}

popd


