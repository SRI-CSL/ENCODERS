#ifndef _JAVACLASS_H
#define _JAVACLASS_H

#include <jni.h>

extern JavaVM *jvm;

typedef enum jclass_type {
        JCLASS_DATAOBJECT,
        JCLASS_INTERFACE,
        JCLASS_NODE,
        JCLASS_ATTRIBUTE,
        JCLASS_HANDLE,
} jclass_type_t;

jclass java_object_class(jclass_type_t type);
jfieldID java_class_fid_native(jclass_type_t type);
jmethodID java_class_mid_constructor(jclass_type_t type);

jobject java_object_new(JNIEnv *env, jclass_type_t type, void *native_obj);
void *get_native_handle(JNIEnv *env, jclass_type_t type, jobject obj);
void set_native_handle(JNIEnv *env, jclass_type_t type, jobject obj, void *handle);

JNIEnv *get_jni_env();

#endif /* _JAVACLASS_H */
