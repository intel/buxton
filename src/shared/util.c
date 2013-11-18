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
#include <string.h>
#include "hashmap.h"
#include "log.h"
#include "util.h"

size_t page_size(void)
{
	static __thread size_t pgsz = 0;
	long r;

	if (_likely_(pgsz > 0))
		return pgsz;

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

	if (*allocated >= need)
		return *p;

	a = MAX(64u, need * 2);
	q = realloc(*p, a);
	if (!q)
		return NULL;

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
			r = asprintf(&path, "%s/%s.db", DB_PATH, layer->name.value);
			if (r == -1)
				return NULL;
			break;
		case LAYER_USER:
			/* uid must already be set in layer before calling */
			sprintf(uid, "%d", (int)layer->uid);
			r = asprintf(&path, "%s/%s-%s.db", DB_PATH, layer->name.value, uid);
			if (r == -1)
				return NULL;
			break;
		default:
			break;
	}

	return path;
}

void buxton_data_copy(BuxtonData* original, BuxtonData *copy)
{
	BuxtonDataStore store;
	BuxtonString label;

	assert(original);
	assert(original->label.value);
	assert(copy);

	memset(&label, 0, sizeof(BuxtonString));
	label.value = malloc(original->label.length);
	if (!label.value)
		goto fail;
	memcpy(label.value, original->label.value, original->label.length);
	label.length = original->label.length;

	switch (original->type) {
		case STRING:
			store.d_string.value = malloc(original->store.d_string.length);
			if (!store.d_string.value)
				goto fail;
			memcpy(store.d_string.value, original->store.d_string.value, original->store.d_string.length);
			store.d_string.length = original->store.d_string.length;
			break;
		case INT32:
			store.d_int32 = original->store.d_int32;
			break;
		case INT64:
			store.d_int64 = original->store.d_int64;
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
	copy->label = label;

	return;

fail:
	if (label.value)
		free(label.value);
	memset(copy, 0, sizeof(BuxtonData));
}

const char* buxton_type_as_string(BuxtonDataType type)
{
	switch (type) {
		case STRING:
			return "string";
		case INT32:
			return "int32_t";
		case INT64:
			return "int64_t";
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
