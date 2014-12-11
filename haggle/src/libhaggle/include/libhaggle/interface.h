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
#ifndef _LIBHAGGLE_INTERFACE_H
#define _LIBHAGGLE_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"
#include "list.h"
#include "exports.h"

#if defined(OS_WINDOWS)
// This is here to avoid a warning with catching the exception in the functions
// below.
#pragma warning( push )
#pragma warning( disable: 4200 )
#endif

/**
   \defgroup Interface Interface
*/
/*@{*/

/*
	The interface types be synchronized with Interface.h in Haggle.
*/
typedef enum HAGGLE_API haggle_interface_type {
	IF_TYPE_UNDEFINED = 0,
	IF_TYPE_APPLICATION_PORT,
	IF_TYPE_APPLICATION_LOCAL,
	IF_TYPE_ETHERNET,
	IF_TYPE_WIFI,
	IF_TYPE_BLUETOOTH,
	IF_TYPE_MEDIA,
	_IF_TYPE_MAX,
} haggle_interface_type_t;


typedef enum HAGGLE_API haggle_interface_status {
	IF_STATUS_UNDEFINED,
	IF_STATUS_UP,
	IF_STATUS_DOWN,
	_IF_STATUS_MAX,
} haggle_interface_status_t;
	
	
typedef struct HAGGLE_API haggle_interface {
	list_t l;
	haggle_interface_type_t type;
	haggle_interface_status_t status;
	char *name;
	char *identifier_str;
	size_t identifier_len;
	char identifier[0];
} haggle_interface_t;

#if defined(OS_WINDOWS)
#pragma warning( pop )
#endif

#if !defined(_LIBHAGGLE_METADATA_H)
struct metadata;
#endif
	
HAGGLE_API haggle_interface_type_t haggle_interface_str_to_type(const char *str);
HAGGLE_API haggle_interface_t *haggle_interface_new(haggle_interface_type_t type, const char *name, const char *identifier, size_t identifier_len);
HAGGLE_API haggle_interface_t *haggle_interface_copy(const haggle_interface_t *iface);
HAGGLE_API haggle_interface_type_t haggle_interface_get_type(const haggle_interface_t *iface);
HAGGLE_API const char *haggle_interface_get_type_name(const haggle_interface_t *iface);
HAGGLE_API haggle_interface_status_t haggle_interface_get_status(const haggle_interface_t *iface);
HAGGLE_API const char *haggle_interface_get_status_name(const haggle_interface_t *iface);
HAGGLE_API const char *haggle_interface_get_name(const haggle_interface_t *iface);
HAGGLE_API const char *haggle_interface_get_identifier(const haggle_interface_t *iface);
HAGGLE_API size_t haggle_interface_get_identifier_length(const haggle_interface_t *iface);
HAGGLE_API char *haggle_interface_get_identifier_str(const haggle_interface_t *iface);
HAGGLE_API void haggle_interface_free(haggle_interface_t *iface);
HAGGLE_API haggle_interface_t *haggle_interface_new_from_metadata(const struct metadata *m);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _LIBHAGGLE_INTERFACE_H */
