#!/bin/bash

#


# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


# Record information about the system under test.

if [[ $UID != 0 ]]; then 
    echo 'Sorry, must be root.  Exiting...'
    exit 1
fi

if [ ! -r /proc ]; then
    echo "File system does not have /proc, skipping hw reporting."
    exit 0
fi

echo "*****[CPU INFORMATION]*****" 
cat /proc/cpuinfo 
echo " " 
echo "*****[DISK & PARTITION]*****"
cat /proc/partitions
df -h
echo " "
echo "*****[MEMORY INFORMATION]*****"
cat /proc/meminfo
echo " "
echo " ******* [SYSTEM] ****** "
uname -a
lsmod
echo " "
echo " ******* [PCI] ****** "
lspci
echo " "
echo " ******* [SOFTWARE] ****** "
uname -a
dpkg -l
echo " ******* [ sysctl.conf ] ****** "
cat /etc/sysctl.conf
