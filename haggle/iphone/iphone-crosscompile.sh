#!/bin/bash

BASEPATH=`dirname $0`

IPHONEOS_VER=3.0
SDKBASE=/Developer/Platforms/iPhoneOS.platform/Developer

if [ ! -z $1 ]; then
    SDKBASE=$1;
fi

echo "This script will configure the Haggle build system to compile against iPhoneOS version $IPHONEOS_VER."
echo "Make sure this version matches the SDKs installed on the host system."
echo "To change the version of iPhoneOS, edit this script."
echo
echo "Press any key to continue, or ctrl-c to cancel."

read

SDKROOT=$SDKBASE/SDKs/iPhoneOS$IPHONEOS_VER.sdk
SDKBIN=$SDKBASE/usr/bin
CC_VER=4.0.1

CC=$SDKBIN/arm-apple-darwin9-gcc-$CC_VER
CXX=$SDKBIN/arm-apple-darwin9-g++-$CC_VER
CPP=$SDKBIN/cpp
CXXCPP=$SDKBIN/cpp
STRIP=$SDKBIN/strip
#LD=$SDKBIN/ld
#LD=$SDKBIN/arm-apple-darwin9-gcc-$CC_VER
NM=$SDKBIN/nm
RANLIB=$SDKBIN/ranlib
AR=$SDKBIN/ar
AS=$SDKBIN/as
LIBTOOL=$SDKBIN/libtool
#MAKE=$SDKBIN/make

#CPPFLAGS="-nostdinc -I$SDKBASE/usr/include -I$SDKROOT/usr/include -I$SDKROOT/usr/lib/gcc/arm-apple-darwin9/$CC_VER/include -iframework$SDKROOT/System/Library/Frameworks -F$SDKROOT/System/Library/Frameworks -DOS_MACOSX_IPHONE"

#CFLAGS="-arch armv6 -pipe -Wno-trigraphs -fpascal-strings -fasm-blocks -O0 -Wreturn-type -Wunused-variable -fmessage-length=0 -fvisibility=hidden -miphoneos-version-min=2.0 -gdwarf-2 -mthumb -isysroot $SDKROOT -L$SDKROOT/usr/lib -DOS_MACOSX_IPHONE"
CFLAGS="-arch armv6 -fmessage-length=0 -pipe -Wno-trigraphs -fpascal-strings -fasm-blocks -O0 -Wreturn-type -Wunused-variable -fomit-frame-pointer -miphoneos-version-min=2.0 -gdwarf-2 -mthumb -fno-common -isysroot $SDKROOT -L$SDKROOT/usr/lib -DOS_MACOSX_IPHONE"
#LDFLAGS="-Z"
#LDFLAGS="$LDFLAGS -F$SDKROOT/System/Library/Frameworks"
#LDFLAGS="$LDFLAGS -L$SDKROOT/usr/lib"
#LDFLAGS="$LDFLAGS -F$SDKROOT/usr/lib"
#LDFLAGS="$LDFLAGS -F$SDKROOT/System/Library/PrivateFrameworks"
#LDFLAGS="$LDFLAGS -bind_at_load"
#LDFLAGS="$LDFLAGS -multiply_defined suppress"
#LDFLAGS="$LDFLAGS -arch armv6"

CXXFLAGS="-arch armv6 -fmessage-length=0 -pipe -Wno-trigraphs -fpascal-strings -fasm-blocks -O0 -Wreturn-type -Wunused-variable -fomit-frame-pointer -miphoneos-version-min=2.0 -gdwarf-2 -mthumb -fno-common -isysroot $SDKROOT -DOS_MACOSX_IPHONE"

PATH=$SDKBIN:$PATH
LIBXML2_INCLUDE_DIR=$SDKROOT/usr/include/libxml2

export PATH CC CXX CPP CXXCPP STRIP LD NM RANLIB AR AS LIBTOOL MAKE CFLAGS LDFLAGS CXXFLAGS CPPFLAGS LIBXML2_INCLUDE_DIR


# Move up to the Haggle root
pushd $BASEPATH
BUILDROOT=`dirname ${PWD}`
pushd ..

if [ ! -f ./configure ]; then
    autoreconf --install --force
fi

echo "Using SDK in $SDKROOT"
echo "Buildroot=$BUILDROOT"

pushd extlibs/openssl-0.9.8j
chmod +x config
# Run config to make sure that we have all symlinks to headers configured
./config shared threads no-asm no-krb5

#Replace Makefile with our own one tuned for iPhone
rm -f Makefile
ln -s Makefile.iphone Makefile
popd

./configure --host=arm-apple-darwin9 --build=i686-apple-darwin9 --enable-debug --enable-debug-leaks --disable-media --disable-bluetooth --prefix=/usr --with-openssl=$BUILDROOT/extlibs/openssl-0.9.8j

popd 
popd 

echo
echo "Now type 'make' to compile"

