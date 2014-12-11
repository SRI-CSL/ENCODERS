#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

for TYPE in no-security signatures-static encryption-static encryption signatures
do
  I=1
  for DIR in N*-$TYPE-acm-icn-*
  do
    NEW_NAME="r$I-$DIR"

    mv $DIR $NEW_NAME
    pushd $NEW_NAME &> /dev/null
    mv apps_output "$TYPE.apps_output"
    mv stats.log "$TYPE.stats_log"
    popd &> /dev/null
    ((I++))
  done
done
