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

#include "util.h"
#include "buxton.h"
#include "log.h"
#include "hashmap.h"
#include "protocol.h"
#include "configurator.h"

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

bool buxton_client_open(BuxtonClient *client)
{
	int bx_socket, r;
	struct sockaddr_un remote;

	if (!client)
		return false;

	if ((bx_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return false;

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, buxton_socket(), sizeof(remote.sun_path));
	r = connect(bx_socket, (struct sockaddr *)&remote, sizeof(remote));
	if ( r == -1) {
		close(bx_socket);
		return false;
	}

	if (fcntl(bx_socket, F_SETFL, O_NONBLOCK)) {
		close(bx_socket);
		return false;
	}

	if (!setup_callbacks()) {
		close(bx_socket);
		return false;
	}

	client->fd = bx_socket;
	return true;
}

void buxton_client_close(BuxtonClient *client)
{
	if (!client)
		return;

	cleanup_callbacks();
	close(client->fd);
	client->direct = 0;
	client->fd = -1;
}

bool buxton_client_get_value(BuxtonClient *client,
			     BuxtonString *key,
			     BuxtonCallback callback,
			     void *data,
			     bool sync)
{
	bool r;

	r = buxton_wire_get_value(client, NULL, key, callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response(client);

	return r;
}

bool buxton_client_get_value_for_layer(BuxtonClient *client,
				       BuxtonString *layer_name,
				       BuxtonString *key,
				       BuxtonCallback callback,
				       void *data,
				       bool sync)
{
	bool r;

	r = buxton_wire_get_value(client, layer_name, key, callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response(client);

	return r;
}

bool buxton_client_register_notification(BuxtonClient *client,
					 BuxtonString *key,
					 BuxtonCallback callback,
					 void * data,
					 bool sync)
{
	return buxton_wire_register_notification(client, key, callback, data);
}

bool buxton_client_unregister_notification(BuxtonClient *client,
					   BuxtonString *key,
					   BuxtonCallback callback,
					   void *data,
					   bool sync)
{
	bool r;

	r = buxton_wire_unregister_notification(client, key, callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response(client);

	return r;
}

bool buxton_client_set_value(BuxtonClient *client,
			     BuxtonString *layer_name,
			     BuxtonString *key,
			     BuxtonData *value,
			     BuxtonCallback callback,
			     void *data,
			     bool sync)
{
	bool r;

	r = buxton_wire_set_value(client, layer_name, key, value, callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response(client);

	return r;
}

bool buxton_client_set_label(BuxtonClient *client,
			     BuxtonString *layer_name,
			     BuxtonString *key,
			     BuxtonData *value,
			     BuxtonCallback callback,
			     void *data,
			     bool sync)
{
	bool r;

	r = buxton_wire_set_label(client, layer_name, key, value, callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response(client);

	return r;
}

bool buxton_client_list_keys(BuxtonClient *client,
			     BuxtonString *layer_name,
			     BuxtonCallback callback,
			     void *data,
			     bool sync)
{
	bool r;

	r = buxton_wire_list_keys(client, layer_name, callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response(client);

	return r;
}

bool buxton_client_unset_value(BuxtonClient *client,
			       BuxtonString *layer_name,
			       BuxtonString *key,
			       BuxtonCallback callback,
			       void *data,
			       bool sync)
{
	bool r;

	r = buxton_wire_unset_value(client, layer_name, key, callback, data);
	if (!r)
		return false;

	if (sync)
		r = buxton_wire_get_response(client);

	return r;
}

BuxtonString *buxton_make_key(char *group, char *name)
{
	BuxtonString *key;
	int len;

	if (!group)
		return NULL;

	key = malloc0(sizeof(BuxtonString));
	if (!key)
		return NULL;

	if (!name) {
		key->value = strdup(group);
		if (!key->value) {
			free(key);
			return NULL;
		}
		key->length = (uint32_t)strlen(key->value) + 1;
		return key;
	}

	len = asprintf(&(key->value), "%s%c%s", group, 0, name);
	if (len < 0) {
		free(key);
		return NULL;
	}
	key->length = (uint32_t)len + 1;

	return key;
}

char *buxton_get_group(BuxtonString *key)
{
	return get_group(key);
}

char *buxton_get_name(BuxtonString *key)
{
	return get_name(key);
}

ssize_t buxton_client_handle_response(BuxtonClient *client)
{
	return buxton_wire_handle_response(client);
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
