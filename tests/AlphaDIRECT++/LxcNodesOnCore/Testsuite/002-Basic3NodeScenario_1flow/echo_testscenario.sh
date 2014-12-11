#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

NUM_IMN=$(ls ${DIR}/*.imn 2> /dev/null | wc -l)
if [ "${NUM_IMN}" -ne 1 ]; then
    exit 1
fi

echo "$(ls ${DIR}/*.imn | head -1)"

exit 0
