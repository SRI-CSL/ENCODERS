#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

# A simple validation template / script that checks if a certain
# number of data objects were received.

OUTPUTFILE="$1"
NUM_NODES="%%num_nodes%%"
FAILLOG="%%fail_file%%"
EXPECTED_DELIVERED="6"

DO_DELIVERED=$(cat ${OUTPUTFILE} | grep "Received," | wc -l)

if [ "${DO_DELIVERED}" != "${EXPECTED_DELIVERED}" ]; then
    echo "Wrong DO delivered. Got: ${DO_DELIVERED}, expected: ${EXPECTED_DELIVERED}" >> ${FAILLOG}
    exit 1
fi

SIZE_STR=$(cat ${OUTPUTFILE} | grep "Received," | awk -F, '{print $8;}' | tr -d '\n')
EXPECTED_SIZE_STR="307200030721024204810250241024000"
if [ "${SIZE_STR}" != "${EXPECTED_SIZE_STR}" ]; then
    echo "Wrong size string. Got ${SIZE_STR}, expected: ${EXPECTED_SIZE_STR}" >> ${FAILLOG}
    exit 1
fi

exit 0
