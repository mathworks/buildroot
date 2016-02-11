#!/usr/bin/env python
# available environment variables
#    BR2_CONFIG: path to .config file
#    HOST_DIR, STAGING_DIR, TARGET_DIR
#    BINARIES_DIR: images dir
#    BASE_DIR: base output directory

import sys
import subprocess
import os
import re
import shutil

########################################
# Helper Functions
########################################
def _get_color(val, bg=False):
    if bg:
        color = _BCOLORS['BG'] + str(val) + 'm'
    else:
        color = _BCOLORS['FG'] + str(val) + 'm'
    return color

########################
# Printing Functions
########################
def print_msg(msg, level=4, mlevel=6, bg = 0, fg = 7, bold=False):
    prefix = ""
    for x in range(0, level):
        prefix = prefix + ">"

    for x in range(level, mlevel):
        prefix = prefix + " "   

    startMsg = _get_color(bg, bg=True) + _get_color(fg)
    if bold:
        startMsg = startMsg + _BCOLORS['BOLD']
    
    print startMsg + prefix + msg + _BMSG['MSG_END']

def print_err(msg):

    print _BMSG['ERR_START'] + "ERROR:   " + msg + _BMSG['MSG_END']

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

########################
# rm utility
########################

def rm(fileDir):
    if os.path.isdir(fileDir):
        shutil.rmtree(fileDir)
    elif os.path.exists(fileDir):
        os.remove(fileDir)


########################
# Grep utility
########################

def grep(s,pattern):
    return '\n'.join(re.findall(r'^.*%s.*?$'%pattern,s,flags=re.M))

########################
# .config parser
########################

def get_cfg_var(var):
    
    f = open(ENV['BR2_CONFIG'], 'r')
    dat = f.read()
    f.close()
    
    line = grep(dat,"^" + var + " *=")
    line = re.sub("BR2_.*=","", line)
    line = re.sub("\"","", line)
    return line

########################
# Get version info
########################

def _git_verinfo(git_dir):
    git_hash = subprocess.check_output(['git','log', '-n', '1', '--pretty="%H"'], cwd=git_dir)
    git_hash = re.sub("\n", "", git_hash)
    git_hash = re.sub('"',"", git_hash)
    return git_hash

def verinfo(pkg):
    
    srcDir = get_src_dir(pkg)
    verFile = "%s/.br2_version" % (srcDir)
    if os.path.exists(verFile):
        f = open(verFile, 'r')
        dat = f.read()
        dat = re.sub("\n","",dat)
        f.close()
    else:
        dat = _git_verinfo(srcDir)
    return dat


########################
# Create version info file
########################

def gen_verinfo_file(tgt_file):
    
    br_hash = _git_verinfo(BR_ROOT)
    linux_hash = verinfo('linux')
    uboot_hash = verinfo('uboot')
    f = open(tgt_file, 'w')
    f.write("Buildroot: %s\n" % (br_hash))    
    f.write("Linux: %s\n" % (linux_hash))
    f.write("U-boot: %s\n" % (uboot_hash))
    f.close()

########################
# Locate source directory
########################

def get_src_dir(pkg):

    # Check if we're overriding the source
    override_file = ""    
    tstFile = "%s/local.mk" % (ENV['BASE_DIR'])
    if os.path.isfile(tstFile):     
        override_file=tstFile
    
    tstFile = "%s/local.mk" % (os.path.dirname(ENV['BR2_CONFIG']))    
    if os.path.isfile(tstFile):
        override_file=tstFile

    if override_file != "":
        # grab the source file from the override dir
        f = open(override_file, 'r')
        dat = f.read()
        f.close()
        dat = grep(dat, "%s_OVERRIDE_SRCDIR" % (pkg.upper()))
        if dat:
            # use the local.mk directory if it's specified
            return re.sub(".*= *","", dat)
            
    # Otherwise check the version and point to the build dir
    if pkg.lower() == "linux":
        ver = get_cfg_var(_LINUX_VAR)        
    elif pkg.lower() == "uboot":        
        ver = get_cfg_var(_UBOOT_VAR)
        pkg = _UBOOT_PKG
        if ver == "":
            ver = get_cfg_var(_UBOOT_ALTERA_VAR)
            pkg = _UBOOT_ALTERA_PKG
    else:
        ver = ""

    if ver != "":
        return "%s/%s-%s" % (ENV['BUILD_DIR'], pkg.lower(), ver)        
    else:        
        return ver

########################
# Load environment variable
########################
def load_env(var, default=""):
    var = os.environ.get(var)
    if var is None:
        var = default
    return var

#######################
# Subprocess logger
#######################
def subproc_log(cmdStr, logfile=None, cwd=None, verbose=True):
    if cwd is None:
        cwd = os.getcwd()
    if (logfile is None) and (verbose == False):
        logfile = "/dev/null"

    if (logfile is None):
        subprocess.call( cmdStr.split(), cwd=cwd)
    else:
        if verbose == False:
            with open(logfile, 'w') as f:
                subprocess.call( cmdStr.split(), cwd=cwd, stdout=f, stderr=subprocess.STDOUT)    
        else:            
            cmdStr += " | tee %s" % logfile
            subprocess.call( cmdStr.split(), cwd=cwd)

########################
# Load buildroot env
########################
def load_br_env():
    global ENV
    envVars = ['BR2_CONFIG', 'HOST_DIR', 'STAGING_DIR', 'TARGET_DIR',
            'BASE_DIR', 'BUILD_DIR', 'BINARIES_DIR']
    for var in envVars:
        ENV[var] = load_env(var)    
    ENV['IMAGE_DIR'] = ENV['BINARIES_DIR']
    ENV['SD_DIR'] = ENV['IMAGE_DIR'] + "/sdcard"

########################
# Load buildroot env
########################
def set_br_env(outputDir):
    os.environ['BR2_CONFIG'] = outputDir + "/.config"
    os.environ['HOST_DIR'] = outputDir + "/host"
    os.environ['STAGING_DIR'] = outputDir + "/staging"
    os.environ['TARGET_DIR'] = outputDir + "/target"
    os.environ['BUILD_DIR'] = outputDir + "/build"
    os.environ['BINARIES_DIR'] = outputDir + "/images"
    os.environ['BASE_DIR'] = outputDir
    load_br_env()

########################################
# Module Globals
########################################

_BCOLORS = {
    'FG' : '\033[3',
    'BG' : '\033[4',
    'FG_WHITE': '\033[37m',
    'FG_GREEN': '\033[32m',
    'BG_BLACK': '\033[40m', 
    'BG_RED': '\033[41m',
    'BOLD' : '\033[1m',
    'ENDC' : '\033(B\033[m'    
}
_BMSG = {
    'MSG_START' : _BCOLORS['FG_WHITE'] + _BCOLORS['BG_BLACK'],    
    'ERR_START' : _BCOLORS['FG_WHITE'] + _BCOLORS['BG_RED'] + _BCOLORS['BOLD'], 
    'MSG_END' : _BCOLORS['ENDC']
}

_LINUX_VAR = "BR2_LINUX_KERNEL_CUSTOM_REPO_VERSION"
_UBOOT_VAR = "BR2_TARGET_UBOOT_CUSTOM_REPO_VERSION"
_UBOOT_PKG = "uboot"
_UBOOT_ALTERA_VAR = "BR2_PACKAGE_UBOOT_ALTERA_CUSTOM_REPO_VERSION"
_UBOOT_ALTERA_PKG = "uboot-altera"

########################################
# Exported Globals
########################################

ENV = dict()
load_br_env()
COMMON_SCRIPTS = os.path.dirname(os.path.realpath(__file__))
COMMON_DIR = os.path.dirname(COMMON_SCRIPTS)
MW_DIR = os.path.dirname(COMMON_DIR)
BR_ROOT = os.path.dirname(os.path.dirname(MW_DIR))


