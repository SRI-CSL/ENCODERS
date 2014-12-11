#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

NUM_NODES=$1
ARRAY_STRING=""

if [ $# -lt 1 ]; then
    echo "Need number of nodes to generate security ID table!"
    exit 1
fi

# Prepare the test spec
for i in $(seq $NUM_NODES)
do
    ARRAY_STRING=$ARRAY_STRING$i
    ARRAY_STRING=$ARRAY_STRING","
done

ARRAY_STRING="["${ARRAY_STRING:0:(${#ARRAY_STRING}-1)}"]"

sed -e s/%%NUM_NODES%%/$NUM_NODES/g test_spec.template > test_spec.json
sed -i.bak s/%%ARRAY_STRING%%/$ARRAY_STRING/g test_spec.json
rm test_spec.json.bak

# Clean
rm -f test_list test_list.bk
rm -rf TestOutput
rm -rf Testsuite

# Generate the test
python ../../../studies/framework/lxc/testgenerator/test_generator.py test_spec.json

if [ $? -ne 0 ]; then
    echo "Test generation failed!"
    exit 1
fi

# Prepare the test list
touch test_list
ls -d Testsuite/*/ > test_list

if [ $? -ne 0 ]; then
    echo "Test generation failed!"
    exit 1
fi

# Run the test
../../../studies/framework/lxc/bin/run.sh .

# Create the haggle node ID table
rm -f haggleNodeIDTable.txt
./echo_id_list.sh $NUM_NODES > haggleNodeIDTable.txt

# Copy it where we need
cp haggleNodeIDTable.txt ../../../studies/framework/lxc/testgenerator/haggleNodeIDTable.txt
