#!/bin/bash


OUTPUT_FILE=$1
OUTPUT_DIR=$( dirname $1 )

#counts, to make sure we have each category done
pushd $OUTPUT_DIR
cd $OUTPUT_DIR

errortn=0
count=`cat n1.haggle.db.hash_sizes.log | wc -l`
if [ "$count" != "7" ]; then
    echo "FAIL! n1 Expected 7, found $count DO's"
fi

count=`cat n2.haggle.db.hash_sizes.log  n3.haggle.db.hash_sizes.log | sort | uniq | wc -l`
if [ "$count" != "10" ]; then
    echo "FAIL! n2 and n3 Expected 10, found $count unique DO's"
    errortn=1
fi

count=`cat n2.haggle.db.hash_sizes.log  n3.haggle.db.hash_sizes.log | wc -l`
if [ "$count" != "10" ]; then
    echo "FAIL! n2 and n3 Expected 10, found $count total DO's"
    errortn=1
fi

count=`cat n4.haggle.db.hash_sizes.log | wc -l`
if [ "$count" != "5" ]; then
    echo "FAIL! n4 Expected 5, found $count DO's"
    errortn=1
fi

count=`cat n5.haggle.db.hash_sizes.log | wc -l`
if [ "$count" != "1" ]; then
    echo "FAIL! n5 Expected 1, found $count DO's"
    errortn=1
fi

count=`cat n6.haggle.db.hash_sizes.log | wc -l`
if [ "$count" != "3" ]; then
    echo "FAIL! n6 Expected 3, found $count DO's"
    errortn=1
fi

exit $errortn

