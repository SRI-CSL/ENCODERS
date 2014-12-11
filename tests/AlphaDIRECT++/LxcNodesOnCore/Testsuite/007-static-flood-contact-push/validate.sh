#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ ! -x "${DIR}/echo_output_path.sh" ]; then
    exit 1
fi

OUTPUT_FILE=$(bash "${DIR}/echo_output_path.sh")

if [ "$?" -ne "0" ]; then
    exit 1
fi

if [ ! -r ${OUTPUT_FILE} ]; then
    exit 1
fi

NUM_PASSED=$(grep "OK" ${OUTPUT_FILE} | wc -l)

if [ "${NUM_PASSED}" -ne "1" ]; then
    exit 1
fi

exit 0
