#!/usr/bin/env python
import sys, os, shutil, glob, imp, subprocess, time
import helper_func
from helper_func import *

####################################
# Helper Functions
####################################   

####################################
# Public Functions
####################################
##############
# Set the default dtb
##############
def set_default_dtb(defaultApp):
    defaultDTB = "devicetree_%s.dtb" % (defaultApp['name'])
    shutil.copyfile("%s/%s" %(ENV['SD_DIR'], defaultDTB), "%s/devicetree.dtb" % (ENV['SD_DIR']))

##############
# Set the default bitstream
##############
def set_default_bitsream(defaultApp):
    shutil.copyfile(defaultApp['bit'], "%s/system.bit" % (ENV['SD_DIR']))

##############
# Build the SD card image
##############
def build_sdimage(outputDir, image, catalog):

    defaultApp = image['defaultApp']
 
    ##############
    # Move the kernel
    ##############
    shutil.copy("%s/Image" % (ENV['IMAGE_DIR']), ENV['SD_DIR'] )

    ##############
    # Copy over the application specific rbfs
    ##############
    for app in image['appList']:
        if not app['bit'] is None:
            appBit = "%s/%s.bit" % (ENV['SD_DIR'], app['name'])
            shutil.copy(app['bit'], appBit)

    ##############
    # Copy the u-boot ramdisk image
    ##############
    CPIO_IMG = ENV['IMAGE_DIR'] + "/rootfs.cpio.uboot"
    UIMAGE= ENV['SD_DIR'] + "/uramdisk.image.gz"
    shutil.copy(CPIO_IMG, UIMAGE)
    
    ##############
    # Copy the boot.bin / uboot files
    ##############
    bootFileList = ["boot.bin"]
    if os.path.isdir(catalog['defaultInfo']['fsbl']):
        bootFileList.append("u-boot.bin")
        bootFileList.append("atf-uboot.ub")

    for bootFile in bootFileList:
        src_path = "%s/%s" % (ENV['IMAGE_DIR'], bootFile)
        dst_path = "%s/%s" % (ENV['SD_DIR'], bootFile)
        shutil.copy(src_path, dst_path)    

    ##############
    # Zip the SD Card Directory
    ##############
    buildDate = time.strftime("%F")
    zipName = "%s_sdcard_%s_%s" % (catalog['boardName'], image['imageName'], buildDate)
    zipFile = zipName + ".zip"
    print_msg("Generating %s/%s" % (outputDir, zipFile), level=5)
    shutil.make_archive("%s/%s" % (outputDir, zipName), "zip", root_dir=ENV['SD_DIR'])

##############
# Configure the build process
##############
def platform_supported():
    supported = dict()
    # List of valid toolchains
    supported['toolchain'] = ["linaro"]
    # List of valid OSes
    supported['rtos'] = ["linux"]
    return supported

def platform_checkconfig(args):
    return

def platform_gen_target(args, catalog):
    # Do nothing for recovery mode
    if catalog['buildMode'] == BuildMode.RECOVERY:
        return

    if os.path.isdir(catalog['defaultInfo']['fsbl']):
        # Build the SPL using the handoff files
        br_set_var("BR2_TARGET_UBOOT_FORMAT_BIN", "y")
        br_set_var("BR2_TARGET_UBOOT_SPL", "y")
        br_set_var("BR2_TARGET_UBOOT_SPL_NAME", "spl/boot.bin")
        br_set_var("BR2_PACKAGE_XILINX_BOOTLOADER_VIVADO_HANDOFF_DIR", catalog['defaultInfo']['fsbl'])
    else:
        # Build the boot.bin from FSBL and UBoot
        br_set_var("BR2_TARGET_UBOOT_FORMAT_ELF","y")
        br_set_var("BR2_TARGET_UBOOT_SPL", None)
        br_set_var("BR2_PACKAGE_XILINX_BOOTLOADER_FSBL_ELF_PATH", catalog['defaultInfo']['fsbl'])

    if not catalog['defaultInfo']['pmufw'] is None:
        br_set_var("BR2_PACKAGE_XILINX_BOOTLOADER_PMUFW", "y")
        br_set_var("BR2_PACKAGE_XILINX_BOOTLOADER_PMUFW_PATH", catalog['defaultInfo']['pmufw'])

    return

def platform_update_catalog(catalog):
    catalog['platformInfo']['kernelDTSDir'] = [
                'arch/arm64/boot/dts/xilinx',
                'arch/arm64/boot/dts/include']
    return
