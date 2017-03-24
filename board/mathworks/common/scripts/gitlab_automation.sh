#!/bin/bash

##############################
# Resolve a given platform variable
#############################
platform_variable() {
	local suffix=$1
	local platName=${CONFIG_JOB_PLATFORM^^}
	local varName=CONFIG_${platName}_${suffix}
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
	${CI_PROJECT_DIR}/build.py --target "$target" --dl $CONFIG_BR2_DL_DIR/$CONFIG_JOB_PLATFORM -b $CONFIG_JOB_BOARD -p $CONFIG_JOB_PLATFORM --ccache -l logs/${CI_BUILD_NAME}.log $brvars $extraargs
	set +x
}

prep_git_credentials() {
	local cred_file=""
	local gitConfig=""

	if [ "$XDG_CONFIG_HOME" != "" ]; then
		gitConfig=$XDG_CONFIG_HOME/git/config
		cred_file=$XDG_CONFIG_HOME/git/cred_store
	else
		gitConfig=$HOME/.gitconfig
		cred_file=$HOME/.git_cred_store
	fi

	echo -n "" > $cred_file

	if [ "$CONFIG_CREDENTIAL_USER_GITHUB" != "" ]; then
		echo "https://${CONFIG_CREDENTIAL_USER_GITHUB}:${CONFIG_CREDENTIAL_PASS_GITHUB}@github.com" >> $cred_file
		echo "http://${CONFIG_CREDENTIAL_USER_GITHUB}:${CONFIG_CREDENTIAL_PASS_GITHUB}@github.com" >> $cred_file
	fi

	git config --file $gitConfig credential.helper "store --file $cred_file"
}

##############################
# Main Script
#############################
prep_git_credentials

case "${CI_BUILD_STAGE}" in
	setup)
		if [ "$CONFIG_SETUP_SCRIPT" != ""]; then
			$CONFIG_SETUP_SCRIPT
		fi
		;;
	teardown)
		if [ "$CONFIG_SETUP_SCRIPT" != ""]; then
			$CONFIG_SETUP_SCRIPT
		fi
		;;
	sources_common)
	  	echo "Preparing Common Sources"
		CONFIG_JOB_PLATFORM=zynq
		CONFIG_JOB_BOARD=zed
		run_build_command source --ccache-clean
		rm -rf $CONFIG_BR2_DL_DIR/zynq/linux-*
		rm -rf $CONFIG_BR2_DL_DIR/zynq/uboot-*
		cp -R $CONFIG_BR2_DL_DIR/zynq $CONFIG_BR2_DL_DIR/socfpga
		cp -R $CONFIG_BR2_DL_DIR/zynq $CONFIG_BR2_DL_DIR/zynqmp
		rm -rf ${CI_PROJECT_DIR}/output/*/build
		;;
	sources_custom)
		echo "Preparing $CONFIG_JOB_PLATFORM Sources"	
		run_build_command source -u
		;;
	build)
		echo "Building $CONFIG_JOB_BOARD/$CONFIG_JOB_PLATFORM"
		run_build_command "all legal-info" -u -q
		;;
	*)
		echo "No automation defined for ${CI_BUILD_STAGE}"
		exit 1
		;;
esac

exit 0
