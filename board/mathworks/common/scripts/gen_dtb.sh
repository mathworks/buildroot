#!/bin/bash
# syntax: <gen_dtb.sh> <output_dir> <platform> <board_name> <app_name> 
OUTPUT_DIR=$( cd $1 && pwd )
PLATFORM=$2
BOARD_NAME=$3
shift 3
APP_LIST="$@"
SCRIPT_DIR=$( cd "$( dirname "$0" )" && pwd )
COMMON_DIR=$( cd "$( dirname "${SCRIPT_DIR}" )" && pwd )
BOARD_DIR=$( cd "$( dirname "${COMMON_DIR}" )" && pwd )/${PLATFORM}
IMAGE_DIR=${OUTPUT_DIR}/images
HOST_DIR=${OUTPUT_DIR}/host
BUILD_DIR=${OUTPUT_DIR}/build
TARGET_DIR=${OUTPUT_DIR}/target
SD_DIR=${IMAGE_DIR}/sdcard
res=''

source ${SCRIPT_DIR}/helper_func.sh


# Get the linux version from the BR .config file
LINUX_INFO='BR2_LINUX_KERNEL_CUSTOM_REPO_VERSION'
get_src_dir linux
LINUX_DIR=$res

# Get the base devicetree name
DTS_INFO='BR2_LINUX_KERNEL_INTREE_DTS_NAME'
get_cfg_var $DTS_INFO
DTS_BASE=${res}.dts

# Tool paths
DTC=${HOST_DIR}/usr/bin/dtc
LINUX_DTS=${LINUX_DIR}/arch/arm/boot/dts
DTS_FILE=devicetree.dts


# Determine the toolchain name for CPP
get_cfg_var 'BR2_ARM_EABI'
EABI=$res
get_cfg_var 'BR2_ARM_EABIHF'
EABIHF=$res
if [ "$EABI" == "y" ]; then
    TC_PREFIX="arm-linux-gnueabi"
elif [ "$EABIHF" == "y" ]; then
    TC_PREFIX="arm-linux-gnueabihf"
else
    print_err "Could not find arachitecture"
fi

DTC_CPP_FLAGS="-Wp,-MD,dependency.pre.tmp -nostdinc \
            -I${COMMON_DIR}/dts \
            -I${BOARD_DIR}/dts \
            -I${LINUX_DTS} \
            -I${LINUX_DTS}/include \
            -undef -D__DTS__  -x assembler-with-cpp"

pushd ${SD_DIR}
for APP_NAME in ${APP_LIST}; do
    APP_DTSI=${PLATFORM}-mw-${BOARD_NAME}-${APP_NAME}.dtsi
    if [ -f ${BOARD_DIR}/dts/${APP_DTSI} ]; then
        print_msg "Generating ${APP_NAME} dtb"
        DTB_FILE=devicetree_${APP_NAME}.dtb
        # Generate a temporary DTS file
cat << EOF > ${DTS_FILE}
    #include "${DTS_BASE}"
    #include "${APP_DTSI}"
EOF
        # Run the DTS through CPP     
        ${HOST_DIR}/usr/bin/${TC_PREFIX}-cpp $DTC_CPP_FLAGS -o ${APP_NAME}.tmp.dts ${DTS_FILE}
        # Call DTC
        ${DTC} -i ${LINUX_DTS} -i ${BOARD_DIR}/dts -i ${COMMON_DIR}/dts -I dts -O dtb -o ${DTB_FILE} ${APP_NAME}.tmp.dts
        # Cleanup
        rm -f dependency.pre.tmp
        rm -f ${APP_NAME}.tmp.dts
        rm -f ${DTS_FILE}
    fi
done
popd
