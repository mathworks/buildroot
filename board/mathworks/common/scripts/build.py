#!/usr/bin/env python
import sys, os, shutil, glob, imp, argparse, subprocess, datetime

import parse_catalog
import helper_func
from helper_func import *
import br_config

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

    if args['logFile'] is None:
        logFile = 'build_%s.log' % datetime.datetime.now().strftime("%b_%d_%Y_%H%M%S")
        args['logFile'] = os.path.join(args['outputDir'], logFile)
    else:
        args['logFile'] = os.path.realpath(args['logFile'])
        if not os.path.isdir(os.path.dirname(args['logFile'])):
            os.makedirs(os.path.dirname(args['logFile']))

    if args['imageDest'] is None:
        args['imageDest'] = "%s/images" % args['outputDir']
    else:
        args['imageDest'] = os.path.realpath(args['imageDest'])

    if args['dlDir'] is None:
        args['dlDir'] = os.path.realpath("%s/dl/%s" % (BR_ROOT,args['platformName']))
    elif not os.path.isabs(args['dlDir']):
        args['dlDir'] = os.path.realpath("%s/%s" % (os.getcwd(), args['dlDir']))
        
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
    subproc_log(argStr, logfile=args['logFile'], cwd=args['outputDir'], verbose=(not args['quietBuild']))

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
buildTypeGrp.add_argument('-l', '--logfile', dest='logFile', metavar='LOG_FILE', type=str,
                        help='File to log the build output(default: build_DD_MM_YYYY.log)')
buildTypeGrp.add_argument('-q', '--quiet', dest='quietBuild', action='store_true',
                        help='Do not print build output to stdout')

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
catalog = parse_catalog.read_catalog(args['catalogFile'], args['imageList'])
# catalog may have provided some info
args['platformName'] = catalog['platformName'].lower()
args['boardName'] = catalog['boardName'].lower()

# load the platform functions
PLATFORM_MODULE = catalog['platformDir'] + "/scripts/build_common.py"
m = imp.load_source('br_platform', PLATFORM_MODULE)
import br_platform

get_build_config(args)

checkconfig(args)

br_config.gen_target(args, catalog)

br_config.clean_defconfig()

build_target(args, catalog)


