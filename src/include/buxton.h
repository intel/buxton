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
 * \file buxton.h Buxton public header
 *
 * This is the public part of libbuxton
 *
 * \mainpage Buxton
 * \link buxton.h Public API
 * \endlink - API listing for libbuxton
 * \copyright Copyright (C) 2013 Intel corporation
 * \par License
 * GNU Lesser General Public License 2.1
 */

#pragma once

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>

#if (__GNUC__ >= 4)
/* Export symbols */
#    define _bx_export_ __attribute__ ((visibility("default")))
#else
#  define _bx_export_
#endif

/**
 * Possible data types for use in Buxton
 */
typedef enum BuxtonDataType {
	BUXTON_TYPE_MIN,
	STRING, /**<Represents type of a string value */
	INT32, /**<Represents type of an int32_t value */
	UINT32, /**<Represents type of an uint32_t value */
	INT64, /**<Represents type of a int64_t value */
	UINT64, /**<Represents type of a uint64_t value */
	FLOAT, /**<Represents type of a float value */
	DOUBLE, /**<Represents type of a double value */
	BOOLEAN, /**<Represents type of a boolean value */
	BUXTON_TYPE_MAX
} BuxtonDataType;

/**
 * Buxton message types
 */
typedef enum BuxtonControlMessage {
	BUXTON_CONTROL_MIN,
	BUXTON_CONTROL_SET, /**<Set a value within Buxton */
	BUXTON_CONTROL_SET_LABEL, /**<Set a label within Buxton */
	BUXTON_CONTROL_CREATE_GROUP, /**<Create a group within Buxton */
	BUXTON_CONTROL_REMOVE_GROUP, /**<Remove a group within Buxton */
	BUXTON_CONTROL_GET, /**<Retrieve a value from Buxton */
	BUXTON_CONTROL_UNSET, /**<Unset a value within Buxton */
	BUXTON_CONTROL_LIST, /**<List keys within a Buxton layer */
	BUXTON_CONTROL_STATUS, /**<Status code follows */
	BUXTON_CONTROL_NOTIFY, /**<Register for notification */
	BUXTON_CONTROL_UNNOTIFY, /**<Opt out of notifications */
	BUXTON_CONTROL_CHANGED, /**<A key changed in Buxton */
	BUXTON_CONTROL_MAX
} BuxtonControlMessage;

/**
 * Used to communicate with Buxton
 */
typedef struct BuxtonClient *BuxtonClient;

/**
 * Represents a data key in Buxton
 */
typedef struct BuxtonKey *BuxtonKey;

/**
 * Represents daemon's reply to client
 */
typedef struct BuxtonResponse *BuxtonResponse;

/**
 * Prototype for callback functions
 *
 * Takes a BuxtonResponse and returns void.
 */
typedef void (*BuxtonCallback)(BuxtonResponse, void *);

/* Buxton API Methods */

/**
 * Sets path to buxton configuration file
 * @param path Path to the buxton configuration file to use
 * @return An int with 0 indicating success or an errno value
 */
_bx_export_ int buxton_set_conf_file(char *path);

/**
 * Open a connection to Buxton
 * @param client A BuxtonClient pointer
 * @return A boolean value, indicating success of the operation
 */
_bx_export_ int buxton_open(BuxtonClient *client)
	__attribute__((warn_unused_result));

/**
 * Close the connection to Buxton
 * @param client A BuxtonClient
 */
_bx_export_ void buxton_close(BuxtonClient client);

/**
 * Set a value within Buxton
 * @param client An open client connection
 * @param layer The layer to manipulate
 * @param key The key to set
 * @param value A pointer to a supported data type
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @param sync Indicator for running a synchronous request
 * @return A int value, indicating success of the operation
 */
_bx_export_ int buxton_set_value(BuxtonClient client,
				 BuxtonKey key,
				 void *value,
				 BuxtonCallback callback,
				 void *data,
				 bool sync)
	__attribute__((warn_unused_result));

/**
 * Set a label within Buxton
 *
 * @note This is a privileged operation; the return value will be false for unprivileged clients.
 *
 * @param client An open client connection
 * @param layer The layer to manipulate
 * @param key The key or group name
 * @param value A struct containing the label to set
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @param sync Indicator for running a synchronous request
 * @return An int value, indicating success of the operation
 */
_bx_export_ int buxton_set_label(BuxtonClient client,
				 BuxtonKey key,
				 char *value,
				 BuxtonCallback callback,
				 void *data,
				 bool sync)
	__attribute__((warn_unused_result));

/**
 * Create a group within Buxton
 *
 * @param client An open client connection
 * @param key A BuxtonKey with only layer and group names initialized
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @param sync Indicator for running a synchronous request
 * @return An int value, indicating success of the operation
 */
_bx_export_ int buxton_create_group(BuxtonClient client,
				    BuxtonKey key,
				    BuxtonCallback callback,
				    void *data,
				    bool sync)
	__attribute__((warn_unused_result));

/**
 * Remove a group within Buxton
 *
 * @param client An open client connection
 * @param key A BuxtonKey with only layer and group names initialized
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @param sync Indicator for running a synchronous request
 * @return An int value, indicating success of the operation
 */
_bx_export_ int buxton_remove_group(BuxtonClient client,
				    BuxtonKey key,
				    BuxtonCallback callback,
				    void *data,
				    bool sync)
	__attribute__((warn_unused_result));

/**
 * Retrieve a value from Buxton
 * @param client An open client connection
 * @param key The key to retrieve
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @param sync Indicator for running a synchronous request
 * @return An int value, indicating success of the operation
 */
_bx_export_ int buxton_get_value(BuxtonClient client,
				 BuxtonKey key,
				 BuxtonCallback callback,
				 void *data,
				 bool sync)
	__attribute__((warn_unused_result));

/**
 * List all keys within a given layer in Buxon
 * @param client An open client connection
 * @param layer_name The layer to query
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @param sync Indicator for running a synchronous request
 * @return An boolean value, indicating success of the operation
 */
_bx_export_ int buxton_client_list_keys(BuxtonClient client,
					char *layer_name,
					BuxtonCallback callback,
					void *data,
					bool sync)
	__attribute__((warn_unused_result));

/**
 * Register for notifications on the given key in all layers
 * @param client An open client connection
 * @param key The key to register interest with
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @param sync Indicator for running a synchronous request
 * @return An int value, indicating success of the operation
 */
_bx_export_ int buxton_register_notification(BuxtonClient client,
					     BuxtonKey key,
					     BuxtonCallback callback,
					     void *data,
					     bool sync)
	__attribute__((warn_unused_result));

/**
 * Unregister from notifications on the given key in all layers
 * @param client An open client connection
 * @param key The key to remove registered interest from
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @param sync Indicator for running a synchronous request
 * @return An int value, indicating success of the operation
 */
_bx_export_ int buxton_unregister_notification(BuxtonClient client,
					       BuxtonKey key,
					       BuxtonCallback callback,
					       void *data,
					       bool sync)
	__attribute__((warn_unused_result));


/**
 * Unset a value by key in the given BuxtonLayer
 * @param client An open client connection
 * @param key The key to remove
 * @param callback A callback function to handle daemon reply
 * @param data User data to be used with callback function
 * @param sync Indicator for running a synchronous request
 * @return An int value, indicating success of the operation
 */
_bx_export_ int buxton_unset_value(BuxtonClient client,
				   BuxtonKey key,
				   BuxtonCallback callback,
				   void *data,
				   bool sync)
	__attribute__((warn_unused_result));

/**
 * Process messages on the socket
 * @note Will not block, useful after poll in client application
 * @param client An open client connection
 * @return Number of messages processed or -1 if there was an error
 */
_bx_export_ ssize_t buxton_client_handle_response(BuxtonClient client)
	__attribute__((warn_unused_result));

/**
 * Create a key for item lookup in buxton
 * @param group Pointer to a character string representing a group
 * @param name Pointer to a character string representing a name
 * @param layer Pointer to a character string representing a layer (optional)
 * @return A pointer to a BuxtonString containing the key
 */
_bx_export_ BuxtonKey buxton_key_create(char *group, char *name, char *layer,
				      BuxtonDataType type)
	__attribute__((warn_unused_result));

/**
 * Get the group portion of a buxton key
 * @param key A BuxtonKey
 * @return A pointer to a character string containing the key's group
 */
_bx_export_ char *buxton_key_get_group(BuxtonKey key)
	__attribute__((warn_unused_result));

/**
 * Get the name portion of a buxton key
 * @param key a BuxtonKey
 * @return A pointer to a character string containing the key's name
 */
_bx_export_ char *buxton_key_get_name(BuxtonKey key)
	__attribute__((warn_unused_result));

/**
 * Get the layer portion of a buxton key
 * @param key a BuxtonKey
 * @return A pointer to a character string containing the key's layer
 */
_bx_export_ char *buxton_key_get_layer(BuxtonKey key)
	__attribute__((warn_unused_result));

/**
 * Get the type portion of a buxton key
 * @param key a BuxtonKey
 * @return The BuxtonDataType of a key
 */
_bx_export_ BuxtonDataType buxton_key_get_type(BuxtonKey key)
	__attribute__((warn_unused_result));

/**
 * Free BuxtonKey
 * @param key a BuxtonKey
 */
_bx_export_ void buxton_key_free(BuxtonKey key);

/**
 * Get the type of a buxton response
 * @param response a BuxtonResponse
 * @return BuxtonControlMessage enum indicating the type of the response
 */
_bx_export_ BuxtonControlMessage buxton_response_type(BuxtonResponse response)
	__attribute__((warn_unused_result));

/**
 * Get the status of a buxton response
 * @param response a BuxtonResponse
 * @return int32_t enum indicating the status of the response
 */
_bx_export_ int32_t buxton_response_status(BuxtonResponse response)
	__attribute__((warn_unused_result));

/**
 * Get the key for a buxton response
 * @param response a BuxtonResponse
 * @return BuxtonKey from the response
 */
_bx_export_ BuxtonKey buxton_response_key(BuxtonResponse response)
	__attribute__((warn_unused_result));

/**
 * Get the value for a buxton response
 * @param response a BuxtonResponse
 * @return pointer to data from the response
 */
_bx_export_ void *buxton_response_value(BuxtonResponse response)
	__attribute__((warn_unused_result));

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
