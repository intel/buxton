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
 * \file lbuxton.c Buxton library implementation
 */
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>

#include "buxton.h"
#include "buxtonclient.h"
#include "buxtonkey.h"
#include "buxtonresponse.h"
#include "buxtonstring.h"
#include "configurator.h"
#include "hashmap.h"
#include "log.h"
#include "protocol.h"
#include "util.h"


bool buxton_client_set_conf_file(char *path)
{
	int r;
	struct stat st;

	r = stat(path, &st);
	if (r == -1)
		return false;
	else
		if (st.st_mode & S_IFDIR)
			return false;

	buxton_add_cmd_line(CONFIG_CONF_FILE, path);

	return true;
}

int buxton_client_open(BuxtonClient *client)
{
	_BuxtonClient **c = (_BuxtonClient **)client;
	int bx_socket, r;
	struct sockaddr_un remote;

	if ((bx_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, buxton_socket(), sizeof(remote.sun_path));
	r = connect(bx_socket, (struct sockaddr *)&remote, sizeof(remote));
	if ( r == -1) {
		close(bx_socket);
		return -1;
	}

	if (fcntl(bx_socket, F_SETFL, O_NONBLOCK)) {
		close(bx_socket);
		return -1;
	}

	if (!setup_callbacks()) {
		close(bx_socket);
		return -1;
	}

	*c = malloc0(sizeof(_BuxtonClient *));
	if (!(*c)) {
		close(bx_socket);
		return -1;
	}

	(*c)->fd = bx_socket;
	return bx_socket;
}

void buxton_client_close(BuxtonClient client)
{
	_BuxtonClient *c;
	if (!client)
		return;

	c = (_BuxtonClient *)client;

	cleanup_callbacks();
	close(c->fd);
	c->direct = 0;
	c->fd = -1;
}

bool buxton_client_get_value(BuxtonClient client,
			     BuxtonKey key,
			     BuxtonCallback callback,
			     void *data,
			     bool sync)
{
	bool r;
	_BuxtonKey *k = (_BuxtonKey *)key;

	if (!k || !(k->group.value) || !(k->name.value) ||
	    k->type <= BUXTON_TYPE_MIN || k->type >= BUXTON_TYPE_MAX)
		return false;

	r = buxton_wire_get_value((_BuxtonClient *)client, k, callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response(client);

	return r;
}

bool buxton_client_register_notification(BuxtonClient client,
					 BuxtonKey key,
					 BuxtonCallback callback,
					 void *data,
					 bool sync)
{
	bool r;
	_BuxtonKey *k = (_BuxtonKey *)key;

	if (!k || !k->group.value || !k->name.value ||
	    k->type <= BUXTON_TYPE_MIN || k->type >= BUXTON_TYPE_MAX)
		return false;

	r = buxton_wire_register_notification((_BuxtonClient *)client, k,
					      callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response((_BuxtonClient *)client);

	return r;
}

bool buxton_client_unregister_notification(BuxtonClient client,
					   BuxtonKey key,
					   BuxtonCallback callback,
					   void *data,
					   bool sync)
{
	bool r;
	_BuxtonKey *k = (_BuxtonKey *)key;

	if (!k || !k->group.value || !k->name.value ||
	    k->type <= BUXTON_TYPE_MIN || k->type >= BUXTON_TYPE_MAX)
		return false;

	r = buxton_wire_unregister_notification((_BuxtonClient *)client, k,
						callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response((_BuxtonClient *)client);

	return r;
}

bool buxton_client_set_value(BuxtonClient client,
			     BuxtonKey key,
			     void *value,
			     BuxtonCallback callback,
			     void *data,
			     bool sync)
{
	bool r;
	_BuxtonKey *k = (_BuxtonKey *)key;

	if (!k || !k->group.value || !k->name.value || !k->layer.value ||
	    k->type <= BUXTON_TYPE_MIN || k->type >= BUXTON_TYPE_MAX || !value)
		return false;

	r = buxton_wire_set_value((_BuxtonClient *)client, k, value, callback,
				  data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response((_BuxtonClient *)client);

	return r;
}

bool buxton_client_set_label(BuxtonClient client,
			     BuxtonKey key,
			     char *value,
			     BuxtonCallback callback,
			     void *data,
			     bool sync)
{
	bool r;
	BuxtonString v;
	_BuxtonKey *k = (_BuxtonKey *)key;

	if (!k || !k->group.value || !k->layer.value || !value ||
	    k->type <= BUXTON_TYPE_MIN || k->type >= BUXTON_TYPE_MAX)
		return false;

	v = buxton_string_pack(value);

	r = buxton_wire_set_label((_BuxtonClient *)client, k, &v, callback,
				  data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response(client);

	return r;
}

bool buxton_client_list_keys(BuxtonClient client,
			     char *layer_name,
			     BuxtonCallback callback,
			     void *data,
			     bool sync)
{
	bool r;
	BuxtonString l;

	if (!layer_name)
		return false;

	l = buxton_string_pack(layer_name);

	r = buxton_wire_list_keys((_BuxtonClient *)client, &l, callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response((_BuxtonClient *)client);

	return r;
}

bool buxton_client_unset_value(BuxtonClient client,
			       BuxtonKey key,
			       BuxtonCallback callback,
			       void *data,
			       bool sync)
{
	bool r;
	_BuxtonKey *k = (_BuxtonKey *)key;

	if (!k || !k->group.value || !k->name.value || !k->layer.value ||
	    k->type <= BUXTON_TYPE_MIN || k->type >= BUXTON_TYPE_MAX)
		return false;

	r = buxton_wire_unset_value((_BuxtonClient *)client, k, callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response((_BuxtonClient *)client);

	return r;
}

BuxtonKey buxton_make_key(char *group, char *name, char *layer,
			  BuxtonDataType type)
{
	_BuxtonKey *key = NULL;
	char *g = NULL;
	char *n = NULL;
	char *l = NULL;

	if (!group)
		goto fail;

	if (type <= BUXTON_TYPE_MIN || type >= BUXTON_TYPE_MAX)
		goto fail;

	g = strdup(group);
	if (!g)
		goto fail;

	if (name) {
		n = strdup(name);
		if (!n)
			goto fail;
	}

	if (layer) {
		l = strdup(layer);
		if (!l)
			goto fail;
	}

	key = malloc0(sizeof(_BuxtonKey));
	if (!key)
		goto fail;

	key->group.value = g;
	key->group.length = (uint32_t)strlen(g) + 1;
	if (name) {
		key->name.value = n;
		key->name.length = (uint32_t)strlen(n) + 1;
	} else {
		key->name.value = NULL;
		key->name.length = 0;
	}
	if (layer) {
		key->layer.value = l;
		key->layer.length = (uint32_t)strlen(l) + 1;
	} else {
		key->layer.value = NULL;
		key->layer.length = 0;
	}
	key->type = type;

	return (BuxtonKey)key;

fail:
	free(g);
	free(n);
	free(l);
	return NULL;
}

char *buxton_get_group(BuxtonKey key)
{
	_BuxtonKey *k = (_BuxtonKey *)key;

	if (!key)
		return NULL;

	return get_group(k);
}

char *buxton_get_name(BuxtonKey key)
{
	_BuxtonKey *k = (_BuxtonKey *)key;

	if (!key)
		return NULL;

	return get_name(k);
}

char *buxton_get_layer(BuxtonKey key)
{
	_BuxtonKey *k = (_BuxtonKey *)key;

	if (!key)
		return NULL;

	return get_layer(k);
}

BuxtonDataType buxton_get_type(BuxtonKey key)
{
	_BuxtonKey *k = (_BuxtonKey *)key;

	if (!key)
		return -1;

	return k->type;
}

void buxton_free_key(BuxtonKey key)
{
	_BuxtonKey *k = (_BuxtonKey *)key;

	if (!k)
		return;

	free(k->group.value);
	free(k->name.value);
	free(k->layer.value);
	free(k);
}

ssize_t buxton_client_handle_response(BuxtonClient client)
{
	return buxton_wire_handle_response((_BuxtonClient *)client);
}

BuxtonControlMessage response_type(BuxtonResponse response)
{
	_BuxtonResponse *r = (_BuxtonResponse *)response;

	if (!response)
		return -1;

	return r->type;
}

BuxtonStatus response_status(BuxtonResponse response)
{
	BuxtonData *d;
	_BuxtonResponse *r = (_BuxtonResponse *)response;

	if (!response)
		return -1;

	if (response_type(response) == BUXTON_CONTROL_CHANGED)
		return BUXTON_STATUS_OK;

	d = buxton_array_get(r->data, 0);

	if (d)
		return d->store.d_int32;

	return -1;
}

BuxtonKey response_key(BuxtonResponse response)
{
	_BuxtonKey *key = NULL;
	_BuxtonResponse *r = (_BuxtonResponse *)response;

	if (!response)
		return NULL;

	if (response_type(response) == BUXTON_CONTROL_LIST)
		return NULL;

	key = malloc0(sizeof(_BuxtonKey));
	if (!key)
		return NULL;

	if (!buxton_key_copy(r->key, key))
		return NULL;

	return (BuxtonKey)key;
}

void *response_value(BuxtonResponse response)
{
	void *p = NULL;
	BuxtonData *d;
	_BuxtonResponse *r = (_BuxtonResponse *)response;
	BuxtonControlMessage type;

	if (!response)
		return NULL;

	type = response_type(response);
	if (type == BUXTON_CONTROL_GET)
		d = buxton_array_get(r->data, 1);
	else if (type == BUXTON_CONTROL_CHANGED)
		d = buxton_array_get(r->data, 0);
	else
		goto out;

	if (!d)
		goto out;

	switch (d->type) {
	case STRING:
		return strdup(d->store.d_string.value);
	case INT32:
		p = malloc0(sizeof(int32_t));
		if (!p)
			goto out;
		*(int32_t *)p = (int32_t)d->store.d_int32;
		break;
	case UINT32:
		p = malloc0(sizeof(uint32_t));
		if (!p)
			goto out;
		*(uint32_t *)p = (uint32_t)d->store.d_uint32;
		break;
	case INT64:
		p = malloc0(sizeof(int64_t));
		if (!p)
			goto out;
		*(int64_t *)p = (int64_t)d->store.d_int64;
		break;
	case UINT64:
		p = malloc0(sizeof(uint64_t));
		if (!p)
			goto out;
		*(uint64_t *)p = (uint64_t)d->store.d_uint64;
		break;
	case FLOAT:
		p = malloc0(sizeof(float));
		if (!p)
			goto out;
		*(float *)p = (float)d->store.d_float;
		break;
	case DOUBLE:
		p = malloc0(sizeof(double));
		if (!p)
			goto out;
		*(double *)p = (double)d->store.d_double;
		break;
	case BOOLEAN:
		p = malloc0(sizeof(bool));
		if (!p)
			goto out;
		*(bool *)p = (bool)d->store.d_boolean;
		break;
	default:
		break;
	}

out:
	return p;
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
