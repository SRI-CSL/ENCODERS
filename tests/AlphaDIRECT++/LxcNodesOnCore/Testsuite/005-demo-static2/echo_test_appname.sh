#!/bin/bash

FILENAME="demo_static2.sh"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

APPFILE="${DIR}/App/${FILENAME}"

if [ ! -x ${APPFILE} ]; then
    exit 1
fi

echo ${APPFILE}

exit 0
