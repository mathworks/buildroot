#!/usr/bin/env python
import sys, os, shutil, glob, imp, subprocess, time
import helper_func
from helper_func import *

supported = dict()
# List of valid toolchains
supported['toolchain'] = ["xilinx", "linaro"]
# List of valid OSes
supported['rtos'] = ["linux", "xenomai"]

def platform_checkconfig(args):
    if (    args['rtos'] == "xenomai" and
            args['toolchain'] == "xilinx"):
        raise RuntimeError("Cannot use Xilinx toolchain with Xenomai OS")

def platform_gen_target(args, catalog, cfgDataList):
    return



