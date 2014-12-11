LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

# Do not build for simulator
ifneq ($(TARGET_SIMULATOR),true)

LOCAL_SRC_FILES := \
	String.cpp \
	Heap.cpp \
	Thread.cpp \
	Timeval.cpp \
	Watch.cpp \
	Mutex.cpp \
	Condition.cpp \
	Signal.cpp \
	Reference.cpp

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../utils

LOCAL_MODULE := libcpphaggle

LOCAL_DEFINES := \
	-DHAVE_CONFIG \
	-DOS_LINUX \
	-DOS_ANDROID \
	-DDEBUG \
	-DHAVE_EXCEPTION=0 \
	$(EXTRA_DEFINES)

LOCAL_CFLAGS :=-O2 -g $(LOCAL_DEFINES)
LOCAL_CPPFLAGS +=$(LOCAL_DEFINES)

include $(BUILD_STATIC_LIBRARY)

endif
