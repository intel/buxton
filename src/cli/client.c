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

#include "bt-daemon.h"
#include "client.h"
#include "hashmap.h"
#include "util.h"
#include "buxton-array.h"

bool cli_set_label(BuxtonControl *control, __attribute__((unused)) BuxtonDataType type,
		   char *one, char *two, char *three, char *four)
{
	BuxtonString layer, label;
	_cleanup_buxton_string_ BuxtonString *key = NULL;
	bool ret = false;

	layer = buxton_string_pack(one);

	if (four != NULL)
		key = buxton_make_key(two, three);
	else
		key = buxton_make_key(two, NULL);

	if (!key)
		return ret;

	if (four != NULL)
		label = buxton_string_pack(four);
	else
		label = buxton_string_pack(three);

	ret = buxton_direct_set_label(control, &layer, key, &label);

	if (!ret)
		printf("Failed to update key \'%s:%s\' label in layer '%s'\n",
		       get_group(key), get_name(key), layer.value);
	return ret;
}

bool cli_get_label(BuxtonControl *control, __attribute__((unused)) BuxtonDataType type,
		   char *one, char *two, char *three, __attribute__((unused)) char *four)
{
	BuxtonString layer;
	_cleanup_buxton_string_ BuxtonString *key = NULL;
	BuxtonData get;
	bool ret = false;

	memset((void*)&get, 0, sizeof(BuxtonData));
	layer = buxton_string_pack(one);
	key = buxton_make_key(two, three);
	if (!key)
		return ret;

	if (control->client.direct)
		ret = buxton_direct_get_value_for_layer(control, &layer, key,
							&get, NULL);
	else
		ret = buxton_client_get_value_for_layer(&control->client, &layer,
							key, NULL, NULL, true);
	if (!ret)
		printf("Failed to get key \'%s:%s\' in layer '%s'\n", get_group(key),
		       get_name(key), layer.value);
	else
		printf("[%s][%s:%s] = %s\n", layer.value, get_group(key),
		       get_name(key),
		       get.label.value ? get.label.value : "");

	return ret;
}

bool cli_set_value(BuxtonControl *control, BuxtonDataType type,
		   char *one, char *two, char *three, char *four)
{
	BuxtonString layer, value;
	_cleanup_buxton_string_ BuxtonString *key = NULL;
	BuxtonData set;
	bool ret = false;

	memset((void*)&set, 0, sizeof(BuxtonData));
	layer.value = one;
	layer.length = (uint32_t)strlen(one) + 1;
	key = buxton_make_key(two, three);
	if (!key)
		return ret;
	value.value = four;
	value.length = (uint32_t)strlen(four) + 1;

	set.label = buxton_string_pack("_");
	set.type = type;
	switch (set.type) {
	case STRING:
		set.store.d_string.value = value.value;
		set.store.d_string.length = value.length;
		break;
	case INT32:
		set.store.d_int32 = (int32_t)strtol(value.value, NULL, 10);
		if (errno) {
			printf("Invalid int32_t value\n");
			return ret;
		}
		break;
	case UINT32:
		set.store.d_uint32 = (uint32_t)strtol(value.value, NULL, 10);
		if (errno) {
			printf("Invalid uint32_t value\n");
			return ret;
		}
		break;
	case INT64:
		set.store.d_int64 = strtoll(value.value, NULL, 10);
		if (errno) {
			printf("Invalid int64_t value\n");
			return ret;
		}
		break;
	case UINT64:
		set.store.d_uint64 = strtoull(value.value, NULL, 10);
		if (errno) {
			printf("Invalid uint64_t value\n");
			return ret;
		}
		break;
	case FLOAT:
		set.store.d_float = strtof(value.value, NULL);
		if (errno) {
			printf("Invalid float value\n");
			return ret;
		}
		break;
	case DOUBLE:
		set.store.d_double = strtod(value.value, NULL);
		if (errno) {
			printf("Invalid double value\n");
			return ret;
		}
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
		break;
	default:
		break;
	}
	if (control->client.direct)
		ret = buxton_direct_set_value(control, &layer, key, &set, NULL);
	else
		ret = buxton_client_set_value(&control->client, &layer, key,
					      &set, NULL, NULL, true);
	if (!ret)
		printf("Failed to update key \'%s:%s\' in layer '%s'\n", get_group(key),
		       get_name(key), layer.value);
	return ret;
}

bool cli_get_value(BuxtonControl *control, BuxtonDataType type,
		   char *one, char *two, char *three, __attribute__((unused)) char * four)
{
	BuxtonString layer;
	_cleanup_buxton_string_ BuxtonString *key = NULL;
	BuxtonData get;
	_cleanup_free_ char *prefix = NULL;
	bool ret = false;

	memset((void*)&get, 0, sizeof(BuxtonData));
	if (three != NULL) {
		layer.value = one;
		layer.length = (uint32_t)strlen(one) + 1;
		key = buxton_make_key(two, three);
		asprintf(&prefix, "[%s] ", layer.value);
	} else {
		key = buxton_make_key(one, two);
		asprintf(&prefix, " ");
	}

	if (!key)
		return false;

	if (three != NULL) {
		if (control->client.direct)
			ret = buxton_direct_get_value_for_layer(control, &layer,
								key, &get, NULL);
		else
			ret = buxton_client_get_value_for_layer(&control->client,
								&layer, key,
								NULL, NULL, true);
		if (!ret) {
			printf("Requested key was not found in layer \'%s\': %s:%s\n",
			       layer.value, get_group(key), get_name(key));
			return false;
		}
	} else {
		if (control->client.direct)
			ret = buxton_direct_get_value(control, key, &get, NULL);
		else
			ret = buxton_client_get_value(&control->client, key,
						      NULL, NULL, true);
		if (!ret) {
			printf("Requested key was not found: %s:%s\n", get_group(key),
			       get_name(key));
			return false;
		}
	}

	if (get.type != type) {
		const char *type_req, *type_got;
		type_req = buxton_type_as_string(type);
		type_got = buxton_type_as_string(get.type);
		printf("You requested a key with type \'%s\', but value is of type \'%s\'.\n\n", type_req, type_got);
		return false;
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
		printf("%s%s:%s = %" PRIo32 "\n", prefix, get_group(key),
		       get_name(key), get.store.d_uint32);
		break;
	case INT64:
		printf("%s%s:%s = %" PRId64 "\n", prefix, get_group(key),
		       get_name(key), get.store.d_int64);
		break;
	case UINT64:
		printf("%s%s:%s = %" PRIo64 "\n", prefix, get_group(key),
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
	default:
		printf("unknown type\n");
		return false;
		break;
	}

	if (get.type == STRING)
		free(get.store.d_string.value);
	free(get.label.value);
	return true;
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
		ret = buxton_client_list_keys(&(control->client), &layer,
					      NULL, NULL, true);
	if (!ret) {
		printf("No keys found for layer \'%s\'\n", one);
		return false;
	}
	if (results == NULL)
		return false;
	/* Print all of the keys in this layer */
	printf("%d keys found in layer \'%s\'\n", results->len, one);
	for (i = 0; i < results->len; i++) {
		current = (BuxtonData*)results->data[i];
		printf("%s\n", current->store.d_string.value);
		free(current->store.d_string.value);
	}

	buxton_array_free(&results, NULL);
	return true;
}

bool cli_unset_value(BuxtonControl *control,
		     __attribute__((unused))BuxtonDataType type,
		     char *one, char *two, char *three,
		     __attribute__((unused)) char *four)
{
	BuxtonString layer;
	_cleanup_buxton_string_ BuxtonString *key = NULL;

	layer.value = one;
	layer.length = (uint32_t)strlen(one) + 1;
	key = buxton_make_key(two, three);

	if (!key)
		return false;

	if (control->client.direct)
		return buxton_direct_unset_value(control, &layer, key, NULL);
	else
		return buxton_client_unset_value(&control->client, &layer,
						 key, NULL, NULL, true);
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
