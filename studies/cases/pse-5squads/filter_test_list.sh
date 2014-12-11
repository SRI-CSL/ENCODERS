#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)
#   Tim McCarthy (TTM)

grep -P -v "^[a-zA-Z0-9-\.\/]*-(NONE|PRIORITY)-5squads-[a-zA-Z0-9-\.]*-APP-UDISS2-[a-zA-Z0-9-\.]*" test_list > test_list_tmp
mv test_list_tmp test_list
rm -f test_list_tmp
