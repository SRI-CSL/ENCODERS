#!/bin/bash

IFS=' '

if [ ! -f .ndkconfig ]; then
    echo
    echo "Android NDK not found."
    echo
    echo "Please put the path to the Android NDK in a .ndkconfig file"
    echo "in this directory according to the following format:"
    echo "NDK_PATH=<my-home-dir/android-ndk-r5b"
    echo
    echo "For example, issue the following command:"
    echo "$ echo \"NDK_PATH=<my-home-dir/android-ndk-r5b\" > .ndkconfig"
    echo
    exit
fi

source .ndkconfig

NDK_BUILD=$NDK_PATH/ndk-build
NDK_PROJECT_PATH=$PWD
BOARD_HAVE_BLUETOOTH=true
GIT_PROTOCOL=http
ANDROID_SOURCE_HOST=android.googlesource.com
GIT_TAG=android-2.3.7_r1

echo
echo "Tips:"
echo "You can target multiple platforms by passing, e.g.,"
echo "$0 APP_ABI=\"armeabi armeabi-v7a\""
echo
echo "You can also speed up the build with the argument -j<NUM_CPUS+1>"
echo
echo "Press any key to continue or ctrl-c to abort"
echo

read

export NDK_PROJECT_PATH BOARD_HAVE_BLUETOOTH

mkdir -p external

pushd external

# Core utils
if [ ! -d core ]; then
    echo
    echo "Downloading core library"
    git clone $GIT_PROTOCOL://$ANDROID_SOURCE_HOST/platform/system/core.git
    pushd core
    git checkout $GIT_TAG
    popd
    #git clone https://github.com/android/platform_system_core.git core
    sed "s/include \$(BUILD_HOST_STATIC_LIBRARY)//;s/include \$(BUILD_HOST_EXECUTABLE)//;" core/libcutils/Android.mk | awk 'BEGIN {count=0} { if ($0=="LOCAL_MODULE := libcutils") { if (count==1) print "LOCAL_MODULE := libcutils_host"; else if (count==3) print "LOCAL_MODULE := libcutils_static";else print $0; count++} else print $0}'> /tmp/Android.mk
    mv /tmp/Android.mk core/libcutils/Android.mk
fi

# Build D-Bus
if [ ! -d dbus ]; then
    echo
    echo "Downloading D-Bus"
    git clone $GIT_PROTOCOL://$ANDROID_SOURCE_HOST/platform/external/dbus.git
    pushd dbus
    git checkout $GIT_TAG
    popd
    #git archive --format=tar --prefix=core $GIT_TAG  -b $GIT_TAG https://github.com/android/platform_external_dbus.git dbus
    sed "s/LOG_TO_ANDROID_LOGCAT := true/LOG_TO_ANDROID_LOGCAT := false/" dbus/dbus/Android.mk >/tmp/Android.mk
    mv /tmp/Android.mk dbus/dbus/Android.mk
fi

# Build Bluez
if [ ! -d bluetooth/bluez ]; then
    echo
    echo "Downloading Bluez"
    mkdir -p bluetooth
    pushd bluetooth
    git clone $GIT_PROTOCOL://$ANDROID_SOURCE_HOST/platform/external/bluetooth/bluez.git
    pushd bluez
    git checkout $GIT_TAG
    popd
    #git clone -b $GIT_TAG https://github.com/android/platform_external_bluez.git bluez
    popd
fi

# Build Sqlite
if [ ! -d sqlite ]; then
    echo
    echo "Downloading Sqlite"
    git clone $GIT_PROTOCOL://$ANDROID_SOURCE_HOST/platform/external/sqlite.git
    pushd sqlite
    git checkout $GIT_TAG
    popd
    #git clone -b $GIT_TAG https://github.com/android/platform_external_sqlite.git sqlite
    sed "s/include \$(BUILD_HOST_EXECUTABLE)//" sqlite/dist/Android.mk > /tmp/Android.mk
    mv /tmp/Android.mk sqlite/dist/Android.mk
fi

# Build Sqlite
if [ ! -d openssl ]; then
    echo
    echo "Downloading OpenSSL"
    git clone $GIT_PROTOCOL://$ANDROID_SOURCE_HOST/platform/external/openssl.git
    pushd openssl
    git checkout $GIT_TAG
    popd
    #git clone -b $GIT_TAG https://github.com/android/platform_external_openssl.git openssl
    sed "s/include \$(BUILD_HOST_STATIC_LIBRARY)//;s/ifeq (\$(TARGET_ARCH),arm)/ifeq (\$(TARGET_ARCH),arm-disabled)/" openssl/crypto/Android.mk > /tmp/Android.mk
    mv /tmp/Android.mk openssl/crypto/Android.mk
fi

# Build Sqlite
if [ ! -d zlib ]; then
    echo
    echo "Downloading zlib"
    git clone $GIT_PROTOCOL://$ANDROID_SOURCE_HOST/platform/external/zlib.git
    pushd zlib
    git checkout $GIT_TAG
    popd
    #git clone -b $GIT_TAG https://github.com/android/platform_external_zlib.git zlib
    sed "s/include \$(BUILD_HOST_STATIC_LIBRARY)//;s/include \$(BUILD_HOST_EXECUTABLE)//;" zlib/Android.mk | awk 'BEGIN {count=0} { if ($0=="LOCAL_MODULE := libz") { if (count==1) print "LOCAL_MODULE := libz_static"; else print $0; count++} else print $0}'> /tmp/Android.mk
    mv /tmp/Android.mk zlib/Android.mk
fi

pushd

$NDK_BUILD "$@"
