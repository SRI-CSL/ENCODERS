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

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../../config.h"
#endif

#define LIBHAGGLE_INTERNAL
#include <libhaggle/haggle.h>
#include "base64.h"
#include "metadata.h"


#ifdef DEBUG
static unsigned long num_node_alloc = 0;
static unsigned long num_node_free = 0;
#endif

static void haggle_node_add_interface(struct node *n,
				      haggle_interface_t *iface);

static int haggle_nodelist_add_node(haggle_nodelist_t *nl, struct node *n);

static haggle_nodelist_t *haggle_nodelist_new()
{
	haggle_nodelist_t *nodelist;

	nodelist = malloc(sizeof(haggle_nodelist_t));

	if (!nodelist)
		return NULL;

	nodelist->num_nodes = 0;
	INIT_LIST(&nodelist->nodes);

	return nodelist;
}

void haggle_nodelist_free(haggle_nodelist_t *nodelist)
{
	list_t *pos, *tmp;

	if (!nodelist)
		return;

	list_for_each_safe(pos, tmp, &nodelist->nodes) {
		struct node *n = (haggle_node_t *)pos;
		haggle_node_free(n);
	}

	free(nodelist);
}

int haggle_nodelist_size(haggle_nodelist_t *nl)
{
	return (nl ? nl->num_nodes : HAGGLE_PARAM_ERROR);
}

haggle_nodelist_t *haggle_nodelist_new_from_metadata(struct metadata *m)
{
	haggle_nodelist_t *nl;
        metadata_t *mn;
        
	if (!m) {
		LIBHAGGLE_DBG("Invalid metadata\n");
		return NULL;
	}
	nl = haggle_nodelist_new();

	if (!nl) {
		LIBHAGGLE_DBG("Could not create node list\n");
		return NULL;
	}
        
        mn = metadata_get(m, "Node");

        while (mn) {
                struct node *n = haggle_node_new_from_metadata(mn);
                
                if (n)
                        haggle_nodelist_add_node(nl, n);

                mn = metadata_get_next(m);
        }
        
	return nl;
}

int haggle_nodelist_add_node(haggle_nodelist_t *nl, struct node *n)
{
	if (!nl || !n)
		return HAGGLE_PARAM_ERROR;

	list_add(&n->l, &nl->nodes);
	nl->num_nodes++;

	return nl->num_nodes;
}

struct node *haggle_nodelist_get_node_n(haggle_nodelist_t *nl, const int num)
{
	list_t *pos;
	int i = 0;

	if (!nl || num < 0)
		return NULL;

	list_for_each(pos, &nl->nodes) {
		struct node *n = (struct node *)pos;
		if (i++ == num)
			return n;
	}
	return NULL;
}

struct node *haggle_nodelist_pop(haggle_nodelist_t *nl)
{
	struct node *n;

	if (!nl || nl->num_nodes == 0 || !nl->nodes.next)
		return NULL;

	n = (struct node *)nl->nodes.next;

	list_detach(&n->l);

	nl->num_nodes--;

	return n;
}

struct node *haggle_node_new_from_metadata(metadata_t *m)
{
	struct node *n;
        metadata_t *mi;
        const char *name, *id;


        if (!m || !metadata_name_is(m, "Node"))
                return NULL;

	n = (struct node *)malloc(sizeof(struct node));

	if (!n) 
		return NULL;

	memset(n, 0, sizeof(struct node));

	INIT_LIST(&n->attributes);
	INIT_LIST(&n->interfaces);
        
        name = metadata_get_parameter(m, "name");

        if (name) {
                n->name = (char *)malloc(strlen(name) + 1);
                
                if (!n->name)
                        return NULL;
                
                strcpy(n->name, name);
        }

        id = metadata_get_parameter(m, "id");

	if (id) {
		struct base64_decode_context b64_ctx;
		size_t len = 0;

		base64_decode_ctx_init(&b64_ctx);
		base64_decode(&b64_ctx, id, strlen(id), n->id, &len);
	}

	mi = metadata_get(m, "Interface");

	while (mi) {
		haggle_interface_t *iface = haggle_interface_new_from_metadata(mi);
		
		if (iface) {
			haggle_node_add_interface(n, iface);
		}
		
		mi = metadata_get_next(m);
	}
#ifdef DEBUG
	num_node_alloc++;
#endif
	return n;
}

void haggle_node_free(struct node *n)
{
	list_t *pos, *tmp;

	if (!n)
		return;

	list_for_each_safe(pos, tmp, &n->attributes) {
		struct attribute *attr = (struct attribute *)pos;
		haggle_attribute_free(attr);
	}
	list_for_each_safe(pos, tmp, &n->interfaces) {
		haggle_interface_t *iface = (haggle_interface_t *)pos;
		haggle_interface_free(iface);
	}

	if (n->name)
		free(n->name);

	free(n);
	
	n = NULL;

#ifdef DEBUG
	num_node_free++;
#endif
}

const char *haggle_node_get_name(const struct node *n)
{
	if(!n)
		return NULL;
	return n->name;
}

void haggle_node_add_interface(struct node *n, haggle_interface_t *iface)
{
	if (!n || !iface)
		return;

        n->num_interfaces++;
	list_add(&iface->l, &n->interfaces);
}

int haggle_node_get_num_interfaces(const struct node *n)
{
	return (n ? n->num_interfaces : HAGGLE_PARAM_ERROR);
}

haggle_interface_t *haggle_node_get_interface_n(struct node *n, const int num)
{
	list_t *pos;
	int i = 0;

	if (!n)
		return NULL;

	list_for_each(pos, &n->interfaces) {
		haggle_interface_t *iface = (haggle_interface_t *)pos;
		if (i++ == num)
			return iface;
	}
	return NULL;
}

int haggle_node_get_num_attributes(const struct node *n)
{
	return (n ? n->num_attr : HAGGLE_PARAM_ERROR);
}

struct attribute *haggle_node_get_attribute_n(struct node *n, const int num)
{
	list_t *pos;
	int i = 0;

	if (!n)
		return NULL;

	list_for_each(pos, &n->attributes) {
		struct attribute *attr = (struct attribute *)pos;
		if (i++ == num)
			return attr;
	}
	return NULL;
}

struct attribute *haggle_node_get_attribute_by_name(struct node *n, const char *name)
{	
	list_t *pos;

	if (!n || !name)
		return NULL;

	list_for_each(pos, &n->attributes) {
		struct attribute *attr = (struct attribute *)pos;
		if (strcmp(attr->name, name) == 0)
			return attr;
	}
	return NULL;
}

struct attribute *haggle_node_get_attribute_by_name_value(struct node *n, const char *name, const char *value)
{	
	list_t *pos;

	if (!n || !name || !value)
		return NULL;

	list_for_each(pos, &n->attributes) {
		struct attribute *attr = (struct attribute *)pos;
		if (strcmp(attr->name, name) == 0 && strcmp(attr->value, value) == 0)
			return attr;
	}
	return NULL;
}

struct attribute *haggle_node_get_attribute_by_name_n(struct node *n, const char *name, const int num)
{	
	list_t *pos;
	int i = 0;

	if (!n || !name)
		return NULL;

	list_for_each(pos, &n->attributes) {
		struct attribute *attr = (struct attribute *)pos;
		if (strcmp(attr->name, name) == 0) {
			if (num == i++)
				return attr;
		}
	}
	return NULL;
}

int haggle_node_add_attribute(struct node *n, const char *name, const char *value)
{
	return haggle_node_add_attribute_weighted(n, name, value, 1);
}

int haggle_node_add_attribute_weighted(struct node *n, const char *name, const char *value, const unsigned long weight)
{
	struct attribute *attr;

	attr = haggle_attribute_new_weighted(name, value, weight);

	if (!attr) {
		LIBHAGGLE_DBG("Could not add attribute %s:%s\n", name, value);
		return HAGGLE_INTERNAL_ERROR;
	}

	n->num_attr++;
	list_add(&attr->l, &n->attributes);

	LIBHAGGLE_DBG("Added attribute %s:%s\n", name, value);

	return n->num_attr;
}

#ifdef DEBUG
void haggle_node_leak_report_print()
{
	LIBHAGGLE_DBG("\n"
		"=== libhaggle node leak report begin ===\n"
		"num alloc %lu\n"
		"num free  %lu\n"
		"=== libhaggle node leak report end ===\n", 
		num_node_alloc, 
		num_node_free);	
}

void haggle_node_print_attributes(struct node *n)
{
	list_t *pos;
	int i = 0;

	list_for_each(pos, &n->attributes) {
		struct attribute *attr = (struct attribute *)pos;
		LIBHAGGLE_DBG("attr %d: %s=%s\n", i++,  haggle_attribute_get_name(attr), haggle_attribute_get_value(attr));
	}
}

#endif /* DEBUG */
