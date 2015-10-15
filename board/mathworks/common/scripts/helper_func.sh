#!/bin/bash

res=''

function print_msg() {
    CC_STARTMSG=$(tput setab 0)$(tput setaf 7)
    CC_NONE=$(tput sgr0)
    MSG=$1

    echo -e "${CC_STARTMSG}>>>   ${MSG}${CC_NONE}"
}

function print_err() {
    CC_STARTMSG=$(tput setab 1)$(tput setaf 7)$(tput bold)
    CC_NONE=$(tput sgr0)
    MSG=$1

    echo -e "${CC_STARTMSG}ERROR:   ${MSG}${CC_NONE}"
}

function get_cfg_var() {
	local var=$1
	res=$(cat $BR2_CONFIG | grep "^${var} *=" | sed -e 's/BR2_.*=//' | tr -d '"')
}


function verinfo() {
	local pkg=$1
	local ver=$2
    get_cfg_var $ver
	res=$(cat ${BUILD_DIR}/${pkg}-${res}/.br2_version)
}

function gen_verinfo_file() {
    local tgt_file=$1
    local uboot_var=$2
    local uboot_pkg=$3
    local linux_var='BR2_LINUX_KERNEL_CUSTOM_REPO_VERSION'

    pushd ${BR_ROOT}
    local br_hash=`git log -n 1 --pretty="%H"`
    popd
    verinfo linux $linux_var
    local linux_hash=$res
    verinfo $uboot_pkg $uboot_var
    local uboot_hash=$res
    echo "Buildroot: ${br_hash}" > $tgt_file
    echo "Linux: ${linux_hash}" >> $tgt_file
    echo "U-boot: ${uboot_hash}" >> $tgt_file
}

function get_src_dir() {
    local pkg=$1
    local override_info
    local build_dir
    # Check if we're overriding the source
    if [ -f ${BASE_DIR}/local.mk ]; then
        override_file=${BASE_DIR}/local.mk
    fi
    build_dir=$( cd "$( dirname "$BR2_CONFIG" )" && pwd )
    if [ -f ${build_dir}/local.mk ]; then
        override_file=${build_dir}/local.mk
    fi

    if [ ! -z "$override_file" ]; then
        # grab the source file from the override dir
        res=$(cat $override_file | grep -i ${pkg}_override_srcdir | sed -e 's/.*= *//')
    else
        # Otherwise check the version and point to the build dir
        case $pkg in
            linux)
                get_cfg_var 'BR2_LINUX_KERNEL_CUSTOM_REPO_VERSION'
                res=${BUILD_DIR}/linux-${res}
                ;;
            uboot)
                get_cfg_var 'BR2_TARGET_UBOOT_CUSTOM_REPO_VERSION'
                res=${BUILD_DIR}/uboot-${res}
                ;;
            *)
                ;;
        esac
    fi
}

