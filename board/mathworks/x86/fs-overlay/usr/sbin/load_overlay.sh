#!/bin/sh

DTBO=$(basename $1)
OVERLAY_FS=/sys/class/fpga_overlay/fpga_overlay0

echo "remove" > ${OVERLAY_FS}/status
echo $DTBO > ${OVERLAY_FS}/firmware_file
echo "apply" > ${OVERLAY_FS}/status
