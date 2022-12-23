#!/bin/bash

# Add any custom code to be run at startup here
is_rfdc_exist=$(ls /sys/bus/platform/devices/*)
if [[ $is_rfdc_exist = *usp_rf_data_converter* ]]; then
   echo "Running rftool application..."
   rftool 1 &
   echo "rftool application closed"
fi
