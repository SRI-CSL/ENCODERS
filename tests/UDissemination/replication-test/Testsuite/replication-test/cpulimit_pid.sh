#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ "15"  == "none" ]; then
    exit 0
fi

#cpulimit -b --pid $1 --limit 20 

if [ "0" != "$?" ]; then
    echo "Could not cpulimit process: $1." >> $(bash "${DIR}/echo_fail_file.sh")
    exit 1
fi

#(sleep 260 && (cpulimit -b --pid $1 --limit 100)) & 

exit 0
