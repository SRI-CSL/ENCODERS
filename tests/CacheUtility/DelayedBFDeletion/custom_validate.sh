#!/bin/bash

OUTPUTFILE="test_output.n2"
NUM_NODES="2"
FAILLOG="faillog.n2"
EXPECTED_DELIVERED="4"

DO_DELIVERED=$(cat ${OUTPUTFILE} | grep "Received," | wc -l)

if [ "${DO_DELIVERED}" != "${EXPECTED_DELIVERED}" ]; then
    echo "Wrong DO delivered. Got: ${DO_DELIVERED}, expected: ${EXPECTED_DELIVERED}" >> ${FAILLOG}
    exit 1
else 
    echo "PASSED!"
fi

exit 0
