#!/bin/bash

adb uninstall org.haggle.kernel
adb install Haggle-debug.apk
adb shell mkdir /data/data/org.haggle.kernel/files
adb push $1 /data/data/org.haggle.kernel/files/config.xml
adb shell am startservice -a android.intent.action.MAIN -n org.haggle.kernel/org.haggle.kernel.Haggle
