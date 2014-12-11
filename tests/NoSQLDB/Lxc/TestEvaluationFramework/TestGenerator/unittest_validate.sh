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

PASSED=$(cat ${OUTPUTFILE} | grep "Memory Data Store - Unit tests PASSED" | wc -l)

if [ "${PASSED}" != "1" ]; then
    echo "Unit tests failed: ${PASSED} (${OUTPUTFILE}) != 1" >> ${FAILLOG}
    exit 1
fi

exit 0
