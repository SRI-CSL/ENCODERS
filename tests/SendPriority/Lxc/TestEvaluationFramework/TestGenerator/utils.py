#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Basic OS utility functions for test_generator.py
# 
import os
import io
import time
import commands

def replace_file(in_file_name, out_file_name, replace_tuples=[]): 
    of = io.open(out_file_name, 'w')
    for line in io.open(in_file_name, 'r'):
        for (f, r) in replace_tuples:
            line = line.replace(f, str(r)) 
        of.write(line)
    of.close()


def cp(in_file_name, out_file_name):
    replace_file(in_file_name, out_file_name)

def now():
    return str(int(time.time() * 1000))

def mkdir(path_name):
    os.makedirs(path_name)

def chmodx(path_name):
    cmd_str = "chmod +x %s" % path_name
    commands.getoutput(cmd_str)

def testset(cfg, param_name, value):
    try:
        if not cfg[param_name]:
            raise Exception()
    except:
        cfg[param_name] = value
