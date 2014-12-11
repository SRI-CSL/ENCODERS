#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

OUTPUT_FILE=$1

if [ ! -r ${OUTPUT_FILE} ]; then
    echo "Could not read output file." >> ${FAILLOG}
    exit 1
fi

if [ -r "$2" ]; then
    FAILLOG="$2"
else
    FAILLOG=$(bash "${DIR}/echo_fail_file.sh")
fi

NUM_IF=$(grep "eth0" ${OUTPUT_FILE} | wc -l)

if [ "${NUM_IF}" != "6" ]; then
    echo "Did not read enough ifconfig output: ${NUM_IF} != 6" >> ${FAILLOG}
    exit 1
fi

if [ -r ${FAILLOG} ]; then
    NUM_ERRORS=$(wc -l ${FAILLOG} | awk '{print $1;}')
    if [ ${NUM_ERRORS} -ne "0" ]; then
        exit 1
    fi
fi

if [ -x "${DIR}/custom_validate.sh" ]; then
    bash "${DIR}/custom_validate.sh" ${OUTPUT_FILE}
    if [ "$?" != "0" ]; then
        exit 1
    fi
fi

exit 0
