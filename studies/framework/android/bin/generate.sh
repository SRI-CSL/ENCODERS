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

TEST_SPEC=$BASEDIR/test_spec.json

python $DIR/../testgenerator/test_generator.py $TEST_SPEC
