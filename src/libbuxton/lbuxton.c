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

#include "../include/bt-daemon.h"
#include "../include/bt-daemon-private.h"
#include "../shared/log.h"
#include "../shared/hashmap.h"

static Hashmap *_databases = NULL;
static Hashmap *_directPermitted = NULL;

bool buxton_client_open(struct BuxtonClient *client) {
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

bool buxton_direct_open(struct BuxtonClient *client) {
	/* Forbid use by non superuser */
	if (geteuid() != 0)
		return false;

	if (!_directPermitted)
		_directPermitted = hashmap_new(trivial_hash_func, trivial_compare_func);

	client->direct = true;
	client->pid = getpid();
	hashmap_put(_directPermitted, (int)client->pid, client);
	return true;
}

char* buxton_client_get_string(struct BuxtonClient *client,
			      const char *layer,
			      const char *key) {
	/* TODO: Implement */
	return NULL;
}

bool buxton_client_set_string(struct BuxtonClient *client,
			      const char *layer,
			      const char *key,
			      const char *value) {
	/* TODO: Implement */
	if (client->direct &&  hashmap_get(_directPermitted, client->pid) == client) {
		/* Handle direct manipulation */
		BuxtonBackend *backend;
		backend = backend_for_layer(layer);
		if (!backend) {
			/* Already logged */
			return false;
		}
		return backend->set_string(key, value);
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
	memcpy(&i_func, &cast, sizeof(i_func));
	if ((error = dlerror()) != NULL) {
		buxton_log("dlsym(): %s", error);
		return false;
	}
	dlerror();

	cast = dlsym(handle, "buxton_module_destroy");
	memcpy(&d_func, &cast, sizeof(d_func));
	if ((error = dlerror()) != NULL) {
		buxton_log("dlsym(): %s", error);
		return false;
	}

	i_func(name, backend);
	backend->module = handle;

	/* TODO: Have this handled at global level and don't close in method */
	d_func(backend);
	dlclose(handle);

	return true;
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
