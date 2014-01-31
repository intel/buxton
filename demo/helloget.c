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
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buxton.h"
#include "buxton-array.h"

void get_cb(BuxtonArray *response, void *data)
{
	BuxtonData **value = (BuxtonData **)data;
	BuxtonData *d;

	d = buxton_array_get(response, 0);

	if (d->store.d_int32) {
		printf("Failed to get value\n");
		return;
	}

	*value = malloc(sizeof(BuxtonData *));
	d = buxton_array_get(response, 2);
	(*value)->type = d->type;
	(*value)->store.d_int32 = d->store.d_int32;
}

int main(void)
{
	BuxtonClient client;
	BuxtonData *gvalue = NULL;
	BuxtonString *key;
	BuxtonString layer;
	struct pollfd pfd[1];
	int r;

	if (!buxton_client_open(&client)) {
		printf("couldn't connect\n");
		return -1;
	}

	layer.value = "base";
	layer.length = strlen("base") + 1;
	key = buxton_make_key("hello", "test");
	if (!key)
		return -1;

	if (!buxton_client_get_value_for_layer(&client, &layer, key, get_cb,
					       &gvalue, false)) {
		printf("get call failed to run\n");
		return -1;
	}

	pfd[0].fd = client.fd;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;
	r = poll(pfd, 1, 5000);

	if (r <= 0) {
		printf("poll error\n");
		return -1;
	}

	if (!buxton_client_handle_response(&client)) {
		printf("bad response from daemon\n");
		return -1;
	}

	if (gvalue)
		printf("got value: %d\n", gvalue->store.d_int32);

	return 0;
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
