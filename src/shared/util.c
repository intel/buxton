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
#include <sys/inotify.h>
#include <malloc.h>
#include "../shared/hashmap.h"
#include "../shared/log.h"
#include "../shared/util.h"
#include "../include/bt-daemon-private.h"

static Hashmap *_smackrules = NULL;

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
			r = asprintf(&path, "%s/%s.db", DB_PATH, layer->name);
			if (r == -1)
				return NULL;
			break;
		case LAYER_USER:
			/* uid must already be set in layer before calling */
			sprintf(uid, "%d", (int)layer->uid);
			r = asprintf(&path, "%s/%s-%s.db", DB_PATH, layer->name, uid);
			if (r == -1)
				return NULL;
			break;
		default:
			break;
	}

	return path;
}

bool buxton_serialize(BuxtonData *source, uint8_t **target)
{
	unsigned int length;
	unsigned int size;
	unsigned int offset = 0;
	uint8_t *data = NULL;
	bool ret = false;

	if (!source)
		goto end;

	/* DataType + length field */
	size = sizeof(BuxtonDataType) + sizeof(unsigned int);

	/* Total size will be different for string data */
	switch (source->type) {
		case STRING:
			length = strlen(source->store.d_string) + 1;
			break;
		default:
			length = sizeof(source->store);
			break;
	}

	size += length;

	/* Allocate memory big enough to hold all information */
	data = malloc(size);
	if (!data)
		goto end;

	/* Write the entire BuxtonDataType to the first block */
	memcpy(data, &(source->type), sizeof(BuxtonDataType));
	offset += sizeof(BuxtonDataType);

	/* Write out the length of the data field */
	memcpy(data+offset, &length, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	/* Write the data itself */
	switch (source->type) {
		case STRING:
			memcpy(data+offset, source->store.d_string, length);
			break;
		case BOOLEAN:
			memcpy(data+offset, &(source->store.d_boolean), length);
			break;
		case FLOAT:
			memcpy(data+offset, &(source->store.d_float), length);
			break;
		case INT:
			memcpy(data+offset, &(source->store.d_int), length);
			break;
		case DOUBLE:
			memcpy(data+offset, &(source->store.d_double), length);
			break;
		case LONG:
			memcpy(data+offset, &(source->store.d_long), length);
		default:
			goto end;
	}

	*target = data;
	ret = true;
end:
	if (!ret && data)
		free(data);

	return ret;
}

bool buxton_deserialize(uint8_t *source, BuxtonData *target)
{
	void *copy_type = NULL;
	void *copy_length = NULL;
	void *copy_data = NULL;
	unsigned int offset = 0;
	unsigned int length = 0;
	unsigned int len;
	BuxtonDataType type;
	bool ret = false;

	len = malloc_usable_size(source);

	if (len < BXT_MINIMUM_SIZE)
		return false;

	/* firstly, retrieve the BuxtonDataType */
	copy_type = malloc(sizeof(BuxtonDataType));
	if (!copy_type)
		goto end;
	memcpy(copy_type, source, sizeof(BuxtonDataType));
	type = *(BuxtonDataType*)copy_type;
	offset += sizeof(BuxtonDataType);

	/* Now retrieve the length */
	copy_length = malloc(sizeof(unsigned int));
	if (!copy_length)
		goto end;
	memcpy(copy_length, source+offset, sizeof(unsigned int));
	length = *(unsigned int*)copy_length;
	offset += sizeof(unsigned int);

	/* Retrieve the remainder of the data */
	copy_data = malloc(length);
	if (!copy_data)
		goto end;
	memcpy(copy_data, source+offset, length);

	switch (type) {
		case STRING:
			/* User must free the string */
			target->store.d_string = strdup((char*)copy_data);
			if (!target->store.d_string)
				goto end;
			break;
		case BOOLEAN:
			target->store.d_boolean = *(bool*)copy_data;
			break;
		case FLOAT:
			target->store.d_float = *(float*)copy_data;
			break;
		case INT:
			target->store.d_int = *(int*)copy_data;
			break;
		case DOUBLE:
			target->store.d_double = *(double*)copy_data;
			break;
		case LONG:
			target->store.d_long = *(long*)copy_data;
			break;
		default:
			goto end;
	}

	/* Successful */
	target->type = type;
	ret = true;

end:
	/* Cleanup */
	if (copy_type)
		free(copy_type);
	if (copy_length)
		free(copy_length);
	if (copy_data)
		free(copy_data);

	return ret;
}

void buxton_data_copy(BuxtonData* original, BuxtonData *copy)
{
	BuxtonDataStore store;

	switch (original->type) {
		case STRING:
			store.d_string = strdup(original->store.d_string);
			break;
		case BOOLEAN:
			store.d_boolean = original->store.d_boolean;
			break;
		case FLOAT:
			store.d_float = original->store.d_float;
			break;
		case INT:
			store.d_int = original->store.d_int;
			break;
		case DOUBLE:
			store.d_double = original->store.d_double;
			break;
		case LONG:
			store.d_long = original->store.d_long;
			break;
		default:
			break;
	}

	if (original->type == STRING && !store.d_string)
		return;

	copy->type = original->type;
	copy->store = store;
}

bool buxton_cache_smack_rules(void)
{
	FILE *load_file = NULL;
	char *rule_pair = NULL;
	int ret = true;

	if (_smackrules)
		hashmap_free(_smackrules);

	_smackrules = hashmap_new(string_hash_func, string_compare_func);

	if (!_smackrules) {
		buxton_log("Failed to allocate Smack access table: %m\n");
		return false;
	}

	load_file = fopen(SMACK_LOAD_FILE, "r");

	if (!load_file) {
		buxton_log("fopen(): %m\n");
		ret = false;
		goto end;
	}

	while (!feof(load_file)) {
		int r;
		int chars;
		BuxtonKeyAccessType *accesstype;

		char subject[SMACK_LABEL_LEN] = { 0, };
		char object[SMACK_LABEL_LEN] = { 0, };
		char access[ACC_LEN] = { 0, };

		/* read contents from load2 */
		chars = fscanf(load_file, "%s %s %s\n", subject, object, access);
		if (chars != 3) {
			buxton_log("fscanf(): %m\n");
			ret = false;
			goto end;
		}

		r = asprintf(&rule_pair, "%s %s", subject, object);
		if (r == -1) {
			buxton_log("asprintf(): %m\n");
			ret = false;
			goto end;
		}

		accesstype = malloc0(sizeof(BuxtonKeyAccessType));
		if (!accesstype) {
			buxton_log("malloc0(): %m\n");
			ret = false;
			goto end;
		}

		*accesstype = ACCESS_NONE;

		if (strchr(access, 'r'))
			*accesstype |= ACCESS_READ;

		if (strchr(access, 'w'))
			*accesstype |= ACCESS_WRITE;

		hashmap_put(_smackrules, rule_pair, accesstype);
	}

end:
	if (load_file)
		fclose(load_file);

	return ret;
}

bool buxton_check_smack_access(char *subject, char *object, BuxtonKeyAccessType request)
{
	char *key;
	int r;
	BuxtonKeyAccessType *access;

	assert(subject);
	assert(object);
	assert((request == ACCESS_READ) || (request == ACCESS_WRITE));
	assert(_smackrules);

	buxton_debug("Subject: %s\n", subject);
	buxton_debug("Object: %s\n", object);

	r = asprintf(&key, "%s %s", subject, object);
	if (r == -1) {
		buxton_log("asprintf(): %m\n");
		exit(1);
	}

	buxton_debug("Key: %s\n", key);

	access = hashmap_get(_smackrules, key);
	if (!access) {
		/* corruption */
		buxton_log("Value of key '%s' is NULL\n", key);
		free(key);
		exit(1);
	}

	free(key);

	/* After debugging, change this code to: */
	/* return ((*access) & request); */
	if (access)
		buxton_debug("Value: %x\n", *access);

	if ((*access) & request) {
		buxton_debug("Access granted!\n");
		return true;
	}

	buxton_debug("Access denied!\n");
	return false;
}

int buxton_watch_smack_rules(void)
{
	int fd;

	fd = inotify_init1(IN_NONBLOCK);
	if (fd < 0) {
		buxton_log("inotify_init(): %m\n");
		return -1;
	}
	if (inotify_add_watch(fd, SMACK_LOAD_FILE, IN_CLOSE_WRITE) < 0) {
		buxton_log("inotify_add_watch(): %m\n");
		return -1;
	}
	return fd;
}

const char* buxton_type_as_string(BuxtonDataType type)
{
	switch (type) {
		case STRING:
			return "string";
		case BOOLEAN:
			return "boolean";
		case FLOAT:
			return "float";
		case DOUBLE:
			return "double";
		case LONG:
			return "long";
		case INT:
			return "int";
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
