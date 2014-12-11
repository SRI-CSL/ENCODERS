#!/bin/bash


OUTPUT_FILE=$1
OUTPUT_DIR=$( dirname $1 )


cd $OUTPUT_DIR
count=$(grep 11264 n2.haggle.db.hash_sizes.log | wc -l)
if [ "${count}" != 1 ]; then
    echo "node n2 locally cached $count != 1 copies"
    exit 1
fi

echo "Test passed with 100% LOCAL retention rate"
exit 0
