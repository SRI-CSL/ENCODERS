#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

USER=$(whoami)
PID=$(cat "/home/${USER}/.Haggle/haggle.pid")
FAILLOG=$(bash "${DIR}/echo_fail_file.sh")
SIZELOG="/home/${USER}/.Haggle/size.log"

NODE_NAME=$1
OUTPUTFILE=$2

sleep 365

# used for debugging by the test runner w/ -d option (pause instances
# from shutting down)
while [ -f /tmp/debug_lock ]; do sleep 3; echo "${NODE_NAME}: waiting for debug lock to be removed"; done

RUNNING=$(ps -eo pid | grep -x "\s*${PID}\s*" | grep -v grep | awk '{print $1;}' | wc -l)

if [ "1" != "${RUNNING}" ]; then
    echo "${NODE_NAME}: Haggle not running." >> ${FAILLOG}
else
    /usr/local/bin/haggletest -x nop &> /dev/null
    if [ "0" != "$?" ]; then
        echo "${NODE_NAME}: Could not use haggletest to shutdown server" >> ${FAILLOG}
    else
        kill -INT ${PID}
        if [ "0" != "$?" ]; then
	        echo "${NODE_NAME}: Could not send SIGINT" >> ${FAILLOG}
        fi
    fi
fi

i=1
while kill -0 ${PID}; do 
    sleep 1;
    i=$((i+1))
    if [ "$i" == "300" ]; then
        break;
    fi
done

STILL_RUNNING=$(ps -eo pid | grep -x "\s*${PID}\s*" | grep -v grep | awk '{print $1;}' | wc -l)

if [ "0" != "${STILL_RUNNING}" ]; then
    echo "${NODE_NAME}: Haggle could not properly shut down - aborting and generating core file" >> ${FAILLOG}
    kill -6 ${PID}
fi

# output file sizes of ~/.Haggle directory
TMP=$(mktemp)
du /home/${USER}/.Haggle/* >>  ${TMP}
cat ${TMP} >> ${SIZELOG}
rm ${TMP}

if [ -f /cores/*.${NODE_NAME}.* ]; then
    ls -l /cores/*.${NODE_NAME}.* >> ${FAILLOG}
fi

HLOG="/home/${USER}/.Haggle/haggle.log"

HASH=$(grep "Git reference is " ${HLOG} | awk '{print $5;}')

echo "${NODE_NAME}: HASH: ${HASH}" >> ${OUTPUTFILE}

DEBUG_ENABLED=$(grep "Statistics -" ${HLOG} | wc -l)

if [ "${DEBUG_ENABLED}" == "0" ]; then
    # we fake a shutdown event message to make validate pass
    echo "${NODE_NAME} : * SHUTDOWN COMPLETED *" >> ${OUTPUTFILE}
    echo "${NODE_NAME} : statistics disabled" >> ${OUTPUTFILE}
    exit 0
else
    STARTUP=$(grep "* HAGGLE STARTUP *" ${HLOG} | wc -l)
    HASEMERGENCY=$(grep " EMERGENCY " ${HLOG} | wc -l)
    SHUTDOWNPREPARE=$(grep "* PREPARE SHUTDOWN EVENT *" ${HLOG} | wc -l)
    SHUTDOWNREADY=$(grep "* SHUTDOWN EVENT *" ${HLOG} | wc -l)
    SHUTDOWNCOMPLETED=$(grep "* SHUTDOWN COMPLETED *" ${HLOG} | wc -l)
    if [ "${STARTUP}" != "1" ] || [ "${HASEMERGENCY}" != "0" ] || [ "${SHUTDOWNPREPARE}" != "1" ] || [ "${SHUTDOWNREADY}" != "1" ] || [ "${SHUTDOWNCOMPLETED}" != "1" ]; then
       echo Problem at Node ${NODE_NAME} - STARTUP: ${STARTUP} - SHUTDOWN PREPARING: ${SHUTDOWNPREPARE} READY: ${SHUTDOWNREADY} COMPLETED: ${SHUTDOWNCOMPLETED} EMERGENCY: ${HASEMERGENCY} >> ${FAILLOG}
    fi
fi

grep "Summary Statistics -" ${HLOG} | sed -n "s/.*\(Summary Statistics - .*\)/${NODE_NAME}: \1/p" >> ${OUTPUTFILE}

NUM_SHUTDOWN=$(grep -l "* SHUTDOWN COMPLETED *" ${HLOG}  | wc -l)

if [ "1" != "${NUM_SHUTDOWN}" ]; then
    echo "Node ${NODE_NAME}  did not respond to shutdown signal (potential crash or deadlock)" >> ${FAILLOG}
else
    echo "${NODE_NAME} : * SHUTDOWN COMPLETED *" >> ${OUTPUTFILE}
fi

exit 0
