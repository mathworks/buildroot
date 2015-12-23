#!/usr/bin/env python
import sys, os, shutil, subprocess

import helper_func
from helper_func import *
########################################
# Constants
########################################
_DYNCONFIG = "dynconfig"
_DYNCONFIG_FILE = "%s/configs/%s_defconfig" % (BR_ROOT, _DYNCONFIG)
_cfgDataList = []
########################################
# Helper Functions
########################################
def _br_set_var(var, value, quoted=True):
    global _cfgDataList

    if value is None:
        argStr = "# %s is not set\n" % var
    else:
        if value.lower() == 'y':
            quoted = False

        if quoted:
            argStr = '%s="%s"\n' % (var, value)
        else:
            argStr = '%s=%s\n' % (var, value)

    _cfgDataList.append(argStr)
    
##################
# Setup config values based on command line
###################
def _get_cmdline_config(args, catalog):
    ## Setup the post script args   

    # point to the catalog file
    argStr = "-c %s" % args['catalogFile']

    # configure the image generation
    argStr += " -i %s" % ' '.join(args['imageList'])

    # configure the output directory    
    argStr += ' -o %s' % args['imageDest']

    _br_set_var('BR2_ROOTFS_POST_SCRIPT_ARGS', argStr)

    ## Setup the DL directory
    _br_set_var('BR2_DL_DIR', args['dlDir'])

##################
# Setup config values based on the catalog
###################
def _get_catalog_config(args, catalog):
    global _cfgDataList
    import br_platform

    # Load the kernel config, if specified
    if not catalog['defaultInfo']['kernel_config'] is None:
        _br_set_var('BR2_LINUX_KERNEL_USE_DEFCONFIG', None)
        _br_set_var('BR2_LINUX_KERNEL_USE_CUSTOM_CONFIG', 'y')
        _br_set_var('BR2_LINUX_KERNEL_DEFCONFIG', None)
        _br_set_var('BR2_LINUX_KERNEL_CUSTOM_CONFIG_FILE', catalog['defaultInfo']['kernel_config'])

    # Populate any board-specific catalog content
    br_platform.platform_gen_target(args, catalog, _cfgDataList)

########################################
# Public Functions
########################################

##################
# Create the buildroot defconfig
###################
def gen_target(args, catalog):
    global _cfgDataList
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
    _cfgDataList = []
    for cfg in configList:
        if not os.path.isfile(cfg):
            raise RuntimeError("Defconfig file '%s' does not exist" % cfg)
        with open(cfg) as f:
            cfgData = f.read()
        _cfgDataList.append(cfgData)
        _cfgDataList.append("\n")
    
    # Load any config data from the catalog
    _get_catalog_config(args, catalog)

    # Load any config data from the cmdline    
    _get_cmdline_config(args, catalog)
    
    # Generate the BR defconfig
    with open(_DYNCONFIG_FILE, 'w') as f:
        cfgData = ''.join(_cfgDataList)
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

