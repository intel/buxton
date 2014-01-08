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

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "buxtonmap.h"
#include "macro.h"

/**
 * Common hash function to prevent collisions
 *
 * @param key The key to hash
 * @return a hash value
 */
static long int t_hash(void *key) {
	return (long int)INT_TO_PTR(key);
}

/**
 * String function to prevent collisions
 *
 * @param key The key to hash
 * @return a hash value
 */
static long int s_hash(const void *key) {
        unsigned hash = 5381;
        const signed char *c;

        /* DJB's hash function (as used by systemd hashmap) */

        for (c = key; *c; c++)
                hash = (hash << 5) + hash + (unsigned) *c;

        return hash;
}

/**
 * Trivial comparison
 * @param a first
 * @param b second
 * @return boolean value, indicating whether the two values match
 */
static bool t_compare(void *a, void *b) {
	return (a == b);
}

/**
 * String comparison
 * @param a first
 * @param b second
 * @return boolean value, indicating whether the two values match
 */
static bool s_compare(void *a, void *b) {
	return strcmp(a, b) == 0;
}

BuxtonHashmap *buxton_hashmap_new(int size, bool auto_free_key, bool auto_free_value)
{
	BuxtonHashmap *ret = NULL;
	ret = calloc(1, sizeof(BuxtonHashmap));
	if (!ret)
		abort();
	ret->buckets = calloc((size_t)size, sizeof(BuxtonList*));
	if (!ret->buckets)
		abort();
	ret->n_buckets = size;
	ret->n_elements = 0;
	ret->auto_free_key = auto_free_key;
	ret->auto_free_value = auto_free_value;
	return ret;
}

bool buxton_hashmap_put(BuxtonHashmap *map, void* key, void* value)
{
	if (!map->compare)
		map->compare = s_compare;
	if (!map->hash)
		map->hash = s_hash;

	long int no = map->hash(key) % (map->n_buckets-1);
	BuxtonList *b = map->buckets[no];
	bool ret = buxton_list_prepend2(&b, key, value);
	map->buckets[no] = b;
	map->n_elements += 1;

	return ret;
}

bool buxton_hashmap_puti(BuxtonHashmap *map, int key, void* value)
{
	bool ret;
	map->compare = t_compare;
	map->hash = t_hash;
	ret = buxton_hashmap_put(map, INT_TO_PTR(key), value);
	map->compare = NULL;
	map->hash = NULL;
	return ret;
}


void *buxton_hashmap_get(BuxtonHashmap *map, void* key)
{
	if (!map->compare)
		map->compare = s_compare;
	if (!map->hash)
		map->hash = s_hash;

	long int no = map->hash(key) % (map->n_buckets-1);
	BuxtonList *b = map->buckets[no];
	BuxtonList *tmp;

	if (!b)
		return NULL;

	buxton_list_foreach(b, tmp) {
		if (!tmp)
			continue;
		if (map->compare(tmp->data, key))
			return tmp->data2;
	}
	return NULL;
}

void *buxton_hashmap_geti(BuxtonHashmap *map, int key)
{
	void *ret;
	map->compare = t_compare;
	map->hash = t_hash;
	ret = buxton_hashmap_get(map, INT_TO_PTR(key));
	map->compare = NULL;
	map->hash = NULL;
	return ret;
}

void buxton_hashmap_del(BuxtonHashmap *map, void* key)
{
	if (!map->compare)
		map->compare = s_compare;
	if (!map->hash)
		map->hash = s_hash;

	long int no = map->hash(key) % (map->n_buckets-1);
	BuxtonList *b = map->buckets[no];

	if (!b)
		return;

	buxton_list_remove2(&b, key, map->auto_free_key, map->auto_free_value);
	map->buckets[no] = b;
	map->n_elements -= 1;
}

void buxton_hashmap_deli(BuxtonHashmap *map, int key)
{
	map->compare = t_compare;
	map->hash = t_hash;
	buxton_hashmap_del(map, INT_TO_PTR(key));
	map->compare = NULL;
	map->hash = NULL;
}

void buxton_hashmap_free(void *p)
{
	BuxtonHashmap *map = *(BuxtonHashmap**)p;
	int i;
	BuxtonList *b = NULL;
	for (i = 0; i < map->n_buckets; i++) {
		b = map->buckets[i];
		if (!b)
			continue;
		buxton_free_list2(&b, map->auto_free_key, map->auto_free_value);
	}
	free(map->buckets);
	map->buckets = NULL;
	free(map);
	map = NULL;
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
