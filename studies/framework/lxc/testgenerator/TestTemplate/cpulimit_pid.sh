#!/bin/bash

#


# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


#
# Limit the cpu resources on each lxc.
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ "%%cpu_limit%%"  == "none" ]; then
    exit 0
fi

timeout %%app_duration%%s limitcpu -b --quiet --pid $1 --limit %%cpu_limit%% &

exit 0
