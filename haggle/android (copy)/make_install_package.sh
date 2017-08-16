#!/bin/bash

THIS_DIR=$PWD
SCRIPT_DIR=`dirname $0`
ANDROID_SRC_DIR=
HAGGLE_SRC_DIR=`basename $THIS_DIR`
HAGGLE_VER="x.y"
HAGGLE_DIR_NAME=haggle

function usage() {
    echo "Usage: $0 ANDROID_SRC_DIR [ PHOTOSHARE_SRC_DIR ]"
    exit;
}

if [ -z $1 ]; then
    usage;
else
    ANDROID_SRC_DIR=$1
    echo "Using $ANDROID_SRC_DIR as Android source directory"
fi

if [ ! -z $2 ]; then
    PHOTOSHARE_SRC_DIR=$2
    echo "Looking for PhotoShare in $PHOTOSHARE_DIR"
fi

if [ ! -f $HAGGLE_SRC_DIR/configure.ac ]; then
    if [ -f $ANDROID_SRC_DIR/external/$HAGGLE_DIR_NAME/configure.ac ]; then
	HAGGLE_SRC_DIR=$ANDROID_SRC_DIR/external/$HAGGLE_DIR_NAME
    fi
fi

if [ ! -f $HAGGLE_SRC_DIR/configure.ac ]; then
    echo "Could not find Haggle source directory"
    exit
fi

HAGGLE_VER=`awk '/AC_INIT/ { l=match($2, /[0-9]/); r=match($2,/\]/); print substr($2,l,r-l) }' $HAGGLE_SRC_DIR/configure.ac`	

PHOTOSHARE_SRC_DIR=$HAGGLE_SRC_DIR/android/PhotoShare
ANDROID_BIN_DIR=$ANDROID_SRC_DIR/out/target/product/passion/system
IWCONFIG_BIN=$ANDROID_BIN_DIR/xbin/iwconfig
IWLIST_BIN=$ANDROID_BIN_DIR/xbin/iwlist
HAGGLE_BIN=$ANDROID_BIN_DIR/bin/haggle
LIBHAGGLE_SO=$ANDROID_BIN_DIR/lib/libhaggle.so
LIBHAGGLE_XML2_SO=$ANDROID_BIN_DIR/lib/libhaggle-xml2.so
LIBHAGGLE_JNI_SO=$ANDROID_BIN_DIR/lib/libhaggle_jni.so
HAGGLE_JAR=$ANDROID_BIN_DIR/framework/org.haggle.jar
PHOTOSHARE_APK=$ANDROID_BIN_DIR/app/PhotoShare.apk
FRAMEWORK_PERMISSIONS_FILE=$HAGGLE_SRC_DIR/android/libhaggle/org.haggle.xml
PACKAGE_DIR_NAME=haggle-$HAGGLE_VER-android
PACKAGE_DIR=/tmp/$PACKAGE_DIR_NAME

if [ ! -f $PHOTOSHARE_APK ]; then
    echo "Could not find PhotoShare android package 'PhotoShare.apk' in '$PHOTOSHARE_SRC_DIR/bin'"
    echo "Make sure the path is correct and that PhotoShare has been compiled with Eclipse"
    echo
    exit
fi

rm -rf $PACKAGE_DIR
mkdir $PACKAGE_DIR
pushd $PACKAGE_DIR

# Copy all the files we need into our package directory
for f in $HAGGLE_BIN $LIBHAGGLE_SO $LIBHAGGLE_XML2_SO $LIBHAGGLE_JNI_SO $HAGGLE_JAR $PHOTOSHARE_APK $IWCONFIG_BIN $IWLIST_BIN $FRAMEWORK_PERMISSIONS_FILE; do
    echo "Including $f in package"

    if [ -f $f ]; then
	cp -f $f .
    fi
done

# Create install script

cat > install.sh <<EOF
#!/bin/bash

if [ -z `which adb` ]; then
    echo "Unable to find the adb command (Android Debug Bridge)."
    echo "You need to install and configure the Android SDK."
    echo "See http://developer.android.com"
    return;
fi

echo "This script will install Haggle and PhotoShare on any Android devices connected to this computer."
echo "Note that the installation will only be successful if you have an Android developer phone,"
echo "or another device that allows root access."
echo

DEVICES=\$(adb devices | awk '{ if (match(\$2,"device")) print \$1}')
NUM_DEVICES=\$(echo \$DEVICES | awk '{print split(\$0,a, " ")}')

if [ \$NUM_DEVICES -lt 1 ]; then
    echo "There are no Android devices connected to the computer."
    echo "Please connect at least one device before installation can proceed."
    echo
    exit
fi 

echo "\$NUM_DEVICES Android devices found."
echo "Press any key to install Haggle and PhotoShare on these devices, or ctrl-c to abort"

# Wait for some user input
read

# Make sure we install with root privileges
adb root

for dev in \$DEVICES; do
    echo "Installing onto device \$dev"
    # Remount /system partition in read/write mode
    adb -s \$dev shell mount -o remount,rw -t yaffs2 /dev/block/mtdblock3 /system

    echo "Installing Haggle binary..."
    adb -s \$dev push haggle /system/bin/haggle
    adb -s \$dev shell chmod 4775 /system/bin/haggle

    echo "Installing libraries..."
    adb -s \$dev push libhaggle.so /system/lib/libhaggle.so
    adb -s \$dev shell chmod 644 /system/lib/libhaggle.so

    adb -s \$dev push libhaggle-xml2.so /system/lib/libhaggle-xml2.so
    adb -s \$dev shell chmod 644 /system/lib/libhaggle-xml2.so

    adb -s \$dev push libhaggle_jni.so /system/lib/libhaggle_jni.so
    adb -s \$dev shell chmod 644 /system/lib/libhaggle_jni.so

    echo "Installing org.haggle.jar"
    adb -s \$dev push org.haggle.jar /system/framework/org.haggle.jar
    adb -s \$dev shell chmod 644 /system/framework/org.haggle.jar

    echo "Installing library permissions file"
    adb -s \$dev push org.haggle.xml /system/etc/permissions/org.haggle.xml

    echo "Installing tools..."
    if [ -f iwconfig ]; then
        adb -s \$dev push iwconfig /system/bin/iwconfig
        adb -s \$dev shell chmod 4775 /system/bin/iwconfig
    fi
    if [ -f iwlist ]; then
        adb -s \$dev push iwlist /system/bin/iwlist
        adb -s \$dev shell chmod 4775 /system/bin/iwlist
    fi

    echo "Uninstalling PhotoShare application..."
    adb -s \$dev uninstall org.haggle.PhotoShare
    echo "Installing PhotoShare application..."
    adb -s \$dev install PhotoShare.apk

    # Reset /system partition to read-only mode
    adb -s \$dev shell mount -o remount,ro -t yaffs2 /dev/block/mtdblock3 /system

    # Create data folder if it does not exist already
    adb -s \$dev shell mkdir /data/haggle &>/dev/null

    # Cleanup data folder if any
    adb -s \$dev shell rm /data/haggle/* &> /dev/null
    
done

    echo "Installation completed."
    echo "Please reboot the device to make sure the haggle libraries are"
    echo "registered with the system."
EOF

chmod +x install.sh

cat > README <<EOF
You need an Android Developer phone, or other Android phone with root access,
in order to install Haggle. You also need to install the Android SDK so that
you have the Android debug bridge (adb) tool.

To install, connect your Android phone to a computer and run ./install.sh

EOF

pushd ..

# Make a tar-ball of everything
tar zcf $THIS_DIR/haggle-$HAGGLE_VER-android-bin.tar.gz $PACKAGE_DIR_NAME

popd
popd
