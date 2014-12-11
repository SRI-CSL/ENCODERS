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

pushd . > /dev/null
cd ${DIR}

ODIR=$(dirname "%%output_path%%")
ONAME=$(basename "%%output_path%%")

mkdir -p ${ODIR}

cd ${ODIR}
OUTPUT_FILE="$(pwd)/${ONAME}"

popd > /dev/null

echo "${OUTPUT_FILE}"

exit 0
