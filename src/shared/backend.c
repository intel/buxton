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
#include "buxton-array.h"

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

	memset(&(control->config), 0, sizeof(BuxtonConfig));
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

bool buxton_direct_get_value(BuxtonControl *control, BuxtonString *key,
			     BuxtonData *data)
{
	/* Handle direct manipulation */
	BuxtonLayer *l;
	BuxtonConfig *config;
	BuxtonString layer = (BuxtonString){ NULL, 0 };
	Iterator i;
	BuxtonData d;
	int priority = 0;
	int r;

	assert(control);
	assert(key);

	config = &control->config;

	HASHMAP_FOREACH(l, config->layers, i) {
		r = buxton_direct_get_value_for_layer(control,
						      &l->name,
						      key,
						      &d);
		if (r) {
			free(d.label.value);
			if (d.type == STRING)
				free(d.store.d_string.value);
			if (priority <= l->priority) {
				priority = l->priority;
				layer.value = l->name.value;
				layer.length = l->name.length;
			}
		}
	}
	if (layer.value) {
		return buxton_direct_get_value_for_layer(control,
							 &layer,
							 key,
							 data);
	}
	return false;
}

bool buxton_direct_get_value_for_layer(BuxtonControl *control,
				       BuxtonString *layer_name,
				       BuxtonString *key,
				       BuxtonData *data)
{
	/* Handle direct manipulation */
	BuxtonBackend *backend = NULL;
	BuxtonLayer *layer = NULL;
	BuxtonConfig *config;

	config = &control->config;
	if ((layer = hashmap_get(config->layers, layer_name->value)) == NULL) {
		return false;
	}
	backend = backend_for_layer(config, layer);
	if (!backend) {
		/* Already logged */
		return false;
	}
	layer->uid = control->client.uid;
	return backend->get_value(layer, key, data);
}

bool buxton_direct_set_value(BuxtonControl *control,
			     BuxtonString *layer_name,
			     BuxtonString *key,
			     BuxtonData *data)
{
	BuxtonBackend *backend;
	BuxtonLayer *layer;
	BuxtonConfig *config;

	assert(control);
	assert(layer_name);
	assert(key);
	assert(data);

	config = &control->config;
	if ((layer = hashmap_get(config->layers, layer_name->value)) == NULL) {
		return false;
	}
	backend = backend_for_layer(config, layer);
	if (!backend) {
		/* Already logged */
		return false;
	}
	layer->uid = control->client.uid;
	return backend->set_value(layer, key, data);
}

bool buxton_direct_set_label(BuxtonControl *control,
			     BuxtonString *layer_name,
			     BuxtonString *key,
			     BuxtonString *label)
{
	BuxtonBackend *backend;
	BuxtonData data;
	BuxtonLayer *layer;
	BuxtonConfig *config;
	bool r;

	assert(control);
	assert(layer_name);
	assert(key);
	assert(label);

	config = &control->config;

	if ((layer = hashmap_get(config->layers, layer_name->value)) == NULL) {
		return false;
	}
	backend = backend_for_layer(config, layer);
	if (!backend) {
		/* Already logged */
		return false;
	}

	char *name = get_name(key);
	if (name) {
		r = buxton_direct_get_value_for_layer(control, layer_name, key, &data);
		if (!r)
			return false;

		free(data.label.value);
	} else {
		/* we have a group, so initialize the data with a dummy value */
		data.type = STRING;
		data.store.d_string = buxton_string_pack("BUXTON_GROUP_VALUE");
	}

	data.label.length = label->length;
	data.label.value = label->value;

	return backend->set_value(layer, key, &data);
}

bool buxton_direct_list_keys(BuxtonControl *control,
			     BuxtonString *layer_name,
			     BuxtonArray **list)
{
	assert(control);
	assert(layer_name);
	assert(layer_name->value);

	/* Handle direct manipulation */
	BuxtonBackend *backend = NULL;
	BuxtonLayer *layer;
	BuxtonConfig *config;

	config = &control->config;
	if ((layer = hashmap_get(config->layers, layer_name->value)) == NULL) {
		return false;
	}
	backend = backend_for_layer(config, layer);
	if (!backend) {
		/* Already logged */
		return false;
	}
	layer->uid = control->client.uid;
	return backend->list_keys(layer, list);
}

bool buxton_direct_unset_value(BuxtonControl *control,
			       BuxtonString *layer_name,
			       BuxtonString *key)
{
	BuxtonBackend *backend;
	BuxtonLayer *layer;
	BuxtonConfig *config;

	assert(control);
	assert(layer_name);
	assert(key);

	config = &control->config;
	if ((layer = hashmap_get(config->layers, layer_name->value)) == NULL) {
		return false;
	}
	backend = backend_for_layer(config, layer);
	if (!backend) {
		/* Already logged */
		return false;
	}
	layer->uid = control->client.uid;
	return backend->unset_value(layer, key, NULL);
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
	backend->list_keys = NULL;
	backend->unset_value = NULL;
	backend->destroy();
	dlclose(backend->module);
	free(backend);
	backend = NULL;
}

void buxton_direct_close(BuxtonControl *control)
{
	Iterator iterator;
	BuxtonBackend *backend;

	if (_directPermitted)
		hashmap_remove(_directPermitted, &(control->client.pid));
	control->client.direct = false;

	HASHMAP_FOREACH(backend, control->config.backends, iterator) {
		destroy_backend(backend);
	}
	hashmap_free(control->config.backends);
	hashmap_free(control->config.databases);
	hashmap_free(control->config.layers);

	control->client.direct = false;
	control->config.backends = NULL;
	control->config.databases = NULL;
	control->config.layers = NULL;
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
