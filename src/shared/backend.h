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
 * \file backend.h Internal header
 * This file is used internally by buxton to provide functionality
 * used by and for the backend
 */
#pragma once

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include "bt-daemon.h"
#include "hashmap.h"

/**
 * Possible backends for Buxton
 */
typedef enum BuxtonBackendType {
	BACKEND_UNSET = 0, /**<No backend set */
	BACKEND_GDBM, /**<GDBM backend */
	BACKEND_MEMORY, /**<Memory backend */
	BACKEND_MAXTYPES
} BuxtonBackendType;

/**
 * Buxton layer type
 */
typedef enum BuxtonLayerType {
	LAYER_SYSTEM, /**<A system layer*/
	LAYER_USER, /**<A user layer */
	LAYER_MAXTYPES
} BuxtonLayerType;

/**
 * Represents a layer within Buxton
 *
 * Keys can be stored in various layers within Buxton, using a variety
 * of backend and configurations. This is all handled transparently and
 * through a consistent API
 */
typedef struct BuxtonLayer {
	BuxtonString name; /**<Name of the layer*/
	BuxtonLayerType type; /**<Type of layer */
	BuxtonBackendType backend; /**<Backend for this layer */
	uid_t uid; /**<User ID for layers of type LAYER_USER */
	int priority; /**<Priority of this layer */
	char *description; /**<Description of this layer */
} BuxtonLayer;

/**
 * Backend manipulation function
 * @param layer The layer to manipulate or query
 * @param key The key to manipulate or query
 * @param data Set or get data, dependant on operation
 * @return a boolean value, indicating success of the operation
 */
typedef bool (*module_value_func) (BuxtonLayer *layer, BuxtonString *key,
				   BuxtonData *data);

/**
 * Backend key list function
 * @param layer The layer to query
 * @param data Pointer to store BuxtonArray in
 * @return a boolean value, indicating success of the operation
 */
typedef bool (*module_list_func) (BuxtonLayer *layer, BuxtonArray **data);

/**
 * Destroy (or shutdown) a backend module
 */
typedef void (*module_destroy_func) (void);

/**
 * A data-backend for Buxton
 *
 * Backends are controlled by Buxton for storing and retrieving data
 */
typedef struct BuxtonBackend {
	void *module; /**<Private handle to the module */
	module_destroy_func destroy; /**<Destroy method */
	module_value_func set_value; /**<Set value function */
	module_value_func get_value; /**<Get value function */
	module_list_func list_keys; /**<List keys function */
	module_value_func unset_value; /**<Unset value function */
} BuxtonBackend;

/**
 * Stores internal configuration of Buxton
 */
typedef struct BuxtonConfig {
	Hashmap *databases; /**<Database mapping */
	Hashmap *layers; /**<Global layer configuration */
	Hashmap *backends; /**<Backend mapping */
} BuxtonConfig;

/**
 * Internal controller for Buxton
 */
typedef struct BuxtonControl {
	BuxtonClient client; /**<Valid client connection */
	BuxtonConfig config; /**<Valid configuration (unused) */
} BuxtonControl;

/**
 * Module initialisation function
 * @param backend The backend to initialise
 * @return A boolean value, representing the success of the operation
 */
typedef bool (*module_init_func) (BuxtonBackend *backend)
	__attribute__((warn_unused_result));

/**
 * Open a direct connection to Buxton
 *
 * @param control Valid BuxtonControl instance
 * @return a boolean value, indicating success of the operation
 */
bool buxton_direct_open(BuxtonControl *control)
	__attribute__((warn_unused_result));

/**
 * Close direct Buxton management connection
 * @param control Valid BuxtonControl instance
 */
void buxton_direct_close(BuxtonControl *control);

/**
 * Return a valid backend for the given configuration and layer
 * @param config A BuxtonControl's configuration
 * @param layer The layer to query
 * @return an initialised backend, or NULL if the layer is not found
 */
BuxtonBackend *backend_for_layer(BuxtonConfig *config,
				 BuxtonLayer *layer)
	__attribute__((warn_unused_result));

/**
 * Initialize layers using the configuration file
 * @param config A BuxtonControl's configuration
 * @return a boolean value, indicating success of the operation
 */
bool buxton_init_layers(BuxtonConfig *config)
	__attribute__((warn_unused_result));

/**
 * Set a value within Buxton
 * @param control An initialized control structure
 * @param layer The layer to manipulate
 * @param key The key name
 * @param label A BuxtonString containing the label to set
 * @return A boolean value, indicating success of the operation
 */
bool buxton_direct_set_label(BuxtonControl *control,
			     BuxtonString *layer,
			     BuxtonString *key,
			     BuxtonString *label)
	__attribute__((warn_unused_result));

/**
 * Set a value within Buxton
 * @param control An initialized control structure
 * @param layer The layer to manipulate
 * @param key The key name
 * @param data A struct containing the data to set
 * @return A boolean value, indicating success of the operation
 */
bool buxton_direct_set_value(BuxtonControl *control,
			     BuxtonString *layer_name,
			     BuxtonString *key,
			     BuxtonData *data)
	__attribute__((warn_unused_result));

/**
 * Retrieve a value from Buxton
 * @param control An initialized control structure
 * @param key The key to retrieve
 * @param data An empty BuxtonData, where data is stored
 * @return A boolean value, indicating success of the operation
 */
bool buxton_direct_get_value(BuxtonControl *control,
			     BuxtonString *key,
			     BuxtonData *data)
	__attribute__((warn_unused_result));

/**
 * Retrieve a value from Buxton
 * @param control An initialized control structure
 * @param layer The layer where the key is set
 * @param key The key to retrieve
 * @param data An empty BuxtonData, where data is stored
 * @return A boolean value, indicating success of the operation
 */
bool buxton_direct_get_value_for_layer(BuxtonControl *control,
				       BuxtonString *layer,
				       BuxtonString *key,
				       BuxtonData *data)
	__attribute__((warn_unused_result));

/**
 * Retrieve a list of keys from Buxton
 * @param control An initialized control structure
 * @param layer_name The layer to pquery
 * @param data An empty BuxtonArray, where results are stored
 * @return A boolean value, indicating success of the operation
 */
bool buxton_direct_list_keys(BuxtonControl *control,
			     BuxtonString *layer,
			     BuxtonArray **list)
	__attribute__((warn_unused_result));

/**
 * Unset a value by key in the given BuxtonLayer
 * @param control An initialized control structure
 * @param layer The layer to remove the value from
 * @param key The key to remove
 * @return a boolean value, indicating success of the operation
 */
bool buxton_direct_unset_value(BuxtonControl *control,
			       BuxtonString *layer,
			       BuxtonString *key)
	__attribute__((warn_unused_result));

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
