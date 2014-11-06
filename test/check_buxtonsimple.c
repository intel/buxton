/*
 * This file is part of buxton.
 *
 * Copyright (C) 2014 Intel Corporation
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
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "buxton.h"
#include "buxtonresponse.h"
#include "buxtonsimple.h"
#include "buxtonsimple-internals.h"
#include "configurator.h"
#include "util.h"
#ifdef NDEBUG
#error "re-run configure with --enable-debug"
#endif

#define BUXTON_ROOT_CHECK_ENV "BUXTON_ROOT_CHECK"

/* for forking the daemon in setup */
static pid_t daemon_pid;

/* setup and teardown functions */
static void exec_daemon(void)
{
	char path[PATH_MAX];

	//FIXME: path is wrong for makedistcheck
	snprintf(path, PATH_MAX, "%s/buxtond", get_current_dir_name());

	if (execl(path, "buxtond", (const char*)NULL) < 0) {
		fail("couldn't exec: %m");
	}
	fail("should never reach here");
}

static void setup(void)
{
	daemon_pid = 0;
	sigset_t sigset;
	pid_t pid;

	unlink(buxton_socket());

	//TODO: find out what sigset is
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigprocmask(SIG_BLOCK, &sigset, NULL);

	pid = fork();
	fail_if(pid < 0, "couldn't fork");
	if (pid) {
		/* parent*/
		daemon_pid = pid;
		usleep(128*1000);
	} else {
		/* child */
		exec_daemon();
	}
}

static void teardown(void)
{
	if (daemon_pid) {
		int status;
		pid_t pid;

		pid = waitpid(daemon_pid, &status, WNOHANG);
		fail_if(pid == -1, "waitpid error");
		if (pid) {
			fail("daemon crashed!");
		} else	{
			/* if the daemon is still running, kill it */
			kill(SIGTERM, daemon_pid);
			usleep(64*1000);
			kill(SIGKILL, daemon_pid);
		}
	}
}

/* start libbuxtonsimple test */
START_TEST (sbuxton_set_group_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EBADMSG, "Set group failed");
}
END_TEST

START_TEST (sbuxton_set_int32_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	int32_t int32_val = 5;
	errno = 0;
	sbuxton_set_int32("int32key", int32_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set int32 failed");
}
END_TEST

START_TEST (sbuxton_get_int32_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	int32_t int32_val = 5;
	errno = 0;
	sbuxton_set_int32("int32key", int32_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set int32 failed");
	errno = 0;
	int32_t ret = sbuxton_get_int32("int32key");
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Get int32 failed");
	fail_if(ret != int32_val, "Get int32 returned wrong value");
}
END_TEST

START_TEST (sbuxton_set_string_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	char *string_val = "Testing...";
	errno = 0;
	sbuxton_set_string("stringkey", string_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set string failed");
}
END_TEST

START_TEST (sbuxton_get_string_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	char *string_val = "Testing...";
	errno = 0;
	sbuxton_set_string("stringkey", string_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set string failed");
	errno = 0;
	char* ret = sbuxton_get_string("stringkey");
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Get string failed");
	fail_if(strcmp(ret, string_val), "Get string returned wrong value");
}
END_TEST

START_TEST (sbuxton_set_uint32_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	uint32_t uint32_val = 5;
	errno = 0;
	sbuxton_set_uint32("uint32key", uint32_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set uint32 failed");
}
END_TEST

START_TEST (sbuxton_get_uint32_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	uint32_t uint32_val = 5;
	errno = 0;
	sbuxton_set_uint32("uint32key", uint32_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set uint32 failed");
	errno = 0;
	uint32_t ret = sbuxton_get_uint32("uint32key");
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Get uint32 failed");
	fail_if(ret != uint32_val, "Get uint32 returned wrong value");
}
END_TEST

START_TEST (sbuxton_set_int64_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	int64_t int64_val = 5;
	errno = 0;
	sbuxton_set_int64("int64key", int64_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set int64 failed");
}
END_TEST

START_TEST (sbuxton_get_int64_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	int64_t int64_val = 5;
	errno = 0;
	sbuxton_set_int64("int64key", int64_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set int64 failed");
	errno = 0;
	int64_t ret = sbuxton_get_int64("int64key");
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Get int64 failed");
	fail_if(ret != int64_val, "Get int64 returned wrong value");
}
END_TEST

START_TEST (sbuxton_set_uint64_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	uint64_t uint64_val = 5;
	errno = 0;
	sbuxton_set_uint64("uint64key", uint64_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set uint64 failed");
}
END_TEST

START_TEST (sbuxton_get_uint64_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	uint64_t uint64_val = 5;
	errno = 0;
	sbuxton_set_uint64("uint64key", uint64_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set uint64 failed");
	errno = 0;
	uint64_t ret = sbuxton_get_uint64("uint64key");
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Get uint64 failed");
	fail_if(ret != uint64_val, "Get uint64 returned wrong value");
}
END_TEST

START_TEST (sbuxton_set_float_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	float float_val = 5.5;
	errno = 0;
	sbuxton_set_float("floatkey", float_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set float failed");
}
END_TEST

START_TEST (sbuxton_get_float_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	float float_val = 5.5;
	errno = 0;
	sbuxton_set_float("floatkey", float_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set float failed");
	errno = 0;
	float ret = sbuxton_get_float("floatkey");
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Get float failed");
	fail_if(ret != float_val, "Get float returned wrong value");
}
END_TEST

START_TEST (sbuxton_set_double_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	double double_val = 5;
	errno = 0;
	sbuxton_set_double("double", double_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set double failed");
}
END_TEST

START_TEST (sbuxton_get_double_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	double double_val = 5;
	errno = 0;
	sbuxton_set_double("doublekey", double_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set double failed");
	errno = 0;
	double ret = sbuxton_get_double("doublekey");
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Get double failed");
	fail_if(ret != double_val, "Get double returned wrong value");
}
END_TEST

START_TEST (sbuxton_set_bool_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	bool bool_val = false;
	errno = 0;
	sbuxton_set_bool("boolkey", bool_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set bool failed");
}
END_TEST

START_TEST (sbuxton_get_bool_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	bool bool_val = false;
	errno = 0;
	sbuxton_set_bool("boolkey", bool_val);
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Set bool failed");
	errno = 0;
	bool ret = sbuxton_get_bool("boolkey");
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Get bool failed");
	fail_if(ret != bool_val, "Get bool returned wrong value");
}
END_TEST

START_TEST (sbuxton_remove_group_check)
{
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	sbuxton_remove_group("tg_s0", "user");
	fail_if(errno == ENOTCONN, "Connection failed");
	fail_if(errno == EACCES, "Removal Failed");
	//wait to do this test until michelle puts errno in this func
}
END_TEST

/* Start buxtonsimple-internal tests */
START_TEST (client_connection_check)
{
	int ret;
	ret = _client_connection();
	fail_if(!ret, "Client connection failed- returned 0");
	fail_if(client == NULL, "could not open client connection");
}
END_TEST

START_TEST (client_disconnect_check)
{
	int ret;
	ret = _client_connection();
	fail_if(!ret, "Client connection failed- returned 0");
	fail_if(client == NULL, "could not open client connection");
	_client_disconnect();
	fail_if(client != NULL, "could not close client connection");
}
END_TEST

START_TEST (cg_cb_check)
{
	_BuxtonResponse resp;
	resp.data = NULL;
	resp.type = BUXTON_CONTROL_CHANGED;
	resp.key = NULL;

	int data = 7;

	_cg_cb(&resp, &data);
}
END_TEST

START_TEST (bs_print_check)
{
	vstatus *data = malloc(sizeof(vstatus));
	fail_if(!data, "Failed to allocate space for vstatus");
	data->status = 1;
	data->type = BUXTON_TYPE_STRING;
	data->val.sval = "test";

	BuxtonKey key = buxton_key_create("tg_s0", "keyname", "user", BUXTON_TYPE_STRING);
	_BuxtonResponse resp;
	resp.data = NULL;
	resp.type = BUXTON_CONTROL_GET;
	BuxtonControlMessage bcm = buxton_response_type(&resp);
	fail_if(bcm !=BUXTON_CONTROL_GET, "Response type incorrect");
	resp.key = key;

	_bs_print(data, &resp);
	free(data);
}
END_TEST

START_TEST (bs_cb_check)
{
	vstatus data;
	data.status = 1;
	data.type = BUXTON_TYPE_STRING;
	data.val.sval = "test";

	BuxtonKey key = buxton_key_create("tg_s0", "keyname", "user", BUXTON_TYPE_STRING);
	_BuxtonResponse resp;
	resp.data = NULL;
	resp.type = BUXTON_CONTROL_GET;
	BuxtonControlMessage bcm = buxton_response_type(&resp);
	fail_if(bcm !=BUXTON_CONTROL_GET, "Response type incorrect");
	resp.key = key;

	_bs_cb(&resp, &data);
}
END_TEST

START_TEST (bg_cb_check)
{
	vstatus data;
	data.type = BUXTON_TYPE_STRING;
	BuxtonKey key = buxton_key_create("tg_s0", "keyname", "user", BUXTON_TYPE_STRING);
	BuxtonData bd;
	bd.type = BUXTON_TYPE_STRING;
	bd.store.d_string = buxton_string_pack("test");
	BuxtonArray *a = buxton_array_new();
	fail_if(!buxton_array_add(a, &bd), "Unable to add element to array");
	_BuxtonResponse resp;
	resp.data = a;
	fail_if (!buxton_array_get(resp.data, 0), "No array in resp.data");
	resp.type = BUXTON_CONTROL_CHANGED;
	BuxtonControlMessage bcm = buxton_response_type(&resp);
	fail_if(bcm !=BUXTON_CONTROL_CHANGED, "Response type incorrect");
	resp.key = key;
	_bg_cb(&resp, &data);
}
END_TEST

START_TEST (buxton_group_create_check)
{
	BuxtonKey key = _buxton_group_create("tg_s0", "user");
	fail_if(!key, "Failed to create group key");
}
END_TEST

START_TEST (rg_cb_check)
{
	int stat = 0;
	_BuxtonResponse resp;
	resp.data = NULL;
	resp.type = BUXTON_CONTROL_CHANGED;
	resp.key = NULL;

	_rg_cb(&resp, &stat);
}
END_TEST

static Suite *
buxtonsimp_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("buxtonsimple");

	/* run in this order so group is set up for all sets and gets */
	tc = tcase_create("buxtonsimple_public");
	tcase_add_unchecked_fixture(tc, setup, teardown);
	tcase_add_test(tc, sbuxton_set_group_check);
	tcase_add_test(tc, sbuxton_set_int32_check);
	tcase_add_test(tc, sbuxton_get_int32_check);
	tcase_add_test(tc, sbuxton_set_string_check);
	tcase_add_test(tc, sbuxton_get_string_check);
	tcase_add_test(tc, sbuxton_set_uint32_check);
	tcase_add_test(tc, sbuxton_get_uint32_check);
	tcase_add_test(tc, sbuxton_set_int64_check);
	tcase_add_test(tc, sbuxton_get_int64_check);
	tcase_add_test(tc, sbuxton_set_uint64_check);
	tcase_add_test(tc, sbuxton_get_uint64_check);
	tcase_add_test(tc, sbuxton_set_float_check);
	tcase_add_test(tc, sbuxton_get_float_check);
	tcase_add_test(tc, sbuxton_set_double_check);
	tcase_add_test(tc, sbuxton_get_double_check);
	tcase_add_test(tc, sbuxton_set_bool_check);
	tcase_add_test(tc, sbuxton_get_bool_check);
	tcase_add_test(tc, sbuxton_remove_group_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("buxtonsimple_internal");
	tcase_add_test(tc, client_connection_check);
	tcase_add_test(tc, client_disconnect_check);
	tcase_add_test(tc, cg_cb_check);
	tcase_add_test(tc, bs_print_check);
	tcase_add_test(tc, bs_cb_check);
	tcase_add_test(tc, bg_cb_check);
	tcase_add_test(tc, buxton_group_create_check);
	tcase_add_test(tc, rg_cb_check);
	suite_add_tcase(s, tc);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	putenv("BUXTON_CONF_FILE=" ABS_TOP_BUILDDIR "/test/test.conf");
	putenv("BUXTON_ROOT_CHECK=0");
	s = buxtonsimp_suite();
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
