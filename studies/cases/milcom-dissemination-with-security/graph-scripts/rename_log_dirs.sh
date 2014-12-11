#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

for TYPE in ID MD PR NONE
do
  I=1
  for DIR in diss*$TYPE*
  do
    NEW_NAME="r$I-$DIR"
    mv $DIR $NEW_NAME
    pushd $NEW_NAME
    mv apps_output "$TYPE.apps_output"
    mv stats.log "$TYPE.stats_log"
    popd
    ((I++))
  done
done
