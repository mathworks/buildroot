 Copyright 2019 The Mathworks, Inc.
 This folder contains required files to build the fsbl for new ZCU102 v1.1 board using Vivado/SDK version 2017.4.
 
 Below are the steps to rebuild the fsbl to support the new board.

 1. Once you have created the fsbl application projct in Vivado, Modify the psu_init.c file.
	i. Comment the function call psu_ddr_phybringup_data() in psu_init function, in the psu_init.c file to disable the ddr phy bringup during psu init.

 2. Copy the two files, xfsbl_ddr_init.c and xfsbl_ddr_init.h from this folder and paste it inside the "src" directory of the fsbl application project.

 3. Replace the "xfsbl_initialization.c" file inside the "src" directory of fsbl prject with "xfsbl_initializtion.c" available in this folder.


 4. Once you have made these changes, rebuild the fsbl project and create the fsbl executable.

 5. Copy the new fsbl executable into the "buildroot/board/mathworks/zynqmp/boards/zcu102/boot" folder and rebuild the sdcard image.
