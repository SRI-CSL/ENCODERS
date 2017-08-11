/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
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
#ifndef _DATAOBJECT_H
#define _DATAOBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"
#include "list.h"
#include "exports.h"
#include "attribute.h"

#ifdef OS_UNIX
#include <sys/types.h>
#include <sys/time.h>
#endif
#ifdef OS_WINDOWS_DESKTOP
#include <winsock.h>
#endif

#include <stdio.h>

/**
   \defgroup DataObject Data object
*/

/*@{*/

#define HASH_LENGTH 20

typedef unsigned char dataobject_id_t[HASH_LENGTH];

/* Do not make visible the internal structure of the data object. */
typedef struct dataobject haggle_dobj_t;

/* Flags for data object */
#define DATAOBJECT_FLAG_NONE       0x0
#define DATAOBJECT_FLAG_PERSISTENT 0x1
#define DATAOBJECT_FLAG_ALL        ~0x0

/* Global parameters */
#define DATAOBJECT_CREATE_TIME_PARAM "create_time"
#define DATAOBJECT_PERSISTENT_PARAM "persistent"

/* 'Data' portion of metadata */
#define DATAOBJECT_METADATA_DATA "Data"
#define DATAOBJECT_METADATA_DATA_DATALEN_PARAM "data_len"
#define DATAOBJECT_METADATA_DATA_FILEPATH "FilePath"
#define DATAOBJECT_METADATA_DATA_FILENAME "FileName"
#define DATAOBJECT_METADATA_DATA_FILEHASH "FileHash"
#define DATAOBJECT_METADATA_DATA_THUMBNAIL "Thumbnail"

/* Attributes in data objects that we can parse */
#define DATAOBJECT_CONTROL_ATTR "Control"

#define DATAOBJECT_METADATA_ATTRIBUTE "Attr"
#define DATAOBJECT_METADATA_ATTRIBUTE_NAME_PARAM "name"
#define DATAOBJECT_METADATA_ATTRIBUTE_WEIGHT_PARAM "weight"

/**
	This variable (which is only valid after getting the haggle handle), points
	to the directory where files belonging to data objects can be put. This is
	the same directory as haggle itself uses to store its data files.
*/
HAGGLE_API extern char *haggle_directory;

/* Data object API functions */
/**
	Creates a new data object. 
	
	The produced object is owned by the receiver.
	
	@returns A valid pointer to a data object struct iff successful, NULL 
	otherwise.
*/
HAGGLE_API struct dataobject *haggle_dataobject_new();

/**
	Creates a new data object with the contents of the file as data. 
	
	The produced object is owned by the receiver.
	
	@returns A valid pointer to a data object struct iff successful, NULL 
	otherwise.
*/
HAGGLE_API struct dataobject *haggle_dataobject_new_from_file(const char *filepath);

/**
	Creates a new data object with the contents of the data buffer as data. 
	
	The produced object is owned by the receiver.
	
	When this data object is received by libhaggle, the data buffer and the 
	length of the buffer will be available using
	haggle_dataobject_get_databuffer() and haggle_dataobject_get_size() 
	respectively.
	
	@returns A valid pointer to a data object struct iff successful, NULL 
	otherwise.
*/
HAGGLE_API struct dataobject *haggle_dataobject_new_from_buffer(const unsigned char *data, const size_t len);

/**
	Creates a new data object with the given metadata. 
	
	The produced object is owned by the receiver.
	
	@returns A valid pointer to a data object struct iff successful, NULL 
	otherwise.
*/
HAGGLE_API struct dataobject *haggle_dataobject_new_from_raw(const unsigned char *raw, const size_t len);

/**
	Releases the memory associated with a data object. The data object can not
	be used after a call to this function.

        @param dobj the data object to free.
*/
HAGGLE_API void haggle_dataobject_free(struct dataobject *dobj);


/**
	Calculates a unique data object identifier for a data object.

	@param dobj the data object to calculate the id for
	@param id the place to store the resulting id
	
	@returns zero on success or a negative error code on failure.
*/
HAGGLE_API int haggle_dataobject_calculate_id(const struct dataobject *dobj, dataobject_id_t *id);

/**
	Set flags in the data object. The flags passed as argument
        will be added to the existing flags.
        
        @param dobj the data object to set flags on.
        @param flags the flags to set.
        @returns the flags of the data object after the operation.
*/
HAGGLE_API unsigned short haggle_dataobject_set_flags(struct dataobject *dobj, unsigned short flags);
/**
	Unset flags in the data object. The flags passed as argument
        will be unset in the existing flags.
        
        @param dobj the data object to unset flags on.
        @param flags the flags to unset.
        @returns the flags of the data object after the operation.
*/
HAGGLE_API unsigned short haggle_dataobject_unset_flags(struct dataobject *dobj, unsigned short flags);

/**
	Sets the create time of the given data object to the specified value.
	
	@param dobj the data object to set the create time of.
	@param createtime the new create time.
	@returns zero on success or an error code on failure.
*/
HAGGLE_API int haggle_dataobject_set_createtime(struct dataobject *dobj, const struct timeval *createtime);

/**
	Get the create time of the given data object into the given data structure.
	
	@param dobj the data object to get the create time of.
	@returns pointer to createtime (owned by the data object) or NULL if none.
*/
HAGGLE_API struct timeval *haggle_dataobject_get_createtime(struct dataobject *dobj); // MOS

/**
	Get the data length of the given data object;
	
	@param dobj the data object to get the data length of.
	@returns length of the data object.
*/
HAGGLE_API ssize_t haggle_dataobject_get_datalen(struct dataobject *dobj); // SW

/**
	Get the size of the data in this data object in bytes, excluding
	the metadata.
	
	@returns 0 on success, or an error code.
*/
HAGGLE_API int haggle_dataobject_get_data_size(const struct dataobject *dobj, size_t *bytes);

/**
	This function starts reading data from the data object, if the data object
	is not already being read from, as long as there is a file in this data 
	object. It is the caller's responisibility to eventually call 
	haggle_dataobject_read_data_stop() at some point after a successful call to
	this function. 
	
	@returns 0 if it succeeded in setting up the data object for reading, 
	or an error code.
*/
HAGGLE_API int haggle_dataobject_read_data_start(struct dataobject *dobj);


/**
	This function stops reading data from the data object, if the data object
	has been previously set up for reading.
	
	@returns 0 if the file was not being read at the time this funcation was 
	called. 1 if successful, or an error code.
*/
HAGGLE_API int haggle_dataobject_read_data_stop(struct dataobject *dobj);

/**
	This function reads up to count bytes data from a data object which
	has previously been set up for reading by calling 
	haggle_dataobject_read_data_start();
	
	@returns the number of bytes read or an error code.
*/
HAGGLE_API ssize_t haggle_dataobject_read_data(struct dataobject *dobj, void *buffer, size_t count);

/**
	This function returns a buffer with all the data object's data in it, 
	excluding the metadata header.
	
	The length of the buffer will be the same as reported by 
	haggle_dataobject_get_data_size().
	
	The returned pointer is owned by the caller, and it's the caller's 
	responsibility to free() it.
	
	This can be seen as a convenience function, similar to using 
	haggle_dataobject_get_data_size(), malloc(), 
	haggle_dataobject_read_data_start(), haggle_dataobject_read_data() and 
	haggle_dataobject_read_data_stop() to extract the information.
	
	@returns A valid pointer to the data or NULL.
*/
HAGGLE_API void *haggle_dataobject_get_data_all(struct dataobject *dobj);


/**
	Returns the raw metadata of the given data object. The metadata belongs
	to the data object and should NOT be freed by the caller
	
	@param dobj the data object
	@returns a valid pointer to the raw metadata on success, or NULL.
*/
HAGGLE_API const unsigned char *haggle_dataobject_get_raw(struct dataobject *dobj);
HAGGLE_API size_t haggle_dataobject_get_raw_length(const struct dataobject *dobj);
	
/**
	Allocate raw metadata buffer from data object.
	@param dobj the data object to get in raw format.
	@param a pointer to a pointer that will point to the allocated buffer.
	@param a pointer to an size_t integer which will hold the length of the
	allocated buffer.
 
	The raw metadata belongs to the caller and should be freed after usage.
 
	@returns HAGGLE_NO_ERROR on success, or a Haggle error code on failure.
*/
HAGGLE_API int haggle_dataobject_get_raw_alloc(struct dataobject *dobj, unsigned char **buf, size_t *len);
HAGGLE_API struct metadata *haggle_dataobject_to_metadata(struct dataobject *dobj);
/**
	Returns the filename of the given data object.
	
	The returned string is owned by the object and should not be released or 
	modified.
	
	The file name returned by this function may or may not be the same as the 
	end of the file path for the data object. The name returned from this 
	function is the same as when the data object was created, which may not be 
	true for the actual file available on disk.
	
	@returns A valid pointer to a string containing the name of the data 
	object's file iff successful, NULL otherwise. 
*/
HAGGLE_API const char *haggle_dataobject_get_filename(const struct dataobject *dobj);
HAGGLE_API const char *haggle_dataobject_set_filename(struct dataobject *dobj, const char *filename);
/**
	Returns the filepath of the given data object.
	
	The returned string is owned by the object and should not be released or 
	modified.
	
	@returns A valid pointer to a string containing a path to the file 
	containing the data object's data iff successful, NULL otherwise.
*/
HAGGLE_API const char *haggle_dataobject_get_filepath(const struct dataobject *dobj);
HAGGLE_API const char *haggle_dataobject_set_filepath(struct dataobject *dobj, const char *filepath);

/**
	If the data object has a file, this function will create an attribute with
	a hash of the file's contents. This is good for two reasons:
	
	1. It will decrease the likelihood of two different data objects being 
	percieved as identical by haggle.
	
	2. It will enable haggle to do an integrity check on the data object when
	sent/received.
	
	@returns On success, see haggle_dataobject_add_attribute, on failiure a 
	negative value (an error code).
*/
HAGGLE_API int haggle_dataobject_add_hash(struct dataobject *dobj);


/** IRD, HK - New API call added
	Adds an interest to the given data object.
	
	The data object and string is property of the caller.
	
	@returns an error code.
*/
HAGGLE_API int haggle_dataobject_add_interest_policy(struct dataobject *dobj, char *policy_name);

/**
	Attaches a thumbnail to a data object by embedding the thumbnail
        in the data object's metadata.

        @param dobj the data object to attach the thumbnail to.
        @param data the thumbnail data buffer
        @param len the length of the data to read from the data buffer

        @returns HAGGLE_NO_ERROR on success, or a failure code on error.
*/
HAGGLE_API int haggle_dataobject_set_thumbnail(struct dataobject *dobj, char *data, size_t len);
        /**
           Get the size of the embedded thumbnail, if any.
           
           @param dobj the data object to get the thumbnail from
           @param bytes a pointer to a size_t integer where to store the size
           
           @returns HAGGLE_NO_ERROR on success, or a failure code on error.
        */
HAGGLE_API ssize_t haggle_dataobject_read_thumbnail(const struct dataobject *dobj, char *data, size_t len);

        /**
           Get the size of the embedded thumbnail, if any. The returned size 
           indicates the length of a buffer needed to read the thumbnail from 
           the embedded metadata and may be slightly larger than the actual
           thumbnail.

           @param dobj the data object to get the thumbnail from
           @param bytes a pointer to a size_t integer where to store the size
           
           @returns HAGGLE_NO_ERROR on success, or a failure code on error.
         */
HAGGLE_API int haggle_dataobject_get_thumbnail_size(const struct dataobject *dobj, size_t *bytes);

/**
	Adds an attribute to the given data object. The attribute will have weight
	1 (default).
	
	The data object and both strings are the property of the caller.
	
	@returns an error code.
*/
HAGGLE_API int haggle_dataobject_add_attribute(struct dataobject *dobj, const char *name, const char *value);

/**
	Adds an attribute to the given data object.
	
	The data object and both strings are the property of the caller.
	
	@returns an error code.
*/
HAGGLE_API int haggle_dataobject_add_attribute_weighted(struct dataobject *dobj, const char *name, const char *value, const unsigned long weight);

/**
	Returns the number of attributes this data object has.
	
	@returns A count of the number of attributes this data object has.
*/
HAGGLE_API unsigned long haggle_dataobject_get_num_attributes(const struct dataobject *dobj);

/**
	Returns the data object's nth attribute.
	
	The returned attribute is the property of the data object and may not
	be released or modified.
	
	@returns A valid pointer to an attribute iff successful, NULL otherwise.
*/
HAGGLE_API struct attribute *haggle_dataobject_get_attribute_n(struct dataobject *dobj, const unsigned long n);

/**
	Returns the data object's first attribute with the given name.
	
	The returned attribute is the property of the data object and may not
	be released or modified.
	
	@returns A valid pointer to an attribute iff successful, NULL otherwise.
*/
HAGGLE_API struct attribute *haggle_dataobject_get_attribute_by_name(struct dataobject *dobj, const char *name);

/**
	Returns the data object's nth attribute with the given name.
	
	The returned attribute is the property of the data object and may not
	be released or modified.
	
	@returns A valid pointer to an attribute iff successful, NULL otherwise.
*/
HAGGLE_API struct attribute *haggle_dataobject_get_attribute_by_name_n(struct dataobject *dobj, const char *name, const unsigned long n);

/**
	Returns the data object's first attribute with the given name and value.
	
	The returned attribute is the property of the data object and may not
	be released or modified.
	
	@returns A valid pointer to an attribute iff successful, NULL otherwise.
*/
HAGGLE_API struct attribute *haggle_dataobject_get_attribute_by_name_value(struct dataobject *dobj, const char *name, const char *value);

/**
	Returns a pointer to the data object's attribute list.
	
	The returned attribute list is the property of the data object and may not
	be released or modified.
	
	@returns A valid pointer to an attribute list iff successful, NULL otherwise.
*/
HAGGLE_API struct attributelist *haggle_dataobject_get_attributelist(struct dataobject *dobj);


/**
	Removes the attribute with the given name and value from a data object.
	Note that the attribute is not freed, but is still the property of the caller.
	
	@param dobj the data object to remove the attribute from 
	@param a a pointer to the attribute to remove

	@returns the number of attributes removed, zero if no such attribute existed, 
	or negative on error.
*/
HAGGLE_API int haggle_dataobject_remove_attribute(struct dataobject *dobj, struct attribute *a);

/**
	Removes the attribute with the given name and value.
	
	@param dobj the data object to remove the attribute from 
	@param name the name part of the attribute
	@param value the value part of the attribute

	@returns the number of attributes removed, zero if no such attribute existed, 
	or negative on error.
*/
HAGGLE_API int haggle_dataobject_remove_attribute_by_name_value(struct dataobject *dobj, const char *name, const char *value);

	
struct metadata;
/**
	Get a handle to the metadata of the data object.
	
	@param dobj the data object to get the metadata from
	@param name the name of the metadata to retrieve, or NULL for the top one
	@returns the metadata handle or NULL on error.
*/

HAGGLE_API struct metadata *haggle_dataobject_get_metadata(struct dataobject *dobj, const char *name);

/**
	Add metadata to a data object.

	@param dobj the data object to add metadata to
	@param m the metadata to add. If the function succeeds, the metadata will be 
	owned by the data object and should not be freed.
	@returns HAGGLE_NO_ERROR on success, or HAGGLE_ERROR or HAGGLE_PARAM_ERROR on failure.
*/
HAGGLE_API int haggle_dataobject_add_metadata(struct dataobject *dobj, struct metadata *m);

#ifdef DEBUG

/**
	Prints debugging information about data objects to stdout.
*/
HAGGLE_API void haggle_dataobject_leak_report_print();
	
/**
	Print the metadata part of the data object to a file stream.
	@param dobj the data object to print
	@param fp the file pointer (stream) to print to
	@returns the number of bytes printed, or HAGGLE_ERROR on error.
 */
HAGGLE_API int haggle_dataobject_print(FILE *fp, struct dataobject *dobj);

/**
	Prints the data object's current attributes and their values to stdout.
*/
HAGGLE_API void haggle_dataobject_print_attributes(struct dataobject *dobj);

#endif /* DEBUG */

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _DATAOBJECT_H */
