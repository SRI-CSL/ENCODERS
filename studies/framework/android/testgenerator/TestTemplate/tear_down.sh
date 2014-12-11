#!/bin/bash

#


# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)

#
# Final shutdown (kill remaining haggle processes)
#
# NOTE: this script is executed by testrunner

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

%%rm_string%%

rm -f ${DIR}/mobile.imn

exit 0
