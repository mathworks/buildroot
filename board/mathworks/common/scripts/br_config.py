#!/usr/bin/env python
import sys, os, shutil, subprocess

import helper_func
from helper_func import *
########################################
# Constants
########################################
_DYNCONFIG = "dynconfig"
_DYNCONFIG_FILE = "%s/configs/%s_defconfig" % (BR_ROOT, _DYNCONFIG)

########################################
# Helper Functions
########################################

##################
# Setup config values based on command line
###################
def _get_config_vals(args, catalog,cfgDataList):
    ## Setup the post script args
    argStr = 'BR2_ROOTFS_POST_SCRIPT_ARGS="'
    
    # point to the catalog file
    argStr += "-c %s" % args['catalogFile']

    # configure the image generation
    argStr += " -i %s" % ' '.join(args['imageList'])
    if args['joinImages']:
        argStr += " -j"
    
    argStr += ' -o %s"\n' % args['imageDest']

    cfgDataList.append(argStr)

    ## Setup the DL directory
    argStr = 'BR2_DL_DIR="%s"\n' % args['dlDir']
    cfgDataList.append(argStr)

########################################
# Public Functions
########################################

##################
# Create the buildroot defconfig
###################
def gen_target(args, catalog):
    import br_platform
        
    CONFIG_DIR = "%s/defconfig" % catalog['platformDir']
    configList = []
    # add configs to the list from lowest to highest priority
    configList.append("%s/defconfig/common.defconfig" % COMMON_DIR) # Company config
    configList.append("%s/common.defconfig" % CONFIG_DIR) # Platform config
    configList.append("%s/%s.defconfig" % (CONFIG_DIR, args['toolchain'])) # Toolchain config
    configList.append("%s/%s.defconfig" % (CONFIG_DIR, args['rtos'])) # OS config
    configList.append("%s/defconfig" % catalog['defaultInfo']['boardInfo']['dir']) # Board config

    # Use the specified config if it exists
    if not catalog['defaultInfo']['br2_config'] is None:
        configList.append(catalog['defaultInfo']['br2_config'])

    # Use the .localconfig if it exists
    lConfig = "%s/.localconfig" % os.getcwd()
    if os.path.isfile(lConfig):
        configList.append(lConfig)

    # Concatentate the config files    
    cfgDataList = []
    for cfg in configList:
        if not os.path.isfile(cfg):
            raise RuntimeError("Defconfig file '%s' does not exist" % cfg)
        with open(cfg) as f:
            cfgData = f.read()
        cfgDataList.append(cfgData)
        cfgDataList.append("\n")
    
    # Populate any board-specific catalog content
    br_platform.platform_gen_target(args, catalog, cfgDataList)

    # Populate the postimage args    
    _get_config_vals(args, catalog,cfgDataList)

    cfgData = ''.join(cfgDataList)
    

    # Generate the BR defconfig
    with open(_DYNCONFIG_FILE, 'w') as f:
        f.write(cfgData)

    # Cleanup as required
    
    if args['cleanBuild']:
        rm(args['outputDir'])
    if args['cleanDL']:
        rm(args['dlDir'])

    # Build the target directory
    if not os.path.isdir(args['outputDir']):
        os.makedirs(args['outputDir'])
    # Call the makefile with the defconfig
    argStr = "make O=%s -C %s %s_defconfig" % (args['outputDir'], BR_ROOT, _DYNCONFIG)
    subprocess.call( argStr.split(), cwd=args['outputDir'])

##################
# Remove the buildroot defconfig
###################
def clean_defconfig():
    rm(_DYNCONFIG_FILE)

