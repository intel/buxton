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

#include "configurator.h"
#include "backend.h"
#include "buxton.h"
#include "hashmap.h"
#include "util.h"
#include "log.h"
#include "buxton-array.h"
#include "smack.h"

/**
 * Create a BuxtonLayer out of a ConfigLayer
 *
 * Validates the data from the config file and creates BuxtonLayer.
 *
 * @param conf_layer the ConfigLayer to validate
 *
 * @return a new BuxtonLayer.  Callers are responsible for managing
 * this memory
 */
static BuxtonLayer *buxton_layer_new(ConfigLayer *conf_layer);

bool buxton_direct_open(BuxtonControl *control)
{

	assert(control);

	memset(&(control->config), 0, sizeof(BuxtonConfig));
	if(!buxton_init_layers(&(control->config)))
		return false;

	control->client.direct = true;
	control->client.pid = getpid();

	return true;
}

bool buxton_direct_get_value(BuxtonControl *control, BuxtonString *key,
			     BuxtonData *data, BuxtonString *label)
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
						      &d,
						      label);
		if (r) {
			free(d.label.value);
			d.label.value = NULL;
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
							 data,
							 label);
	}
	return false;
}

bool buxton_direct_get_value_for_layer(BuxtonControl *control,
				       BuxtonString *layer_name,
				       BuxtonString *key,
				       BuxtonData *data,
				       BuxtonString *label)
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

	if (backend->get_value(layer, key, data)) {
		/* Access checks are not needed for direct clients, where label is NULL */
		if (label && !buxton_check_read_access(control, layer_name,
						       key, &data->label, label)) {
			/* Client lacks permission to read the value */
			return false;
		}
		buxton_debug("SMACK check succeeded for get_value for layer %s\n", layer_name->value);
		return true;
	} else {
		return false;
	}
}

bool buxton_direct_set_value(BuxtonControl *control,
			     BuxtonString *layer_name,
			     BuxtonString *key,
			     BuxtonData *data,
			     BuxtonString *label)
{
	BuxtonBackend *backend;
	BuxtonLayer *layer;
	BuxtonConfig *config;

	assert(control);
	assert(layer_name);
	assert(key);
	assert(data);

	/* Access checks are not needed for direct clients, where label is NULL */
	if (label) {
		if (!buxton_check_write_access(control, layer_name, key, &data->label, label)) {
			return false;
		}
	}

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
		r = buxton_direct_get_value_for_layer(control, layer_name, key, &data, label);
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
			       BuxtonString *key,
			       BuxtonString *label)
{
	BuxtonBackend *backend;
	BuxtonLayer *layer;
	BuxtonConfig *config;

	assert(control);
	assert(layer_name);
	assert(key);

	/* Access checks are not needed for direct clients, where label is NULL */
	if (label) {
		if (!buxton_check_write_access(control, layer_name, key, NULL, label)) {
			return false;
		}
	}

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
	int nlayers = 0;
	ConfigLayer *config_layers = NULL;

	nlayers = buxton_get_layers(&config_layers);
	layers = hashmap_new(string_hash_func, string_compare_func);
	if (!layers)
		goto end;

	for (int n = 0; n < nlayers; n++) {
		BuxtonLayer *layer;

		layer = buxton_layer_new(&(config_layers[n]));
		if (!layer)
			continue;

		hashmap_put(layers, layer->name.value, layer);
	}
	ret = true;
	config->layers = layers;

end:
	free(config_layers);
	return ret;
}

static BuxtonLayer *buxton_layer_new(ConfigLayer *conf_layer)
{
	BuxtonLayer *out;

	assert(conf_layer);
	out= malloc0(sizeof(BuxtonLayer));
	if (!out)
		abort();

	if (conf_layer->priority < 0)
		goto fail;
	out->name.value = strdup(conf_layer->name);
	if (!out->name.value)
		goto fail;
	out->name.length = (uint32_t)strlen(conf_layer->name);

	if (strcmp(conf_layer->type, "System") == 0) {
		out->type = LAYER_SYSTEM;
	} else if (strcmp(conf_layer->type, "User") == 0) {
		out->type = LAYER_USER;
	} else {
		buxton_log("Layer %s has unknown type: %s\n", conf_layer->name, conf_layer->type);
		goto fail;
	}

	if (strcmp(conf_layer->backend, "gdbm") == 0) {
		out->backend = BACKEND_GDBM;
	} else if(strcmp(conf_layer->backend, "memory") == 0) {
		out->backend = BACKEND_MEMORY;
	} else {
		buxton_log("Layer %s has unknown database: %s\n", conf_layer->name, conf_layer->backend);
		goto fail;
	}

	if (conf_layer->description != NULL)
		out->description = strdup(conf_layer->description);

	out->priority = conf_layer->priority;
	return out;
 fail:
	free(out->name.value);
	free(out->description);
	free(out);
	return NULL;
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
	if (!backend_tmp) {
		return false;
	}

	r = asprintf(&path, "%s/%s.so", buxton_module_dir(), name);
	if (r == -1) {
		free(backend_tmp);
		return false;
	}

	/* Load the module */
	handle = dlopen(path, RTLD_LAZY);

	if (!handle) {
		buxton_log("dlopen(): %s\n", dlerror());
		free(backend_tmp);
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
	BuxtonLayer *layer;

	control->client.direct = false;

	HASHMAP_FOREACH(backend, control->config.backends, iterator) {
		destroy_backend(backend);
	}
	hashmap_free(control->config.backends);
	hashmap_free(control->config.databases);

	HASHMAP_FOREACH(layer, control->config.layers, iterator) {
		free(layer->name.value);
		free(layer->description);
		free(layer);
	}
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
