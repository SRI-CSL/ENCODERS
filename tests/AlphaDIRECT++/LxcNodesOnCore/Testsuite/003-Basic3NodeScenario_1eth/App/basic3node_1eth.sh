#!/bin/bash

T1="/tmp/t1"
T3="/tmp/t2"

sleep 7

rm -f ${T1}
rm -f ${T3}

TDIR=""

if [ $1 == "n1" ]; then
    TDIR="${T1}"
    haggletest -b 3 pub test1
    haggletest -s 5 -f ${TDIR} sub test3
elif [ $1 == "n3" ]; then
    TDIR="${T3}"
    haggletest -b 3 pub test3
    haggletest -s 5 -f ${TDIR} sub test1
fi

COUNT=$(grep "Received" ${TDIR} | wc -l)

touch $2

if [ 3 -eq "${COUNT}" ]; then
    echo "OK" >> $2
fi
