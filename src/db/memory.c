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

#include <assert.h>

#include "hashmap.h"
#include "log.h"
#include "buxton.h"
#include "backend.h"
#include "util.h"

/**
 * Memory Database Module
 *
 * Used for quick testing and debugging of Buxton, to ensure protocol
 * and direct access are working as intended.
 * Note this is not persistent.
 */


static Hashmap *_resources;

/* Return existing hashmap or create new hashmap on the fly */
static Hashmap *_db_for_resource(BuxtonLayer *layer)
{
	Hashmap *db;
	char *name = NULL;
	int r;

	assert(layer);
	assert(_resources);

	if (layer->type == LAYER_USER)
		r = asprintf(&name, "%s-%d", layer->name.value, layer->uid);
	else
		r = asprintf(&name, "%s", layer->name.value);
	if (r == -1)
		return NULL;

	db = hashmap_get(_resources, name);
	if (!db) {
		db = hashmap_new(string_hash_func, string_compare_func);
		hashmap_put(_resources, name, db);
	} else {
		free(name);
	}

	return db;
}

static int32_t set_value(BuxtonLayer *layer, _BuxtonKey *key, BuxtonData *data,
		      BuxtonString *label)
{
	Hashmap *db;
	BuxtonArray *array = NULL;
	BuxtonData *data_copy = NULL;
	BuxtonString *label_copy = NULL;
	char *full_key = NULL;

	assert(layer);
	assert(key);
	assert(data);
	assert(label);

	db = _db_for_resource(layer);
	if (!db)
		goto fail;

	if (key->name.value) {
		if (asprintf(&full_key, "%s%s", key->group.value, key->name.value) == -1)
			abort();
	} else {
		full_key = strdup(key->group.value);
		if (!full_key)
			abort();
	}

	array = buxton_array_new();
	if (!array)
		abort();
	data_copy = malloc0(sizeof(BuxtonData));
	if (!data_copy)
		abort();
	label_copy = malloc0(sizeof(BuxtonString));
	if (!label_copy)
		abort();

	buxton_data_copy(data, data_copy);
	if (!data_copy)
		abort();
	if (!buxton_string_copy(label, label_copy))
		abort();
	if (!buxton_array_add(array, data_copy))
		abort();
	if (!buxton_array_add(array, label_copy))
		abort();
	if (!buxton_array_add(array, full_key))
		abort();

	//FIXME replace value if already in db
	hashmap_put(db, full_key, array);

	return 0;

fail:
	buxton_array_free(&array, NULL);
	if (data_copy && data_copy->type == STRING &&
	    data_copy->store.d_string.value)
		free(data_copy->store.d_string.value);
	free(data_copy);
	if (label_copy && label_copy->value)
		free(label_copy->value);
	free(label_copy);
	free(full_key);

	return -1;
}

static bool get_value(BuxtonLayer *layer, _BuxtonKey *key, BuxtonData *data,
		      BuxtonString *label)
{
	Hashmap *db;
	BuxtonArray *stored;
	BuxtonData *d;
	BuxtonString *l;
	char *full_key;

	assert(layer);
	assert(key);
	assert(label);
	assert(data);

	db = _db_for_resource(layer);
	if (!db)
		return false;

	//FIXME leaking full_key
	if (key->name.value) {
		if (asprintf(&full_key, "%s%s", key->group.value, key->name.value) == -1)
			abort();
	} else {
		full_key = strdup(key->group.value);
		if (!full_key)
			abort();
	}

	stored = (BuxtonArray *)hashmap_get(db, full_key);
	if (!stored)
		return false;
	d = buxton_array_get(stored, 0);
	if (d->type != key->type)
		return false;

	buxton_data_copy(d, data);
	//FIXME have buxton_data_copy return a bool

	l = buxton_array_get(stored, 1);
	if (!buxton_string_copy(l, label)) {
		abort();
	}

	return true;
}

static bool unset_value(BuxtonLayer *layer,
			_BuxtonKey *key,
			__attribute__((unused)) BuxtonData *data,
			__attribute__((unused)) BuxtonString *label)
{
	Hashmap *db;
	BuxtonArray *stored;
	BuxtonData *d;
	BuxtonString *l;
	char *full_key;

	assert(layer);
	assert(key);

	db = _db_for_resource(layer);
	if (!db)
		return false;

	//FIXME fix full_key leak
	if (key->name.value) {
		if (asprintf(&full_key, "%s%s", key->group.value, key->name.value) == -1)
			abort();
	} else {
		full_key = strdup(key->group.value);
		if (!full_key)
			abort();
	}

	/* test if the value exists */
	stored = (BuxtonArray *)hashmap_get(db, full_key);
	if (!stored)
		return false;
	/* free the data */
	d = buxton_array_get(stored, 0);
	data_free(d);
	l = buxton_array_get(stored, 1);
	string_free(l);
	/* Now remove value from the database */
	hashmap_remove(db, key->group.value);
	buxton_array_free(&stored, NULL);

	return true;
}

_bx_export_ void buxton_module_destroy(void)
{
	const char *key1, *key2;
	Iterator iteratori, iteratoro;
	Hashmap *map;
	BuxtonArray *array;
	BuxtonData *d;
	BuxtonString *l;

	/* free all hashmaps */
	HASHMAP_FOREACH_KEY(map, key1, _resources, iteratoro) {
		HASHMAP_FOREACH_KEY(array, key2, map, iteratori) {
			hashmap_remove(map, key2);
			d = buxton_array_get(array, 0);
			data_free(d);
			l = buxton_array_get(array, 1);
			string_free(l);
			buxton_array_free(&array, NULL);
		}
		hashmap_remove(_resources, key1);
		hashmap_free(map);
		free((void *)key1);
		map = NULL;
	}
	hashmap_free(_resources);
	_resources = NULL;
}

_bx_export_ bool buxton_module_init(BuxtonBackend *backend)
{

	assert(backend);

	/* Point the struct methods back to our own */
	backend->set_value = &set_value;
	backend->get_value = &get_value;
	backend->unset_value = &unset_value;

	_resources = hashmap_new(string_hash_func, string_compare_func);
	if (!_resources)
		abort();
	return true;
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
