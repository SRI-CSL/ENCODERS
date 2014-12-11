#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ ! -x "${DIR}/echo_test_appname.sh" ]; then
    exit 1
fi

APPNAME=$(bash "${DIR}/echo_test_appname.sh")

if [ "$?" -ne "0" ]; then
    exit 1
fi

echo "/tmp/hope/"

exit 0
