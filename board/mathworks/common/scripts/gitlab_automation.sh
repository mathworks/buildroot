#!/bin/bash

##############################
# Resolve a given platform variable
#############################
platform_variable() {
	local suffix=$1
	local platName=${CONFIG_JOB_PLATFORM^^}
	local varName=CONFIG_${platName}_${suffix}
	varName=$(echo $varName | tr -cd [:alnum:]_)
	local varValue=${!varName}
	echo $varValue
}

project_variable() {
	local suffix=$1
	local projName=${CONFIG_JOB_PROJECT^^}
	local varName=CONFIG_${projName}_${suffix}
	varName=$(echo $varName | tr -cd [:alnum:]_)
	local varValue=${!varName}
	echo $varValue
}

board_variable() {
	local board=$1
	local suffix=$2
	local boardName=${board^^}
	local varName=CONFIG_${boardName}_${suffix}
	varName=$(echo $varName | tr -cd [:alnum:]_)
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
	local build_spec=$1
	local target=$2
	shift 2
	local extraargs=$@
	local brvars=$(resolve_br_vars)
	local skipPlatform=$(platform_variable SKIP)

	if [ "$skipPlatform" != "" ] && [ "$skipPlatform" != "0" ] && [ "$CONFIG_PLATFORM_NOSKIP" == "" ]; then
		echo "Skipping platform $CONFIG_JOB_PLATFORM"
		return 0
	fi

	${CI_PROJECT_DIR}/build.py --target "$target" --dl $CONFIG_BR2_DL_DIR/$CONFIG_JOB_PLATFORM \
			$build_spec --ccache -l logs/${CI_BUILD_NAME}.log \
			-d images/${CONFIG_JOB_PROJECT}/ $brvars $extraargs
	rc=$?
	set +x
	if [ "$rc" != "0" ]; then
		echo "build error: $rc"
		exit $rc
	fi
}

##############################
# Build the catalog files
#############################
build_catalogs() {
	local target=$1
	shift
	local extraargs=$@
	local skipProject=$(project_variable SKIP_PROJECT)
	local skipBoard=""
	local board=""
	local catalog=""
	
	if [ "$skipProject" != "" ] && [ "$skipProject" != "0" ]; then
		echo "Skipping project $CONFIG_JOB_PROJECT"
		return 0
	fi

	for bd in board/mathworks/${CONFIG_JOB_PROJECT}/boards/*; do
		board=$(basename $bd)
		catalog=${bd}/catalog.xml
		if [ ! -e $catalog ]; then
			continue
		fi
		skipBoard=$(board_variable $board SKIP)
		if [ "$skipBoard" != "" ] && [ "$skipBoard" != "0" ]; then
			echo "Skipping board $board"
			continue
		fi
		echo "Building $board"
		run_build_command "-c $catalog" "$target" $extraargs
	done
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
	sources_common)
	  	echo "Preparing Common Sources"
		# Use an arbitrary board
		catalog_file=board/mathworks/zynq/boards/zed/catalog.xml	
		CONFIG_JOB_PLATFORM=zynq
		CONFIG_PLATFORM_NOSKIP="true"
		run_build_command "-c $catalog_file" source --ccache-clean
		rm -rf $CONFIG_BR2_DL_DIR/zynq/linux-*
		rm -rf $CONFIG_BR2_DL_DIR/zynq/uboot-*
		cp -rf $CONFIG_BR2_DL_DIR/zynq $CONFIG_BR2_DL_DIR/socfpga
		cp -rf $CONFIG_BR2_DL_DIR/zynq $CONFIG_BR2_DL_DIR/zynqmp
		rm -rf ${CI_PROJECT_DIR}/output/*/build
		;;
	build)
		echo "Building $CONFIG_JOB_PROJECT"
		build_catalogs "all" -u -q
		;;
	sysroot)
		echo "Packaging $CONFIG_JOB_PLATFORM Sysroot"
		CONFIG_JOB_PROJECT=$CONFIG_JOB_PLATFORM
		run_build_command "-b $CONFIG_JOB_BOARD -p $CONFIG_JOB_PLATFORM" "legal-info all" -u --sysroot
		;;
	*)
		echo "No automation defined for ${CI_BUILD_STAGE}"
		exit 1
		;;
esac

exit 0
