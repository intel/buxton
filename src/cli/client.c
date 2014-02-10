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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buxton.h"
#include "buxtonarray.h"
#include "buxtonresponse.h"
#include "client.h"
#include "direct.h"
#include "hashmap.h"
#include "protocol.h"
#include "util.h"


bool cli_set_label(BuxtonControl *control, BuxtonDataType type,
		   char *one, char *two, char *three, char *four)
{
	BuxtonString label;
	BuxtonKey key;
	bool ret = false;

	if (four != NULL)
		key = buxton_make_key(two, three, one, type);
	else
		key = buxton_make_key(two, NULL, one, type);

	if (!key)
		return ret;

	if (four != NULL)
		label = buxton_string_pack(four);
	else
		label = buxton_string_pack(three);

	if (control->client.direct)
		ret = buxton_direct_set_label(control, (_BuxtonKey *)key, &label);
	else
		ret = buxton_client_set_label(&control->client, key, label.value,
					      NULL, NULL, true);

	if (!ret) {
		char *name = get_name(key);
		printf("Failed to update key \'%s:%s\' label in layer '%s'\n",
		       two, name, one);
		free(name);
	}
	buxton_free_key(key);
	return ret;
}

bool cli_get_label(BuxtonControl *control, BuxtonDataType type,
		   char *one, char *two, char *three,
		   __attribute__((unused)) char *four)
{
	/* Not yet implemented */
	return false;
}

bool cli_set_value(BuxtonControl *control, BuxtonDataType type,
		   char *one, char *two, char *three, char *four)
{
	BuxtonString value;
	BuxtonKey key;
	BuxtonData set;
	bool ret = false;

	memzero((void*)&set, sizeof(BuxtonData));
	key = buxton_make_key(two, three, one, type);
	if (!key)
		return ret;

	value.value = four;
	value.length = (uint32_t)strlen(four) + 1;

	set.type = type;
	switch (set.type) {
	case STRING:
		set.store.d_string.value = value.value;
		set.store.d_string.length = value.length;
		if (control->client.direct)
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		else
			ret = buxton_client_set_value(&control->client, key,
						      four, NULL, NULL, true);
		break;
	case INT32:
		set.store.d_int32 = (int32_t)strtol(four, NULL, 10);
		if (errno) {
			printf("Invalid int32_t value\n");
			return ret;
		}
		if (control->client.direct)
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		else
			ret = buxton_client_set_value(&control->client, key,
						      &set.store.d_int32, NULL,
						      NULL, true);
		break;
	case UINT32:
		set.store.d_uint32 = (uint32_t)strtol(value.value, NULL, 10);
		if (errno) {
			printf("Invalid uint32_t value\n");
			return ret;
		}
		if (control->client.direct)
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		else
			ret = buxton_client_set_value(&control->client, key,
						      &set.store.d_uint32, NULL,
						      NULL, true);
		break;
	case INT64:
		set.store.d_int64 = strtoll(value.value, NULL, 10);
		if (errno) {
			printf("Invalid int64_t value\n");
			return ret;
		}
		if (control->client.direct)
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		else
			ret = buxton_client_set_value(&control->client, key,
						      &set.store.d_int64, NULL,
						      NULL, true);
		break;
	case UINT64:
		set.store.d_uint64 = strtoull(value.value, NULL, 10);
		if (errno) {
			printf("Invalid uint64_t value\n");
			return ret;
		}
		if (control->client.direct)
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		else
			ret = buxton_client_set_value(&control->client, key,
						      &set.store.d_uint64, NULL,
						      NULL, true);
		break;
	case FLOAT:
		set.store.d_float = strtof(value.value, NULL);
		if (errno) {
			printf("Invalid float value\n");
			return ret;
		}
		if (control->client.direct)
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		else
			ret = buxton_client_set_value(&control->client, key,
						      &set.store.d_float, NULL,
						      NULL, true);
		break;
	case DOUBLE:
		set.store.d_double = strtod(value.value, NULL);
		if (errno) {
			printf("Invalid double value\n");
			return ret;
		}
		if (control->client.direct)
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		else
			ret = buxton_client_set_value(&control->client, key,
						      &set.store.d_double, NULL,
						      NULL, true);
		break;
	case BOOLEAN:
		if (strcaseeq(value.value, "true") ||
		    strcaseeq(value.value, "on") ||
		    strcaseeq(value.value, "enable") ||
		    strcaseeq(value.value, "yes") ||
		    strcaseeq(value.value, "y") ||
		    strcaseeq(value.value, "t") ||
		    strcaseeq(value.value, "1"))
			set.store.d_boolean = true;
		else if (strcaseeq(value.value, "false") ||
			 strcaseeq(value.value, "off") ||
			 strcaseeq(value.value, "disable") ||
			 strcaseeq(value.value, "no") ||
			 strcaseeq(value.value, "n") ||
			 strcaseeq(value.value, "f") ||
			 strcaseeq(value.value, "0"))
			set.store.d_boolean = false;
		else {
			printf("Invalid bool value\n");
			return ret;
		}
		if (control->client.direct)
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		else
			ret = buxton_client_set_value(&control->client, key,
						      &set.store.d_boolean,
						      NULL, NULL, true);
		break;
	default:
		break;
	}

	if (!ret)
		printf("Failed to update key \'%s:%s\' in layer '%s'\n", get_group(key),
		       get_name(key), get_layer(key));

	free(key);
	return ret;
}

void get_value_callback(BuxtonResponse response, void *data)
{
	BuxtonKey key;
	BuxtonData *r = (BuxtonData *)data;
	void *p;

	if (response_status(response) != BUXTON_STATUS_OK)
		return;

	p = response_value(response);
	if (!p)
		return;
	key = response_key(response);
	if (!key) {
		free(p);
		return;
	}

	switch (buxton_get_type(key)) {
	case STRING:
		r->store.d_string.value = (char *)p;
		r->store.d_string.length = (uint32_t)strlen(r->store.d_string.value) + 1;
		r->type = STRING;
		break;
	case INT32:
		r->store.d_int32 = *(int32_t *)p;
		r->type = INT32;
		break;
	case UINT32:
		r->store.d_uint32 = *(uint32_t *)p;
		r->type = UINT32;
		break;
	case INT64:
		r->store.d_int64 = *(int64_t *)p;
		r->type = INT64;
		break;
	case UINT64:
		r->store.d_uint64 = *(uint64_t *)p;
		r->type = UINT64;
		break;
	case FLOAT:
		r->store.d_float = *(float *)p;
		r->type = FLOAT;
		break;
	case DOUBLE:
		r->store.d_double = *(double *)p;
		r->type = DOUBLE;
		break;
	case BOOLEAN:
		r->store.d_boolean = *(bool *)p;
		r->type = BOOLEAN;
		break;
	default:
		break;
	}

	if (buxton_get_type(key) != STRING)
		free(p);
	free(key);
}

bool cli_get_value(BuxtonControl *control, BuxtonDataType type,
		   char *one, char *two, char *three, __attribute__((unused)) char * four)
{
	BuxtonKey key;
	BuxtonData get;
	_cleanup_free_ char *prefix = NULL;
	bool ret = false;

	memzero((void*)&get, sizeof(BuxtonData));
	if (three != NULL) {
		key = buxton_make_key(two, three, one, type);
		asprintf(&prefix, "[%s] ", one);
	} else {
		key = buxton_make_key(one, two, NULL, type);
		asprintf(&prefix, " ");
	}

	if (!key)
		return false;

	if (three != NULL) {
		if (control->client.direct)
			ret = buxton_direct_get_value_for_layer(control, key,
								&get, NULL,
								NULL);
		else
			ret = buxton_client_get_value(&control->client,
						      key,
						      get_value_callback,
						      &get, true);
		if (!ret) {
			printf("Requested key was not found in layer \'%s\': %s:%s\n",
			       one, get_group(key), get_name(key));
			return false;
		}
	} else {
		if (control->client.direct)
			ret = buxton_direct_get_value(control, key, &get, NULL, NULL);
		else
			ret = buxton_client_get_value(&control->client, key,
						      get_value_callback, &get,
						      true);
		if (!ret) {
			printf("Requested key was not found: %s:%s\n", get_group(key),
			       get_name(key));
			return false;
		}
	}

	switch (get.type) {
	case STRING:
		printf("%s%s:%s = %s\n", prefix, get_group(key), get_name(key),
		       get.store.d_string.value ? get.store.d_string.value : "");
		break;
	case INT32:
		printf("%s%s:%s = %" PRId32 "\n", prefix, get_group(key),
		       get_name(key), get.store.d_int32);
		break;
	case UINT32:
		printf("%s%s:%s = %" PRIu32 "\n", prefix, get_group(key),
		       get_name(key), get.store.d_uint32);
		break;
	case INT64:
		printf("%s%s:%s = %" PRId64 "\n", prefix, get_group(key),
		       get_name(key), get.store.d_int64);
		break;
	case UINT64:
		printf("%s%s:%s = %" PRIu64 "\n", prefix, get_group(key),
		       get_name(key), get.store.d_uint64);
		break;
	case FLOAT:
		printf("%s%s:%s = %f\n", prefix, get_group(key),
		       get_name(key), get.store.d_float);
		break;
	case DOUBLE:
		printf("%s%s:%s = %f\n", prefix, get_group(key),
		       get_name(key), get.store.d_double);
		break;
	case BOOLEAN:
		if (get.store.d_boolean == true)
			printf("%s%s:%s = true\n", prefix, get_group(key),
			       get_name(key));
		else
			printf("%s%s:%s = false\n", prefix, get_group(key),
			       get_name(key));
		break;
	case BUXTON_TYPE_MIN:
		printf("Requested key was not found: %s:%s\n", get_group(key),
			       get_name(key));
		return false;
		break;
	default:
		printf("unknown type\n");
		return false;
		break;
	}

	if (get.type == STRING)
		free(get.store.d_string.value);
	return true;
}

static void list_keys_callback(BuxtonResponse response, void *data)
{
	_BuxtonResponse *r = (_BuxtonResponse *)response;
	BuxtonArray *array = r->data;
	BuxtonData *d;
	//FIXME change to use api (make api first)
	if (response_status(response) != BUXTON_STATUS_OK)
		return;

	BuxtonString *layer = (BuxtonString *)data;
	if (!layer)
		return;

	/* Print all of the keys in this layer */
	printf("%d keys found in layer \'%s\':\n", array->len - 1, layer->value);
	for (uint16_t i = 1; i < array->len; i++) {
		d = buxton_array_get(array, i);
		if (!d)
			break;
		printf("%s\n", d->store.d_string.value);
	}
}

bool cli_list_keys(BuxtonControl *control,
		   __attribute__((unused))BuxtonDataType type,
		   char *one, char *two, char *three,
		   __attribute__((unused)) char *four)
{
	BuxtonString layer;
	BuxtonArray *results = NULL;
	BuxtonData *current = NULL;
	bool ret = false;
	int i;

	layer.value = one;
	layer.length = (uint32_t)strlen(one) + 1;

	if (control->client.direct)
		ret = buxton_direct_list_keys(control, &layer, &results);
	else
		ret = buxton_client_list_keys(&(control->client), one,
					      list_keys_callback, &layer, true);
	if (!ret) {
		printf("No keys found for layer \'%s\'\n", one);
		return false;
	}
	if (results == NULL && control->client.direct)
		return false;

	if (control->client.direct) {
		/* Print all of the keys in this layer */
		printf("%d keys found in layer \'%s\':\n", results->len, one);
		for (i = 0; i < results->len; i++) {
			current = (BuxtonData*)results->data[i];
			printf("%s\n", current->store.d_string.value);
			free(current->store.d_string.value);
		}

		buxton_array_free(&results, NULL);
	}

	return true;
}

void unset_value_callback(BuxtonResponse response, void *data)
{
	BuxtonKey key = response_key(response);

	if (!key)
		return;

	printf("unset key %s:%s\n", buxton_get_group(key), buxton_get_name(key));

	buxton_free_key(key);
}

bool cli_unset_value(BuxtonControl *control,
		     BuxtonDataType type,
		     char *one, char *two, char *three,
		     __attribute__((unused)) char *four)
{
	BuxtonKey key;

	key = buxton_make_key(two, three, one, type);

	if (!key)
		return false;

	if (control->client.direct)
		return buxton_direct_unset_value(control, key, NULL);
	else
		return buxton_client_unset_value(&control->client,
						 key, unset_value_callback,
						 NULL, true);

	buxton_free_key(key);
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
