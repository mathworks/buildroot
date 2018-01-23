#!/usr/bin/env python
# syntax: <gen_dtb.py> <platform> <board_name> image 
import os, sys, imp
import helper_func
from helper_func import *

def generate_dtbs(catalog, image):
    platform = catalog['platformInfo']['platformName']
    boardName = catalog['boardName']
    LINUX_DIR = get_src_dir("linux")

    # Tool paths
    DTC = ENV['HOST_DIR'] +"/usr/bin/dtc"
    LINUX_DTS = [LINUX_DIR + "/" + dtsi for dtsi in catalog['platformInfo']['kernelDTSDir']]
    DTS_FILE = "devicetree.dts"

    # Determine the toolchain name for CPP
    TC_PREFIX = get_cfg_var("BR2_TOOLCHAIN_EXTERNAL_PREFIX")
    # drop the $(ARCH) string
    TC_PREFIX = re.sub("\$\(ARCH\)", "arm", TC_PREFIX)
    
    PLATFORM_DIR = "%s/%s" % (os.path.dirname(COMMON_DIR), platform)
    BOARD_DIR = "%s/boards/%s" % (PLATFORM_DIR,boardName)


    INCLUDE_DIRS = ["%s/dts" % (COMMON_DIR),
                "%s/dts" % (PLATFORM_DIR),
                "%s/dts" % (BOARD_DIR)]
    INCLUDE_DIRS.extend(image['dtsIncDirs'])
    INCLUDE_DIRS.extend(LINUX_DTS)

    for app in image['appList']:
        print_msg("Generating %s dtb" % (app['name']))
        app_dts_path = app['dts']
        dts_dir = os.path.dirname(app_dts_path)
        app_dts = os.path.basename(app_dts_path)

        dtb_file = "devicetree_%s.dtb" %(app['name'])

        include_dirs = list(INCLUDE_DIRS)
        include_dirs.append(dts_dir)
        
        # Expand the DTS file
        tmpFile = "%s/%s.tmp.dts" % (ENV['SD_DIR'], app['name'])
        cpp_expand(app_dts_path, tmpFile, include_dirs, extraPostFlags="-D__DTS__")

        # Call DTC
        args = [DTC]
        for inc in include_dirs:
            args.extend(["-i", inc])
        strArgs = "-@ -I dts -O dtb -o %s %s" % (dtb_file, tmpFile)
        args.extend(strArgs.split())
        subproc(args, cwd=ENV['SD_DIR'])

        # Cleanup
        os.remove(tmpFile)


########################################
# Module Globals
########################################

