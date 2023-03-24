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
import shlex
import logging
import select

########################################
# GLOBALS
########################################
SYS_LOGGER = None
_SUBPROC_LOGGER = None

########################################
# Logging Utilities
########################################

########################
# SubProcLogger
#
# Helper class to emit stdout/stderr to logger object
########################
class _SubProcLogger(object):
    """
    Helper class to log subprocess call output to a log file
    
    Inspired by https://gist.github.com/bgreenlee/1402841
    """
    def __init__(self, logger, log_level=logging.INFO):
        self.logger = logger
        self.log_level_stdout = log_level
        self.log_level_stderr = logging.ERROR
        self.proc = None

    def getArgs(self, args):
        if isinstance(args, str):
            args = shlex.split(args)
        return args

    def strArgs(self, args):
        if not isinstance(args, str):
            args = " ".join(args)
        return args

    def check_io(self):
        log_level = {self.proc.stdout: self.log_level_stdout,
                     self.proc.stderr: self.log_level_stderr}
        ready_to_read = select.select([self.proc.stdout, self.proc.stderr], [], [], 1000)[0]

        for io in ready_to_read:
            line = io.readline()
            if line:
                self.logger.log(log_level[io], line[:-1])

    def call(self, args, cwd=None, shell=False):
         # Support strings or lists as args
        args = self.getArgs(args)
        self.proc = subprocess.Popen(args, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=shell)
        
        # Keep checking until the process exits
        while self.proc.poll() is None:
            self.check_io()

        # Get any trailing output
        self.check_io()
        
        if self.proc.returncode != 0:
            raise subprocess.CalledProcessError(self.proc.returncode, self.strArgs(args), None)

########################
# Level Formatter
#
# Customize message format based on level
########################
class _LevelFormatter(object):
    """
    Vary the format based on the log level
    """
    def __init__(self, formatters, default_formatter):
        self.formatters = formatters
        self.default_formatter = default_formatter

    def format(self, record):
        formatter = self.formatters.get(record.levelno, self.default_formatter)
        return formatter.format(record)

########################
# Context Filter
#
# Filter messages based on supress level
########################
class _ContextFilter(logging.Filter):
    """
    Filter the messages based on supress level
        0: Supress no messages
        1: Supress messages that do not announce a build stage
        2: Supress all messages
    """
    def __init__(self, supress):
        self.supress = supress

    def filter(self, record):
        if self.supress == 0:
            return True
        if self.supress == 2:
            return False
        if ">>>" in record.getMessage():
            return True


########################
# Exception Handler
#
# Send exceptions to log
########################
def _handle_exception(exc_type, exc_value, exc_traceback):
    if issubclass(exc_type, KeyboardInterrupt):
        sys.__excepthook__(exc_type, exc_value, exc_traceback)
        return

    SYS_LOGGER.critical("Uncaught exception", exc_info=(exc_type, exc_value, exc_traceback))

########################
# Exception Handler
#
# Send exceptions to log
########################
def init_logging(level=logging.DEBUG, filename=None, filemode='w', console=None, loggerName=''):
    
    global SYS_LOGGER
    global _SUBPROC_LOGGER

    SYS_LOGGER=logging.getLogger(loggerName)
    SYS_LOGGER.setLevel(level)

    if not console is None:
        consoleH = logging.StreamHandler()
        consoleH.setLevel(level)
        consoleH.setFormatter(logging.Formatter('%(message)s'))
        consoleH.addFilter(_ContextFilter(console))
        SYS_LOGGER.addHandler(consoleH)

    if not filename is None:
        fh = logging.FileHandler(filename, mode=filemode)
        fh.setLevel(level)
        fh.setFormatter(_LevelFormatter({
                logging.ERROR : logging.Formatter('%(levelname)s:%(name)s: %(message)s'),
                logging.CRITICAL: logging.Formatter('%(levelname)s:%(name)s: %(message)s')},
            logging.Formatter('%(message)s')))
        SYS_LOGGER.addHandler(fh)
 
    _SUBPROC_LOGGER = _SubProcLogger(SYS_LOGGER, log_level=level)
    sys.excepthook = _handle_exception

########################################
# Helper Functions
########################################

########################
# Colorize Prompt
########################
def _get_color(val, bg=False):
    if bg:
        color = _BCOLORS['BG'] + str(val) + 'm'
    else:
        color = _BCOLORS['FG'] + str(val) + 'm'
    return color


########################
# File Listing
########################
def build_file_list(inDir):
    fList = os.listdir(inDir)
    for idx, f in enumerate(fList):
        fList[idx] = os.path.realpath(os.path.join(inDir, f))
    return fList

def build_relative_file_list(inDir, toDir):
    fList = build_file_list(inDir)       
    toDir = "%s/" % os.path.realpath(toDir)
    for idx, f in enumerate(fList):
        fList[idx] = fList[idx].replace(toDir,"")

    return fList

########################
# Genimage
########################
def run_genimage(cfgFile, ioPath, rootPath=""):
    genimgDir = "%s/genimg" % ioPath
    mkdir(genimgDir)
    if rootPath is None:
        rootPath = "%s/root" % genimgDir
        mkdir(rootPath)
    elif rootPath == "":
        rootPath = ENV['TARGET_DIR']
    genimgTmp = "%s/tmp" % genimgDir
    rm(genimgTmp)
    argStr = (  "genimage "
                "--rootpath %s " 
                "--tmppath %s " 
                "--inputpath %s " 
                "--outputpath %s " 
                "--config %s"  ) % (
                    rootPath,
                    genimgTmp,
                    ioPath,
                    ioPath,
                    cfgFile)
    print (argStr)
    subproc(argStr, cwd=ENV['IMAGE_DIR'] )
    rm(genimgDir)

def generate_fat_genimg_cfg(fileList, cfgFile, imgFile, size="250M"):
    f = open(cfgFile, 'w')
    f.write("image %s {\n" % imgFile)
    f.write("    vfat {\n")
    f.write("        files = {\n")
        
    for idx, elem in enumerate(fileList):
        f.write('            "%s"' % elem)
        if idx != len(fileList)-1:
            f.write(",")
        f.write("\n")
    f.write("        }\n")
    f.write("    }\n")
    f.write("    size = %s\n" % size)
    f.write("}\n")
        
    f.close()

def generate_fat_image(fatDir, imgFile):
    imgFilePath = os.path.realpath("%s/%s" % (ENV['IMAGE_DIR'], imgFile))
    rm(imgFilePath)
    print_msg("Generating FAT32 image: %s" % imgFilePath)

    cfgFile = "%s/sdcard.vfat.cfg" % ENV['IMAGE_DIR']
    fileList = build_relative_file_list(fatDir,ENV['IMAGE_DIR'])
    generate_fat_genimg_cfg(fileList, cfgFile, imgFile)

    run_genimage(cfgFile, ENV['IMAGE_DIR'])
    return imgFilePath

######################
# Generate the standard SD FAT partition config
######################
def gen_sd_fat_cfg():
    imgFile = "sdcard.vfat.img"
    cfgFile = "%s/sdcard.vfat.cfg" % ENV['IMAGE_DIR']
    fileList = build_relative_file_list(ENV['SD_DIR'],ENV['IMAGE_DIR'])
    generate_fat_genimg_cfg(fileList, cfgFile, imgFile)

########################
# CPP expansion
########################
def cpp_expand(infile, outfile, include_dirs=[], extraPreFlags="", extraPostFlags=""):

    CPP_PRE_FLAGS = "-nostdinc"
    CPP_POST_FLAGS = "-undef -x assembler-with-cpp -P"

    outfile = os.path.realpath(outfile)
    infile = os.path.realpath(infile)
    outdir = os.path.dirname(outfile)

    # Automatically add the infile directory
    include_dirs.append(os.path.dirname(infile))

    # build the cpp command
    dtc_cpp_flags = list()
    dtc_cpp_flags.extend(CPP_PRE_FLAGS.split())
    dtc_cpp_flags.extend(extraPreFlags.split())
    for inc in include_dirs:
        inc_str = "-I" + inc
        dtc_cpp_flags.append(inc_str)
    dtc_cpp_flags.extend(CPP_POST_FLAGS.split())
    dtc_cpp_flags.extend(extraPostFlags.split())

    args = ["cpp"]
    args.extend(dtc_cpp_flags)
    args.extend(["-o", outfile, infile])
    subproc(args, cwd=outdir)

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
    
    print (startMsg + prefix + msg + _BMSG['MSG_END'])

def print_err(msg):

    print (_BMSG['ERR_START'] + "ERROR:   " + msg + _BMSG['MSG_END'])

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
    subproc(cmd, cwd=cwd)

########################
# rm utility
########################

def rm(fileDir):
    if os.path.isdir(fileDir):
        shutil.rmtree(fileDir)
    elif os.path.exists(fileDir):
        os.remove(fileDir)

########################
# mkdir utility
########################
def mkdir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)


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
# Buildroot config helpers
########################
def br_add_includeDir(incdir):
    global BRCONFIG_cfgIncludeDirs    

    BRCONFIG_cfgIncludeDirs.append(incdir)

def br_add_include(incfile, addPath=True):
    global BRCONFIG_cfgDataList

    BRCONFIG_cfgDataList.append('#include "%s"\n' % incfile)
    if addPath:
        br_add_includeDir(os.path.dirname(incfile))

def br_set_var(var, value, quoted=True):
    global BRCONFIG_cfgDataList

    if (value is None) or (value.lower() == 'n'):
        argStr = "# %s is not set\n" % var
    else:
        if value.lower() == 'y':
            quoted = False

        if quoted:
            argStr = '%s="%s"\n' % (var, value)
        else:
            argStr = '%s=%s\n' % (var, value)

    BRCONFIG_cfgDataList.append(argStr)

########################
# Get version info
########################

def _git_verinfo(git_dir):
    git_hash = subproc_output('git log -n 1 --pretty="%H"', cwd=git_dir)
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
        dat = grep(dat, "%s_OVERRIDE_SRCDIR" % (pkg.upper().replace("-", "_")))
        if dat:
            # use the local.mk directory if it's specified
            return re.sub(".*= *","", dat)
            
    # Otherwise check the version and point to the build dir
    if pkg.lower() == "linux":
        ver = get_cfg_var(_LINUX_VAR)        
    elif pkg.lower() == _UBOOT_PKG:        
        ver = get_cfg_var(_UBOOT_VAR)
        pkg = _UBOOT_PKG
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
# Subprocess With Error Checking
#######################
def subproc(args, cwd=None, shell=False):

   _SUBPROC_LOGGER.call(args, cwd=cwd, shell=shell)

#######################
# Subprocess With Output
#######################
def subproc_output(args, cwd=None, stderr=None, shell=False, universal_newlines=False):

    # Support strings or lists as args
    if isinstance(args, basestring):
        args = shlex.split(args)

    return subprocess.check_output( args, cwd=cwd, stderr=stderr, shell=shell, universal_newlines=universal_newlines)

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

########################################
# Enumerations
######################################## 
class BuildMode:
    NORMAL="NORMAL"
    RECOVERY="RECOVERY"

########################################
# Exported Globals
########################################

ENV = dict()
load_br_env()
COMMON_SCRIPTS = os.path.dirname(os.path.realpath(__file__))
COMMON_DIR = os.path.dirname(COMMON_SCRIPTS)
MW_DIR = os.path.dirname(COMMON_DIR)
BR_ROOT = os.path.dirname(os.path.dirname(MW_DIR))

BRCONFIG_cfgDataList = []
BRCONFIG_cfgIncludeDirs = []

#init_logging()
