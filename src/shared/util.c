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

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "configurator.h"
#include "hashmap.h"
#include "log.h"
#include "util.h"

size_t page_size(void)
{
	static __thread size_t pgsz = 0;
	long r;

	if (_likely_(pgsz > 0)) {
		return pgsz;
	}

	r = sysconf(_SC_PAGESIZE);
	assert(r > 0);

	pgsz = (size_t) r;
	return pgsz;
}

void* greedy_realloc(void **p, size_t *allocated, size_t need)
{
	size_t a;
	void *q;

	assert(p);
	assert(allocated);

	if (*allocated >= need) {
		return *p;
	}

	a = MAX(64u, need * 2);
	q = realloc(*p, a);
	if (!q) {
		return NULL;
	}

	*p = q;
	*allocated = a;
	return q;
}

char* get_layer_path(BuxtonLayer *layer)
{
	char *path = NULL;
	int r;
	char uid[15];

	assert(layer);

	switch (layer->type) {
	case LAYER_SYSTEM:
		r = asprintf(&path, "%s/%s.db", buxton_db_path(), layer->name.value);
		if (r == -1) {
			return NULL;
		}
		break;
	case LAYER_USER:
		/* uid must already be set in layer before calling */
		sprintf(uid, "%d", (int)layer->uid);
		r = asprintf(&path, "%s/%s-%s.db", buxton_db_path(), layer->name.value, uid);
		if (r == -1) {
			return NULL;
		}
		break;
	default:
		break;
	}

	return path;
}

bool buxton_data_copy(BuxtonData* original, BuxtonData *copy)
{
	BuxtonDataStore store;

	assert(original);
	assert(copy);

	switch (original->type) {
	case STRING:
		store.d_string.value = malloc(original->store.d_string.length);
		if (!store.d_string.value) {
			goto fail;
		}
		memcpy(store.d_string.value, original->store.d_string.value, original->store.d_string.length);
		store.d_string.length = original->store.d_string.length;
		break;
	case INT32:
		store.d_int32 = original->store.d_int32;
		break;
	case UINT32:
		store.d_uint32 = original->store.d_uint32;
		break;
	case INT64:
		store.d_int64 = original->store.d_int64;
		break;
	case UINT64:
		store.d_uint64 = original->store.d_uint64;
		break;
	case FLOAT:
		store.d_float = original->store.d_float;
		break;
	case DOUBLE:
		store.d_double = original->store.d_double;
		break;
	case BOOLEAN:
		store.d_boolean = original->store.d_boolean;
		break;
	default:
		goto fail;
	}

	copy->type = original->type;
	copy->store = store;

	return true;

fail:
	memset(copy, 0, sizeof(BuxtonData));
	return false;
}

bool buxton_string_copy(BuxtonString *original, BuxtonString *copy)
{
	if (!original || !copy) {
		return false;
	}

	copy->value = malloc0(original->length);
	if (!copy->value) {
		return false;
	}

	memcpy(copy->value, original->value, original->length);
	copy->length = original->length;

	return true;
}

bool buxton_key_copy(_BuxtonKey *original, _BuxtonKey *copy)
{
	if (!original || !copy) {
		return false;
	}

	if (original->group.value) {
		if (!buxton_string_copy(&original->group, &copy->group)) {
			goto fail;
		}
	}
	if (original->name.value) {
		if (!buxton_string_copy(&original->name, &copy->name)) {
			goto fail;
		}
	}
	if (original->layer.value) {
		if (!buxton_string_copy(&original->layer, &copy->layer)) {
			goto fail;
		}
	}
	copy->type = original->type;

	return true;

fail:
	if (original->group.value) {
		free(copy->group.value);
	}
	if (original->name.value) {
		free(copy->name.value);
	}
	if (original->layer.value) {
		free(copy->layer.value);
	}
	copy->type = BUXTON_TYPE_MIN;

	return false;
}

bool buxton_copy_key_group(_BuxtonKey *original, _BuxtonKey *group)
{
	if (!original || !group) {
		return false;
	}

	if (original->group.value) {
		if (!buxton_string_copy(&original->group, &group->group)) {
			goto fail;
		}
	}
	group->name = (BuxtonString){ NULL, 0 };
	if (original->layer.value) {
		if (!buxton_string_copy(&original->layer, &group->layer)) {
			goto fail;
		}
	}
	group->type = STRING;

	return true;

fail:
	if (original->group.value) {
		free(group->group.value);
	}
	if (original->layer.value) {
		free(group->layer.value);
	}
	group->type = BUXTON_TYPE_MIN;

	return false;
}

void data_free(BuxtonData *data)
{
	if (!data) {
		return;
	}

	if (data->type == STRING && data->store.d_string.value) {
		free(data->store.d_string.value);
	}
	free(data);
}

void string_free(BuxtonString *string)
{
	if (!string) {
		return;
	}

	if (string->value) {
		free(string->value);
	}
	free(string);
}

void key_free(_BuxtonKey *key)
{
	if (!key) {
		return;
	}

	free(key->group.value);
	free(key->name.value);
	free(key->layer.value);
	free(key);
}

const char* buxton_type_as_string(BuxtonDataType type)
{
	switch (type) {
	case STRING:
		return "string";
	case INT32:
		return "int32_t";
	case UINT32:
		return "uint32_t";
	case INT64:
		return "int64_t";
	case UINT64:
		return "uint64_t";
	case FLOAT:
		return "float";
	case DOUBLE:
		return "double";
	case BOOLEAN:
		return "boolean";
	default:
		return "[unknown]";
	}
}

char *get_group(_BuxtonKey *key)
{
	if (key && key->group.value) {
		return strdup(key->group.value);
	}

	return NULL;
}

char *get_name(_BuxtonKey *key)
{
	if (key && key->name.value) {
		return strdup(key->name.value);
	}

	return NULL;
}

char *get_layer(_BuxtonKey *key)
{
	if (key && key->layer.value) {
		return strdup(key->layer.value);
	}

	return NULL;
}

bool _write(int fd, uint8_t *buf, size_t nbytes)
{
	size_t nbytes_out = 0;

	while (nbytes_out != nbytes) {
		ssize_t b;
		b = write(fd, buf + nbytes_out, nbytes - nbytes_out);

		if (b == -1 && errno != EAGAIN) {
			buxton_debug("write error\n");
			return false;
		}
		nbytes_out += (size_t)b;
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
