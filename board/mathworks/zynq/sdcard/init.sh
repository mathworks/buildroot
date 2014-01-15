#!/bin/sh

echo "Starting MathWorks Linux image..."

cd /mnt

# load the MW axi kernel module
modprobe mwipcore
modprobe mwgeneric

# run dhcp script to obtain IP Address
#udhcpc -s /mnt/dhcp.script
ifconfig eth0 192.168.1.101 netmask 255.255.255.0
