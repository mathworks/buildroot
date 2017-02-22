#!/usr/bin/env python
import sys, os, shutil, subprocess

import helper_func
from helper_func import *
########################################
# Constants
########################################
_DYNCONFIG = "dynconfig"
_CONFIG_DIR = "%s/configs" % BR_ROOT
_DYNCONFIG_FILE = "%s/%s_defconfig" % (_CONFIG_DIR, _DYNCONFIG)
_DYNCONFIG_INC_FILE = "%s/%s_defconfig.h" % (_CONFIG_DIR, _DYNCONFIG)
########################################
# Helper Functions
########################################

    
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

    br_set_var('BR2_ROOTFS_POST_SCRIPT_ARGS', argStr)

    ## Setup the DL directory
    br_set_var('BR2_DL_DIR', args['dlDir'])

    if args['enableCCache']:
        br_set_var('BR2_CCACHE', 'y')
        br_set_var('BR2_CCACHE_USE_BASEDIR', 'y')
        br_set_var('BR2_CCACHE_INITIAL_SETUP', '')

##################
# Setup config values based on command line args (if any)
###################
def _get_cmdline_config_args(args, catalog):
    ## Setup the post script args  


    for cfg in args['brconfig']:
        cfg_args = cfg.split('=')
        varName = cfg_args[0].strip('"')
        varValue = cfg_args[1].strip('"')
        br_set_var(varName, varValue)

##################
# Setup config values based on the catalog
###################
def _get_catalog_config(args, catalog):
    global BRCONFIG_cfgDataList
    import br_platform

    # Load the kernel config, if specified
    if not catalog['defaultInfo']['kernel_config'] is None:
        br_set_var('BR2_LINUX_KERNEL_USE_DEFCONFIG', None)
        br_set_var('BR2_LINUX_KERNEL_USE_CUSTOM_CONFIG', 'y')
        br_set_var('BR2_LINUX_KERNEL_DEFCONFIG', None)
        br_set_var('BR2_LINUX_KERNEL_CUSTOM_CONFIG_FILE', catalog['defaultInfo']['kernel_config'])

    # Populate any board-specific catalog content
    br_platform.platform_gen_target(args, catalog)

##################
# Generate the config file
###################
def _gen_defconfig(args, catalog):
    global BRCONFIG_cfgDataList
    global BRCONFIG_cfgIncludeDirs

    CONFIG_DIR = "%s/defconfig" % catalog['platformInfo']['platformDir']
    # add configs to the list from lowest to highest priority
    br_add_include("%s/defconfig/common.defconfig" % COMMON_DIR) # Company config
    br_add_include("%s/common.defconfig" % CONFIG_DIR) # Platform config
    br_add_include("%s/%s.defconfig" % (CONFIG_DIR, args['toolchain'])) # Toolchain config
    br_add_include("%s/%s.defconfig" % (CONFIG_DIR, args['rtos'])) # OS config
    br_add_include("%s/defconfig" % catalog['defaultInfo']['boardInfo']['dir']) # Board config

    # Add the mw directory
    br_add_includeDir(MW_DIR)       

    # Load any config data from the catalog
    _get_catalog_config(args, catalog)

    # Load any config data from the cmdline    
    _get_cmdline_config(args, catalog)
    
    # Use the specified config if it exists
    if not catalog['defaultInfo']['br2_config'] is None:
        br_add_include(catalog['defaultInfo']['br2_config'])

    # Use the .localconfig if it exists
    lConfig = "%s/.localconfig" % os.getcwd()
    if os.path.isfile(lConfig):
        br_add_include(lConfig)

    # Load any config data from the cmdline args   
    _get_cmdline_config_args(args, catalog)

    # Generate the BR defconfig
    with open(_DYNCONFIG_INC_FILE, 'w') as f:
        cfgData = ''.join(BRCONFIG_cfgDataList)
        f.write(cfgData)

    # use CPP to expand the includes
    postArgs = "-DBUILD_MODE_%s" % args['buildMode']
    cpp_expand(_DYNCONFIG_INC_FILE, _DYNCONFIG_FILE, BRCONFIG_cfgIncludeDirs, extraPostFlags=postArgs)
    rm(_DYNCONFIG_INC_FILE)

########################################
# Public Functions
########################################

##################
# Create the buildroot config and run make
###################
def gen_target(args, catalog):

    # Generate the config file
    _gen_defconfig(args, catalog)
    
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

