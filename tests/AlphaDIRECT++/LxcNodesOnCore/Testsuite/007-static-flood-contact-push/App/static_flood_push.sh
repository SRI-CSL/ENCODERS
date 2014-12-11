#!/bin/bash

TFILE="/tmp/tempfile1"

sleep 7

rm -f ${TFILE}

TDIR=""

if [ $1 == "n1" ]; then
    haggletest -b 1 pub RoutingType=Flood2 test2
    haggletest -b 2 pub RoutingType=Flood1 test1
    exit 0
elif [ $1 == "n4" ]; then
    haggletest -s 25 -f ${TFILE} sub test1 test2
fi

COUNT=$(grep "Received" ${TFILE} | wc -l)

touch $2

if [ 2 -eq "${COUNT}" ]; then
    echo "OK" >> $2
fi
