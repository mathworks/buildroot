#!/bin/sh

echo "Starting MW Xenomai TRD..."

cd /mnt

# load the MW axi kernel module
# insmod /mnt/mwipcore.ko

# run dhcp script to obtain IP Address
#udhcpc -s /mnt/dhcp.script
ifconfig eth0 192.168.1.101 netmask 255.255.255.0

# Comment out the sobel TRD to obtain console
# ./run_sobel.sh -qt &


# echo "To re-run this application, type the following commands:"
# echo "cd /mnt"
# echo "./run_sobel.