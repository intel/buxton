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

bool cli_set_label(BuxtonClient *self, __attribute__((unused)) BuxtonDataType type,
		   char *one, char *two, char *three, char *four)
{
	BuxtonString layer, label;
	BuxtonString *key;
	bool ret = false;

	layer = buxton_string_pack(one);
	key = buxton_make_key(two, three);
	if (!key)
		return ret;
	label = buxton_string_pack(four);

	ret = buxton_client_set_label(self, &layer, key, &label);
	if (!ret)
		printf("Failed to update key \'%s:%s\' label in layer '%s'\n",
		       buxton_get_group(key), buxton_get_name(key), layer.value);
	return ret;
}

bool cli_get_label(BuxtonClient *self, __attribute__((unused)) BuxtonDataType type,
		   char *one, char *two, char *three, __attribute__((unused)) char *four)
{
	BuxtonString layer;
	BuxtonString *key;
	BuxtonData get;
	bool ret = false;

	layer = buxton_string_pack(one);
	key = buxton_make_key(two, three);
	if (!key)
		return ret;

	ret = buxton_client_get_value_for_layer(self, &layer, key, &get);
	if (!ret)
		printf("Failed to get key \'%s:%s\' in layer '%s'\n", buxton_get_group(key),
		       buxton_get_name(key), layer.value);
	else
		printf("[%s][%s:%s] = %s\n", layer.value, buxton_get_group(key),
		       buxton_get_name(key), get.label.value);

	return ret;
}

bool cli_set_value(BuxtonClient *self, BuxtonDataType type,
		   char *one, char *two, char *three, char *four)
{
	BuxtonString layer, value;
	BuxtonString *key;
	BuxtonData set;
	bool ret = false;

	layer.value = one;
	layer.length = strlen(one) + 1;
	key = buxton_make_key(two, three);
	if (!key)
		return ret;
	value.value = three;
	value.length = strlen(three) + 1;

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
			return false;
		}
		break;
	case INT64:
		set.store.d_int64 = strtoll(value.value, NULL, 10);
		if (errno) {
			printf("Invalid int64_t value\n");
			return false;
		}
		break;
	case FLOAT:
		set.store.d_float = strtof(value.value, NULL);
		if (errno) {
			printf("Invalid float value\n");
			return false;
		}
		break;
	case DOUBLE:
		set.store.d_double = strtod(value.value, NULL);
		if (errno) {
			printf("Invalid double value\n");
			return false;
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
			return false;
		}
		break;
	default:
		break;
	}
	ret = buxton_client_set_value(self, &layer, key, &set);
	if (!ret)
		printf("Failed to update key \'%s:%s\' in layer '%s'\n", buxton_get_group(key),
		       buxton_get_name(key), layer.value);
	return ret;
}

bool cli_get_value(BuxtonClient *self, BuxtonDataType type,
		   char *one, char *two, char *three, __attribute__((unused)) char * four)
{
	BuxtonString layer;
	BuxtonString *key;
	BuxtonData get;
	_cleanup_free_ char *prefix = NULL;

	if (three != NULL) {
		layer.value = one;
		layer.length = strlen(one) + 1;
		key = buxton_make_key(two, three);
		asprintf(&prefix, "[%s] ", layer.value);
	} else {
		key = buxton_make_key(one, two);
		asprintf(&prefix, " ");
	}

	if (!key)
		return false;

	if (two != NULL) {
		if (!buxton_client_get_value_for_layer(self, &layer, key, &get)) {
			printf("Requested key was not found in layer \'%s\': %s:%s\n",
			       layer.value, buxton_get_group(key), buxton_get_name(key));
			return false;
		}
	} else {
		if (!buxton_client_get_value(self, key, &get)) {
			printf("Requested key was not found: %s:%s\n", buxton_get_group(key),
			       buxton_get_name(key));
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
		printf("%s%s:%s = %s\n", prefix, buxton_get_group(key), buxton_get_name(key),
		       get.store.d_string.value);
		break;
	case INT32:
		printf("%s%s:%s = %" PRId32 "\n", prefix, buxton_get_group(key),
		       buxton_get_name(key), get.store.d_int32);
		break;
	case INT64:
		printf("%s%s:%s = %" PRId64 "\n", prefix, buxton_get_group(key),
		       buxton_get_name(key), get.store.d_int64);
		break;
	case FLOAT:
		printf("%s%s:%s = %f\n", prefix, buxton_get_group(key),
		       buxton_get_name(key), get.store.d_float);
		break;
	case DOUBLE:
		printf("%s%s:%s = %f\n", prefix, buxton_get_group(key),
		       buxton_get_name(key), get.store.d_double);
		break;
	case BOOLEAN:
		if (get.store.d_boolean == true)
			printf("%s%s:%s = true\n", prefix, buxton_get_group(key),
			       buxton_get_name(key));
		else
			printf("%s%s:%s = false\n", prefix, buxton_get_group(key),
			       buxton_get_name(key));
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
