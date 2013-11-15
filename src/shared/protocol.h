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

#include <sys/poll.h>
#include <sys/socket.h>

#include "bt-daemon.h"
#include "list.h"
#include "serialize.h"
#include "smack.h"
#include "hashmap.h"

/**
 * List for daemon's clients
 */
typedef struct client_list_item {
	LIST_FIELDS(struct client_list_item, item); /**<List type */
	int fd; /**<File descriptor of connected client */
	struct ucred cred; /**<Credentials of connected client */
	BuxtonString *smack_label; /**<Smack label of connected client */
	uint8_t *data; /**<Data buffer for the client */
	size_t offset; /**<Current position to write to data buffer */
	size_t size; /**<Size of the data buffer */
} client_list_item;

/**
 * List of clients interested in a key
 */
typedef struct notification_list_item {
	LIST_FIELDS(struct notification_list_item, item); /**<List type */
	client_list_item *client; /**<Client */
	BuxtonData *old_data; /**<Old value of a particular key*/
} notification_list_item;

/**
 * Buxton Status Codes
 */
typedef enum BuxtonStatus {
	BUXTON_STATUS_OK = 0, /**<Operation succeeded */
	BUXTON_STATUS_FAILED /**<Operation failed */
} BuxtonStatus;

typedef struct BuxtonDaemon BuxtonDaemon;

/**
 * Prototype for get and set value functions
 */
typedef BuxtonData* (*daemon_value_func) (struct BuxtonDaemon *self,
					  client_list_item *client,
					  BuxtonData *list,
					  size_t n_params,
					  BuxtonStatus *status);

/**
 * Global store of bt-daemon state
 */
struct BuxtonDaemon {
	size_t nfds_alloc;
	size_t accepting_alloc;
	nfds_t nfds;
	bool *accepting;
	struct pollfd *pollfds;
	client_list_item *client_list;
	Hashmap *notify_mapping;
	BuxtonClient buxton;
	daemon_value_func set_value;
	daemon_value_func get_value;
	daemon_value_func register_notification;
};

/**
 * Wait for and parse a response from bt-daemon
 * @param client A Buxton Client
 * @param msg Pointer to BuxtonControlMessage
 * @param list Pointer to empty BuxtonData list (NULL)
 * @return the number of parameters returned, or 0
 */
size_t buxton_wire_get_response(BuxtonClient *client, BuxtonControlMessage *msg,
			      BuxtonData **list);
/**
 * Send a SET message over the wire protocol, return the response
 * @param client Client connection
 * @param layer Layer name
 * @param key Key name
 * @param value A BuxtonData storing the new value
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_set_value(BuxtonClient *client, BuxtonString *layer_name, BuxtonString *key,
			   BuxtonData *value);

/**
 * Send a GET message over the wire protocol, return the data
 * @param client Client connection
 * @param layer Layer name (optional)
 * @param key Key name
 * @param value A pointer to store retrieved value in
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_get_value(BuxtonClient *client, BuxtonString *layer_name, BuxtonString *key,
			   BuxtonData *value);

/**
 * Send a NOTIFY message over the protocol, register for events
 * @param client Client connection
 * @param key Key name
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_register_notification(BuxtonClient *client, BuxtonString *key);

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
