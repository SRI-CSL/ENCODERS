#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd . > /dev/null
cd ${DIR}
DEL=0
ODIR=$(dirname "/tmp/faillog")
ONAME=$(basename "/tmp/faillog")
if [ ! -d ${ODIR} ]; then
    mkdir -p ${ODIR}
    DEL=1
fi

cd ${ODIR}
FAIL_FILE="$(pwd)/${ONAME}"

if [ "${DEL}" == "1" ]; then
    rmdir ${ODIR}
fi

popd > /dev/null

echo "${FAIL_FILE}"

exit 0
