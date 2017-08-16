/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Joshua Joy (JJ, jjoy)
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

/* Copyright 2008 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *     
 *     http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */ 
#ifndef _LIBHAGGLE_IPC_H_
#define _LIBHAGGLE_IPC_H_

#include "exports.h"
#include "error.h"
#include "dataobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
	The types of events that can be registered.
*/
typedef enum event_type {
	// Shutdown event: haggle is shutting down.
	LIBHAGGLE_EVENT_SHUTDOWN = 0,
	// Neighbor update event.
	LIBHAGGLE_EVENT_NEIGHBOR_UPDATE,
	// New data object event.
	LIBHAGGLE_EVENT_NEW_DATAOBJECT,
	// Interest list
	LIBHAGGLE_EVENT_INTEREST_LIST,
	// The number of possible events
	_LIBHAGGLE_NUM_EVENTS
} haggle_event_type_t;
	
typedef struct event {
	haggle_event_type_t type;
	union {
		struct dataobject *dobj;
		struct attributelist *interests;
		struct nodelist *neighbors;
		unsigned int shutdown_reason;
	};
} haggle_event_t;
	
enum daemon_status {
	HAGGLE_DAEMON_ERROR = HAGGLE_ERROR,
	HAGGLE_DAEMON_NOT_RUNNING = HAGGLE_NO_ERROR,
	HAGGLE_DAEMON_RUNNING = 1,
	HAGGLE_DAEMON_CRASHED = 2
};

typedef struct haggle_handle *haggle_handle_t;

#if defined(OS_WINDOWS_MOBILE)
#define STDCALL __stdcall
#else
#define STDCALL 
#endif

/**
	A haggle event handler. 
	
	An event contains a type and auxiliary data that depends on the event type.
	The auxiliary data will automatically be freed after the execution of the
	event handler. In case the handling application wants to keep, e.g., a
	data object, then the data pointer should be nullified, or the return value
	be 1.
*/
typedef int (STDCALL *haggle_event_handler_t) (haggle_event_t *, void *);
/**

*/
typedef void (STDCALL *haggle_event_loop_start_t) (void *);
typedef void (STDCALL *haggle_event_loop_stop_t) (void *);

/* 
Callback for providing feedback during spawning of Haggle. The unsigned integer passed is the 
total number of milliseconds that have passed since spawning was initiated. The value will be
zero once the daemon has been spawned, and the callback will not be called again.

The return value of the callback can be used to cancel the spawning. Return -1 to cancel, 
any other value will continue the operation.
*/
typedef int (STDCALL *daemon_spawn_callback_t) (unsigned int);

/* Errors */
#define	LIBHAGGLE_ERR_BAD_HANDLE    0x01
#define	LIBHAGGLE_ERR_NOT_CONNECTED 0x02
	
typedef enum control_type {
	CTRL_TYPE_INVALID = -1,
	CTRL_TYPE_REGISTRATION_REQUEST = 0,
	CTRL_TYPE_REGISTRATION_REPLY,
	CTRL_TYPE_DEREGISTRATION_NOTICE,
	CTRL_TYPE_REGISTER_INTEREST,
	CTRL_TYPE_REMOVE_INTEREST,
	CTRL_TYPE_GET_INTERESTS,
	CTRL_TYPE_REGISTER_EVENT_INTEREST,
	CTRL_TYPE_MATCHING_DATAOBJECT,
	CTRL_TYPE_DELETE_DATAOBJECT,
	CTRL_TYPE_GET_DATAOBJECTS,
	CTRL_TYPE_SEND_NODE_DESCRIPTION,
	CTRL_TYPE_SHUTDOWN,
	CTRL_TYPE_EVENT,
	CTRL_TYPE_SET_PARAM,
	CTRL_TYPE_CONFIGURE_SECURITY, // CBMEN, HL
	CTRL_TYPE_DYNAMIC_CONFIGURE,
} control_type_t;
	
#define DATAOBJECT_METADATA_APPLICATION "Application"
#define DATAOBJECT_METADATA_APPLICATION_NAME_PARAM "name"
#define DATAOBJECT_METADATA_APPLICATION_ID_PARAM "id"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL "Control"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_TYPE_PARAM "type"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_MESSAGE "Message"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_DIRECTORY "Directory"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SESSION "Session"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT "Event"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_TYPE_PARAM "type"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST "Interest"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST_NAME_PARAM "name"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST_WEIGHT_PARAM "weight"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_MAX_DATAOBJECTS_IN_MATCH_PARAM "max_data_objects_in_match" // MOS
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_MATCHING_THRESHOLD_PARAM "matching_threshold" // MOS
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_DEREG_DELETE_PARAM "delete_state_on_dereg" // SW
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST "Interest"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST_NAME_PARAM "name"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST_WEIGHT_PARAM "weight"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT "DataObject"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT_ID_PARAM "id"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT_BLOOMFILTER_PARAM "keep_in_bloomfilter"

// CBMEN, HL, Begin
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION "SecurityConfiguration" 

#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLES_METADATA "Roles" 
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_METADATA "Role" 
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_NAME_PARAM "name" 
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_SECRET_PARAM "shared_secret" 

#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODES_METADATA "Nodes"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_METADATA "Node"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_ID_PARAM "id" 
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_SECRET_PARAM "shared_secret" 

#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITIES_METADATA "Authorities"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_METADATA "Authority"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_ID_PARAM "id" 
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_NAME_PARAM "name" 

#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_CERTIFY_PARAM "certify"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_CERTIFY_TRUE_PARAM "true"

#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_METADATA "Attribute"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_NAME_PARAM "name"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_ENCRYPTION_PARAM "encryption"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_DECRYPTION_PARAM "decryption"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_AUTHORIZED_PARAM "true"


#define DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION "ObserverConfiguration"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_NOTIFICATION_INTERVAL "NotificationInterval"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_NODE_METRICS "ObserveNodeMetrics"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_INTERFACES "ObserveInterfaces"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_CERTIFICATES "ObserveCertificates"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_NODE_DESCRIPTION "ObserveNodeDescription"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_NODE_STORE "ObserveNodeStore"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_PROTOCOLS "ObserveProtocols"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_ROUTING_TABLE "ObserveRoutingTable"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_CACHE_STRATEGY "ObserveCacheStrategy"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_DATA_STORE_DUMP "ObserveDataStoreDump"
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_DYNAMIC_CONFIGURATION "DynamicConfiguration"
// CBMEN, HL, End

/* Attribute name definitions */
#define HAGGLE_ATTR_CONTROL_NAME "HaggleIPC"  // <-- all control messages should have at this one.

/* Defines whether to expect a reply in response to a sent data object */
#define IO_NO_REPLY                -2
#define IO_REPLY_BLOCK             -1
#define IO_REPLY_NON_BLOCK          0

/* IPC API functions */

/**
   \defgroup IPC Inter Process Communication 

	GENERAL NOTES:

	Most IPC functions return a value indicating success or failure. This value only 
	indicates if the operation was successful within "libhaggle" -- it does not say
	anything about the success of the operation in the Haggle daemon.

	Some operations will trigger a response from Haggle in the form of a control 
	data object. In that case, the application will be notified asynchronously through
	its event handlers. Any success of failure of the operations previously initiated
	may be indicated in those messages.
*/

/*@{*/

/**
	Returns (if possible) a handle that can be used to communicate with haggle
	given a unique application name.
	
	Fills in the given haggle handle iff successful. Otherwise leaves the handle
	unmodified.

	@returns zero if successful, or an error code.
	
	Some specific error codes:
		HAGGLE_REGISTRATION_ERROR - Unable to establish contact with haggle
		HAGGLE_BUSY_ERROR - Haggle reported that there was already an 
			application with that name.
*/

HAGGLE_API int haggle_handle_get(const char *name, haggle_handle_t *handle);

/**
	Relinquishes a handle used to communicate with haggle.
	
	The handle will be invalid after calling this function.
*/
HAGGLE_API void haggle_handle_free(haggle_handle_t hh);

/**
	This function returns the Process ID (PID) of a running
        Haggle daemon.
	
	@param the pid to return, or NULL if caller does not care.
	@returns HAGGLE_DAEMON_NOT_RUNNING (HAGGLE_NO_ERROR) if Haggle is 
	not running and previously exited cleanly, HAGGLE_DAEMON_CRASHED if 
	Haggle exited uncleanly (crashed)), HAGGLE_DAEMON_RUNNING if 
	Haggle is running, or HAGGLE_DAEMON_ERROR if the function call failed.
*/
HAGGLE_API int haggle_daemon_pid(unsigned long *pid);

/**
	This function can be used by an application to spawn a new
        Haggle daemon, if none was already running.
	
        @param daemonpath The path to the Haggle daemon executable, or
        NULL if libhaggle should try standard paths.

	@returns HAGGLE_NO_ERROR if a haggle daemon is already running, 1 if a daemon 
        was spawned and HAGGLE_ERROR if an error occured.
*/
HAGGLE_API int haggle_daemon_spawn(const char *daemonpath);
HAGGLE_API int haggle_daemon_spawn_with_callback(const char *daemonpath, daemon_spawn_callback_t callback);

/**
	This function can be used by an application to remove any previous 
	registration by a same-named application from haggle.
	
	PLEASE USE ONLY WHEN ABSOLUTELY CERTAIN THAT THERE IS NO OTHER APPLICATION.
	
        @param name the name of the application to unregister
	@returns an error code.
*/
HAGGLE_API int haggle_unregister(const char *name);


/**
	Get the session Id associated with the handle. 
	A valid handle has a positive session id.
*/
HAGGLE_API int haggle_handle_get_session_id(haggle_handle_t hh);

/**
	Publishes a data object into haggle.
	
	This function does not take possession of the data object.
	
	@returns an error code.
*/
HAGGLE_API int haggle_ipc_publish_dataobject(haggle_handle_t hh, struct dataobject *dobj);

/**
	Register interest in an event type. See event types above for the different
	event types that can be registered.
	
	If this data object matches any interest of this application, a copy of the
	data object will be given to the registered event handler.
	
	@returns an error code.
*/
HAGGLE_API int STDCALL haggle_ipc_register_event_interest(haggle_handle_t hh, const int eventId, haggle_event_handler_t handler);
HAGGLE_API int STDCALL haggle_ipc_register_event_interest_with_arg(haggle_handle_t hh, const int eventId, haggle_event_handler_t handler, void *arg);

/**
	Register interest in a particular attribute name/value combination.
	
	If there is an event handler registered to receive data objects, that event
	handler will be given any new data objects received by haggle that match
	any interest this application has received.
	
	@returns an error code.
*/
HAGGLE_API int haggle_ipc_add_application_interest(haggle_handle_t hh, const char *name, const char *value);

/**
	Register interest in a particular attribute name/value combination with weight.
	
	If there is an event handler registered to receive data objects, that event
	handler will be given any new data objects received by haggle that match
	any interest this application has received.
	
	@returns an error code.
*/
HAGGLE_API int haggle_ipc_add_application_interest_weighted(haggle_handle_t hh, const char *name, const char *value, const unsigned long weight);

/**
	Register several interests in the form of an attribute list.

	@returns an error code.

	Note: the attribute list will not be freed or modified. 
	It still has to be freed by the caller.
*/

HAGGLE_API int haggle_ipc_add_application_interests(haggle_handle_t hh, const struct attributelist *al);


/**
	Deregister interest in a particular attribute name/value combination.
	
	@returns an error code.
*/
HAGGLE_API int haggle_ipc_remove_application_interest(haggle_handle_t hh, const char *name, const char *value);

/**
	Deregister several interests in the form of a list of attributes
	
	@returns an error code.

	Note: the attribute list will not be freed or modified. 
	It still has to be freed by the caller.
*/
HAGGLE_API int haggle_ipc_remove_application_interests(haggle_handle_t hh, const struct attributelist *al);

/**
    SW: - new API function delete application state on deregistration

    Deletes the application state on deregistration (i.e. from the database and
    in memory caches). Useful to save resources. Parameter is treated as
    a boolean. 

	@returns positive value on success, negative (error code) on failure.
*/
HAGGLE_API int haggle_ipc_set_delete_app_state_on_deregister(haggle_handle_t hh, int deleteOnDeReg);


/**     MOS - new API call added

	Set the maximum number of data objects that should be computed by the matching algorithm.
        Not that high numbers can consume significant resources at local and remote nodes. 
        The default threshold for applications is specified in the config.xml file and is usually set to 100. 
        The idea is that this function is used to set the maximum before an application adds interests.

	@returns positive value on success, negative (error code) on failure.
*/
HAGGLE_API int haggle_ipc_set_max_data_objects_in_match(haggle_handle_t hh, 
						 unsigned long maxDataObjectsInMatch);
/**     MOS - new API call added

	Set the matching threshold for the application. This allows us to do more precise
        queries including conjunctions (threshold = 100). The default threshold for applications
        is specified in the config.xml file and is usually set to 0 (which corresponds to a 
        disjuctive interpretation). The idea is that this function is used to set the threshold
        before an application adds interests.

	@returns positive value on success, negative (error code) on failure.
*/
HAGGLE_API int haggle_ipc_set_matching_threshold(haggle_handle_t hh, 
						 unsigned long matchingThreshold);

/**		CBMEN, HL - new API call added

	Dynamically update the configuration.
	config should be an xml object, as a string,
	with DATAOBJECT_METADATA_APPLICATION_CONTROL_DYNAMIC_CONFIGURATION 
	as the root element.

	@returns positive value on success, negative (error code) on failure.
*/

HAGGLE_API int haggle_ipc_update_configuration_dynamic(haggle_handle_t hh,
                                                       const char *config, size_t len);

/**
	Get the currently registered application interests for this application.
	The application interests will be asynchrounously returned in a response
	data object, which the applications receive in one of its event handlers.
	The response data object must be handled by the application itself.

	@returns positive value on success, negative (error code) on failure.
*/
HAGGLE_API int haggle_ipc_get_application_interests_async(haggle_handle_t hh);

/**
	Get the data objects that match the currently registered application 
	interests for this application. The application interests will be 
	asynchrounously returned as usual. MOS: Most importantly, this function
	provides a means to explicitly reset the applications Bloom filter.

	@returns positive value on success, negative (error code) on failure.
*/
HAGGLE_API int haggle_ipc_get_data_objects_async(haggle_handle_t hh);

/**
	Delete a data object that is managed by Haggle. After this call,
	Haggle will no longer store this data object, but it may still exist
	in the application (i.e., the data object given is not deleted in 
	the application).

	@returns a positive value on success, negative (error code) on failure.
	A success of this function does not necessarily mean that the data object
	was deleted from Haggle. It only means that this asynchronous call was
	successful.

	NOTE: This function is not yet implemented.
*/

HAGGLE_API int haggle_ipc_delete_data_object_by_id_bloomfilter(haggle_handle_t hh, const unsigned char id[20], int keep_in_bloomfilter);
HAGGLE_API int haggle_ipc_delete_data_object_bloomfilter(haggle_handle_t hh, const struct dataobject *dobj, int keep_in_bloomfilter);
HAGGLE_API int haggle_ipc_delete_data_object_by_id(haggle_handle_t hh, const unsigned char id[20]);
HAGGLE_API int haggle_ipc_delete_data_object(haggle_handle_t hh, const struct dataobject *dobj);

/**
        Forces Haggle to try and send its node description to
	all neighbors.

	Use lightly. This functions should in general never be used.
	It can cause excessive transmissions.
*/
HAGGLE_API int haggle_ipc_send_node_description(haggle_handle_t hh);

/** CBMEN, HL - New API call added

	Add a new role with the given name and shared secret.

	@returns an error code.
*/
HAGGLE_API int haggle_ipc_add_role_shared_secret(haggle_handle_t hh, const char *name, const char *secret);

/** CBMEN, HL - New API call added

	Add the new roles with the attribute list containing name/shared secret pairs.

	@returns an error code.
*/
HAGGLE_API int haggle_ipc_add_role_shared_secrets(haggle_handle_t hh, const struct attributelist *al);

/** CBMEN, HL - New API call added

	Add a new node shared secret.

	@returns an error code.
*/
HAGGLE_API int haggle_ipc_add_node_shared_secret(haggle_handle_t hh, const char *id, const char *secret);

/** CBMEN, HL - New API call added

	Add the new node shared secrets with the attribute list containing id/shared secret pairs.

	@returns an error code.
*/
HAGGLE_API int haggle_ipc_add_node_shared_secrets(haggle_handle_t hh, const struct attributelist *al);

/** CBMEN, HL - New API call added

	Add a new authority.

	@returns an error code.
*/
HAGGLE_API int haggle_ipc_add_authority(haggle_handle_t hh, const char *name, const char *id);

/** CBMEN, HL - New API call added

	Add the new authorities with the attribute list containing name/id pairs.

	@returns an error code.
*/
HAGGLE_API int haggle_ipc_add_authorities(haggle_handle_t hh, const struct attributelist *al);

/** CBMEN, HL - New API call added

	Authorizes a node for certification

	@returns an error code.
*/
HAGGLE_API int haggle_ipc_authorize_node_for_certification(haggle_handle_t hh, const char *id);

/** CBMEN, HL - New API call added

	Authorizes the nodes for certification with the attribute list containing IDs as keys.

	@returns an error code.
*/
HAGGLE_API int haggle_ipc_authorize_nodes_for_certification(haggle_handle_t hh, const struct attributelist *al);

/** CBMEN, HL - New API call added

	Authorizes a role for the given attributes

	@returns an error code.
*/
HAGGLE_API int haggle_ipc_authorize_role_for_attributes(haggle_handle_t hh, const char *roleName, const struct attributelist *encryption, const struct attributelist *decryption);

/**
	Send shutdown event to Haggle daemon.

	@returns an error code.
*/
HAGGLE_API int haggle_ipc_shutdown(haggle_handle_t hh);

/* Event loop */
/**
	Checks if the event loop is running.
	@returns 1 if the event loop is running, 0 if it is not, or an error code on error.
*/

HAGGLE_API int haggle_event_loop_is_running(haggle_handle_t hh);
/**
	Starts the haggle event loop. This function runs synchronously, and as such
	will not  return until the haggle_event_loop_stop function is called.
	
	If an event loop is already running for the given haggle handle...?
	
	@returns an error code.
*/

HAGGLE_API int haggle_event_loop_run(haggle_handle_t hh);
/**
	Starts running the haggle event loop asynchronously. The haggle event loop
	can be stopped by a call to haggle_event_loop_stop.
	
	If an event loop is already running for the given haggle handle...?
	
	@returns an error code.
*/
HAGGLE_API int haggle_event_loop_run_async(haggle_handle_t hh);
/**
	Stops a running haggle event loop.
	
	@returns an error code.
*/
HAGGLE_API int haggle_event_loop_stop(haggle_handle_t hh);

HAGGLE_API int haggle_event_loop_run_async(haggle_handle_t hh);

/**
	Registers callback functions to be called by the event loop
        when it starts and stops, respectively. The callbacks are
        executed in the event loop's thread context, which might be a
        different one from the thread if the event loop is started
        asynchronously.
	
        @param hh the haggle handle
        @param start the startup callback function, or NULL if none.
        @param stop the stop callback function, or NULL if none.
        @param arg an argument to be passed with the callbacks.
	@returns an error code.
*/
HAGGLE_API int haggle_event_loop_register_callbacks(haggle_handle_t hh, haggle_event_loop_start_t start, haggle_event_loop_stop_t stop, void *arg);

/*@}*/

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* _LIBHAGGLE_IPC_H */
