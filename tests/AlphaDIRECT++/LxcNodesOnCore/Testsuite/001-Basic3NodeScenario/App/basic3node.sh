#!/bin/bash

sleep 7

if [ $1 == "n1" ]; then
    haggletest -b 3 pub test
elif [ $1 == "n3" ]; then
    haggletest -s 5 -f $2 sub test 
fi
