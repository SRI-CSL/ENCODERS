#!/bin/bash

#replace config.sub and config.guess from http://git.savannah.gnu.org/gitweb/?p=config.git;a=tree
#TODO, confirm the above is nessasary

export BD="$BD"
export NDK="$BD/android-ndk-r8e"

export HOST="arm-linux-androideabi"

#export PLATFORM="$NDK/platforms/android-9/arch-arm"
#export SYSROOT=$PLATFORM
#export TC="$NDK/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86"
#export PATH="$TC/bin:$PATH"

#for some reason configure injects 'unknown' into the host prefix, so we manually select the compiler
#export CC="$HOST-gcc " 

#export CC="$CC $CFLAGS" #configure seems to ignore CFLAGS

export CFLAGS="--sysroot=$NDK/platforms/android-9/arch-arm -I$BD/obj/include -L$BD/obj/lib -L$NDK/platforms/android-9/arch-arm/usr/lib" 
export LDFLAGS="-L$NDK/platforms/android-9/arch-arm/usr/lib -L$BD/obj/lib"

export CC="arm-linux-androideabi-gcc $CFLAGS"

./configure --host=$HOST --target=$HOST --enable-shared --prefix=$BD/obj LDFLAGS="$LDFLAGS" && make && make install
