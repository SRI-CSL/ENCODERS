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
#ifndef _LIBHAGGLE_ATTRIBUTE_H
#define _LIBHAGGLE_ATTRIBUTE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"
#include "exports.h"

#if defined(OS_WINDOWS)
// This is here to avoid a warning with catching the exception in the functions
// below.
#pragma warning( push )
#pragma warning( disable: 4200 )
#endif

/**
   \defgroup Attribute Attribute

   Attributes are name value pairs that define the metadata of data
   objects.

*/

/*@{*/

typedef struct HAGGLE_API attribute {
	list_t l;
	unsigned long weight;
	char *value;
	char *name;
} haggle_attr_t;

#if defined(OS_WINDOWS)
#pragma warning( pop )
#endif

/* Attributes */

/**
	Returns the attribute's name.
	
	The returned string is the property of the attribute and may not be 
	released.
	
	@returns A valid pointer to a null-terminated string iff successful, NULL 
	otherwise.
*/
HAGGLE_API const char *haggle_attribute_get_name(const struct attribute *attr);

/**
	Returns the attribute's value.
	
	The returned string is the property of the attribute and may not be 
	released.
	
	@returns A valid pointer to a null-terminated string iff successful, NULL 
	otherwise.
*/
HAGGLE_API const char *haggle_attribute_get_value(const struct attribute *attr);
/**
	Sets the attribute's name.
	
	The returned string is the property of the attribute and may not be 
	released.
	
        @param attr the attribute to set the name on
        @param name the new name 
	@returns A valid pointer to the new name if successful. On failure, NULL 
        is returned and the name is unmodified.
*/
HAGGLE_API const char *haggle_attribute_set_name(struct attribute *attr, const char *name);

/**
	Sets the attribute's value.
	
	The returned string is the property of the attribute and may not be 
	released.
	
        @param attr the attribute to set the name on
        @param value the new value
	@returns A valid pointer to the new value if successful. On failure, NULL 
        is returned and the value is unmodified.

*/
HAGGLE_API const char *haggle_attribute_set_value(struct attribute *attr, const char *value);

/**
	Returns the attribute's weight.
	
	@returns An integer, representing the weight of the attribute.
*/
HAGGLE_API unsigned long haggle_attribute_get_weight(const struct attribute *attr);

/**
	Sets the attribute's weight.
	
	@returns An integer, representing the weight of the attribute.
*/
HAGGLE_API unsigned long haggle_attribute_set_weight(struct attribute *attr, const unsigned long weight);

/**
	Creates a new attribute. The attribute's weight will be 1 (default).
	
	The returned attribute is the property of the caller.
	
	@returns A valid pointer to an attribute iff successful, NULL otherwise.
*/
HAGGLE_API struct attribute *haggle_attribute_new(const char *name, const char *value);

/**
	Create a copy of an attribute.

	The returned attribute is the property of the caller.

	@returns A pointer to the new attribute on success, NULL on error.
*/
HAGGLE_API struct attribute *haggle_attribute_copy(const struct attribute *a);

/**
	Creates a new attribute.
	
	The returned attribute is the property of the caller.
	
	This function is not exported since attributes are only supposed to be 
	associated with a dataobject
	
	@returns A valid pointer to an attribute iff successful, NULL otherwise.
*/
HAGGLE_API struct attribute *haggle_attribute_new_weighted(const char *name, 
						const char *value, 
						const unsigned long weight);

/**
	Releases the memory associated with an attribute.
	
	An attribute should only be freed manually if it is not 
	part of, e.g., a data object, node, or attribute list.

	The attribute can not be used after a call to this function.
*/
HAGGLE_API void haggle_attribute_free(struct attribute *attr);


/*@}*/

/**
   \defgroup AttributeList Attribute list
   @{
*/

typedef struct HAGGLE_API attributelist {
	list_t attributes;
	unsigned long num_attributes;
} haggle_attrlist_t;

/**
	Create a new attribute list with zero contained attributes.
	
	@returns a pointer to the allocated attribute list, or NULL
	on failure.

	
	The list is the property of the caller and has to be disposed of by 
	calling haggle_attributelist_free(), once not needed anymore.
*/
HAGGLE_API struct attributelist *haggle_attributelist_new();

/**
	Create a new attribute list with an initial attribute.
	
	@returns a pointer to the allocated attribute list, or NULL
	on failure. 
	
	The list is the property of the caller and has to be disposed of by 
	calling haggle_attributelist_free(), once not needed anymore.

*/
HAGGLE_API struct attributelist *haggle_attributelist_new_from_attribute(struct attribute *);


/**
	Create a new attribute list from a comma separated string of 
	attributes in the format <name>=<value>. An example valid string
	is, e.g., "foo=bar,bin=baz".
	
	@returns a pointer to the allocated attribute list, or NULL
	on failure. 
	
	The list is the property of the caller and has to be disposed of by 
	calling haggle_attributelist_free(), once not needed anymore.
*/
HAGGLE_API struct attributelist *haggle_attributelist_new_from_string(const char *str);

/**
	Returns an identical copy of the given attribute list.

	
	@returns a pointer to the allocated copy of theattribute list, 
	or NULL on failure. 
	
	The list copy is the property of the caller and has to
	be freed by haggle_attributelist_free() once not needed anymore.
*/
HAGGLE_API struct attributelist *haggle_attributelist_copy(const struct attributelist *al);

/**
	Free the memory associated with an attribute list. This will also
	automatically free any attributes contained within the list.
*/
HAGGLE_API void haggle_attributelist_free(struct attributelist *al);

/**
	Remove an attribute matching a name string and a value string from the
	given list.

	@returns The attribute matching the name value pair on success, or NULL 
	if the attribute was not found in the list or there was a failure.

	The returned attribute is the property of the caller and has to be freed
	once not needed anymore.
*/
HAGGLE_API struct attribute *haggle_attributelist_remove_attribute(struct attributelist *al, const char *name, const char *value);

/**
	Add an attribute to an attribute list.

	@returns 0 on failure, or the number of attributes in the list on success.

	Note: Once the attribute is associated with the list, it can only be freed
	along with the list, unless first detached.
*/
HAGGLE_API unsigned long haggle_attributelist_add_attribute(struct attributelist *al, struct attribute *attr);


/**
	Get the n:th attribute from a list, starting from 0.

	@returns a pointer to n:th attribute if the attribute was found in the
	list, or NULL if it was not.

	The returned attribute is still associated with the list, and hence cannot be
	freed before being detached from the list.
*/
HAGGLE_API struct attribute *haggle_attributelist_get_attribute_n(struct attributelist *al, const unsigned long n);


/**
	Get the first occurence of an attribute matching the given name.

	@returns a pointer to a matching attribute if the attribute was found in the
	list, or NULL if it was not.

	The returned attribute is still associated with the list, and hence cannot be
	freed before being detached from the list.
*/
HAGGLE_API struct attribute *haggle_attributelist_get_attribute_by_name(struct attributelist *al, const char *name);

/**
	Get the first occurence of an attribute matching the given name and value pair.

	@returns a pointer to a matching attribute if the attribute was found in the
	list, or NULL if it was not.

	The returned attribute is still associated with the list, and hence cannot be
	freed before being detached from the list.
*/
HAGGLE_API struct attribute *haggle_attributelist_get_attribute_by_name_value(struct attributelist *al, const char *name, const char *value);

/**
	Get the n:th occurence of an attribute matching the given name, starting from 0.

	@returns a pointer to a matching attribute if the attribute was found in the
	list, or NULL if it was not.

	The returned attribute is still associated with the list, and hence cannot be
	freed before being detached from the list.
*/
HAGGLE_API struct attribute *haggle_attributelist_get_attribute_by_name_n(struct attributelist *al, const char *name, const int n);

/**
	Detach an attribute from its attribute list.

	@returns the number of attributes detached from the list, or an error code on failure.

	Note, that once detached, the attribute is the property of the caller and has
	to be freed manually with haggle_attribute_free().
*/
HAGGLE_API int haggle_attributelist_detach_attribute(struct attributelist *al, struct attribute *a);

/**
	Get the number of attributes contained in the given attribute list.

	@returns the size of the attribute list, or 0 if it is empty or 
	invalid.

*/
HAGGLE_API unsigned long haggle_attributelist_size(struct attributelist *al);

/**
	Remove the first attribute in the list.

	@returns A valid attribute, or NULL on error.

	Note: The returned attribute will no longer be part of the list and is hence
	the property of the caller, which means it must be freed when no longer needed.
*/
HAGGLE_API struct attribute *haggle_attributelist_pop(struct attributelist *al);

#ifdef DEBUG

HAGGLE_API void haggle_attributelist_print(struct attributelist *al);

#endif

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _LIBHAGGLE_ATTRIBUTE_H */
