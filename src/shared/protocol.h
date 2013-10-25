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

typedef struct client_list_item {
	LIST_FIELDS(struct client_list_item, item); /**<List type */
	int fd; /**<File descriptor of connected client */
	struct ucred cred; /**<Credentials of connected client */
	char *smack_label; /**<Smack label of connected client */
	char data[256]; /**<Data buffer for the client */
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
 * @param client Current client
 */
void bt_daemon_handle_message(BuxtonDaemon *self, client_list_item *client);

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
