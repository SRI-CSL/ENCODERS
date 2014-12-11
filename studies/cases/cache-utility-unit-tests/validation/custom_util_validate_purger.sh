#!/bin/bash

OUTPUTFILE="$1"
NUM_NODES="%%num_nodes%%"
FAILLOG="%%fail_file%%"
EXPECTED_DELIVERED="%%expected_delivered%%"

DO_DELIVERED=$(cat ${OUTPUTFILE} | grep "Received," | wc -l)

HAS_STATS=$(grep -l "Data objects in cache: " ${OUTPUTFILE} | wc -l)
if [ "${HAS_STATS}" != "1" ]; then
    echo "No stats data object." >> ${FAILLOG}
    exit 1
fi

if [ "${DO_DELIVERED}" != "${EXPECTED_DELIVERED}" ]; then
    echo "Wrong DO delivered. ${DO_DELIVERED} != ${EXPECTED_DELIVERED}" >> ${FAILLOG}
    exit 1
fi

NUM_DO=$(grep "^Data objects in cache: " ${OUTPUTFILE} | sed 's/.*: \(.*\)/\1/')
NUM_INSERTED=$(grep "^Total data objects inserted: " ${OUTPUTFILE} | sed 's/.*: \(.*\) (.*/\1/')
NUM_EVICTED=$(grep "^Total data objects evicted: " ${OUTPUTFILE} | sed 's/.*: \(.*\) (.*/\1/')

if [ "${NUM_DO}" != "5" ]; then
    echo "${NUM_DO} != 5, fail!" >> ${FAILLOG}
    exit 1
fi


if [ "${NUM_INSERTED}" != "10" ]; then
    echo "Incorrect number inserted: ${NUM_INSERTED} != 10" >> ${FAILLOG}
    exit 1
fi

if [ "${NUM_EVICTED}" != "5" ]; then 
    echo "Incorrect number evicted: ${NUM_EVICTED} != 5" >> ${FAILLOG}
    exit 1
fi

#NUM_CUR_DO=$(grep " 78848 " ${OUTPUTFILE} | wc -l) 
#if [ "${NUM_CUR_DO}" != "4" ]; then
#    echo "Missing DO 78848 " >> ${FAILLOG}
#    exit 1
#fi

NUM_CUR_DO=$(grep " 77824 " ${OUTPUTFILE} | wc -l) 
if [ "${NUM_CUR_DO}" != "4" ]; then
    echo "Missing DO 77824" >> ${FAILLOG}
    exit 1
fi

NUM_CUR_DO=$(grep " 74752 " ${OUTPUTFILE} | wc -l) 
if [ "${NUM_CUR_DO}" != "4" ]; then
    echo "Missing DO 74752" >> ${FAILLOG}
    exit 1
fi

NUM_CUR_DO=$(grep " 76800 " ${OUTPUTFILE} | wc -l) 
if [ "${NUM_CUR_DO}" != "4" ]; then
    echo "Missing DO 76800" >> ${FAILLOG}
    exit 1
fi

NUM_CUR_DO=$(grep " 75776 " ${OUTPUTFILE} | wc -l) 
if [ "${NUM_CUR_DO}" != "4" ]; then
    echo "Missing DO 75776" >> ${FAILLOG}
    exit 1
fi

exit 0
