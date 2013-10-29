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
 * \file protocol.h Internal header
 * This file is used internally by buxton to provide functionality
 * used for the wire protocol
 */
#pragma once

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <sys/socket.h>
#include <sys/poll.h>
#include "list.h"
#include "bt-daemon.h"
#include "serialize.h"

typedef struct client_list_item {
	LIST_FIELDS(struct client_list_item, item); /**<List type */
	int fd; /**<File descriptor of connected client */
	struct ucred cred; /**<Credentials of connected client */
	char *smack_label; /**<Smack label of connected client */
	uint8_t *data; /**<Data buffer for the client */
	size_t offset; /**<Current position to write to data buffer */
	size_t size; /**<Size of the data buffer */
} client_list_item;

/**
 * Buxton Status Codes
 */
typedef enum BuxtonStatus {
	BUXTON_STATUS_OK = 0, /**<Operation succeeded */
	BUXTON_STATUS_FAILED /**<Operation failed */
} BuxtonStatus;

typedef BuxtonData* (*daemon_value_func) (client_list_item *client,
					   BuxtonData *list,
					   int n_params,
					   BuxtonStatus *status);

/**
 * Global store of bt-daemon state
 */
typedef struct BuxtonDaemon {
	size_t nfds_alloc;
	size_t accepting_alloc;
	nfds_t nfds;
	bool *accepting;
	struct pollfd *pollfds;
	client_list_item *client_list;
	BuxtonClient buxton;
	daemon_value_func set_value;
	daemon_value_func get_value;
} BuxtonDaemon;

/**
 * Handle a message within bt-daemon
 * @param self Reference to BuxtonDaemon
 * @param client Current client
 * @param size Size of the data being handled
 */
void bt_daemon_handle_message(BuxtonDaemon *self, client_list_item *client, int size);

/**
 * Wait for and parse a response from bt-daemon
 * @param client A Buxton Client
 * @param msg Pointer to BuxtonControlMessage
 * @param list Pointer to empty BuxtonData list (NULL)
 * @return the number of parameters returned, or -1
 */
int buxton_wire_get_response(BuxtonClient *client, BuxtonControlMessage *msg,
			      BuxtonData **list);
/**
 * Send a SET message over the wire protocol, return the response
 * @param client Client connection
 * @param layer Layer name
 * @param key Key name
 * @param value A BuxtonData storing the new value
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_set_value(BuxtonClient *client, const char *layer_name, const char *key,
			   BuxtonData *value);
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
