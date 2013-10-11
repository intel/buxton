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

void buxton_cache_smack_rules(char *load_path)
{
	FILE *load_file = NULL;
	char *rule_pair = NULL;

	if (_smackrules == NULL) {
		_smackrules = hashmap_new(string_hash_func, string_compare_func);
	}

	load_file = fopen(load_path, "r");

	if (load_file == NULL) {
		buxton_log("Failed to load Smack rules\n");
		return;
	}

	while (!feof(load_file)) {
		int r;
		int chars;
		char *match;
		BuxtonKeyAccessType *accesstype;

		char subject[SMACK_LABEL_LEN] = { 0, };
		char object[SMACK_LABEL_LEN] = { 0, };
		char access[ACC_LEN] = { 0, };

		/* read contents from load2 */
		chars = fscanf(load_file, "%s %s %s\n", subject, object, access);
		if (chars != 3) {
			buxton_log("fscanf(): %m\n");
			goto end;
		}

		r = asprintf(&rule_pair, "%s %s", subject, object);
		if (r == -1) {
			buxton_log("asprintf(): %m\n");
			goto end;
		}

		accesstype = malloc0(sizeof(BuxtonKeyAccessType));
		if (accesstype == NULL) {
			goto end;
		}

		*accesstype = ACCESS_NONE;

		match = strchr(access, 'r');
		if (match) {
			*accesstype |= ACCESS_READ;
		}

		match = strchr(access, 'w');
		if (match) {
			*accesstype |= ACCESS_WRITE;
		}

		hashmap_put(_smackrules, rule_pair, accesstype);
	}

end:
	if (rule_pair)
		free(rule_pair);

	fclose(load_file);

	/* need to implement a cleanup handler for the hashmap */
	return;
}

bool buxton_check_smack_access(char *subject, char *object, BuxtonKeyAccessType request)
{
	char *key;
	int r;
	BuxtonKeyAccessType *access;

	buxton_log("Subject: %s\n", subject);
	buxton_log("Object: %s\n", object);

	r = asprintf(&key, "%s %s", subject, object);
	if (r == -1) {
		return false;
	}

	buxton_log("Key: %s\n", key);

	if ((access = hashmap_get(_smackrules, key)) == NULL) {
		buxton_log("Value of key '%s' is NULL\n", key);
		free(key);
		return false;
	}

	free(key);

	if (access) {
		buxton_log("Value: %x\n", *access);
	}

	if ((*access) & request) {
		buxton_log("Access granted!\n");
		return true;
	}

	buxton_log("Access denied!\n");
	return false;
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
