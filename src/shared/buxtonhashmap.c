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

#include "buxtonhashmap.h"

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>


/* Initial bucket size */
#define INITIAL_SIZE 31

/* When we're "full", 75% */
#define FULL_FACTOR 0.75

/* Multiple to increase bucket size by */
#define INCREASE_FACTOR 4

/* Note we're incredibly unlikely to hit a hash collision situation in this
 * map, however in the instance that we do, a simple linked list mechanism
 * is used, optimized by tracking the tail.
 *
 * Unfortunately the addition of the 'occ' bool adds tiny overhead, however this
 * is the correct behaviour if you wish to store integers as keys or values.
 * The hashed pointer for 0 is NULL, and performing null checks on keys intentionally
 * set to 0 (or even values) will fail, as ((void *)0) *is* NULL.
 */

/**
 * Internal hashmap cache bucket
 */
typedef struct HKVEntry {
	HashKey key; /**<Root or subnode key */
	HashValue value; /**<Root or subnode value */
	struct HKVEntry *next; /**<Next item in this chain */
	struct HKVEntry *tail; /**<Tail of the chain */
	bool occ; /**Occupied status, to enable void inclusions */
} HKVEntry;


/**
 * BuxtonHashmap structure
 */
struct BuxtonHashmap {
	unsigned int n_buckets; /**<Current number of buckets */

	HKVEntry *entries; /**<Our buckets */
	unsigned int size; /**<Current size (number of items) */
	bool resizing; /**<Protection against recursive resize */

	hash_compare_func compare; /**<Hash comparison function */
	hash_create_func create; /**<Hash creation function */

	hash_free_func key_free; /**<Callback for freeing keys */
	hash_free_func value_free; /**<Callback for freeing values */

	bool can_resize; /** Whether resizing is permitted */
};

void map_resize(BuxtonHashmap **self);

/* Default string hash */
unsigned string_hash(const HashKey key)
{
        unsigned hash = 5381;
        const signed char *c;

        /* DJB's hash function */

	for (c = key; *c; c++) {
		hash = (hash << 5) + hash + (unsigned) *c;
	}
	return hash;
}

unsigned simple_hash(const HashKey source)
{
	return UNHASH_KEY(source);
}

bool string_compare(const HashKey l, const HashKey r)
{
	if (!l || !r) {
		return false;
	}
	return (strcmp(l,r) == 0);
}


bool simple_compare(const HashKey l, const HashKey r)
{
	return (l == r);
}


BuxtonHashmap *buxton_hashmap_new_internal(uint size, hash_compare_func compare, hash_create_func create, hash_free_func key_free, hash_free_func value_free, bool can_resize)
{
	BuxtonHashmap *ret = NULL;
	HKVEntry *entries = NULL;

	ret = calloc(1, sizeof(BuxtonHashmap));
	if (!ret) {
		return NULL;
	}

	entries = calloc(size, sizeof(HKVEntry));
	if (!entries) {
		/* Leak from previous allocation but its likely we're dead
		 * in the water now anyway. */
		return NULL;
	}
	ret->entries = entries;
	ret->n_buckets = size;
	ret->size = 0;
	ret->resizing = false;
	ret->compare = compare;
	ret->create = create;
	ret->key_free = key_free;
	ret->value_free = value_free;
	ret->can_resize = can_resize;

	entries = NULL;

	return ret;
}

BuxtonHashmap *buxton_hashmap_new(hash_free_func key_free, hash_free_func value_free)
{
	return buxton_hashmap_new_internal(INITIAL_SIZE, simple_compare, simple_hash, key_free, value_free, true);
}

BuxtonHashmap *buxton_hashmap_new_full(hash_compare_func compare, hash_create_func create, hash_free_func key_free, hash_free_func value_free)
{
	return buxton_hashmap_new_internal(INITIAL_SIZE, compare, create, key_free, value_free, true);
}

void buxton_hashmap_free(BuxtonHashmap *map)
{
	HKVEntry *entry = NULL;

	if (!map) {
		return;
	}

	/* Determine if linked nodes need cleaning */
	for (uint i = 0; i < map->n_buckets; i++) {
		entry = &(map->entries[i]);
		if (map->key_free && entry->occ) {
			map->key_free(entry->key);
		}
		if (map->value_free && entry->occ) {
			map->value_free(entry->value);
		}
		entry->key = NULL;
		entry->value = NULL;
		if (!entry->next) {
			continue;
		}
		HKVEntry *next = map->entries[i].next;
		HKVEntry *tmp = NULL;
		/* Visit linked nodes */
		while (next) {
			tmp = next->next;
			if (map->key_free) {
				map->key_free(next->key);
			}
			if (map->value_free) {
				map->value_free(next->value);
			}
			free(next);
			next = tmp;
		}
	}

	free(map->entries);
	free(map);
}

bool buxton_hashmap_put(BuxtonHashmap **_self, HashKey key, HashValue value)
{
	BuxtonHashmap *self = *_self;

	if (!_self) {
		return false;
	}

	HKVEntry *entry;
	unsigned hash = self->create(key) % self->n_buckets;
	HKVEntry *head = NULL;

	/* Potential for resize? (increase only right now) */
	if (self->can_resize) {
		float used = (float)self->size / (float)self->n_buckets;
		if (!self->resizing && used > FULL_FACTOR) {
			map_resize(&self);
		}
	}

	entry = &(self->entries[hash]);
	head = &(self->entries[hash]);

	/* handle collision */
	if (entry->occ) {
		/* Attempt to find an existing container.. */
		bool dupe = false;

		while (entry) {
			if (entry->occ && self->compare(entry->key, key)) {
				dupe = true;
				break;
			}
			entry = entry->next;
		}
		if (!dupe) {
			/* Append to tail */
			entry = calloc(1, sizeof(HKVEntry));
			if (!entry) {
				return false;
			}
			/* Shove to the tail */
			if (head->tail) {
				head->tail->next = entry;
				head->tail = entry;
			} else {
				head->next = entry;
				head->tail = entry;
			}
			assert(head->tail != NULL);
			self->size++;
			entry->key = key;
			entry->occ = true;
			entry->value = value;
		} else {
			/* Occupied node, free old value and replace */	
			if (self->value_free) {
				self->value_free(entry->value);
			}
			entry->value = value;
		}
	} else {
		/* Unoccupied root node */
		entry->key = key;
		entry->value = value;
		entry->occ = true;
		self->size++;
	}

	*_self = self;

	return true;
}

void* buxton_hashmap_get(BuxtonHashmap *self, HashKey key)
{
	HKVEntry *entry;
	HKVEntry *next = NULL;

	if (!self || !key) {
		return NULL;
	}

	unsigned hash = self->create(key) % self->n_buckets;
	entry = &(self->entries[hash]);

	/* Root node */
	if (entry->occ && self->compare(key, entry->key)) {
		return (void*)entry->value;
	}
	next = entry;
	while (next) {
		/* Found in subnode on chain */
		if (entry->occ && self->compare(key, next->key)) {
			return (void*)next->value;
		}
		next = next->next;
	}
	return NULL;
}

/**
 * Actually easier than rehashing the world, copy into a new map
 */
void map_resize(BuxtonHashmap **s)
{
	BuxtonHashmap *self = *s;

	BuxtonHashmap *dummy = NULL;

	uint new_size = self->n_buckets * INCREASE_FACTOR;

	dummy = buxton_hashmap_new_internal(new_size, self->compare, self->create, self->key_free, self->value_free, self->can_resize);
	if (!dummy) {
		/* We're screwed here. */
		abort();
	}

	/* Just push all of our own entries across */
	dummy->resizing = true;
	for (uint i = 0; i< self->size; i++)
	{
		HKVEntry *entry = &(self->entries[i]);

		entry = entry;
		while (entry) {
			buxton_hashmap_put(&dummy, entry->key, entry->value);
			entry = entry->next;
		}
	}

	dummy->resizing = false;
	/* Avoid double-tap on killing original hashmap */
	self->value_free = NULL;
	self->key_free = NULL;

	buxton_hashmap_free(*s);
	*s = dummy;
}

/* Stop looking. Now. */
HashKey buxton_hashmap_iter(BuxtonHashmap *map, BuxtonHashmapIter *iter, HashValue *value)
{
	if (!map) {
		/* Prevent iteration of no map */
		if (iter) {
			iter->done = true;
		}
		return NULL;
	}

	BuxtonHashmapIter liter = *iter;
	HKVEntry *entry = NULL;
	bool found_next = false;

head_check:
	if (liter.bucket > map->n_buckets) {
		goto end;
	}

	if (liter.head) {
		entry = &(map->entries[liter.bucket]);
		if (!entry->occ) {
			for (uint i = liter.bucket; i < map->n_buckets; i++) {
				if (map->entries[i].occ) {
					entry = &(map->entries[i]);
					liter.bucket = i;
					found_next = true;
					break;
				}
			}
		} else {
			found_next = true;
		}
		liter.last = entry;
		liter.head = false;
		/* Quick bail */
		if (!found_next) {
			goto end;
		}
	} else {
		if (!liter.last) {
			if (map->entries[liter.bucket].next) {
				if (map->entries[liter.bucket].next->occ) {
					liter.last = map->entries[liter.bucket].next;
					entry = map->entries[liter.bucket].next;
				}
			}
			if (!liter.last) {
				liter.bucket += 1;
				liter.head = true;
				goto head_check;
			}
			entry = liter.last;
		} else {
			entry = (HKVEntry*)liter.last;
			liter.last = entry->next;
			entry = liter.last;
			/* Skip to next bucket on next run */
			if (!liter.last) {
				liter.bucket += 1;
				liter.head = true;
				goto head_check;
			}
		}
	}

	*iter = liter;

	if (entry && entry->occ) {
		*value = entry->value;
		return entry->key;
	}
	if (liter.bucket < map->n_buckets) {
		liter.bucket += 1;
		goto head_check;
	}
end:
	iter->done = true;
	return NULL;
}

bool buxton_hashmap_remove(BuxtonHashmap *self, HashKey key)
{
	HKVEntry *entry, *head;
	HKVEntry *prev = NULL, *next = NULL;

	if (!self || !key) {
		return false;
	}

	unsigned hash = self->create(key) % self->n_buckets;
	entry = &(self->entries[hash]);
	head = entry;

	/* Trivial head removal */
	if (self->compare(key, entry->key)) {
		if (self->key_free) {
			self->key_free(entry->key);
		}
		if (self->value_free) {
			self->value_free(entry->value);
		}
		entry->value = NULL;
		entry->key = NULL;
		entry->occ = false;
		self->size -= 1;
		return true;
	}
	next = entry;
	prev = next;
	while (next) {
		/* Relink list to mask out old child */
		if (self->compare(key, next->key)) {
			prev->next = next->next;
			head->tail = next->next;
			if (self->key_free) {
				self->key_free(next->key);
			}
			if (self->value_free) {
				self->value_free(next->value);
			}
			next->value = NULL;
			next->key = NULL;
			free(next);
			self->size -= 1;
			return true;
		}
		prev = next;
		next = next->next;
	}
	return false;
}

unsigned int buxton_hashmap_size(BuxtonHashmap *map)
{
	if (map) {
		return map->size;
	}
	return 0;
}

bool buxton_hashmap_contains(BuxtonHashmap *map, HashKey key)
{
	return buxton_hashmap_get(map,key) != NULL;
}

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
