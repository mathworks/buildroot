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
def _create_bif(bifFilePath,fsblFile,ubootFile):
    f = open(bifFilePath, 'w')
    f.write("the_ROM_image:\n")
    f.write("{\n")
    f.write("    [bootloader]%s\n" % (fsblFile))
    f.write("    %s\n" % (ubootFile))
    f.write("}\n")
    f.close()

##############
# Copy boot.bin
##############
def _copy_boot():
    spl_bin = "boot.bin"
    spl_src_path = "%s/%s" % (ENV['IMAGE_DIR'], spl_bin)
    spl_dst_path = "%s/%s" % (ENV['SD_DIR'], spl_bin)
    uboot = "u-boot.img"
    uboot_src_path = "%s/%s"  % (ENV['IMAGE_DIR'], uboot)
    uboot_dst_path = "%s/%s"  % (ENV['SD_DIR'], uboot)

    shutil.copy(spl_src_path, spl_dst_path)
    shutil.copy(uboot_src_path, uboot_dst_path)  

##############
# Create BOOT.BIN for a given image
##############
def _create_boot(image, catalog):
    boot_bin = "BOOT.BIN"
    boot_bin_path = "%s/%s" % (ENV['IMAGE_DIR'], boot_bin)
    bif_file = "bootimage.bif"
    bif_file_path = "%s/%s" % (ENV['IMAGE_DIR'], bif_file)
    uboot_dst = "u-boot.elf"
    uboot_dst_path = "%s/%s"  % (ENV['IMAGE_DIR'], uboot_dst)
    fsbl_dst = "zynq_fsbl.elf"
    fsbl_dst_path = "%s/%s" % (ENV['IMAGE_DIR'], fsbl_dst)

    tc_path = get_cfg_var("BR2_TOOLCHAIN_EXTERNAL_PATH")
    sdk_root = os.path.dirname(os.path.dirname(os.path.dirname(tc_path)))
    bootgen_bin = sdk_root + "/bin/bootgen"    

    print_msg("Generating %s" % (boot_bin) )

    # stage files in image dir 
    # rename to .elf for bitgen
    shutil.copy("%s/u-boot" % (ENV['IMAGE_DIR']), uboot_dst_path)
    shutil.copy(catalog['defaultInfo']['fsbl'], fsbl_dst_path)

    # create BOOT.BIN
    rm(boot_bin_path)
    _create_bif(bif_file_path,fsbl_dst, uboot_dst)
    argStr = "%s -image %s -o i %s" % (bootgen_bin, bif_file, boot_bin)
    subprocess.call( argStr.split(), cwd=ENV['IMAGE_DIR'] )
    shutil.move(boot_bin_path, "%s/%s" % (ENV['SD_DIR'],boot_bin) )

    # cleanup
    os.remove(uboot_dst_path)
    os.remove(fsbl_dst_path)
    os.remove(bif_file_path)       

##############
# Copy over the app files
##############

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
    shutil.copy("%s/uImage" % (ENV['IMAGE_DIR']), ENV['SD_DIR'] )

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
    # Create/copy the boot.bin file
    ##############
    if os.path.isdir(catalog['defaultInfo']['fsbl']):
        # use the SPL
        _copy_boot()
    else:
        # Build the boot.bin, with uboot built in
        _create_boot(image, catalog)

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
    supported['toolchain'] = ["xilinx", "linaro"]
    # List of valid OSes
    supported['rtos'] = ["linux", "xenomai"]
    return supported

def platform_checkconfig(args):
    if (    args['rtos'] == "xenomai" and
            args['toolchain'] == "xilinx"):
        raise RuntimeError("Cannot use Xilinx toolchain with Xenomai OS")

def platform_gen_target(args, catalog, cfgDataList):
    # Do nothing for recovery mode
    if catalog['buildMode'] == BuildMode.RECOVERY:
        return

    if os.path.isdir(catalog['defaultInfo']['fsbl']):
        # Build the SPL using the handoff files
        cfgDataList.append("BR2_PACKAGE_UBOOT_XILINX=y\n")
        cfgDataList.append("BR2_TARGET_UBOOT_FORMAT_IMG=y\n")
        cfgDataList.append("BR2_TARGET_UBOOT_SPL=y\n")
        cfgDataList.append('BR2_TARGET_UBOOT_SPL_NAME="spl/boot.bin"\n')
        handoffStr = 'BR2_PACKAGE_UBOOT_XILINX_VIVADO_HANDOFF_DIR='
        handoffStr += '"%s"\n' % catalog['defaultInfo']['fsbl']
        cfgDataList.append(handoffStr)
    else:
        # Build the boot.bin from FSBL and UBoot
        cfgDataList.append("BR2_TARGET_UBOOT_FORMAT_ELF=y\n")
    
    return

def platform_update_catalog(catalog):
    catalog['platformInfo']['kernelDTSDir'] = [
                'arch/arm/boot/dts',
                'arch/arm/boot/dts/include']
    return

