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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define LIBHAGGLE_INTERNAL
#include <libhaggle/platform.h>
#include <libhaggle/exports.h>
#include <libhaggle/list.h>
#include <libhaggle/attribute.h>
#include <libhaggle/debug.h>
#include <libhaggle/error.h>


/**
	Create a new attribute from a name and a value.

	Returns: A pointer to the new attribute on success, NULL on error.
*/
struct attribute *haggle_attribute_new(const char *name, const char *value)
{
	return haggle_attribute_new_weighted(name, value, 1);
}

/**
	Create a copy of an attribute.

	Returns: A pointer to the new attribute on success, NULL on error.
*/
struct attribute *haggle_attribute_copy(const struct attribute *a)
{
	return haggle_attribute_new_weighted(a->name, a->value, a->weight);
}

/*
	Create a new weighted attribute from a name, value and a non-negative weight.

	Returns: A pointer to the new attribute on success, NULL on error.
	TODO: We should define a range for valid weights.
*/
struct attribute *haggle_attribute_new_weighted(const char *name, const char *value, const unsigned long weight)
{
	struct attribute *attr;
	int malloclen;
	
	if (!name || !value)
		return NULL;

	// Size of the struct plus the two strings and two '\0' characters
	malloclen = sizeof(struct attribute);
	
	attr = (struct attribute *)malloc(malloclen);

	if (!attr)
		return NULL;

	memset(attr, 0, malloclen);

	INIT_LIST(&attr->l);
	
        attr->name = (char *)malloc(strlen(name) + 1);

        if (!attr->name) {
                free(attr);
                return NULL;
        }

        attr->value = (char *)malloc(strlen(value) + 1);

        if (!attr->value) {
                free(attr->name);
                free(attr);
                return NULL;
        }
	attr->weight = weight;
	strcpy(attr->name, name);
        strcpy(attr->value, value);

	return attr;	
}
/**
	Free an attribute. An attribute should only be freed manually if it is not 
	part of, e.g., a data object, node, or attribute list.
*/
void haggle_attribute_free(struct attribute *attr)
{
        if (attr->name)
                free(attr->name);
        
        if (attr->value)
                free(attr->value);

	free(attr);
}
/**

*/
const char *haggle_attribute_get_name(const struct attribute *attr)
{
	return (attr ? attr->name : NULL);
}

const char *haggle_attribute_get_value(const struct attribute *attr)
{
	return (attr ? attr->value : NULL);
}

const char *haggle_attribute_set_name(struct attribute *attr, const char *name)
{
        char *tmpname;

	if (!attr)
                return NULL;

        tmpname = (char *)malloc(strlen(name) + 1);
        
        if (!tmpname)
                return NULL;
        
        if (attr->name)
                free(attr->name);

        attr->name = tmpname;

        strcpy(attr->name, name);


        return attr->name;        
}

const char *haggle_attribute_set_value(struct attribute *attr, const char *value)
{
        char *tmpval;

	if (!attr)
                return NULL;

        tmpval = (char *)malloc(strlen(value) + 1);
        
        if (!tmpval)
                return NULL;
        
        if (attr->value)
                free(attr->value);
        
        attr->value = tmpval;
        
        strcpy(attr->value, value);

        return attr->value;
}

unsigned long haggle_attribute_get_weight(const struct attribute *attr)
{
	return (attr ? attr->weight : 1);
}

unsigned long haggle_attribute_set_weight(struct attribute *attr, const unsigned long weight)
{
	if (!attr) 
                return 0;

        attr->weight = weight;
        
        return attr->weight;
}


struct attributelist *haggle_attributelist_new()
{
	struct attributelist *al;

	al = (struct attributelist *)malloc(sizeof(struct attributelist));

	if (!al)
		return NULL;

	INIT_LIST(&al->attributes);
	al->num_attributes = 0;

	return al;
}

struct attributelist *haggle_attributelist_new_from_attribute(struct attribute *a)
{
	struct attributelist *al = haggle_attributelist_new();

	if (!haggle_attributelist_add_attribute(al, a))
		return NULL;

	return al;
}

struct attributelist *haggle_attributelist_new_from_string(const char *str)
{
	char *wstr, *ptr, *save_ptr = NULL, *token;
	struct attributelist *al;
	int n = 0;
	
    // SW: fixed off-by-one with malloc here
	wstr = malloc(strlen(str) + 1);

	if (!wstr)
		return NULL;
	
	al = haggle_attributelist_new();

	if (!al) {
		free(wstr);
		return NULL;
	}

	strcpy(wstr, str);
	
	ptr = wstr;

	token = strtok_r(ptr, ",", &save_ptr);
	ptr = NULL;
	
	while(1) {
		int i;
		char *save_ptr2 = NULL;
		char *namevalue[2];

		if (!token)
			break;
			
		for (i = 0; i < 2; i++, token = NULL) {
			namevalue[i] = strtok_r(token, "=", &save_ptr2);
			
			if  (!namevalue[i]) {
				LIBHAGGLE_ERR("bad name-value-pair in string\n");
				haggle_attributelist_free(al);
				free(wstr);
				return NULL;
			}
		}
		haggle_attributelist_add_attribute(al, haggle_attribute_new(namevalue[0], namevalue[1]));
		n++;
		token = strtok_r(ptr, ",", &save_ptr);
	}
	
	free(wstr);

	if (n == 0) {
		haggle_attributelist_free(al);
		LIBHAGGLE_ERR("%s: Bad attribute string \'%s\'\n", __FUNCTION__, str);
		return NULL;
	}

	return al;
}


struct attributelist *haggle_attributelist_copy(const struct attributelist *al)
{
	list_t *pos;
	struct attributelist *alc;

	if (!al)
		return NULL;

	alc = haggle_attributelist_new();

	if (!alc)
		return NULL;

	list_for_each(pos, &al->attributes) {
		struct attribute *a = (struct attribute *)pos;

		if (!haggle_attributelist_add_attribute(alc, haggle_attribute_copy(a))) {
			free(alc);
			return NULL;
		}
	}
	return alc;
}

void haggle_attributelist_free(struct attributelist *al)
{
	list_t *pos, *tmp;

	if (!al)
		return;

	list_for_each_safe(pos, tmp, &al->attributes) {
		struct attribute *a = (struct attribute *)pos;
		haggle_attribute_free(a);
	}

	free(al);
}
/**
	Add an attribute to an attribute list.

	Returns: 0 on failure, or the number of attributes in the list on success.
*/
unsigned long haggle_attributelist_add_attribute(struct attributelist *al, struct attribute *a)
{
        list_t *pos;

	if (!al || !a || !list_unattached(&a->l))
		return 0;

	// MOS - keep list sorted to ensure canonical form for id calculation

	list_for_each(pos, &al->attributes) {
	  struct attribute *a_old = (struct attribute *)pos;
	  int found = 0;
	  int cmp1 = strcmp(a->name, a_old->name);
	  if(cmp1 < 0) {
	    found = 1;
	  }
	  else if (cmp1 == 0) {
	    int cmp2 = strcmp(a->value, a_old->value);
	    if(cmp2 < 0) {
	      found = 1;
	    }
	    else if(cmp2 == 0) {
	      if(a->weight < a_old->weight) found = 1;
	    }
	  }

	  if (found) {
	    list_add_tail(&a->l, pos); 
	    return ++(al->num_attributes);
	  }
	}

	list_add_tail(&a->l, &al->attributes);

	return ++(al->num_attributes);
}

struct attribute *haggle_attributelist_get_attribute_n(struct attributelist *al, const unsigned long _n)
{
	list_t *pos;
	unsigned long n = 0;

	if (!al)
		return NULL;

	list_for_each(pos, &al->attributes) {
		struct attribute *a = (struct attribute *)pos;
		
		if (n++ == _n)
			return a;
	}
	return NULL;
}

struct attribute *haggle_attributelist_get_attribute_by_name(struct attributelist *al, const char *name)
{
	list_t *pos;

	if (!al || !name)
		return NULL;

	list_for_each(pos, &al->attributes) {
		struct attribute *a = (struct attribute *)pos;
		if (strcmp(a->name, name) == 0)
			return a;
	}
	return NULL;
}


struct attribute *haggle_attributelist_get_attribute_by_name_value(struct attributelist *al, const char *name, const char *value)
{	
	list_t *pos;

	if (!al || !name || !value)
		return NULL;

	list_for_each(pos, &al->attributes) {
		struct attribute *a = (struct attribute *)pos;
		if (strcmp(a->name, name) == 0 && strcmp(a->value, value) == 0)
			return a;
	}
	return NULL;
}


struct attribute *haggle_attributelist_get_attribute_by_name_n(struct attributelist *al, const char *name, const int n)
{	
	list_t *pos;
	int i = 0;

	if (!al|| !name)
		return NULL;

	list_for_each(pos, &al->attributes) {
		struct attribute *a = (struct attribute *)pos;

		if (strcmp(a->name, name) == 0) {
			if (n == i++)
				return a;
		}
	}
	return NULL;
}
struct attribute *haggle_attributelist_remove_attribute(struct attributelist *al, const char *name, const char *value)
{
	struct attribute *a;

	if (!al)
		return NULL;

	a = haggle_attributelist_get_attribute_by_name_value(al, name, value);

	if (!a)
		return NULL;

	list_detach(&a->l);
	al->num_attributes--;

	return a;
}

int haggle_attributelist_detach_attribute(struct attributelist *al, struct attribute *a)
{
	/*
		Just detaching an element from the list doesn't require us to first search
		for it. However, as we need to decrement the number of attributes in the list
		we first have to make sure that the list in question actually contains the 
		attribute.
	*/
	if (!al || !a || !haggle_attributelist_get_attribute_by_name_value(al, a->name, a->value))
		return HAGGLE_PARAM_ERROR;

	list_detach(&a->l);
	al->num_attributes--;

	return 1;
}

unsigned long haggle_attributelist_size(struct attributelist *al)
{
	if (!al)
		return 0;

	return al->num_attributes;
}

struct attribute *haggle_attributelist_pop(struct attributelist *al)
{
	struct attribute *a;

	if (!al || !list_first(&al->attributes))
		return NULL;

	al->num_attributes--;

	a = (struct attribute *)list_first(&al->attributes);

	list_detach(&a->l);

	return a;

}

#ifdef DEBUG

void haggle_attributelist_print(struct attributelist *al)
{
	list_t *pos;
	int i = 0;

	list_for_each(pos, &al->attributes) {
		struct attribute *a = (struct attribute *)pos;
		LIBHAGGLE_DBG("attr %d: %s=%s weight=%lu\n", i++,  
			haggle_attribute_get_name(a), 
			haggle_attribute_get_value(a), 
			haggle_attribute_get_weight(a));
	}
}
#endif
