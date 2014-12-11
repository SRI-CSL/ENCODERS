#include "javaclass.h"

#include <stdint.h>
#include <stdio.h>

/*
  These conversion macros are needed to stop the compiler from
  complaining about converting a 64-bit jlong value to a 32-bit
  pointer value on 32-bit platforms.
 */
#if defined(__LP64__)
#define ptr_to_jlong(p) (p ? (jlong)p : 0)
#define jlong_to_ptr(l) ((void *)((uintptr_t)l))
#else
#define ptr_to_jlong(p) (p ? (jlong)((jint)p) : 0)
#define jlong_to_ptr(l) ((void *) (((uintptr_t)l) & 0xffffffff))
#endif

JavaVM *jvm;

typedef struct java_class {
        jclass cls;
        struct {
                jfieldID native;
        } fid;
        struct {
                jmethodID constructor;
        } mid;
} java_class_t;

static java_class_t jc_dataobject;
static java_class_t jc_node;
static java_class_t jc_attribute;
static java_class_t jc_handle;
static java_class_t jc_interface;

jobject java_object_new(JNIEnv *env, jclass_type_t type, void *native_obj)
{
        jobject obj;
        jclass cls;
        jfieldID fid;

        // Create object without calling constructor.
        // The constructor would call into native methods to create a new native data object.

        switch (type) {
                case JCLASS_DATAOBJECT:
                        fid = jc_dataobject.fid.native;
                        cls = jc_dataobject.cls;
                        break;
                case JCLASS_NODE:
                        fid = jc_node.fid.native;
                        cls = jc_node.cls;
                        break;
                case JCLASS_ATTRIBUTE:
                        fid = jc_attribute.fid.native;
                        cls = jc_attribute.cls;
                        break;
                case JCLASS_HANDLE:
                        fid = jc_handle.fid.native;
                        cls = jc_handle.cls;
                        break;
                case JCLASS_INTERFACE:
                        fid = jc_interface.fid.native;
                        cls = jc_interface.cls;
                        break;
                default:
                        return NULL;
        }

        obj = (*env)->AllocObject(env, cls);

        if (!obj)
                return NULL;
        
        (*env)->SetLongField(env, obj, fid, ptr_to_jlong(native_obj));
	
        return obj;
}

jclass java_object_class(jclass_type_t type)
{
        switch (type) {
                case JCLASS_DATAOBJECT:
                        return jc_dataobject.cls;
                case JCLASS_NODE:
                        return jc_node.cls;
                case JCLASS_ATTRIBUTE:
                        return jc_attribute.cls;
                case JCLASS_HANDLE:
                        return jc_handle.cls;
                case JCLASS_INTERFACE:
                        return jc_interface.cls;
        }
        return 0;
}

jfieldID java_class_fid_native(jclass_type_t type)
{
        switch (type) {
                case JCLASS_DATAOBJECT:
                        return jc_dataobject.fid.native;
                case JCLASS_NODE:
                        return jc_node.fid.native;
                case JCLASS_ATTRIBUTE:
                        return jc_attribute.fid.native;
                case JCLASS_HANDLE:
                        return jc_handle.fid.native;
                case JCLASS_INTERFACE:
                        return jc_interface.fid.native;
        }
        return 0;
}

jmethodID java_class_mid_constructor(jclass_type_t type)
{
        switch (type) {
                case JCLASS_DATAOBJECT:
                        return jc_dataobject.mid.constructor;
                case JCLASS_NODE:
                        return jc_node.mid.constructor;
                case JCLASS_ATTRIBUTE:
                        return jc_attribute.mid.constructor;
                case JCLASS_HANDLE:
                        return jc_handle.mid.constructor;
                case JCLASS_INTERFACE:
                        return jc_interface.mid.constructor;
        }
        return 0;
}

void *get_native_handle(JNIEnv *env, jclass_type_t type, jobject obj)
{
        switch (type) {
                case JCLASS_DATAOBJECT:
                        return jlong_to_ptr((*env)->GetLongField(env, obj, jc_dataobject.fid.native));
                case JCLASS_NODE:
                        return jlong_to_ptr((*env)->GetLongField(env, obj, jc_node.fid.native));
                case JCLASS_ATTRIBUTE:
                        return jlong_to_ptr((*env)->GetLongField(env, obj, jc_attribute.fid.native));
                case JCLASS_HANDLE:
                        return jlong_to_ptr((*env)->GetLongField(env, obj, jc_handle.fid.native));
                case JCLASS_INTERFACE:
                        return jlong_to_ptr((*env)->GetLongField(env, obj, jc_interface.fid.native));
        }
        return NULL;
}

void set_native_handle(JNIEnv *env, jclass_type_t type, jobject obj, void *handle)
{
         switch (type) {
                case JCLASS_DATAOBJECT:
                        return (*env)->SetLongField(env, obj, jc_dataobject.fid.native, ptr_to_jlong(handle));
                case JCLASS_NODE:
                        return (*env)->SetLongField(env, obj, jc_node.fid.native, ptr_to_jlong(handle));
                case JCLASS_ATTRIBUTE:
                        return (*env)->SetLongField(env, obj, jc_attribute.fid.native, ptr_to_jlong(handle));
                case JCLASS_HANDLE:
                        return (*env)->SetLongField(env, obj, jc_handle.fid.native, ptr_to_jlong(handle));
                case JCLASS_INTERFACE:
                        return (*env)->SetLongField(env, obj, jc_interface.fid.native, ptr_to_jlong(handle));
        }
}

JNIEnv *get_jni_env()
{
        JNIEnv *env;
        
        if ((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_4) != JNI_OK) {
                fprintf(stderr, "Could not get JNI env\n");
                return NULL;
        }

        return env;
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
        JNIEnv *env;
	jclass cls;

	/* printf("OnLoad called\n"); */

        jvm = vm;

        if ((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_4) != JNI_OK) {
                fprintf(stderr, "Could not get JNI env in JNI_OnLoad\n");
                return -1;
        }         

        // Get handle info
	cls = (*env)->FindClass(env, "org/haggle/Handle");
	
	if (!cls)
		return -1;

	jc_handle.cls = (*env)->NewGlobalRef(env, cls);

	(*env)->DeleteLocalRef(env, cls);

        if (!jc_handle.cls)
                return -1;

        jc_handle.mid.constructor = (*env)->GetMethodID(env, jc_handle.cls, "<init>", "(Ljava/lang/String;)V");

        if (!jc_handle.mid.constructor)
                return -1;

        jc_handle.fid.native = (*env)->GetFieldID(env, jc_handle.cls, "nativeHandle", "J");

	if (!jc_handle.fid.native)
		return -1;

        // Get interface info
	cls = (*env)->FindClass(env, "org/haggle/Interface");

	if (!cls)
		return -1;
	
	jc_interface.cls = (*env)->NewGlobalRef(env, cls);

	(*env)->DeleteLocalRef(env, cls);

        if (!jc_interface.cls)
                return -1;

        jc_interface.mid.constructor = (*env)->GetMethodID(env, jc_interface.cls, "<init>", "()V");

        if (!jc_interface.mid.constructor)
                return -1;

        jc_interface.fid.native = (*env)->GetFieldID(env, jc_interface.cls, "nativeInterface", "J");

	if (!jc_interface.fid.native)
		return -1;

        // Get data object info
        cls = (*env)->FindClass(env, "org/haggle/DataObject");

	if (!cls)
		return -1;

	jc_dataobject.cls = (*env)->NewGlobalRef(env, cls);

	(*env)->DeleteLocalRef(env, cls);

        if (!jc_dataobject.cls)
                return -1;

        jc_dataobject.mid.constructor = (*env)->GetMethodID(env, jc_dataobject.cls, "<init>", "()V");

        if (!jc_dataobject.mid.constructor)
                return -1;

        jc_dataobject.fid.native = (*env)->GetFieldID(env, jc_dataobject.cls, "nativeDataObject", "J");

	if (!jc_dataobject.fid.native)
		return -1;

        // Get node info
        cls = (*env)->FindClass(env, "org/haggle/Node");

        if (!cls)
                return -1;

	jc_node.cls = (*env)->NewGlobalRef(env, cls);

	(*env)->DeleteLocalRef(env, cls);

	if (!jc_node.cls)
		return -1;

        jc_node.mid.constructor = (*env)->GetMethodID(env, jc_node.cls, "<init>", "()V");

        if (!jc_node.mid.constructor)
                return -1;

        jc_node.fid.native = (*env)->GetFieldID(env, jc_node.cls, "nativeNode", "J");

	if (!jc_node.fid.native)
		return -1;
        
        // Get attribute info
	cls = (*env)->FindClass(env, "org/haggle/Attribute");

	if (!cls)
		return -1;
	
	jc_attribute.cls = (*env)->NewGlobalRef(env, cls);

	(*env)->DeleteLocalRef(env, cls);

        if (!jc_attribute.cls)
                return -1;
	
	jc_attribute.mid.constructor = (*env)->GetMethodID(env, jc_attribute.cls, "<init>", "(Ljava/lang/String;Ljava/lang/String;J)V");
	
        if (!jc_attribute.mid.constructor)
	       return -1;

        jc_attribute.fid.native = (*env)->GetFieldID(env, jc_attribute.cls, "nativeAttribute", "J");

	if (!jc_attribute.fid.native)
		return -1;
	
	return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved)
{
	JNIEnv *env = NULL;
	
        if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_4) != JNI_OK) {
                fprintf(stderr, "Could not get JNI env in JNI_OnUnload\n");
                return;
        }         

	/* printf("JNI_OnUnload: cleaning up global references\n"); */

	if (jc_dataobject.cls) {
		/* printf("Cleaning up data object class reference\n"); */
		(*env)->DeleteGlobalRef(env, jc_dataobject.cls);
	}

	if (jc_interface.cls) {
		/* printf("Cleaning up interface class reference\n"); */
		(*env)->DeleteGlobalRef(env, jc_interface.cls);
	}

	if (jc_attribute.cls) {
		/* printf("Cleaning up attribute class reference\n"); */
		(*env)->DeleteGlobalRef(env, jc_attribute.cls);
	}

	if (jc_node.cls) {
		/* printf("Cleaning up node class reference\n"); */
		(*env)->DeleteGlobalRef(env, jc_node.cls);
	}

	if (jc_handle.cls) {
		/* printf("Cleaning up handle class reference\n"); */
		(*env)->DeleteGlobalRef(env, jc_handle.cls);
	}
}
