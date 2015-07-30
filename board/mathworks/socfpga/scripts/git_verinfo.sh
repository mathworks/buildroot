#!/bin/bash
# syntax: git_verinfo.sh <BUILDROOT CONFIG FILE> <BUILD DIRECTORY> <BUILDROOT ROOT DIRECTORY> <TARGET FILE>
CONFIG_FILE=$1
BUILD_DIR=$2
BR_ROOT=$3
TGT=$4
res=''

verinfo() {
	local pkg=$1
	local ver=$2
	ver=`cat ${CONFIG_FILE} | grep $ver | sed -e 's/BR2_.*=//' | tr -d '"'`
	res=`cat ${BUILD_DIR}/${pkg}-${ver}/.br2_version`
}


LINUX_VER='BR2_LINUX_KERNEL_CUSTOM_REPO_VERSION'
UBOOT_VER='BR2_TARGET_UBOOT_CUSTOM_REPO_VERSION'
pushd ${BR_ROOT}
BR_HASH=`git log -n 1 --pretty="%H"`
popd
verinfo linux $LINUX_VER
LINUX_HASH=$res
verinfo uboot $UBOOT_VER
UBOOT_HASH=$res
echo "Buildroot: ${BR_HASH}" > $TGT
echo "Linux: ${LINUX_HASH}" >> $TGT
echo "U-boot: ${UBOOT_HASH}" >> $TGT
