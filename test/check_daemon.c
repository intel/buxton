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

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <check.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>

#include "bt-daemon.h"
#include "log.h"
#include "util.h"

pid_t daemon_pid;

static void setup(void)
{
	daemon_pid = 0;
	sigset_t sigset;
	pid_t pid;

	unlink(BUXTON_SOCKET);

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigprocmask(SIG_BLOCK, &sigset, NULL);

	pid = fork();
	fail_if(pid < 0, "couldn't fork");
	if (pid) {
		/* parent*/
		daemon_pid = pid;
		usleep(32*1000);
	} else {
		/* child */
		char path[PATH_MAX];

		//FIXME: path is wrong for makedistcheck
		snprintf(path, PATH_MAX, "%s/check_bt_daemon", get_current_dir_name());

		if (execl(path, "check_bt_daemon", (const char*)NULL) < 0) {
			fail("couldn't exec: %m");
		}
		fail("should never reach here");
	}
}

static void teardown(void)
{
	/* if the daemon is still running, kill it */
	if (daemon_pid) {
		kill(SIGTERM, daemon_pid);
		usleep(500*1000);
		kill(SIGKILL, daemon_pid);
	}
}

START_TEST(buxton_client_open_check)
{
	BuxtonClient c;
	fail_if(buxton_client_open(&c) == false,
		"Connection failed to open with daemon.");
}
END_TEST

START_TEST(buxton_client_set_value_check)
{
	BuxtonClient c;
	BuxtonString layer = buxton_string_pack("test-gdbm");
	BuxtonString key = buxton_string_pack("bxt_test");
	fail_if(buxton_client_open(&c) == false,
		"Open failed with daemon.");
	BuxtonData data;
	data.type = STRING;
	data.store.d_string = buxton_string_pack("bxt_test_value");
	fail_if(buxton_client_set_value(&c, &layer, &key, &data) == false,
		"Setting value in buxton failed.");
}
END_TEST

START_TEST(buxton_client_get_value_for_layer_check)
{
	BuxtonClient c;
	BuxtonString layer = buxton_string_pack("test-gdbm");
	BuxtonString key = buxton_string_pack("bxt_test");
	BuxtonData result;
	fail_if(buxton_client_open(&c) == false,
		"Open failed with daemon.");
	fail_if(buxton_client_get_value_for_layer(&c, &layer, &key, &result) == false,
		"Retrieving value from buxton gdbm backend failed.");
	fail_if(result.type != STRING,
		"Buxton gdbm backend returned incorrect result type.");
	fail_if(strcmp(result.store.d_string.value, "bxt_test_value") != 0,
		"Buxton gdbm returned a different value to that set.");
	if (result.store.d_string.value)
		free(result.store.d_string.value);
}
END_TEST

START_TEST(buxton_client_get_value_check)
{
	BuxtonClient c;
	BuxtonString layer = buxton_string_pack("test-gdbm-user");
	BuxtonString key = buxton_string_pack("bxt_test");
	BuxtonData data, result;
	usleep(250*1000);
	fail_if(buxton_client_open(&c) == false,
		"Open failed with daemon.");

	data.type = STRING;
	data.store.d_string = buxton_string_pack("bxt_test_value2");
	fail_if(data.store.d_string.value == NULL,
		"Failed to allocate test string.");
	fail_if(buxton_client_set_value(&c, &layer, &key, &data) == false,
		"Failed to set second value.");
	fail_if(buxton_client_get_value(&c, &key, &result) == false,
		"Retrieving value from buxton gdbm backend failed.");
	fail_if(result.type != STRING,
		"Buxton gdbm backend returned incorrect result type.");
	fail_if(strcmp(result.store.d_string.value, "bxt_test_value2") != 0,
		"Buxton gdbm returned a different value to that set.");
	if (result.store.d_string.value)
		free(result.store.d_string.value);
}
END_TEST

Suite *
daemon_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("daemon");
	tc = tcase_create("daemon test functions");
	tcase_add_checked_fixture(tc, setup, teardown);
	tcase_add_test(tc, buxton_client_open_check);
	tcase_add_test(tc, buxton_client_set_value_check);
	tcase_add_test(tc, buxton_client_get_value_for_layer_check);
	tcase_add_test(tc, buxton_client_get_value_check);
	suite_add_tcase(s, tc);
	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = daemon_suite();
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
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
