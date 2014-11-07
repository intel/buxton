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

#include "buxtonhashmap.h"
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


static BuxtonHashmap *_resources;

/** Cleanup handler */
static void memory_vfree(const void *p)
{
	BuxtonArray *array = NULL;
	BuxtonString *l;
	BuxtonData *d;

	if (!p) {
		return;
	}
	array = (BuxtonArray*)p;

	d = buxton_array_get(array, 0);
	data_free(d);
	l = buxton_array_get(array, 1);
	string_free(l);
	/* Item 3 is also the key, autofreed */
	buxton_array_free(&array, NULL);
}

/* Return existing hashmap or create new hashmap on the fly */
static BuxtonHashmap *_db_for_resource(BuxtonLayer *layer)
{
	BuxtonHashmap *db;
	char *name = NULL;
	int r;

	assert(layer);
	assert(_resources);

	if (layer->type == LAYER_USER) {
		r = asprintf(&name, "%s-%d", layer->name.value, layer->uid);
	} else {
		r = asprintf(&name, "%s", layer->name.value);
	}
	if (r == -1) {
		return NULL;
	}

	db = buxton_hashmap_get(_resources, name);
	if (!db) {
		db = buxton_hashmap_new_full(string_compare, string_hash, (hash_free_func)free, (hash_free_func)memory_vfree);
		buxton_hashmap_put(&_resources, name, db);
	} else {
		free(name);
	}

	return db;
}

static int set_value(BuxtonLayer *layer, _BuxtonKey *key, BuxtonData *data,
		      BuxtonString *label)
{
	BuxtonHashmap *db;
	BuxtonArray *array = NULL;
	BuxtonArray *stored;
	BuxtonData *data_copy = NULL;
	BuxtonString *label_copy = NULL;
	char *full_key = NULL;
	bool should_free = false;
	int ret;

	assert(layer);
	assert(key);
	assert(label);

	db = _db_for_resource(layer);
	if (!db) {
		ret = ENOENT;
		goto end;
	}

	if (key->name.value) {
		if (asprintf(&full_key, "%s%s", key->group.value, key->name.value) == -1) {
			abort();
		}
	} else {
		full_key = strdup(key->group.value);
		if (!full_key) {
			abort();
		}
	}

	if (!data) {
		stored = (BuxtonArray *)buxton_hashmap_get(db, full_key);
		if (!stored) {
			ret = ENOENT;
			free(full_key);
			goto end;
		}
		data = buxton_array_get(stored, 0);
	}

	array = buxton_array_new();
	if (!array) {
		abort();
	}
	data_copy = malloc0(sizeof(BuxtonData));
	if (!data_copy) {
		abort();
	}
	label_copy = malloc0(sizeof(BuxtonString));
	if (!label_copy) {
		abort();
	}

	if (!buxton_data_copy(data, data_copy)) {
		abort();
	}
	if (!buxton_string_copy(label, label_copy)) {
		abort();
	}
	if (!buxton_array_add(array, data_copy)) {
		abort();
	}
	if (!buxton_array_add(array, label_copy)) {
		abort();
	}
	if (!buxton_array_add(array, full_key)) {
		abort();
	}

	/* Implicit replace -ENOMEM is only possibility for failure */
	if (buxton_hashmap_contains(db, full_key)) {
		should_free = true;
	}
	ret = buxton_hashmap_put(&db, full_key, array);
	if (ret != 1) {
		abort();
	}

	ret = 0;
	if (should_free) {
		free(full_key);
	}
end:
	return ret;
}

static int get_value(BuxtonLayer *layer, _BuxtonKey *key, BuxtonData *data,
		      BuxtonString *label)
{
	BuxtonHashmap *db;
	BuxtonArray *stored;
	BuxtonData *d;
	BuxtonString *l;
	char *full_key = NULL;
	int ret;

	assert(layer);
	assert(key);
	assert(label);
	assert(data);

	db = _db_for_resource(layer);
	if (!db) {
		/*
		 * Set negative here to indicate layer not found
		 * rather than key not found, optimization for
		 * set value
		 */
		ret = -ENOENT;
		goto end;
	}

	if (key->name.value) {
		if (asprintf(&full_key, "%s%s", key->group.value, key->name.value) == -1) {
			abort();
		}
	} else {
		full_key = strdup(key->group.value);
		if (!full_key) {
			abort();
		}
	}

	stored = (BuxtonArray *)buxton_hashmap_get(db, full_key);
	if (!stored) {
		ret = ENOENT;
		goto end;
	}
	d = buxton_array_get(stored, 0);
	if (d->type != key->type) {
		ret = EINVAL;
		goto end;
	}

	if (!buxton_data_copy(d, data)) {
		abort();
	}

	l = buxton_array_get(stored, 1);
	if (!buxton_string_copy(l, label)) {
		abort();
	}

	ret = 0;
end:
	free(full_key);
	return ret;
}

static int unset_value(BuxtonLayer *layer,
			_BuxtonKey *key,
			__attribute__((unused)) BuxtonData *data,
			__attribute__((unused)) BuxtonString *label)
{
	BuxtonHashmap *db;
	bool stored;
	char *full_key = NULL;
	int ret;

	assert(layer);
	assert(key);

	db = _db_for_resource(layer);
	if (!db) {
		ret = ENOENT;
		goto end;
	}

	if (key->name.value) {
		if (asprintf(&full_key, "%s%s", key->group.value, key->name.value) == -1) {
			abort();
		}
	} else {
		full_key = strdup(key->group.value);
		if (!full_key) {
			abort();
		}
	}

	/* test if the value exists */
	stored = buxton_hashmap_remove(db, full_key);
	if (!stored) {
		ret = ENOENT;
		goto end;
	}

	ret = 0;

end:
	free(full_key);
	return ret;
}

_bx_export_ void buxton_module_destroy(void)
{
	/* free all hashmaps */
	buxton_hashmap_free(_resources);
	_resources = NULL;
}

_bx_export_ bool buxton_module_init(BuxtonBackend *backend)
{

	assert(backend);

	/* Point the struct methods back to our own */
	backend->set_value = &set_value;
	backend->get_value = &get_value;
	backend->unset_value = &unset_value;
	backend->list_keys = NULL;
	backend->create_db = NULL;

	_resources = buxton_hashmap_new_full(string_compare, string_hash, (hash_free_func)free, (hash_free_func)buxton_hashmap_free);
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
