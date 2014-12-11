#!/bin/bash

OUTPUTFILE="$1"
NUM_NODES="%%num_nodes%%"
FAILLOG="%%fail_file%%"
EXPECTED_DELIVERED="%%expected_delivered%%"

DO_DELIVERED=$(cat ${OUTPUTFILE} | grep "Received," | wc -l)

if [ "${DO_DELIVERED}" != "${EXPECTED_DELIVERED}" ]; then
    echo "Wrong DO delivered: ${DO_DELIVERED} != ${EXPECTED_DELIVERED}" >> ${FAILLOG}
    exit 1
fi

NUM_DO=$(grep "^Data objects in cache: " ${OUTPUTFILE} | sed 's/.*: \(.*\)/\1/')
NUM_INSERTED=$(grep "^Total data objects inserted: " ${OUTPUTFILE} | sed 's/.*: \(.*\) (.*/\1/')
NUM_EVICTED=$(grep "^Total data objects evicted: " ${OUTPUTFILE} | sed 's/.*: \(.*\) (.*/\1/')
NUM_SMALL=$(grep " 72704 " ${OUTPUTFILE} | wc -l)
NUM_MEDIUM=$(grep " 73728 " ${OUTPUTFILE} | wc -l)

if [ "${NUM_DO}" != "7" ]; then
    echo "${NUM_DO} != 7, fail!"
    exit 1
fi

if [ "${NUM_INSERTED}" != "10" ]; then
    echo "num inserted: ${NUM_INSERTED} != 10"
    exit 1
fi

if [ "${NUM_EVICTED}" != "3" ]; then 
    echo "num evicted: ${NUM_EVICTED} != 3"
    exit 1
fi

if [ "${NUM_SMALL}" != "4" ]; then 
    echo "num large: ${NUM_SMALL} != 4"
    exit 1
fi

if [ "${NUM_MEDIUM}" != "24" ]; then 
    echo "num medium: ${NUM_MEDIUM} != 24"
    exit 1
fi

exit 0
