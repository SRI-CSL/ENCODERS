#!/usr/bin/python

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Minyoung Kim (MK)

import sys
import re
import subprocess

def usage () :
	print "Usage: tombstone.py <test_root> [<date> <test_start_time> <test_end_time>"
	print ""
	print "It assumes that test_root directory's structure is as below"
	print "test_root - android-systme-lib: contains all android system libs including libhagglekernel_jni.so"
	print "          - maps: memory map file from /proc/<hagglepid>/maps/ on a Phone tested"
	print "          - tombstones_list.txt: result of the command \"adb_all.sh ls -al /data/tombstones\""
	print "          - tombstones - usb1 - data - tombstone - tombstone_00: tombstone "
	print "                                                 - tombstone_01"
	print "                                                   ..."
	print "                       - usb2 - data - tombstone - tombstone_00"
	print "                         ..."

PRINT_ADDRESS = False
PRINT_TOMBSTONE = True
CHECK_TIME_RANGE = False
PRINT_TO_CONSOLE = True
OUTPUT_FILE = "tomb-result.txt"
MAP_FILE_NAME="maps"
TOMBSTONE_DIR_NAME="tombstones"
ANDROID_SYSTEM_LIB_DIR_NAME="android-system-lib"
TOMBSTONE_LIST_FILE_NAME = "tombstones_list.txt"

if len(sys.argv) < 2:
	usage()
	sys.exit(0)
elif len(sys.argv) != 2 and len(sys.argv) !=5:
	usage()
	sys.exit(0)

testRoot = sys.argv[1]
tombstoneRoot = testRoot+"/"+TOMBSTONE_DIR_NAME

if True == CHECK_TIME_RANGE:
	date = sys.argv[2]
	start = sys.argv[3].replace(":","")
	end = sys.argv[4].replace(":","")

ANDROID_SYSTEM_LIB = testRoot+"/"+ANDROID_SYSTEM_LIB_DIR_NAME

tomblist = []

def getTombstones () :
	#pattern = "^Device.*\n.*"+date+" "+startH
	p1 = re.compile("usb(\d+)")
	p2 = re.compile("\s+(\d+):(\d+)\s+(tombstone_\d+)");
	p3 = re.compile("(tombstone_\d+)");

	f = open(TOMBSTONE_LIST_FILE_NAME, "rb")
	text = ""
	usb = ""
	for line in f:
		if "Device" in line:
			ret = p1.findall(line)
			if len(ret) > 1:
				print "Invalud result from pattern usb(\d+)"
				continue
			usb = ret[0]

		elif True == CHECK_TIME_RANGE and date in line:
			ret = p2.findall(line)
			if len(ret) == 1:
				if len(ret[0]) == 3:
					time = ret[0][0]+ret[0][1]
					print line
					if start <= time and time <= end:
						tomblist.append(["usb"+usb+"/data/tombstones/"+ret[0][2], ret[0][0], ret[0][1]])

		elif False == CHECK_TIME_RANGE:
			ret = p2.findall(line)
			if len(ret) == 1:
				if len(ret[0]) == 3:
					tomblist.append(["usb"+usb+"/data/tombstones/"+ret[0][2], ret[0][0], ret[0][1]])
	return

def printTombstones () :
	f = None
	if True == PRINT_TO_CONSOLE:
		f = sys.stdout
	else:
		f = open(OUTPUT_FILE, "wb")
	p3 = re.compile("pc\s([\d\w]+)")

	for entry in tomblist:
		stone = open(tombstoneRoot+"/"+entry[0], "rb")
		f.write(entry[0]+"   creation time = "+entry[1]+":"+entry[2]+"\n")
		for line in stone:
			if "pc" in line:
				ret = p3.findall(line)
				if len(ret) == 1:
					pc = ret[0]
					f.write(checklib(pc)+"\n")

		if True == PRINT_TOMBSTONE:
			stone = open(tombstoneRoot+"/"+entry[0], "rb")
			for line in stone:
				f.write(line)
			f.write("\n")
	return

def checklib (pc) :
	p = re.compile("([\d\w]+)-([\d\w]+)");
	p2 = re.compile("lib/([\d\w\+]+\.so)");
	maps = open(MAP_FILE_NAME, "rb")
	for line in maps:
		if "lib" in line:
			ret1 = p.findall(line)
			if len(ret1) > 1:
				ret2 = p2.findall(line)
				s = ret1[0][0]
				e = ret1[0][1]
				lib = ret2[0]
				if len(ret2) != 1:
					return "NO library.."
				if s <= pc and pc <= e:
					text=""
					if True == PRINT_ADDRESS:
						text =  pc +" >> "+line + "\n"
					text = text + addr2line(int("0x"+pc,0)-int("0x"+s,0), lib)
					return text
	return "NO address in the memory map"

def addr2line (addr, lib) :
	lib = ANDROID_SYSTEM_LIB+"/"+lib
	process = subprocess.Popen(['arm-linux-androideabi-addr2line','-f','-C','-e',lib,str(hex(addr))[2:]], stdout=subprocess.PIPE).communicate()[0]
	cmd = "arm-linux-androideabi-addr2line -f -C -e "+lib+" "+str(hex(addr))[2:]
	text = cmd + "\n"
	for ch in process:
		text = text+ch
	return text

process = subprocess.Popen(['which', 'arm-linux-androideabi-addr2line'], stdout=subprocess.PIPE).communicate()[0]
text=""
for ch in process:
	text = text+ch
if text == "" :
	print "Set path for arm-linux-androideabi-addr2line"
	sys.exit(0)

getTombstones()
printTombstones()
print OUTPUT_FILE
