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
 * \file bt-daemon-private.h Internal header
 * This file is used internally by buxton to provide functionality
 * to bt-daemon
 */
#pragma once

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <stdint.h>

#include "../shared/list.h"
#include "bt-daemon.h"

/**
 * Maximum length for a Smack label
 */
#define SMACK_LABEL_LEN 255
/**
 * Smack label xattr key
 */
#define SMACK_ATTR_NAME "security.SMACK64"
/**
 * Length of ACC
 */
#define ACC_LEN 5
/**
 * Minimum size of serialized BuxtonData
 */
#define BXT_MINIMUM_SIZE sizeof(BuxtonDataType) + (sizeof(int)*2)

/**
 * Daemon's list of clients
 */
typedef struct client_list_item {
	LIST_FIELDS(struct client_list_item, item); /**<List type */
	int fd; /**<File descriptor of connected client */
	struct ucred cred; /**<Credentials of connected client */
	char *smack_label; /**<Smack label of connected client */
} client_list_item;

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
 * Represents client access to a given resource
 */
typedef enum BuxtonKeyAccessType {
	ACCESS_NONE = 0, /**<No access permitted */
	ACCESS_READ = 1 << 0, /**<Read access permitted */
	ACCESS_WRITE = 1 << 1, /**<Write access permitted */
	ACCESS_MAXACCESSTYPES = 1 << 2
} BuxtonKeyAccessType;

/**
 * Represents a layer within Buxton
 *
 * Keys can be stored in various layers within Buxton, using a variety
 * of backend and configurations. This is all handled transparently and
 * through a consistent API
 */
typedef struct BuxtonLayer {
	char *name; /**<Name of the layer*/
	BuxtonLayerType type; /**<Type of layer */
	BuxtonBackendType backend; /**<Backend for this layer */
	uid_t uid; /**<User ID for layers of type LAYER_USER */
	char *priority; /**<Priority of this layer in relation to others */
	char *description; /**<Description of this layer */
} BuxtonLayer;

/* Module related code */
/**
 * Backend manipulation function
 * @param layer The layer to manipulate or query
 * @param key The key to manipulate or query
 * @param data Set or get data, dependant on operation
 * @return a boolean value, indicating success of the operation
 */
typedef bool (*module_value_func) (BuxtonLayer *layer, const char *key, BuxtonData *data);

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

} BuxtonBackend;

/**
 * Module initialisation function
 * @param backend The backend to initialise
 * @return An integer value, representing the success of the operation
 */
typedef int (*module_init_func) (BuxtonBackend *backend);

/**
 * Initialise a backend with the given layer
 * @param layer The Buxtonlayer being queried or manipulated
 * @param backend An empty struct with which to initialise the module
 * @return a boolean value, indicating success of the operation
 */
bool init_backend(BuxtonLayer *layer, BuxtonBackend *backend);

/**
 * Shut down a backend and its resources
 * @param backend The backend to destroy
 */
void destroy_backend(BuxtonBackend *backend);

/**
 * Get the backend for the given layer
 * @param layer The layer in question
 * @return A BuxtonBackend if the layer exists, or NULL
 */
BuxtonBackend *backend_for_layer(BuxtonLayer *layer);

/**
 * Open a direct connection to Buxton
 *
 * This function can only be used internally by Buxton.
 * @param client The client struct will be used throughout the life of Buxton operations
 * @return a boolean value, indicating success of the operation
 */
_bx_export_ bool buxton_direct_open(BuxtonClient *client);

/**
 * Retrieve the filesystem path for the given layer
 * @param layer The layer in question
 * @return a string containing the filesystem path
 */
char* get_layer_path(BuxtonLayer *layer);

/**
 * Perform a deep copy of one BuxtonData to another
 * @param original The data being copied
 * @param copy Pointer where original should be copied to
 */
void buxton_data_copy(BuxtonData* original, BuxtonData *copy);

/**
 * Serialize data internally for backend consumption
 * @param source Data to be serialized
 * @param dest Pointer to store serialized data in
 * @return a boolean value, indicating success of the operation
 */
bool buxton_serialize(BuxtonData *source, uint8_t** dest);

/**
 * Deserialize internal data for client consumption
 * @param source Serialized data pointer
 * @param dest A pointer where the deserialize data will be stored
 * @return a boolean value, indicating success of the operation
 */
bool buxton_deserialize(uint8_t *source, BuxtonData *dest);

/**
 * Load Smack rules from the kernel
 * @return a boolean value, indicating success of the operation
 */
bool buxton_cache_smack_rules(void);

/**
 * Check whether the smack access matches the buxton client access
 * @param subject Smack subject label
 * @param object Smack object label
 * @param request The buxton access type being queried
 * @return true if the smack access matches the given request, otherwise false
 */
bool buxton_check_smack_access(char *subject, char *object, BuxtonKeyAccessType request);

/* Set up inotify to track the Smack rule file for changes */
/**
 * Set up inotify to track Smack rule file for changes
 * @return an exit code for the operation
 */
int buxton_watch_smack_rules(void);

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
