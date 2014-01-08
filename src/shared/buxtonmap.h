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
#include "buxtonlist.h"

/**
 * Use increments of 32 for safety
 */
#define BUXTON_HASHMAP_SIZE 32
#define _cleanup_hashmap_ __attribute__ ((cleanup(buxton_hashmap_free)))

/**
 * A simple Hashmap backed by BuxtonList
 */
typedef struct BuxtonHashmap {
	int n_buckets; /**<Number of buckets */
	BuxtonList **buckets; /**<BuxtonList array */
	int n_elements; /**<Number of elements */
	bool auto_free_key; /**<Free all keys */
	bool auto_free_value; /**<Free all elements */
} BuxtonHashmap;

/**
 * Create a new BuxtonHashmap
 * @note a bigger size will help prevent hash collisions
 * 
 * @param size Requested size for the BuxtonMap
 * @param auto_free Whether to automatically free stored keys
 * @param auto_free Whether to automatically free stored members
 * @return a new map, or NULL
 */
BuxtonHashmap *buxton_hashmap_new(int size, bool auto_free_key, bool auto_free_value);

/**
 * Add a value to the map by key
 * @param key Unique key
 * @param value Value to store (pointer)
 * @return true if the operation succeed, or false
 */
bool buxton_hashmap_put(BuxtonHashmap *map, void* key, void* value);

/**
 * Add a value to the map by key
 * @param key Unique key
 * @param value Value to store (pointer)
 * @return true if the operation succeed, or false
 */
bool buxton_hashmap_puti(BuxtonHashmap *map, int key, void* value);

/**
 * Get a value from the map by key
 * @param key Unique key
 * @return A pointer previously stored, or NULL if it cannot be found
 */
void *buxton_hashmap_get(BuxtonHashmap *map, void* key);

/**
 * Get a value from the map by key
 * @param key Unique key
 * @return A pointer previously stored, or NULL if it cannot be found
 */
void *buxton_hashmap_geti(BuxtonHashmap *map, int key);

/**
 * Delete a key/value mapping from the BuxtonMap
 * @param key the unique key
 */
void buxton_hashmap_del(BuxtonHashmap *map, void* key);

/**
 * Delete a key/value mapping from the BuxtonMap
 * @param key the unique key
 */
void buxton_hashmap_deli(BuxtonHashmap *map, int key);

/**
 * Free a BuxtonMap
 * @param p Pointer to BuxtonMap pointer (BuxtonMap**)
 */
void buxton_hashmap_free(void *p);

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
