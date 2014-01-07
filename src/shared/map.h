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

#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define BUXTON_HASHMAP_DEFAULT_SIZE 32
#define _cleanup_hashmap_ __attribute__ ((cleanup(free_hashmap)))

/**
 * BuxtonHashmapBuckets store BuxtonHashMap values and other BuxtonHashmapBuckets
 */
typedef struct BuxtonHashmapBucket {
	long int key; /**<The unique key */
	void *value; /**<Value */
	struct BuxtonHashmapBucket *next; /**<Next key/value pair in the bucket */
} BuxtonHashmapBucket;

/**
 * A simple Hashmap
 */
typedef struct BuxtonHashmap {
	long int n_buckets; /**<Number of buckets */
	BuxtonHashmapBucket **buckets; /**<BuxtonHashmapBucket array */
	long int n_elements; /**Number of elements */
} BuxtonHashmap;

/**
 * Create a new BuxtonHashmap
 * @note a bigger size will help prevent hash collisions
 * 
 * @param size Requested size for the BuxtonHashmap
 * @return a new map, or NULL
 */
BuxtonHashmap *buxton_hashmap_new(long int size);

/**
 * Add a value to the map by key
 * @param key Unique key
 * @param value Value to store (polong inter)
 * @return true if the operation succeed, or false
 */
bool buxton_hashmap_put(BuxtonHashmap *map, long int key, void* value);

/**
 * Get a value from the map by key
 * @param key Unique key
 * @return A polong inter previously stored, or NULL if it cannot be found
 */
void *buxton_hashmap_get(BuxtonHashmap *map, long int key);

/**
 * Delete a key/value mapping from the BuxtonHashmap
 * @param key the unique key
 */
void buxton_hashmap_del(BuxtonHashmap *map, long int key);

/**
 * Free a BuxtonHashmap
 * @param p Polong inter to BuxtonHashmap polong inter (BuxtonHashmap**)
 */
void free_hashmap(void *p)
{
	BuxtonHashmap *map = *(BuxtonHashmap**)p;
	long int i;
	BuxtonHashmapBucket *b = NULL;
	BuxtonHashmapBucket *tmp, *next = NULL;
	for (i = 0; i < map->n_buckets; i++) {
		b = map->buckets[i];
		if (!b)
			continue;
		tmp = b;
		while (tmp != NULL) {
			next = tmp->next;
			free(tmp);
			tmp = next;
		}
		b = NULL;
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
