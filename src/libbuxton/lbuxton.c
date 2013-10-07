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

bool buxton_init_layers(void);
bool parse_layer(dictionary *ini, char *name, BuxtonLayer *out);

bool buxton_client_open(BuxtonClient *client)
{
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

bool buxton_direct_open(BuxtonClient *client)
{
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
			      BuxtonData *data)
{
	/* TODO: Implement */
	return false;
}

bool buxton_client_set_value(BuxtonClient *client,
			      const char *layer,
			      const char *key,
			      BuxtonData *data)
{
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

BuxtonBackend* backend_for_layer(const char *layer)
{
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

bool init_backend(const char *name, BuxtonBackend* backend)
{
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
bool buxton_init_layers(void)
{
	bool ret = false;
	dictionary *ini;
	char *path;
	int nlayers = 0;

	path = DEFAULT_CONFIGURATION_FILE;

	ini = iniparser_load(path);
	if (ini == NULL) {
		buxton_log("Failed to load buxton conf file: %s\n", path);
		goto finish;
	}

	nlayers = iniparser_getnsec(ini);
	if (nlayers <= 0) {
		buxton_log("No layers defined in buxton conf file: %s\n", path);
		goto end;
	}

	_layers = hashmap_new(string_hash_func, string_compare_func);
	if (!_layers)
		goto end;

	for (int n = 0; n < nlayers; n++) {
		BuxtonLayer *layer;
		char *section_name;

		layer = malloc(sizeof(BuxtonLayer));
		if (!layer)
			continue;

		section_name = iniparser_getsecname(ini, n);
		if (!section_name) {
			buxton_log("Failed to find section number: %d\n", n);
			continue;
		}

		if (!parse_layer(ini, section_name, layer)) {
			buxton_log("Failed to load layer: %s\n", section_name);
			continue;
		}
		hashmap_put(_layers, layer->name, layer);
	}
	ret = true;

end:
	iniparser_freedict(ini);
finish:
	return ret;
}

bool parse_layer(dictionary *ini, char *name, BuxtonLayer *out)
{
	bool ret = false;
	size_t len = strlen(name);
	char *k_desc, *k_backend, *k_type, *k_priority;
	char *_desc, *_backend, *_type, *_priority;

	k_desc = malloc(len + strlen(":description"));
	if (!k_desc)
		goto end;
	sprintf(k_desc, "%s:description", name);

	k_backend = malloc(len + strlen(":backend"));
	if (!k_backend)
		goto end;
	sprintf(k_backend, "%s:backend", name);

	k_type = malloc(len + strlen(":type"));
	if (!k_type)
		goto end;
	sprintf(k_type, "%s:type", name);

	k_priority = malloc(len + strlen(":priority"));
	if (!k_priority)
		goto end;
	sprintf(k_priority, "%s:priority", name);

	_type = iniparser_getstring(ini, k_type, NULL);
	/* Type and Name are mandatory! */
	if (_type == NULL || name == NULL)
		goto end;

	out->name = strdup(name);
	out->type = strdup(_type);

	/* Ok to be null */
	_desc = iniparser_getstring(ini, k_desc, NULL);
	if (_desc != NULL)
		out->description = strdup(_desc);
	_backend = iniparser_getstring(ini, k_backend, NULL);
	if (_backend != NULL)
		out->backend = strdup(_backend);
	_priority = iniparser_getstring(ini, k_priority, NULL);
	if (_priority != NULL)
		out->priority = strdup(_priority);
	ret = true;
end:
	if (k_desc)
		free(k_desc);
	if (k_backend)
		free(k_backend);
	if (k_type)
		free(k_type);
	if (k_priority)
		free(k_priority);

	return ret;
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
