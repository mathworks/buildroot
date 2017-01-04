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
_cfgDataList = []
_cfgIncludeDirs = []
########################################
# Helper Functions
########################################
def _br_add_includeDir(incdir):
    global _cfgIncludeDirs    

    _cfgIncludeDirs.append(incdir)

def _br_add_include(incfile, addPath=True):
    global _cfgDataList

    _cfgDataList.append('#include "%s"\n' % incfile)
    if addPath:
        _br_add_includeDir(os.path.dirname(incfile))

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

    if args['enableCCache']:
        _br_set_var('BR2_CCACHE', 'y')
        _br_set_var('BR2_CCACHE_USE_BASEDIR', 'y')
        _br_set_var('BR2_CCACHE_INITIAL_SETUP', '')

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

##################
# Generate the config file
###################
def _gen_defconfig(args, catalog):
    global _cfgDataList
    global _cfgIncludeDirs

    CONFIG_DIR = "%s/defconfig" % catalog['platformDir']
    # add configs to the list from lowest to highest priority
    _br_add_include("%s/defconfig/common.defconfig" % COMMON_DIR) # Company config
    _br_add_include("%s/common.defconfig" % CONFIG_DIR) # Platform config
    _br_add_include("%s/%s.defconfig" % (CONFIG_DIR, args['toolchain'])) # Toolchain config
    _br_add_include("%s/%s.defconfig" % (CONFIG_DIR, args['rtos'])) # OS config
    _br_add_include("%s/defconfig" % catalog['defaultInfo']['boardInfo']['dir']) # Board config

    # Add the mw directory
    _br_add_includeDir(MW_DIR)       

    # Load any config data from the catalog
    _get_catalog_config(args, catalog)

    # Load any config data from the cmdline    
    _get_cmdline_config(args, catalog)
    
    # Use the specified config if it exists
    if not catalog['defaultInfo']['br2_config'] is None:
        _br_add_include(catalog['defaultInfo']['br2_config'])

    # Use the .localconfig if it exists
    lConfig = "%s/.localconfig" % os.getcwd()
    if os.path.isfile(lConfig):
        _br_add_include(lConfig)


    # Generate the BR defconfig
    with open(_DYNCONFIG_INC_FILE, 'w') as f:
        cfgData = ''.join(_cfgDataList)
        f.write(cfgData)

    # use CPP to expand the includes
    postArgs = "-DBUILD_MODE_%s" % args['buildMode']
    cpp_expand(_DYNCONFIG_INC_FILE, _DYNCONFIG_FILE, _cfgIncludeDirs, extraPostFlags=postArgs)
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

