#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Echo the application file path (the bash script that
# each lxc executes after haggle start up).
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

APPFILE="${DIR}/app.sh"

if [ ! -x ${APPFILE} ]; then
    exit 1
fi

echo ${APPFILE}

exit 0
