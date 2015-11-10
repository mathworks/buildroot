#!/usr/bin/env python
import sys, os, shutil, glob, imp, subprocess, time
import helper_func
from helper_func import *

####################################
# Helper Functions
####################################

##############
# Create a bif file
##############
def _create_bif(bifFilePath,fsblFile,bitFile,ubootFile):
    f = open(bifFilePath, 'w')
    f.write("the_ROM_image:\n")
    f.write("{\n")
    f.write("    [bootloader]%s\n" % (fsblFile))
    f.write("    %s\n" % (bitFile))
    f.write("    %s\n" % (ubootFile))
    f.write("}\n")
    f.close()

##############
# Create BOOT.BIN for a given app
##############
def _create_boot(app):
    boot_bin = "BOOT_%s.BIN" % (app['name'])
    boot_bin_path = "%s/%s" % (ENV['IMAGE_DIR'], boot_bin)
    bif_file = "bootimage.bif"
    bif_file_path = "%s/%s" % (ENV['IMAGE_DIR'], bif_file)
    uboot_dst = "u-boot.elf"
    uboot_dst_path = "%s/%s"  % (ENV['IMAGE_DIR'], uboot_dst)
    fsbl_dst = "zynq_fsbl.elf"
    fsbl_dst_path = "%s/%s" % (ENV['IMAGE_DIR'], fsbl_dst)
    bit_dst = "zynq.bit"
    bit_dst_path = "%s/%s" % (ENV['IMAGE_DIR'], bit_dst)

    tc_path = get_cfg_var("BR2_TOOLCHAIN_EXTERNAL_PATH")
    sdk_root = os.path.dirname(os.path.dirname(os.path.dirname(tc_path)))
    bootgen_bin = sdk_root + "/bin/bootgen"    

    print_msg("Generating %s" % (boot_bin) )

    # stage files in image dir 
    # rename to .elf for bitgen
    shutil.copy("%s/u-boot" % (ENV['IMAGE_DIR']), uboot_dst_path)
    shutil.copy(app['fsbl'], fsbl_dst_path)
    shutil.copy(app['bit'], bit_dst_path)

    # create BOOT.BIN
    rm(boot_bin_path)
    _create_bif(bif_file_path,fsbl_dst,bit_dst,uboot_dst)
    argStr = "%s -image %s -o i %s" % (bootgen_bin, bif_file, boot_bin)
    subprocess.call( argStr.split(), cwd=ENV['IMAGE_DIR'] )
    shutil.move(boot_bin_path, "%s/%s" % (ENV['SD_DIR'],boot_bin) )

    # cleanup
    os.remove(uboot_dst_path)
    os.remove(fsbl_dst_path)
    os.remove(bit_dst_path)
    os.remove(bif_file_path)       
    #rm ${UBOOT_ELF} ${UBOOT_ELF}.bin ${FSBL_DST} ${FSBL_DST}.bin ${BITSTREAM_DST} ${BIF_FILE} ${BOOT_BIN} &>/dev/null

##############
# Create BOOT.BIN for the image
##############
def _gen_boot(image):
    for app in image['appList']:
        if app['buildBoot']:
            _create_boot(app)

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
# Build the SD card image
##############
def build_sdimage(outputDir, image, catalog):

    defaultApp = image['defaultApp']
 
    ##############
    # Move the kernel
    ##############
    shutil.copy("%s/uImage" % (ENV['IMAGE_DIR']), ENV['SD_DIR'] )

    ##############
    # Create the u-boot ramdisk image
    ##############
    MKIMAGE_BIN = ENV['HOST_DIR'] + "/usr/bin/mkimage"
    CPIO_IMG = ENV['IMAGE_DIR'] + "/rootfs.cpio.gz"
    UIMAGE= ENV['SD_DIR'] + "/uramdisk.image.gz"
    print_msg("Creating uramdisk %s" % (UIMAGE))
    argStr = "%s -A arm -T ramdisk -C gzip -d %s %s" % (MKIMAGE_BIN, CPIO_IMG, UIMAGE)
    subprocess.call(argStr.split())
    
    ##############
    # Create the boot.bin files
    ##############
    _gen_boot(image)
    print_msg("Setting %s as default BOOT.BIN" % (defaultApp['name']))
    shutil.copy("%s/BOOT_%s.BIN" % (ENV['SD_DIR'], defaultApp['name']), "%s/BOOT.BIN" % (ENV['SD_DIR']) )

    ##############
    # Zip the SD Card Directory
    ##############
    buildDate = time.strftime("%F")
    zipName = "%s_sdcard_%s_%s" % (catalog['boardName'], image['imageName'], buildDate)
    zipFile = zipName + ".zip"
    print_msg("Generating %s/%s" % (outputDir, zipFile), level=5)
    shutil.make_archive("%s/%s" % (outputDir, zipName), "zip", root_dir=ENV['SD_DIR'])


