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

# load the soundcard config, if present
if [ -d /sys/class/sound/card0 ]; then
	echo "+++ Loading alsa settings"
	alsactl restore -f /mnt/asound.state 0
fi

# Program the FPGA if system.bit file is present
if [ -f /mnt/system.bit.bin ]
then
    echo "+++ Loading default bitstream..."
    /usr/bin/zynqfpgaprog /mnt/system.bit.bin
fi

# load the MW axi kernel module
modprobe mwadma
modprobe mwgeneric_of

# Run the default ARM application if file is present
if [ -f /mnt/system.elf ]; then
    echo "+++ Launching default application..."
    /mnt/system.elf > /tmp/system.elf.log &
    echo $! > /tmp/system.elf.pid
fi

echo "### Starting MathWorks Linux image..."
cd /mnt


