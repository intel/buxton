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

#include <gdbm.h>

#include "../shared/log.h"
#include "../include/bt-daemon.h"
#include "../include/bt-daemon-private.h"


/**
 * GDBM Database Module
 */


static const char *_resource = NULL;
static GDBM_FILE _database = NULL;

static int set_string(const char *key, const char *value) {
	return false;
}

static int get_string(const char *key, char **out_value) {
	return false;
}

_bx_export_ void buxton_module_destroy(BuxtonBackend *backend) {
	backend->set_string = NULL;
	backend->get_string = NULL;

	_resource = NULL;
	/* TODO: We'd close the GDBM here */
	_database = NULL;
}

_bx_export_ int buxton_module_init(const char *resource, BuxtonBackend *backend) {
	/* TODO: Initialise database */

	/* Point the struct methods back to our own */
	backend->set_string = &set_string;
	backend->get_string = &get_string;

	_resource = resource;

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
