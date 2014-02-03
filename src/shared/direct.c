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

#include <string.h>
#include <stdlib.h>

#include "direct.h"
#include "log.h"
#include "smack.h"
#include "util.h"

bool buxton_direct_open(BuxtonControl *control)
{

	assert(control);

	memzero(&(control->config), sizeof(BuxtonConfig));
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

	/* FIXME: should check if client has CAP_MAC_ADMIN instead */
	if (control->client.uid != 0)
		return false;

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
		r = buxton_direct_get_value_for_layer(control, layer_name, key, &data, NULL);
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
