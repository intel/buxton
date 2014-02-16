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
#include "hashmap.h"
#include "serialize.h"
#include "util.h"

/**
 * GDBM Database Module
 */


static Hashmap *_resources = NULL;

static char *key_get_name(BuxtonString *key)
{
	char *c;

	c = strchr(key->value, 0);
	if (!c)
		return NULL;
	if (c - (key->value + (key->length - 1)) >= 0)
		return NULL;
	c++;

	return c;
}

/* Open or create databases on the fly */
static GDBM_FILE _db_for_resource(BuxtonLayer *layer)
{
	GDBM_FILE db;
	_cleanup_free_ char *path = NULL;
	char *name = NULL;
	int r;

	assert(layer);
	assert(_resources);

	if (layer->type == LAYER_USER)
		r = asprintf(&name, "%s-%d", layer->name.value, layer->uid);
	else
		r = asprintf(&name, "%s", layer->name.value);
	if (r == -1)
		return 0;

	db = hashmap_get(_resources, name);
	if (!db) {
		path = get_layer_path(layer);
		if (!path) {
			free(name);
			return 0;
		}
		db = gdbm_open(path, 0, GDBM_WRCREAT, 0600, NULL);
		if (!db) {
			free(name);
			buxton_log("Couldn't create db for path: %s\n", path);
			return 0;
		}
		hashmap_put(_resources, name, db);
	} else {
		db = (GDBM_FILE) hashmap_get(_resources, name);
		free(name);
	}

	return db;
}

static bool set_value(BuxtonLayer *layer, _BuxtonKey *key, BuxtonData *data,
		      BuxtonString *label)
{
	GDBM_FILE db;
	int ret = -1;
	datum key_data;
	datum value;
	_cleanup_free_ uint8_t *data_store = NULL;
	size_t size;
	uint32_t sz;

	assert(layer);
	assert(key);
	assert(data);
	assert(label);

	if (key->name.value) {
		sz = key->group.length + key->name.length;
		key_data.dptr = malloc(sz);
		if (!key_data.dptr)
			return false;

		/* size is string\0string\0 so just write, bonus for
		   nil seperator being added without extra work */
		key_data.dsize = (int)sz;
		memcpy(key_data.dptr, key->group.value, key->group.length);
		memcpy(key_data.dptr + key->group.length, key->name.value,
		       key->name.length);
	} else {
		key_data.dptr = malloc(key->group.length);
		if (!key_data.dptr)
			return false;

		memcpy(key_data.dptr, key->group.value, key->group.length);
		key_data.dsize = (int)key->group.length;
	}

	db = _db_for_resource(layer);
	if (!db)
		goto end;

	size = buxton_serialize(data, label, &data_store);
	if (size < BXT_MINIMUM_SIZE)
		goto end;

	value.dptr = (char *)data_store;
	value.dsize = (int)size;
	ret = gdbm_store(db, key_data, value, GDBM_REPLACE);

end:
	free(key_data.dptr);

	if (ret == -1)
		return false;
	return true;
}

static bool get_value(BuxtonLayer *layer, _BuxtonKey *key, BuxtonData *data,
		      BuxtonString *label)
{
	GDBM_FILE db;
	datum key_data;
	datum value;
	uint8_t *data_store = NULL;
	bool ret = false;
	uint32_t sz;

	assert(layer);

	if (key->name.value) {
		sz = key->group.length + key->name.length;
		key_data.dptr = malloc(sz);
		if (!key_data.dptr)
			return false;

		/* size is string\0string\0 so just write, bonus for
		   nil seperator being added without extra work */
		key_data.dsize = (int)sz;
		memcpy(key_data.dptr, key->group.value, key->group.length);
		memcpy(key_data.dptr + key->group.length, key->name.value,
		       key->name.length);
	} else {
		key_data.dptr = malloc(key->group.length);
		if (!key_data.dptr)
			return false;

		memcpy(key_data.dptr, key->group.value, key->group.length);
		key_data.dsize = (int)key->group.length;
	}

	memzero(&value, sizeof(datum));
	db = _db_for_resource(layer);
	if (!db)
		goto end;

	value = gdbm_fetch(db, key_data);
	if (value.dsize < 0 || value.dptr == NULL)
		goto end;

	data_store = (uint8_t*)value.dptr;
	if (!buxton_deserialize(data_store, data, label))
		goto end;

	ret = true;

end:
	free(key_data.dptr);
	free(value.dptr);
	data_store = NULL;

	return ret;
}

static bool unset_value(BuxtonLayer *layer,
			_BuxtonKey *key,
			__attribute__((unused)) BuxtonData *data,
			__attribute__((unused)) BuxtonString *label)
{
	GDBM_FILE db;
	datum key_data;
	bool ret = false;
	int rc;
	uint32_t sz;

	assert(layer);
	assert(key);

	if (key->name.value) {
		sz = key->group.length + key->name.length;
		key_data.dptr = malloc(sz);
		if (!key_data.dptr)
			return false;

		/* size is string\0string\0 so just write, bonus for
		   nil seperator being added without extra work */
		key_data.dsize = (int)sz;
		memcpy(key_data.dptr, key->group.value, key->group.length);
		memcpy(key_data.dptr + key->group.length, key->name.value,
		       key->name.length);
	} else {
		key_data.dptr = key->group.value;
		key_data.dsize = (int)key->group.length;
	}

	db = _db_for_resource(layer);
	if (!db)
		goto end;

	/* Negative value means the key wasn't found */
	rc = gdbm_delete(db, key_data);
	ret = rc == 0 ? true : false;

end:
	free(key_data.dptr);

	return ret;
}

static bool list_keys(BuxtonLayer *layer,
		      BuxtonArray **list)
{
	GDBM_FILE db;
	datum key, nextkey;
	BuxtonArray *k_list = NULL;
	BuxtonData *current = NULL;
	BuxtonString in_key;
	char *name;
	bool ret = false;

	assert(layer);

	db = _db_for_resource(layer);
	if (!db)
		goto end;

	k_list = buxton_array_new();
	key = gdbm_firstkey(db);
	/* Iterate through all of the keys */
	while (key.dptr) {
		/* Split the key name from the rest of the key */
		in_key.value = (char*)key.dptr;
		in_key.length = (uint32_t)key.dsize;
		name = key_get_name(&in_key);
		if (!name)
			continue;

		current = malloc0(sizeof(BuxtonData));
		if (!current)
			goto end;
		current->type = STRING;
		current->store.d_string.value = strdup(name);
		if (!current->store.d_string.value)
			goto end;
		current->store.d_string.length = (uint32_t)strlen(name) + 1;
		if (!buxton_array_add(k_list, current)) {
			buxton_log("Unable to add key to to gdbm list\n");
			goto end;
		}

		/* Visit the next key */
		nextkey = gdbm_nextkey(db, key);
		free(key.dptr);
		key = nextkey;
	}

	/* Pass ownership of the array to the caller */
	*list = k_list;
	ret = true;

end:
	if (!ret && k_list) {
		for (uint16_t i = 0; i < k_list->len; i++) {
			current = buxton_array_get(k_list, i);
			if (!current)
				break;
			free(current->store.d_string.value);
			free(current);
		}
		buxton_array_free(&k_list, NULL);
	}
	return ret;
}

_bx_export_ void buxton_module_destroy(void)
{
	const char *key;
	Iterator iterator;
	GDBM_FILE db;

	/* close all gdbm handles */
	HASHMAP_FOREACH_KEY(db, key, _resources, iterator) {
		hashmap_remove(_resources, key);
		gdbm_close(db);
		free((void *)key);
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
	backend->list_keys = &list_keys;
	backend->unset_value = &unset_value;

	_resources = hashmap_new(string_hash_func, string_compare_func);
	if (!_resources)
		return false;

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
