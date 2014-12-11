#!/bin/bash

TFILE="/tmp/$1_temp_file"
rm -f ${TFILE}

sleep 10

if [ $1 == "n1" ]; then
    haggletest -s 7 -f ${TFILE} sub test 
elif [ $1 == "n2" ]; then
    haggletest -s 7 -f ${TFILE} sub test
elif [ $1 == "n3" ]; then
    haggletest -s 7 -f ${TFILE} sub test
elif [ $1 == "n7" ]; then
    sleep 1
    haggletest -b 3 pub RoutingType=Direct test
    exit 0
elif [ $1 == "n8" ]; then
    sleep 1
    haggletest -b 3 pub RoutingType=Direct test
    exit 0
elif [ $1 == "n9" ]; then
    sleep 1
    haggletest -b 3 pub RoutingType=Direct test
    exit 0
else
    exit 0
fi

COUNT=$(grep "Received" ${TFILE} | wc -l)

touch $2

if [ 9 -eq "${COUNT}" ]; then
    echo "OK" >> $2
fi
