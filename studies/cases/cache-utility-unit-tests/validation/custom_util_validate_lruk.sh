#!/bin/bash

OUTPUTFILE="$1"
NUM_NODES="%%num_nodes%%"
FAILLOG="%%fail_file%%"
EXPECTED_DELIVERED="%%expected_delivered%%"

DO_DELIVERED=$(cat ${OUTPUTFILE} | grep "Received," | wc -l)

HAS_STATS=$(grep -l "^Data objects in cache: " ${OUTPUTFILE} | wc -l)
if [ "${HAS_STATS}" != "1" ]; then
    echo "No stats data object (${HAS_STATS})." >> ${FAILLOG}
    exit 1
fi

if [ "${DO_DELIVERED}" != "${EXPECTED_DELIVERED}" ]; then
    echo "Wrong DO delivered. ${DO_DELIVERED} != ${EXPECTED_DELIVERED}" >> ${FAILLOG}
    exit 1
fi

NUM_DO=$(grep "^Data objects in cache: " ${OUTPUTFILE} | sed 's/.*: \(.*\)/\1/')
NUM_INSERTED=$(grep "^Total data objects inserted: " ${OUTPUTFILE} | sed 's/.*: \(.*\) (.*/\1/')
NUM_EVICTED=$(grep "^Total data objects evicted: " ${OUTPUTFILE} | sed 's/.*: \(.*\) (.*/\1/')
NUM_LARGE=$(grep " 74752 " ${OUTPUTFILE} | wc -l)
NUM_MEDIUM=$(grep " 73728 " ${OUTPUTFILE} | wc -l)
NUM_SMALL=$(grep " 72704 " ${OUTPUTFILE} | wc -l)

if [ "${NUM_DO}" -ne "7" ]; then
    echo "Failed! Saw only ${NUM_DO} out of 7 objects"
    exit 1
fi

if [ "${NUM_INSERTED}" -ne "10" ]; then
    echo "Failed! Saw only ${NUM_INSERTED} inserted out of 10 objects"
    exit 1
fi

if [ "${NUM_EVICTED}" -ne "3" ]; then 
    echo "Failed! Saw only ${NUM_EVICTED} Evicted out of 3 objects"
    exit 1
fi

if [ "${NUM_LARGE}" -ne "12" ]; then 
    echo "Failed! Saw only ${NUM_LARGE} large  out of 12 objects"
    exit 1
fi

if [ "${NUM_MEDIUM}" -ne "12" ]; then 
    echo "Failed! Saw only ${NUM_MEDIUM} medium out of 12 objects"
    exit 1
fi

if [ "${NUM_SMALL}" -ne "4" ]; then 
    echo "Failed! Saw only ${NUM_SMALL} small out of 4 objects"
    exit 1
fi

exit 0
