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
 * used for the buxtond
 */
#pragma once

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <sys/poll.h>
#include <sys/socket.h>

#include "buxton.h"
#include "backend.h"
#include "hashmap.h"
#include "list.h"
#include "protocol.h"
#include "serialize.h"

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
 * Notification registration
 */
typedef struct BuxtonNotification {
	client_list_item *client; /**<Client */
	BuxtonData *old_data; /**<Old value of a particular key*/
	uint32_t msgid; /**<Message id from the client */
} BuxtonNotification;

/**
 * Global store of buxtond state
 */
typedef struct BuxtonDaemon {
	size_t nfds_alloc;
	size_t accepting_alloc;
	nfds_t nfds;
	bool *accepting;
	struct pollfd *pollfds;
	client_list_item *client_list;
	Hashmap *notify_mapping;
	BuxtonControl buxton;
} BuxtonDaemon;

/**
 * Take a BuxtonData array and set key, layer and value items
 * correctly
 * @param msg Control message specifying how to parse the array
 * @param count Number of elements in the array
 * @param list BuxtonData array to parse
 * @param key Pointer to pointer to set if key is used
 * @param value Pointer to pointer to set if value is used
 * @returns bool indicating success of parsing
 */
bool parse_list(BuxtonControlMessage msg, size_t count, BuxtonData *list,
		_BuxtonKey *key, BuxtonData **value)
	__attribute__((warn_unused_result));

/**
 * Handle a message within buxtond
 * @param self Reference to BuxtonDaemon
 * @param client Current client
 * @param size Size of the data being handled
 * @returns bool True if message was successfully handled
 */
bool buxtond_handle_message(BuxtonDaemon *self,
			      client_list_item *client,
			      size_t size)
	__attribute__((warn_unused_result));

/**
 * Notify clients a value changes in buxtond
 * @param self Refernece to BuxtonDaemon
 * @param client Current client
 * @param key Modified key
 * @param value Modified value
 */
void buxtond_notify_clients(BuxtonDaemon *self, client_list_item *client,
			      _BuxtonKey* key, BuxtonData *value);

/**
 * Buxton daemon function for setting a value
 * @param self buxtond instance being run
 * @param client Used to validate smack access
 * @param key Key for the value being set
 * @param value Value being set
 * @param status Will be set with the int32_t result of the operation
 */
void set_value(BuxtonDaemon *self, client_list_item *client,
	       _BuxtonKey *key, BuxtonData *value, int32_t *status);

/**
 * Buxton daemon function for setting a label
 * @param self buxtond instance being run
 * @param client Used to validate smack access
 * @param key Key or group for the label being set
 * @param value Label being set
 * @param status Will be set with the int32_t result of the operation
 */
void set_label(BuxtonDaemon *self, client_list_item *client,
	       _BuxtonKey *key, BuxtonData *value, int32_t *status);

/**
 * Buxton daemon function for creating a group
 * @param self buxtond instance being run
 * @param client Used to validate smack access
 * @param key Key with layer and group members initialized
 * @param status Will be set with the int32_t result of the operation
 */
void create_group(BuxtonDaemon *self, client_list_item *client,
		  _BuxtonKey *key, int32_t *status);

/**
 * Buxton daemon function for removing a group
 * @param self buxtond instance being run
 * @param client Used to validate smack access
 * @param key Key with layer and group members initialized
 * @param status Will be set with the int32_t result of the operation
 */
void remove_group(BuxtonDaemon *self, client_list_item *client,
		  _BuxtonKey *key, int32_t *status);

/**
 * Buxton daemon function for getting a value
 * @param self buxtond instance being run
 * @param client Used to validate smack access
 * @param key Key for the value being sought
 * @param status Will be set with the int32_t result of the operation
 * @returns BuxtonData Value stored for key if successful otherwise NULL
 */
BuxtonData *get_value(BuxtonDaemon *self, client_list_item *client,
		      _BuxtonKey *key, int32_t *status)
	__attribute__((warn_unused_result));

/**
 * Buxton daemon function for unsetting a value
 * @param self buxtond instance being run
 * @param client Used to validate smack access
 * @param key Key for the value being dunset
 * @param status Will be set with the int32_t result of the operation
 */
void unset_value(BuxtonDaemon *self, client_list_item *client,
		 _BuxtonKey *key, int32_t *status);

/**
 * Buxton daemon function for listing keys in a given layer
 * @param self buxtond instance being run
 * @param client Used to validate smack access
 * @param layer Layer to query
 * @param status Will be set with the int32_t result of the operation
 */
BuxtonArray *list_keys(BuxtonDaemon *self, client_list_item *client,
		       BuxtonString *layer, int32_t *status)
	__attribute__((warn_unused_result));

/**
 * Buxton daemon function for registering notifications on a given key
 * @param self buxtond instance being run
 * @param client Used to validate smack access
 * @param key Key to notify for changes on
 * @param msgid Message ID from the client
 * @param status Will be set with the int32_t result of the operation
 */
void register_notification(BuxtonDaemon *self, client_list_item *client,
			   _BuxtonKey *key, uint32_t msgid,
			   int32_t *status);

/**
 * Buxton daemon function for unregistering notifications from the given key
 * @param self buxtond instance being run
 * @param client Used to validate smack access
 * @param key Key to no longer recieve notifications for
 * @param status Will be set with the int32_t result of the operation
 * @return Message ID used to send key's notifications to the client
 */
uint32_t unregister_notification(BuxtonDaemon *self, client_list_item *client,
				 _BuxtonKey *key, int32_t *status)
	__attribute__((warn_unused_result));

/**
 * Verify credentials for the client socket
 * @param cl Client to check the credentials of
 * @return bool indicating credentials where found or not
 */
bool identify_client(client_list_item *cl)
	__attribute__((warn_unused_result));

/**
 * Add a fd to daemon's poll list
 * @param self buxtond instance being run
 * @param fd File descriptor to add to the poll list
 * @param events Priority mask for events
 * @param a Accepting status of the fd
 * @return None
 */
void add_pollfd(BuxtonDaemon *self, int fd, short events, bool a);

/**
 * Add a fd to daemon's poll list
 * @param self buxtond instance being run
 * @param i File descriptor to remove from poll list
 * @return None
 */
void del_pollfd(BuxtonDaemon *self, nfds_t i);

/**
 * Setup a client's smack label
 * @param cl Client to set smack label on
 * @return None
 */
void handle_smack_label(client_list_item *cl);

/**
 * Handle a client connection
 * @param self buxtond instance being run
 * @param cl The currently activate client
 * @param i The currently active file descriptor
 * @return bool indicating more data to process
 */
bool handle_client(BuxtonDaemon *self, client_list_item *cl, nfds_t i)
	__attribute__((warn_unused_result));

/**
 * Terminate client connectoin
 * @param self buxtond instance being run
 * @param cl The client to terminate
 * @param i File descriptor to remove from poll list
 */
void terminate_client(BuxtonDaemon *self, client_list_item *cl, nfds_t i);

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
