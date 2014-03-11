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
#include <string.h>

#include "buxton.h"

void get_cb(BuxtonResponse response, void *data)
{
	int32_t* ret = (int32_t*)data;

	if (buxton_response_status(response) != 0) {
		printf("Failed to get value\n");
		return;
	}

	*ret = *(int32_t*)buxton_response_value(response);
}

int main(void)
{
	BuxtonClient client;
	BuxtonKey key;
	struct pollfd pfd[1];
	int r;
	int32_t gvalue = -1;
	int fd;

	if ((fd = buxton_open(&client)) < 0) {
		printf("couldn't connect\n");
		return -1;
	}

	key = buxton_key_create("hello", "test", "user", INT32);
	if (!key) {
		return -1;
	}

	if (buxton_get_value(client, key, get_cb,
			     &gvalue, false)) {
		printf("get call failed to run\n");
		return -1;
	}

	pfd[0].fd = fd;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;
	r = poll(pfd, 1, 5000);

	if (r <= 0) {
		printf("poll error\n");
		return -1;
	}

	if (!buxton_client_handle_response(client)) {
		printf("bad response from daemon\n");
		return -1;
	}

	if (gvalue >= 0) {
		printf("got value: %d\n", gvalue);
	}

	buxton_key_free(key);
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
