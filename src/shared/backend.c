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
 * \file backend.h Internal backend implementatoin
 * This file is used internally by buxton to provide functionality
 * used by and for the backend management
 */

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <dlfcn.h>
#include <iniparser.h>

#include "backend.h"
#include "bt-daemon.h"
#include "hashmap.h"
#include "util.h"
#include "log.h"

/**
 * Eventually this will be dropped
 */
static Hashmap *_directPermitted = NULL;

/**
 * Parse a given layer using the buxton configuration file
 * @param ini the configuration dictionary
 * @param name the layer to query
 * @param out The new BuxtonLayer to store
 * @return a boolean value, indicating success of the operation
 */
bool parse_layer(dictionary *ini, char *name, BuxtonLayer *out);

bool buxton_direct_open(BuxtonControl *control)
{

	assert(control);

	if (!_directPermitted)
		_directPermitted = hashmap_new(trivial_hash_func, trivial_compare_func);

	buxton_init_layers(&(control->config));

	control->client.direct = true;
	control->client.pid = getpid();
	hashmap_put(_directPermitted, &(control->client.pid), control);
	return true;
}

bool buxton_direct_permitted(BuxtonClient *client)
{
	BuxtonControl *control = NULL;

	if (_directPermitted && client->direct) {
		control = hashmap_get(_directPermitted, &(client->pid));
		if (&(control->client) == client)
			return true;
	}

	return false;
}

BuxtonConfig *buxton_get_config(BuxtonClient *client)
{
	assert(client != NULL);
	BuxtonControl *control = NULL;

	if (!_directPermitted)
		return NULL;
	control = hashmap_get(_directPermitted, &(client->pid));
	if (!control)
		return NULL;
	/* Safety */
	if (&(control->client) != client || !client->direct)
		return NULL;

	return &(control->config);
}

/* Load layer configurations from disk */
bool buxton_init_layers(BuxtonConfig *config)
{
	Hashmap *layers = NULL;
	bool ret = false;
	dictionary *ini;
	const char *path = DEFAULT_CONFIGURATION_FILE;
	int nlayers = 0;

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

	layers = hashmap_new(string_hash_func, string_compare_func);
	if (!layers)
		goto end;

	for (int n = 0; n < nlayers; n++) {
		BuxtonLayer *layer;
		char *section_name;

		layer = malloc0(sizeof(BuxtonLayer));
		if (!layer)
			continue;

		section_name = iniparser_getsecname(ini, n);
		if (!section_name) {
			buxton_log("Failed to find section number: %d\n", n);
			continue;
		}

		if (!parse_layer(ini, section_name, layer)) {
			free(layer);
			buxton_log("Failed to load layer: %s\n", section_name);
			continue;
		}
		hashmap_put(layers, layer->name.value, layer);
	}
	ret = true;
	config->layers = layers;

end:
	iniparser_freedict(ini);
finish:
	return ret;
}

bool parse_layer(dictionary *ini, char *name, BuxtonLayer *out)
{
	int r;
	_cleanup_free_ char *k_desc = NULL;
	_cleanup_free_ char *k_backend = NULL;
	_cleanup_free_ char *k_type = NULL;
	_cleanup_free_ char *k_priority = NULL;
	char *_desc = NULL;
	char *_backend = NULL;
	char *_type = NULL;
	int _priority;

	assert(ini);
	assert(name);
	assert(out);

	r = asprintf(&k_desc, "%s:description", name);
	if (r == -1)
		return false;

	r = asprintf(&k_backend, "%s:backend", name);
	if (r == -1)
		return false;

	r = asprintf(&k_type, "%s:type", name);
	if (r == -1)
		return false;

	r = asprintf(&k_priority, "%s:priority", name);
	if (r == -1)
		return false;

	_type = iniparser_getstring(ini, k_type, NULL);
	_backend = iniparser_getstring(ini, k_backend, NULL);
	_priority = iniparser_getint(ini, k_priority, -1);
	_desc = iniparser_getstring(ini, k_desc, NULL);

	if (!_type || !name || !_backend || _priority < 0)
		return false;

	out->name.value = strdup(name);
	if (!out->name.value)
		goto fail;
	out->name.length = (uint32_t)strlen(name);

	if (strcmp(_type, "System") == 0)
		out->type = LAYER_SYSTEM;
	else if (strcmp(_type, "User") == 0)
		out->type = LAYER_USER;
	else {
		buxton_log("Layer %s has unknown type: %s\n", name, _type);
		goto fail;
	}

	if (strcmp(_backend, "gdbm") == 0)
		out->backend = BACKEND_GDBM;
	else if(strcmp(_backend, "memory") == 0)
		out->backend = BACKEND_MEMORY;
	else
		goto fail;

	if (_desc != NULL)
		out->description = strdup(_desc);

	out->priority = _priority;
	return true;


fail:
	free(out->name.value);
	free(out->description);
	return false;
}

static bool init_backend(BuxtonConfig *config,
			 BuxtonLayer *layer,
			 BuxtonBackend **backend)
{
	void *handle, *cast;
	_cleanup_free_ char *path = NULL;
	const char *name;
	char *error;
	int r;
	bool rb;
	module_init_func i_func;
	module_destroy_func d_func;
	BuxtonBackend *backend_tmp;

	assert(layer);
	assert(backend);

	if (layer->backend == BACKEND_GDBM)
		name = "gdbm";
	else if (layer->backend == BACKEND_MEMORY)
		name = "memory";
	else
		return false;

	backend_tmp = hashmap_get(config->backends, name);

	if (backend_tmp) {
		*backend = backend_tmp;
		return true;
	}

	backend_tmp = malloc0(sizeof(BuxtonBackend));
	if (!backend_tmp)
		return false;

	r = asprintf(&path, "%s/%s.so", MODULE_DIRECTORY, name);
	if (r == -1)
		return false;

	/* Load the module */
	handle = dlopen(path, RTLD_LAZY);

	if (!handle) {
		buxton_log("dlopen(): %s\n", dlerror());
		return false;
	}

	dlerror();
	cast = dlsym(handle, "buxton_module_init");
	if ((error = dlerror()) != NULL || !cast) {
		buxton_log("dlsym(): %s\n", error);
		dlclose(handle);
		return false;
	}
	memcpy(&i_func, &cast, sizeof(i_func));
	dlerror();

	cast = dlsym(handle, "buxton_module_destroy");
	if ((error = dlerror()) != NULL || !cast) {
		buxton_log("dlsym(): %s\n", error);
		dlclose(handle);
		return false;
	}
	memcpy(&d_func, &cast, sizeof(d_func));

	rb = i_func(backend_tmp);
	if (!rb) {
		buxton_log("buxton_module_init failed\n");
		dlclose(handle);
		return false;
	}

	if (!config->backends) {
		config->backends = hashmap_new(trivial_hash_func, trivial_compare_func);
		if (!config->backends) {
			dlclose(handle);
			return false;
		}
	}

	r = hashmap_put(config->backends, name, backend_tmp);
	if (r != 1) {
		dlclose(handle);
		return false;
	}

	backend_tmp->module = handle;
	backend_tmp->destroy = d_func;

	*backend = backend_tmp;

	return true;
}

BuxtonBackend *backend_for_layer(BuxtonConfig *config,
				 BuxtonLayer *layer)
{
	BuxtonBackend *backend;

	assert(layer);

	if (!config->databases)
		config->databases = hashmap_new(string_hash_func, string_compare_func);
	if ((backend = (BuxtonBackend*)hashmap_get(config->databases, layer->name.value)) == NULL) {
		/* attempt load of backend */
		if (!init_backend(config, layer, &backend)) {
			buxton_log("backend_for_layer(): failed to initialise backend for layer: %s\n", layer->name);
			free(backend);
			return NULL;
		}
		hashmap_put(config->databases, layer->name.value, backend);
	}
	return (BuxtonBackend*)hashmap_get(config->databases, layer->name.value);
}

static void destroy_backend(BuxtonBackend *backend)
{

	assert(backend);

	backend->set_value = NULL;
	backend->get_value = NULL;
	backend->unset_value = NULL;
	backend->destroy();
	dlclose(backend->module);
	free(backend);
	backend = NULL;
}


void buxton_direct_revoke(BuxtonClient *client)
{
	if (_directPermitted)
		hashmap_remove(_directPermitted, &(client->pid));
	client->direct = false;
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
