#!/bin/bash

#


# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)


#
# Keep track of haggle metrics throughout the test.
#

USER=$(whoami)
TEST_DIR=$1
NODE_NAME=$2
LOGFILE="observer.log"
ENABLED=%%enable_observer%%
DURATION=%%duration%%

if [ "${ENABLED}" != "true" ]; then
    exit 0
fi

touch ${TEST_DIR}/${LOGFILE}
chmod 777 ${TEST_DIR}/${LOGFILE}

/data/haggleobserver --logfile=${TEST_DIR}/${LOGFILE} --duration=${DURATION} --attribute_name=%%attribute_name%% --attribute_value=%%attribute_value%% >> ${TEST_DIR}/observer_fail.log &
