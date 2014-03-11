/*
 * This file is part of buxton.
 *
 * Copyright (C) 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#define _GNU_SOURCE
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>

#include "buxton.h"

void set_cb(BuxtonResponse response, void *data)
{
	bool *status = (bool *)data;
	if (buxton_response_status(response) != 0) {
		*status = false;
	} else {
		*status = true;
	}
}

void notify_cb(BuxtonResponse response, void *data)
{
	bool *status = (bool *)data;
	BuxtonKey key;
	int32_t *value;
	char *name;

	if (buxton_response_status(response) != 0) {
		*status = false;
		return;
	}

	key = buxton_response_key(response);
	name = buxton_key_get_name(key);

	value = (int32_t*)buxton_response_value(response);
	if (value) {
		printf("key %s updated with new value %d\n", name, *value);
	} else {
		printf("key %s was removed\n", name);
	}

	buxton_key_free(key);
	free(value);
	free(name);
}

int main(void)
{
	BuxtonClient client;
	BuxtonKey key1, key2, key3, key4;
	bool status = true;
	struct pollfd pfd[1];
	int r;
	int fd;
	int do_update = 0;
	int32_t val = 10;

	if ((fd = buxton_open(&client)) < 0) {
		printf("couldn't connect\n");
		return -1;
	}

	key1 = buxton_key_create("hello", "test1", NULL, INT32);
	if (!key1) {
		return -1;
	}

	key2 = buxton_key_create("hello", "test2", NULL, INT32);
	if (!key2) {
		return -1;
	}

	key3 = buxton_key_create("hello", "test3", NULL, INT32);
	if (!key3) {
		return -1;
	}

	key4 = buxton_key_create("hello", "test1", "user", INT32);
	if (!key4) {
		return -1;
	}

	if (buxton_register_notification(client, key1, notify_cb, &status, false)) {
		printf("set call failed to run\n");
		return -1;
	}

	if (buxton_register_notification(client, key2, notify_cb, &status, false)) {
		printf("set call failed to run\n");
		return -1;
	}

	if (buxton_register_notification(client, key3, notify_cb, &status, false)) {
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
		if (do_update >> 1 == 0) {
			val++;
			if (buxton_set_value(client, key4, &val, set_cb, &status, false)) {
				printf("set value failed\n");
				return -1;
			}
		} else {
			do_update++;
		}
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

	buxton_key_free(key1);
	buxton_key_free(key2);
	buxton_key_free(key3);
	buxton_close(client);

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
