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
		   char *one, char *two, char *three)
{
	BuxtonString layer, key, label;
	bool ret = false;

	layer = buxton_string_pack(one);
	key = buxton_string_pack(two);
	label = buxton_string_pack(three);

	ret = buxton_client_set_label(self, &layer, &key, &label);
	if (!ret)
		printf("Failed to update key \'%s\' label in layer '%s'\n", key.value, layer.value);
	return ret;
}

bool cli_get_label(BuxtonClient *self, __attribute__((unused)) BuxtonDataType type,
		   char *one, char *two, __attribute__((unused)) char *three)
{
	BuxtonString layer, key;
	BuxtonData get;
	bool ret = false;

	layer = buxton_string_pack(one);
	key = buxton_string_pack(two);

	ret = buxton_client_get_value_for_layer(self, &layer, &key, &get);
	if (!ret)
		printf("Failed to get key \'%s\' in layer '%s'\n", key.value, layer.value);
	else
		printf("[%s][%s] = %s\n", layer.value, key.value, get.label.value);

	return ret;
}

bool cli_set_value(BuxtonClient *self, BuxtonDataType type,
		   char *one, char *two, char *three)
{
	BuxtonString layer, key, value;
	BuxtonData set;

	layer.value = one;
	layer.length = strlen(one) + 1;
	key.value = two;
	key.length = strlen(two) + 1;
	value.value = three;
	value.length = strlen(three) + 1;

	bool ret = false;

	set.label = buxton_string_pack("_");
	set.type = type;
	switch (set.type) {
	case STRING:
		set.store.d_string.value = value.value;
		set.store.d_string.length = value.length;
		break;
	case BOOLEAN:
		if (streq(value.value, "true"))
			set.store.d_boolean = true;
		else if (streq(value.value, "false"))
			set.store.d_boolean = false;
		else {
			printf("Accepted values are [true] [false]. Not updating\n");
			return false;
		}
		break;
	case FLOAT:
		set.store.d_float = strtof(value.value, NULL);
		if (errno) {
			printf("Invalid floating point value\n");
			return false;
		}
		break;
	case DOUBLE:
		set.store.d_double = strtod(value.value, NULL);
		if (errno) {
			printf("Invalid double precision value\n");
			return false;
		}
		break;
	case LONG:
		set.store.d_long = strtol(value.value, NULL, 10);
		if (errno) {
			printf("Invalid long integer value\n");
			return false;
		}
		break;
	case INT:
		set.store.d_int = strtol(value.value, NULL, 10);
		if (errno) {
			printf("Invalid integer\n");
			return false;
		}
		break;
	default:
		break;
	}
	ret = buxton_client_set_value(self, &layer, &key, &set);
	if (!ret)
		printf("Failed to update key \'%s\' in layer '%s'\n", key.value, layer.value);
	return ret;
}

bool cli_get_value(BuxtonClient *self, BuxtonDataType type,
		   char *one, char *two, __attribute__((unused)) char *three)
{
	BuxtonString layer, key;
	BuxtonData get;
	_cleanup_free_ char *prefix = NULL;

	if (two != NULL) {
		layer.value = one;
		layer.length = strlen(one) + 1;
		key.value = two;
		key.length = strlen(two) + 1;
		asprintf(&prefix, "[%s] ", layer.value);
	} else {
		key.value = one;
		key.length = strlen(one) + 1;
		asprintf(&prefix, " ");
	}

	if (two != NULL) {
		if (!buxton_client_get_value_for_layer(self, &layer, &key, &get)) {
			printf("Requested key was not found in layer \'%s\': %s\n", layer.value, key.value);
			return false;
		}
	} else {
		if (!buxton_client_get_value(self, &key, &get)) {
			printf("Requested key was not found: %s\n", key.value);
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
		printf("%s%s = %s\n", prefix, key.value, get.store.d_string.value);
		break;
	case BOOLEAN:
		if (get.store.d_boolean == true)
			printf("%s%s = true\n", prefix, key.value);
		else
			printf("%s%s = false\n", prefix, key.value);
		break;
	case FLOAT:
		printf("%s%s = %f\n", prefix, key.value, get.store.d_float);
		break;
	case DOUBLE:
		printf("%s%s = %f\n", prefix, key.value, get.store.d_double);
		break;
	case LONG:
		printf("%s%s = %ld\n", prefix, key.value, get.store.d_long);
		break;
	case INT:
		printf("%s%s = %d\n", prefix, key.value, get.store.d_int);
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
