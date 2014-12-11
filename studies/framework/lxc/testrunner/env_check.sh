#!/bin/bash

#


# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)
#   Tim McCarthy (TTM)

# Environment check for TestRunner.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ "${USER}" == "root" ]; then echo "Cannot run as root."
    exit 1
fi

HASCPULIMIT=$(which limitcpu | wc -l)
if [ "${HASCPULIMIT}" != "1" ]; then
    echo "limitcpu is not installed. install it from setup/cpulimit\n"
    exit 1
fi

DIRSET=$(grep "custom_services_dir" /etc/core/core.conf | grep "^#" | wc -l)
if [ "${DIRSET}" != "0" ]; then
    echo "Custom services dir is not set."
    exit 1
fi

HASEMANE=$(which emane | wc -l)
if [ "${HASEMANE}" != "1" ]; then
    echo "emane is not installed."
    exit 1
fi

if [ -z "${LANGUAGE}" ]; then
    echo "Setting your \$LANGUAGE var to 'en_US.UTF-8'! EMANE can fail otherwise."
    export LANGUAGE="en_US.UTF-8"
fi

if [ -z "${LANG}" ]; then
    echo "Setting your \$LANG var to 'en_US.UTF-8'! EMANE can fail otherwise."
    export LANG="en_US.UTF-8"
fi

if [ -z "${LC_ALL}" ]; then
    echo "Setting your \$LC_ALL var to 'en_US.UTF-8'! EMANE can fail otherwise."
    export LC_ALL="en_US.UTF-8"
fi

TMPMEMMOUNT=$(grep -l "/tmp *tmpfs" /etc/fstab | wc -l)

if [ "${TMPMEMMOUNT}" == "0" ]; then
    echo "Warning: _not_ using in memory /tmp, emulation may be too slow!"
    echo "We suggest having lots of RAM and adding the following line to '/etc/fstab':"
    echo "none /tmp tmpfs defaults,size=0 0 0"
    echo ""
fi

emanegentransportxml --help &> /dev/null

if [ "$?" != "1" ]; then
    echo "Could not execute emanegentransportxml, is emane installed correctly? (you may need to install 'sudo apt-get install libxml-libxml-perl')"
    exit 1
fi

TCP_SYN_RETRIES=$(sysctl net.ipv4.tcp_syn_retries | awk '{print $3;}')
if [ "${TCP_SYN_RETRIES}" != "0" ]; then
    echo "WARNING: we recommend 'sysctl -w net.ipv4.tcp_syn_retries=0'"
fi

HAGGLETEST=$(which haggletest | wc -l)
if [ "${HAGGLETEST}" == "0" ]; then
    echo "WARNING: could not find haggletest in path, is it installed?"
fi

ARPHELPER=$(which arphelper | wc -l)
if [ "${ARPHELPER}" == "0" ]; then
    echo "WARNING: could not find arphelper, is it installed w/ setuid root and execute?"
else
    ARPHELPER_PERMISSIONS=$(ls -lta $(which arphelper) | cut -b 4)
    if [ "${ARPHELPER_PERMISSIONS}" != "s" ]; then
        echo "WARNING: arphelper does not have setuid root (need to do sudo chmod +xs \$(which arphelper))"
    fi
fi

RELEASE=$(lsb_release -r | awk '{print $2;}')
if [ "${RELEASE}" != "11.10" ]; then
    echo "WARNING: testrunner/generator has ONLY been tested on Ubuntu 11.10, proceed at your own risk (${RELEASE})"
fi

# CORE can sometimes create some very large files (syslog was
# 500 GB at one point)
du /var/log/syslog | awk '{if ($1 > 1024*100) print "WARNING: /var/log/syslog > 100MB!" }'
du /var/log/kern.log | awk '{if ($1 > 1024*100) print "WARNING: /var/log/kern.log > 100MB!" }'
du /var/log/coredpy.log | awk '{if ($1 > 1024*100) print "WARNING: /var/log/coredpy.log > 100MB!" }'

sudo cat /etc/sudoers | grep timestamp | awk -F= '{if ($2 > 0) print "WARNING: sudo time may expire! we recommend disabling sudo timeout (/etc/sudoers, timestamp_timeout=-1)";}'

exit 0
