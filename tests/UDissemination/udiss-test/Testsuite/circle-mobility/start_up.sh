#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Generate the .imn at run time (this is to avoid PATH issues).
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

dd if=/dev/urandom of=/tmp/100k_n1 bs=1024 count=100 >& /dev/null
dd if=/dev/urandom of=/tmp/102k_n2 bs=1024 count=102 >& /dev/null


sed "s:%%scen_path%%:${DIR}/mobile.scen:" ${DIR}/mobile.imn.template > ${DIR}/mobile.imn

exit 0
