/*
 * This file is part of buxton.
 *
 * Copyright (C) 2014 Intel Corporation
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

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct BuxtonHashmap BuxtonHashmap;
typedef const void* HashKey;
typedef const void* HashValue;

/* Convert between uint and void* */
#define HASH_KEY(x) ((void*)((uintptr_t)(x)))
#define HASH_VALUE(x) HASH_KEY(x)
#define UNHASH_KEY(x) ((unsigned int)((uintptr_t)(x)))
#define UNHASH_VALUE(x) UNHASH_KEY(x)

/**
 * Hash comparison function definition
 *
 * @param l First value to compare
 * @param r Second value to compare
 *
 * @return true if l and r both match, otherwise false
 */
typedef bool (*hash_compare_func)(const HashKey l, const HashKey r);

/**
 * Hash creation function definition
 *
 * @param key Key to generate a hash for
 *
 * @return an unsigned integer hash result
 */
typedef unsigned (*hash_create_func)(const HashKey key);

/**
 * Callback definition to free keys and values
 *
 * @param p Single-depth pointer to either a key or value that should be freed
 */
typedef void (*hash_free_func)(const void *p);

/**
 * Default hash/comparison functions
 *
 * @note These are only used for comparison of *keys*, not values. Unless
 * explicitly using string keys, you should most likely stick with the default
 * simple_hash and simple_compare functions
 */
unsigned string_hash(const HashKey key);

unsigned simple_hash(const HashKey source);

bool string_compare(const HashKey l, const HashKey r);

bool simple_compare(const HashKey l, const HashKey r);


/**
 * Track iteration operations
 */
typedef struct BuxtonHashmapIter {
	unsigned int bucket; /**<Current bucket index */
	bool head; /**<Whether we're at the head or not */
	void *last; /**<Last collision node encountered */
	bool done; /**<Indicates the iteration is complete */
} BuxtonHashmapIter;



/**
 * Utility macro to perform a foreach on every key and value in the HashMap
 *
 * @param m Valid BuxtonHashmap instance
 * @param i Name of local BuxtonHashmapIter
 * @param k Name of local HashKey
 * @param v Name of local HashValue
 */
#define BUXTON_HASHMAP_FOREACH(m,i,k,v) \
	BuxtonHashmapIter i = {0, true, NULL, false} ; HashKey k = NULL; HashValue v = NULL; k = buxton_hashmap_iter(m, &i, &v); for (; !i.done; k = buxton_hashmap_iter(m, &i, &v))

/**
 * Construct a new BuxtonHashmap
 *
 * @param key_free Function to free keys, or NULL for none
 * @param value_free Function to free values, or NULL for none
 */
BuxtonHashmap *buxton_hashmap_new(hash_free_func key_free, hash_free_func value_free);

/**
 * Construct a new BuxtonHashmap with different compare/hash functions
 *
 * @param compare Comparison function
 * @param create Hash creation function
 * @param key_free Function to free keys, or NULL for none
 * @param value_free Function to free values, or NULL for none
 */
BuxtonHashmap *buxton_hashmap_new_full(hash_compare_func compare, hash_create_func create, hash_free_func key_free, hash_free_func value_free);

/**
 * Destroy a BuxtonHashmap
 *
 * @param map A valid BuxtonHashmap instance to destroy
 */
void buxton_hashmap_free(BuxtonHashmap *map);

/**
 * Iterate a BuxtonHashmap, visiting all keys and values
 *
 * @note This function should be called until iter has been marked as "done". You
 * will probably want to use BUXTON_HASHMAP_FOREACH for convenience.
 *
 * @param map A valid BuxtonHashmap instance
 * @param iter An iterator to store current iteration progress in
 * @param value Pointer to store the current HashValue in
 *
 * @return On each successful iteration the current key is set and returned
 */
HashKey buxton_hashmap_iter(BuxtonHashmap *map, BuxtonHashmapIter *iter, HashValue *value);

/**
 * Add data to the BuxtonHashmap
 *
 * @note The only instance in which this can possibly fail is OOM issues or an uninitialised
 * map instance, in which case you should likely just abort(). You must also pass a pointer to
 * the map pointer, as in some instances it will be modified with a new BuxtonHashmap instance
 * that has been increased in size. This is expected behaviour and the map will resize when it
 * hits 75% full, increasing in size each time.
 *
 * If an existing key is found, the value will be overwritten automatically. If you have specified
 * value_free and key_free functions during creation, this will automatically be executed here.
 *
 * @param map A valid BuxtonHashmap instance
 * @param key The key to set
 * @param value Value to set
 *
 * @return True, if this succeeded. 
 */
bool buxton_hashmap_put(BuxtonHashmap **map, HashKey key, HashValue value);

/**
 * Retrieve data from BuxtonHashmap
 *
 * @param map A valid BuxtonHashmap instance
 * @param key The key to retrieve data for
 *
 * @return Value of key if found, otherwise NULL
 */
void* buxton_hashmap_get(BuxtonHashmap *map, HashKey key);

/**
 * Unset an item in the BuxtonHashmap
 *
 * @note if you have specified value_free and key_free functions, they will
 * be called here if the key is found.
 *
 * @param map A valid BuxtonHashmap instance
 * @param key The key to unset
 *
 * @return True if the key was actually removed from the map, otherwise false
 */
bool buxton_hashmap_remove(BuxtonHashmap *map, HashKey key);


/**
 * Get the current size of a BuxtonHashmap (number of items)
 * @param map A valid BuxtonHashmap instance
 */
unsigned int buxton_hashmap_size(BuxtonHashmap *map);

/**
 * Determine if the BuxtonHashmap contains the given key
 *
 * @note It is quicker to use buxton_hashmap_get and operate on the premise
 * that NULL will be returned if the key was not found, saving excess lookups.
 *
 * @param map A valid BuxtonHashmap instance
 * @param key A key to search for
 *
 * @return true if the key is set, otherwise false
 */
bool buxton_hashmap_contains(BuxtonHashmap *map, HashKey key);

/*
 * Editor modelines  -	http://www.wireshark.org/tools/modelines.html
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
