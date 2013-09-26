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

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>

#include "../shared/list.h"
#include "bt-daemon.h"

#ifndef btdaemonh_private
#define btdaemonh_private

#define TIMEOUT 5000

typedef struct client_list_item {
	LIST_FIELDS(struct client_list_item, item);

	int client_socket;

	struct ucred credentials;
} client_list_item;

/* Module related code */
typedef int (*get_string_func) (const char *key, char **out_value);
typedef int (*set_string_func) (const char *key, const char *value);

typedef struct BuxtonBackend {
	void *module;

	set_string_func set_string;
	get_string_func get_string;

} BuxtonBackend;

typedef int (*module_init_func) (const char *resource, BuxtonBackend *backend);
typedef void (*module_destroy_func) (BuxtonBackend *backend);

/* Initialise a backend module */
bool init_backend(const char *name, BuxtonBackend *backend);

#endif /* btdaemonh_private */

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
