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
#ifndef _LIBHAGGLE_LIST_H_
#define _LIBHAGGLE_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#if defined(_WIN32) && !defined(inline)
#define inline _inline
#endif

/* Simple double linked list inspired by the Linux kernel list
 * implementation */
typedef struct list {
	struct list *prev, *next;
} list_t;

#define LIST_NULL -1
#define LIST_SUCCESS 1

#define LIST_INIT_HEAD(name) { &(name), &(name) }

#define LIST(name) list_t name = LIST_INIT_HEAD(name)

#define INIT_LIST(h) do { \
	(h)->next = (h); (h)->prev = (h); \
} while (0)

#define INIT_LIST_ELM(le) do { \
	(le)->next = NULL; (le)->prev = NULL; \
} while (0)

static inline int listelm_detach(list_t *prev, list_t *next)
{
	next->prev = prev;
	prev->next = next;

	return LIST_SUCCESS;
}

static inline int listelm_add(list_t *le, list_t *prev, list_t *next)
{
	prev->next = le;
	le->prev = prev;
	le->next = next;
	next->prev = le;

	return LIST_SUCCESS;
}

static inline int list_add(list_t *le, list_t *head)
{

	if (!head || !le)
		return LIST_NULL;

	listelm_add(le, head, head->next);

	return LIST_SUCCESS;
}

static inline int list_add_tail(list_t *le, list_t *head)
{

	if (!head || !le)
		return LIST_NULL;

	listelm_add(le, head->prev, head);

	return LIST_SUCCESS;
}

static inline int list_detach(list_t *le)
{
	if (!le)
		return LIST_NULL;

	listelm_detach(le->prev, le->next);

	le->next = le->prev = NULL;

	return LIST_SUCCESS;
}

#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_reverse(pos, head) \
        for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define list_for_each_safe(pos, tmp, head) \
        for (pos = (head)->next, tmp = pos->next; pos != (head); \
                pos = tmp, tmp = pos->next)

#define list_empty(head) ((head) == (head)->next)

#define list_first(head) ((head)->next)

#define list_unattached(le) ((le)->next == (le) && (le)->prev == (le))

#define list_del(le) list_detach(le)

#ifdef __cplusplus
}
#endif

#endif /* _LIBHAGGLE_LIST_H */
