#!/bin/sh

source /etc/bootvars.conf

SD_DEV=$1

# Wipe the uboot environment
dd if=/dev/zero of=${SD_DEV} seek=512 bs=1 count=4k status=none 2>/dev/null

# Reformat the disk
sfdisk --delete ${SD_DEV}
sfdisk ${SD_DEV} <<EOF
$((1*1024*1024/512)),$((250*1024*1024/512)),b,*
,$((10*1024*1024/512)),a2
,,83
EOF

# Make a DOS boot partition
mkfs -t fat ${SD_DEV}p1
# Make an ext3 rootfs partition
mkfs -t ext4 -F ${SD_DEV}p3
tune2fs -i 0 ${SD_DEV}p3
tune2fs -c 0 ${SD_DEV}p3


# Mount the folders
mkdir -p ${_SD_ROOT}/p1
mount ${SD_DEV}p1 ${_SD_ROOT}/p1
mkdir -p ${_SD_ROOT}/p3
mount ${SD_DEV}p3 ${_SD_ROOT}/p3

#Unpack the partition contents
unzip -d ${_SD_ROOT}/p1 sd.zip
tar -C ${_SD_ROOT}/p3 -xzf rootfs.tar.gz

# Flash the a2 partition
cat boot.a2 > ${SD_DEV}p2

# Unmount the folders
umount ${_SD_ROOT}/p1
rm -rf ${_SD_ROOT}/p1
umount ${_SD_ROOT}/p3
rm -rf ${_SD_ROOT}/p3





