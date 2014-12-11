#!/bin/bash

#


# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


#
# Echo the path of the fail file if there is an error to report.
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd . > /dev/null
cd ${DIR}
DEL=0
ODIR=$(dirname "%%fail_file%%")
ONAME=$(basename "%%fail_file%%")
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
