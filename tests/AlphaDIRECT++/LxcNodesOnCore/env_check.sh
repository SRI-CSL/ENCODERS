#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ "${USER}" == "root" ]; then
    echo "Cannot run as root."
    exit 1
fi

exit 0
