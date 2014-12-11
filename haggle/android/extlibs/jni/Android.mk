LOCAL_PATH := $(call my-dir)

subdirs := $(addprefix $(APP_PROJECT_PATH)/,$(addsuffix /Android.mk, \
		external/sqlite/dist \
		external/dbus/dbus \
                external/bluetooth/bluez/lib \
		external/zlib \
		external/openssl/crypto \
       ))

include jni/pathmap.mk
include $(subdirs)

