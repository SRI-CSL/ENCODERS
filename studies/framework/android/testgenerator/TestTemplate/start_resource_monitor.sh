#!/bin/bash

#


# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)

#
# Keep track of basic resource usage throughout the test.
#

TEST_DIR=$1
DURATION=$2
PERIOD=10
LOGFILE="$TEST_DIR/resources.log"
PID=$(cat "/data/data/org.haggle.kernel/files/haggle.pid")
TT=0
while [ ${TT} -lt ${DURATION} ]; do
    sleep ${PERIOD}
    TT=$((TT + PERIOD))
    echo $(date '+%s'),$(ps -eo pid,pcpu,rss | grep " ${PID} " | grep -v 'grep' | awk '{print $2","$3;}') >> ${LOGFILE}
done
