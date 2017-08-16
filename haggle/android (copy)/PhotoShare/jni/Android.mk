LOCAL_PATH := $(call my-dir)

subdirs := $(addprefix $(APP_PROJECT_PATH)/../../,$(addsuffix /Android.mk, \
		extlibs/libxml2-2.9.0 \
		src/libhaggle \
		src/libhaggle/jni/native \
        ))

EXTRA_LIBS_PATH := $(APP_PROJECT_PATH)/../extlibs/external

include $(subdirs)
