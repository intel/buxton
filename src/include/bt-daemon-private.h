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

#pragma once

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>

#include "../shared/list.h"
#include "bt-daemon.h"

#define TIMEOUT 5000

#define SMACK_LABEL_LEN 255
#define ACC_LEN 5

typedef struct client_list_item {
	LIST_FIELDS(struct client_list_item, item);

	int client_socket;

	struct ucred credentials;
} client_list_item;

typedef enum BuxtonBackendType {
	BACKEND_UNSET = 0,
	BACKEND_GDBM,
	BACKEND_MEMORY,
	BACKEND_MAXTYPES
} BuxtonBackendType;

typedef enum BuxtonLayerType {
	LAYER_SYSTEM,
	LAYER_USER,
	LAYER_MAXTYPES
} BuxtonLayerType;

typedef enum BuxtonKeyAccessType {
	ACCESS_NONE = 0,
	ACCESS_READ = 1 << 0,
	ACCESS_WRITE = 1 << 1,
	ACCESS_MAXACCESSTYPES = 1 << 2
} BuxtonKeyAccessType;

typedef struct BuxtonLayer {
	char *name;
	BuxtonLayerType type;
	BuxtonBackendType backend;
	uid_t uid;
	char *priority;
	char *description;
} BuxtonLayer;

/* Module related code */
typedef bool (*module_value_func) (BuxtonLayer *layer, const char *key, BuxtonData *data);

typedef void (*module_destroy_func) (void);

typedef struct BuxtonBackend {
	void *module;
	module_destroy_func destroy;
	module_value_func set_value;
	module_value_func get_value;

} BuxtonBackend;

typedef int (*module_init_func) (BuxtonBackend *backend);

/* Initialise a backend module */
bool init_backend(BuxtonLayer *layer, BuxtonBackend *backend);

/* Close down the backend */
void destroy_backend(BuxtonBackend *backend);

/* Obtain the current backend for the given layer */
BuxtonBackend *backend_for_layer(BuxtonLayer *layer);

/* Directly manipulate buxton without socket connection */
_bx_export_ bool buxton_direct_open(BuxtonClient *client);

/* Utility function available only to backend modules */
char* get_layer_path(BuxtonLayer *layer);

/* Utility function to deep copy a BuxtonData */
void buxton_data_copy(BuxtonData* original, BuxtonData *copy);

/* Utility function to load Smack rules from the smackfs file LOAD_PATH */
void buxton_cache_smack_rules(char *load_path);

/* Utility function to check access permissions from set of Smack rules */
bool buxton_check_smack_access(char *subject, char *object, BuxtonKeyAccessType request);

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
