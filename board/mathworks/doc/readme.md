# The MathWorks&reg; Build System

The MathWorks build system wraps Buildroot in pre- and post- processing scripts
in order to automate the generation of system images for various platforms. The
build system is based around scripts that take as input the target
board/platform or image description file and output a complete system image,
including bootloader(s), kernel and user space.

## Overview

The build system combines a number of "shared" Buildroot config settings into a
configuration specific to the build. The scripts do this by combining common
settings, platform settings, board settings, and build-specific settings into a
single Buildroot configuration file, and then executing the build.

The buildroot build process handles the general of a kernel, a user-space image,
and the U-boot bootloader. Once these elements have been built, a
post-processing script is called to combine them into a system image (format
dependent on the platform). The post-processing script automates such tasks as:
* Setting up any necessary files on the root of the SD Card
* Formatting U-Boot and any pre-loader files for the platform
    * For Zynq, this involves creating a BOOT.BIN file from the bitstream, FSBL and U-Boot
    * For Altera SoC, this involves loading the preloader and U-boot into the A2 partition
* Combining all of the files into a platform-specific system image
    * For Zynq, this is a zip of the contents for a FAT32 SD Card
    * For Alter SoC, this is a pre-formatted disk image

## Running a Build

The [build script][common-builds-script] is used to run the overall build. For
convenience, a symbolic link to it is placed at the root of the Buildroot repo.
To run a build, use one of the following commands:
```
build.py -b <board> -p <platform>
```

or

```
build.py -c <path-to-catalog-file>
```

For example, to build ZedBoard, we would use one of the following:
```
build.py -b zed -p zynq
```

or

```
build.py -c board/mathworks/zynq/boards/zed/catalog.xml
```

The build script has a number of other options for configuring the build process
and determining where the resulting files are placed. To explore the options run:

```
build.py --help
```

## Configuring/Customizing a Build


### Board / Platform Description File
The content to build is described in a _catalog_ XML file, described in the
[catalog documentation][1]. Examples of the catalog file can be found in the
platform/board directories:
* [ZedBoard](board/mathworks/zynq/boards/zed/catalog.xml)
* [ZC706](board/mathworks/zynq/boards/zc706/catalog.xml)
* [Arrow/Terasic SoCKit](board/mathworks/socfpga/boards/sockit/catalog.xml)

These files are designed to exist either in-tree or out-of-tree. To extend the
build system to support new applications or new boards, simply author a new
catalog file and pass it to the build script.

### Buildroot Configuration

The build system uses a prioritized system for combining settings files,
allowing more-specific settings files to override settings from more-general
settings files. The hierarchy, from least-specific to most-specific is:
* [MathWorks common][mw-common-defconfig]
* Platform: ([Zynq][zynq-platform-defconfig]) | ([Altera SoC][socfpga-platform-defconfig])
* Toolchain: ([Xilinx][xilinx-toolchain-defconfig]) | ([Linaro][linaro-toolchain-defconfig]) | ([Altera EDS][eds-toolchain-defconfig])
* OS: ([Linux][linux-os-defconfig]) | ([Xenomai][xenomai-os-defconfig])
* Board: The _defconfig_ file located in the board directory
    * E.g. [ZedBoard][zedboard-board-defconfig]
    * Note that the board directory can be overridden by the catalog file
* Catalog: An additional configuration file can be specified in the catalog
* Local: A _.localconfig_ file located in the directory the script is called from
    * This file is recommended for setting transient parameters, such as *OVERRIDE_SRCDIR* or *BR2_JLEVEL*





[1]:board/mathworks/doc/catalog.md
[mw-common-defconfig]:board/mathworks/common/defconfig/common.defconfig
[zynq-platform-defconfig]:board/mathworks/zynq/defconfig/common.defconfig
[xilinx-toolchain-defconfig]:board/mathworks/zynq/defconfig/xilinx.defconfig
[linaro-toolchain-defconfig]:board/mathworks/zynq/defconfig/linaro.defconfig
[zedboard-board-defconfig]:board/mathworks/zynq/boards/zed/defconfig
[linux-os-defconfig]:board/mathworks/zynq/defconfig/linux.defconfig
[xenomai-os-defconfig]:board/mathworks/zynq/defconfig/xenomai.defconfig
[eds-toolchain-defconfig]:board/mathworks/socfpga/defconfig/linaro.defconfig
[socfpga-platform-defconfig]:board/mathworks/socfpga/defconfig/common.defconfig
[common-builds-script]:board/mathworks/common/scripts/build.py
