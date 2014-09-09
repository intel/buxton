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

/* structure for storing keys */
struct keyrec {
	unsigned hash;
	uint32_t size;
	char value[1];
};

/* creates a keyrec from the key */
static struct keyrec *make_keyrec(_BuxtonKey *key)
{
	uint32_t sz;
	struct keyrec *result;
        unsigned hash;

	/* compute requested size */
	sz = key->group.length;
	if (key->name.value) {
		sz += key->name.length;
	}

	/* allocate */
	result = malloc(sz - 1 + sizeof * result);
	if (!result) {
		abort();
		return NULL;
	}

	/* set */
	result->size = sz;
	memcpy(result->value, key->group.value, key->group.length);
	if (key->name.value) {
		memcpy(result->value + key->group.length, key->name.value,
		       key->name.length);
	}

        /* DJB's hash function */
        hash = 5381;
        while (sz) {
                hash = (hash << 5) + hash + (unsigned char)result->value[--sz];
	}
	result ->hash = hash;

	return result;
}

/* free the keyrec */
static inline void free_keyrec(struct keyrec *item)
{
	free(item);
}

/* gets the hash code of the keyrec */
static unsigned hash_keyrec(const struct keyrec *item)
{
	return item->hash;
}

/* compares two keyrecs */
static int compare_keyrec(const struct keyrec *a, const struct keyrec *b)
{
	return a->size == b->size ? memcmp(a->value, b->value, a->size)
		: a->size < b->size ? -1 : 1;
}

/* Return existing hashmap or create new hashmap on the fly */
static Hashmap *_db_for_resource(BuxtonLayer *layer)
{
	Hashmap *db;
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

	db = hashmap_get(_resources, name);
	if (!db) {
		db = hashmap_new((hash_func_t)hash_keyrec,
			(compare_func_t)compare_keyrec);
		hashmap_put(_resources, name, db);
	} else {
		free(name);
	}

	return db;
}

static int set_value(BuxtonLayer *layer, _BuxtonKey *key, BuxtonData *data,
		      BuxtonString *label)
{
	Hashmap *db;
	BuxtonArray *array = NULL;
	BuxtonArray *stored;
	BuxtonData *data_copy = NULL;
	BuxtonData *d;
	BuxtonString *label_copy = NULL;
	BuxtonString *l;
	int ret;
	struct keyrec *keyrec;
	struct keyrec *k;

	assert(layer);
	assert(key);
	assert(label);

	db = _db_for_resource(layer);
	if (!db) {
		ret = ENOENT;
		goto end;
	}

	keyrec = make_keyrec(key);
	if (!keyrec) {
		ret = ENOMEM;
		goto end;
	}

	if (!data) {
		stored = (BuxtonArray *)hashmap_get(db, keyrec);
		if (!stored) {
			ret = ENOENT;
			free_keyrec(keyrec);
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
	if (!buxton_array_add(array, keyrec)) {
		abort();
	}

	ret = hashmap_put(db, keyrec, array);
	if (ret != 1) {
		if (ret == -ENOMEM) {
			abort();
		}
		/* remove the old value */
		stored = (BuxtonArray *)hashmap_remove(db, keyrec);
		assert(stored);

		/* free the data */
		d = buxton_array_get(stored, 0);
		data_free(d);
		l = buxton_array_get(stored, 1);
		string_free(l);
		k = buxton_array_get(stored, 2);
		free_keyrec(k);
		buxton_array_free(&stored, NULL);
		ret = hashmap_put(db, keyrec, array);
		if (ret != 1) {
			abort();
		}
	}

	ret = 0;

end:
	return ret;
}

static int get_value(BuxtonLayer *layer, _BuxtonKey *key, BuxtonData *data,
		      BuxtonString *label)
{
	Hashmap *db;
	BuxtonArray *stored;
	BuxtonData *d;
	BuxtonString *l;
	int ret;
	struct keyrec *keyrec;

	assert(layer);
	assert(key);
	assert(label);
	assert(data);

	keyrec = NULL;
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

	keyrec = make_keyrec(key);
	if (!keyrec) {
		ret = ENOMEM;
		goto end;
	}

	stored = (BuxtonArray *)hashmap_get(db, keyrec);
	if (!stored) {
		ret = ENOENT;
		goto end;
	}
	d = buxton_array_get(stored, 0);
	if (d->type != key->type && key->type != BUXTON_TYPE_UNSET) {
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
	free_keyrec(keyrec);
	return ret;
}

static int unset_key(BuxtonLayer *layer,
			_BuxtonKey *key)
{
	Hashmap *db;
	BuxtonArray *stored;
	BuxtonData *d;
	BuxtonString *l;
	int ret;
	struct keyrec *keyrec;
	struct keyrec *k;

	assert(layer);
	assert(key);
	assert(key->name.value);

	keyrec = NULL;
	db = _db_for_resource(layer);
	if (!db) {
		ret = ENOENT;
		goto end;
	}

	keyrec = make_keyrec(key);
	if (!keyrec) {
		ret = ENOMEM;
		goto end;
	}

	/* test if the value exists */
	stored = (BuxtonArray *)hashmap_remove(db, keyrec);
	if (!stored) {
		ret = ENOENT;
		goto end;
	}

	/* free the data */
	d = buxton_array_get(stored, 0);
	data_free(d);
	l = buxton_array_get(stored, 1);
	string_free(l);
	k = buxton_array_get(stored, 2);
	free_keyrec(k);
	buxton_array_free(&stored, NULL);

	ret = 0;

end:
	free(keyrec);
	return ret;
}

static int unset_group(BuxtonLayer *layer,
			_BuxtonKey *key)
{
	Hashmap *db;
	int ret;
	struct keyrec *keyrec;
	Iterator iterator;
	BuxtonArray *array;
	
	assert(layer);
	assert(key);
	assert(!key->name.value);

	db = _db_for_resource(layer);
	if (!db) {
		ret = EROFS;
		goto end;
	}

	ret = ENOENT;
	/* Iterate through the keys and record matching keys in k_list */
	HASHMAP_FOREACH_KEY(array, keyrec, db, iterator) {

		/* test if the key matches the group */
		if (!strcmp(keyrec->value, key->group.value)) {
			/* yes it matches */
			hashmap_remove(db, keyrec);
			data_free(buxton_array_get(array, 0));
			string_free(buxton_array_get(array, 1));
			free_keyrec(buxton_array_get(array, 2));
			buxton_array_free(&array, NULL);
			ret = 0;
		}
	}

end:
	return ret;
}

static int unset_value(BuxtonLayer *layer,
			_BuxtonKey *key,
			__attribute__((unused)) BuxtonData *data,
			__attribute__((unused)) BuxtonString *label)
{
	assert(layer);
	assert(key);

	if (key->name.value) {
		return unset_key(layer, key);
	} else {
		return unset_group(layer, key);
	}
}

static bool list_names(BuxtonLayer *layer,
		       BuxtonString *group,
		       BuxtonString *prefix,
		       BuxtonArray **ret_list)
{
	Hashmap *db;
	BuxtonArray *list = NULL;
	BuxtonData *data;
	Iterator iterator;
	BuxtonArray *array;
	char *gname;
	char *value;
	char *copy;
	uint32_t glen;
	uint32_t klen;
	uint32_t length;
	bool ret = false;
	struct keyrec *keyrec;

	assert(layer);

	db = _db_for_resource(layer);
	if (!db) {
		goto end;
	}

	if (group && !group->length) {
		group = NULL;
	}
	if (prefix && !prefix->length) {
		prefix = NULL;
	}

	value = NULL;
	list = buxton_array_new();

	/* Iterate through all of the keys */
	HASHMAP_FOREACH_KEY(array, keyrec, db, iterator) {

		/* get main data of the key */
		gname = (char*)keyrec->value;
		glen = (uint32_t)strlen(gname) + 1;
		assert(keyrec->size >= glen);
		klen = keyrec->size - glen;
		assert(!klen || klen == (uint32_t)strlen(gname+glen) + 1);

		/* treat the key value if it*/
		if (klen) {
			/* it is a key */
			if (group && glen == group->length
				&& !strcmp(gname, group->value)) {
				value = gname + glen;
				length = klen;
			}
		} else {
			/* it is a group */
			if (!group) {
				value = gname;
				length = glen;
			}
		}

		/* treat a potential value */
		if (value) {
			/* check the prefix */
			if (!prefix || !strncmp(value, prefix->value,
				prefix->length - 1))  {
				/* add the value */
				data = malloc0(sizeof(BuxtonData));
				copy = malloc(length);
				if (data && copy
				    && buxton_array_add(list, data)) {
					data->type = BUXTON_TYPE_STRING;
					data->store.d_string.value = copy;
					data->store.d_string.length = length;
					memcpy(copy, value, length);
				} else {
					free(data);
					free(copy);
					goto end;
				}
			}
			value = NULL;
		}
	}

	/* Pass ownership of the array to the caller */
	*ret_list = list;
	ret = true;

end:
	if (!ret && list) {
		buxton_array_free(&list, (buxton_free_func)data_free);
	}
	return ret;
}

_bx_export_ void buxton_module_destroy(void)
{
	const struct keyrec *key1, *key2;
	Iterator iteratori, iteratoro;
	Hashmap *map;
	BuxtonArray *array;
	BuxtonData *d;
	BuxtonString *l;

	/* free all hashmaps */
	HASHMAP_FOREACH_KEY(map, key1, _resources, iteratoro) {
		HASHMAP_FOREACH_KEY(array, key2, map, iteratori) {
			hashmap_remove(map, key2);
			free_keyrec((struct keyrec*)key2);
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
	backend->list_names = list_names;
	backend->create_db = NULL;

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
