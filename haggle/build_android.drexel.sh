# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Mark-Oliver Stehr (MOS)

#!/bin/bash -e

# execute mkgitref to get latest Haggle check-in hash id
./mkgitref

ANDROID_TARGET="android-10"
        
echo "using ANDROID_TARGET="$ANDROID_TARGET
echo "using ANDROID_NDK="$ANDROID_NDK        
        
pushd android/extlibs
echo "NDK_PATH=$ANDROID_NDK" > .ndkconfig
./build.sh $1
popd

pushd android/Haggle

# android-10 doesn't exist as an NDK target, use android-9 instead
echo "target=android-9" > project.properties

$ANDROID_NDK/ndk-build $1
$ANDROID_NDK/ndk-build clitool $1
cp obj/local/armeabi/clitool ../../bin
$ANDROID_NDK/ndk-build haggletest $1
cp obj/local/armeabi/haggletest ../../bin
$ANDROID_NDK/ndk-build gps_test gps_person now $1
cp obj/local/armeabi/{gps_test,gps_person,now} ../../bin
$ANDROID_NDK/ndk-build test-pub test-sub $1
cp obj/local/armeabi/{test-pub,test-sub} ../../bin

$ANDROID_NDK/ndk-build arphelper $1
cp obj/local/armeabi/arphelper ../../bin

# MIGHT HAVE TO UPDATE TARGET ID
android update project -p . --target ${ANDROID_TARGET}

$ANDROID_NDK/ndk-build clean
$ANDROID_NDK/ndk-build $1
make clean
make
popd
