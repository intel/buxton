/*
 * This file is part of buxton.
 *
 * Copyright (C) 2014 Intel Corporation
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

#include <errno.h>
#include <inttypes.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "buxtonsimple.h"

/*
 * This demo creates a key "tk_i32" on group "tg_s5" and layer "user".
 * It then registers for notification on that key in buxton, and registers
 * for notifications on this instance of buxtonsimple in pollfd. Users can then
 * changed the value of tk_i32 with the commandline interface (buxtonctl), which
 * will trigger this callback, which prints the name of the key, and its new value.
 */

/* Callback function for notifications */
void tk_i32_notify_cb(void *key_data, char *key_name)
{
	int32_t *data = (int32_t *)key_data;
	if (!data) {
		printf("key %s was removed\n", key_name);
	} else {
		printf("key %s was changed to value %d\n", key_name, *data);
	}
}

int main(void)
{
	struct pollfd pfd[1];
	int r;
	int fd;
	int repoll_count = 10;

	/* Create group */
	errno = 0;
	sbuxton_set_group("tg_s5", "user");
	printf("set_group: 'tg_s5', 'user', Error number: %s.\n", strerror(errno));

	/* Test Int setting */
	int32_t i32 = (int32_t) rand() % 50 + 1;
	printf("value should be set to %d.\n", i32);
	errno = 0;
	sbuxton_set_int32("tk_i32", i32);
	printf("set_int32: 'tg_s5', 'tk_i32', Error number: %s.\n", strerror(errno));

	/* Register for notifications in buxton */
	printf("Register for int32_t tk_i32\n");
	sbuxton_register_notify("tk_i32", &tk_i32_notify_cb);

	/* get fd */
	fd = sbuxton_get_fd();


repoll:
	pfd[0].fd = fd;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;
	r = poll(pfd, 1, 5000);

	if (r < 0) {
		printf("poll error\n");
		return -1;
	} else if (r == 0) {
		if (repoll_count-- > 0) {
			goto out;
		}
		goto repoll;
	}

	if (!sbuxton_handle_response()) {
		printf("bad response from daemon\n");
		return -1;
	}

	goto repoll;

out:
	/* unregister notifications */
	sbuxton_unregister_notify("tk_i32");

	return 0;
}
