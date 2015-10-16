#!/bin/sh


RESTART_NETWORK=false


if [ -f /mnt/hostname ]
then
    echo "+++ Updating device hostname"
    mv -f /mnt/hostname /etc/hostname
    sync
    RESTART_NETWORK=true
fi

if [ -f /mnt/interfaces ]
then
    echo "+++ Updating network interfaces"
    # Make the network-interface directories if not already present
    mv -f /mnt/interfaces /etc/network/interfaces
    sync
    RESTART_NETWORK=true
fi

if [ "$RESTART_NETWORK" = true ]; then
    echo "+++ Restarting the network"
    # Restart network so that interfaces file change takes effect
    /etc/init.d/*network restart
fi

# load the MW axi kernel module
modprobe mwipcore
modprobe mwgeneric


echo "Starting MathWorks Linux image..."
cd /mnt
