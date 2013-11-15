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
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "bt-daemon.h"
#include "daemon.h"
#include "log.h"
#include "util.h"

static pid_t daemon_pid;

typedef struct _fuzz_context_t {
		uint8_t buf[4096];
		size_t size;
		int iteration;
} FuzzContext;

static char* dump_fuzz(FuzzContext *fuzz)
{
	char* buf;
	size_t buff_size;
	FILE* s;
	int l = 0;
	int c = 0;

	s = open_memstream(&buf, &buff_size);

	fprintf(s, "\n\n******************************************\n");
	fprintf(s, "current time %ld\n", time(NULL));
	fprintf(s, "iteration: %d\tsize: %ld\n", fuzz->iteration, fuzz->size);
	for (int i = 0; i < fuzz->size; i++) {
		fprintf(s, "%02X ", fuzz->buf[i]);
		c+= 3;
		if (c > 80) {
			fprintf(s, "\n");
			c = 0;
			l++;
		}
	}
	fclose(s);

	return buf;
}

static void check_did_not_crash(pid_t pid, FuzzContext *fuzz)
{
	pid_t rpid;
	int status;

	rpid = waitpid(pid, &status, WNOHANG);
	fail_if(rpid == -1, "couldn't wait for pid %m");
	if (rpid == 0) {
		 /* child is still running */
		return;
	}
	fail_if(WIFEXITED(status), "daemon exited with status %d%s",
		WEXITSTATUS(status), dump_fuzz(fuzz));
	fail_if(WIFSIGNALED(status), "daemon was killed with signal %d%s",
		WTERMSIG(status), dump_fuzz(fuzz));
}

static void exec_daemon(void)
{
		char path[PATH_MAX];

		//FIXME: path is wrong for makedistcheck
		snprintf(path, PATH_MAX, "%s/check_bt_daemon", get_current_dir_name());

		if (execl(path, "check_bt_daemon", (const char*)NULL) < 0) {
			fail("couldn't exec: %m");
		}
		fail("should never reach here");
}

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
		} else  {
			/* if the daemon is still running, kill it */
			kill(SIGTERM, daemon_pid);
			usleep(64*1000);
			kill(SIGKILL, daemon_pid);
		}
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
	data.label = buxton_string_pack("label");
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
	fail_if(strcmp(result.label.value, "label") != 0,
		"Buxton gdbm returned a different label to that set.");
	fail_if(strcmp(result.store.d_string.value, "bxt_test_value") != 0,
		"Buxton gdbm returned a different value to that set.");
	if (result.label.value)
		free(result.label.value);
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
	data.label = buxton_string_pack("label2");
	data.store.d_string = buxton_string_pack("bxt_test_value2");
	fail_if(data.store.d_string.value == NULL,
		"Failed to allocate test string.");
	fail_if(buxton_client_set_value(&c, &layer, &key, &data) == false,
		"Failed to set second value.");
	fail_if(buxton_client_get_value(&c, &key, &result) == false,
		"Retrieving value from buxton gdbm backend failed.");
	fail_if(result.type != STRING,
		"Buxton gdbm backend returned incorrect result type.");
	fail_if(strcmp(result.label.value, "label2") != 0,
		"Buxton gdbm returned a different label to that set.");
	fail_if(strcmp(result.store.d_string.value, "bxt_test_value2") != 0,
		"Buxton gdbm returned a different value to that set.");
	if (result.label.value)
		free(result.label.value);
	if (result.store.d_string.value)
		free(result.store.d_string.value);
}
END_TEST

START_TEST(parse_list_check)
{
	BuxtonData l3[3];
	BuxtonData l2[2];
	BuxtonData l1[1];
	BuxtonString *key = NULL;
	BuxtonString *layer = NULL;
	BuxtonData *value = NULL;

	l1[0].type = INT32;
	fail_if(parse_list(BUXTON_CONTROL_NOTIFY, 1, l1, &key, &layer, &value),
		"Parsed bad notify type");
	l1[0].type = STRING;
	l1[0].store.d_string = buxton_string_pack("s1");
	fail_if(!parse_list(BUXTON_CONTROL_NOTIFY, 1, l1, &key, &layer, &value),
		"Unable to parse valid notify");
	fail_if(!streq(key->value, l1[0].store.d_string.value),
		"Failed to set correct notify key");
	l2[0].type = INT32;
	l2[1].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_GET, 2, l2, &key, &layer, &value),
		"Parsed bad get type 1");
	l2[0].type = STRING;
	l2[1].type = INT32;
	fail_if(parse_list(BUXTON_CONTROL_GET, 2, l2, &key, &layer, &value),
		"Parsed bad get type 2");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[0].store.d_string = buxton_string_pack("s2");
	l2[1].store.d_string = buxton_string_pack("s3");
	fail_if(!parse_list(BUXTON_CONTROL_GET, 2, l2, &key, &layer, &value),
		"Unable to parse valid get 1");
	fail_if(!streq(layer->value, l2[0].store.d_string.value),
		"Failed to set correct get layer 1");
	fail_if(!streq(key->value, l2[1].store.d_string.value),
		"Failed to set correct get key 1");
	fail_if(!parse_list(BUXTON_CONTROL_GET, 1, l2, &key, &layer, &value),
		"Unable to parse valid get 2");
	fail_if(!streq(key->value, l2[0].store.d_string.value),
		"Failed to set correct get key 2");

	l3[0].type = INT32;
	l3[1].type = STRING;
	l3[2].type = FLOAT;
	fail_if(parse_list(BUXTON_CONTROL_SET, 3, l3, &key, &layer, &value),
		"Parsed bad set type 1");
	l3[0].type = STRING;
	l3[1].type = INT32;
	l3[2].type = FLOAT;
	fail_if(parse_list(BUXTON_CONTROL_SET, 3, l3, &key, &layer, &value),
		"Parsed bad set type 2");
	l3[0].type = STRING;
	l3[1].type = STRING;
	l3[2].type = FLOAT;
	l3[0].store.d_string = buxton_string_pack("s4");
	l3[1].store.d_string = buxton_string_pack("s5");
	l3[2].store.d_float = 3.14F;
	fail_if(!parse_list(BUXTON_CONTROL_SET, 3, l3, &key, &layer, &value),
		"Unable to parse valid set 1");
	fail_if(!streq(layer->value, l3[0].store.d_string.value),
		"Failed to set correct set layer 1");
	fail_if(!streq(key->value, l3[1].store.d_string.value),
		"Failed to set correct set key 1");
	fail_if(value->store.d_float != l3[2].store.d_float,
		"Failed to set correct set value 1");
}
END_TEST

START_TEST(bt_daemon_handle_message_check)
{
}
END_TEST

START_TEST(identify_client_check)
{
}
END_TEST

START_TEST(add_pollfd_check)
{
}
END_TEST

START_TEST(del_pollfd_check)
{
}
END_TEST

START_TEST(handle_client_check)
{
}
END_TEST

START_TEST(bt_daemon_eat_garbage_check)
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
	if (pid) {		/* parent*/
		BuxtonClient c;
		FuzzContext fuzz;
		time_t start;
		bool keep_going = true;


		srand(0);
		bzero(&fuzz, sizeof(FuzzContext));

		daemon_pid = pid;
		usleep(250*1000);
		check_did_not_crash(daemon_pid, &fuzz);


		fail_if(time(&start) == -1, "call to time() failed");
		do {
			ssize_t bytes;
			time_t now;

			fail_if(time(&now) == -1, "call to time() failed");
			if (now - start >= 2) {
				keep_going = false;
			}

			fail_if(buxton_client_open(&c) == false,
				"Open failed with daemon%s", dump_fuzz(&fuzz));

			fuzz.size = (unsigned int)rand() % 4096;
			for (int i=0; i < fuzz.size; i++) {
				fuzz.buf[i] = (uint8_t)(rand() % 255);
			}

			bytes = write(c.fd, (void*)(fuzz.buf), fuzz.size);
			fail_if(bytes == -1, "write failed: %m%s", dump_fuzz(&fuzz));
			fail_unless(bytes == fuzz.size, "write was %d instead of %d", bytes, fuzz.size);

			buxton_client_close(&c);
			usleep(1*1000);

			check_did_not_crash(daemon_pid, &fuzz);
			fuzz.iteration++;
		} while (keep_going);
	} else {		/* child */
		exec_daemon();
	}
}
END_TEST

static Suite *
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

	tc = tcase_create("buxton_daemon_functions");
	tcase_add_test(tc, parse_list_check);
	tcase_add_test(tc, bt_daemon_handle_message_check);
	tcase_add_test(tc, identify_client_check);
	tcase_add_test(tc, add_pollfd_check);
	tcase_add_test(tc, del_pollfd_check);
	tcase_add_test(tc, handle_client_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("buxton daemon evil tests");
	tcase_add_checked_fixture(tc, NULL, teardown);
	tcase_add_test(tc, bt_daemon_eat_garbage_check);
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
