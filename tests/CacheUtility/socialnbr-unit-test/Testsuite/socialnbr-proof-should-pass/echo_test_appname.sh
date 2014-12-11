#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

APPFILE="${DIR}/app.sh"

if [ ! -x ${APPFILE} ]; then
    exit 1
fi

echo ${APPFILE}

exit 0
