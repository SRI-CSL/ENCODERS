#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW)

#
# Echo the test output path.
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd . > /dev/null
cd ${DIR}

ODIR=$(dirname "../../../social-other-test/TestOutput/social-other-test")
ONAME=$(basename "../../../social-other-test/TestOutput/social-other-test")

mkdir -p ${ODIR}

cd ${ODIR}
OUTPUT_FILE="$(pwd)/${ONAME}"

popd > /dev/null

echo "${OUTPUT_FILE}"

exit 0
