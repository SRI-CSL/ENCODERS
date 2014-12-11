#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Final shutdown (kill remaining haggle processes)
#
# NOTE: this script is executed by testrunner

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

rm -f /tmp/100k_n*


# synchronize with the core haggles that are shutting down
NUM_PIDS=$(ps h -C haggle | grep haggle | awk '{print $1;}' | wc -l)
PIDS=$(ps h -C haggle | grep haggle | awk '{print $1;}')
if [ "${NUM_PIDS}" != "0" ]; then
    echo "${NUM_PIDS} Haggle instances are still running, waiting at most 300 seconds to shutdown."
fi
i=0
for PID in ${PIDS}; do 
    while kill -0 ${PID} &> /dev/null; do 
        if [ "$i" == "300" ]; then
            break;
        fi
        sleep 1
        i=$((i+1))
    done
done

if [ "$i" == "300" ]; then
    echo "Not all instances shut down within 300 seconds"
fi

rm -f ${DIR}/mobile.imn

exit 0
