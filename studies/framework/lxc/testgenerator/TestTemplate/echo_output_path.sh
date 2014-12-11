#!/bin/bash

#


# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


#
# Echo the test output path. 
#

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
ODIR=$(dirname "%%output_path%%")
ONAME=$(basename "%%output_path%%")
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
