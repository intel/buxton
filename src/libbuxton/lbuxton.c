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
#include "bt-daemon.h"
#include "log.h"
#include "hashmap.h"
#include "protocol.h"

bool buxton_client_open(BuxtonClient *client)
{
	int bx_socket, r;
	struct sockaddr_un remote;

	if (!client)
		return false;

	if ((bx_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		return false;
	}

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, BUXTON_SOCKET, sizeof(remote.sun_path));
	r = connect(bx_socket, (struct sockaddr *)&remote, sizeof(remote));
	client->fd = bx_socket;
	if ( r == -1) {
		close(client->fd);
		return false;
	}

	return true;
}

void buxton_client_close(BuxtonClient *client)
{
	if (!client)
		return;

	close(client->fd);
	client->direct = 0;
	client->fd = -1;
}

bool buxton_client_get_value(BuxtonClient *client,
			      BuxtonString *key,
			      BuxtonData *data)
{
	return buxton_wire_get_value(client, NULL, key, data);
}

bool buxton_client_get_value_for_layer(BuxtonClient *client,
			      BuxtonString *layer_name,
			      BuxtonString *key,
			      BuxtonData *data)
{
	return buxton_wire_get_value(client, layer_name, key, data);
}

bool buxton_client_register_notification(BuxtonClient *client, BuxtonString *key)
{
	return buxton_wire_register_notification(client, key);
}

bool buxton_client_unregister_notification(BuxtonClient *client, BuxtonString *key)
{
	return buxton_wire_unregister_notification(client, key);
}

bool buxton_client_set_value(BuxtonClient *client,
			      BuxtonString *layer_name,
			      BuxtonString *key,
			      BuxtonData *data)
{
	return buxton_wire_set_value(client, layer_name, key, data);
}

bool buxton_client_list_keys(BuxtonClient *client,
			     BuxtonString *layer_name,
			     BuxtonArray **list)
{
	return buxton_wire_list_keys(client, layer_name, list);
}

bool buxton_client_unset_value(BuxtonClient *client,
			       BuxtonString *layer_name,
			       BuxtonString *key)
{
	return buxton_wire_unset_value(client, layer_name, key);
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
