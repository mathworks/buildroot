#!/bin/sh

# load the MW axi kernel module
modprobe mwipcore
modprobe mwgeneric_of


echo "Starting MathWorks Linux image..."
cd /mnt
