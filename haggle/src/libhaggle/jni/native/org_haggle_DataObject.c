/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 */

#include "org_haggle_DataObject.h"
#include "javaclass.h"
#include "common.h"

#include <libhaggle/haggle.h>
#include <string.h>

/*
 * Class:     org_haggle_DataObject
 * Method:    newEmpty
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_DataObject_newEmpty(JNIEnv *env, jobject obj)
{
	haggle_dobj_t *dobj;

	dobj = haggle_dataobject_new();
	
	if (!dobj)
		return JNI_FALSE;
	
	set_native_handle(env, JCLASS_DATAOBJECT, obj, dobj);

	return JNI_TRUE;
}

/*
 * Class:     org_haggle_DataObject
 * Method:    newFromFile
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_DataObject_newFromFile(JNIEnv *env, jobject obj, jstring filename)
{
	const char *str;
	haggle_dobj_t *dobj;
	
        str = (*env)->GetStringUTFChars(env, filename, 0); 
        
        if (!str)
                return JNI_FALSE;

	dobj = haggle_dataobject_new_from_file(str);	

        (*env)->ReleaseStringUTFChars(env, filename, str);

	if (!dobj)
		return JNI_FALSE;
	
	set_native_handle(env, JCLASS_DATAOBJECT, obj, dobj);

	return JNI_TRUE;
}

/*
 * Class:     org_haggle_DataObject
 * Method:    newFromBuffer
 * Signature: ([B)Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_DataObject_newFromBuffer(JNIEnv *env, jobject obj, jbyteArray byteArray)
{
	jbyte *buf;
	jsize len;
	haggle_dobj_t *dobj;

	len = (*env)->GetArrayLength(env, byteArray);

	buf = (jbyte *)malloc(len);
	
	if (!buf)
		return JNI_FALSE;

	(*env)->GetByteArrayRegion(env, byteArray, 0, len, buf);
		
	dobj = haggle_dataobject_new_from_buffer((unsigned char *)buf, len);	
	
	free(buf);

   	if (!dobj)
		return JNI_FALSE;
	
	set_native_handle(env, JCLASS_DATAOBJECT, obj, dobj);
        
	return JNI_TRUE;
}

/*
 * Class:     org_haggle_DataObject
 * Method:    nativeFree
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_haggle_DataObject_nativeFree(JNIEnv *env, jobject obj)
{
	haggle_dobj_t *dobj;
	
	dobj = (haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj);
	
	if (!dobj)
		return;

        /* Set the handle to NULL */
	set_native_handle(env, JCLASS_DATAOBJECT, obj, NULL);

	haggle_dataobject_free(dobj);
}

/*
 * Class:     org_haggle_DataObject
 * Method:    addAttribute
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
/*
 * fixed interpretafion of return value from
 * haggle_dataobject_add_attribute_weighted() to be an
 * error code and NOT the number of attributes
 * ttafoni 2013-01-08
 *
*/
JNIEXPORT jboolean JNICALL Java_org_haggle_DataObject_addAttribute(JNIEnv *env, jobject obj, jstring name, jstring value, jlong weight)
{
	const char *namestr, *valuestr;
	haggle_dobj_t *dobj;
        // changed 'ret' data type and added new boolean data type
	int ret;
	jboolean bRv = JNI_FALSE;

	dobj = (haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj);
	
	if (!dobj)
		return JNI_FALSE;
	
        namestr = (*env)->GetStringUTFChars(env, name, 0); 
        
        if (!namestr)
                return JNI_FALSE;
	
        valuestr = (*env)->GetStringUTFChars(env, value, 0); 
        
        if (!valuestr) {
		(*env)->ReleaseStringUTFChars(env, name, namestr);
		return JNI_FALSE;
	}

	ret = haggle_dataobject_add_attribute_weighted(dobj, namestr, valuestr, weight);
	
	(*env)->ReleaseStringUTFChars(env, name, namestr);
	(*env)->ReleaseStringUTFChars(env, value, valuestr);

        /* now if 0(=HAGGLE_NO_ERROR) is returned, the
         * correct boolean is returned
         */
	bRv = ( ret==HAGGLE_NO_ERROR?JNI_TRUE:JNI_FALSE );
	return bRv;
}

/*
 * Class:     org_haggle_DataObject
 * Method:    getAttribute
 * Signature: (Ljava/lang/String;I)Lorg/haggle/Attribute;
 */
JNIEXPORT jobject JNICALL Java_org_haggle_DataObject_getAttribute__Ljava_lang_String_2I(JNIEnv *env, jobject obj, jstring name, jint n)
{
        haggle_attr_t *attr;
	const char *namestr;

        namestr = (*env)->GetStringUTFChars(env, name, 0); 
        
        if (!namestr)
                return NULL;
	
        attr = haggle_dataobject_get_attribute_by_name_n((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj), namestr, n);

	(*env)->ReleaseStringUTFChars(env, name, namestr);

        if (!attr)
                return NULL;

        return java_object_new(env, JCLASS_ATTRIBUTE, haggle_attribute_copy(attr));
}

/*
 * Class:     org_haggle_DataObject
 * Method:    getAttribute
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Lorg/haggle/Attribute;
 */
JNIEXPORT jobject JNICALL Java_org_haggle_DataObject_getAttribute__Ljava_lang_String_2Ljava_lang_String_2(JNIEnv *env, jobject obj, jstring name, jstring value)
{
        haggle_attr_t *attr;
	const char *namestr, *valuestr;

        namestr = (*env)->GetStringUTFChars(env, name, 0); 
        
        if (!namestr)
                return NULL;
	
        valuestr = (*env)->GetStringUTFChars(env, value, 0); 
        
        if (!valuestr) {
		(*env)->ReleaseStringUTFChars(env, name, namestr);
		return NULL;
	}
        attr = haggle_dataobject_get_attribute_by_name_value((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj), namestr, valuestr);

	(*env)->ReleaseStringUTFChars(env, name, namestr);
	(*env)->ReleaseStringUTFChars(env, value, valuestr);

        if (!attr)
                return NULL;

        return java_object_new(env, JCLASS_ATTRIBUTE, haggle_attribute_copy(attr));
}
/*
 * Class:     org_haggle_DataObject
 * Method:    getNumAttributes
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_org_haggle_DataObject_getNumAttributes(JNIEnv *env, jobject obj)
{
        return (jlong)haggle_dataobject_get_num_attributes((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj));
}

/*
 * Class:     org_haggle_DataObject
 * Method:    getAttributes
 * Signature: ()[Lorg/haggle/Attribute;
 */
JNIEXPORT jobjectArray JNICALL Java_org_haggle_DataObject_getAttributes(JNIEnv *env, jobject obj)
{
        return libhaggle_jni_dataobject_to_attribute_jobjectArray(env, (haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj));
}

/*
 * Class:     org_haggle_DataObject
 * Method:    getFilePath
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_haggle_DataObject_getFilePath(JNIEnv *env, jobject obj)
{
        return (*env)->NewStringUTF(env, haggle_dataobject_get_filepath((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj)));
}


/*
 * Class:     org_haggle_DataObject
 * Method:    getFileName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_haggle_DataObject_getFileName(JNIEnv *env, jobject obj)
{
        return (*env)->NewStringUTF(env, haggle_dataobject_get_filename((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj)));
}

/*
 * Class:     org_haggle_DataObject
 * Method:    addPersistentFlag
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_haggle_DataObject_addPersistentFlag(JNIEnv *env, jobject obj)
{
        return (jint)haggle_dataobject_set_flags((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj),DATAOBJECT_FLAG_PERSISTENT);
}

/*
 * Class:     org_haggle_DataObject
 * Method:    addFileHash
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_haggle_DataObject_addFileHash(JNIEnv *env, jobject obj)
{
        return (jint)haggle_dataobject_add_hash((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj));
}


/*
 * Class:     org_haggle_DataObject
 * Method:    setCreateTime
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_DataObject_setCreateTime__JJ(JNIEnv *env, jobject obj, jlong secs, jlong usecs)
{
	struct timeval t = { secs, usecs };

	return (jint)haggle_dataobject_set_createtime((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj), &t);
}


/*
 * Class:     org_haggle_DataObject
 * Method:    setCreateTime
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_haggle_DataObject_setCreateTime__(JNIEnv *env, jobject obj)
{
	return (jint)haggle_dataobject_set_createtime((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj), NULL);
}

/*
 * Class:     org_haggle_DataObject
 * Method:    setThumbnail
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_DataObject_setThumbnail(JNIEnv *env, jobject obj, jbyteArray arr)
{
        jbyte *carr;
        int res;

        carr = (*env)->GetByteArrayElements(env, arr, NULL);

        if (carr == NULL) {
                return -1;
        }
        
        res = haggle_dataobject_set_thumbnail((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj), (char *)carr, (*env)->GetArrayLength(env, arr));

        (*env)->ReleaseByteArrayElements(env, arr, carr, 0);
        
        return 0;
}

/*
 * Class:     org_haggle_DataObject
 * Method:    getThumbnailSize
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_org_haggle_DataObject_getThumbnailSize(JNIEnv *env, jobject obj)
{
        int ret;
        size_t bytes;

        ret = haggle_dataobject_get_thumbnail_size((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj), &bytes);

        if (ret != HAGGLE_NO_ERROR)
                return ret;

        return (jlong)bytes;
}
/*
 * Class:     org_haggle_DataObject
 * Method:    getThumbnail
 * Signature: ([B)J
 */
JNIEXPORT jlong JNICALL Java_org_haggle_DataObject_getThumbnail(JNIEnv *env, jobject obj, jbyteArray arr)
{
        jbyte *carr;
        size_t bytes;
        int ret;
        
        carr = (*env)->GetByteArrayElements(env, arr, NULL);

        if (carr == NULL) {
                return -1;
        }

        bytes = (*env)->GetArrayLength(env, arr);
        
        ret = haggle_dataobject_read_thumbnail((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj), (char *)carr, bytes);

        (*env)->ReleaseByteArrayElements(env, arr, carr, 0);

        if (ret != HAGGLE_NO_ERROR)
                return ret;
        
        return (jlong)bytes;        
}

/*
 * Class:     org_haggle_DataObject
 * Method:    getRaw
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_haggle_DataObject_getRaw(JNIEnv *env, jobject obj)
{
        unsigned char *raw = NULL;
        jbyteArray jbarr;
        size_t len;
	int res;

	res = haggle_dataobject_get_raw_alloc((haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, obj), &raw, &len);
	
	if (res != HAGGLE_NO_ERROR || !raw)
                return NULL;

        jbarr = (*env)->NewByteArray(env, len);

        (*env)->SetByteArrayRegion(env, jbarr, 0, len, (jbyte *)raw);

        free(raw);

        return jbarr;
}
