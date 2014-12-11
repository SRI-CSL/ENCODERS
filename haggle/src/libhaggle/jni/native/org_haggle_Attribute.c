#include "org_haggle_Attribute.h"
#include "javaclass.h"

#include <libhaggle/haggle.h>
#include <string.h>

/*
 * Class:     org_haggle_Attribute
 * Method:    nativeNew
 * Signature: (Ljava/lang/String;Ljava/lang/String;J)Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_Attribute_nativeNew(JNIEnv *env, jobject obj, jstring name, jstring value, jlong weight)
{
        haggle_attr_t *attr;
        const char *namestr, *valuestr;
       
        namestr = (*env)->GetStringUTFChars(env, name, 0); 
        
        if (!namestr)
                return JNI_FALSE;
         
        valuestr = (*env)->GetStringUTFChars(env, value, 0); 
        
        if (!valuestr) {
                (*env)->ReleaseStringUTFChars(env, name, namestr);
                return JNI_FALSE;
        }
        
        attr = haggle_attribute_new_weighted(namestr, valuestr, weight); 

        if (!attr)
                return JNI_FALSE;
        
        (*env)->ReleaseStringUTFChars(env, name, namestr);
        (*env)->ReleaseStringUTFChars(env, value, valuestr);
        
        set_native_handle(env, JCLASS_ATTRIBUTE, obj, attr);
        
        return JNI_TRUE;
}

/*
 * Class:     org_haggle_Attribute
 * Method:    nativeFree
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_haggle_Attribute_nativeFree(JNIEnv *env, jobject obj)
{
        haggle_attribute_free((haggle_attr_t *)get_native_handle(env, JCLASS_ATTRIBUTE, obj));
}

/*
 * Class:     org_haggle_Attribute
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_haggle_Attribute_getName(JNIEnv *env, jobject obj)
{
        return (*env)->NewStringUTF(env, haggle_attribute_get_name((haggle_attr_t *)get_native_handle(env, JCLASS_ATTRIBUTE, obj)));
}

/*
 * Class:     org_haggle_Attribute
 * Method:    getValue
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_haggle_Attribute_getValue(JNIEnv *env, jobject obj)
{
        return (*env)->NewStringUTF(env, haggle_attribute_get_value((haggle_attr_t *)get_native_handle(env, JCLASS_ATTRIBUTE, obj)));
}

/*
 * Class:     org_haggle_Attribute
 * Method:    getWeight
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_org_haggle_Attribute_getWeight(JNIEnv *env, jobject obj)
{
        return haggle_attribute_get_weight((haggle_attr_t *)get_native_handle(env, JCLASS_ATTRIBUTE, obj));
}
