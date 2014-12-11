#include "common.h"
#include "javaclass.h"

#include <libhaggle/attribute.h>

jobjectArray libhaggle_jni_nodelist_to_node_jobjectArray(JNIEnv *env, haggle_nodelist_t *nl)
{
        jobjectArray nodeArray;
        int i = 0;
	jlong n = haggle_nodelist_size(nl);

	if (n < 0) {
		return NULL;
	}

        nodeArray = (*env)->NewObjectArray(env, n, java_object_class(JCLASS_NODE), NULL);

        while (haggle_nodelist_size(nl)) {
		jobject jnode = java_object_new(env, JCLASS_NODE, haggle_nodelist_pop(nl));
                (*env)->SetObjectArrayElement(env, nodeArray, i++, jnode);
		(*env)->DeleteLocalRef(env, jnode);
        }

        return nodeArray;
}

jobjectArray libhaggle_jni_attributelist_to_attribute_jobjectArray(JNIEnv *env, haggle_attrlist_t *al)
{
        jobjectArray attrArray;
        list_t *pos;
        int i = 0;
        
        attrArray = (*env)->NewObjectArray(env, haggle_attributelist_size(al), java_object_class(JCLASS_ATTRIBUTE), NULL);

	list_for_each(pos, &al->attributes) {
		struct attribute *a = (struct attribute *)pos;
		jobject jattr = java_object_new(env, JCLASS_ATTRIBUTE, haggle_attribute_copy(a));
                (*env)->SetObjectArrayElement(env, attrArray, i++, jattr);
		(*env)->DeleteLocalRef(env, jattr);
        }

        return attrArray;
}

jobjectArray libhaggle_jni_dataobject_to_attribute_jobjectArray(JNIEnv *env, haggle_dobj_t *dobj)
{
        jobjectArray attrArray;
        haggle_attrlist_t *al;
        list_t *pos;

        int i = 0;

        /* This list belongs to the data object and should not be
         * free'd. We need to make copies of the attributes that we
         * add to the Java array.  */
        al = haggle_dataobject_get_attributelist(dobj);
        
        if (!al)
                return NULL;
        
        attrArray = (*env)->NewObjectArray(env, haggle_attributelist_size(al), java_object_class(JCLASS_ATTRIBUTE), NULL);

        list_for_each(pos, &al->attributes) {
                struct attribute *a = (struct attribute *)pos;     
		jobject jattr = java_object_new(env, JCLASS_ATTRIBUTE, haggle_attribute_copy(a));
                (*env)->SetObjectArrayElement(env, attrArray, i++, jattr);
		(*env)->DeleteLocalRef(env, jattr);
        }

        return attrArray;
}

