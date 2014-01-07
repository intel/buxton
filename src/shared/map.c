/*
 * This file is part of buxton.
 *
 * Copyright (C) 2013 Intel Corporation
 *
 * buxton is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include "map.h"

/**
 * Common hash function to prevent collisions
 *
 * @param key The key to hash
 * @return a hash value
 */
static inline long int i_hash(long int key)
{
	return (2654435761 * key) % (2^32);
}

BuxtonHashmap *buxton_hashmap_new(long int size)
{
	BuxtonHashmap *ret = NULL;
	ret = calloc(1, sizeof(BuxtonHashmap));
	if (!ret)
		return NULL;
	ret->buckets = calloc((size_t)size, sizeof(BuxtonHashmapBucket*));
	if (!ret->buckets)
		return NULL;
	ret->n_buckets = size;
	ret->n_elements = 0;
	return ret;
}

bool buxton_hashmap_put(BuxtonHashmap *map, long int key, void* value)
{
	long int no = (map->n_buckets-1) % i_hash(key);
	BuxtonHashmapBucket *b = map->buckets[no];
	BuxtonHashmapBucket *tmp = NULL;
	if (!b) {
		b = calloc(1, sizeof(BuxtonHashmapBucket));
		b->next = NULL;
		memset(&b, sizeof(BuxtonHashmapBucket), 0);
		if (!b)
			return false;
	}

	if (!b->value) {
		b->key = key;
		b->value = value;
		b->next = NULL;
	} else {
		tmp = calloc(1, sizeof(BuxtonHashmapBucket));
		if (!tmp)
			return false;	
		tmp->key = key;
		tmp->value = value;
		b->next= tmp;
	}
	map->buckets[no] = b;
	map->n_elements += 1;
	return true;
}

void *buxton_hashmap_get(BuxtonHashmap *map, long int key)
{
	long int no = (map->n_buckets-1) % i_hash(key);
	BuxtonHashmapBucket *b = map->buckets[no];
	BuxtonHashmapBucket *tmp;
	if (!b)
		return NULL;

	if (b->key == key)
		return b->value;

	tmp = b;
	while ((tmp = tmp->next) != NULL) {
		if (tmp->key == key)
			return tmp->value;
	}
	return NULL;
}

void buxton_hashmap_del(BuxtonHashmap *map, long int key)
{
	long int no = (map->n_buckets-1) % i_hash(key);
	BuxtonHashmapBucket *b = map->buckets[no];
	BuxtonHashmapBucket *current, *prev = NULL;

	if (!b)
		return;

	current = b->next;
	prev = b;
	while (current != NULL && prev != NULL) {
		if (b->key == current->key) {
			BuxtonHashmapBucket *temp = current;
			prev->next = current->next;
			free(temp);
			return;
		}
		prev = current;
		current = current->next;
	}
	map->n_elements -= 1;
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
