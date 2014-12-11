#include "org_haggle_Handle.h"
#include "javaclass.h"

#include <stdio.h>
#include <string.h>
#include <libhaggle/haggle.h>
#include <libhaggle/platform.h>
#include <jni.h>

#include "common.h"

LIST(cdlist);

typedef struct callback_data {
        list_t l;
        haggle_handle_t hh;
	int type;
	JNIEnv *env;
        jclass cls;
	jobject obj;
} callback_data_t;

static void callback_data_free(callback_data_t *cd)
{
        if (!cd)
                return;

        (*cd->env)->DeleteGlobalRef(cd->env, cd->obj);
	(*cd->env)->DeleteGlobalRef(cd->env, cd->cls);
        free(cd);
}

static callback_data_t *callback_list_get(haggle_handle_t hh, int type)
{
	list_t *pos;
	
	list_for_each(pos, &cdlist) {
                callback_data_t *e = (callback_data_t *)pos;
                
                if (e->type == type && e->hh == hh)
			return e;
	}
	return NULL;
}


static callback_data_t *callback_list_insert(haggle_handle_t hh, int type, JNIEnv *env, jobject obj)
{
        callback_data_t *cd;
        jclass cls;

        if (callback_list_get(hh, type) != NULL)
                return NULL;

        cd = (callback_data_t *)malloc(sizeof(callback_data_t));

        if (!cd)
                return NULL;

        cd->hh = hh;
	cd->type = type;
	cd->env = env;
        /* Cache the object class */
        cls = (*env)->GetObjectClass(env, obj);
	
	if (!cls) {
		free(cd);
		return NULL;
	}

        /* We must retain these objects */
	cd->obj = (*env)->NewGlobalRef(env, obj);
	cd->cls = (*env)->NewGlobalRef(env, cls);

        list_add(&cd->l, &cdlist);

        return cd;
}

static int callback_list_erase_all_with_handle(haggle_handle_t hh)
{
        int n = 0;
        list_t *pos, *tmp;
        
        list_for_each_safe(pos, tmp, &cdlist) {
                callback_data_t *cd = (callback_data_t *)pos;
                
                if (cd->hh == hh) {
                        list_detach(&cd->l);
			n++;
                        callback_data_free(cd);
                }
        }
        
        haggle_handle_free(hh);

        return n;
}

JNIEXPORT void JNICALL Java_org_haggle_Handle_setDataPath(JNIEnv *env, jclass cls, jstring path)
{
	const char *path_str;
  
        path_str = (*env)->GetStringUTFChars(env, path, 0); 
        
        if (!path_str)
                return;

	libhaggle_platform_set_path(PLATFORM_PATH_APP_DATA, path_str);

        (*env)->ReleaseStringUTFChars(env, path, path_str);

	return;
}

struct event_loop_data {
	int is_async;
	jobject obj;
	jclass cls;
	JNIEnv *env;
	JNIEnv *thr_env;
};

static struct event_loop_data *event_loop_data_create(JNIEnv *env, int is_async, jobject obj)
{
	struct event_loop_data *data;
	jclass cls;

        /* Get the object class */
        cls = (*env)->GetObjectClass(env, obj);
	
	if (!cls)
		return NULL;

	data = (struct event_loop_data *)malloc(sizeof(struct event_loop_data));

	if (!data)
		return NULL;

	memset(data, 0, sizeof(struct event_loop_data));

	data->is_async = is_async;
	data->obj = (*env)->NewGlobalRef(env, obj);
	data->cls = (*env)->NewGlobalRef(env, cls);
	data->env = env;

	return data;
}

static void event_loop_data_free(struct event_loop_data *data)
{
	(*data->env)->DeleteGlobalRef(data->env, data->obj);
	(*data->env)->DeleteGlobalRef(data->env, data->cls);
	free(data);
}

static void on_event_loop_start(void *arg)
{
	struct event_loop_data *data = (struct event_loop_data *)arg;
	jmethodID mid;
	JNIEnv *env = NULL; 	
/* 
   The definition of AttachCurrentThread seems to be different depending
   on platform. We use this define just to avoid compiler warnings.
 */
	if (!data || data->is_async) {
		if ((*jvm)->AttachCurrentThread(jvm, &env, NULL) != JNI_OK) {
			fprintf(stderr, "libhaggle_jni: Could not attach thread\n");
			return;
		}
		if (data) {
			data->thr_env = env;
		}
	}
	
	if (data) {
		mid = (*env)->GetMethodID(env, data->cls, "onEventLoopStart", "()V");
		
		if (mid) {
			(*env)->CallVoidMethod(env, data->obj, mid);
			
			if ((*env)->ExceptionCheck(env)) {
				fprintf(stdout, "An exception occurred when calling onEventLoopStart()\n");
			}
		}
	}
}

static void on_event_loop_stop(void *arg)
{                
	struct event_loop_data *data = (struct event_loop_data *)arg;
	jmethodID mid;
	JNIEnv *env = data ? data->thr_env : NULL;

	if (data) {
		mid = (*env)->GetMethodID(env, data->cls, "onEventLoopStop", "()V");
		
		if (mid) {
			(*env)->CallVoidMethod(env, data->obj, mid);
			
			if ((*env)->ExceptionCheck(env)) {
				fprintf(stdout, "An exception occurred when calling onShutdown()\n");
			}
		}
	}

	if (!data || data->is_async) {
		if ((*jvm)->DetachCurrentThread(jvm) != JNI_OK) {
			fprintf(stderr, "libhaggle_jni: Could not detach thread\n");
		}
	}
	
	if (data) {
		event_loop_data_free(data);
	}
}

static int event_handler(haggle_event_t *e, void *arg)
{
	callback_data_t *cd = (callback_data_t *)arg;
        jmethodID mid = 0;
        JNIEnv *env;
        int ret = 0;
     
        if (!e || !cd)
                return -1;

        env = get_jni_env();

        if (!env)
		return -1;

	if ((*env)->PushLocalFrame(env, 20) < 0) {
		return -1; 
	}

	switch (cd->type) {
                case LIBHAGGLE_EVENT_SHUTDOWN:
                        mid = (*env)->GetMethodID(env, cd->cls, "onShutdown", "(I)V");
                        if (mid) {
                                (*env)->CallVoidMethod(env, cd->obj, mid, (jint)e->shutdown_reason);

				if ((*env)->ExceptionCheck(env)) {
					fprintf(stdout, "An exception occurred when calling onShutdown()\n");
				}
                        }
                        break;
                case LIBHAGGLE_EVENT_NEIGHBOR_UPDATE:
                        mid = (*env)->GetMethodID(env, cd->cls, "onNeighborUpdate", "([Lorg/haggle/Node;)V");
			
                        if (mid) {
				jobjectArray jarr = libhaggle_jni_nodelist_to_node_jobjectArray(env, e->neighbors);
				
				if (!jarr)
					break;

                                (*env)->CallVoidMethod(env, cd->obj, mid, jarr);

				if ((*env)->ExceptionCheck(env)) {
					fprintf(stdout, "An exception occurred when calling onNeighborUpdate()\n");
				}
				
				(*env)->DeleteLocalRef(env, jarr);
                        }
                        break;
                case LIBHAGGLE_EVENT_NEW_DATAOBJECT:                
                        mid = (*env)->GetMethodID(env, cd->cls, "onNewDataObject", "(Lorg/haggle/DataObject;)V");	
                        if (mid) {
				jobject jdobj = java_object_new(env, JCLASS_DATAOBJECT, e->dobj);

				if (!jdobj)
					break;

                                (*env)->CallVoidMethod(env, cd->obj, mid, jdobj);
				 
				if ((*env)->ExceptionCheck(env)) {
					fprintf(stdout, "An exception occurred when calling onNewDataObject()\n");
				}

				(*env)->DeleteLocalRef(env, jdobj);
                        }
                        /* For this event the data object is turned
                         * into a java data object which will be garbage
                         * collected. */
                        e->dobj = NULL;
                        ret = 1;
                        break;
                case LIBHAGGLE_EVENT_INTEREST_LIST:
                        mid = (*env)->GetMethodID(env, cd->cls, "onInterestListUpdate", "([Lorg/haggle/Attribute;)V");	
                        if (mid) {
				jobjectArray jarr = libhaggle_jni_attributelist_to_attribute_jobjectArray(env, e->interests);

				if (!jarr)
					break;

                                (*env)->CallVoidMethod(env, cd->obj, mid, jarr);
				 
				if ((*env)->ExceptionCheck(env)) {
					fprintf(stdout, "An exception occurred when calling onInterestListUpdate()\n");
				}

				(*env)->DeleteLocalRef(env, jarr);
                        }
                        break;
                default:
                        break;
	}

	(*env)->PopLocalFrame(env, NULL);

	return ret;
}

/*
 * Class:     org_haggle_Handle
 * Method:    getHandle
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_getHandle(JNIEnv *env, jobject obj, jstring name)
{
        haggle_handle_t hh;
        const char *str;
        int ret = 0;
       
        str = (*env)->GetStringUTFChars(env, name, 0); 
        
        if (!str)
                return -1;

        ret = haggle_handle_get(str, &hh);

        (*env)->ReleaseStringUTFChars(env, name, str);

        if (ret == HAGGLE_NO_ERROR) {
                set_native_handle(env, JCLASS_HANDLE, obj, hh); 
        }

        return ret;
}

/*
 * Class:     org_haggle_Handle
 * Method:    nativeFree
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_haggle_Handle_nativeFree(JNIEnv *env, jobject obj)
{
        callback_list_erase_all_with_handle((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj));
}

/*
 * Class:     org_haggle_Handle
 * Method:    unregister
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_haggle_Handle_unregister(JNIEnv *env, jobject obj, jstring name)
{
        const char *str;

        str = (*env)->GetStringUTFChars(env, name, 0); 
        
        if (!str)
                return;

        haggle_unregister(str);
        
        (*env)->ReleaseStringUTFChars(env, name, str);
}

/*
 * Class:     org_haggle_Handle
 * Method:    getSessionId
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_getSessionId(JNIEnv *env, jobject obj)
{
        return haggle_handle_get_session_id((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj));
}

/*
 * Class:     org_haggle_Handle
 * Method:    shutdown
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_shutdown(JNIEnv *env, jobject obj)
{
	return haggle_ipc_shutdown((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj));
}

/*
 * Class:     org_haggle_Handle
 * Method:    registerEventInterest
 * Signature: (ILorg/haggle/EventHandler;)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_registerEventInterest(JNIEnv *env, jobject obj, jint type, jobject handler)
{
        callback_data_t *cd;
	haggle_handle_t hh;
        jint res;

        hh = (haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj);
        
        if (!hh)
                return -1;

        cd = callback_list_insert(hh, type, env, handler);
	
	if (!cd)
		return -1;

        res = haggle_ipc_register_event_interest_with_arg(hh, (int)type, event_handler, cd);

	if (res != HAGGLE_NO_ERROR)
                return res;

        return res;
}
/*
 * Class:     org_haggle_Handle
 * Method:    registerInterest
 * Signature: (Lorg/haggle/Attribute;)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_registerInterest(JNIEnv *env, jobject obj, jobject attribute)
{
        haggle_attr_t *attr = (haggle_attr_t *)get_native_handle(env, JCLASS_ATTRIBUTE, attribute);
        
        if (!attr)
                return -1;

        return haggle_ipc_add_application_interest_weighted((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj), haggle_attribute_get_name(attr), haggle_attribute_get_value(attr), haggle_attribute_get_weight(attr));
}

/*
 * Class:     org_haggle_Handle
 * Method:    registerInterests
 * Signature: ([Lorg/haggle/Attribute;)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_registerInterests(JNIEnv *env, jobject obj, jobjectArray attrArr)
{
        haggle_attrlist_t *al;
        int i, ret;

        al = haggle_attributelist_new();
        
        if (!al)
                return -1;

        if ((*env)->GetArrayLength(env, attrArr) == 0)
                return 0;
        
        for (i = 0; i < (*env)->GetArrayLength(env, attrArr); i++) {
		jobject jattr = (*env)->GetObjectArrayElement(env, attrArr, i);
                haggle_attr_t *attr = (haggle_attr_t *)get_native_handle(env, JCLASS_ATTRIBUTE, jattr);
                haggle_attributelist_add_attribute(al, haggle_attribute_copy(attr));
		(*env)->DeleteLocalRef(env, jattr);
        }

        ret = haggle_ipc_add_application_interests((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj), al);

        haggle_attributelist_free(al);

        return ret;
}

/*
 * Class:     org_haggle_Handle
 * Method:    unregisterInterest
 * Signature: (Lorg/haggle/Attribute;)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_unregisterInterest(JNIEnv *env, jobject obj, jobject attribute)
{
        haggle_attr_t *attr = (haggle_attr_t *)get_native_handle(env, JCLASS_ATTRIBUTE, attribute);
        
        if (!attr)
                return -1;

        return haggle_ipc_remove_application_interest((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj), haggle_attribute_get_name(attr), haggle_attribute_get_value(attr));
}

/*
 * Class:     org_haggle_Handle
 * Method:    unregisterInterests
 * Signature: ([Lorg/haggle/Attribute;)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_unregisterInterests(JNIEnv *env, jobject obj, jobjectArray attrArr)
{
        haggle_attrlist_t *al;
        int i, ret;

        al = haggle_attributelist_new();
        
        if (!al)
                return -1;

        if ((*env)->GetArrayLength(env, attrArr) == 0)
                return 0;
        
        for (i = 0; i < (*env)->GetArrayLength(env, attrArr); i++) {
		jobject jattr = (*env)->GetObjectArrayElement(env, attrArr, i);
                haggle_attr_t *attr = (haggle_attr_t *)get_native_handle(env, JCLASS_ATTRIBUTE, jattr);
                haggle_attributelist_add_attribute(al, haggle_attribute_copy(attr));
		(*env)->DeleteLocalRef(env, jattr);
        }

        ret = haggle_ipc_remove_application_interests((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj), al);

        haggle_attributelist_free(al);

        return ret;
}


/*
 * Class:     org_haggle_Handle
 * Method:    getApplicationInterestsAsync
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_getApplicationInterestsAsync(JNIEnv *env, jobject obj)
{
        return (jint)haggle_ipc_get_application_interests_async((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj));
}

/*
 * Class:     org_haggle_Handle
 * Method:    getDataObjectsAsync
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_getDataObjectsAsync(JNIEnv *env, jobject obj)
{
        return (jint)haggle_ipc_get_data_objects_async((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj));
}


/*
 * Class:     org_haggle_Handle
 * Method:    deleteDataObjectById
 * Signature: ([CZ)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_deleteDataObjectById___3CZ(JNIEnv *env, jobject obj, jcharArray idArray, jboolean keep_in_bloomfilter)
{
        unsigned char id[20];
        jchar *carr;
        int i;

        if ((*env)->GetArrayLength(env, idArray) != 20)
                return -1;
        
        carr = (*env)->GetCharArrayElements(env, idArray, NULL);

        if (carr == NULL) {                
                return -1;
        }

        for (i = 0; i < 20; i++) {
                id[i] = (unsigned char)carr[i];
        }
        
        (*env)->ReleaseCharArrayElements(env, idArray, carr, 0);

        return (jint)haggle_ipc_delete_data_object_by_id_bloomfilter((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj), id, (int)(keep_in_bloomfilter == JNI_TRUE));
}

/*
 * Class:     org_haggle_Handle
 * Method:    deleteDataObject
 * Signature: (Lorg/haggle/DataObject;Z)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_deleteDataObject__Lorg_haggle_DataObject_2Z(JNIEnv *env, jobject obj, jobject dobj, jboolean keep_in_bloomfilter)
{
	return (jint)haggle_ipc_delete_data_object_bloomfilter((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj), (haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, dobj), (int)(keep_in_bloomfilter == JNI_TRUE));
}

/*
 * Class:     org_haggle_Handle
 * Method:    deleteDataObjectById
 * Signature: ([C)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_deleteDataObjectById___3C(JNIEnv *env, jobject obj, jcharArray idArray)
{
	return Java_org_haggle_Handle_deleteDataObjectById___3CZ(env, obj, idArray, JNI_FALSE);
}
/*
 * Class:     org_haggle_Handle
 * Method:    deleteDataObject
 * Signature: (Lorg/haggle/DataObject;)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_deleteDataObject__Lorg_haggle_DataObject_2(JNIEnv *env, jobject obj, jobject dobj)
{
	return (jint)haggle_ipc_delete_data_object((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj), (haggle_dobj_t *)get_native_handle(env, JCLASS_DATAOBJECT, dobj));
}

/*
 * Class:     org_haggle_Handle
 * Method:    sendNodeDescription
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_sendNodeDescription(JNIEnv *env, jobject obj)
{
	return (jint)haggle_ipc_send_node_description((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj));
}

/*
 * Class:     org_haggle_Handle
 * Method:    publishDataObject
 * Signature: (Lorg/haggle/DataObject;)I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_publishDataObject(JNIEnv *env, jobject obj, jobject jdObj)
{	
	struct dataobject *dobj;

	dobj = (struct dataobject *)get_native_handle(env, JCLASS_DATAOBJECT, jdObj);
	
	if (!dobj)
		return -1;
	
	return haggle_ipc_publish_dataobject((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj), dobj);
}

/*
 * Class:     org_haggle_Handle
 * Method:    eventLoopRunAsync
 * Signature: (Lorg/haggle/EventHandler;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_Handle_eventLoopRunAsync__Lorg_haggle_EventHandler_2(JNIEnv *env, jobject obj, jobject handler)
{
	haggle_handle_t hh = (haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj);
	struct event_loop_data *data;
	int ret;
	
        if (!hh)
                return JNI_FALSE;
	
	data = event_loop_data_create(env, 1, handler);

	if (!data)
		return JNI_FALSE;

	ret = haggle_event_loop_register_callbacks(hh, on_event_loop_start, on_event_loop_stop, data);
	
        if (ret != HAGGLE_NO_ERROR) {
		event_loop_data_free(data);
		return JNI_FALSE;
	}

	ret = haggle_event_loop_run_async(hh);
	
	if (ret != HAGGLE_NO_ERROR) {
		event_loop_data_free(data);
	}
        return ret == HAGGLE_NO_ERROR ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_haggle_Handle
 * Method:    eventLoopRunAsync
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_Handle_eventLoopRunAsync__(JNIEnv *env, jobject obj)
{       
        haggle_handle_t hh = (haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj);
	int ret;

        if (!hh)
                return JNI_FALSE;

	ret = haggle_event_loop_register_callbacks(hh, on_event_loop_start, on_event_loop_stop, NULL);
	
        if (ret != HAGGLE_NO_ERROR)
		return JNI_FALSE;

	ret = haggle_event_loop_run_async(hh);
	
        return ret == HAGGLE_NO_ERROR ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_haggle_Handle
 * Method:    eventLoopRun
 * Signature: (Lorg/haggle/EventHandler;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_Handle_eventLoopRun__Lorg_haggle_EventHandler_2(JNIEnv *env, jobject obj, jobject handler)
{
	haggle_handle_t hh = (haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj);
	struct event_loop_data *data;
	int ret;
	
	data = event_loop_data_create(env, 1, handler);

	if (!data)
		return JNI_FALSE;
	
	ret = haggle_event_loop_register_callbacks(hh, on_event_loop_start, on_event_loop_stop, data);
	
        if (ret != HAGGLE_NO_ERROR) {
		event_loop_data_free(data);
		return JNI_FALSE;
	}

	ret = haggle_event_loop_run(hh);

	if (ret != HAGGLE_NO_ERROR) {
		event_loop_data_free(data);
	}
	return ret == HAGGLE_NO_ERROR ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_haggle_Handle
 * Method:    eventLoopRun
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_Handle_eventLoopRun__(JNIEnv *env, jobject obj)
{
        return haggle_event_loop_run((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj)) == HAGGLE_NO_ERROR ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_haggle_Handle
 * Method:    eventLoopStop
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_Handle_eventLoopStop(JNIEnv *env, jobject obj)
{
        return haggle_event_loop_stop((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj)) == HAGGLE_NO_ERROR ? JNI_TRUE : JNI_FALSE;
}


/*
 * Class:     org_haggle_Handle
 * Method:    eventLoopIsRunning
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_Handle_eventLoopIsRunning(JNIEnv *env, jobject obj)
{
        return haggle_event_loop_is_running((haggle_handle_t)get_native_handle(env, JCLASS_HANDLE, obj)) == 1 ? JNI_TRUE : JNI_FALSE;
}


/*
 * Class:     org_haggle_Handle
 * Method:    getDaemonPid
 * Signature: ()I
 */
JNIEXPORT jlong JNICALL Java_org_haggle_Handle_getDaemonPid(JNIEnv *env, jclass cls)
{
        unsigned long pid;

	int ret = haggle_daemon_pid(&pid);

        /* Check if Haggle is running */
        if (ret == 1)
                return (jlong)pid;

        return (jlong)ret;
}

static jobject spawn_object;

static int spawn_daemon_callback(unsigned int milliseconds) 
{
        int ret = 0;
        jmethodID mid = 0; 
        JNIEnv *env;
        jclass cls;

        env = get_jni_env();
        
        if (!env)
                return -1;
        
        cls = (*env)->GetObjectClass(env, spawn_object);

	if (!cls) {
		LIBHAGGLE_ERR("Could not get spawn object class\n");
		return -1;
	}
        mid = (*env)->GetMethodID(env, cls, "callback", "(J)I");
	
	(*env)->DeleteLocalRef(env, cls);

        if (mid) {
                ret = (*env)->CallIntMethod(env, spawn_object, mid, (jlong)milliseconds);
						
		if ((*env)->ExceptionCheck(env)) {
			(*env)->DeleteGlobalRef(env, spawn_object);
			spawn_object = NULL;
			return -1;
		}
        } else {
                (*env)->DeleteGlobalRef(env, spawn_object);
		spawn_object = NULL;
                return -1;
        }

        if (milliseconds == 0 || ret == 1 || ret == -1) {
                (*env)->DeleteGlobalRef(env, spawn_object);
		spawn_object = NULL;
        }

        return ret;
}

/*
 * Class:     org_haggle_Handle
 * Method:    spawnDaemon
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_Handle_spawnDaemon__(JNIEnv *env, jclass cls)
{
         return haggle_daemon_spawn(NULL) >= 0 ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_haggle_Handle
 * Method:    spawnDaemon
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_Handle_spawnDaemon__Ljava_lang_String_2(JNIEnv *env, jclass cls, jstring path)
{
        const char *daemonpath;
        jint ret;

        daemonpath = (*env)->GetStringUTFChars(env, path, 0); 
        
        if (!daemonpath)
                return JNI_FALSE;

        ret = haggle_daemon_spawn(daemonpath);

        (*env)->ReleaseStringUTFChars(env, path, daemonpath);

        return ret >= 0 ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_haggle_Handle
 * Method:    spawnDaemon
 * Signature: (Lorg/haggle/LaunchCallback;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_Handle_spawnDaemon__Lorg_haggle_LaunchCallback_2(JNIEnv *env, jclass cls, jobject obj)
{
	if (spawn_object) {
		(*env)->DeleteGlobalRef(env, spawn_object);
	}

        spawn_object = (*env)->NewGlobalRef(env, obj);

	if (!spawn_object) {
		LIBHAGGLE_ERR("Could not get spawn object class\n");
		return JNI_FALSE;
	}
        return haggle_daemon_spawn_with_callback(NULL, spawn_daemon_callback) >= 0 ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_haggle_Handle
 * Method:    spawnDaemon
 * Signature: (Ljava/lang/String;Lorg/haggle/LaunchCallback;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_haggle_Handle_spawnDaemon__Ljava_lang_String_2Lorg_haggle_LaunchCallback_2(JNIEnv *env, jclass cls, jstring path, jobject obj)
{
        const char *daemonpath;
        jint ret;

	if (spawn_object) {
		(*env)->DeleteGlobalRef(env, spawn_object);
	}

        spawn_object = (*env)->NewGlobalRef(env, obj);

	if (!spawn_object)
		return JNI_FALSE;

        daemonpath = (*env)->GetStringUTFChars(env, path, 0); 
        
        if (!daemonpath)
                return JNI_FALSE;

        ret = haggle_daemon_spawn_with_callback(daemonpath, spawn_daemon_callback);

        (*env)->ReleaseStringUTFChars(env, path, daemonpath);

        return ret >= 0 ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_haggle_Handle
 * Method:    getDaemonStatus
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_haggle_Handle_getDaemonStatus(JNIEnv *env, jclass cls)
{
        return haggle_daemon_pid(NULL);
}
