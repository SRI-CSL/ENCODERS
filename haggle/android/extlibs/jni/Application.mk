APP_PROJECT_PATH := $(shell pwd)
APP_ABI := armeabi armeabi-v7a
APP_MODULES := \
	libz \
	libsqlite \
	libdbus \
	libbluetooth \
	libcrypto \

APP_OPTIM=release
APP_CFLAGS +=-Iexternal/core/include 
APP_BUILD_SCRIPT=jni/Android.mk
