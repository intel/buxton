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
typedef bool (*module_init_func) (BuxtonBackend *backend);

/**
 * Open a direct connection to Buxton
 *
 * @param control Valid BuxtonControl instance
 * @return a boolean value, indicating success of the operation
 */
bool buxton_direct_open(BuxtonControl *control);

/**
 * Determine if a client is permitted to use direct connections
 * @param client Valid buxton client connection
 * @return a boolean value, indicating permission to use direct mode
 */
bool buxton_direct_permitted(BuxtonClient *client);

/**
 * Close direct Buxton management connection
 * @param control Valid BuxtonControl instance
 */
void buxton_direct_close(BuxtonControl *control);

/**
 * Return the associated configuration for a given client (direct access)
 * @param client The client to query
 * @return a valid BuxtonConfig, or NULL if none is found
 */
BuxtonConfig *buxton_get_config(BuxtonClient *client);

/**
 * Return a valid backend for the given configuration and layer
 * @param config A BuxtonControl's configuration
 * @param layer The layer to query
 * @return an initialised backend, or NULL if the layer is not found
 */
BuxtonBackend *backend_for_layer(BuxtonConfig *config,
				 BuxtonLayer *layer);

/**
 * Initialize layers using the configuration file
 * @param config A BuxtonControl's configuration
 * @return a boolean value, indicating success of the operation
 */
bool buxton_init_layers(BuxtonConfig *config);

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
