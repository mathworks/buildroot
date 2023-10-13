#!/usr/bin/env python
# syntax <build_board.py> <output_dir> <platform> <board_name> [<image name>]
# executed out of main buildroot source directory
# available environment variables
#    BR2_CONFIG: path to .config file
#    HOST_DIR, STAGING_DIR, TARGET_DIR
#    BINARIES_DIR: images dir
#    BASE_DIR: base output directory
import sys, os, shutil, glob, importlib, argparse, distutils.dir_util
import csv, time
from importlib.machinery import SourceFileLoader
import parse_catalog
import gen_dtb
import helper_func
from helper_func import *

########################################
# Helper Functions
########################################
def _gen_legalinfo(catalog, outputDir):
    
    legalDir = "%s/legal-info" % ENV['BASE_DIR']
    manifest = "%s/manifest.csv" % legalDir
    licenseHeader = "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n"
    licenseFmt = "\t%s: %s file\n"

    buildDate = time.strftime("%F")
    licDstFile = "%s/licenses_%s_%s.txt" % (outputDir, catalog['boardName'], buildDate)

    if not os.path.exists(manifest):
        return

    print_msg("Creating combined license file %s " % (licDstFile),
        level=3, fg=2, bold=True)

    with open(licDstFile,'w') as licDst:
        with open(manifest,'r',encoding='latin-1') as csvFile:
            manifestReader = csv.DictReader(csvFile)
            for licInfo in manifestReader:
                licSrcs = licInfo['LICENSE FILES'].split()
                pkgNameVer = "%s-%s" % (licInfo['PACKAGE'], licInfo['VERSION'])
                for lic in licSrcs:
                    licDst.write(licenseHeader)
                    licDst.write(licenseFmt %
                        (pkgNameVer, lic))
                    licDst.write(licenseHeader)
                    licSrcFile = "%s/licenses/%s/%s" % (legalDir, pkgNameVer, lic)
                    with open(licSrcFile,'r',encoding='latin-1') as licSrc:
                        lines = licSrc.readlines()
                        licDst.writelines(lines)
                    licDst.write("\n\n")

def _gen_sysroot(catalog, outputDir):
    buildDate = time.strftime("%F")
    sysroot_file = "%s/sysroot_%s_%s.tar.gz" % (outputDir, catalog['boardName'], buildDate)
    sysroot_rsync = "%s/sysroot" % (ENV['BASE_DIR'])

    # Filter out the <tuple>/<tuple>/... directories symlinked within the root
    arch=get_cfg_var('BR2_ARCH')
    prefix=get_cfg_var('BR2_TOOLCHAIN_EXTERNAL_PREFIX')
    prefix=prefix.replace("$(ARCH)", arch)
    exclude="--exclude \"/**/%s/%s/\"" % (prefix,prefix)

    print_msg("Generating sysroot: %s " % sysroot_file, level=3, fg=2, bold=True)
    #rsync the files to remove symlinks
    rm(sysroot_rsync)
    subproc("rsync -arL %s %s %s" % (exclude, ENV['STAGING_DIR'], sysroot_rsync))

    # tar the sysroot
    sysroot_root = os.path.join(sysroot_rsync, os.path.basename(os.path.abspath(ENV['STAGING_DIR'])))
    fileList = " ".join(build_relative_file_list(sysroot_root, sysroot_root))
    subproc("tar -czf %s %s" % (sysroot_file,fileList), cwd=sysroot_root)

    rm(sysroot_rsync)

def _gen_sdcard(image, catalog, outputDir):
    
    print("")
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
    print_msg("Sourcing SD card contents from %s" %(sdSrc))
    distutils.dir_util.copy_tree(sdSrc, ENV['SD_DIR'])
    if image['sdcardDir'] != sdSrc:
        print_msg("Adding SD card contents from %s" % (image['sdcardDir']))
        distutils.dir_util.copy_tree(image['sdcardDir'], ENV['SD_DIR'])

    ##############
    # Copy the recovery image, if it exists
    ##############
    recovery_file = get_cfg_var('BR2_PACKAGE_RECOVERY_IMAGE_FILE')
    if recovery_file != "":
        recovery_image = "%s/%s" % (ENV['BINARIES_DIR'] ,recovery_file)
        if os.path.exists(recovery_image):
            shutil.copyfile(recovery_image, "%s/%s" % (ENV['SD_DIR'], recovery_file))
        else:
            raise IOError("Cannot find specified recovery file: %s" % recovery_file)

    ##############
    # Add the verinfo script
    ##############
    gen_verinfo_file(ENV['SD_DIR'] + "/BUILDINFO")


    defaultApp = image['defaultApp']
    ##############
    # Generate DTBs
    ##############
    gen_dtb.generate_dtbs(catalog, image)
    # Copy the default DTB to devicetree.dtb
    print_msg("Setting %s as default dtb" %(defaultApp['name']))
    br_platform.set_default_dtb(defaultApp)

    ##############
    # Setup bitstreams
    ##############
    if not defaultApp['bit'] is None:
        print_msg("Setting %s as default bitstream" %(defaultApp['name']))
        br_platform.set_default_bitsream(defaultApp)

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
parser.add_argument('-e', '--environ', dest='setEnvrion', action="store_true",
                        help='Set Buildroot Environment variables based on BR2_OUTPUT_DIR')
parser.add_argument('-o', '--output', dest='outputDir',
                        help='Output directory for image files(default: BR2_OUTPUT_DIR/images)')
parser.add_argument('--sysroot', dest='sysrootOnly', action="store_true",
                        help='Generate the sysroot tarball instead of an image file')



# Move the output dir to the end of the list
args = vars(parser.parse_args())

init_logging(console=0)

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
# Create the output dir if needed
mkdir(outputDir)

# read in the tree
catalog = parse_catalog.read_catalog(CATALOG_FILE, args['imageList'])
# catalog may have provided some info
PLATFORM_NAME = catalog['platformInfo']['platformName']
BOARD_NAME = catalog['boardName']
PLATFORM_DIR = os.path.dirname(COMMON_DIR) + "/" + PLATFORM_NAME

# load the platform functions
PLATFORM_MODULE = catalog['platformInfo']['platformDir'] + "/scripts/platform_support.py"
br_platform = SourceFileLoader('br_platform',PLATFORM_MODULE).load_module()

br_platform.platform_update_catalog(catalog)

if catalog['buildMode'] == BuildMode.NORMAL:
    _gen_legalinfo(catalog, outputDir)
    if args['sysrootOnly']:
        _gen_sysroot(catalog, outputDir)
    else:
        for image in catalog['imageList']:
            _gen_sdcard(image, catalog, outputDir)

