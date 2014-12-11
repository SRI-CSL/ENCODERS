#include "org_haggle_Interface.h"
#include "javaclass.h"

#include <libhaggle/haggle.h>

/*
 * Class:     org_haggle_Interface
 * Method:    nativeFree
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_haggle_Interface_nativeFree(JNIEnv *env, jobject obj)
{
        haggle_interface_free((haggle_interface_t *)get_native_handle(env, JCLASS_INTERFACE, obj));
}
/*
 * Class:     org_haggle_Interface
 * Method:    getType
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Interface_getType(JNIEnv *env, jobject obj)
{
        return haggle_interface_get_type((haggle_interface_t *)get_native_handle(env, JCLASS_INTERFACE, obj));
}

/*
 * Class:     org_haggle_Interface
 * Method:    getStatus
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Interface_getStatus(JNIEnv *env, jobject obj)
{
	return haggle_interface_get_status((haggle_interface_t *)get_native_handle(env, JCLASS_INTERFACE, obj));
}



/*
 * Class:     org_haggle_Interface
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_haggle_Interface_getName(JNIEnv *env, jobject obj)
{
        return (*env)->NewStringUTF(env, haggle_interface_get_name((haggle_interface_t *)get_native_handle(env, JCLASS_INTERFACE, obj)));
}

/*
 * Class:     org_haggle_Interface
 * Method:    getIdentifierString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_haggle_Interface_getIdentifierString(JNIEnv *env, jobject obj)
{
        return (*env)->NewStringUTF(env, haggle_interface_get_identifier_str((haggle_interface_t *)get_native_handle(env, JCLASS_INTERFACE, obj)));
}
