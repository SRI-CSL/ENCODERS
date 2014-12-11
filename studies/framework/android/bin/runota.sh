#!/bin/bash



# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   James Mathewson (JM)
#   Tim McCarthy (TTM)
#   Hasnain Lakhani (HL)

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

TEST_LIST=$BASEDIR/test_list
OUTPUT_CSV=$BASEDIR/output.csv

$DIR/../testrunner/testrunner.sh -o $TEST_LIST $OUTPUT_CSV
