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

#include <stdio.h>

#include "config.h"
#include "../shared/util.h"
#include "../include/bt-daemon-private.h"

size_t page_size(void) {
        static __thread size_t pgsz = 0;
        long r;

        if (_likely_(pgsz > 0))
                return pgsz;

        r = sysconf(_SC_PAGESIZE);
        assert(r > 0);

        pgsz = (size_t) r;
        return pgsz;
}

char* get_layer_path(BuxtonLayer *layer)
{
	char *path = NULL;
	int r;
	char uid[15];

	switch (layer->type) {
		case LAYER_SYSTEM:
			r = asprintf(&path, "%s/%s.db", DB_PATH, layer->name);
			if (r == -1)
				return NULL;
			break;
		case LAYER_USER:
			/* uid must already be set in layer before calling */
			sprintf(uid, "%d", (int)layer->uid);
			r = asprintf(&path, "%s/user-%s.db", DB_PATH, uid);
			if (r == -1)
				return NULL;
			break;
		default:
			break;
	}

	return path;
}

void buxton_data_copy(BuxtonData* original, BuxtonData *copy)
{
	copy->type = original->type;
	BuxtonDataStore store;
	switch (original->type) {
		case STRING:
			store.d_string = strdup(original->store.d_string);
			break;
		case BOOLEAN:
			store.d_boolean = original->store.d_boolean;
			break;
		case FLOAT:
			store.d_float = original->store.d_float;
			break;
		case INT:
			store.d_int = original->store.d_int;
			break;
		case DOUBLE:
			store.d_double = original->store.d_double;
			break;
		case LONG:
			store.d_long = original->store.d_long;
			break;
		default:
			break;
	}
	copy->store = store;
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
