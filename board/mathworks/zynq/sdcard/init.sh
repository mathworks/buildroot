#!/bin/sh

# Run the default ARM application if file is present
if [ -f /mnt/system.elf ]; then
    echo "+++ Launching default application..."
    /mnt/system.elf > /tmp/system.elf.log &
    echo $! > /tmp/system.elf.pid
fi

echo "### Starting MathWorks Linux image..."
cd /mnt


