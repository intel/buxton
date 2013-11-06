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
#include "bt-daemon.h"
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

	assert(layer);
	assert(_resources);

	db = hashmap_get(_resources, layer->name);
	if (!db) {
		db = hashmap_new(string_hash_func, string_compare_func);
		hashmap_put(_resources, layer->name, db);
	}

	return (Hashmap*) hashmap_get(_resources, layer->name);
}

static bool set_value(BuxtonLayer *layer, const char *key, BuxtonData *data)
{
	Hashmap *db;
	assert(layer);
	assert(key);
	assert(data);

	db = _db_for_resource(layer);
	if (!db)
		return false;

	hashmap_put(db, key, data);
	return true;
}

static bool get_value(BuxtonLayer *layer, const char *key, BuxtonData *data)
{
	Hashmap *db;
	BuxtonData *stored;

	assert(layer);
	assert(key);

	db = _db_for_resource(layer);
	if (!db)
		return false;

	stored = (BuxtonData*)hashmap_get(db, key);
	if (!stored)
		return false;
	data = stored;
	return true;
}

_bx_export_ void buxton_module_destroy(void)
{
	const char *key;
	Iterator iterator;
	Hashmap *map;

	/* free all hashmaps */
	HASHMAP_FOREACH_KEY(map, key, _resources, iterator) {
		hashmap_free(map);
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

	_resources = hashmap_new(string_hash_func, string_compare_func);
	if (!_resources)
		return false;
	return true;
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
