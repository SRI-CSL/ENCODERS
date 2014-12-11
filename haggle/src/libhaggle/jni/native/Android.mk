LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

# Do not build for simulator
ifneq ($(TARGET_SIMULATOR),true)

LOCAL_SRC_FILES := \
	javaclass.c \
	common.c \
	org_haggle_Attribute.c \
	org_haggle_DataObject.c \
	org_haggle_Handle.c \
	org_haggle_Interface.c \
	org_haggle_Node.c

LOCAL_C_INCLUDES += \
	$(JNI_H_INCLUDE) \
	$(LOCAL_PATH)/../../include \

EXTRA_DEFINES :=-DOS_ANDROID -DDEBUG
LOCAL_CPPFLAGS += $(EXTRA_DEFINES)
LOCAL_CFLAGS :=-O2 -g $(EXTRA_DEFINES)
LOCAL_LDFLAG :=-L$(TARGET_OUT)

LOCAL_SHARED_LIBRARIES += libdl liblog
LOCAL_STATIC_LIBRARIES = libhaggle libhaggle-xml2
LOCAL_PRELINK_MODULE := false
LOCAL_LDLIBS := -llog

LOCAL_MODULE := libhaggle_jni

include $(BUILD_SHARED_LIBRARY)

endif
