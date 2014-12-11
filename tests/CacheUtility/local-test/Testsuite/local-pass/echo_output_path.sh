#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ ! -x "${DIR}/echo_test_appname.sh" ]; then
    exit 1
fi

APPNAME=$(bash "${DIR}/echo_test_appname.sh")

if [ "$?" -ne "0" ]; then
    exit 1
fi

pushd . > /dev/null
cd ${DIR}
DEL=0
ODIR=$(dirname "../../../local-test/TestOutput/local-pass")
ONAME=$(basename "../../../local-test/TestOutput/local-pass")
if [ ! -d ${ODIR} ]; then
    mkdir -p ${ODIR}
    DEL=1
fi

cd ${ODIR}
OUTPUT_FILE="$(pwd)/${ONAME}"

if [ "${DEL}" == "1" ]; then
    rmdir ${ODIR}
fi

popd > /dev/null

echo "${OUTPUT_FILE}"

exit 0
