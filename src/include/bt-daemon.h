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
 * \file bt-daemon.h Buxton public header
 *
 * This is the public part of libbuxton
 *
 * \mainpage Buxton
 * \link bt-daemon.h Public API
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
 * Used to communicate with Buxton
 */
typedef struct BuxtonClient {
	int fd; /**<The file descriptor for the connection */
	bool direct; /**<Only used for direction connections */
	pid_t pid; /**<Process ID, used within libbuxton */
	uid_t uid; /**<User ID of currently using user */
} BuxtonClient;

/**
 * Possible data types for use in Buxton
 */
typedef enum BuxtonDataType {
	BUXTON_TYPE_MIN,
	STRING, /**<Represents type of a string value */
	INT32, /**<Represents type of an int32_t value */
	INT64, /**<Represents type of a int64_t value */
	FLOAT, /**<Represents type of a float value */
	DOUBLE, /**<Represents type of a double value */
	BOOLEAN, /**<Represents type of a boolean value */
	BUXTON_TYPE_MAX
} BuxtonDataType;

/**
 * Stores a string entity in Buxton
 */
typedef struct BuxtonString {
	char *value; /**<The content of the string */
	uint32_t length; /**<The length of the string */
} BuxtonString;

/**
 * Stores values in Buxton, may have only one value
 */
typedef union BuxtonDataStore {
	BuxtonString d_string; /**<Stores a string value */
	int32_t d_int32; /**<Stores an int32_t value */
	int64_t d_int64; /**<Stores a int64_t value */
	float d_float; /**<Stores a float value */
	double d_double; /**<Stores a double value */
	bool d_boolean; /**<Stores a boolean value */
} BuxtonDataStore;

/**
 * Represents data in Buxton
 *
 * In Buxton we operate on all data using BuxtonData, for both set and
 * get operations. The type must be set to the type of value being set
 * in the BuxtonDataStore
 */
typedef struct BuxtonData {
	BuxtonDataType type; /**<Type of data stored */
	BuxtonDataStore store; /**<Contains one value, correlating to
			       * type */
	BuxtonString label; /**<SMACK label for data */
} BuxtonData;

static inline void buxton_string_to_data(BuxtonString *s, BuxtonData *d)
{
	d->type = STRING;
	d->store.d_string.value = s->value;
	d->store.d_string.length = s->length;
}

/* Buxton API Methods */

/**
 * Open a connection to Buxton
 * @param client A BuxtonClient
 * @return A boolean value, indicating success of the operation
 */
_bx_export_ bool buxton_client_open(BuxtonClient *client);

/**
 * Close the connection to Buxton
 * @param client A BuxtonClient
 */
_bx_export_ void buxton_client_close(BuxtonClient *client);

/**
 * Set a value within Buxton
 * @param client An open client connection
 * @param layer The layer to manipulate
 * @param key The key name
 * @param label A BuxtonString containing the label to set
 * @return A boolean value, indicating success of the operation
 */
_bx_export_ bool buxton_client_set_label(BuxtonClient *client,
					 BuxtonString *layer,
					 BuxtonString *key,
					 BuxtonString *label);

/**
 * Set a value within Buxton
 * @param client An open client connection
 * @param layer The layer to manipulate
 * @param key The key name
 * @param data A struct containing the data to set
 * @return A boolean value, indicating success of the operation
 */
_bx_export_ bool buxton_client_set_value(BuxtonClient *client,
					 BuxtonString *layer,
					 BuxtonString *key,
					 BuxtonData *data);

/**
 * Retrieve a value from Buxton
 * @param client An open client connection
 * @param key The key to retrieve
 * @param data An empty BuxtonData, where data is stored
 * @return A boolean value, indicating success of the operation
 */
_bx_export_ bool buxton_client_get_value(BuxtonClient *client,
					 BuxtonString *key,
					 BuxtonData *data);

/**
 * Retrieve a value from Buxton
 * @param client An open client connection
 * @param layer The layer where the key is set
 * @param key The key to retrieve
 * @param data An empty BuxtonData, where data is stored
 * @return A boolean value, indicating success of the operation
 */
_bx_export_ bool buxton_client_get_value_for_layer(BuxtonClient *client,
						   BuxtonString *layer,
						   BuxtonString *key,
						   BuxtonData *data);

/**
 * Register for notifications on the given key in all layers
 * @param client An open client connection
 * @param key The key to register interest with
 * @return a boolean value, indicating success of the operation
 */
_bx_export_ bool buxton_client_register_notification(BuxtonClient *client,
						     BuxtonString *key);

/**
 * Delete a value by key in the given BuxtonLayer
 * @param client An open client connection
 * @param layer The layer to remove the value from
 * @param key The key to remove
 * @return a boolean value, indicating success of the operation
 */
_bx_export_ bool buxton_client_delete_value(BuxtonClient *client,
					    BuxtonString *layer,
					    BuxtonString *key);
/**
 * Open a direct connection to Buxton
 *
 * @param client The client struct will be used throughout the life of Buxton operations
 * @return a boolean value, indicating success of the operation
 */
_bx_export_ bool buxton_direct_open(BuxtonClient *client);

/**
 * Create a key for item lookup in buxton
 * @param group Pointer to a character string representing a group
 * @param name Pointer to a character string representing a name
 * @return A pointer to a BuxtonString containing the key
 */
_bx_export_ BuxtonString *buxton_make_key(char *group, char *name);

/**
 * Get the group portion of a buxton key
 * @param key Pointer to BuxtonString representing a buxton key
 * @return A pointer to a character string containing the key's group
 */
_bx_export_ char *buxton_get_group(BuxtonString *key);

/**
 * Get the name portion of a buxton key
 * @param key Pointer to BuxtonString representing a buxton key
 * @return A pointer to a character string containing the key's name
 */
_bx_export_ char *buxton_get_name(BuxtonString *key);

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
