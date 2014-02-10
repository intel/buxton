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

bool buxton_direct_get_value(BuxtonControl *control, _BuxtonKey *key,
			     BuxtonData *data, BuxtonString *data_label,
			     BuxtonString *client_label)
{
	/* Handle direct manipulation */
	BuxtonLayer *l;
	BuxtonConfig *config;
	BuxtonString layer = (BuxtonString){ NULL, 0 };
	Iterator i;
	BuxtonData d;
	int priority = 0;
	int r;
	BuxtonLayerType layer_origin = -1;

	assert(control);
	assert(key);

	if (key->layer.value)
		return buxton_direct_get_value_for_layer(control, key, data,
							 data_label,
							 client_label);

	config = &control->config;

	HASHMAP_FOREACH(l, config->layers, i) {
		key->layer.value = l->name.value;
		key->layer.length = l->name.length;
		r = buxton_direct_get_value_for_layer(control,
						      key,
						      &d,
						      data_label,
						      client_label);
		if (r) {
			free(data_label->value);
			data_label->value = NULL;
			data_label->length = 0;
			if (d.type == STRING)
				free(d.store.d_string.value);

			if ((l->type == LAYER_SYSTEM && (layer_origin != LAYER_SYSTEM ||
							 priority <= l->priority)) ||
			    (l->type == LAYER_USER && layer_origin != LAYER_SYSTEM &&
			     priority <= l->priority)) {
				priority = l->priority;
				layer.value = l->name.value;
				layer.length = l->name.length;
			}
		}
	}
	if (layer.value) {
		key->layer.value = layer.value;
		key->layer.length = layer.length;
		r = buxton_direct_get_value_for_layer(control,
						      key,
						      data,
						      data_label,
						      client_label);
		key->layer.value = NULL;
		key->layer.length = 0;
		return r;
	}
	return false;
}

bool buxton_direct_get_value_for_layer(BuxtonControl *control,
				       _BuxtonKey *key,
				       BuxtonData *data,
				       BuxtonString *data_label,
				       BuxtonString *client_label)
{
	/* Handle direct manipulation */
	BuxtonBackend *backend = NULL;
	BuxtonLayer *layer = NULL;
	BuxtonConfig *config;
	BuxtonData g;
	_BuxtonKey group;
	BuxtonString group_label;

	assert(control);
	assert(key);
	assert(data_label);

	if (!key->layer.value)
		return false;

	config = &control->config;
	if ((layer = hashmap_get(config->layers, key->layer.value)) == NULL) {
		return false;
	}
	backend = backend_for_layer(config, layer);
	if (!backend) {
		/* Already logged */
		return false;
	}
	layer->uid = control->client.uid;

	/* The group checks are only needed for key lookups, or we recurse endlessly */
	if (key->name.value && client_label) {
		if (!buxton_copy_key_group(key, &group))
			goto fail;

		/* Groups must be created first, so bail if this key's group doesn't exist */
		if (!buxton_direct_get_value_for_layer(control, &group, &g, &group_label, NULL)) {
			buxton_debug("Group %s for key %s does not exist\n", key->group.value, key->name.value);
			goto fail;
		}

		if (!buxton_check_smack_access(client_label, &group_label, ACCESS_READ))
			goto fail;
	}

	if (backend->get_value(layer, key, data, data_label)) {
		/* Access checks are not needed for direct clients, where client_label is NULL */
		if (data_label->value && client_label && client_label->value &&
		    !buxton_check_read_access(control, key, data_label,
					      client_label)) {
			/* Client lacks permission to read the value */
			free(data_label->value);
			return false;
		}
		buxton_debug("SMACK check succeeded for get_value for layer %s\n", key->layer.value);
		return true;
	} else {
		return false;
	}

fail:
	return false;
}

bool buxton_direct_set_value(BuxtonControl *control,
			     _BuxtonKey *key,
			     BuxtonData *data,
			     BuxtonString *label)
{
	BuxtonBackend *backend;
	BuxtonLayer *layer;
	BuxtonConfig *config;
	BuxtonString default_label = buxton_string_pack("_");
	BuxtonString *l;
	_cleanup_buxton_data_ BuxtonData *d = NULL;
	_cleanup_buxton_data_ BuxtonData *g = NULL;
	_cleanup_buxton_key_ _BuxtonKey *group = NULL;
	_cleanup_buxton_string_ BuxtonString *data_label = NULL;
	_cleanup_buxton_string_ BuxtonString *group_label = NULL;
	bool r;

	assert(control);
	assert(key);
	assert(data);

	group = malloc0(sizeof(_BuxtonKey));
	if (!group)
		goto fail;
	g = malloc0(sizeof(BuxtonData));
	if (!g)
		goto fail;
	group_label = malloc0(sizeof(BuxtonString));
	if (!group_label)
		goto fail;

	d = malloc0(sizeof(BuxtonData));
	if (!d)
		goto fail;
	data_label = malloc0(sizeof(BuxtonString));
	if (!data_label)
		goto fail;

	/* Groups must be created first, so bail if this key's group doesn't exist */
	if (!buxton_copy_key_group(key, group))
		goto fail;

	if (!buxton_direct_get_value_for_layer(control, group, g, group_label, NULL)) {
		buxton_debug("Group %s for key %s does not exist\n", key->group.value, key->name.value);
		goto fail;
	}

	/* Access checks are not needed for direct clients, where label is NULL */
	if (label) {
		if (!buxton_check_smack_access(label, group_label, ACCESS_WRITE))
			goto fail;
		if (buxton_direct_get_value_for_layer(control, key, d, data_label, NULL)) {
			if (!buxton_check_smack_access(label, data_label, ACCESS_WRITE)) {
				goto fail;
			}
			l = data_label;
		} else {
			l = label;
		}
	} else {
		if (buxton_direct_get_value_for_layer(control, key, d, data_label, NULL)) {
			l = data_label;
		} else {
			l = &default_label;
		}
	}

	config = &control->config;
	if ((layer = hashmap_get(config->layers, key->layer.value)) == NULL)
		return false;

	backend = backend_for_layer(config, layer);
	if (!backend) {
		/* Already logged */
		return false;
	}

	layer->uid = control->client.uid;
	r = backend->set_value(layer, key, data, l);

	return r;

fail:
	return false;
}

bool buxton_direct_set_label(BuxtonControl *control,
			     _BuxtonKey *key,
			     BuxtonString *label)
{
	BuxtonBackend *backend;
	BuxtonData data;
	BuxtonLayer *layer;
	BuxtonConfig *config;
	BuxtonString data_label = (BuxtonString){ NULL, 0 };
	bool r;

	assert(control);
	assert(key);
	assert(label);

	/* FIXME: should check if client has CAP_MAC_ADMIN instead */
	if (control->client.uid != 0)
		return false;

	config = &control->config;

	if ((layer = hashmap_get(config->layers, key->layer.value)) == NULL) {
		return false;
	}
	backend = backend_for_layer(config, layer);
	if (!backend) {
		/* Already logged */
		return false;
	}

	char *name = get_name(key);
	if (name) {
		r = buxton_direct_get_value_for_layer(control, key, &data, &data_label, NULL);
		if (!r)
			return false;

		free(data_label.value);
	} else {
		/* we have a group, so initialize the data with a dummy value */
		data.type = STRING;
		data.store.d_string = buxton_string_pack("BUXTON_GROUP_VALUE");
	}

	data_label.length = label->length;
	data_label.value = label->value;

	r = backend->set_value(layer, key, &data, &data_label);
	if (name && data.type == STRING)
		free(data.store.d_string.value);
	free(name);

	return r;
}

bool buxton_direct_list_keys(BuxtonControl *control,
			     BuxtonString *layer_name,
			     BuxtonArray **list)
{
	assert(control);
	assert(layer_name);

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
			       _BuxtonKey *key,
			       BuxtonString *label)
{
	BuxtonBackend *backend;
	BuxtonLayer *layer;
	BuxtonConfig *config;
	BuxtonString data_label = (BuxtonString){ NULL, 0 };

	assert(control);
	assert(key);

	/* Access checks are not needed for direct clients, where label is NULL */
	if (label) {
		if (!buxton_check_write_access(control, key, &data_label, label))
			return false;
		free(data_label.value);
	}

	config = &control->config;
	if ((layer = hashmap_get(config->layers, key->layer.value)) == NULL)
		return false;

	backend = backend_for_layer(config, layer);
	if (!backend) {
		/* Already logged */
		return false;
	}
	layer->uid = control->client.uid;
	return backend->unset_value(layer, key, NULL, NULL);
}

void buxton_direct_close(BuxtonControl *control)
{
	Iterator iterator;
	BuxtonBackend *backend;
	BuxtonLayer *layer;
	BuxtonString *key;

	control->client.direct = false;

	HASHMAP_FOREACH(backend, control->config.backends, iterator) {
		destroy_backend(backend);
	}
	hashmap_free(control->config.backends);
	hashmap_free(control->config.databases);

	HASHMAP_FOREACH_KEY(layer, key, control->config.layers, iterator) {
		hashmap_remove(control->config.layers, key);
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
