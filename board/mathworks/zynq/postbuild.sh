#!/bin/bash
IMAGE_DIR=$1
# Create the u-boot ramdisk image
CPIO_IMG=${IMAGE_DIR}/rootfs.cpio.gz
UIMAGE=${IMAGE_DIR}/uramdisk.image.gz
mkimage -A arm -T ramdisk -C gzip -d $CPIO_IMG $UIMAGE