#!/usr/bin/env python
import sys, os, shutil, glob, imp, subprocess, time
import helper_func
from helper_func import *

supported = dict()
# List of valid toolchains
supported['toolchain'] = ["xilinx", "linaro"]
# List of valid OSes
supported['rtos'] = ["linux", "xenomai"]

def platform_checkconfig(args):
    if (    args['rtos'] == "xenomai" and
            args['toolchain'] == "xilinx"):
        raise RuntimeError("Cannot use Xilinx toolchain with Xenomai OS")

def platform_gen_target(args, catalog, cfgDataList):
    # Do nothing for recovery mode
    if catalog['buildMode'] == BuildMode.RECOVERY:
        return

    if os.path.isdir(catalog['defaultInfo']['fsbl']):
        # Build the SPL using the handoff files
        cfgDataList.append("BR2_PACKAGE_UBOOT_XILINX=y\n")
        cfgDataList.append("BR2_TARGET_UBOOT_FORMAT_IMG=y\n")
        cfgDataList.append("BR2_TARGET_UBOOT_SPL=y\n")
        cfgDataList.append('BR2_TARGET_UBOOT_SPL_NAME="spl/boot.bin"\n')
        handoffStr = 'BR2_PACKAGE_UBOOT_XILINX_VIVADO_HANDOFF_DIR='
        handoffStr += '"%s"\n' % catalog['defaultInfo']['fsbl']
        cfgDataList.append(handoffStr)
    else:
        # Build the boot.bin from FSBL and UBoot
        cfgDataList.append("BR2_TARGET_UBOOT_FORMAT_ELF=y\n")
    
    return



