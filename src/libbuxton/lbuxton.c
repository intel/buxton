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
bool buxton_load_layer_config(char *file);
bool parse_layer(dictionary *ini, char *name, BuxtonLayer *out);

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
	DIR *directory;
	struct dirent *dir_entry;
	const char *extension = ".layer";
	size_t extension_size = strlen(extension);

	if ((directory = opendir(CONFIG_DIRECTORY)) == NULL) {
		buxton_log("Failed to open configuration directory\n");
		return;
	}

	while (dir_entry = readdir(directory)) {
		size_t stlen;

		stlen = strlen(dir_entry->d_name);
		if (stlen < extension_size)
			continue;

		if (strncmp(dir_entry->d_name + stlen - extension_size, extension, stlen) == 0) {
			buxton_load_layer_config(dir_entry->d_name);
		}
	}

	closedir(directory);
}

bool buxton_load_layer_config(char *file) {
	bool ret = false;
	dictionary *ini;
	char *path;
	int length;
	char *top_section;
	char *top_layers;
	char *top_label;
	char *top_backend;
	char *keys;
	char *current_key;

	length = strlen(CONFIG_DIRECTORY) + strlen(file) + 2;
	path = malloc(length);

	if (!path)
		return

	snprintf(path, length, "%s/%s", CONFIG_DIRECTORY, file);

	ini = iniparser_load(path);
	if (ini == NULL) {
		buxton_log("Failed to load buxton layer: %s\n", file);
		goto end;
	}

	/* Load layers, etc, from layer file */
	top_section = iniparser_getsecname(ini, 0);
	printf("Top section: %s\n", top_section);

	/* Do we consider 'defaults' as standard ? */

	/* Now read in all layer names */
	top_layers = iniparser_getstring(ini, "defaults:layers", NULL);
	if (top_layers == NULL) {
		buxton_log("Failed to determine default layers for %s\n", file);
		goto fail;
	}

	/* Get the main smack label */
	top_label = iniparser_getstring(ini, "defaults:smacklabel", NULL);
	if (top_label == NULL) {
		buxton_log("Failed to determine default SMACK Label for %s\n", file);
		goto fail;
	}

	/* Get the primary backend */
	top_backend = iniparser_getstring(ini, "defaults:backend", NULL);
	if (top_backend == NULL) {
		buxton_log("Failed to determine default backend for %s\n", file);
		goto fail;
	}

	keys = strdup(top_layers);
	current_key = strtok(keys, ", ");

	if (current_key == NULL) {
		buxton_log("Invalid file: %s\n", path);
		goto tok_end;
	}

	/* search and load key o_O */
	while ((current_key = strtok(NULL, ", ")) != NULL) {
		BuxtonLayer layer;
		if (!parse_layer(ini, current_key, &layer)) {
			buxton_log("Failed to load layer: %s\n", current_key);
			continue;
		}
		if (layer.smack_label == NULL)
			layer.smack_label = top_label;
		if (layer.backend == NULL)
			layer.backend = top_backend;
		/* TOOD: Store layer */		
	}

	ret = EXIT_SUCCESS;

tok_end:
	free(keys);

	goto end;

fail:
	ret = EXIT_FAILURE;
end:
	free(path);
	iniparser_freedict(ini);

	return ret;
}

bool parse_layer(dictionary *ini, char *name, BuxtonLayer *out) {
	bool ret = false;
	size_t len = strlen(name);
	char *desc, *label, *backend_name;
	char *description;

	desc = malloc(len + strlen(":description"));
	if (!desc)
		goto end;
	sprintf(desc, "%s:description", name);

	label = malloc(len + strlen(":smacklabel"));
	if (!label)
		goto end;
	sprintf(label, "%s:smacklabel", name);

	backend_name = malloc(len + strlen(":backend"));
	if (!backend_name)
		goto end;
	sprintf(backend_name, "%s:backend", name);

	description = iniparser_getstring(ini, desc, NULL);
	/* Description is mandatory! */
	if (description == NULL)
		goto end;

	out->name = name;
	out->description = description;

	/* Ok to be null */
	out->smack_label = iniparser_getstring(ini, label, NULL);
	out->backend = iniparser_getstring(ini, backend_name, NULL);

	ret = true;
end:
	if (desc)
		free(desc);
	if (label)
		free(label);
	if (backend_name)
		free(backend_name);

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
