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

rm /tmp/data.30k
rm /tmp/data.60k
rm /tmp/data.90k
rm /tmp/data.120k
rm /tmp/data.150k
rm /tmp/data.180k

rm -f ${DIR}/mobile.imn

exit 0
