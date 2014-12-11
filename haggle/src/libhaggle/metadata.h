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
#ifndef _LIBHAGGLE_METADATA_H
#define _LIBHAGGLE_METADATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libhaggle/platform.h>
#include <libhaggle/list.h>
#include <libhaggle/attribute.h>
#include <stdio.h>

/**
   The metadata is an intermediate representation of the metadata part
   of data objects. The purpose of this intermediate layer is to be
   internally agnostic towards the exact raw wire format of data
   objects. This makes the API for adding, removing and parsing
   metadata much simpler, while making it possible to easily do
   changes in the raw wire format, or to change it completely (e.g.,
   from XML to binary).  */
typedef struct metadata_iterator {
        char *name;
        list_t *head, *tmp, *pos;
} metadata_iterator_t;

typedef struct metadata {
        list_t l;
        struct metadata_iterator it;
        struct metadata *parent;
        char *name;
        char *content;
        struct attributelist *parameters;
        unsigned int num_children;
        list_t children;
} metadata_t;

metadata_t *metadata_new(const char *name, const char *content, metadata_t *parent);
void metadata_free(metadata_t *m);
int metadata_name_is(const metadata_t *m, const char *name);
const char *metadata_get_name(const metadata_t *m);
const char *metadata_get_content(const metadata_t *m);
const char *metadata_set_name(metadata_t *m, const char *name);
const char *metadata_set_content(metadata_t *m, const char *content);
int metadata_add(metadata_t *parent, metadata_t *child);
int metadata_detach(metadata_t *child);
metadata_t *metadata_get_next(metadata_t *m);
metadata_t *metadata_get(metadata_t *m, const char *name);
int metadata_set_parameter(metadata_t *m, const char *name, const char *value);
const char *metadata_get_parameter(const metadata_t *m, const char *name);
void metadata_print(FILE *fp, metadata_t *m);

#include "metadata_xml.h"
#define HAGGLE_TAG "Haggle"

#ifdef __cplusplus
}
#endif

#endif /* _LIBHAGGLE_METADATA_H */
