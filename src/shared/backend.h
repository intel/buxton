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

#include <gdbm.h>

#include "buxtonarray.h"
#include "buxtondata.h"
#include "buxtonstring.h"
#include "protocol.h"
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
	bool readonly; /**<Layer is readonly or not */
} BuxtonLayer;

/**
 * Backend manipulation function
 * @param layer The layer to manipulate or query
 * @param key The key to manipulate or query
 * @param data Set or get data, dependant on operation
 * @param label The key's label
 * @return a int value, indicating success of the operation or errno
 */
typedef int (*module_value_func) (BuxtonLayer *layer, _BuxtonKey *key,
				  BuxtonData *data, BuxtonString *label);

/**
 * Backend key list function
 * @param layer The layer to query
 * @param data Pointer to store BuxtonArray in
 * @return a boolean value, indicating success of the operation
 */
typedef bool (*module_list_func) (BuxtonLayer *layer, BuxtonArray **data);

/**
 * Backend database creation function
 * @param layer The layer matching the db to create
 * @return A Database (not intended to be used from direct functions)
 */
typedef void *(*module_db_init_func) (BuxtonLayer *layer);

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
	module_db_init_func create_db; /**<DB file creation function */
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
	_BuxtonClient client; /**<Valid client connection */
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
 */
void buxton_init_layers(BuxtonConfig *config);

/**
 * Remove association with backend (free and dlclose)
 * @param backend A BuxtonBackend to be removed
 */
void destroy_backend(BuxtonBackend *backend);

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
