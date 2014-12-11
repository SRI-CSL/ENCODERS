#!/bin/sh

# This script pushes built files to the sdcard on the phone, assuming
# that the Haggle build dir is in external/haggle of the Android
# source tree. The 'adb' utility from the Android SDK must
# be in the path.

SCRIPT_DIR=$PWD/`dirname $0`
DEVICE_FILES_DIR=$SCRIPT_DIR/device-files
PUSH_DIR=/sdcard
DATA_DIR=/data/local
ADB=adb
ADB_PARAMS=
ANDROID_DIR=$ANDROID_BUILD_TOP

echo 
echo "NOTICE"
echo "This script is deprecated as Haggle can now be installed as a"
echo "regular Android application bundle (using, e.g., adb). See"
echo "android/README for more information."
echo "Press any key to continue, or ctrl-c to abort"
# Wait for some user input
read

if [ -z $TARGET_PRODUCT ]; then
        echo "There is no TARGET_PRODUCT environment variable set."
	echo "Please make sure that the Android build environment"
	echo "is configured by running \'source build/env-setup-sh\'."
	echo
	exit
fi

# Restart adb with root permissions
adb root

pushd $ANDROID_DIR

if [ ! -d $ANDROID_PRODUCT_OUT ]; then
    echo "Cannot find product directory $ANDROID_PRODUCT_OUT"
    exit
fi

popd
	
echo "Looking for Android devices..."

DEVICES=$(adb devices | awk '{ if (match($2,"device")) print $1}')
NUM_DEVICES=$(echo $DEVICES | awk '{print split($0,a, " ")}')

if [ $NUM_DEVICES -lt 1 ]; then
    echo "There are no Android devices connected to the computer."
    echo "Please connect at least one device before installation can proceed."
    echo
    exit
fi 

echo "$NUM_DEVICES Android devices found."
echo
echo "Assuming android source is found in $ANDROID_DIR"
echo "Please make sure this is correct before proceeding."
echo
echo "Press any key to install Haggle on these devices, or ctrl-c to abort"
echo "Check your device in case you need to allow permissions."
# Wait for some user input
read

pushd $SCRIPT_DIR

for dev in $DEVICES; do
    echo "Installing configuration files onto device $dev"
    $ADB -s $dev push $DEVICE_FILES_DIR/tiwlan.ini $DATA_DIR/
    $ADB -s $dev shell su -c "mkdir /data/haggle"
    $ADB -s $dev shell mkdir /sdcard/PhotoShare
done

FRAMEWORK_PATH_PREFIX="system/framework"
FRAMEWORK_FILES="org.haggle.jar"
FRAMEWORK_PERMISSIONS_FILE="$SCRIPT_DIR/libhaggle/org.haggle.xml"

LIB_PATH_PREFIX="system/lib"
LIBS="libhaggle.so libhaggle_jni.so libhaggle-xml2.so"

BIN_HOST_PREFIX=
BIN_PATH_PREFIX="system/bin"
HAGGLE_BIN="haggle"

APP_PATH_PREFIX="system/app"

pushd $ANDROID_DIR
pushd $ANDROID_PRODUCT_OUT

echo $PWD

function install_file()
{
    local src=$1
    local dir=$2
    local file=`basename $1`

    if [ -z "$3" ]; then
	local perm="755"
	else 
	local perm=$3
    fi

    echo "Installing $file in $dir"

    #$ADB -s $dev push $src $dir/$file
    $ADB -s $dev push $src /sdcard/$file.tmp
    $ADB -s $dev shell su -c "dd if=/sdcard/$file.tmp of=$dir/$file"
    $ADB -s $dev shell rm /sdcard/$file
    $ADB -s $dev shell su -c "chmod $perm $dir/$file"
}

for dev in $DEVICES; do
    echo
    echo "Installing files onto device $dev"

    # Remount /system partition in rw mode
    $ADB -s $dev shell su -c "mount -o remount,rw -t yaffs2 /dev/block/mtdblock3 /system"

    # Enter directory holding unstripped binaries
    pushd symbols

    # Install ad hoc settings script
    adb shell rm /$BIN_PATH_PREFIX/adhoc
    install_file $DEVICE_FILES_DIR/adhoc.sh /$BIN_PATH_PREFIX 755
    
    # Install Haggle binary
    echo
    echo "Installing binaries"
    echo "    $HAGGLE_BIN"
    install_file $BIN_PATH_PREFIX/$HAGGLE_BIN /$BIN_PATH_PREFIX 4775

    echo "    LuckyMe (CLI c-version)"
    install_file $BIN_PATH_PREFIX/luckyme /$BIN_PATH_PREFIX 4775

    echo "    clitool"
    install_file $BIN_PATH_PREFIX/clitool /$BIN_PATH_PREFIX 4775
  
    # Install libraries.
    echo
    echo "Installing library files"
    for file in $LIBS; do
	echo "    $file"
	
	install_file $LIB_PATH_PREFIX/$file /$LIB_PATH_PREFIX 644
    done
    
    # Back to product dir
    popd

    # Install framework files
    #    
    # There are two approaches here: 1) install libraries and framework files on device
    # to be shared by all apps, or, 2) bundle them with each app. The first approach 
    # requires that the framework files are registered with the system,
    # so we need to reboot the device for that. 
    echo
    echo "Installing framework files"
    for file in $FRAMEWORK_FILES; do
	echo "    $file"
	
	install_file $FRAMEWORK_PATH_PREFIX/$file /$FRAMEWORK_PATH_PREFIX 644

	
    done
    install_file $FRAMEWORK_PERMISSIONS_FILE /system/etc/permissions 644
    #echo "Installing applications"
    #echo "    LuckyMe"
    #$ADB -s $dev uninstall org.haggle.LuckyMe
    #$ADB -s $dev install $APP_PATH_PREFIX/LuckyMe.apk

    #echo "    PhotoShare"
    #$ADB -s $dev uninstall org.haggle.PhotoShare
    #$ADB -s $dev install $APP_PATH_PREFIX/PhotoShare.apk

    # Cleanup data folder if any
    $ADB -s $dev shell rm /data/haggle/haggle.db
    $ADB -s $dev shell rm /data/haggle/haggle.log
    $ADB -s $dev shell rm /data/haggle/trace.log
    $ADB -s $dev shell rm /data/haggle/libhaggle.txt
    $ADB -s $dev shell rm /data/haggle/haggle.pid

    # Reset filesystem to read-only
    $ADB -s $dev shell su -c "mount -o remount,ro -t yaffs2 /dev/block/mtdblock3 /system"
    
    # synchronize time
    $ADB -s $dev shell date -s $(date "+%Y%m%d.%H%M%S")
done

popd
popd
popd

echo
echo "NOTE:"
echo "You might have to reboot the device if this is the first time"
echo "the Haggle framework files (org.haggle.jar) are installed."
echo "A reboot is necessary to register the library with the system."
echo "If you are running applications and experience problems, please"
echo "try rebooting the device."
echo
