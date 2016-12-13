#!/usr/bin/env python
import sys, os, shutil, glob, imp, subprocess, time
import helper_func
from helper_func import *

supported = dict()
# List of valid toolchains
supported['toolchain'] = ["linaro"]
# List of valid OSes
supported['rtos'] = ["linux"]

def platform_checkconfig(args):
    return

def platform_gen_target(args, catalog, cfgDataList):
    # Do nothing for recovery mode
    if catalog['buildMode'] == BuildMode.RECOVERY:
        return

    handoffStr = 'BR2_PACKAGE_UBOOT_ALTERA_QUARTUS_HANDOFF_DIR='
    handoffStr += '"%s"\n' % catalog['defaultInfo']['fsbl']

    cfgDataList.append(handoffStr)

    bspBuildStr = "BR2_PACKAGE_UBOOT_ALTERA_GENERATE_BSP="

    # Validate the handoff directory
    handoffDir = "%s/handoff" % catalog['defaultInfo']['fsbl']
    if os.path.isdir(handoffDir):
        if not os.path.isdir("%s/generated" % catalog['defaultInfo']['fsbl']):
            errStr = "The handoff directory (%s) does not contain both handoff and generated folders\n" % catalog['defaultInfo']['fsbl']
            errStr += "When supplying both the handoff files and the BSP, they must be placed in 'handoff'"
            errStr += " and 'generated' subfolders respectively\n"
            raise RuntimeError(errStr)
        bspBuildStr += "n\n"
    else:
        handoffDir = catalog['defaultInfo']['fsbl']
        bspBuildStr += "y\n"
    if not os.path.isfile("%s/hps.xml" % handoffDir):
        errStr = "The handoff directory (%s) does not contain the hps.xml file\n" % handoffDir
        errStr += "Please ensure this folder contains the contents of the hps_isw_software directory\n"
        raise RuntimeError(errStr)

    cfgDataList.append(bspBuildStr)

    return


