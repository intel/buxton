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
 * \file daemon.h Internal header
 * This file is used internally by buxton to provide functionality
 * used for the bt-daemon
 */
#pragma once

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include "bt-daemon.h"
#include "protocol.h"

/**
 * Handle a message within bt-daemon
 * @param self Reference to BuxtonDaemon
 * @param client Current client
 * @param size Size of the data being handled
 */
void bt_daemon_handle_message(BuxtonDaemon *self, client_list_item *client, size_t size);

/**
 * Notify clients a value changes in bt-daemon
 * @param self Refernece to BuxtonDaemon
 * @param client Current client
 * @param data Modified data
 */
void bt_daemon_notify_clients(BuxtonDaemon *self, client_list_item *client, BuxtonData* data);

/**
 * Buxton daemon function for setting a value
 * @param self bt-daemon instance being run
 * @param client Used to validate smack access
 * @param list Contains layer, key and value data being used
 * @param n_params Number of items contained in the list
 * @param status Will be set with the BuxtonStatus result of the operation
 * @returns BuxtonData Always NULL currently, may be changed in the future
 */
BuxtonData *set_value(BuxtonDaemon *self, client_list_item *client, BuxtonData *list,
			     size_t n_params, BuxtonStatus *status);

/**
 * Buxton daemon function for getting a value
 * @param self bt-daemon instance being run
 * @param client Used to validate smack access
 * @param list Contains key and possibly layer data being used
 * @param n_params Number of items contained in the list
 * @param status Will be set with the BuxtonStatus result of the operation
 * @returns BuxtonData Value stored for key if successful otherwise NULL
 */
BuxtonData *get_value(BuxtonDaemon *self, client_list_item *client, BuxtonData *list,
			     size_t n_params, BuxtonStatus *status);

/**
 * Buxton daemon function for registering notifications on a given key
 * @param self bt-daemon instance being run
 * @param client Used to validate smack access
 * @param list Contains key being used
 * @param n_params Number of items contained in the list
 * @param status Will be set with the BuxtonStatus result of the operation
 * @returns BuxtonData Always NULL currently, may be changed in the future
 */
BuxtonData *register_notification(BuxtonDaemon *self, client_list_item *client, BuxtonData *list,
			     int n_params, BuxtonStatus *status);

/**
 * Verify credentials for the client socket
 * @param cl Client to check the credentials of
 * @return bool indicating credentials where found or not
 */
bool identify_client(client_list_item *cl);

/**
 * Add a fd to daemon's poll list
 * @param self bt-daemon instance being run
 * @param fd File descriptor to add to the poll list
 * @param events Priority mask for events
 * @param a Accepting status of the fd
 * @return None
 */
void add_pollfd(BuxtonDaemon *self, int fd, short events, bool a);

/**
 * Add a fd to daemon's poll list
 * @param self bt-daemon instance being run
 * @param i File descriptor to remove from poll list
 * @return None
 */
void del_pollfd(BuxtonDaemon *self, nfds_t i);

/**
 * Handle a client connection
 * @param self bt-daemon instance being run
 * @param cl The currently activate client
 * @param i The currently active file descriptor
 */
void handle_client(BuxtonDaemon *self, client_list_item *cl, nfds_t i);

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
