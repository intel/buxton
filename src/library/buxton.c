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

/**
 * \file buxton.c Buxton client library
 */


#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <stdlib.h>

#include "buxton.h"
#include "bt-daemon.h" /* Rename this part of the library */
#include "util.h"

/* Keep a global client instead of having the using application
 * refer to yet another client struct. Also consider being
 * able to export this client fd */
static BuxtonClient __client;
static bool __setup = false;


/**
 * Ensure client is connected
 */
static bool prepare_client(void);

/**
 * Ensure we close Buxton connection properly
 */
static void __cleanup(void);

/**
 * Utility, convert BuxtonData to BuxtonValue
 */
static BuxtonValue* convert_from_buxton(BuxtonData *data);

/**
 * Utility, convert BuxtonValue to BuxtonData
 */
static bool convert_to_buxton(BuxtonValue *value, BuxtonData *data);


BuxtonValue* buxton_get_value(char *layer,
			      char *group,
			      char *key)
{
	BuxtonString k_layer;
	_cleanup_buxton_string_ BuxtonString *k_key;
	BuxtonData get;
	BuxtonValue *value = NULL;
	bool ret;

	if (!prepare_client())
		return NULL;

	k_layer = buxton_string_pack(layer);
	k_key = buxton_make_key(group, key);
	if (!key)
		return NULL;

	ret = buxton_client_get_value_for_layer(&__client, &k_layer,
		k_key, &get);
	if (!ret)
		return NULL;

	value = convert_from_buxton(&get);
	if (!value)
		/* Raise error? */
		return NULL;

	if (get.type == STRING)
		free(get.store.d_string.value);

	return value;
}

bool buxton_set_value(char *layer,
		      char *group,
		      char *key,
		      BuxtonValue *data)
{
	BuxtonString k_layer;
	_cleanup_buxton_string_ BuxtonString *k_key;
	BuxtonData set;
	bool ret = false;

	if (!prepare_client())
		return false;

	k_layer = buxton_string_pack(layer);
	k_key = buxton_make_key(group, key);
	if (!key)
		return false;

	if (!convert_to_buxton(data, &set))
		return false;

	/* Check... */
	ret = buxton_client_set_value(&__client, &k_layer, k_key,
		&set);

	if (set.type == STRING)
		free(set.store.d_string.value);

	return ret;
}

bool buxton_unset_value(char *layer,
		        char *group,
		        char *key)
{
	BuxtonString k_layer;
	_cleanup_buxton_string_ BuxtonString *k_key;
	bool ret = false;

	if (!prepare_client())
		return false;

	k_layer = buxton_string_pack(layer);
	k_key = buxton_make_key(group, key);
	if (!k_key)
		return false;

	ret = buxton_client_unset_value(&__client, &k_layer, k_key);

	return ret;
}

void buxton_free_value(BuxtonValue *p)
{
	if (!p)
		return;
	free(p->store);
	free(p);
}

static bool prepare_client(void)
{
	bool ret = false;

	if (__setup)
		return true;

	ret = buxton_client_open(&__client);
	if (!ret)
		return false;

	/* Registering our handler after the buxton library ensures
	 * our one runs first */
	if (!__setup)
		atexit(__cleanup);
	__setup = true;
	return ret;
}

static void __cleanup(void)
{
	if (!__setup)
		return;

	buxton_client_close(&__client);
}

static BuxtonValue* convert_from_buxton(BuxtonData *data)
{
	BuxtonValue *value =  NULL;

	value = malloc0(sizeof(BuxtonValue));
	if (!value)
		return NULL;

	switch (data->type)
	{
		case STRING:
			value->store = strdup(data->store.d_string.value);
			if (!value->store)
				return NULL;
			free(data->store.d_string.value);
			value->type = BUXTON_STRING;
			break;
		case INT32:
			value->store = malloc0(sizeof(int32_t));
			if (!value->store)
				return NULL;
			value->type = BUXTON_INT32;
			*(int32_t*)value->store = data->store.d_int32;
			break;
		case INT64:
			value->store = malloc0(sizeof(int64_t));
			if (!value->store)
				return NULL;
			value->type = BUXTON_INT64;
			*(int64_t*)value->store = data->store.d_int64;
			break;
		case FLOAT:
			value->store = malloc0(sizeof(float));
			if (!value->store)
				return NULL;
			value->type = BUXTON_FLOAT;
			*(float*)value->store = data->store.d_float;
			break;
		case DOUBLE:
			value->store = malloc0(sizeof(double));
			if (!value->store)
				return NULL;
			value->type = BUXTON_DOUBLE;
			*(double*)value->store = data->store.d_double;
			break;
		case BOOLEAN:
			value->store = malloc0(sizeof(bool));
			if (!value->store)
				return NULL;
			value->type = BUXTON_BOOLEAN;
			*(bool*)value->store = data->store.d_boolean;
			break;
		default:
			return NULL;
	}
	return value;
}

static bool convert_to_buxton(BuxtonValue *value, BuxtonData *data)
{
	if (!value->store)
		return false;

	switch (value->type)
	{
		case BUXTON_STRING:
			data->type = STRING;
			data->store.d_string = buxton_string_pack(value->store);
			break;
		case BUXTON_INT32:
			data->type = INT32;
			data->store.d_int32 = *(int32_t*)value->store;
			break;
		case BUXTON_INT64:
			data->type = INT64;
			data->store.d_int64 = *(int64_t*)value->store;
			break;
		case BUXTON_FLOAT:
			data->type = FLOAT;
			data->store.d_float = *(float*)value->store;
			break;
		case BUXTON_DOUBLE:
			value->type = DOUBLE;
			data->store.d_double = *(double*)value->store;
			break;
		case BUXTON_BOOLEAN:
			value->type = BOOLEAN;
			data->store.d_boolean = *(bool*)value->store;
			break;
		default:
			return false;
	}
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
