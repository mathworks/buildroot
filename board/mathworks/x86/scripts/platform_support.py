#!/usr/bin/env python
import sys, os, shutil, glob, imp, time
import helper_func
from helper_func import *
 
##############
# Build the SD disk iamge
##############
def _make_sdimage(outputDir, image, catalog):
    gen_sd_fat_cfg()

    buildDate = time.strftime("%F")
    tmpImg = "%s/hdd.img" % ENV['IMAGE_DIR']
    imageFile = "%s/%s_hdd_%s_%s.img" % (
        outputDir, catalog['boardName'], image['imageName'], buildDate)

    # Cleanup any previous images
    files = [tmpImg, imageFile, imageFile + ".gz"]
    for f in files:
        rm(f)

    print_msg("Generating target image: %s" % imageFile)
    run_genimage(catalog['defaultInfo']['genimage'], ENV['IMAGE_DIR'], None)

    # Rename the image file
    #os.rename(tmpImg, imageFile)

####################################
# Public Functions
####################################
##############
# Set the default dtb
##############
def set_default_dtb(defaultApp):
    return

##############
# Set the default bitstream
##############
def set_default_bitsream(defaultApp):
    return
    
##############
# Build the SD card image
##############
def build_sdimage(outputDir, image, catalog):

    ##############
    # Call the Genimage Script
    ##############
    _make_sdimage(outputDir, image, catalog)

##############
# Configure the build process
##############
def platform_supported():
    supported = dict()
    # List of valid toolchains
    supported['toolchain'] = ["br2"]
    # List of valid OSes
    supported['rtos'] = ["linux"]
    return supported

def platform_checkconfig(args):
    return

def platform_gen_target(args, catalog): 
    return

def platform_update_catalog(catalog):
    catalog['platformInfo']['kernelDTSDir'] = [];
    return

####################################
# Module Globals
####################################
_PLATFORM_SCRIPTS = os.path.dirname(os.path.realpath(__file__))
_PLATFORM_DIR = os.path.dirname(_PLATFORM_SCRIPTS)


