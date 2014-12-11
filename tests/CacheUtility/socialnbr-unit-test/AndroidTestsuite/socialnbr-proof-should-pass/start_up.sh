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
dd if=/dev/urandom of=/tmp/data-a1.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-a2.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-a3.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-b1.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-b2.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-b3.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-c1.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-c2.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-c3.10K bs=1024 count=10 >& /dev/null

adb_n $NUM_DEVICES "push /tmp/data-a1.10K /data/tmp/data-a1.10K" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data-a2.10K /data/tmp/data-a2.10K" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data-a3.10K /data/tmp/data-a3.10K" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data-b1.10K /data/tmp/data-b1.10K" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data-b2.10K /data/tmp/data-b2.10K" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data-b3.10K /data/tmp/data-b3.10K" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data-c1.10K /data/tmp/data-c1.10K" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data-c2.10K /data/tmp/data-c2.10K" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data-c3.10K /data/tmp/data-c3.10K" &> /dev/null

sed "s:%%scen_path%%:${DIR}/mobile.scen:" ${DIR}/mobile.imn.template > ${DIR}/mobile.imn

exit 0
