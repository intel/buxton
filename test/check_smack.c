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
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "buxton.h"
#include "configurator.h"
#include "check_utils.h"
#include "daemon.h"
#include "log.h"
#include "smack.h"
#include "util.h"

#ifdef NDEBUG
	#error "re-run configure with --enable-debug"
#endif

static pid_t daemon_pid;

static void exec_daemon(void)
{
	char path[PATH_MAX];

	//FIXME: path is wrong for makedistcheck
	snprintf(path, PATH_MAX, "%s/check_buxtond", get_current_dir_name());

	if (execl(path, "check_buxtond", (const char*)NULL) < 0) {
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
		} else {
			/* if the daemon is still running, kill it */
			kill(SIGTERM, daemon_pid);
			usleep(64*1000);
			kill(SIGKILL, daemon_pid);
		}
	}
}

START_TEST(smack_access_check)
{
	bool ret;
	BuxtonString subject;
	BuxtonString object;

	ret = buxton_cache_smack_rules();
	fail_if(!ret, "Failed to cache Smack rules");

	subject = buxton_string_pack("system");
	object = buxton_string_pack("base/sample/key");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_READ);
	fail_if(!ret, "Read access was denied, but should have been granted");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_WRITE);
	fail_if(ret, "Write access was granted, but should have been denied");

	subject = buxton_string_pack("system");
	object = buxton_string_pack("system/sample/key");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_READ);
	fail_if(!ret, "Read access was denied");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_WRITE);
	fail_if(!ret, "Write access was denied");

	subject = buxton_string_pack("*");
	object = buxton_string_pack("foo");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_READ);
	fail_if(ret, "Read access granted for * subject");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_WRITE);
	fail_if(ret, "Write access granted for * subject");

	subject = buxton_string_pack("foo");
	object = buxton_string_pack("@");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_READ);
	fail_if(!ret, "Read access denied for @ object");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_WRITE);
	fail_if(!ret, "Write access denied for @ object");

	subject = buxton_string_pack("@");
	object = buxton_string_pack("foo");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_READ);
	fail_if(!ret, "Read access denied for @ subject");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_WRITE);
	fail_if(!ret, "Write access denied for @ subject");

	subject = buxton_string_pack("foo");
	object = buxton_string_pack("*");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_READ);
	fail_if(!ret, "Read access denied for * object");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_WRITE);
	fail_if(!ret, "Write access denied for * object");

	subject = buxton_string_pack("foo");
	object = buxton_string_pack("foo");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_READ);
	fail_if(!ret, "Read access denied for matching subject/object");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_WRITE);
	fail_if(!ret, "Write access denied for matching subject/object");

	subject = buxton_string_pack("foo");
	object = buxton_string_pack("_");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_READ);
	fail_if(!ret, "Read access denied for _ object");

	subject = buxton_string_pack("^");
	object = buxton_string_pack("foo");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_READ);
	fail_if(!ret, "Read access denied for ^ subject");

	subject = buxton_string_pack("subjecttest");
	object = buxton_string_pack("objecttest");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_READ);
	fail_if(ret, "Read access granted for unrecognized subject/object");

	subject = buxton_string_pack("subjecttest");
	object = buxton_string_pack("objecttest");
	ret = buxton_check_smack_access(&subject, &object, ACCESS_WRITE);
	fail_if(ret, "Write access granted for unrecognized subject/object");
}
END_TEST

static Suite *
daemon_suite(void)
{
	Suite *s;
	TCase *tc;
	bool __attribute__((unused))dummy;

	s = suite_create("smack");

	dummy = buxton_cache_smack_rules();
	if (buxton_smack_enabled()) {
		tc = tcase_create("smack access test functions");
		tcase_add_checked_fixture(tc, setup, teardown);
		/* TODO: add tests that use actual client Smack labels */
		suite_add_tcase(s, tc);

		tc = tcase_create("smack libsecurity functions");
		tcase_add_test(tc, smack_access_check);
		suite_add_tcase(s, tc);
	} else {
		buxton_log("Smack support not detected; skipping this test suite\n");
	}

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	putenv("BUXTON_CONF_FILE=" ABS_TOP_BUILDDIR "/test/test.conf");
	s = daemon_suite();
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
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
