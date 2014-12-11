#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

for i in $(seq $1)
do
    grep "calcId" TestOutput/*/n$i.haggle.log | grep -oEi "\[[0-9a-f]*\]"
done
