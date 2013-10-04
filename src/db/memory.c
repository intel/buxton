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

#include "../shared/hashmap.h"
#include "../shared/log.h"
#include "../include/bt-daemon.h"
#include "../include/bt-daemon-private.h"

/**
 * Memory Database Module
 *
 * Used for quick testing and debugging of Buxton, to ensure protocol
 * and direct access are working as intended.
 * Note this is not persistent.
 */


static Hashmap *_resources;

static int set_value(const char *resource, const char *key, BuxtonData *data) {
	return false;
}

static BuxtonData* get_value(const char *resource, const char *key) {
	return NULL;
}

_bx_export_ void buxton_module_destroy(BuxtonBackend *backend) {
	backend->set_value = NULL;
	backend->get_value = NULL;

}

_bx_export_ int buxton_module_init(BuxtonBackend *backend) {
	/* Point the struct methods back to our own */
	backend->set_value = &set_value;
	backend->set_value = &get_value;

	return 0;
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
