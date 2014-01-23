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

#include "buxton.h"
#include "list.h"
#include "serialize.h"
#include "hashmap.h"

/**
 * Buxton Status Codes
 */
typedef enum BuxtonStatus {
	BUXTON_STATUS_OK = 0, /**<Operation succeeded */
	BUXTON_STATUS_FAILED /**<Operation failed */
} BuxtonStatus;

/**
 * Initialize callback hashamps
 * @return a boolean value, indicating success of the operation
 */
bool setup_callbacks(void)
	__attribute__((warn_unused_result));

/**
 * free callback hashmaps
 */
void cleanup_callbacks(void);

/**
 * Execute callback function on list using user data
 * @param callback User callback function executed
 * @param data User data passed to callback function
 * @param count number of elements in list
 * @param list Data from bt-daemon
 */
void run_callback(BuxtonCallback callback, void *data, size_t count, BuxtonData *list);

/**
 * Write message to bt-daemon
 * @param client Client connection
 * @param send serialized data to send to bt-daemon
 * @param send_len size of send
 * @param callback Callback function used to handle bt-daemon's response
 * @param data User data passed to callback function
 * @param msgid Message id used to identify bt-daemon's response
 * @param type The type of request being sent to bt-daemon
 * @return a boolean value, indicating success of the operation
 */
bool send_message(BuxtonClient *client, uint8_t *send, size_t send_len,
		  BuxtonCallback callback, void *data, uint64_t msgid,
		  BuxtonControlMessage type)
	__attribute__((warn_unused_result));

/**
 * Parse responses from bt-daemon and run callbacks on received messages
 * @param client A BuxtonClient
 * @return number of received messages processed
 */
size_t buxton_wire_handle_response(BuxtonClient *client)
	__attribute__((warn_unused_result));

/**
 * Wait for a response from bt-daemon and then call handle response
 * @param client Client connection
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_get_response(BuxtonClient *client);

/**
 * Send a SET message over the wire protocol, return the response
 * @param client Client connection
 * @param layer_name Layer name
 * @param key Key name
 * @param value A BuxtonData storing the new value
 * @param data A BuxtonData storing the new value
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_set_value(BuxtonClient *client, BuxtonString *layer_name,
			   BuxtonString *key, BuxtonData *value,
			   BuxtonCallback callback, void *data)
	__attribute__((warn_unused_result));

/**
 * Send a SET_LABEL message over the wire protocol, return the response
 *
 * @note This is a privileged operation, so it will return false for unprivileged clients
 *
 * @param client Client connection
 * @param layer_name Layer name
 * @param key Key or group name
 * @param value A BuxtonData storing the key or group label
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_set_label(BuxtonClient *client, BuxtonString *layer_name,
			   BuxtonString *key, BuxtonData *value,
			   BuxtonCallback callback, void *data)
	__attribute__((warn_unused_result));

/**
 * Send a GET message over the wire protocol, return the data
 * @param client Client connection
 * @param layer_name Layer name (optional)
 * @param key Key name
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback functionb
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_get_value(BuxtonClient *client, BuxtonString *layer_name,
			   BuxtonString *key, BuxtonCallback callback,
			   void *data)
	__attribute__((warn_unused_result));


/**
 * Send an UNSET message over the wire protocol, return the response
 * @param client Client connection
 * @param layer_name Layer name
 * @param key Key name
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_unset_value(BuxtonClient *client,
			     BuxtonString *layer_name,
			     BuxtonString *key,
			     BuxtonCallback callback,
			     void *data)
	__attribute__((warn_unused_result));
/**
 * Send a NOTIFY message over the protocol, register for events
 * @param client Client connection
 * @param key Key name
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_register_notification(BuxtonClient *client,
				       BuxtonString *key,
				       BuxtonCallback callback,
				       void *data)
	__attribute__((warn_unused_result));

/**
 * Send a LIST message over the protocol, retrieve key list
 * @param client Client connection
 * @param layer Layer name
 * @param array An empty array pointer to store results
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_list_keys(BuxtonClient *client,
			   BuxtonString *layer,
			   BuxtonCallback callback,
			   void *data)
	__attribute__((warn_unused_result));

/**
 * Send an UNNOTIFY message over the protocol, no longer recieve events
 * @param client Client connection
 * @param key Key name
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_unregister_notification(BuxtonClient *client,
				         BuxtonString *key,
					 BuxtonCallback callback,
					 void *data)
	__attribute__((warn_unused_result));

void include_protocol(void);

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
