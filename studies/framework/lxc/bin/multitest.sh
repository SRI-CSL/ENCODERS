#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Tim McCarthy (TTM)
#   Hasnain Lakhani (HL)

INPUTFILE=$1
LOGS_DIR=$2

compile_haggle() {
  BRANCH=$1

  # Don't re-compile the most recent branch we compiled
  if [ $BRANCH == "$LAST_COMPILED_BRANCH" ]; then
    echo "Already compiled branch $BRANCH...skipping."
    return 0
  fi

  echo "Compiling branch $BRANCH."

  pushd $HAGGLE_SRC_DIR &> /dev/null
  make clean &> /dev/null
  git checkout $BRANCH &> /dev/null

  if [ $? -ne 0 ]; then
    echo "Could not checkout $BRANCH. Aborting."
    exit 1
  fi

  git pull &> /dev/null
  (./autogen.sh && ./configure && make -j48 && sudo make install) &> /dev/null
  RETVAL=$?
  popd &> /dev/null

  if [ $RETVAL -eq 0 ]; then
    LAST_COMPILED_BRANCH=$BRANCH
  fi

  return $RETVAL
}


test_run() {
  SCRIPT_DIR=$1
  BRANCH=$2
  TEST=$3
  LOGS_DIR=$4

  $SCRIPT_DIR/clean.sh $TEST &> /dev/null
  $SCRIPT_DIR/generate.sh $TEST &> /dev/null
  $SCRIPT_DIR/generate_test_list.sh $TEST &> /dev/null
  $SCRIPT_DIR/run.sh $TEST

  DATE=$(date "+%y%m%d-%H%m")
  HOST=$(hostname)
  USER=$(whoami)

  LOG=$LOGS_DIR/$DATE-$BRANCH-$TEST-$HOST-$USER

  mv $SCRIPT_DIR/../../../cases/$TEST/TestOutput $LOG
}

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BRANCHES=(`awk '{print $1;}' < $INPUTFILE`)
TESTS=(`awk '{print $2;}' < $INPUTFILE`)
NUM_BRANCHES=${#BRANCHES[@]}

for ((i=0; i<${NUM_BRANCHES}; i++));
do
  compile_haggle ${BRANCHES[i]}
  if [ $? -ne 0 ]; then
    echo "Failed to compile branch $BRANCH".
    exit 1
  fi
done

mkdir -p $LOGS_DIR

for ((i=0; i<${NUM_BRANCHES}; i++));
do
  compile_haggle ${BRANCHES[i]}
  test_run $SCRIPT_DIR ${BRANCHES[i]} ${TESTS[i]} $LOGS_DIR
done
