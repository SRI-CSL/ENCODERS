#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Start-up script for each lxc. Launches haggle and pauses until
# haggletest can connect (waits until haggle is initialized). 
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NODE_NAME=$1

if [ ! -x "/usr/local/bin/haggle" ]; then
    echo "No haggle executable daemon." >> $(bash "${DIR}/echo_fail_file.sh")
    exit 1
fi

ulimit -c unlimited

if [ "0" != "$?" ]; then
    echo "Could not execute ulimit." >> $(bash "${DIR}/echo_fail_file.sh")
    exit 1
fi

/usr/local/bin/haggle -d -dd -f

if [ ! -x "/usr/local/bin/haggletest" ]; then
    echo "No /usr/local/bin/haggletest executable." >> $(bash "${DIR}/echo_fail_file.sh")
    exit 1
fi

CONNECTED=0
for i in $(seq 1 6); do
    sleep ${i}
    /usr/local/bin/haggletest nop &> /dev/null
    if [ "0" == "$?" ]; then
        CONNECTED=1
        break
    fi
done

if [ "0" == "${CONNECTED}" ]; then
    echo "Could not get connect to haggle." >> $(bash "${DIR}/echo_fail_file.sh")
    exit 1
fi

USER=$(/usr/bin/whoami)

if [ ! -r "/home/${USER}/.Haggle/haggle.pid" ]; then
    echo "Could not get haggle pid." >> $(bash "${DIR}/echo_fail_file.sh")
    exit 1
fi
PID=$(cat /home/${USER}/.Haggle/haggle.pid)
bash "${DIR}/cpulimit_pid.sh" ${PID}

if [ "0" != "$?" ]; then
    echo "Haggle could not properly start, pid : ${PID}" >> $(bash "${DIR}/echo_fail_file.sh")
    exit 1
fi

exit 0
