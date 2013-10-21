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
			r = asprintf(&path, "%s/user-%s.db", DB_PATH, uid);
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

bool buxton_serialize_message(uint8_t **dest, BuxtonControlMessage message,
			      unsigned int n_params, ...)
{
	va_list args;
	int i = 0;
	uint8_t *data;
	bool ret = false;
	unsigned int offset = 0;
	unsigned int size = 0;
	ssize_t curSize = 0;
	uint16_t control, msg;

	/* Empty message not permitted */
	if (n_params == 0)
		return false;

	data = malloc(sizeof(uint32_t) + sizeof(unsigned int));
	if (!data)
		goto end;

	control = BUXTON_CONTROL_CODE;
	memcpy(data, &control, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	msg = (uint16_t)message;
	memcpy(data+offset, &msg, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	/* Now write the length */
	memcpy(data+offset, &n_params, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	size = offset;

	/* Deal with parameters */
	va_start(args, n_params);
	BuxtonData *param;
	unsigned int p_length = 0;
	for (i=0; i<n_params; i++) {
		/* Every parameter must be a BuxtonData. */
		param = va_arg(args, BuxtonData*);
		if (param->type == STRING)
			p_length = strlen(param->store.d_string)+1;
		else
			p_length = sizeof(param->store);

		/* Need to allocate enough room to hold this data */
		size += sizeof(BuxtonDataType) + sizeof(unsigned int);
		size += p_length;

		if (curSize < size) {
			if (!(data = greedy_realloc((void**)&data, &curSize, size)))
				goto fail;
		}

		/* Begin copying */
		memcpy(data+offset, &(param->type), sizeof(BuxtonDataType));
		offset += sizeof(BuxtonDataType);

		/* Length of following data */
		memcpy(data+offset, &p_length, sizeof(unsigned int));
		offset += sizeof(unsigned int);

		switch (param->type) {
			case STRING:
				memcpy(data+offset, param->store.d_string, p_length);
				break;
			case BOOLEAN:
				memcpy(data+offset, &(param->store.d_boolean), p_length);
				break;
			case FLOAT:
				memcpy(data+offset, &(param->store.d_float), p_length);
				break;
			case INT:
				memcpy(data+offset, &(param->store.d_int), p_length);
				break;
			case DOUBLE:
				memcpy(data+offset, &(param->store.d_double), p_length);
				break;
			case LONG:
				memcpy(data+offset, &(param->store.d_long), p_length);
			default:
				goto fail;
		}
		offset += p_length;
		p_length = 0;
	}

	ret = true;
	*dest = data;
fail:
	/* Clean up */
	if (!ret && data)
		free(data);
	va_end(args);
end:
	return ret;
}

int buxton_deserialize_message(uint8_t *data, BuxtonControlMessage *r_message,
			       BuxtonData** list)
{
	int size = 0;
	int offset = 0;
	int ret = -1;
	int min_length = BUXTON_CONTROL_LENGTH;
	void *copy_control = NULL;
	void *copy_message = NULL;
	void *copy_params = NULL;
	uint16_t control, message;
	unsigned int n_params, c_param;
	/* For each parameter */
	ssize_t p_size = 0;
	ssize_t c_size = 0;
	void *p_content = NULL;
	BuxtonDataType c_type;
	unsigned int c_length;
	BuxtonData *k_list = NULL;
	BuxtonData *c_data = NULL;

	size = malloc_usable_size(data);
	if (size < min_length)
		goto end;

	/* Copy the control code */
	copy_control = malloc(sizeof(uint16_t));
	if (!copy_control)
		goto end;
	memcpy(copy_control, data, sizeof(uint16_t));
	offset += sizeof(uint16_t);
	control = *(uint16_t*)copy_control;

	/* Check this is a valid buxton message */
	if (control != BUXTON_CONTROL_CODE)
		goto end;

	/* Obtain the control message */
	copy_message = malloc(sizeof(uint16_t));
	if (!copy_message)
		goto end;
	memcpy(copy_message, data+offset, sizeof(uint16_t));
	offset += sizeof(uint16_t);
	message = *(BuxtonControlMessage*)copy_message;
	/* Ensure control message is in valid range */
	if (message >= BUXTON_CONTROL_MAX)
		goto end;

	/* Obtain number of parameters */
	copy_params = malloc(sizeof(unsigned int));
	if (!copy_params)
		goto end;
	memcpy(copy_params, data+offset, sizeof(unsigned int));
	offset += sizeof(unsigned int);
	n_params = *(unsigned int*)copy_params;

	k_list = malloc(sizeof(BuxtonData)*n_params);

	for (c_param = 0; c_param < n_params; c_param++) {
		/* Now unpack type + length */
		memcpy(&c_type, data+offset, sizeof(BuxtonDataType));
		offset += sizeof(BuxtonDataType);

		if (c_type >= BUXTON_TYPE_MAX || c_type < STRING)
			goto end;

		/* Length */
		memcpy(&c_length, data+offset, sizeof(unsigned int));
		offset += sizeof(unsigned int);

		if (c_length <= 0)
			goto end;

		if (!p_content)
			p_content = malloc(c_length);
		else
			p_content = greedy_realloc((void**)&p_content, &p_size, c_length);

		/* If it still doesn't exist, bail */
		if (!p_content)
			goto end;
		memcpy(p_content, data+offset, c_length);

		if (!c_data)
			c_data = malloc(sizeof(BuxtonData));
		if (!c_data)
			goto end;

		switch (c_type) {
			case STRING:
				c_data->store.d_string = strndup((char*)p_content, c_length);
				if (!c_data->store.d_string)
					goto end;
				break;
			case BOOLEAN:
				c_data->store.d_boolean = *(bool*)p_content;
				break;
			case FLOAT:
				c_data->store.d_float = *(float*)p_content;
				break;
			case INT:
				c_data->store.d_int = *(int*)p_content;
				break;
			case DOUBLE:
				c_data->store.d_double = *(double*)p_content;
				break;
			case LONG:
				c_data->store.d_long = *(long*)p_content;
				break;
			default:
				goto end;
		}
		c_data->type = c_type;
		k_list[c_param] = *c_data;
		c_data = NULL;
		offset += c_length;
	}
	*r_message = message;
	*list = k_list;
	ret = n_params;
end:

	if (copy_control)
		free(copy_control);
	if (copy_message)
		free(copy_message);
	if (copy_params)
		free(copy_params);
	if (p_content)
		free(p_content);
	if (c_data)
		free(c_data);

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
