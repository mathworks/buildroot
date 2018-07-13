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

get_br2_variables() {
	local mask=$1
	if [ "$mask" != "" ]; then
		mask="_${mask^^}"
	fi
	for br2 in ${!CONFIG_*}; do
		# Check if it matches the mode
		if [ ${br2%%${mask}_BR2_*} == "CONFIG" ]; then
			echo "--brconfig ${br2#CONFIG${mask}_}=${!br2}"; 
		fi
	done
}

append_br_vars() {
	local mask=$1
	shift
	local brvars=$@
	local new_brvars=$(get_br2_variables $mask)
	if [ "$new_brvars" != "" ]; then
		brvars="$brvars $new_brvars"
	fi
	echo $brvars
}

##############################
# Resolve all buildroot source variables
#############################
resolve_br_vars() {

	brvars=$(get_br2_variables)
	if [ "${CONFIG_JOB_PLATFORM}" != "" ]; then
		brvars=$(append_br_vars ${CONFIG_JOB_PLATFORM} $brvars)
	fi;
	if [ "${CONFIG_JOB_PROJECT}" != "" ]; then
		brvars=$(append_br_vars ${CONFIG_JOB_PROJECT} $brvars)
	fi;
	if [ "${CONFIG_JOB_PROJECT}" != "" ] && [ "${CONFIG_JOB_PLATFORM}" != "" ]; then
		brvars=$(append_br_vars ${CONFIG_JOB_PLATFORM}_${CONFIG_JOB_PROJECT} $brvars)
	fi;
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

    set -x
	${CI_PROJECT_DIR}/build.py --target "$target" --dl $CONFIG_BUILDROOT_DL_ROOT/$CONFIG_JOB_PLATFORM \
			$build_spec --ccache -l logs/${CI_JOB_NAME}.log.tmp \
			-d images/${CONFIG_JOB_PROJECT}/ $brvars $extraargs
	rc=$?
	set +x

    cat logs/${CI_JOB_NAME}.log.tmp >> logs/${CI_JOB_NAME}.log
	rm -f logs/${CI_JOB_NAME}.log.tmp

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
		board_brvars=$(get_br2_variables $board)
		echo "Building $board"
		run_build_command "-c $catalog" "$target" $board_brvars $extraargs
	done
}

##############################
# Prep the git credentials
#############################
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
# Deploy
#############################
run_deploy_command() {
    if [ "${CONFIG_DEPLOY_URL_ROOT}" != "" ] && [ "$CONFIG_DEPLOY_PATH_ROOT" != "" ]; then
        if [ -d ${CONFIG_DEPLOY_PATH_ROOT} ]; then
            deploy_path=${CONFIG_DEPLOY_PATH_ROOT}/${CI_ENVIRONMENT_NAME}
            ${CONFIG_GITLAB_CMD} mkdir -p $deploy_path
            for d in images logs; do
                ${CONFIG_GITLAB_CMD} rm -rf ${deploy_path}/$d
                echo "Copying $d to $deploy_path"
                ${CONFIG_GITLAB_CMD} cp -r $d ${deploy_path}/
                rc=$?
                if [ $rc != 0 ]; then
                    echo "Deploying $d failed"
                    exit $rc
                fi
            done
        fi
    fi
}

##############################
# Main Script
#############################
prep_git_credentials

case "${CI_JOB_STAGE}" in
	sources_common)
	  	echo "Preparing Common Sources"
		# Use an arbitrary board
		catalog_file=board/mathworks/zynq/boards/zed/catalog.xml	
		CONFIG_JOB_PLATFORM=zynq
		CONFIG_PLATFORM_NOSKIP="true"
		run_build_command "-c $catalog_file" source --ccache-clean
		rm -rf $CONFIG_BUILDROOT_DL_ROOT/zynq/linux-*
		rm -rf $CONFIG_BUILDROOT_DL_ROOT/zynq/uboot-*
		cp -rf $CONFIG_BUILDROOT_DL_ROOT/zynq $CONFIG_BUILDROOT_DL_ROOT/socfpga
		cp -rf $CONFIG_BUILDROOT_DL_ROOT/zynq $CONFIG_BUILDROOT_DL_ROOT/zynqmp
		rm -rf ${CI_PROJECT_DIR}/output/*/build
		;;
	build)
		echo "Building $CONFIG_JOB_PROJECT"
		build_catalogs "all" -u -q
		;;
	sysroot)
		echo "Packaging $CONFIG_JOB_PLATFORM Sysroot"
		if [[ -z "$CONFIG_JOB_PROJECT" ]]; then
			CONFIG_JOB_PROJECT=$CONFIG_JOB_PLATFORM
		fi
		run_build_command "-b $CONFIG_JOB_BOARD -p $CONFIG_JOB_PLATFORM" "legal-info all" -u --sysroot
		;;
    deploy)
        echo "Deploying to $CI_ENVIRONMENT_NAME"
        run_deploy_command
        ;;
	*)
		echo "No automation defined for ${CI_JOB_STAGE}"
		exit 1
		;;
esac

exit 0
