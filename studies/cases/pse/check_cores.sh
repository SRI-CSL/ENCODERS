#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

for DUMP in `find TestOutput -name cores.tar.gz`
do
rm -rf cores
echo $DUMP
tar xzvf $DUMP
pushd cores
echo "quit" >> cmds
for CORE in `find . -name 'core.*'`
do
    RESULT=`gdb -x cmds ./haggle $CORE 2>/dev/null | grep "Program terminated with signal [a-zA-Z0-9 ,\.]*"`
    echo "$CORE -> $RESULT"
done
popd
done
