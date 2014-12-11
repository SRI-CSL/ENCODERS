LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

EXTRA_LIBS_PATH := external

# Do not build for simulator
ifneq ($(TARGET_SIMULATOR),true)

subdirs := $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, \
                extlibs/libxml2-2.9.0 \
                src/libcpphaggle \
                src/hagglekernel \
                src/libhaggle \
		src/luckyMe \
		src/clitool \
		src/haggletest \
        src/haggleobserver \
		src/arphelper \
		src/repltest \
		android \
        ))

include $(subdirs)

endif
