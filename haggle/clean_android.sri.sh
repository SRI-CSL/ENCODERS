# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Mark-Oliver Stehr (MOS)

#!/bin/bash -e

ANDROID_TARGET="android-10"
        
echo "using ANDROID_TARGET="$ANDROID_TARGET
echo "using ANDROID_NDK="$ANDROID_NDK        
        
rm -rf bin/*

pushd android/extlibs
rm -rf lib/* obj/*
popd

pushd android/Haggle
rm -rf bin/* obj/* lib/*
$ANDROID_NDK/ndk-build clean
make clean
popd

pushd android/PhotoShare
rm -rf bin/* obj/* lib/*
$ANDROID_NDK/ndk-build clean
make clean
popd
