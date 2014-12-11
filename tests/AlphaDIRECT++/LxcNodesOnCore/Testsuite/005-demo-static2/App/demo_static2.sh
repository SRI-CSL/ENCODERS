#!/bin/bash

TFILE="/tmp/$1_temp_file"
rm -f ${TFILE}

sleep 10

if [ $1 == "n1" ]; then
    haggletest -b 3 pub RoutingType=Direct test1
    haggletest -s 7 -f ${TFILE} sub test2
elif [ $1 == "n5" ]; then
    haggletest -b 3 pub RoutingType=Direct test2
    haggletest -s 7 -f ${TFILE} sub test1
else
    exit 0
fi

COUNT=$(grep "Received" ${TFILE} | wc -l)

touch $2

if [ 3 -eq "${COUNT}" ]; then
    echo "OK" >> $2
fi
