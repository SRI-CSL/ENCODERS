APP_PROJECT_PATH := $(shell pwd)
APP_ABI := armeabi armeabi-v7a
APP_MODULES := \
	libz \
	libdbus \
	libbluetooth \
	libcrypto \
	libsqlite \
	

APP_OPTIM=release
APP_CFLAGS +=-Iexternal/core/include
NDK_TOOLCHAIN_VERSION=4.6
APP_STL := gnustl_static
APP_PLATFORM := android-21
#APP_LDFLAGS = --format=binary
APP_BUILD_SCRIPT=jni/Android.mk
