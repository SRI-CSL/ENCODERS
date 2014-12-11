#!/bin/bash

TFILE="/tmp/tempfile1"

sleep 7

rm -f ${TFILE}

TDIR=""

if [ $1 == "n1" ]; then
    haggletest -b 1 pub RoutingType=Direct test1
    haggletest -b 3 pub RoutingType=Flood test1
    sleep 3
    kill -INT $(cat ~/.Haggle/haggle.pid)
    exit 0
elif [ $1 == "n4" ]; then
    sleep 10
    haggletest -s 5 -f ${TFILE} sub test1
fi

COUNT=$(grep "Received" ${TFILE} | wc -l)

touch $2

if [ 3 -eq "${COUNT}" ]; then
    echo "OK" >> $2
fi
