#!/bin/bash
USER=$1
LOGFILE="/home/${USER}/.Haggle/neighbors.log"
PERIOD=%%neighbor_monitor_period_s%%
DURATION=2900
MAXLATENCY_MS=%%neighbor_monitor_max_latency_ms%%
ENABLED=%%enable_neighbor_monitor%%
if [ "${ENABLED}" != "true" ]; then
    exit 0
fi
touch ${LOGFILE}
TT=0
while [ ${TT} -lt ${DURATION} ]; do
    sleep ${PERIOD}
    TT=$((TT + PERIOD))
    NOW=$(date '+%s')
    while read line; do
        NBRIP=$(echo $line | awk '{print $1;}')
        IFACE=$(echo $line | awk '{print $2;}')
        IS_NBR=$(ping -w ${MAXLATENCY_MS} -c 1 ${NBRIP} | grep "64 bytes from" | sed 's/64 bytes from \(.*\):.*/\1/p' | sort | uniq | wc -l)
        if [ "${IS_NBR}" != "1" ]; then
            continue
        fi
        OURIP=$(ifconfig ${IFACE} | grep "inet addr"  | awk '{print $2;}' | sed 's/.*:\(.*\)/\1/p' | uniq)
        echo "[${NOW}, \"${OURIP}\", \"${NBRIP}\", ${PERIOD}, ${MAXLATENCY_MS}]" >> ${LOGFILE}
    done < <(arp -an | sed 's/.*(\(.*\)).* on \(.*\)/\1 \2/p' | sort | uniq)
done
