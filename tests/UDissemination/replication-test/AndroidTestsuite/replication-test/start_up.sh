#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW)

#
# Generate the .imn at run time (this is to avoid PATH issues).
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

NUM_DEVICES=$1
. $ANDROID_TESTRUNNER/adbtool.sh
dd if=/dev/urandom of=/tmp/data-a1.10k bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-b1.11k bs=1024 count=11 >& /dev/null

adb_n $NUM_DEVICES "push /tmp/data-a1.10k /data/tmp/data-a1.10k" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data-b1.11k /data/tmp/data-b1.11k" &> /dev/null

sed "s:%%scen_path%%:${DIR}/mobile.scen:" ${DIR}/mobile.imn.template > ${DIR}/mobile.imn

exit 0
