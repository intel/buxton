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

#define _GNU_SOURCE

#include <gdbm.h>
#include <stdlib.h>

#include "../shared/log.h"
#include "../include/bt-daemon.h"
#include "../include/bt-daemon-private.h"
#include "../shared/hashmap.h"

/**
 * GDBM Database Module
 */


static Hashmap *_resources = NULL;

/* Open or create databases on the fly */
static GDBM_FILE _db_for_resource(BuxtonLayer *layer) {
	GDBM_FILE db;
	char *path = NULL;

	db = hashmap_get(_resources, layer->name);
	if (!db) {
		path = get_layer_path(layer);
		if (!path)
			goto end;
		db = gdbm_open(path, 0, GDBM_WRCREAT | GDBM_READER, 0666, NULL);
		if (!db)
			goto end;
		hashmap_put(_resources, layer->name, db);
	}
end:
	if (path)
		free(path);
	return (GDBM_FILE) hashmap_get(_resources, layer->name);
}

static int set_value(BuxtonLayer *layer, const char *key_name, BuxtonData *data)
{
	GDBM_FILE db;
	int ret = true;

	db = _db_for_resource(layer);
	if (!db)
		return false;

	datum key = { (char *)key_name, strlen(key_name) + 1};
	datum value;
	switch (data->type) {
		case STRING:
			value.dsize = strlen(data->store.d_string) + 1 + sizeof(*data);
			break;
		default:
			value.dsize = sizeof(*data);
			break;
	}
	value.dptr = (char *)data;
	ret = gdbm_store(db, key, value, GDBM_INSERT | GDBM_REPLACE);

	return ret;
}

static BuxtonData* get_value(BuxtonLayer *layer, const char *key)
{
	/* TODO: Add code for copying the BuxtonData* first */
	return NULL;
}

_bx_export_ void buxton_module_destroy(BuxtonBackend *backend)
{
	const char *key;
	Iterator iterator;
	GDBM_FILE db;
	backend->set_value = NULL;
	backend->get_value = NULL;

	/* close all gdbm handles */
	HASHMAP_FOREACH_KEY(db, key, _resources, iterator) {
		gdbm_close(db);
	}
	hashmap_free(_resources);
	_resources = NULL;
}

_bx_export_ int buxton_module_init(BuxtonBackend *backend)
{
	/* Point the struct methods back to our own */
	backend->set_value = &set_value;
	backend->get_value = &get_value;

	_resources = hashmap_new(string_hash_func, string_compare_func);
	return 0;
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
