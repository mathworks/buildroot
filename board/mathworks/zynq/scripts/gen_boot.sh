#!/bin/bash
# syntax: <gen_boot.sh> <output_dir> <tgt_name> <app_name> 
OUTPUT_DIR=$( cd $1 && pwd )
TGTNAME=$2
shift 2
APP_LIST="$@"
SCRIPT_DIR=$( cd "$( dirname "$0" )" && pwd )
BOARD_DIR=$( cd "$( dirname "${SCRIPT_DIR}" )" && pwd )
BOOT_DIR=${BOARD_DIR}/boot
IMAGE_DIR=${OUTPUT_DIR}/images
HOST_DIR=${OUTPUT_DIR}/host
BUILD_DIR=${OUTPUT_DIR}/build
TARGET_DIR=${OUTPUT_DIR}/target
SD_DIR=${IMAGE_DIR}/sdcard
res=''

source ${SCRIPT_DIR}/helper_func.sh

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



function create_boot() {
    local TGTBOOT=$1
    local TGTFSBL=$2
    local TGTBIT=$3
    
    local FSBL_SRC=${BOOT_DIR}/${TGTFSBL}_fsbl.elf
    local BOOT_BIN=${TGTBOOT}.BIN
    local BITSTREAM_SRC=${BOOT_DIR}/${TGTBIT}.bit
    
    local UBOOT_BIN=u-boot
    local UBOOT_ELF=u-boot.elf
    local BIF_FILE=bootimage.bif
    local FSBL_DST=zynq_fsbl.elf
    local BITSTREAM_DST=zynq.bit

    get_cfg_var "BR2_TOOLCHAIN_EXTERNAL_PATH"
    local SDK_ROOT=$(dirname $(dirname $(dirname ${res})))
    local BOOTGEN_BIN=/opt/Xilinx/SDK/2014.4/bin/bootgen

    print_msg "Generating ${TGTBOOT}.BIN"

    # rename to .elf for bootgen
    pushd ${IMAGE_DIR}
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
    popd

    
}

#Create the default BOOT.BIN
create_boot BOOT ${TGTNAME} ${TGTNAME}

for APP_NAME in ${APP_LIST}; do
    APP_FSBL=${TGTNAME}-${APP_NAME}
    APP_BIT=${TGTNAME}-${APP_NAME}
    APP_BOOT=BOOT_${APP_NAME}
    USE=false

    if [ -f ${BOOT_DIR}/${APP_FSBL}_fsbl.elf ]; then
        USE=true
    else
        APP_FSBL=${TGTNAME}
    fi

    if [ -f ${BOOT_DIR}/${APP_BIT}.bit ]; then
        USE=true
    else
        APP_BIT=${TGTNAME}
    fi

    if [ $USE == true ]; then
        create_boot ${APP_BOOT} ${APP_FSBL} ${APP_BIT}
    fi
    
done

