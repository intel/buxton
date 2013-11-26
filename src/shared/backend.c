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
