#!/bin/sh

DTBO=$(realpath $1)
DTBO_NAME=$(basename $DTBO)
DTB_FIRMWARE=/lib/firmware/${DTBO_NAME}
OVERLAY_FS=/sys/class/fpga_overlay/fpga_overlay0

if [ "${DTBO}" != "${DTB_FIRMWARE}" ]; then
	echo "Copying overlay to firmware directory"
	cp -f $DTBO $DTB_FIRMWARE
fi

echo "remove" > ${OVERLAY_FS}/status
echo "${DTBO_NAME}" > ${OVERLAY_FS}/firmware_file
echo "apply" > ${OVERLAY_FS}/status
