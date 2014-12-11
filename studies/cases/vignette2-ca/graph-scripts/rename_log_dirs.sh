#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

for TYPE in ca-frag ca-0.2 ca-0.4 ca-0.6 ca-0.8 ca-coding
do
  I=1
  for DIR in haggle-*-${TYPE}-*
  do
    if [ -d "${DIR}" ]; then
    NEW_NAME="r$I-$DIR"
    mv $DIR $NEW_NAME
    pushd $NEW_NAME &> /dev/null
    mv apps_output "${TYPE}.apps_output"
    mv stats.log "${TYPE}.stats_log"
    popd &> /dev/null
    ((I++))
    fi
  done
done
