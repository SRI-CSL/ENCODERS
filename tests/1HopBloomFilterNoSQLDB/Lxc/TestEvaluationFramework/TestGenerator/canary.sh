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
EXPECTED_DELIVERED="1"

DO_DELIVERED=$(cat ${OUTPUTFILE} | grep "Received," | wc -l)

if [ "${DO_DELIVERED}" != "${EXPECTED_DELIVERED}" ]; then
    echo "Wrong DO delivered. Got: ${DO_DELIVERED}, expected: ${EXPECTED_DELIVERED}" >> ${FAILLOG}
    exit 1
fi

## n1 

GOT=$(grep "n1: Summary Statistics - InterestManager - Interest Data Objects Created:" ${OUTPUTFILE}  | awk '{print $11;}')
EXPECTED="0"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n1 Interest data objects created did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n1: Summary Statistics - InterestManager - Interest Nodes Created:" ${OUTPUTFILE}  | awk '{print $10;}')
EXPECTED="0"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n1 Interest nodes created did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n1: Summary Statistics - InterestManager - Interest Data Objects Received:" ${OUTPUTFILE}  | awk '{print $11;}')
EXPECTED="2"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n1 Interest interest data objects received did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n1: Summary Statistics - InterestManager - Interest Nodes Received:" ${OUTPUTFILE} | awk '{print $10;}')
EXPECTED="1"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n1 Interest nodes received received did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n1: Summary Statistics - InterestManager - Node Entry: " ${OUTPUTFILE} | wc -l)
EXPECTED="2"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n1 # node descriptions not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

## n2

GOT=$(grep "n2: Summary Statistics - InterestManager - Interest Data Objects Created:" ${OUTPUTFILE}  | awk '{print $11;}')
EXPECTED="0"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n2 Interest data objects created did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n2: Summary Statistics - InterestManager - Interest Nodes Created:" ${OUTPUTFILE} | awk '{print $10;}')
EXPECTED="0"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n2 Interest nodes created did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n2: Summary Statistics - InterestManager - Interest Data Objects Received:" ${OUTPUTFILE}  | awk '{print $11;}')
EXPECTED="2"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n2 Interest interest data objects received did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n2: Summary Statistics - InterestManager - Interest Nodes Received:" ${OUTPUTFILE}  | awk '{print $10;}')
EXPECTED="1"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n2 Interest ndoes received received did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n2: Summary Statistics - InterestManager - Node Entry: " ${OUTPUTFILE} | wc -l)
EXPECTED="3"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n2 # node descriptions not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

## n3

GOT=$(grep "n3: Summary Statistics - InterestManager - Interest Data Objects Created:" ${OUTPUTFILE} | awk '{print $11;}')
EXPECTED="2"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n3 Interest data objects created did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n3: Summary Statistics - InterestManager - Interest Nodes Created:" ${OUTPUTFILE} | awk '{print $10;}')
EXPECTED="1"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n3 Interest nodes created did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n3: Summary Statistics - InterestManager - Interest Data Objects Received:" ${OUTPUTFILE} | awk '{print $11;}')
EXPECTED="0"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n3 Interest interest data objects received did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n3: Summary Statistics - InterestManager - Interest Nodes Received:" ${OUTPUTFILE} | awk '{print $10;}')
EXPECTED="0"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n3 Interest ndoes received received did not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

GOT=$(grep "n3: Summary Statistics - InterestManager - Node Entry: " ${OUTPUTFILE} | wc -l)
EXPECTED="2"
if [ "${GOT}" != "${EXPECTED}" ]; then
    echo "n3 # node descriptions not match: ${GOT} != ${EXPECTED}" >> ${FAILLOG}
    exit 1
fi

exit 0
