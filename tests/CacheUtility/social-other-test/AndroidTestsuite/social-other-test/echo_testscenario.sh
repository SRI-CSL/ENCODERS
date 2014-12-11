#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Echo the path of the CORE file. 
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

NUM_IMN=$(ls ${DIR}/*.imn 2> /dev/null | wc -l)
if [ "${NUM_IMN}" -ne 1 ]; then
    exit 1
fi

echo "$(ls ${DIR}/*.imn | head -1)"

exit 0
