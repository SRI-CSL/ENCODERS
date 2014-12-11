#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Keep track of basic resource usage throughout the test.
#

PERIOD=10
USER=$(whoami)
LOGFILE="/home/${USER}/.Haggle/resources.log"
PID=$(cat "/home/${USER}/.Haggle/haggle.pid")
DURATION=180
TT=0
while [ ${TT} -lt ${DURATION} ]; do
    sleep ${PERIOD}
    TT=$((TT + PERIOD))
    echo $(date '+%s'),$(ps -eo pid,pcpu,rss | grep " ${PID} " | grep -v 'grep' | awk '{print $2","$3;}') >> ${LOGFILE}
done
