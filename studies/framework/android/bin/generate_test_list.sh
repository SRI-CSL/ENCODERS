#!/bin/bash



# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   James Mathewson (JM)
#   Hasnain Lakhani (HL)
#   Tim McCarthy (TTM)

if [ $# -lt 1 ]; then
    echo "No case provided, using ."
    CASE="."
else
    CASE=$1
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ "$CASE" == "." ]; then
  BASEDIR=$(pwd)
else
  BASEDIR=$DIR/../../../cases/$CASE
fi

pushd $BASEDIR > /dev/null

touch test_list
ls -d ./AndroidTestsuite/*/ > test_list

if [ -x filter_test_list.sh ]; then
    echo "Test list filter script found, executing..."
    ./filter_test_list.sh
fi

popd > /dev/null
