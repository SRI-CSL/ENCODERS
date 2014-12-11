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
NUM_LARGE=$(grep " 81920 " ${OUTPUTFILE} | wc -l)

if [ "${NUM_DO}" != "1" ]; then
    echo "${NUM_DO} != 1, fail!"
    exit 1
fi

if [ "${NUM_INSERTED}" != "1" ]; then
    echo "num inserted: ${NUM_INSERTED} != 1"
    exit 1
fi

if [ "${NUM_EVICTED}" != "0" ]; then 
    echo "num evicted: ${NUM_EVICTED} != 0"
    exit 1
fi

if [ "${NUM_LARGE}" != "4" ]; then 
    echo "num large: ${NUM_LARGE} != 4"
    exit 1
fi

UTIL=$(grep " 81920 " ${OUTPUTFILE} | uniq | awk '{print $5;}')

if [ "${UTIL}" != "0.97" ]; then
    echo "Incorrect utility computation: ${NBR_UTIL} != 0.97"
    exit 1
fi

exit 0
