/*
 * This file is part of buxton.
 *
 * Copyright (C) 2014 Intel Corporation
 *
 * buxton is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the license, or (at your option) any later version.
 */
 
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buxton.h"
#include "buxtonsimple-internals.h"
#include "buxtonresponse.h"
#include "log.h"

BuxtonClient client = NULL;

typedef void (*NotifyCallback)(void *, char*);

typedef struct nstatus {
	int status;
	NotifyCallback callback;
} nstatus;

extern BuxtonClient client;
/* Make sure client connection is open */
int _client_connection(void)
{
	/* Check if client connection is open */
	if (!client) {
		/* Open connection if needed */
		if ((buxton_open(&client)) <0 ) {
			buxton_debug("Couldn't connect.\n");
			return 0;
		}	
		buxton_debug("Connection successful.\n");
	}
	return 1;
}

/* Close an open client connection */
void _client_disconnect(void)
{
	/* Only attempt to close the client if it != NULL */
	if (client) {
		/* Close the connection */
		buxton_close(client);
		buxton_debug("Connection closed\n");
		client = NULL;
	}
}

/* Create group callback */
void _cg_cb(BuxtonResponse response, void *data)
{
	int *status = (int *)data;
	*status = 0;
	if (buxton_response_status(response) != 0) {
		buxton_debug("Failed to create group (may already exist).\n");
	} else {
		buxton_debug("Created group.\n");
		*status = 1;
	}
}

/* Debug message for setting buxton key values */
void _bs_print(vstatus *data, BuxtonResponse response)
{
	switch (data->type) {
		case STRING:
		{
			char *val = data->val.sval;
			buxton_debug("Success: value has been set: %s(string). ", val);
			break;
		}
		case INT32:
		{
			int32_t val = data->val.i32val;
			buxton_debug("Success: value has been set: %d(int32_t). ", val);
			break;
		}
		case UINT32:
		{
			uint32_t val = data->val.ui32val;
			buxton_debug("Success: value has been set: %d(uint32_t). ", val);
			break;
		}
		case INT64:
		{
			int64_t val = data->val.i64val;
			buxton_debug("Success: value has been set: ""%"PRId64"(int64_t). ", val);
			break;
		}
		case UINT64:
		{
			uint64_t val = data->val.ui64val;
			buxton_debug("Success: value has been set: ""%"PRIu64"(uint64_t). ", val);
			break;
		}
		case FLOAT:
		{
			float val = data->val.fval;
			buxton_debug("Success: value has been set: %f(float). ", val);
			break;
		}
		case DOUBLE:
		{
			double val = data->val.dval;
			buxton_debug("Success: value has been set: %e(double). ", val);
			break;
		}
		case BOOLEAN:
		{
			bool val = data->val.bval;
			buxton_debug("Success: value has been set: %i(bool). ", val);
			break;
		}
		default:
		{
			buxton_debug("Data type not found\n");
			break;
		}
	}
	BuxtonKey k = buxton_response_key(response);
	buxton_debug("Key: %s, Group: %s, Layer: %s.\n", buxton_key_get_name(k), buxton_key_get_group(k), buxton_key_get_layer(k));
	buxton_key_free(k);	
}

/* buxton_set_value callback for all buxton data types */
void _bs_cb(BuxtonResponse response, void *data){
	vstatus *ret = (vstatus*)data;
	ret->status = 0;
	/* check response before switch */
	if (buxton_response_status(response)) {
		buxton_debug("Failed to set value.\n");
		return;
	}
	ret->status = 1;
	_bs_print(ret, response);

}

/* buxton_get_value callback for all buxton data types */
void _bg_cb(BuxtonResponse response, void *data)
{
	char *type;
	vstatus *ret = (vstatus *)data;
	ret->status = 0;
	if (buxton_response_status(response)) {
		buxton_debug("Failed to get value. \n");
		return;
	}
	void *p = buxton_response_value(response);
	if (!p) {
		buxton_debug("Null response value.\n");
		return;
	}
	switch (ret->type) {
		case STRING:
		{
			ret->val.sval = *(char**)p;
			type = "string";
			break;
		}
		case INT32:
		{
			ret->val.i32val = *(int32_t*)p;
			type = "int32_t";
			break;
		}
		case UINT32:
		{
			ret->val.ui32val = *(uint32_t*)p;
			type = "uint32_t";
			break;
		}
		case INT64:
		{
			ret->val.i64val = *(int64_t*)p;
			type = "int64_t";
			break;
		}
		case UINT64:
		{
			ret->val.ui64val = *(uint64_t*)p;
			type = "uint64_t";
			break;
		}
		case FLOAT:
		{
			ret->val.fval = *(float*)p;
			type = "float";
			break;
		}
		case DOUBLE:
		{
			ret->val.dval = *(double*)p;
			type = "double";
			break;
		}
		case BOOLEAN:
		{
			ret->val.bval = *(bool*)p;
			type = "bool";
			break;
		}
		default:
		{
			type = "unknown";
			break;
		}
	}
	buxton_debug("Got %s value.\n", type);
	ret->status = 1;
}

/* create a client side group TODO: create BuxtonGroup type probably not really needed */
BuxtonKey _buxton_group_create(char *name, char *layer)
{
	BuxtonKey ret = buxton_key_create(name, NULL, layer, STRING);
	return ret;
}

/* buxton_remove_group callback and function */
void _rg_cb(BuxtonResponse response, void *data)
{
	int *ret = (int *)data;
	*ret = 0;
	if (buxton_response_status(response) != 0) {
		buxton_debug("Failed to remove group.\n");
	} else {
		*ret = 1;
		buxton_debug("Removed group.\n");
	}
}

/* Callback for buxton_register_notification */
void _rn_cb(BuxtonResponse response, void *data)
{
	nstatus *ret = (nstatus *)data;
	NotifyCallback cb; 
	BuxtonKey key = NULL;
	char *name = NULL;
	void *value = NULL;

	if (buxton_response_status(response) != 0) {
		buxton_debug("Notify failed\n");
		ret->status = -1;
		return;
	}

	key = buxton_response_key(response);
	name = buxton_key_get_name(key);
	value = buxton_response_value(response);

	buxton_debug("Calling client cb....\n");
	cb = (NotifyCallback)ret->callback;

	cb(value, name);
}

BuxtonKey _buxton_notify_create(char *layer, char *group, char *name)
{
	BuxtonKey key;
	BuxtonDataType type = UNKNOWN;
	char *stype;

	if (!group || !name) {
		return NULL;
	}

	if (!client) {
		return NULL;
	}

	key = buxton_key_create(group, name, layer, UNKNOWN);

	if (buxton_get_key_type(client, key, _gkt_cb, &type, true)) {
		buxton_debug("Get key type call failed\n");
		return NULL;
	}

	switch (type) {
		case BUXTON_TYPE_MIN:
		{
			stype = "invalid- still min";
		}
		case STRING:
		{
			stype = "string";
			break;
		}
		case INT32:
		{
			stype = "int32_t";
			break;
		}
		case UINT32:
		{
			stype = "uint32_t";
			break;
		}
		case INT64:
		{
			stype = "int64_t";
			break;
		}
		case UINT64:
		{
			stype = "uint64_t";
			break;
		}
		case FLOAT:
		{
			stype = "float";
			break;
		}
		case DOUBLE:
		{
			stype = "double";
			break;
		}
		case BOOLEAN:
		{
			stype = "bool";
			break;
		}
		default:
		{
			stype = "unknown";
			break;
		}
	}

	printf("type of key is: %d = %s\n", type, stype);

	if (type > BUXTON_TYPE_MIN && type < BUXTON_TYPE_MAX && type != UNKNOWN) {
		BuxtonKey k = buxton_key_create(group, name, layer, type);
		return k;
	} else {
		buxton_debug("Invalid type returned\n");
	}

	return NULL;
}

void _gkt_cb(BuxtonResponse response, void *data)
{
	if (data == NULL) {
		return;
	}

	BuxtonDataType *ret = (BuxtonDataType*) data;

	if (buxton_response_status(response) != 0) {
		
		buxton_debug("Failed to get type\n");
		return;
	} else {
		buxton_debug("Get successful, got type\n");
		void *p = buxton_response_value(response);
		*ret = *(BuxtonDataType*)p;
		return;
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


