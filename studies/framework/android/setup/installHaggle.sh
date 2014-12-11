#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)
#   Tim McCarthy (TTM)

# Load adb tool
. $ANDROID_TESTRUNNER/adbtool.sh

  BINDIR=$1
  BUILD_NAME=$(basename $BINDIR)
  adb_all "uninstall org.haggle.kernel"
  installAPK "$BINDIR/Haggle-debug.apk"
  adb_all "shell mkdir -p /data/data/org.haggle.kernel/files"

  for device in $(cat /tmp/device_ids.txt); do
    APP_ID=$(adb -s $device shell dumpsys package org.haggle.kernel | grep userId | awk '{split($1,a,"="); print a[2];}')
    adb -s $device shell chown -R $APP_ID /data/data/org.haggle.kernel/files
  done

  #copy haggletest binary
  pushFile $BINDIR/haggletest /data/haggletest
  adb_all "shell chmod u+x /data/haggletest"

  pushFile $BINDIR/haggleobserver /data/haggleobserver
  adb_all "shell chmod u+x /data/haggleobserver"

  echo $BUILD_NAME > /tmp/latest_installed_haggle
