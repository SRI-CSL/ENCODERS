APP_PROJECT_PATH := $(shell pwd)
APP_ABI := armeabi armeabi-v7a
APP_MODULES := \
	libhagglekernel_jni

APP_OPTIM=release
APP_BUILD_SCRIPT=jni/Android.mk
APP_STL := gnustl_static
# APP_STL := stlport_static
