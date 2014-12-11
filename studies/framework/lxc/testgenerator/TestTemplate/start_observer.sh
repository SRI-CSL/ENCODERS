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
NODE_NAME=$1
LOGFILE="/home/${USER}/.Haggle/observer.log"
ENABLED=%%enable_observer%%
DURATION=%%duration%%

if [ "${ENABLED}" != "true" ]; then
    exit 0
fi

haggleobserver --logfile=${LOGFILE} --duration=${DURATION} --attribute_name=%%attribute_name%% --attribute_value=%%attribute_value%% 2>>${LOGFILE}
