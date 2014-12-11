#!/system/bin/sh

# This script can be installed on the phone, and used to copy Haggle
# files from an sdcard to the filesystem. Must be run as root on the
# device.

mount -o remount,rw -t yaffs2 /dev/block/mtdblock3 /system

SDCARD=/sdcard
BINDIR=/system/bin
LIBDIR=/system/lib
DATADIR=/data/haggle
FRAMEWORKDIR=/system/framework

#if [ -f $SDCARD/haggle ]; then
    dd if=$SDCARD/haggle of=$BINDIR/haggle
    chmod 4775 $BINDIR/haggle
#fi

#if [ -f $SDCARD/libhaggle.so ]; then
    dd if=$SDCARD/libhaggle.so of=$LIBDIR/libhaggle.so
    chmod 644 $LIBDIR/libhaggle.so
#fi

#if [ -f $SDCARD/libhaggle_jni.so ]; then
    dd if=$SDCARD/libhaggle_jni.so of=$LIBDIR/libhaggle_jni.so
    chmod 644 $LIBDIR/libhaggle_jni.so
#fi

#if [ -f $SDCARD/libhaggle-xml2.so ]; then
    dd if=$SDCARD/libhaggle-xml2.so of=$LIBDIR/libhaggle-xml2.so
    chmod 644 $LIBDIR/libhaggle-xml2.so
#fi

#if [ -f $SDCARD/haggle.jar ]; then
    dd if=$SDCARD/haggle.jar of=$FRAMEWORKDIR/haggle.jar
    chmod 644 $FRAMEWORKDIR/haggle.jar
#fi

