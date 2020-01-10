/*
** Generic doubly linked lists and hash tables.
** Copyright (C) 2002-2009 Ryan McCabe <ryan@numb.org>
** Copyright (C) 2009 Red Hat, Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License, version 2,
** as published by the Free Software Foundation.
**
** Author: Ryan McCabe <ryan@redhat.com>
*/

#ifndef __RICCI_LIST_H
#define __RICCI_LIST_H

#if __WORDSIZE == 64
	#define POINTER_TO_INT(p)   ((int) (long) (p))
	#define POINTER_TO_UINT(p)  ((unsigned int) (unsigned long) (p))
	#define INT_TO_POINTER(i)   ((void *) (long) (i))
	#define UINT_TO_POINTER(u)  ((void *) (unsigned long) (u))
#elif __WORDSIZE == 32
	#define POINTER_TO_INT(p)   ((int) (p))
	#define POINTER_TO_UINT(p)  ((unsigned int) (p))
	#define INT_TO_POINTER(i)   ((void *) (i))
	#define UINT_TO_POINTER(u)  ((void *) (u))
#else
#	error "Unknown word size"
#endif

typedef struct dlist {
	struct dlist *next;
	struct dlist *prev;
	void *data;
} dlist_t;

dlist_t *dlist_add_head(dlist_t *head, void *data);
void dlist_destroy(dlist_t *head, void *param, void (*cleanup)(void *, void *));
void *dlist_remove_head(dlist_t **list);
dlist_t *dlist_remove(dlist_t *head, dlist_t *node);
dlist_t *dlist_find(dlist_t *head, void *data, int (*comp)(void *, void *));
dlist_t *dlist_add_after(dlist_t *head, dlist_t *node, void *data);
dlist_t *dlist_add_tail(dlist_t *head, void *data);
dlist_t *dlist_tail(dlist_t *head);
void dlist_iterate(dlist_t *head, void (*func)(void *, void *), void *data);
size_t dlist_len(dlist_t *head);

typedef struct hash {
	uint32_t order;
	int (*compare)(void *l, void *r);
	uint32_t (*data_hash)(void *l, uint32_t order);
	void (*remove)(void *param, void *data);
	dlist_t **map;
} hash_t;

int hash_init(	hash_t *hash,
				uint32_t order,
				int (*compare)(void *, void *),
				uint32_t (*data_hash)(void *, uint32_t),
				void (*rem)(void *param, void *data));

void *hash_find(hash_t *hash, void *key);
void hash_add(hash_t *hash, void *key, void *data);
int hash_remove(hash_t *hash, void *key);
void hash_clear(hash_t *hash);
void hash_destroy(hash_t *hash);
int hash_exists(hash_t *hash, void *key);
void hash_iterate(hash_t *hash, void (*func)(void *, void *), void *data);
uint32_t hash_string(void *data, uint32_t order);
uint32_t hash_int32(void *d, uint32_t order);

#else
#	warning "included multiple times"
#endif
