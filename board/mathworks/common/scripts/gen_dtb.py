#!/usr/bin/env python
# syntax: <gen_dtb.py> <platform> <board_name> image 
import os, sys, imp, subprocess
import helper_func
from helper_func import *


def generate_dtbs(platform, boardName, image):
    LINUX_DIR = get_src_dir("linux")

    # Tool paths
    DTC = ENV['HOST_DIR'] +"/usr/bin/dtc"
    LINUX_DTS = LINUX_DIR + "/arch/arm/boot/dts"
    DTS_FILE = "devicetree.dts"

    # Determine the toolchain name for CPP
    TC_PREFIX = get_cfg_var("BR2_TOOLCHAIN_EXTERNAL_PREFIX")
    # drop the $(ARCH) string
    TC_PREFIX = re.sub("\$\(ARCH\)", "arm", TC_PREFIX)
    
    PLATFORM_DIR = "%s/%s" % (os.path.dirname(COMMON_DIR), platform)
    BOARD_DIR = "%s/boards/%s" % (PLATFORM_DIR,boardName)


    INCLUDE_DIRS = ["%s/dts" % (COMMON_DIR),
                "%s/dts" % (PLATFORM_DIR),
                "%s/dts" % (BOARD_DIR),
                "%s" % (LINUX_DTS),
                "%s/include" % (LINUX_DTS)]
    INCLUDE_DIRS.extend(image['dtsIncDirs'])

    DEPENDENCY_FILE = "dependency.pre.tmp"
    DTC_CPP_PRE_FLAGS = "-Wp,-MD,%s -nostdinc" % (DEPENDENCY_FILE)
    DTC_CPP_POST_FLAGS = "-undef -D__DTS__ -x assembler-with-cpp"

    for app in image['appList']:
        print_msg("Generating %s dtb" % (app['name']))
        app_dts_path = app['dts']
        dts_dir = os.path.dirname(app_dts_path)
        app_dts = os.path.basename(app_dts_path)

        dtb_file = "devicetree_%s.dtb" %(app['name'])

        include_dirs = list(INCLUDE_DIRS)
        include_dirs.append(dts_dir)
    
        # build the cpp command
        dtc_cpp_flags = list()
        dtc_cpp_flags.extend(DTC_CPP_PRE_FLAGS.split())
        for inc in include_dirs:
            inc_str = "-I" + inc
            dtc_cpp_flags.append(inc_str)
        dtc_cpp_flags.extend(DTC_CPP_POST_FLAGS.split())
        
        # Run the DTS through CPP     
        tmpFile = app['name'] + ".tmp.dts"    
        args = ["%s/usr/bin/%s-cpp" % (ENV['HOST_DIR'], TC_PREFIX)]
        args.extend(dtc_cpp_flags)
        args.extend(["-o", tmpFile, app_dts_path])
        subprocess.call(args, cwd=ENV['SD_DIR'])

        # Call DTC
        args = [DTC]
        for inc in include_dirs:
            args.extend(["-i", inc])
        strArgs = "-I dts -O dtb -o %s %s" % (dtb_file, tmpFile)
        args.extend(strArgs.split())
        subprocess.call(args, cwd=ENV['SD_DIR'])

        # Cleanup
        os.remove("%s/%s" % (ENV['SD_DIR'],DEPENDENCY_FILE))
        os.remove("%s/%s" % (ENV['SD_DIR'],tmpFile))


########################################
# Module Globals
########################################

