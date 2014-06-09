#!/bin/bash 

#################################################
# Internals
SELF=$(basename $0)
SELFPATH=$(dirname $0)

#################################################
# constants
CONST_DEMO_BASE_PATH=~/demo
CONST_DEMO_BIN_PATH=${CONST_DEMO_BASE_PATH}/bin
CONST_1K=1024
CONST_1M=$((${CONST_1K} * ${CONST_1K}))
CONST_1G=$((${CONST_1M} * ${CONST_1K}))
CONST_RFS_TYPE=ext3

# partition table settings
# unit = bytes. adjustments to satisfy the partitioner tool done below
CONST_PT_BOOT_SIZE=${CONST_1M}
CONST_PT_BOOT_OFFSET=${CONST_1M}
CONST_PT_BOOT_TYPE=a2
CONST_PT_BOOT_NUM=3

# 1/ Root file System partition, Type 83 = Linux EXT
CONST_PT_RFS_TYPE=83
CONST_PT_RFS_NUM=2

# 2/ Parition for the kernel, Device tree, etc. 
CONST_PT_LX_TYPE=b
CONST_PT_LX_NUM=1

# Temporary mount point for RFS
CONST_MNT_POINT="./blob-$(date +%s)"
CONST_TMPDIR=./tmp

#################################################
# defaults
DEF_IMAGE_SIZE="2G"
DEF_IMAGE_NAME="image_blk_demo.bin"
DEF_RFS_TYPE=${CONST_RFS_TYPE}
DEF_DEBUG=0
DEF_MERGE_DIR=./merge

#################################################
# functions
function usage() {


    echo "usage: ${SELF} [-h] [-k f1,f2] [[-p preloader |-rp preloader] -b bootloader] [-r rfs_dir [-m merge_dir]] [-o image] [-g size] [-t tool]"
    echo "-k f1,f2,...   comma separated list of files to be copied into FAT partition, i.e. OS files (zImage, dtb) or FPGA image (rbf)"
    echo "-p preloader   preloader file with mkpimage header (i.e preloader-mkpimage.bin). Mutually exclusive with -rp."
    echo "-rp preloader  raw preloader file (i.e u-boot-spl.bin). Specify your preloader header tool with -t option. Mutually exclusive with -p."
    echo "-b bootloader  bootloader file (i.e. u-boot.img)"
    echo "-r rfs_dir     location of the root file system files."
    echo "-o image       name of generated image file. Default filename is ${DEF_IMAGE_NAME}"
    echo "-m merge_dir   copy files located in dir/ to rfs."
    echo "-g size[K/M/G] size of generated image, in unit KB, MB or GB (i.e 2000M). Default size is ${DEF_IMAGE_SIZE}B"
    echo "-t tool        tool to geneate preloader with header (i.e <path-to>/mkpimage). Use this with -rp option."
    echo "-h             this message"
}

#  This function returns the name of the distribution
#! Input:
#!  none
#! Output
#!  Distro's name as follows:
#!   c = centos
#!   u = ubuntu
#! return
#!  0 on success
function get_distro() {

    local distro
    distro=$(lsb_release -i | awk -F: ' { print $2 } ' | sed -e 's/\t *//')
    if [ $? -ne 0 ] ; then
       echo "${SELF}: ${FUNCNAME}: error: could not get distro name."
       return 1
    fi

    case ${distro} in
        CentOS)
            echo 'c'
            ;;
        Ubun*)
            echo 'u'
            ;;
        Debian)
            echo 'd'
            ;;
        *)
            echo "unknown(${distro})"
            ;;
    esac 

    return 0
}

#  create a sparse image
#! input:
#!   $1 = image name
#!   $2 = image size
#! output:
#!   none
#! return
#!   0 on success
function create_sparse_image() {

    local image_name="$1"
    local image_size=$2

    #  here we allocate the space, without actually writing any byte to the file
    #! this speeds it up!
    dd if=/dev/zero of=${image_name} bs=1 seek=${image_size} count=0 >/dev/null 2>&1
    return $?
 
}

#  add a loopback device 
#! input:
#!  $1 = image name
#!  $2 = offset
#! output:
#!  loopback device name
#! return:
#!  0 on success
function add_loopback_device() {

    local image_name="$1"
    local offset="${2:-0}"
    local size="${3:-0}"
    local sizeargs=

    if [ ${size} -gt 0 ] ; then
        sizeargs="--sizelimit ${size}"
    fi
   
    losetup -s -f -o ${offset} ${sizeargs} ${image_name} 
    return $?
}

#  remove a loopback device
#! input:
#!  $1 = loopback device
#! output:
#!  none
#! return
#!  0 on success
function remove_loopback_device() {

    local loop="$1"
    local err=0

    sync
    for i in $(seq 0 9) ; do
    	losetup -d ${loop}
        err=$?
        if [ ${err} -eq 0 ] ; then
		break
        fi
        sleep 1
    done

    return ${err}
}

#  add a partition table to an image
#! input
#!  $1 = loop back device
#! output
#!  none
#! return
#!  0 on success
#! note:
#!  the parameters of the partition table are here
#!  as a "here" document
function make_partition_table() {

    local loop=$1
    local distro
    local option
    local command

    distro=$(get_distro)
    case ${distro} in 
        c)
            command=u
            option=""
            ;;
        [ud])
            command=""
            option="-u=sectors"
            ;;
        *)
            echo "${SELF}: ${FUNCNAME}: error: distro unknown... (${distro})"
            return 1
            ;;
    esac

    cat <<EOT | fdisk ${option} ${loop} >/dev/null 2>&1 
${command}
n
p
${CONST_PT_RFS_NUM}
$((${CONST_PT_RFS_OFFSET}/512))
+$((${CONST_PT_RFS_SIZE}/1024))K
t
${CONST_PT_RFS_TYPE}
n
p
${CONST_PT_LX_NUM}
$((${CONST_PT_LX_OFFSET}/512))
+$((${CONST_PT_LX_SIZE}/1024))K
t
${CONST_PT_LX_NUM}
${CONST_PT_LX_TYPE}
n
p
${CONST_PT_BOOT_NUM}
$((${CONST_PT_BOOT_OFFSET}/512))
+$((${CONST_PT_BOOT_SIZE}/1024))K
t
${CONST_PT_BOOT_NUM}
${CONST_PT_BOOT_TYPE}
w
EOT
   #  sometimes, the kernel fails to read the updated partition table
   #! partprobe comes to help...
   RET=$?
   if [ ${RET} -eq 1 ] ; then
       partprobe ${loop}
       RET=$?
       if [ $? -ne 0 ] ; then
           echo "${SELF}: ${FUNCNAME}: error: partprobe failed"
           RET=1
       fi
   else
       echo "${SELF}: ${FUNCNAME}: error: fdisk failed $RET"
       RET=2
   fi       

   return ${RET}
}

#  format a partition
#! input:
#!  $1 = block device
#!  $2 = type. 
#! output:
#!  none
#! return
#!  0 on success, !0 on sucksess
#! note:
#!  - valid types are: ext2, ext3, ext4, fat
function format_partition() {

    local blk=$1
    local ptype=$2
    local command
    local flags

    case ${ptype} in
        ext2)
            command=mkfs.ext2
            flags=""
            ;;
        ext3)
            command=mkfs.ext3
            flags=""
            ;;
        ext4)
            command=mkfs.ext4
            flags=""
            ;;
        fat)
            command=mkfs.vfat
            flags=""
            ;;
        *)
            echo "${SELF}: ${FUNCNAME}: error: unknown partition type ${ptype}."
            return 1
            ;;
    esac

    ${command} ${flags} ${blk} >/dev/null 2>&1
    if [ $? -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: command ${command} failed."
        return 2
    fi

    return 0
}

#  get the type of a partition
#! input:
#!  mount point, absolute path
#! ouput:
#!  file system type
#! return
#!  0 on success
function get_fs_type() {

    local blk=$1
    local ftype

    ftype=$(cat /etc/mtab | grep ${blk} | awk ' { print $3 } ' | sed -e 's/vfat/fat/')
    if [ $? -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: could not get type of ${blk}."
        return 1
    fi

    echo ${ftype}

    return 0
}


#  copy the a file to a partition
#! input:
#!  $1: block device
#!  ${2,3,4,...}: file/dir
#! output:
#!  none
#! return
#!  0 on success
function copy_to_partition() {

    local BLK=$1
    local file
    local ftype
    local cpflags

    # we create the dir for the mount point
    create_directory ${CONST_MNT_POINT}
    if [ $? -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: could not create dir ${CONST_MNT_POINT}."
        return 1
    fi

    # need to mount the partition
    mount ${BLK} ${CONST_MNT_POINT}
    if [ $? -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: could not mount partition."
        return 2
    fi

    #  fat does not provide some of the attributes that a regular
    #! file system does
    ftype=$(get_fs_type ${BLK})
    if [ $? -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: could not get partition type".
        return 6
    fi

    case ${ftype} in 
        fat)
            cpflags="-rL"
            ;;
        *)
	    cpflags="-rpaL"
            ;;
    esac

    # now we copy
    # we want the rest of the args to be the files to copy...
    shift
    while [ $# -gt 0 ] ; do
        f=$(make_path_absolute "$1")
        if [ ! -e ${f} ] ; then
            echo "${SELF}: ${FUNCNAME}: error: ${f}: no such file or directory."
            return 3
        fi

        cp ${cpflags} "${f}" ${CONST_MNT_POINT} 
        if [ $? -ne 0 ] ; then
            echo "${SELF}: ${FUNCNAME}: error could not copy ${f}."
            return 4
        fi
  
        # move on to the next file
        shift
    done

    # umount
    err=0
    for i in 0 1 2 3 4 ; do 
	    sync && sync && sleep 1
       	    umount ${CONST_MNT_POINT}
            err=$?
            if [ ${err} -eq 0 ] ; then
                break
            fi
            sleep 1
    done
    if [ ${err} -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: could not unmount ${CONST_MNT_POINT}"
        return 5
    fi 


    remove_directory ${CONST_MNT_POINT}
    if [ $? -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: could not remove directory."
        return 7
    fi

    return 0
}

#  do a raw copy to a block device
#! input
#!  $1=block device
#!  $2=image file
#! output
#!  none
#! return 
#!  0 on success
function raw_copy_to_blk() {

    local blk=$1
    local image=$2

    dd if=${image} of=${blk} bs=512 2>/dev/null
    if [ $? -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: could not copy image."
        return 1
    fi

    return 0
}

#  do merge. Relies on copy_to_partition
#! input:
#!  $1 = block device to use
#!  $2 = directory with files
#! output:
#!  none
#! return
#!  0 on success
#! notes
#!  it is assumed that the directory exists!
function do_merge() {

    local BLK=$1
    local merge_dir=$2
    local ret
    
    copy_to_partition ${BLK} ${merge_dir}/*
    return $?
}

#  create the directory 
#! input: 
#!  directory 
#! output:
#!  none
#! return
#!  0 on success. 
#!  if directory exists, 0 is returned
#!  something else otherwise
function create_directory() {

    local dir="$1"

    if [ -d ${dir} ] ; then
        return 0
    fi

    if [ -e ${dir} ] ; then
        echo "${SELF}: ${FUNCNAME}: error: ${dir} exists, but is not a directory."
        return 1
    fi

    mkdir -p ${dir}
    if [ $? -ne 0 ] ; then
       echo "${SELF}: ${FUNCNAME}: error: could not create ${dir}."
       return 2
    fi

    return 0
}

#  remove a directory
#! input
#!  $1 = directory to remove
#! output
#!  none
#! return
#!  0 on success
function remove_directory() {

    local dir="$1"

    dir=$(make_path_absolute ${dir})
    rm -r ${dir}
    if [ $? -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: could not remove directory ${dir}."
        return 1
    fi

    return 0
}

#  formats and copies the RFS files into the partition
#! input:
function init_rfs() {

    local loop=$1
    local rfs_dir="$2"
    local rfs_type=$3

    [ ${DEBUG} -eq 1 ] && echo "${SELF}: ${FUNCNAME}: info: formatting partition..."
    #mkfs.ext3 ${loop} >/dev/null 2>&1
    format_partition ${loop} ext3
    if [ $? -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: mkfs failed."
        return 1
    fi    

    [ ${DEBUG} -eq 1 ] && echo "${SELF}: ${FUNCNAME}: info: mounting partition..."
    create_directory ${CONST_MNT_POINT}
    if [ $? -ne 0 ] ; then
        echo ${SELF}: ${FUNCNAME}: error: could not create ${CONST_MNT_POINT}.
        return 1
    fi

    mount ${loop} ${CONST_MNT_POINT} >/dev/null 2>&1
    if [ $? -ne 0 ] ; then
        echo ${SELF}: ${FUNCNAME}: error: could not mount.
        return 1
    fi 
    
    [ ${DEBUG} -eq 1 ] && echo "${SELF}: ${FUNCNAME}: info: copying RFS files from ${rfs_dir}..."
    #cp -rpa ${rfs_dir}/* ${CONST_MNT_POINT}
    tar -C ${CONST_MNT_POINT} -xf ${rfs_dir}
    if [ $? -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: failed to copy files."
        return 1
    fi   
    sync
    
    [ ${DEBUG} -eq 1 ] && echo "${SELF}: ${FUNCNAME}: info: unmounting..."
    umount ${CONST_MNT_POINT} >/dev/null 2>&1
    if [ $? -ne 0 ] ; then
        echo "${SELF}: ${FUNCNAME}: error: failed to umount."
        return 1
    fi 

    rm -rf ${CONST_MNT_POINT}

    return 0
}
  
#  take a path and make it absolute
#! input:
#!  $1 = path to "absolutize"
#! output:
#!  absolute path
#! return
#!  0 on success
function make_path_absolute() {

    local path="$1"
    local abs_path

    # check if path starts with /
    echo $path | egrep -e '^/' >/dev/null 2>&1
    if [ $? -eq 0 ] ; then
        abs_path=${path}
    else
        # may start with ~
        echo ${path} | egrep -e '^~' >/dev/null 2>&1
        if [ $? -eq 0 ] ; then
            abs_path=$(echo $path | sed -e 's,^~,'${HOME}',')
        else
            abs_path=$(pwd)/${path}
        fi
    fi


    echo ${abs_path}

    # not really necessary, just for deedee the monster
    return 0  
}

#  check if a file exists and returns an absolute path to it
#! input
#!  $1=message
#!  $2=type of $3. Either d or f
#!  $3=file/dir to check
#! output
#!  absolute path to file
#! return
#!  0 on success, exit otherwise
function check_file () {

    local message="$1"
    local type=$2
    local file=$3

    if [ -z ${file} ] ; then
        echo "${SELF}: ${FUNCNAME}: error: ${message}"
        usage
        exit -1
    fi
    file=$(make_path_absolute "${file}")
    case ${type} in 
        f) 
            if [ ! -f ${file} ] ; then
                echo "${SELF}: ${FUNCNAME}: error: ${file}: no such file."
                exit -1
            fi
            ;;
        d)
            if [ ! -d ${file} ] ; then
                echo "${SELF}: ${FUNCNAME}: error: ${file}: no such directory."
                exit -1
            fi
            ;;
 
    esac

    echo ${file}

    return 0
}

#  extract the list of files and check each file exists
#! input
function get_os_file_list() {

	local raw_list=""
	local raw_a_list=""

	raw_list=$(echo $1 | sed -e 's/,/ /g')

	for f in ${raw_list} ; do
		f=$(check_file f "no such file or directory" ${f})
		raw_a_list="$(echo ${f} ${raw_a_list})"
	done

	echo ${raw_list}

	return 0
}

#################################################
# script starts here
# Here we parse options
PRELOADER_IMAGER_TOOL=${DEF_PRELOADER_IMAGER_TOOL}
IMAGE_NAME=${DEF_IMAGE_NAME}
IMAGE_SIZE=${DEF_IMAGE_SIZE}
IMAGE_OFFSET=0
RFS_TYPE=${DEF_RFS_TYPE}
RFS_DIR=
MERGE_DIR=
PRELOADER=
BOOTLOADER=
OS_FILES=
DEBUG=${DEF_DEBUG}
DO_CMD_MKPIMAGE=no
CMD_MKPIMAGE=

if [ $# -eq 0 ]; then
    usage
    exit 0
fi

while [ $# -gt 0 ] ; do
    case $1 in
        -h) usage ; exit 0 ;;
        -k) OS_FILES=$2 ; DO_OS=yes ; shift ;;
        -r) RFS_DIR=$2 ; DO_RFS=yes ; shift ;;
        -o) IMAGE_NAME=$2 ; shift ;;
        -m) MERGE_DIR=$2 ; DO_RFS=yes ; shift ;;
        -s) DEBUG=1 ;;
        -p) PRELOADER=$2 ; DO_PRELOADER=yes ; PRELOADER_MKPIMAGE=yes ; shift ;;
        -rp) PRELOADER=$2 ; DO_PRELOADER=yes ; DO_GEN_PRELOADER=yes ; shift ;;
        -b) BOOTLOADER=$2 ; DO_PRELOADER=yes ; shift ;;
        -g) IMAGE_SIZE=$2 ; shift ;;
        -t) CMD_MKPIMAGE=$2 ; DO_CMD_MKPIMAGE=yes ; shift ;;
        *)  echo "${SELF}: error: unknown option ${1}" ; usage ; exit ;;
    esac
    shift
done

#################################################
# root only can run this script
if [ ${UID} -ne 0 ] ; then
    echo "${SELF}: error: only root can run this script. Use sudo."
    exit -1
fi

# image exists?
if [ -f ${IMAGE_NAME} ] ; then
    echo "${SELF}: warning: image ${IMAGE_NAME} exists. Remove? [y|n]"
    read yes_or_no;
    if [ "${yes_or_no}" == "y" ] ; then
        rm ${IMAGE_NAME}
    else
        exit -1
    fi
fi

# Calculates image size based on user input or use default image size
# delete trailing character
IMAGE_SIZE_NUMERIC=$(echo $IMAGE_SIZE | sed s/[GMK]$//g)

# delete leading numeric characters and get unit K/M/G
IMAGE_UNIT=$(echo $IMAGE_SIZE | sed 's/[0-9]\+\.\?[0-9]*//g')

# Convert size of image to bytes
if [ $IMAGE_UNIT == "K" ] ; then
	IMAGE_SIZE=$((${IMAGE_SIZE_NUMERIC} * ${CONST_1K}))
elif [ $IMAGE_UNIT == "M" ] ; then
	IMAGE_SIZE=$((${IMAGE_SIZE_NUMERIC} * ${CONST_1M}))
elif [ $IMAGE_UNIT == "G" ] ; then
	IMAGE_SIZE=$((${IMAGE_SIZE_NUMERIC} * ${CONST_1G}))
else
	echo "${SELF}: invalid unit of size. Valid units are K, M or G."
	exit -1
fi

# The following happens no matter how many tasks have to be executed.
# We start by creating a sparse image
echo "${SELF}: info: creating image file..."
create_sparse_image ${IMAGE_NAME} ${IMAGE_SIZE}
if [ $? -ne 0 ] ; then
    echo "${SELF}: create_sparse_image: error: could not create sparse image"
    exit -1
fi

# Calculate EXT and FAT partition size based on image size. Preloader partition size is fixed (1MB).
# Root file System partition, Type 83 = Linux EXT, 50% of image size
CONST_PT_RFS_SIZE=$((${IMAGE_SIZE} * 50 / 100))
CONST_PT_RFS_OFFSET=$((${CONST_PT_BOOT_OFFSET} + ${CONST_PT_BOOT_SIZE} + 5*${CONST_1M}))

# FAT file system, 40% of image size
CONST_PT_LX_SIZE=$((${IMAGE_SIZE} * 40 / 100))
CONST_PT_LX_OFFSET=$((${CONST_PT_RFS_OFFSET} + ${CONST_PT_RFS_SIZE} + 5 * ${CONST_1M}))

# we add a loopback device onto the image
LOOP=$(add_loopback_device ${IMAGE_NAME} ${IMAGE_OFFSET})
if [ $? -ne 0 ] ; then
    echo "${SELF}: add_loopback_device: error: could not add loopback device"
    exit -1
fi

# we now create the partition table
echo "${SELF}: info: creating partition table..."
make_partition_table ${LOOP}
if [ $? -ne 0 ] ; then
    echo "${SELF}: error: could not create partition table"
    exit -1
fi

echo "${SELF}: info: clean up..."
sync && sync && sleep 1	
remove_loopback_device ${LOOP}
if [ $? -ne 0 ] ; then
    echo "${SELF}: error: could not remove loopback device ${LOOP}"
    exit -1
fi


# create the preloader image + bootloader
if [ "X${DO_PRELOADER}" == "Xyes" ] ; then
	echo "${SELF}: info: creating preloader/bootloader image..."
	BOOTLOADER=$(check_file f "no bootloader specified" ${BOOTLOADER})
	PRELOADER=$(check_file f "no bootloader specified" ${PRELOADER})

	# Check if both "-p" and "-rp" options are enabled. If yes, error out.
	if [ "X${PRELOADER_MKPIMAGE}" == "Xyes" ] && [ "X${DO_GEN_PRELOADER}" == "Xyes" ] ; then
		echo "${SELF}: error: both '-p' and '-rp' options are enabled. They should mutually exclusive enable."
		exit -1
	fi

	if [ "X${DO_GEN_PRELOADER}" == "Xyes" ] ; then
		if [ "X${DO_CMD_MKPIMAGE}" == "Xyes" ] ; then
			# Call to mkpimage tool set by user.
			PRELOADER_MKPIMAGE=preloader_image.bin
			rm -f ${PRELOADER_MKPIMAGE}
			# BootROM can support up to 4 preloader images.
			# Use same preloader for 4 images by default.
			# It is good to have 4 preloader images. This is because BootROM
			# will use the next preloader image if warm reset.
			${CMD_MKPIMAGE} -o ${PRELOADER_MKPIMAGE} ${PRELOADER} ${PRELOADER} ${PRELOADER} ${PRELOADER}
			if [ ! -f ${PRELOADER_MKPIMAGE} ] ; then
				echo "${SELF}: error: could not create preloader image."
				exit -1
			fi
		else
			# User needs to specify preloader header generation tool if using raw preloader.
			echo "${SELF}: error: no preloader header tool is specified. Please use -t option to specify."
			exit -1
		fi
	else
		# Preloader passed in by user is with mkpimage header.
		# We can use it directly.
		PRELOADER_MKPIMAGE=${PRELOADER}
	fi

	echo "${SELF}: info: copying preloader image and bootloader to partition..."
	LOOP2=$(add_loopback_device ${IMAGE_NAME} ${CONST_PT_BOOT_OFFSET} ${CONST_PT_BOOT_SIZE})
	if [ $? -ne 0 ] ; then
    		echo "${SELF}: error: could not add loopback device."
    		exit -1
	fi

	raw_copy_to_blk ${LOOP2} ${PRELOADER_MKPIMAGE}
	if [ $? -ne 0 ] ; then
    		echo "${SELF}: error: could not copy preloader image to block ${LOOP2}"
                remove_loopback_device ${LOOP2}
		exit -1
	fi
	PRELOADER_IMAGE_SIZE=256k
	dd if=${BOOTLOADER} of=${LOOP2} seek=${PRELOADER_IMAGE_SIZE} bs=1 conv=fsync 2>/dev/null
	if [ $? -ne 0 ] ; then
    		echo "${SELF}: error: could not copy bootloader image to block ${LOOP2}"
                remove_loopback_device ${LOOP2}
    		exit -1
	fi
	remove_loopback_device ${LOOP2}
	if [ $? -ne 0 ] ; then
    		echo "${SELF}: error: could not remove loopback device ${LOOP2}"
    		exit -1
	fi
fi      # if [ ! -z ${DO_PRELOADER} ]

# here we should copy the kernel, etc.
if [ "X${DO_OS}" == "Xyes" ] ; then
	echo "${SELF}: info: copying OS files, etc..."	

	# get the list of files and check it
	OS_FILE_LIST_RAW="$(get_os_file_list ${OS_FILES})"

	# now move on with copying the files
	LOOP2=$(add_loopback_device ${IMAGE_NAME} ${CONST_PT_LX_OFFSET} ${CONST_PT_LX_SIZE}) 
	if [ $? -ne 0 ] ; then
	    echo "${SELF}: error: could not add loopback device ${LOOP2}"
	    exit -1
	fi
	format_partition ${LOOP2} fat
	if [ $? -ne 0 ] ; then
	        echo "${SELF}: error: failed to format ${LOOP2} as fat"
	        remove_loopback_device ${LOOP2}
	        exit -1
	fi
	copy_to_partition ${LOOP2} ${OS_FILE_LIST_RAW}
	if [ $? -ne 0 ] ; then
	    echo "${SELF}: error: could not copy files"
	    remove_loopback_device ${LOOP2}
	    exit -1
	fi


	remove_loopback_device ${LOOP2}
	if [ $? -ne 0 ] ; then
	    echo "${SELF}: error: could not remove loopback device ${LOOP2}"
	    exit -1
	fi
fi    # if [ "X${D_OS}" == "Xyes" ]

# copy the RFS
if [ "X${DO_RFS}" == "Xyes" ] ; then
 
	LOOP2=$(add_loopback_device ${IMAGE_NAME} ${CONST_PT_RFS_OFFSET} ${CONST_PT_RFS_SIZE})
	if [ $? -ne 0 ] ; then
 	   echo "${SELF}: error: could not add loopback device ${LOOP2}"
           exit -1
 	fi

	echo "${SELF}: info: creating root file system..."

	init_rfs ${LOOP2} ${RFS_DIR} ${RFS_TYPE}
	if [ $? -ne 0 ] ; then
	    echo "${SELF}: error: init_rfs failed."
	    remove_loopback_device ${LOOP2}
	    exit -1
	fi
	
	# add merge if required
	if [ ! -z ${MERGE_DIR} ] ; then
	    echo "${SELF}: info: merging files..."
	    if [ ! "$(ls -A ${MERGE_DIR})" ]; then
	         echo "${SELF}: warning: merge {MERGE_DIR} is empty. Skip"
	    else
	        do_merge ${LOOP2} ${MERGE_DIR}
	        if [ $? -ne 0 ] ; then
	            echo "{$SELF}: error: failed to merge."
	            remove_loopback_device ${LOOP2}
	            exit -1
	        fi
	    fi	
	fi


	echo "${SELF}: info: cleaning up (rfs)..."
	remove_loopback_device ${LOOP2}
	if [ $? -ne 0 ] ; then
	    echo "${SELF}: error: could not remove loopback device ${LOOP2}"
	    exit -1
	fi
fi    # if [ "X${DO_RFS}" == "Xyes" ]

exit 0
