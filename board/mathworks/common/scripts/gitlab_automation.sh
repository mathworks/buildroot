#!/bin/bash

##############################
# Resolve a given platform variable
#############################
platform_variable() {
	local suffix=$1
	local platName=${CI_JOB_PLATFORM^^}
	local varName=${platName}_${suffix}
	local varValue=${!varName}
	echo $varValue
}

##############################
# Resolve all buildroot source variables
#############################
resolve_br_vars() {

	local linuxURL=$(platform_variable LINUX_URL)
	local linuxVer=$(platform_variable LINUX_VER)

	local ubootURL=$(platform_variable UBOOT_URL)
	local ubootVer=$(platform_variable UBOOT_VER)	

	local brvars=""
	if [ "$linuxURL" != "" ]; then
		brvars="$brvars --brconfig BR2_LINUX_KERNEL_CUSTOM_REPO_URL=${linuxURL}"
	fi
	if [ "$linuxVer" != "" ]; then
		brvars="$brvars --brconfig BR2_LINUX_KERNEL_CUSTOM_REPO_VERSION=${linuxVer}"
	fi
	if [ "$ubootURL" != "" ]; then
		brvars="$brvars --brconfig BR2_TARGET_UBOOT_CUSTOM_REPO_URL=${ubootURL}"
	fi
	if [ "$ubootVer" != "" ]; then
		brvars="$brvars --brconfig BR2_TARGET_UBOOT_CUSTOM_REPO_VERSION=${ubootVer}"
	fi

	echo $brvars
}

##############################
# Execute the build command
#############################
run_build_command() {
	local target=$1
	shift
	local extraargs=$@
	local brvars=$(resolve_br_vars)
	set -x
	${CI_PROJECT_DIR}/build.py --target $target --dl $CI_BR2_DL_DIR/$CI_JOB_PLATFORM -b $CI_JOB_BOARD -p $CI_JOB_PLATFORM --ccache -l logs/${CI_BUILD_NAME}.log $brvars $extraargs
	set +x
}

##############################
# Main Script
#############################
case "${CI_BUILD_STAGE}" in
	sources_common)
	  	echo "Preparing Common Sources"
		CI_JOB_PLATFORM=zynq
		CI_JOB_BOARD=zed
		run_build_command source --ccache-clean
		rm -rf $CI_BR2_DL_DIR/zynq/linux-*
		rm -rf $CI_BR2_DL_DIR/zynq/uboot-*
		cp -R $CI_BR2_DL_DIR/zynq $CI_BR2_DL_DIR/socfpga
		cp -R $CI_BR2_DL_DIR/zynq $CI_BR2_DL_DIR/zynqmp
		rm -rf ${CI_PROJECT_DIR}/output/*/build
		;;
	sources_custom)
		echo "Preparing $CI_JOB_PLATFORM Sources"	
		run_build_command source -u
		;;
	build)
		echo "Building $CI_JOB_BOARD/$CI_JOB_PLATFORM"
		run_build_command all -u -q
		;;
	*)
		echo "No automation defined for ${CI_BUILD_STAGE}"
		exit 1
		;;
esac

exit 0
