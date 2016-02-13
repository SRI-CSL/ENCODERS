LOCAL_PATH := $(call my-dir)

subdirs := $(addprefix $(APP_PROJECT_PATH)/,$(addsuffix /Android.mk, \
		external/sqlite/dist \
		external/dbus/dbus \
                external/bluetooth/bluez/lib \
		external/zlib \
       ))
subdirs += $(APP_PROJECT_PATH)/external/openssl/Android.mk
#subdirs += $(APP_PROJECT_PATH)/external/icu/icu4c/source/Android.mk
#subdirs += $(APP_PROJECT_PATH)/external/bionic/Android.mk
include jni/pathmap.mk
include $(subdirs)

