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

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <dlfcn.h>

#include "configurator.h"
#include "backend.h"
#include "hashmap.h"
#include "util.h"
#include "log.h"
#include "buxtonarray.h"
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

/* Load layer configurations from disk */
void buxton_init_layers(BuxtonConfig *config)
{
	Hashmap *layers = NULL;
	int nlayers = 0;
	ConfigLayer *config_layers = NULL;
	int r;

	nlayers = buxton_key_get_layers(&config_layers);
	layers = hashmap_new(string_hash_func, string_compare_func);
	if (!layers) {
		abort();
	}

	for (int n = 0; n < nlayers; n++) {
		BuxtonLayer *layer;

		layer = buxton_layer_new(&(config_layers[n]));
		if (!layer) {
			abort();
		}

		r = hashmap_put(layers, layer->name.value, layer);
		if (r != 1) {
			abort();
		}
	}

	config->layers = layers;
	free(config_layers);
}

static bool is_read_only(ConfigLayer *conf_layer)
{
	return strcmp(conf_layer->access, "read-only") == 0;
}

static BuxtonLayer *buxton_layer_new(ConfigLayer *conf_layer)
{
	BuxtonLayer *out;

	assert(conf_layer);
	out= malloc0(sizeof(BuxtonLayer));
	if (!out) {
		abort();
	}

	if (conf_layer->priority < 0) {
		goto fail;
	}
	out->name.value = strdup(conf_layer->name);
	if (!out->name.value) {
		abort();
	}
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
	} else if (strcmp(conf_layer->backend, "memory") == 0) {
		out->backend = BACKEND_MEMORY;
	} else {
		buxton_log("Layer %s has unknown database: %s\n", conf_layer->name, conf_layer->backend);
		goto fail;
	}

	if (conf_layer->description != NULL) {
		out->description = strdup(conf_layer->description);
		if (!out->description) {
			abort();
		}
	}

	out->readonly = is_read_only(conf_layer);
	out->priority = conf_layer->priority;
	return out;
fail:
	free(out->name.value);
	free(out->description);
	free(out);
	return NULL;
}

static void init_backend(BuxtonConfig *config,
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

	if (layer->backend == BACKEND_GDBM) {
		name = "gdbm";
	} else if (layer->backend == BACKEND_MEMORY) {
		name = "memory";
	} else {
		buxton_log("Invalid backend type for layer: %s\n", layer->name);
		abort();
	}

	backend_tmp = hashmap_get(config->backends, name);

	if (backend_tmp) {
		*backend = backend_tmp;
		return;
	}

	backend_tmp = malloc0(sizeof(BuxtonBackend));
	if (!backend_tmp) {
		abort();
	}

	r = asprintf(&path, "%s/%s.so", buxton_module_dir(), name);
	if (r == -1) {
		abort();
	}

	/* Load the module */
	handle = dlopen(path, RTLD_LAZY);

	if (!handle) {
		buxton_log("dlopen(): %s\n", dlerror());
		abort();
	}

	dlerror();
	cast = dlsym(handle, "buxton_module_init");
	if ((error = dlerror()) != NULL || !cast) {
		buxton_log("dlsym(): %s\n", error);
		abort();
	}
	memcpy(&i_func, &cast, sizeof(i_func));
	dlerror();

	cast = dlsym(handle, "buxton_module_destroy");
	if ((error = dlerror()) != NULL || !cast) {
		buxton_log("dlsym(): %s\n", error);
		abort();
	}
	memcpy(&d_func, &cast, sizeof(d_func));

	rb = i_func(backend_tmp);
	if (!rb) {
		buxton_log("buxton_module_init failed\n");
		abort();
	}

	if (!config->backends) {
		config->backends = hashmap_new(trivial_hash_func, trivial_compare_func);
		if (!config->backends) {
			abort();
		}
	}

	r = hashmap_put(config->backends, name, backend_tmp);
	if (r != 1) {
		abort();
	}

	backend_tmp->module = handle;
	backend_tmp->destroy = d_func;

	*backend = backend_tmp;
}

BuxtonBackend *backend_for_layer(BuxtonConfig *config,
				 BuxtonLayer *layer)
{
	BuxtonBackend *backend;
	int ret;

	assert(layer);

	if (!config->databases) {
		config->databases = hashmap_new(string_hash_func, string_compare_func);
		if (!config->databases) {
			abort();
		}
	}
	if ((backend = (BuxtonBackend*)hashmap_get(config->databases, layer->name.value)) == NULL) {
		/* attempt load of backend */
		init_backend(config, layer, &backend);

		ret = hashmap_put(config->databases, layer->name.value, backend);
		if (ret != 1) {
			abort();
		}
	}
	return (BuxtonBackend*)hashmap_get(config->databases, layer->name.value);
}

void destroy_backend(BuxtonBackend *backend)
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
