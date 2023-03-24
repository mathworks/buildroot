#!/usr/bin/env python3
import sys, os, shutil, glob, importlib, argparse, datetime

from importlib.machinery import SourceFileLoader
import parse_catalog
import helper_func
from helper_func import *
import br_config

########################################
# Helper Functions
########################################

##################
# Check the config for compatibility errors
###################
def checkconfig(args):
    # common checking
    if not args['toolchain'] in SUPPORTED['toolchain']:
        errStr = "Toolchain '%s' is not supported for platform '%s'" % (args['toolchain'], args['platformName'])
        errStr = errStr + "\n\tSupported toolchains: %s" % SUPPORTED['toolchain']
        raise RuntimeError(errStr)
        
    if not args['rtos'] in SUPPORTED['rtos']:
        errStr = "RTOS '%s' is not supported for platform '%s'" % (args['toolchain'], args['platformName'])
        errStr = errStr + "\n\tSupported RTOS: %s" % SUPPORTED['rtos']
        raise RuntimeError(errStr)

    # platform specific checking
    br_platform.platform_checkconfig

##################
# Resolve the build config options
###################  
def get_build_config(args):
    # Use the platform defaults to populate the values
    if args['toolchain'] is None:
        args['toolchain'] = SUPPORTED['toolchain'][0]

    if args['rtos'] is None:
        args['rtos'] = SUPPORTED['rtos'][0]

    if args['outputDir'] is None:
        if args['buildMode'] == BuildMode.RECOVERY:
            pathStr = "%s/output/%s_%s_%s" % (os.getcwd(), args['platformName'], args['boardName'], args['toolchain'])
        else:
            pathStr = "%s/output/%s_%s_%s" % (os.getcwd(), args['boardName'], args['rtos'], args['toolchain'])
        args['outputDir'] = os.path.realpath(pathStr)
    else:
        args['outputDir'] = os.path.realpath(args['outputDir'])

    if args['logFile'] is None:
        logFile = 'build.log'
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

    args['catalogFile'] = os.path.realpath(args['catalogFile'])

    # Setup the BR environment variables for future use
    set_br_env(args['outputDir'])
        
##################
# Run the build
###################
def build_target(args, catalog):
    # Clean if required
    if not args['updateBuild']:
        subproc("make clean", cwd=args['outputDir'])

    if (args['cleanCCache'] and args['enableCCache']):
        cacheDir = get_cfg_var('BR2_CCACHE_DIR').replace("$(HOME)", os.environ['HOME'])
        rm(cacheDir)

    # Call the makefile
    argStr = "make %s" % args['makeTarget']
    #subproc_log(argStr, logfile=args['logFile'], cwd=args['outputDir'], verbose=(not args['quietBuild']))
    subproc(argStr, cwd=args['outputDir'])

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
                        default="legal-info all", help='Make target (default: legal-info all)')
buildTypeGrp.add_argument('--dl', dest='dlDir', metavar='ML_DIR', type=str,
                        help='Buildroot download directory (default: dl/<platform>)')
buildTypeGrp.add_argument('-l', '--logfile', dest='logFile', metavar='LOG_FILE', type=str,
                        help='File to log the build output(default: build.log)')
buildTypeGrp.add_argument('-q', '--quiet', dest='quietBuild', action='count', default=0,
                        help='Limit output to stdout')
buildTypeGrp.add_argument('--ccache', dest='enableCCache', action='store_true',
                        help='Enable the ccache build mechanism (default: false)')
buildTypeGrp.add_argument('--ccache-clean', dest='cleanCCache', action='store_true',
                        help='Clean the ccache cache (default: false)')
buildTypeGrp.add_argument('--brconfig', dest='brconfig', action='append', default=[],
                        help='Specify buildroot variables in the form <VARNAM>=<VALUE>')
buildTypeGrp.add_argument('--sysroot', dest='sysrootOnly', action="store_true",
                        help='Generate the sysroot tarball instead of an image file')


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
args['platformName'] = catalog['platformInfo']['platformName'].lower()
args['boardName'] = catalog['boardName'].lower()
args['buildMode'] = catalog['buildMode']

# load the platform functions
PLATFORM_MODULE = catalog['platformInfo']['platformDir'] + "/scripts/platform_support.py"
br_platform = SourceFileLoader('br_platform', PLATFORM_MODULE).load_module()
br_platform.platform_update_catalog(catalog)
SUPPORTED = br_platform.platform_supported()

get_build_config(args)

mkdir(args['outputDir'])

checkconfig(args)

init_logging(filename=args['logFile'], console=args['quietBuild'])

br_config.gen_target(args, catalog)

br_config.clean_defconfig(args, catalog)

build_target(args, catalog)


