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
#include "util.h"

pid_t daemon_pid;

static void setup()
{
	daemon_pid = 0;
}

static void teardown()
{
	/* if the daemon is still running, kill it */
	if (daemon_pid) {
		kill(SIGTERM, daemon_pid);
		usleep(500*1000);
		kill(SIGKILL, daemon_pid);
	}
}

START_TEST(daemon_start_check)
{
	const char magic[] = "!";
	char buf[2];
	sigset_t sigset;
	pid_t pid;
	int socks[2];

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
	
	fail_if(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, socks),
		"socketpair: %m");
	pid = fork();
	fail_if(pid < 0, "couldn't fork");
	if (pid) {
		/* parent*/
		int status;

		close(socks[0]);
		daemon_pid = pid;
		
		fail_if(write(socks[1], (void*)magic, 1) < 0,
			"can't write to child: %m");
		fail_if(read(socks[1], (void*)buf, 1) < 0,
			"couldn't read from child: %m");

		usleep(250*1000);
		pid_t waited = waitpid(daemon_pid, &status, WNOHANG);
		if (daemon_pid == waited) {
			/* the daemon exited already */
			fail_if(WIFSIGNALED(status), "daemon was killed");
			fail_if(WIFEXITED(status), "daemon exited");
			fail("the daemon exited somewhere, but we don't know how");
		}
	} else {
		/* child */
		char path[PATH_MAX];

		close(socks[1]);

		fail_if(read(socks[0], (void*)buf, 1) < 0,
			"couldn't read from parent: %m");
		
		//FIXME: path is wrong for makedistcheck
		snprintf(path, PATH_MAX, "%s/bt-daemon", get_current_dir_name());
		fprintf(stderr, "path is %s\n", path);

		fail_if(write(socks[0], (void*)magic, 1) < 0,
			"can't write to parent: %m");
		if (execl(path, "bt-daemon", (const char*)NULL) < 0) {
			fail("couldn't exec: %m");
		}
		fail("should never reach here");
	}
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
	tcase_add_test(tc, daemon_start_check);

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
