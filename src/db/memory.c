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
	unsigned hash; /**< Precomputed hash  */
	uint32_t size; /**< Key size in bytes */
	char *value; /**< The key value */
};

/* structure for storing values */
struct valrec {
	BuxtonData data;    /**< Recorded data */
	BuxtonString label; /**< Recorded label */
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
	result = malloc(sizeof(struct keyrec));
	if (!result) {
		return NULL;
	}
	result->value = malloc(sz);
	if (!result->value) {
		free(result);
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
	result->hash = hash;

	return result;
}

/* free the keyrec item */
static inline void free_keyrec(struct keyrec *item)
{
	free(item->value);
	free(item);
}

/* gets the hash code of the keyrec item */
static unsigned hash_keyrec(const struct keyrec *item)
{
	return item->hash;
}

/* compares two keyrecs a and b */
static int compare_keyrec(const struct keyrec *a, const struct keyrec *b)
{
	return a->size == b->size ? memcmp(a->value, b->value, a->size) :
		a->size < b->size ? -1 : 1;
}

/* free the valrec item */
static void free_valrec(struct valrec *item)
{
	if (item) {
		if (item->data.type == BUXTON_TYPE_STRING) {
			free(item->data.store.d_string.value);
		}
		free(item->label.value);
		free(item);
	}
}

static bool set_valrec(struct valrec *item, BuxtonData *data,
			      BuxtonString *label)
{
	char *sdata;
	char *slabel;

	/* allocate data if needed */
	if (data && data->type == BUXTON_TYPE_STRING &&
	    data->store.d_string.value) {
		sdata = malloc(data->store.d_string.length);
		if (!sdata) {
			return false;
		}
		memcpy(sdata, data->store.d_string.value,
			data->store.d_string.length);
	} else {
		sdata = NULL;
	}

	/* allocate label if needed */
	if (label && label->value) {
		slabel = malloc(label->length);
		if (!slabel) {
			free(sdata);
			return false;
		}
		memcpy(slabel, label->value, label->length);
	} else {
		slabel = NULL;
	}

	/* copy now */
	if (data) {
		if (item->data.type == BUXTON_TYPE_STRING) {
			free(item->data.store.d_string.value);
		}
		item->data = *data;
		if (data->type == BUXTON_TYPE_STRING) {
			item->data.store.d_string.value = sdata;
		}
	}

	if (label) {
		free(item->label.value);
		item->label.length = label->length;
		item->label.value = slabel;
	}

	return true;
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
	int ret;
	struct keyrec *keyrec;
	struct valrec *valrec;

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
		abort();
	}

	valrec = hashmap_get(db, keyrec);
	if (valrec) {
		free_keyrec(keyrec);
		if (!set_valrec(valrec, data, label)) {
			abort();
		}
	} else {
		if (!data) {
			ret = ENOENT;
			goto end;
		}
		valrec = calloc(1, sizeof * valrec);
		if (!valrec) {
			abort();
		}
		if (!set_valrec(valrec, data, label) ||
		    hashmap_put(db, keyrec, valrec) != 1) {
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
	int ret;
	struct keyrec *keyrec;
	struct valrec *valrec;

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

	keyrec = make_keyrec(key);
	if (!keyrec) {
		ret = ENOMEM;
		goto end;
	}

	valrec = hashmap_get(db, keyrec);
	free_keyrec(keyrec);

	if (!valrec) {
		ret = ENOENT;
		goto end;
	}
	if (valrec->data.type != key->type && key->type != BUXTON_TYPE_UNSET) {
		ret = EINVAL;
		goto end;
	}

	if (!buxton_data_copy(&valrec->data, data)) {
		abort();
	}

	if (!buxton_string_copy(&valrec->label, label)) {
		abort();
	}

	ret = 0;

end:
	return ret;
}

static int unset_key(BuxtonLayer *layer,
			_BuxtonKey *key)
{
	Hashmap *db;
	int ret;
	struct keyrec *keyrec;
	struct keyrec *remkey;
	struct valrec *valrec;

	assert(layer);
	assert(key);
	assert(key->name.value);

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
	valrec = hashmap_remove2(db, keyrec, (void**)&remkey);
	free_keyrec(keyrec);
	if (!valrec) {
		ret = ENOENT;
		goto end;
	}

	/* free the data */
	free_valrec(valrec);
	free_keyrec(remkey);

	ret = 0;

end:
	return ret;
}

static int unset_group(BuxtonLayer *layer,
			_BuxtonKey *key)
{
	Hashmap *db;
	int ret;
	struct keyrec *keyrec;
	struct valrec *valrec;
	Iterator iterator;

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
	HASHMAP_FOREACH_KEY(valrec, keyrec, db, iterator) {

		/* test if the key matches the group */
		if (!strcmp(keyrec->value, key->group.value)) {
			/* yes it matches */
			hashmap_remove(db, keyrec);
			free_valrec(valrec);
			free_keyrec(keyrec);
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
				if (data && copy &&
				    buxton_array_add(list, data)) {
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
	char *klayer;
	struct keyrec *keyrec;
	struct valrec *valrec;
	Iterator iteratori, iteratoro;
	Hashmap *map;

	/* free all hashmaps */
	HASHMAP_FOREACH_KEY(map, klayer, _resources, iteratoro) {
		HASHMAP_FOREACH_KEY(valrec, keyrec, map, iteratori) {
			hashmap_remove(map, keyrec);
			free_valrec(valrec);
			free_keyrec(keyrec);
		}
		hashmap_remove(_resources, klayer);
		hashmap_free(map);
		free(klayer);
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
	backend->list_keys = NULL;
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
