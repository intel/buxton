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

#include "buxton.h"

void notify_cb(BuxtonResponse response, void *data)
{
	bool *status = (bool *)data;
	BuxtonKey key;
	int32_t value;

	key = response_key(response);
	if (response_value(response) == NULL) {
		*status = false;
		return;
	}

	value = *(int32_t*)response_value(response);
	printf("key %s updated with new value %d\n", buxton_get_name(key),
		value);
}

int main(void)
{
	BuxtonClient client;
	BuxtonKey key;
	bool status = true;
	struct pollfd pfd[1];
	int r;
	int fd;

	if ((fd = buxton_client_open(&client)) < 0) {
		printf("couldn't connect\n");
		return -1;
	}

	key = buxton_make_key("hello", "test", NULL, INT32);
	if (!key)
		return -1;

	if (!buxton_client_register_notification(client, key, notify_cb, &status, false)) {
		printf("set call failed to run\n");
		return -1;
	}
repoll:
	pfd[0].fd = fd;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;
	r = poll(pfd, 1, 5000);

	if (r < 0) {
		printf("poll error\n");
		return -1;
	} else if (r == 0) {
		goto repoll;
	}

	if (!buxton_client_handle_response(client)) {
		printf("bad response from daemon\n");
		return -1;
	}

	if (!status) {
		printf("Failed to register for notification\n");
		return -1;
	}

	goto repoll;

	buxton_free_key(key);
	buxton_client_close(client);

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
