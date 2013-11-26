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

#include "backend.h"
#include "bt-daemon.h"
#include "hashmap.h"

/**
 * Eventually this will be dropped
 */
static Hashmap *_directPermitted = NULL;

bool buxton_direct_open(BuxtonControl *control)
{

	assert(control);

	if (!_directPermitted)
		_directPermitted = hashmap_new(trivial_hash_func, trivial_compare_func);

	buxton_init_layers();

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
