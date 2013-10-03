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

#define _GNU_SOURCE
#include "config.h"
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include <iniparser.h>
#include "../include/bt-daemon.h"
#include "../include/bt-daemon-private.h"
#include "../shared/log.h"
#include "../shared/hashmap.h"

static Hashmap *_databases = NULL;
static Hashmap *_directPermitted = NULL;
static Hashmap *_layers = NULL;

void buxton_init_layers(void);
void buxton_load_layer_config(char *file);

bool buxton_client_open(BuxtonClient *client) {
	int bx_socket, r;
	struct sockaddr_un remote;
	bool ret;

	if ((bx_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		ret = false;
		goto end;
	}

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, BUXTON_SOCKET, sizeof(remote.sun_path));
	r = connect(bx_socket, (struct sockaddr *)&remote, sizeof(remote));
	client->fd = bx_socket;
	if ( r == -1) {
		ret = false;
		goto close;
	}

	ret = true;
close:
	/* Will be moved to a buxton_client_close method */
	close(client->fd);
end:
	return ret;
}

bool buxton_direct_open(BuxtonClient *client) {
	if (!_directPermitted)
		_directPermitted = hashmap_new(trivial_hash_func, trivial_compare_func);

	if (!_layers)
		buxton_init_layers();

	client->direct = true;
	client->pid = getpid();
	hashmap_put(_directPermitted, (int)client->pid, client);
	return true;
}

bool buxton_client_get_value(BuxtonClient *client,
			      const char *layer,
			      const char *key,
			      BuxtonData *data) {
	/* TODO: Implement */
	return false;
}

bool buxton_client_set_value(BuxtonClient *client,
			      const char *layer,
			      const char *key,
			      BuxtonData *data) {
	/* TODO: Implement */
	if (_directPermitted && client->direct &&  hashmap_get(_directPermitted, client->pid) == client) {
		/* Handle direct manipulation */
		BuxtonBackend *backend;
		backend = backend_for_layer(layer);
		if (!backend) {
			/* Already logged */
			return false;
		}
		return backend->set_value(layer, key, data);
	}

	/* Normal interaction (wire-protocol) */
	return false;
}

BuxtonBackend* backend_for_layer(const char *layer) {
	BuxtonBackend *backend;

	if (!_databases)
		_databases = hashmap_new(string_hash_func, string_compare_func);
	if ((backend = (BuxtonBackend*)hashmap_get(_databases, layer)) == NULL) {
		/* attempt load of backend */
		if (!init_backend(layer, backend)) {
			buxton_log("backend_for_layer(): failed to initialise backend for layer: %s\n", layer);
			return NULL;
		}
		hashmap_put(_databases, layer, backend);
	}
	return (BuxtonBackend*)hashmap_get(_databases, layer);
}

bool init_backend(const char *name, BuxtonBackend* backend) {
	void *handle, *cast;
	char *path;
	char *error;
	int length;
	module_init_func i_func;
	module_destroy_func d_func;

	length = strlen(name) + strlen(MODULE_DIRECTORY) + 5;
	path = malloc(length);

	if (!path)
		return false;

	sprintf(path, "%s/%s.so", MODULE_DIRECTORY, name);

	/* Load the module */
	handle = dlopen(path, RTLD_LAZY);
	free(path);

	if (!handle) {
		buxton_log("dlopen(): %s", dlerror());
		return false;
	}

	dlerror();
	cast = dlsym(handle, "buxton_module_init");
	if ((error = dlerror()) != NULL || !cast) {
		buxton_log("dlsym(): %s", error);
		return false;
	}
	memcpy(&i_func, &cast, sizeof(i_func));
	dlerror();

	cast = dlsym(handle, "buxton_module_destroy");
	if ((error = dlerror()) != NULL || !cast) {
		buxton_log("dlsym(): %s", error);
		return false;
	}
	memcpy(&d_func, &cast, sizeof(d_func));

	i_func(backend);
	backend->module = handle;

	/* TODO: Have this handled at global level and don't close in method */
	d_func(backend);
	dlclose(handle);

	return true;
}

/* Load layer configurations from disk */
void buxton_init_layers(void) {
	dictionary *ini;
	char *path;
	int length;

	path = DEFAULT_CONFIGURATION_FILE;

	ini = iniparser_load(path);
	if (ini == NULL) {
		buxton_log("Failed to load buxton conf file: %s\n", path);
		goto finish;
	}

	/* Load layers, etc, from layer file */
	iniparser_freedict(ini);

finish:
	return;

}

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
