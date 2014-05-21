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

static char *nv(char *s)
{
	if (s) {
		return s;
	}
	return "(null)";
}

bool cli_create_db(BuxtonControl *control,
		   __attribute__((unused)) BuxtonDataType type,
		   char *one,
		   __attribute__((unused)) char *two,
		   __attribute__((unused)) char *three,
		   __attribute__((unused)) char *four)
{
	BuxtonString layer_name;
	bool ret;

	if (!control->client.direct) {
		printf("Unable to create db in non direct mode\n");
	}

	layer_name = buxton_string_pack(one);

	ret = buxton_direct_init_db(control, &layer_name);

	return ret;
}

bool cli_set_label(BuxtonControl *control, BuxtonDataType type,
		   char *one, char *two, char *three, char *four)
{
	BuxtonString label;
	BuxtonKey key;
	bool ret = false;

	if (four != NULL) {
		key = buxton_key_create(two, three, one, type);
	} else {
		key = buxton_key_create(two, NULL, one, type);
	}

	if (!key) {
		return ret;
	}

	if (four != NULL) {
		label = buxton_string_pack(four);
	} else {
		label = buxton_string_pack(three);
	}

	if (control->client.direct) {
		ret = buxton_direct_set_label(control, (_BuxtonKey *)key, &label);
	} else {
		ret = !buxton_set_label(&control->client, key, label.value,
					NULL, NULL, true);
	}

	if (!ret) {
		char *name = get_name(key);
		printf("Failed to update key \'%s:%s\' label in layer '%s'\n",
		       two, nv(name), one);
		free(name);
	}
	buxton_key_free(key);
	return ret;
}

bool cli_create_group(BuxtonControl *control, BuxtonDataType type,
		      char *one, char *two, char *three, char *four)
{
	BuxtonKey key;
	bool ret = false;

	key = buxton_key_create(two, NULL, one, type);
	if (!key) {
		return ret;
	}

	if (control->client.direct) {
		ret = buxton_direct_create_group(control, (_BuxtonKey *)key, NULL);
	} else {
		ret = !buxton_create_group(&control->client, key, NULL, NULL, true);
	}

	if (!ret) {
		char *group = get_group(key);
		printf("Failed to create group \'%s\' in layer '%s'\n",
		       nv(group), one);
		free(group);
	}
	buxton_key_free(key);
	return ret;
}

bool cli_remove_group(BuxtonControl *control, BuxtonDataType type,
		      char *one, char *two, char *three, char *four)
{
	BuxtonKey key;
	bool ret = false;

	key = buxton_key_create(two, NULL, one, type);
	if (!key) {
		return ret;
	}

	if (control->client.direct) {
		ret = buxton_direct_remove_group(control, (_BuxtonKey *)key, NULL);
	} else {
		ret = !buxton_remove_group(&control->client, key, NULL, NULL, true);
	}

	if (!ret) {
		char *group = get_group(key);
		printf("Failed to remove group \'%s\' in layer '%s'\n",
		       nv(group), one);
		free(group);
	}
	buxton_key_free(key);
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
	key = buxton_key_create(two, three, one, type);
	if (!key) {
		return ret;
	}

	value.value = four;
	value.length = (uint32_t)strlen(four) + 1;

	set.type = type;
	switch (set.type) {
	case STRING:
		set.store.d_string.value = value.value;
		set.store.d_string.length = value.length;
		if (control->client.direct) {
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		} else {
			ret = !buxton_set_value(&control->client, key,
						four, NULL, NULL, true);
		}
		break;
	case INT32:
		set.store.d_int32 = (int32_t)strtol(four, NULL, 10);
		if (errno) {
			printf("Invalid int32_t value\n");
			return ret;
		}
		if (control->client.direct) {
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		} else {
			ret = !buxton_set_value(&control->client, key,
						&set.store.d_int32, NULL,
						NULL, true);
		}
		break;
	case UINT32:
		set.store.d_uint32 = (uint32_t)strtol(value.value, NULL, 10);
		if (errno) {
			printf("Invalid uint32_t value\n");
			return ret;
		}
		if (control->client.direct) {
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		} else {
			ret = !buxton_set_value(&control->client, key,
						&set.store.d_uint32, NULL,
						NULL, true);
		}
		break;
	case INT64:
		set.store.d_int64 = strtoll(value.value, NULL, 10);
		if (errno) {
			printf("Invalid int64_t value\n");
			return ret;
		}
		if (control->client.direct) {
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		} else {
			ret = !buxton_set_value(&control->client, key,
						&set.store.d_int64, NULL,
						NULL, true);
		}
		break;
	case UINT64:
		set.store.d_uint64 = strtoull(value.value, NULL, 10);
		if (errno) {
			printf("Invalid uint64_t value\n");
			return ret;
		}
		if (control->client.direct) {
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		} else {
			ret = !buxton_set_value(&control->client, key,
						&set.store.d_uint64, NULL,
						NULL, true);
		}
		break;
	case FLOAT:
		set.store.d_float = strtof(value.value, NULL);
		if (errno) {
			printf("Invalid float value\n");
			return ret;
		}
		if (control->client.direct) {
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		} else {
			ret = !buxton_set_value(&control->client, key,
						&set.store.d_float, NULL,
						NULL, true);
		}
		break;
	case DOUBLE:
		set.store.d_double = strtod(value.value, NULL);
		if (errno) {
			printf("Invalid double value\n");
			return ret;
		}
		if (control->client.direct) {
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		} else {
			ret = !buxton_set_value(&control->client, key,
						&set.store.d_double, NULL,
						NULL, true);
		}
		break;
	case BOOLEAN:
		if (strcaseeq(value.value, "true") ||
		    strcaseeq(value.value, "on") ||
		    strcaseeq(value.value, "enable") ||
		    strcaseeq(value.value, "yes") ||
		    strcaseeq(value.value, "y") ||
		    strcaseeq(value.value, "t") ||
		    strcaseeq(value.value, "1")) {
			set.store.d_boolean = true;
		} else if (strcaseeq(value.value, "false") ||
			 strcaseeq(value.value, "off") ||
			 strcaseeq(value.value, "disable") ||
			 strcaseeq(value.value, "no") ||
			 strcaseeq(value.value, "n") ||
			 strcaseeq(value.value, "f") ||
			 strcaseeq(value.value, "0")) {
			set.store.d_boolean = false;
		} else {
			printf("Invalid bool value\n");
			return ret;
		}
		if (control->client.direct) {
			ret = buxton_direct_set_value(control,
						      (_BuxtonKey *)key,
						      &set, NULL);
		} else {
			ret = !buxton_set_value(&control->client, key,
						&set.store.d_boolean,
						NULL, NULL, true);
		}
		break;
	default:
		break;
	}

	if (!ret) {
		char *group = get_group(key);
		char *name = get_name(key);
		char *layer = get_layer(key);

		printf("Failed to update key \'%s:%s\' in layer '%s'\n",
		       nv(group), nv(name), nv(layer));
		free(group);
		free(name);
		free(layer);
	}

	return ret;
}

void get_value_callback(BuxtonResponse response, void *data)
{
	BuxtonKey key;
	BuxtonData *r = (BuxtonData *)data;
	void *p;

	if (buxton_response_status(response) != 0) {
		return;
	}

	p = buxton_response_value(response);
	if (!p) {
		return;
	}
	key = buxton_response_key(response);
	if (!key) {
		free(p);
		return;
	}

	switch (buxton_key_get_type(key)) {
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
		memcpy(&r->store.d_double, p, sizeof(double));
		r->type = DOUBLE;
		break;
	case BOOLEAN:
		r->store.d_boolean = *(bool *)p;
		r->type = BOOLEAN;
		break;
	default:
		break;
	}

	if (buxton_key_get_type(key) != STRING) {
		free(p);
	}
	free(key);
}

bool cli_get_value(BuxtonControl *control, BuxtonDataType type,
		   char *one, char *two, char *three, __attribute__((unused)) char * four)
{
	BuxtonKey key;
	BuxtonData get;
	_cleanup_free_ char *prefix = NULL;
	_cleanup_free_ char *group = NULL;
	_cleanup_free_ char *name = NULL;
	BuxtonString dlabel;
	bool ret = false;
	int32_t ret_val;
	int r;

	memzero((void*)&get, sizeof(BuxtonData));
	if (three != NULL) {
		key = buxton_key_create(two, three, one, type);
		r = asprintf(&prefix, "[%s] ", one);
		if (!r) {
			abort();
		}
	} else {
		key = buxton_key_create(one, two, NULL, type);
		r = asprintf(&prefix, " ");
		if (!r) {
			abort();
		}
	}

	if (!key) {
		return false;
	}

	if (three != NULL) {
		if (control->client.direct) {
			ret = buxton_direct_get_value_for_layer(control, key,
								&get, &dlabel,
								NULL);
		} else {
			ret = buxton_get_value(&control->client,
						      key,
						      get_value_callback,
						      &get, true);
		}
		if (ret) {
			group = get_group(key);
			name = get_name(key);
			printf("Requested key was not found in layer \'%s\': %s:%s\n",
			       one, nv(group), nv(name));
			return false;
		}
	} else {
		if (control->client.direct) {
			ret_val = buxton_direct_get_value(control, key, &get, &dlabel, NULL);
			if (ret_val == 0) {
				ret = true;
			}
		} else {
			ret = buxton_get_value(&control->client, key,
						      get_value_callback, &get,
						      true);
		}
		if (ret) {
			group = get_group(key);
			name = get_name(key);
			printf("Requested key was not found: %s:%s\n", nv(group),
			       nv(name));
			return false;
		}
	}

	group = get_group(key);
	name = get_name(key);
	switch (get.type) {
	case STRING:
		printf("%s%s:%s = %s\n", prefix, nv(group), nv(name),
		       get.store.d_string.value ? get.store.d_string.value : "");
		break;
	case INT32:
		printf("%s%s:%s = %" PRId32 "\n", prefix, nv(group),
		       nv(name), get.store.d_int32);
		break;
	case UINT32:
		printf("%s%s:%s = %" PRIu32 "\n", prefix, nv(group),
		       nv(name), get.store.d_uint32);
		break;
	case INT64:
		printf("%s%s:%s = %" PRId64 "\n", prefix, nv(group),
		       nv(name), get.store.d_int64);
		break;
	case UINT64:
		printf("%s%s:%s = %" PRIu64 "\n", prefix, nv(group),
		       nv(name), get.store.d_uint64);
		break;
	case FLOAT:
		printf("%s%s:%s = %f\n", prefix, nv(group),
		       nv(name), get.store.d_float);
		break;
	case DOUBLE:
		printf("%s%s:%s = %f\n", prefix, nv(group),
		       nv(name), get.store.d_double);
		break;
	case BOOLEAN:
		if (get.store.d_boolean == true) {
			printf("%s%s:%s = true\n", prefix, nv(group),
			       nv(name));
		} else {
			printf("%s%s:%s = false\n", prefix, nv(group),
			       nv(name));
		}
		break;
	case BUXTON_TYPE_MIN:
		printf("Requested key was not found: %s:%s\n", nv(group),
			       nv(name));
		return false;
	default:
		printf("unknown type\n");
		return false;
	}

	if (get.type == STRING) {
		free(get.store.d_string.value);
	}
	return true;
}

bool cli_list_keys(BuxtonControl *control,
		   __attribute__((unused))BuxtonDataType type,
		   char *one, char *two, char *three,
		   __attribute__((unused)) char *four)
{
	/* not yet implemented */
	return false;
}

void unset_value_callback(BuxtonResponse response, void *data)
{
	BuxtonKey key = buxton_response_key(response);
	char *group, *name;

	if (!key) {
		return;
	}

	group = buxton_key_get_group(key);
	name = buxton_key_get_name(key);
	printf("unset key %s:%s\n", nv(group), nv(name));

	free(group);
	free(name);
	buxton_key_free(key);
}

bool cli_unset_value(BuxtonControl *control,
		     BuxtonDataType type,
		     char *one, char *two, char *three,
		     __attribute__((unused)) char *four)
{
	BuxtonKey key;

	key = buxton_key_create(two, three, one, type);

	if (!key) {
		return false;
	}

	if (control->client.direct) {
		return buxton_direct_unset_value(control, key, NULL);
	} else {
		return !buxton_unset_value(&control->client,
					   key, unset_value_callback,
					   NULL, true);
	}
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
