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
#include <gdbm.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "bt-daemon.h"
#include "hashmap.h"
#include "serialize.h"
#include "util.h"

/**
 * GDBM Database Module
 */


static Hashmap *_resources = NULL;

/* Open or create databases on the fly */
static GDBM_FILE _db_for_resource(BuxtonLayer *layer)
{
	GDBM_FILE db;
	_cleanup_free_ char *path = NULL;

	assert(layer);
	assert(_resources);

	db = hashmap_get(_resources, layer->name.value);
	if (!db) {
		path = get_layer_path(layer);
		if (!path)
			return 0;
		db = gdbm_open(path, 0, GDBM_WRCREAT, 0600, NULL);
		if (!db) {
			buxton_log("Couldn't create db for path: %s\n", path);
			return 0;
		}
		hashmap_put(_resources, layer->name.value, db);
	}

	return (GDBM_FILE) hashmap_get(_resources, layer->name.value);
}

static bool set_value(BuxtonLayer *layer, BuxtonString *key_name, BuxtonData *data)
{
	GDBM_FILE db;
	int ret;
	datum key;
	datum value;
	_cleanup_free_ uint8_t *data_store = NULL;
	size_t size;

	assert(layer);
	assert(key_name);
	assert(data);

	key.dptr = key_name->value;
	key.dsize = (int)key_name->length;

	db = _db_for_resource(layer);
	if (!db)
		return false;

	size = buxton_serialize(data, &data_store);
	if (size < BXT_MINIMUM_SIZE)
		return false;

	value.dptr = (char*)data_store;
	value.dsize = (int)size;
	ret = gdbm_store(db, key, value, GDBM_REPLACE);

	if (ret == -1)
		return false;
	return true;
}

static bool get_value(BuxtonLayer *layer, BuxtonString *key_name, BuxtonData *data)
{
	GDBM_FILE db;
	datum key;
	datum value;
	uint8_t *data_store = NULL;
	bool ret = false;

	assert(layer);
	assert(key_name);

	key.dptr = key_name->value;
	key.dsize = (int)key_name->length;

	memset(&value, 0, sizeof(datum));
	db = _db_for_resource(layer);
	if (!db)
		goto end;

	value = gdbm_fetch(db, key);
	if (value.dsize < 0 || value.dptr == NULL)
		goto end;

	data_store = (uint8_t*)value.dptr;
	if (!buxton_deserialize(data_store, data))
		goto end;

	ret = true;

end:
	free(value.dptr);
	data_store = NULL;

	return ret;
}

_bx_export_ void buxton_module_destroy(void)
{
	const char *key;
	Iterator iterator;
	GDBM_FILE db;

	/* close all gdbm handles */
	HASHMAP_FOREACH_KEY(db, key, _resources, iterator) {
		gdbm_close(db);
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
