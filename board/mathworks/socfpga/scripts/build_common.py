#!/usr/bin/env python
import sys, os, shutil, glob, imp, subprocess, time
import helper_func
from helper_func import *

supported = dict()
# List of valid toolchains
supported['toolchain'] = ["linaro"]
# List of valid OSes
supported['rtos'] = ["linux"]

def platform_checkconfig(args):
    return

def platform_gen_target(args, catalog, cfgDataList):
    return


