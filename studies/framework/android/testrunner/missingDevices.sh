#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

adb devices | grep -e "device$" | grep -v offline | awk '{print $1}' | sort -r | diff /tmp/device_ids.txt -
