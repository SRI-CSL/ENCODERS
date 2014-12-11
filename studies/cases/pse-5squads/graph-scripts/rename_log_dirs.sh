#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

for SECURITY in security-off security-static security-dynamic
do
for TYPE in NONE PRIORITY-UDISS-SCACHING PRIORITY-UDISS-UCACHING PRIORITY-UDISS PRIORITY
do
for APP in UDISS1 UDISS2
do
  I=1
  for DIR in pse5squads-*-${TYPE}-*APP-${APP}-${SECURITY}-*
  do
    if [ -d "${DIR}" ]; then
    NEW_NAME="r$I-$DIR"
    mv $DIR $NEW_NAME
    pushd $NEW_NAME &> /dev/null
    mv apps_output "${TYPE}_${APP}_${SECURITY}.apps_output"
    mv stats.log "${TYPE}_${APP}_${SECURITY}.stats_log"
    popd &> /dev/null
    ((I++))
    fi
  done
done
done
done
