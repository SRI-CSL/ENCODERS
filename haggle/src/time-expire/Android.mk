LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gps_test.c \
	../utils/thread.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/../utils \
        $(LOCAL_PATH)/../libhaggle/include

LOCAL_SHARED_LIBRARIES := libdl libstdc++

LOCAL_STATIC_LIBRARIES += libhaggle

EXTRA_DEFINES:=-DOS_ANDROID -DOS_LINUX -DDEBUG
LOCAL_CPPFLAGS+=$(EXTRA_DEFINES)
LOCAL_CFLAGS+=-O2 -g $(EXTRA_DEFINES)
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := gps_test

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	time_expire_data.c \
	../utils/thread.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/../utils \
        $(LOCAL_PATH)/../libhaggle/include

LOCAL_SHARED_LIBRARIES := libdl libstdc++

LOCAL_STATIC_LIBRARIES += libhaggle

EXTRA_DEFINES:=-DOS_ANDROID -DOS_LINUX -DDEBUG
LOCAL_CPPFLAGS+=$(EXTRA_DEFINES)
LOCAL_CFLAGS+=-O2 -g $(EXTRA_DEFINES)
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := time_expire_data 

include $(BUILD_EXECUTABLE)


