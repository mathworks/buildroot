#!/usr/bin/env python
# syntax <build_board.py> <output_dir> <platform> <board_name> [<image name>]
# executed out of main buildroot source directory
# available environment variables
#	BR2_CONFIG: path to .config file
#	HOST_DIR, STAGING_DIR, TARGET_DIR
#	BINARIES_DIR: images dir
#	BASE_DIR: base output directory
import sys, os, shutil, glob, imp, argparse

import parse_catalog
import gen_dtb
import helper_func
from helper_func import *

########################################
# Helper Functions
########################################
def _gen_sdcard(image, catalog, outputDir):
    
    print ""
    print_msg("Starting on image %s " % (image['imageName']), level=3, fg=2, bold=True)
    ##############
    # prep the directory
    ##############
    rm(ENV['SD_DIR'])
    os.makedirs(ENV['SD_DIR'])

    ##############
    # Copy Over the sdcard files
    ##############
    sdSrc = catalog['defaultInfo']['sdcardDir']
    print_msg("Sourcing SD card contents from %s" %(catalog['defaultInfo']['sdcardDir']))
    for fil in glob.iglob(catalog['defaultInfo']['sdcardDir'] + "/*"):
        shutil.copy2(fil, ENV['SD_DIR'])
    if image['sdcardDir'] != sdSrc:
        print_msg("Adding SD card contents from %s" % (image['sdcardDir']))
        for fil in glob.iglob(image['sdcardDir'] + "/*"):
            shutil.copy2(fil, ENV['SD_DIR'])

    ##############
    # Add the verinfo script
    ##############
    gen_verinfo_file(ENV['SD_DIR'] + "/BUILDINFO")

    ##############
    # Generate DTBs
    ##############
    defaultApp = image['defaultApp']
    gen_dtb.generate_dtbs(PLATFORM_NAME, BOARD_NAME, image)
    # Copy the default DTB to devicetree.dtb
    print_msg("Setting %s as default dtb" %(defaultApp['name']))
    br_platform.set_default_dtb(defaultApp)

    ##############
    # Create the output dir if needed
    ##############
    if not os.path.isdir(outputDir):
        os.makedirs(outputDir)

    ##############
    # Call the platform-specific function
    ##############
    br_platform.build_sdimage(outputDir, image, catalog)

########################################
# Main
########################################

# syntax <build_board.py> <output_dir> <platform> <board_name> [<image name>]
parser = argparse.ArgumentParser(description='Post Process the Buildroot images')

parser.add_argument('brOutput', metavar='BR2_OUTPUT_DIR', type=str, help='The buildroot output directory')
parser.add_argument('-p', '--platform', dest='platformName', metavar='PLATFORM', type=str, 
                        help='The platform (e.g. zynq or alterasoc)')
parser.add_argument('-b', '--board', dest='boardName', metavar='BOARD_NAME', type=str,
                        help='The board name (e.g. zed)')
parser.add_argument('-c', '--catalog', dest='catalogFile', metavar='CATALOG_FILE', type=str,
                        help='The catalog XML file')
parser.add_argument('-i', '--images', dest='imageList', metavar='IMAGE', type=str,
                        nargs="*", default=["all"],help='The board name (e.g. zed)')
parser.add_argument('-j', '--join', dest='joinImages', action="store_true",
                        help='Combine specified images')
parser.add_argument('-e', '--environ', dest='setEnvrion', action="store_true",
                        help='Set Buildroot Environment variables based on BR2_OUTPUT_DIR')
parser.add_argument('-o', '--output', dest='outputDir',
                        help='Output directory for image files(default: BR2_OUTPUT_DIR/images)')


# Move the output dir to the end of the list
args = vars(parser.parse_args())

if (args['catalogFile'] is None and 
    ((args['platformName'] is None) or (args['boardName'] is None))):
    raise SyntaxError("Either a CATALOG_FILE or a BOARD_NAME and PLATFORM are required")

BR2_OUTPUT_DIR = os.path.realpath(args['brOutput'])
if args['catalogFile'] is None:
    PLATFORM_NAME = args['platformName']
    BOARD_NAME = args['boardName']
    CATALOG_FILE = "%s/%s/boards/%s/catalog.xml" % (MW_DIR, PLATFORM_NAME,BOARD_NAME)
else:
    CATALOG_FILE = args['catalogFile']
    
if args['setEnvrion']:
    set_br_env(BR2_OUTPUT_DIR)

if args['outputDir'] is None:
    args['outputDir'] = ENV['IMAGE_DIR']

outputDir = os.path.realpath(args['outputDir'])

# read in the tree
catalog = parse_catalog.read_catalog(CATALOG_FILE, args['imageList'], args['joinImages'])
# catalog may have provided some info
PLATFORM_NAME = catalog['platformName']
BOARD_NAME = catalog['boardName']
PLATFORM_DIR = os.path.dirname(COMMON_DIR) + "/" + PLATFORM_NAME

# load the platform functions
PLATFORM_MODULE = PLATFORM_DIR + "/scripts/postimage_common.py"
m = imp.load_source('br_platform', PLATFORM_MODULE)
import br_platform

for image in catalog['imageList']:
    _gen_sdcard(image, catalog, outputDir)




