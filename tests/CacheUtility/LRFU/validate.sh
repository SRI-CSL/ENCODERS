#!/bin/bash

OUTPUT_FILE="val_temp_file"

rm $OUTPUT_FILE

echo "Testing ..."

(./haggletest -w 1 -c sub CacheStrategyUtility=stats) >> ${OUTPUT_FILE} 

NUM_DO=$(grep "^Data objects in cache: " ${OUTPUT_FILE} | sed 's/.*: \(.*\)/\1/')
NUM_INSERTED=$(grep "^Total data objects inserted: " ${OUTPUT_FILE} | sed 's/.*: \(.*\) (.*/\1/')
NUM_EVICTED=$(grep "^Total data objects evicted: " ${OUTPUT_FILE} | sed 's/.*: \(.*\) (.*/\1/')
NUM_LARGE=$(grep " 74752 " ${OUTPUT_FILE} | wc -l)
NUM_MEDIUM=$(grep " 73728 " ${OUTPUT_FILE} | wc -l)
NUM_SMALL=$(grep " 72704 " ${OUTPUT_FILE} | wc -l)

echo "$NUM_DO DO's in cache, evicted $NUM_EVICTED out of $NUM_INSERTED"
if [ "${NUM_DO}" -ne "10" ]; then
   echo "Found $NUM_DO != 10 DO's"
   exit 1
fi

if [ "${NUM_EVICTED}" -ne "40" ]; then 
   echo "Evicted $NUM_EVICTED, expected 40"
   exit 1
fi

if [ "${NUM_LARGE}" -ne "4" ]; then 
   echo "Expected 4, received $NUM_LARGE large objects"
   exit 1
fi

if [ "${NUM_MEDIUM}" -ne "16" ]; then 
   echo "Expected 16, received $NUM_MEDIUM medium objects"
    exit 1
fi

if [ "${NUM_SMALL}" -ne "20" ]; then 
   echo "Expected 20, received $NUM_SMALL small objects"
    exit 1
fi

if [ "${NUM_INSERTED}" -ne "50" ]; then
   echo "Found $NUM_INSERTED DO's, expected 50"
   exit 1
fi

echo "PASS!"
exit 0
