#!/bin/bash
PERIOD=10
USER=$(whoami)
LOGFILE="/home/${USER}/.Haggle/resources.log"
PID=$(cat "/home/${USER}/.Haggle/haggle.pid")
DURATION=60
TT=0
while [ ${TT} -lt ${DURATION} ]; do
    sleep ${PERIOD}
    TT=$((TT + PERIOD))
    echo $(date '+%s'),$(ps -eo pid,pcpu,rss | grep " ${PID} " | grep -v 'grep' | awk '{print $2","$3;}') >> ${LOGFILE}
done
