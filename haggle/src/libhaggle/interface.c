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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define LIBHAGGLE_INTERNAL
#include "metadata.h"
#include "base64.h"
#include <libhaggle/haggle.h>
#include <libhaggle/debug.h>
#include <libhaggle/interface.h>

static const char *interface_typestr[] = {
	"undefined",
	"application[port]",
	"application[local]",
	"ethernet",
	"wifi",
	"bluetooth",
	"media",
	NULL
};

static const char *interface_statusstr[] = {
	"undefined",
	"up",
	"down",
	NULL
};

haggle_interface_type_t haggle_interface_str_to_type(const char *str)
{
	int i = 0;

        if (!str)
                return IF_TYPE_UNDEFINED;

	while (interface_typestr[i]) {
		if (strcmp(str, interface_typestr[i]) == 0)
			return i;
		i++;
	}
	return IF_TYPE_UNDEFINED;
}

char *haggle_interface_get_identifier_str(const haggle_interface_t *iface)
{
	if (!iface)
		return NULL;

	return iface->identifier_str;
}

#define INTERFACE_MALLOC_LEN(name, id_len, id_strlen) \
	(sizeof(haggle_interface_t) + id_len + strlen(name) + 1 + id_strlen)

static inline int interface_identifier_strlen(haggle_interface_type_t type)
{
        switch (type) {
                case IF_TYPE_BLUETOOTH:
                case IF_TYPE_ETHERNET:
                case IF_TYPE_WIFI:
                        return 18;
                case IF_TYPE_MEDIA:
                default:
                        break;
	}
        return 0;
}

haggle_interface_t *haggle_interface_new(haggle_interface_type_t type, const char *name, 
					 const char *identifier, size_t identifier_len)
{
	haggle_interface_t *iface;
	
	if (!name || !identifier)
		return NULL;

	iface = malloc(INTERFACE_MALLOC_LEN(name, identifier_len, 
					    interface_identifier_strlen(type)));

	if (!iface)
		return NULL;

	iface->type = type;
	iface->status = IF_STATUS_UNDEFINED;
	iface->identifier_len = identifier_len;
	memcpy(iface->identifier, identifier, identifier_len);
	iface->name = iface->identifier + identifier_len;
	strcpy(iface->name, name);
	iface->identifier_str = iface->name + strlen(iface->name) + 1;

	switch (type) {
	case IF_TYPE_BLUETOOTH:
	case IF_TYPE_ETHERNET:
	case IF_TYPE_WIFI:
		sprintf(iface->identifier_str, "%02x:%02x:%02x:%02x:%02x:%02x", 
			(unsigned char)iface->identifier[0], (unsigned char)iface->identifier[1], 
			(unsigned char)iface->identifier[2], (unsigned char)iface->identifier[3], 
			(unsigned char)iface->identifier[4], (unsigned char)iface->identifier[5]);
		break;
	case IF_TYPE_MEDIA:
	default:
		break;		
	}
/*
	LIBHAGGLE_DBG("New interface type=%s name=%s identifier_str=%s\n", 
		      interface_typestr[type], name, iface->identifier_str);
*/
	return iface;
}

haggle_interface_t *haggle_interface_new_from_metadata(const metadata_t *m)
{
	haggle_interface_type_t type;
	haggle_interface_t *iface = NULL;
	struct base64_decode_context b64_ctx;
	char *identifier = NULL;
	const char *id, *status, *name;
	size_t len;
	
	type = haggle_interface_str_to_type(metadata_get_parameter(m, "type"));
	
	if (type == IF_TYPE_UNDEFINED)
		return NULL;
	
	id = metadata_get_parameter(m, "identifier");
	
	if (!id)
		return NULL;
		
	base64_decode_ctx_init(&b64_ctx);
	
	if (!base64_decode_alloc(&b64_ctx, id, strlen(id), &identifier, &len))
		return NULL;
	
	name = metadata_get_parameter(m, "name");
	
	// Add interface to node
	iface = haggle_interface_new(type, name ? name : "Unknown interface", identifier, len);
	free(identifier);
	
	if (!iface) 
		return NULL;
	
	status = metadata_get_parameter(m, "status");
	
	if (status) {
		if (strcmp(status, "up") == 0) {
			iface->status = IF_STATUS_UP;
		} else {
			iface->status = IF_STATUS_DOWN;
		}
	}
	
	return iface;
}

void haggle_interface_free(haggle_interface_t *iface)
{
	free(iface);
}

haggle_interface_t *haggle_interface_copy(const haggle_interface_t *iface)
{
        haggle_interface_t *iface_copy;
        int malloc_len = 0;

        if (!iface)
                return NULL;

        malloc_len = INTERFACE_MALLOC_LEN(iface->name, iface->identifier_len, interface_identifier_strlen(iface->type));

        iface_copy = (haggle_interface_t *)malloc(malloc_len);
        
        if (!iface_copy)
                return NULL;

        memcpy(iface_copy, iface, malloc_len);

        return iface_copy;
}

haggle_interface_type_t haggle_interface_get_type(const haggle_interface_t *iface)
{
	return (iface ? iface->type : IF_TYPE_UNDEFINED);
}

const char *haggle_interface_get_type_name(const haggle_interface_t *iface)
{
	return (iface ? interface_typestr[iface->type] : "undefined");
}

haggle_interface_status_t haggle_interface_get_status(const haggle_interface_t *iface)
{
	return (iface ? iface->status : IF_STATUS_UNDEFINED);
}

const char *haggle_interface_get_status_name(const haggle_interface_t *iface)
{
	return (iface ? interface_statusstr[iface->status] : "undefined");
}

const char *haggle_interface_get_name(const haggle_interface_t *iface)
{
	return (iface ? iface->name : NULL);
}

const char *haggle_interface_get_identifier(const haggle_interface_t *iface)
{
	return (iface ? iface->identifier : NULL);
}

size_t haggle_interface_get_identifier_length(const haggle_interface_t *iface)
{
	return (iface ? iface->identifier_len : 0);
}
