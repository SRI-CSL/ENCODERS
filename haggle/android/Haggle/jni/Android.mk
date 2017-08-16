LOCAL_PATH := $(call my-dir)

subdirs := $(addprefix $(APP_PROJECT_PATH)/../../,$(addsuffix /Android.mk, \
		extlibs/libxml2-2.9.0 \
		src/libhaggle \
		src/libcpphaggle \
		src/hagglekernel \
		src/clitool \
		src/haggletest \
		src/haggleobserver \
		src/arphelper \
		src/repltest \
        ))

EXTRA_LIBS_PATH := $(APP_PROJECT_PATH)/../extlibs/external
EXTRA_DEFINES= -pthread

include $(subdirs)
