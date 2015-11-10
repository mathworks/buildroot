#!/usr/bin/env python
import sys, os, shutil, glob, imp, subprocess, time, shlex
import helper_func
from helper_func import *

####################################
# Helper functions
####################################
##############
# Build a list of files in a dir
##############
def _build_file_list(inDir):
    fList = os.listdir(inDir)
    for idx, f in enumerate(fList):
        fList[idx] = os.path.join(inDir, f)
    return fList
    
##############
# Run a sudo command
##############
def sudocmd(cmd, cwd=None, env=None):
    if cwd is None:
        cwd = os.getcwd()
    if isinstance(cmd, str):
        cmd = shlex.split(cmd)
    if not env is None:
        newcmd = env
        newcmd.insert(0,"env")
        newcmd.extend(cmd)
        cmd = newcmd
    cmd.insert(0,"sudo")
    subprocess.call(cmd, cwd=cwd)
    
##############
# Build the SD disk iamge
##############
def _make_sdimage(image, catalog):
    # Build up a path for the sudo command
    binDirs = ["bin", "sbin", "usr/sbin", "usr/bin"]
    for idx, d in enumerate(binDirs):
        binDirs[idx] = os.path.join(ENV['HOST_DIR'], d)
    sudoPath = "PATH=%s:%s" % (":".join(binDirs), os.environ['PATH'])
    
    SPL = "%s/preloader-mkpimage.bin" % (ENV['IMAGE_DIR'])
    bootLoader = "%s/u-boot.img" % (ENV['IMAGE_DIR'])
    buildDate = time.strftime("%F")
    fatFileList = ",".join(_build_file_list(ENV['SD_DIR']))
    imageSize = 1000
    fatSize = 250
    a2Size = 10
    extSize = (imageSize - fatSize - a2Size - 10)
    rootFSFile = "%s/rootfs.tar.gz" % (ENV['IMAGE_DIR'])
    imageFile = "%s/%s_sdcard_%s_%s.img" % (ENV['IMAGE_DIR'], catalog['boardName'], image['imageName'], buildDate)
    # Cleanup any previous images
    files = [imageFile, imageFile + ".gz"]
    for f in files:
        rm(f)        

    print_msg("Generating disk image: %s.gz" % (imageFile), level=5)
    # Untar the rootfs
    rootFSDir = "%s/tempRootFS" % (ENV['IMAGE_DIR'])
    # remove the dir first, then create it (need to be root)
    rmRootFSCmd = "rm -rf %s" % (rootFSDir)
    sudocmd(rmRootFSCmd)
    os.makedirs(rootFSDir)
    # untar as root
    sudocmd("tar -xzf %s" % (rootFSFile), cwd=rootFSDir)

     
    mkSDStr = ["%s/make_sdimage.py -f" % (_PLATFORM_SCRIPTS),
        "-P %s,%s,num=3,format=raw,size=%dM,type=A2" % (SPL, bootLoader, a2Size),
        "-P %s,num=2,format=ext3,size=%dM" % (rootFSDir, extSize), 
        "-P %s,num=1,format=vfat,size=%dM" % (fatFileList, fatSize),
        "-s %dM" % (imageSize),
        "-n %s" % (imageFile)]
    mkSDStr = ' '.join(mkSDStr)
    sudocmd(mkSDStr, cwd=ENV['IMAGE_DIR'], env=[sudoPath])
    
    # Cleanup the temp rootfs
    sudocmd(rmRootFSCmd)

    # Change the owner back to the current user
    thisUser = subprocess.check_output("id -un".split()).strip('\n')
    thisGroup = subprocess.check_output("id -gn".split()).strip('\n')
    sudocmd("chown %s:%s %s" % (thisUser,thisGroup, imageFile))    

    #compress the SD card image
    sudocmd("gzip %s" % (imageFile), cwd=ENV['IMAGE_DIR'])

####################################
# Public Functions
####################################
##############
# Set the default dtb
##############
def set_default_dtb(defaultApp):
    defaultDTB = "devicetree_%s.dtb" % (defaultApp['name'])
    shutil.copyfile("%s/%s" %(ENV['SD_DIR'], defaultDTB), "%s/socfpga.dtb" % (ENV['SD_DIR']))
    
##############
# Build the SD card image
##############
def build_sdimage(outputDir, image, catalog):

    defaultApp = image['defaultApp']

    ##############
    # Move the kernel
    ##############
    shutil.copy("%s/zImage" % (ENV['IMAGE_DIR']), ENV['SD_DIR'] )

    ##############
    # Copy over the application specific rbfs
    ##############
    for app in image['appList']:
        appBit = "%s/socfpga_%s.rbf" % (ENV['SD_DIR'], app['name'])
        shutil.copy(app['bit'], appBit)

    # boot from the base rbf by default
    print_msg("Setting %s as default rbf" % (defaultApp['name']))
    shutil.copy("%s/socfpga_%s.rbf" % (ENV['SD_DIR'], defaultApp['name']), "%s/socfpga.rbf" % (ENV['SD_DIR']) )

    ##############
    # Copy over the u-boot script
    ##############
    # Copy to image dir
    shutil.copy("%s/boot/u-boot-scr.txt" % (_PLATFORM_DIR), "%s/u-boot-scr.txt" % (ENV['IMAGE_DIR']) )    
    # Convert to uimage
    argStr = """%s/usr/bin/mkimage -A arm -O linux -T script -C none -a 0 -e 0 -n "U-Boot Script" -d u-boot-scr.txt u-boot.scr""" % (ENV['HOST_DIR'])
    subprocess.call(shlex.split(argStr), cwd=ENV['IMAGE_DIR'])
    # move to sd card
    shutil.move("%s/u-boot.scr" % (ENV['IMAGE_DIR']), "%s/u-boot.scr" % (ENV['SD_DIR']))

    ##############
    # Copy over u-boot (SPL will load u-boot.img)
    ##############
    shutil.copy("%s/u-boot.img" % (ENV['IMAGE_DIR']), "%s/u-boot.img" % (ENV['SD_DIR']))
    
    ##############
    # Call the Altera Script
    ##############
    _make_sdimage(image, catalog)

####################################
# Module Globals
####################################
_PLATFORM_SCRIPTS = os.path.dirname(os.path.realpath(__file__))
_PLATFORM_DIR = os.path.dirname(_PLATFORM_SCRIPTS)


