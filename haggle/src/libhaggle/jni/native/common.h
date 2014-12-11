#ifndef LIBHAGGLE_JNI_COMMON_H
#define LIBHAGGLE_JNI_COMMON_H

#include <libhaggle/haggle.h>
#include <libhaggle/attribute.h>
#include <libhaggle/node.h>
#include <jni.h>

jobjectArray libhaggle_jni_nodelist_to_node_jobjectArray(JNIEnv *env, haggle_nodelist_t *nl);
jobjectArray libhaggle_jni_attributelist_to_attribute_jobjectArray(JNIEnv *env, haggle_attrlist_t *al);
jobjectArray libhaggle_jni_dataobject_to_attribute_jobjectArray(JNIEnv *env, haggle_dobj_t *dobj);

#endif /* LIBHAGGLE_JNI_COMMON_H */
