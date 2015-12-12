#!/usr/bin/env python
import sys, os, shutil, glob, imp, argparse, subprocess

import parse_catalog
import helper_func
from helper_func import *
########################################
# Constants
########################################
DYNCONFIG = "dynconfig"
DYNCONFIG_FILE = "%s/configs/%s_defconfig" % (BR_ROOT, DYNCONFIG)

########################################
# Helper Functions
########################################

##################
# Check the config for compatibility errors
###################
def checkconfig(args):
    # common checking
    if not args['toolchain'] in br_platform.supported['toolchain']:
        errStr = "Toolchain '%s' is not supported for platform '%s'" % (args['toolchain'], args['platformName'])
        errStr = errStr + "\n\tSupported toolchains: %s" % br_platform.supported['toolchain']
        raise RuntimeError(errStr)
        
    if not args['rtos'] in br_platform.supported['rtos']:
        errStr = "RTOS '%s' is not supported for platform '%s'" % (args['toolchain'], args['platformName'])
        errStr = errStr + "\n\tSupported RTOS: %s" % br_platform.supported['rtos']
        raise RuntimeError(errStr)

    # platform specific checking
    br_platform.platform_checkconfig

##################
# Resolve the build config options
###################  
def get_build_config(args):
    # Use the platform defaults to populate the values
    if args['toolchain'] is None:
        args['toolchain'] = br_platform.supported['toolchain'][0]

    if args['rtos'] is None:
        args['rtos'] = br_platform.supported['rtos'][0]

    if args['outputDir'] is None:
        pathStr = "%s/output/%s_%s_%s" % (os.getcwd(), args['boardName'], args['rtos'], args['toolchain'])
        args['outputDir'] = os.path.realpath(pathStr)
    else:
        args['outputDir'] = os.path.realpath(args['outputDir'])

    if args['imageDest'] is None:
        args['imageDest'] = "%s/images" % args['outputDir']
    else:
        args['imageDest'] = os.path.realpath(args['imageDest'])

    if args['dlDir'] is None:
        args['dlDir'] = os.path.realpath("%s/dl/%s" % (BR_ROOT,args['platformName']))
    elif not os.path.isabs(args['dlDir']):
        args['dlDir'] = os.path.realpath("%s/%s" % (os.getcwd(), args['dlDir']))
        
        

##################
# Setup config values based on command line
###################
def get_config_vals(args, catalog,cfgDataList):
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

##################
# Create the buildroot defconfig
###################
def gen_target(args, catalog):
    CONFIG_DIR = "%s/defconfig" % PLATFORM_DIR
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
    get_config_vals(args, catalog,cfgDataList)

    cfgData = ''.join(cfgDataList)
    

    # Generate the BR defconfig
    with open(DYNCONFIG_FILE, 'w') as f:
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
    argStr = "make O=%s -C %s %s_defconfig" % (args['outputDir'], BR_ROOT, DYNCONFIG)
    subprocess.call( argStr.split(), cwd=args['outputDir'])

##################
# Remove the buildroot defconfig
###################
def clean_defconfig():
    rm(DYNCONFIG_FILE)

##################
# Run the build
###################
def build_target(args, catalog):
    # Clean if required
    if not args['updateBuild']:
        argStr = "make clean"   
        subprocess.call( argStr.split(), cwd=args['outputDir'])

    # Call the makefile
    argStr = "make %s" % args['makeTarget']
    subprocess.call( argStr.split(), cwd=args['outputDir'])

########################################
# Main
########################################

parser = argparse.ArgumentParser(description='Generate Buildroot images')

platformGrp = parser.add_argument_group('Platform Specification')
platformGrp.add_argument('-c', '--catalog', dest='catalogFile', metavar='CATALOG_FILE', type=str,
                        help='The catalog XML file')
platformGrp.add_argument('-p', '--platform', dest='platformName', metavar='PLATFORM', type=str, 
                        help='The platform (e.g. zynq or alterasoc)')
platformGrp.add_argument('-b', '--board', dest='boardName', metavar='BOARD_NAME', type=str,
                        help='The board name (e.g. zed)')

tgtGrp = parser.add_argument_group('Target Specification')
tgtGrp.add_argument('-t', '--toolchain', dest='toolchain', metavar='TOOLCHAIN', type=str,
                        help='The toolchain to build with (default based on platform)')
tgtGrp.add_argument('-r', '--rtos', dest='rtos', metavar='RTOS', type=str,
                        default="linux", help='The RTOS to build (default: linux)')

imagesGrp=parser.add_argument_group('Image Control')
imagesGrp.add_argument('-i', '--images', dest='imageList', metavar='IMAGE', type=str,
                        nargs="*", default=["all"],help='The Images to build (default: all)')
imagesGrp.add_argument('-j', '--join', dest='joinImages', action="store_true",
                        help='Combine specified images')
imagesGrp.add_argument('-d', '--dest', dest='imageDest', metavar='IMAGE_DEST', type=str,
                        help='The destination directory for the images (default: OUTPUT_DIR/images)')

buildTypeGrp=parser.add_argument_group('Build Control')

buildTypeGrp.add_argument('-o', '--output', dest='outputDir', metavar='OUTPUT_DIR', type=str,
                        help='Buildroot output directory (default: <pwd>/output/<target name>)')
buildTypeGrp.add_argument('--cleandl', dest='cleanDL', action='store_true',
                        help='Clean the download directory before building (default: false)')
buildTypeGrp.add_argument('--target', dest='makeTarget', metavar='MAKE_TARGET', type=str,
                        default="all", help='Make target (default: all)')
buildTypeGrp.add_argument('--dl', dest='dlDir', metavar='ML_DIR', type=str,
                        help='Buildroot download directory (default: dl/<platform>)')

buildType=buildTypeGrp.add_mutually_exclusive_group(required=False)
buildType.add_argument('-u', '--update', dest='updateBuild', action='store_true',
                        help='''Don't run 'make clean' before building(default: false)''')
buildType.add_argument('--clean', dest='cleanBuild', action='store_true',
                        help='Delete OUTPUT_DIR before building (default: false)')


# Move the output dir to the end of the list
args = vars(parser.parse_args())

if (args['catalogFile'] is None and 
    ((args['platformName'] is None) or (args['boardName'] is None))):
    errStr = "Either a CATALOG_FILE or a BOARD_NAME and PLATFORM are required\n"
    errStr += parser.format_usage()
    raise SyntaxError(errStr)

#normalize the arguments

if args['catalogFile'] is None:
    args['platformName'] = args['platformName'].lower()
    args['boardName'] = args['boardName'].lower()
    args['catalogFile'] = "%s/%s/boards/%s/catalog.xml" % (MW_DIR, args['platformName'],args['boardName'])

# read in the tree
catalog = parse_catalog.read_catalog(args['catalogFile'], args['imageList'], args['joinImages'])
# catalog may have provided some info
args['platformName'] = catalog['platformName'].lower()
args['boardName'] = catalog['boardName'].lower()
PLATFORM_DIR = os.path.dirname(COMMON_DIR) + "/" + args['platformName']


# load the platform functions
PLATFORM_MODULE = PLATFORM_DIR + "/scripts/build_common.py"
m = imp.load_source('br_platform', PLATFORM_MODULE)
import br_platform

get_build_config(args)

checkconfig(args)

gen_target(args, catalog)

clean_defconfig()

build_target(args, catalog)


