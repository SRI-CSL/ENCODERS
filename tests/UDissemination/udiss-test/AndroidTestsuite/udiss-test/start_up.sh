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
dd if=/dev/urandom of=/tmp/data.30k bs=1024 count=30 >& /dev/null
dd if=/dev/urandom of=/tmp/data.60k bs=1024 count=60 >& /dev/null
dd if=/dev/urandom of=/tmp/data.90k bs=1024 count=90 >& /dev/null
dd if=/dev/urandom of=/tmp/data.120k bs=1024 count=120 >& /dev/null
dd if=/dev/urandom of=/tmp/data.150k bs=1024 count=150 >& /dev/null
dd if=/dev/urandom of=/tmp/data.180k bs=1024 count=180 >& /dev/null

adb_n $NUM_DEVICES "push /tmp/data.30k /data/tmp/data.30k" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data.60k /data/tmp/data.60k" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data.90k /data/tmp/data.90k" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data.120k /data/tmp/data.120k" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data.150k /data/tmp/data.150k" &> /dev/null
adb_n $NUM_DEVICES "push /tmp/data.180k /data/tmp/data.180k" &> /dev/null

sed "s:%%scen_path%%:${DIR}/mobile.scen:" ${DIR}/mobile.imn.template > ${DIR}/mobile.imn

exit 0
