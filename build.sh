#!/bin/bash

#############################
# Auto build/template script
#############################

BR_DIR=$( cd "$( dirname "$0" )" && pwd )
DYNCONFIG=dynconfig
OUTPUTDIR=${BR_DIR}/output
MYSHELL=`echo $SHELL | sed -e 's/.*\///'`
LOCALCONFIG=${BR_DIR}/.localconfig
CLEANDL=0
UPDATE=0
echoerr() { echo "Error: $@" 1>&2; }

usage() {
	cat >&2 <<EOL

Usage: build.sh [options]<board_dir> <configname>
Generate and build a buildroot dynamic configuration
parameters:
	<board_dir> 			path to the board directory containing a build.sh script
	<configname>			arguement to pass to the build.sh script
options:
	--target| -t target		target to pass to make command, useful for downloading
					sources before launching multiple builds
	--cleandl| -c	        clean the download directory before building
EOL
	exit 1
}

OPTPARSE=`getopt -o hcut: -l help,cleandl,update,target: -n 'build.sh' -- "$@"`

strip_quote() {
    echo $1 | sed -e "s/^'\|'$//g"
}

parse_cmds() {

    while true; do
	    CMD=$1
	    ARGC=$#
	    case "${CMD}" in
	    --help|-h)
		    usage
		    ;;
        --cleandl|-c)
		    shift		
		    CLEANDL=1
		    ((ARGC=ARGC-1))
		    ;;
        --update|-u)
            shift
		    UPDATE=1
		    ((ARGC=ARGC-1))
		    ;;
	    --target|-t)
		    shift		
		    MKTGT=$(strip_quote $1)
		    shift
		    ((ARGC=ARGC-2))
		    ;;
        -- ) 
            shift
		    ((ARGC=ARGC-1))
            break;
            ;;
	    *)
            break
		    ;;
	    esac
    done
	
	if [ "$ARGC" -ne 2 ]; then
		echoerr "Incorrect syntext"	
		usage
	fi

    
	BOARD_DIR=$(strip_quote $1)
    COMMON_DIR=$( cd "$( dirname "$BOARD_DIR" )" && pwd )/common	
    CONFIG=$(strip_quote $2)
}

set_title() {
	title=$1
	case "${TERM}" in
		screen)
			echo -ne "\033k${title}\033\ "
		;;
	esac

}

local_override() {
	if [ -f ${LOCALCONFIG} ]; then
		cat ${LOCALCONFIG} >> configs/${DYNCONFIG}_defconfig
	fi
}
# Parse the input options / commands
parse_cmds $OPTPARSE



# Get the specified board dir's build.sh to generate the dynamic defconfig
BUILD_SH=${COMMON_DIR}/build.sh

if [ ! -d ${BOARD_DIR} ]; then
	echoerr "${BOARD_DIR} is not a directory"
	usage
fi

if [ ! -x ${BUILD_SH} ]; then
	echoerr "${BUILD_SH} is not an executatble file"
	usage
fi

# Execute the script
${BUILD_SH} ${BR_DIR} ${DYNCONFIG} ${BOARD_DIR} ${CONFIG}

if [ $? -eq 0 ]; then
	# Apply any local overrides
	local_override
	# Clean the dl directory
	if [ $CLEANDL -eq 1 ]; then
		rm -rf ${BR_DIR}/dl
	fi
	# Navigate to the target directory
	TGTDIR=${OUTPUTDIR}/${CONFIG}
	mkdir -p ${TGTDIR}
	cd ${TGTDIR}
	make O=${TGTDIR} -C ${BR_DIR} ${DYNCONFIG}_defconfig
	rm ${BR_DIR}/configs/${DYNCONFIG}_defconfig
	set_title ${CONFIG}
    if [ $UPDATE -eq 0 ]; then
        make clean
    fi
	make ${MKTGT}
	set_title ${MYSHELL}
fi

