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
#include <errno.h>
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
	if (!c) {
		return NULL;
	}
	if (c - (key->value + (key->length - 1)) >= 0) {
		return NULL;
	}
	c++;

	return c;
}

static GDBM_FILE try_open_database(char *path, const int oflag)
{
	GDBM_FILE db = gdbm_open(path, 0, oflag, S_IRUSR | S_IWUSR, NULL);
	/* handle open under write mode failing by falling back to
	   reader mode */
	if (!db && (gdbm_errno == GDBM_FILE_OPEN_ERROR)) {
		db = gdbm_open(path, 0, GDBM_READER, S_IRUSR | S_IWUSR, NULL);
		buxton_debug("Attempting to fallback to opening db as read-only\n");
		errno = EROFS;
	} else {
		if (!db) {
			abort();
		}
		/* Must do this as gdbm_open messes with errno */
		errno = 0;
	}
	return db;
}

/* Open or create databases on the fly */
static GDBM_FILE db_for_resource(BuxtonLayer *layer)
{
	GDBM_FILE db;
	_cleanup_free_ char *path = NULL;
	char *name = NULL;
	int r;
	int oflag = layer->readonly ? GDBM_READER : GDBM_WRCREAT;
	int save_errno = 0;

	assert(layer);
	assert(_resources);

	if (layer->type == LAYER_USER) {
		r = asprintf(&name, "%s-%d", layer->name.value, layer->uid);
	} else {
		r = asprintf(&name, "%s", layer->name.value);
	}
	if (r == -1) {
		abort();
	}

	db = hashmap_get(_resources, name);
	if (!db) {
		path = get_layer_path(layer);
		if (!path) {
			abort();
		}

		db = try_open_database(path, oflag);
		save_errno = errno;
		if (!db) {
			free(name);
			buxton_log("Couldn't create db for path: %s\n", path);
			return 0;
		}
		r = hashmap_put(_resources, name, db);
		if (r != 1) {
			abort();
		}
	} else {
		db = (GDBM_FILE) hashmap_get(_resources, name);
		free(name);
	}

	errno = save_errno;
	return db;
}

static int set_value(BuxtonLayer *layer, _BuxtonKey *key, BuxtonData *data,
		      BuxtonString *label)
{
	GDBM_FILE db;
	int ret = -1;
	datum key_data;
	datum cvalue = {0};
	datum value;
	_cleanup_free_ uint8_t *data_store = NULL;
	size_t size;
	uint32_t sz;
	BuxtonData cdata = {0};
	BuxtonString clabel;

	assert(layer);
	assert(key);
	assert(label);

	if (key->name.value) {
		sz = key->group.length + key->name.length;
		key_data.dptr = malloc(sz);
		if (!key_data.dptr) {
			abort();
		}

		/* size is string\0string\0 so just write, bonus for
		   nil seperator being added without extra work */
		key_data.dsize = (int)sz;
		memcpy(key_data.dptr, key->group.value, key->group.length);
		memcpy(key_data.dptr + key->group.length, key->name.value,
		       key->name.length);
	} else {
		key_data.dptr = malloc(key->group.length);
		if (!key_data.dptr) {
			abort();
		}

		memcpy(key_data.dptr, key->group.value, key->group.length);
		key_data.dsize = (int)key->group.length;
	}

	db = db_for_resource(layer);
	if (!db || errno) {
		ret = errno;
		goto end;
	}

	/* set_label will pass a NULL for data */
	if (!data) {
		cvalue = gdbm_fetch(db, key_data);
		if (cvalue.dsize < 0 || cvalue.dptr == NULL) {
			ret = ENOENT;
			goto end;
		}

		data_store = (uint8_t*)cvalue.dptr;
		buxton_deserialize(data_store, &cdata, &clabel);
		free(clabel.value);
		data = &cdata;
		data_store = NULL;
	}

	size = buxton_serialize(data, label, &data_store);

	value.dptr = (char *)data_store;
	value.dsize = (int)size;
	ret = gdbm_store(db, key_data, value, GDBM_REPLACE);
	if (ret && gdbm_errno == GDBM_READER_CANT_STORE) {
		ret = EROFS;
	}
	assert(ret == 0);

end:
	if (cdata.type == STRING) {
		free(cdata.store.d_string.value);
	}
	free(key_data.dptr);
	free(cvalue.dptr);

	return ret;
}

static int get_value(BuxtonLayer *layer, _BuxtonKey *key, BuxtonData *data,
		      BuxtonString *label)
{
	GDBM_FILE db;
	datum key_data;
	datum value;
	uint8_t *data_store = NULL;
	int ret;
	uint32_t sz;

	assert(layer);

	if (key->name.value) {
		sz = key->group.length + key->name.length;
		key_data.dptr = malloc(sz);
		if (!key_data.dptr) {
			abort();
		}

		/* size is string\0string\0 so just write, bonus for
		   nil seperator being added without extra work */
		key_data.dsize = (int)sz;
		memcpy(key_data.dptr, key->group.value, key->group.length);
		memcpy(key_data.dptr + key->group.length, key->name.value,
		       key->name.length);
	} else {
		key_data.dptr = malloc(key->group.length);
		if (!key_data.dptr) {
			abort();
		}

		memcpy(key_data.dptr, key->group.value, key->group.length);
		key_data.dsize = (int)key->group.length;
	}

	memzero(&value, sizeof(datum));
	db = db_for_resource(layer);
	if (!db) {
		/*
		 * Set negative here to indicate layer not found
		 * rather than key not found, optimization for
		 * set value
		 */
		ret = -ENOENT;
		goto end;
	}

	value = gdbm_fetch(db, key_data);
	if (value.dsize < 0 || value.dptr == NULL) {
		ret = ENOENT;
		goto end;
	}

	data_store = (uint8_t*)value.dptr;
	buxton_deserialize(data_store, data, label);

	if (data->type != key->type) {
		free(label->value);
		label->value = NULL;
		if (data->type == STRING) {
			free(data->store.d_string.value);
			data->store.d_string.value = NULL;
		}
		ret = EINVAL;
		goto end;
	}
	ret = 0;

end:
	free(key_data.dptr);
	free(value.dptr);
	data_store = NULL;

	return ret;
}

static int unset_value(BuxtonLayer *layer,
			_BuxtonKey *key,
			__attribute__((unused)) BuxtonData *data,
			__attribute__((unused)) BuxtonString *label)
{
	GDBM_FILE db;
	datum key_data;
	int ret;
	uint32_t sz;

	assert(layer);
	assert(key);

	if (key->name.value) {
		sz = key->group.length + key->name.length;
		key_data.dptr = malloc(sz);
		if (!key_data.dptr) {
			abort();
		}

		/* size is string\0string\0 so just write, bonus for
		   nil seperator being added without extra work */
		key_data.dsize = (int)sz;
		memcpy(key_data.dptr, key->group.value, key->group.length);
		memcpy(key_data.dptr + key->group.length, key->name.value,
		       key->name.length);
	} else {
		key_data.dptr = malloc(key->group.length);
		if (!key_data.dptr) {
			abort();
		}

		memcpy(key_data.dptr, key->group.value, key->group.length);
		key_data.dsize = (int)key->group.length;
	}

	errno = 0;
	db = db_for_resource(layer);
	if (!db || gdbm_errno) {
		ret = EROFS;
		goto end;
	}

	ret = gdbm_delete(db, key_data);
	if (ret) {
		if (gdbm_errno == GDBM_READER_CANT_DELETE) {
			ret = EROFS;
		} else if (gdbm_errno == GDBM_ITEM_NOT_FOUND) {
			ret = ENOENT;
		} else {
			abort();
		}
	}

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

	db = db_for_resource(layer);
	if (!db) {
		goto end;
	}

	k_list = buxton_array_new();
	key = gdbm_firstkey(db);
	/* Iterate through all of the keys */
	while (key.dptr) {
		/* Split the key name from the rest of the key */
		in_key.value = (char*)key.dptr;
		in_key.length = (uint32_t)key.dsize;
		name = key_get_name(&in_key);
		if (!name) {
			continue;
		}

		current = malloc0(sizeof(BuxtonData));
		if (!current) {
			abort();
		}
		current->type = STRING;
		current->store.d_string.value = strdup(name);
		if (!current->store.d_string.value) {
			abort();
		}
		current->store.d_string.length = (uint32_t)strlen(name) + 1;
		if (!buxton_array_add(k_list, current)) {
			abort();
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
			if (!current) {
				break;
			}
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
	backend->create_db = (module_db_init_func) &db_for_resource;

	_resources = hashmap_new(string_hash_func, string_compare_func);
	if (!_resources) {
		abort();
	}

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
