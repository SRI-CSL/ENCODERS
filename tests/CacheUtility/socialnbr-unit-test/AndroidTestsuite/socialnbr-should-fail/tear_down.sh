#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW)

#
# Final shutdown (kill remaining haggle processes)
#
# NOTE: this script is executed by testrunner

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

rm /tmp/data-a1.10K
rm /tmp/data-a2.10K
rm /tmp/data-a3.10K
rm /tmp/data-b1.10K
rm /tmp/data-b2.10K
rm /tmp/data-b3.10K
rm /tmp/data-c1.10K
rm /tmp/data-c2.10K
rm /tmp/data-c3.10K

rm -f ${DIR}/mobile.imn

exit 0
