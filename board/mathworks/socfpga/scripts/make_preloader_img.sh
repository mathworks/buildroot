#!/bin/bash 

#################################################
# Internals
SELF=$(basename $0)
SELF_PATH=$(dirname $0)

#################################################
# constants
CONST_TMP_DIR=./tmp
CONST_JAVA_CMD=$(which java)
CONST_BUILDHEADER_CMD="$SELF_PATH/mkpimage.jar com.altera.embeddedsw.preloader.PreloaderImageTool"
CONST_IMG_BLOCK_SIZE_KB=64		       # size of a block for preloaders
CONST_IMG_NUM_BLOCKS=4

#################################################
# defaults
DEF_OUTPUT_FILE=bundle.bin
DEF_CLEANUP=no

OUTPUT_FILE=${DEF_OUTPUT_FILE}
NO_IMAGE=0
PRELOADER=./spl/u-boot-spl.bin
BUILDHEADER_CMD=${CONST_BUILDHEADER_CMD}
CLEANUP=${DEF_CLEANUP}


#################################################
# functions
function usage() {


    echo "usage: ${SELF} [-h] -p preloader [-o output] [-t path to toolchain] [-c]"
    echo "-h		this message"
    echo "-o output	output file. Default is ${DEF_OUTPUT_FILE}"
    echo "-p preloader  preloader raw binary file (default: ${PRELOADER})."
    echo "-c            remove tmp files before finishing"

    return
}

#################################################
# script starts here

# Here we parse options
while [ $# -gt 0 ] ; do
    case $1 in
        -c) CLEANUP=yes ;;
        -h) usage ; exit 0 ;;
        -ni) NO_IMAGE=1 ;;
        -o) OUTPUT_FILE=$2 ; shift ;;
        -p) PRELOADER=$2 ; shift ;;
        -t) TC_PREFIX=$2 ; shift ;;
        *)  echo "${SELF}: error: unknown option $1" ; usage ; exit ;;
    esac
    shift
done

#  Check for error and print message and exit
#! input
#!  $1 = error code
#!  $2 = message
#! output
#!  message printed when $1 -ne 0
#! return
#!  none
function check_error() {

    local code=$1
    local message="$2"

    if [ ${code} -ne 0 ] ; then
        echo "$2"
        exit -1
    fi

    return
}

#  this function puts the preloader images (wrapped) 
#! into a single image
#! input:
#!  $1: image name
#!  $2->$5: preloader binaries. If $1 is set only,the same binary will be used
#! output:
#!  the consolidated image
#! return
#!  0 on success, otherwise on sucksess
#! note
#!  it is assumed that the image name is set. No check.
function collate() {

    local image_name=$1
    local blk
    local i
    local unique
 
    # create a sparse image
    dd if=/dev/zero of=${image_name} bs=1 seek=$((${CONST_IMG_BLOCK_SIZE_KB}*1024*${CONST_IMG_NUM_BLOCKS})) count=0 2>/dev/null
    if [ $? -ne 0 ] ; then
       echo "${SELF}: ${FUNCNAME}: error: could not create sparse image."
       return 1
    fi

    blk=$(losetup -s -f ${image_name})
    if [ $? -ne 0 ] ; then
       echo "${SELF}: ${FUNCNAME}: error: could not create loopback device." 
       return 1
    fi

    #  remove the image name from the args list
    shift 

    # we may have only $2 set.
    unique=0
    if [ $# -eq 1 ] ; then
        unique=1
    fi
   
    # copy
    for i in 0 1 2 3 ; do
        dd if=${1} of=${blk} bs=$((${CONST_IMG_BLOCK_SIZE_KB}*1024)) count=1 seek=${i} 2>/dev/null
        if [ $? -ne 0 ] ; then
            echo "${SELF}: ${FUNCNAME}: error could not write block ${i}."
            return 1
       fi

       # move to the next preloader image
       if [ ${unique} -ne 1 ] ; then
           shift
       fi
    done 

    # now we clean up
    losetup -d ${blk}
    if [ $? -ne 0 ] ; then
       echo "${SELF}: ${FUNCNAME}: error: could not remove loopback device." 
       return 1
    fi

    return 0
}

function init() {

    if [ ! -d ${CONST_TMP_DIR} ] ; then
        mkdir -p ${CONST_TMP_DIR}
    fi

    return
}

#################################################
# Tests
if [ ${UID} -ne 0 ] ; then
    echo "${SELF}: error: you need root privileges. Use sudo."
    exit -1
fi

if [ -z ${CONST_JAVA_CMD} ] ; then
    echo "${SELF}: error: java not found."
    exit -1
fi

# preloader binary
if [ -z ${PRELOADER} ] ; then
    usage
    exit 0
fi
if [ ! -e ${PRELOADER} -o ! -f ${PRELOADER} ] ; then
    echo "${SELF}: error: preloader ${PRELOADER} not found."
    exit -1
fi
TMP_FILE=$(basename ${PRELOADER})

# init the environment
init

# create the image
echo "${SELF}: info: using preloader $PRELOADER"
${CONST_JAVA_CMD} -cp ${CONST_BUILDHEADER_CMD} -o ${OUTPUT_FILE} ${PRELOADER}
check_error $? "${SELF}: error: failed to generate preloader image."

# clean up
if [ "${CLEANUP}" == "yes" ] ; then
    echo "${SELF}: info: clean up..."
    rm -f ${CONST_TMP_DIR}/${TMP_FILE}.*
fi

echo "${SELF}: info: done."

