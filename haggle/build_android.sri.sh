# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Mark-Oliver Stehr (MOS)
#   Hasnain Lakhani (HL)

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
$ANDROID_NDK/ndk-build $1
$ANDROID_NDK/ndk-build clitool $1
cp obj/local/armeabi/clitool ../../bin
$ANDROID_NDK/ndk-build haggletest $1
cp obj/local/armeabi/haggletest ../../bin
$ANDROID_NDK/ndk-build haggleobserver $1
cp obj/local/armeabi/haggleobserver ../../bin
$ANDROID_NDK/ndk-build gps_test gps_person now $1
cp obj/local/armeabi/{gps_test,gps_person,now} ../../bin

$ANDROID_NDK/ndk-build arphelper $1
cp obj/local/armeabi/arphelper ../../bin

# MIGHT HAVE TO UPDATE TARGET ID
android update project -p . --target ${ANDROID_TARGET}

# So that these get into the APK
cp ../../../ccb/py4a/libz.so libs/armeabi/
cp ../../../ccb/py4a/libz.so libs/armeabi-v7a/
cp ../../../ccb/py4a/libsqlite3.so libs/armeabi/
cp ../../../ccb/py4a/libsqlite3.so libs/armeabi-v7a/
cp ../../../ccb/py4a/libgmp.so libs/armeabi/
cp ../../../ccb/py4a/libgmp.so libs/armeabi-v7a/
cp ../../../ccb/py4a/libpbc.so libs/armeabi/
cp ../../../ccb/py4a/libpbc.so libs/armeabi-v7a/
cp ../../../ccb/py4a/libpython2.7.so libs/armeabi/
cp ../../../ccb/py4a/libpython2.7.so libs/armeabi-v7a/

make
popd

pushd android/PhotoShare
$ANDROID_NDK/ndk-build $1

# MIGHT HAVE TO UPDATE TARGET ID
android update project -p . --target ${ANDROID_TARGET}

make
popd
