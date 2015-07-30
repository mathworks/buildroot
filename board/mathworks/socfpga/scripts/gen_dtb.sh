#!/bin/bash
# syntax: <gen_dtb.sh> <output_dir> <board_name> <app_name> 
OUTPUT_DIR=$( cd $1 && pwd )
BOARD_NAME=$2
shift 2
APP_LIST="$@"
SCRIPT_DIR=$( cd "$( dirname "$0" )" && pwd )
BOARD_DIR=$( cd "$( dirname "${SCRIPT_DIR}" )" && pwd )
IMAGE_DIR=${OUTPUT_DIR}/images
HOST_DIR=${OUTPUT_DIR}/host
BUILD_DIR=${OUTPUT_DIR}/build
TARGET_DIR=${OUTPUT_DIR}/target
SD_DIR=${IMAGE_DIR}/sdcard
res=''

verinfo() {
	local ver=$1
	res=`cat ${OUTPUT_DIR}/.config | grep $ver | sed -e 's/BR2_.*=//' | tr -d '"'`
}

# Get the linux version from the BR .config file
LINUX_INFO='BR2_LINUX_KERNEL_CUSTOM_REPO_VERSION'
verinfo $LINUX_INFO
LINUX_VER=$res

# Get the base devicetree name
DTS_INFO='BR2_LINUX_KERNEL_INTREE_DTS_NAME'
verinfo $DTS_INFO
DTS_BASE=$res


# Tool paths
DTC=${HOST_DIR}/usr/bin/dtc
LINUX_DTS=${BUILD_DIR}/linux-${LINUX_VER}/arch/arm/boot/dts
DTS_NAME=socfpga
DTS_FILE=${DTS_NAME}.dts

pushd ${SD_DIR}
for APP_NAME in ${APP_LIST}; do
    echo "Generating ${APP_NAME} dtb"
    DTB_FILE=${DTS_NAME}_${APP_NAME}.dtb
# Generate a temporary DTS file
cat << EOF > ${DTS_FILE}
    /include/ "${DTS_BASE}.dts"
    /include/ "socfpga-mw-${BOARD_NAME}-${APP_NAME}.dtsi"
EOF
    ${DTC} -i ${LINUX_DTS} -i ${BOARD_DIR}/dts -I dts -O dtb -o ${DTB_FILE} ${DTS_FILE}
    rm -f ${DTS_FILE}
done
popd
