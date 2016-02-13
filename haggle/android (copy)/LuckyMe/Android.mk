LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Build all java files in the src subdirectory
LOCAL_SRC_FILES := $(call all-java-files-under,src)

#LOCAL_JAVA_LIBRARIES := org.haggle
LOCAL_STATIC_JAVA_LIBRARIES := org.haggle

# Enabling the following line will bundle all necessary libraries with the 
# application package
#LOCAL_JNI_SHARED_LIBRARIES += libhaggle_jni libhaggle libhaggle-xml2

# Name of the APK to build
LOCAL_PACKAGE_NAME := LuckyMe

LOCAL_PROGUARD_FLAGS := -include $(LOCAL_PATH)/proguard.cfg

# Tell it to build an APK
include $(BUILD_PACKAGE)

