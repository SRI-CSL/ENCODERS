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
#ifndef _LIBHAGGLE_NODE_H
#define _LIBHAGGLE_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"
#include "exports.h"
#include "attribute.h"
#include "interface.h"

/**
   \defgroup Node Node
*/
/*@{*/

#define NODE_ID_LEN 20

typedef struct HAGGLE_API node {
	list_t l;
        char id[NODE_ID_LEN];
	char *name;
	int num_attr;
	int num_interfaces;
	list_t attributes;
	list_t interfaces;
} haggle_node_t;

#define HAGGLE_XML_NODE_NAME "Node"

#if !defined(_LIBHAGGLE_METADATA_H)
struct metadata;
#endif

HAGGLE_API struct node *haggle_node_new_from_metadata(struct metadata *m);

/**
	Releases the memory associated with a node. The node cannot
	be used after a call to this function.
*/
HAGGLE_API void haggle_node_free(struct node *n);

HAGGLE_API const char *haggle_node_get_name(const struct node *n);

HAGGLE_API int haggle_node_get_num_interfaces(const struct node *n);


HAGGLE_API haggle_interface_t *haggle_node_get_interface_n(struct node *n, const int num);

/**
	Adds an attribute to the given node. The attribute will have weight
	1 (default).
	
	The node and both strings are the property of the caller.
	
	@returns an error code.
*/
HAGGLE_API int haggle_node_add_attribute(struct node *node, const char *name, const char *value);

/**
	Adds an attribute to the given node.
	
	The node and both strings are the property of the caller.
	
	@returns an error code.
*/
HAGGLE_API int haggle_node_add_attribute_weighted(struct node *n, const char *name, const char *value, const unsigned long weight);

/**
	Returns the number of attributes this node has.
	
	@returns A count of the number of attributes this node has iff 
	successful, negative (an error code) otherwise.
*/
HAGGLE_API int haggle_node_get_num_attributes(const struct node *node);

/**
	Returns the node's nth attribute.
	
	The returned attribute is the property of the node and may not
	be released.
	
	@returns A valid pointer to an attribute iff successful, NULL otherwise.
*/
HAGGLE_API struct attribute *haggle_node_get_attribute_n(struct node *n, const int num);

/**
	Returns the node's first attribute with the given name.
	
	The returned attribute is the property of the node and may not
	be released.
	
	@returns A valid pointer to an attribute iff successful, NULL otherwise.
*/
HAGGLE_API struct attribute *haggle_node_get_attribute_by_name(struct node *n, const char *name);

/**
	Returns the node's n:th attribute with the given name.
	
	The returned attribute is the property of the node and may not
	be released.
	
	@returns A valid pointer to an attribute iff successful, NULL otherwise.
*/
HAGGLE_API struct attribute *haggle_node_get_attribute_by_name_n(struct node *n, const char *name, const int num);

/**
	Returns the node's first attribute with the given name and value.
	
	The returned attribute is the property of the node and may not
	be released.
	
	@returns A valid pointer to an attribute iff successful, NULL otherwise.
*/
HAGGLE_API struct attribute *haggle_node_get_attribute_by_name_value(struct node *n, const char *name, const char *value);

#ifdef DEBUG

/**
	Prints debugging information about nodes to stdout.
*/
HAGGLE_API void haggle_node_leak_report_print();

/**
	Prints the node's current attributes and their values to stdout.
*/
void haggle_node_print_attributes(struct node *n);
#endif

/*@}*/

/**
   \defgroup NodeList Node list
*/
/*@{*/

typedef struct HAGGLE_API nodelist {
	list_t nodes;
	int num_nodes;
} haggle_nodelist_t;

HAGGLE_API haggle_nodelist_t *haggle_nodelist_new_from_metadata(struct metadata *m);
HAGGLE_API struct node *haggle_nodelist_get_node_n(haggle_nodelist_t *nl, const int num);

HAGGLE_API struct node *haggle_nodelist_pop(haggle_nodelist_t *nl);

HAGGLE_API int haggle_nodelist_size(haggle_nodelist_t *nl);

HAGGLE_API void haggle_nodelist_free(haggle_nodelist_t *nodelist);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _LIBHAGGLE_NODE_H */
