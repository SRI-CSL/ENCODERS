#!/bin/bash



# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
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

rm -f test_list test_list.bk output.csv
rm -rf Testsuite
rm -rf TestOutput

popd > /dev/null
