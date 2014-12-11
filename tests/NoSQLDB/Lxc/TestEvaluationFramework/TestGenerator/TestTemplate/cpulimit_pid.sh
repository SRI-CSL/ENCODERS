#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Limit the cpu resources on each lxc.
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ "%%cpu_limit%%"  == "none" ]; then
    exit 0
fi

cpulimit -b --pid $1 --limit %%cpu_limit%%

if [ "0" != "$?" ]; then
    echo "Could not cpulimit process: $1." >> $(bash "${DIR}/echo_fail_file.sh")
    exit 1
fi

(sleep %%app_duration%% && (cpulimit -b --pid $1 --limit 100)) & 

exit 0
