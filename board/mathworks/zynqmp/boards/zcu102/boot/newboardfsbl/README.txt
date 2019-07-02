This folder contains required files to build the fsbl for new zcu102v1.1 board.
Below are the steps to rebuild the fsbl to support the new board.

1. Having creating the fsbl application projct in Vivado, Modify the psu_init.c file to disable the ddr initialization during psu_init.
	i. Comment the function call psu_ddr_init_data() in psu_init function.

2. Add the two files, xfsbl_ddr_init.c and xfsbl_ddr_init.h inside the "src" directory of the fsbl application project.

3. Take the file "xfsbl_initialization.c" from this folder and replace it with the "xfsbl_initialization.c" file inside the "src" directory of fsbl project.


4. Having made these changes, rebuild the fsbl project and create the fsbl executable.

5. Take the new fsbl executable and put it inside "buildroot/board/mathworks/zynqmp/boards/zcu102/boot" folder and rebuild the sdcard image.
