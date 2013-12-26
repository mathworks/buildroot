#!/bin/bash

#############################
# Auto build/template script
#############################

BR_DIR=$( cd "$( dirname "$0" )" && pwd )
DYNCONFIG=dynconfig
OUTPUTDIR=${BR_DIR}/output
MYSHELL=`echo $SHELL | sed -e 's/.*\///'`

echoerr() { echo "Error: $@" 1>&2; }

usage() {
	cat >&2 <<EOL

Usage: build.sh <board_dir> <configname>
Generate and build a buildroot dynamic configuration
parameters:
	<board_dir> 			path to the board directory containing a build.sh script
	<configname>			arguement to pass to the build.sh script
EOL
	exit 1
}

parse_cmds() {
	CMD=$1
	
	case "${CMD}" in
	--help|-h)
		usage
		;;
	*)
		;;
	esac
	
	if [ "$#" -ne 2 ]; then
		echoerr "Incorrect syntext"	
		usage
	fi
}

set_title() {
	title=$1
	case "${TERM}" in
		screen)
			echo -ne "\033k${title}\033\ "
		;;
	esac

}

parse_cmds $@

BOARD_DIR=$1
CONFIG=$2
BUILD_SH=${BOARD_DIR}/build.sh

if [ ! -d ${BOARD_DIR} ]; then
	echoerr "${BOARD_DIR} is not a directory"
	usage
fi

if [ ! -x ${BUILD_SH} ]; then
	echoerr "${BUILD_SH} is not an executatble file"
	usage
fi

# Execute the script
${BUILD_SH} ${BR_DIR} ${DYNCONFIG} ${CONFIG}
if [ $? -eq 0 ]; then
	# Navigate to the target directory
	TGTDIR=${OUTPUTDIR}/${CONFIG}
	mkdir -p ${TGTDIR}
	cd ${TGTDIR}
	make O=${TGTDIR} -C ${BR_DIR} ${DYNCONFIG}_defconfig
	rm ${BR_DIR}/configs/${DYNCONFIG}_defconfig
	set_title ${CONFIG}
	make clean && make
	set_title ${MYSHELL}
fi

