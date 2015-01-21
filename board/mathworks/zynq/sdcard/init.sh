#!/bin/sh

# Uncomment following lines for viewing verbose kernel debug messages
# dmesg -n 8
# echo 7 > /proc/sys/kernel/printk

echo "+++ Configuring device hostname"
if [ -f /mnt/hostname ]
then
    cp -f /mnt/hostname /etc/hostname
else
    echo "### hostname file not found in the SD card. Using default settings in /etc/hostname..."
fi

echo "+++ Configuring network interfaces"
if [ -f /mnt/interfaces ]
then
    # Make the network-interface directories if not already present
    cp -f /mnt/interfaces /etc/network/interfaces
else
    echo "### interfaces file not found in the SD card. Using default settings in /etc/interfaces..."
fi

# Restart network so that interfaces file change takes effect
/etc/init.d/*network restart

echo "### Starting MathWorks Linux image..."
cd /mnt

# Program the FPGA if system.bit.bin file is present
if [ -f system.bit.bin ]
then
    /usr/bin/zynqfpgaprog system.bit.bin 
fi

# load the MW axi kernel module
modprobe mwadma 
modprobe mwgeneric

# restore boot-environment: required for automated test env.
# test will overwrite this to use axi4stream
cp -f _default_uboot_env_ uboot.env
rm -f system.bit.bin
sync
sync

