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

#include "bt-daemon.h"
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
 * Wait for and parse a response from bt-daemon
 * @param client A Buxton Client
 * @param msg Pointer to BuxtonControlMessage
 * @param list Pointer to empty BuxtonData list (NULL)
 * @return the number of parameters returned, or 0
 */
size_t buxton_wire_get_response(BuxtonClient *client,
				BuxtonControlMessage *msg,
				BuxtonData **list)
	__attribute__((warn_unused_result));
/**
 * Send a SET message over the wire protocol, return the response
 * @param client Client connection
 * @param layer_name Layer name
 * @param key Key name
 * @param value A BuxtonData storing the new value
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_set_value(BuxtonClient *client, BuxtonString *layer_name,
			   BuxtonString *key, BuxtonData *value)
	__attribute__((warn_unused_result));

/**
 * Send a GET message over the wire protocol, return the data
 * @param client Client connection
 * @param layer_name Layer name (optional)
 * @param key Key name
 * @param value A pointer to store retrieved value in
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_get_value(BuxtonClient *client, BuxtonString *layer_name,
			   BuxtonString *key, BuxtonData *value)
	__attribute__((warn_unused_result));


/**
 * Send an UNSET message over the wire protocol, return the response
 * @param client Client connection
 * @param layer_name Layer name
 * @param key Key name
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_unset_value(BuxtonClient *client,
			     BuxtonString *layer_name,
			     BuxtonString *key)
	__attribute__((warn_unused_result));
/**
 * Send a NOTIFY message over the protocol, register for events
 * @param client Client connection
 * @param key Key name
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_register_notification(BuxtonClient *client,
				       BuxtonString *key)
	__attribute__((warn_unused_result));

/**
 * Send a LIST message over the protocol, retrieve key list
 * @param client Client connection
 * @param layer Layer name
 * @param array An empty array pointer to store results
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_list_keys(BuxtonClient *client,
			   BuxtonString *layer,
			   BuxtonArray **array)
	__attribute__((warn_unused_result));

/**
 * Send an UNNOTIFY message over the protocol, no longer recieve events
 * @param client Client connection
 * @param key Key name
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_unregister_notification(BuxtonClient *client,
				         BuxtonString *key)
	__attribute__((warn_unused_result));

/**
 * Check for notifications on the wire
 * @param client Client connection
 * @param array An empty array pointer to store results
 * @return a boolean value, indicating success of the operation
 */
bool buxton_wire_get_notifications(BuxtonClient *client,
				   BuxtonArray **array)
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
