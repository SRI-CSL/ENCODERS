#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
RX=$(cat r*/$1.stats_log | grep -v "name,date" |  awk -F, '{print $8/1000000.}' | awk -f ${DIR}/stats.awk | awk -F, '{print $1, $3, $4;}')
TX=$(cat r*/$1.stats_log | grep -v "name,date" |  awk -F, '{print $7/1000000.}' | awk -f ${DIR}/stats.awk | awk -F, '{print $1, $3, $4;}')

GP=""
for f in $(ls r*/$1.apps_output); do
    GP="$GP $(cat $f | grep "Received," | egrep -v "1024,?$" | egrep -v "5120,?$" | egrep -v "10240,?$" | awk -F, '{s+=$8;} END { print s/1000000. }' )"
done
GP=$(echo $GP | tr ' ' '\n' | awk -f ${DIR}/stats.awk | awk -F, '{print $1, $3, $4;}')

DO=""
for f in $(ls r*/$1.apps_output); do
    DO="$DO $(cat $f | grep "Received," | egrep -v "1024,\(null\)" | egrep -v "5120,\(null\)" | egrep -v "10240,\(null\)" | wc -l)"
done
DO=$(echo $DO | tr ' ' '\n' | awk -f ${DIR}/stats.awk | awk -F, '{print $1/3590., $3/3590., $4/3590.;}')

ALLDO=$(cat r*/$1.stats_log | grep -v "name,date" |  awk -F, '{print $9/118790.}' | awk -f ${DIR}/stats.awk | awk -F, '{print $1, $3, $4;}')

echo "$RX $TX $GP $DO $ALLDO"
