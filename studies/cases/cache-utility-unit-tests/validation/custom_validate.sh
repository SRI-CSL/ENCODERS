#!/bin/bash

OUTPUTFILE="$1"
NUM_NODES="%%num_nodes%%"
FAILLOG="%%fail_file%%"
EXPECTED_DELIVERED="%%expected_delivered%%"

DO_DELIVERED=$(cat ${OUTPUTFILE} | grep "Received," | wc -l)

if [ "${DO_DELIVERED}" != "${EXPECTED_DELIVERED}" ]; then
    echo "Wrong DO delivered. Got: ${DO_DELIVERED}, expected: ${EXPECTED_DELIVERED}" >> ${FAILLOG}
    exit 1
fi

exit 0
