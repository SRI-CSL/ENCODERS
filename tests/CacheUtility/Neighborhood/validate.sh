#!/bin/bash

OUTPUTFILE="output_file.n2"
NUM_NODES="3"
FAILLOG="faillog"
EXPECTED_DELIVERED="3"

rm -f ${FAILLOG}
DO_DELIVERED=$(cat ${OUTPUTFILE} | grep "Received," | wc -l)

HAS_STATS=$(grep -l "^Data objects in cache: " ${OUTPUTFILE} | wc -l)
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

if [ "${NUM_DO}" != "1" ]; then
    echo "${NUM_DO} != 1, fail!" >> ${FAILLOG}
    exit 1
fi


if [ "${NUM_INSERTED}" != "2" ]; then
    echo "Incorrect number inserted: ${NUM_INSERTED} != 2" >> ${FAILLOG}
    exit 1
fi

if [ "${NUM_EVICTED}" != "1" ]; then 
    echo "Incorrect number evicted: ${NUM_EVICTED} != 1" >> ${FAILLOG}
    exit 1
fi

NBR_UTIL=$(grep " 72704 " ${OUTPUTFILE} | uniq | awk '{print $5;}')

if [ "${NBR_UTIL}" != "0.67" ]; then
    echo "Incorrect utility computation: ${NBR_UTIL} != 0.67"
    exit 1
fi

echo "PASSED!"
exit 0
