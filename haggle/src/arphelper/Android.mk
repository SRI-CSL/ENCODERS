LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# Do not build for simulator
ifneq ($(TARGET_SIMULATOR),true)

#
# arphelper
#
LOCAL_SRC_FILES := \
	arphelper.c \
	../utils/thread.c

LOCAL_C_INCLUDES += 

LOCAL_SHARED_LIBRARIES := libdl libstdc++

LOCAL_STATIC_LIBRARIES += 

EXTRA_DEFINES:=-DOS_ANDROID -DOS_LINUX -DDEBUG
LOCAL_CPPFLAGS+=$(EXTRA_DEFINES)
LOCAL_CFLAGS+=-O2 -g $(EXTRA_DEFINES)
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := arphelper

include $(BUILD_EXECUTABLE)

endif
