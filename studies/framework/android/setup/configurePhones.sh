#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

. $ANDROID_TESTRUNNER/adbtool.sh

./changeHostName.sh
./fix_system_settings.sh

# Install usb tethering app
installAPK "bin/com.saic.tetherusb-1.apk"

# Copy arphelper to /etc/
pushFile "bin/arphelper" "/etc/"

# Set superuser right to /etc/arphelper
adb_all "shell chmod +sx /etc/arphelper"

sleep 5

# install python
# ./installPython.sh
# adb_all "shell am start -a android.intent.action.MAIN -n com.googlecode.python3forandroid/.Python3Main"

# echo ""
# echo "Press the Install button on phones to install Python"
# echo "After Python installation, press ENTER to procceed"

# read A

# # copy file for ccb
# ./copyCCBfiles.sh

# sleep 1

# Reboot to apply heap size and ssh setting
echo ""
echo "Reboot phones to activate settings"
adb_all "shell reboot"
