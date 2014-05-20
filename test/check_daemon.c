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
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "buxton.h"
#include "buxtonresponse.h"
#include "configurator.h"
#include "check_utils.h"
#include "daemon.h"
#include "direct.h"
#include "hashmap.h"
#include "log.h"
#include "smack.h"
#include "util.h"

#ifdef NDEBUG
#error "re-run configure with --enable-debug"
#endif

#define BUXTON_ROOT_CHECK_ENV "BUXTON_ROOT_CHECK"

static pid_t daemon_pid;
int fuzz_time;

typedef struct _fuzz_context_t {
	uint8_t buf[4096];
	size_t size;
	int iteration;
} FuzzContext;

static bool use_smack(void)
{
	bool __attribute__((unused))dummy;

	dummy = buxton_cache_smack_rules();

	return buxton_smack_enabled();
}

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
	fprintf(s, "iteration: %d\tsize: %llu\n", fuzz->iteration, (unsigned long long)fuzz->size);
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
		} else	{
			/* if the daemon is still running, kill it */
			kill(SIGTERM, daemon_pid);
			usleep(64*1000);
			kill(SIGKILL, daemon_pid);
		}
	}
}

START_TEST(buxton_open_check)
{
	BuxtonClient c = NULL;
	fail_if(buxton_open(&c) == -1,
		"Connection failed to open with daemon.");
}
END_TEST

static void client_create_group_test(BuxtonResponse response, void *data)
{
	char *k = (char *)data;
	BuxtonKey key;
	char *group;
	uid_t uid = getuid();
	char *root_check = getenv(BUXTON_ROOT_CHECK_ENV);
	bool skip_check = (root_check && streq(root_check, "0"));

	fail_if(buxton_response_type(response) != BUXTON_CONTROL_CREATE_GROUP,
		"Failed to get create group response type");

	if (uid == 0) {
		fail_if(buxton_response_status(response) != 0,
			"Create group failed");
		key = buxton_response_key(response);
		fail_if(!key, "Failed to get create group key");
		group = buxton_key_get_group(key);
		fail_if(!group, "Failed to get group from key");
		fail_if(!streq(group, k),
			"Incorrect set key returned");
		free(group);
		buxton_key_free(key);
	} else {
		fail_if(buxton_response_status(response) == 0  && !skip_check,
			"Create group succeeded, but the client is not root");
	}
}
START_TEST(buxton_create_group_check)
{
	BuxtonClient c;
	BuxtonKey key = buxton_key_create("tgroup", NULL, "base", STRING);
	fail_if(!key, "Failed to create key");
	fail_if(buxton_open(&c) == -1,
		"Open failed with daemon.");
	fail_if(buxton_create_group(c, key, client_create_group_test,
				    "tgroup", true),
		"Creating group in buxton failed.");
	buxton_key_free(key);
}
END_TEST

static void client_remove_group_test(BuxtonResponse response, void *data)
{
	char *k = (char *)data;
	BuxtonKey key;
	char *group;
	uid_t uid = getuid();
	char *root_check = getenv(BUXTON_ROOT_CHECK_ENV);
	bool skip_check = (root_check && streq(root_check, "0"));

	fail_if(buxton_response_type(response) != BUXTON_CONTROL_REMOVE_GROUP,
		"Failed to get remove group response type");

	if (uid == 0) {
		fail_if(buxton_response_status(response) != 0,
			"Remove group failed");
		key = buxton_response_key(response);
		fail_if(!key, "Failed to get create group key");
		group = buxton_key_get_group(key);
		fail_if(!group, "Failed to get group from key");
		fail_if(!streq(group, k),
			"Incorrect set key returned");
		free(group);
		buxton_key_free(key);
	} else {
		fail_if(buxton_response_status(response) == 0  && !skip_check,
			"Create group succeeded, but the client is not root");
	}
}
START_TEST(buxton_remove_group_check)
{
	BuxtonClient c;
	BuxtonKey key = buxton_key_create("tgroup", NULL, "base", STRING);
	fail_if(!key, "Failed to create key");
	fail_if(buxton_open(&c) == -1,
		"Open failed with daemon.");
	fail_if(buxton_remove_group(c, key, client_remove_group_test,
				    "tgroup", true),
		"Removing group in buxton failed.");
	buxton_key_free(key);
}
END_TEST

static void client_set_value_test(BuxtonResponse response, void *data)
{
	char *k = (char *)data;
	BuxtonKey key;
	char *group;

	fail_if(buxton_response_type(response) != BUXTON_CONTROL_SET,
		"Failed to get set response type");
	fail_if(buxton_response_status(response) != 0,
		"Set value failed");
	key = buxton_response_key(response);
	fail_if(!key, "Failed to get set key");
	group = buxton_key_get_group(key);
	fail_if(!group, "Failed to get group from key");
	fail_if(!streq(group, k),
		"Incorrect set group returned");
	free(group);
	buxton_key_free(key);
}
START_TEST(buxton_set_value_check)
{
	BuxtonClient c;
	BuxtonKey group = buxton_key_create("group", NULL, "test-gdbm-user", STRING);
	fail_if(!group, "Failed to create key for group");
	BuxtonKey key = buxton_key_create("group", "name", "test-gdbm-user", STRING);
	fail_if(!key, "Failed to create key");
	fail_if(buxton_open(&c) == -1,
		"Open failed with daemon.");
	fail_if(buxton_create_group(c, group, NULL, NULL, true),
		"Creating group in buxton failed.");
	fail_if(buxton_set_label(c, group, "*", NULL, NULL, true),
		"Setting group in buxton failed.");
	fail_if(buxton_set_value(c, key, "bxt_test_value",
				 client_set_value_test,
				 "group", true),
		"Setting value in buxton failed.");
	buxton_key_free(group);
	buxton_key_free(key);
}
END_TEST

static void client_set_label_test(BuxtonResponse response, void *data)
{
	BuxtonKey user_key = (BuxtonKey)data;
	BuxtonKey key;
	char *user_group, *group;
	char *user_name, *name;
	uid_t uid = getuid();
	char *root_check = getenv(BUXTON_ROOT_CHECK_ENV);
	bool skip_check = (root_check && streq(root_check, "0"));

	fail_if(buxton_response_type(response) != BUXTON_CONTROL_SET_LABEL,
		"Failed to get set label response type");

	if (uid == 0) {
		fail_if(buxton_response_status(response) != 0,
			"Set label failed");
		key = buxton_response_key(response);
		fail_if(!key, "Failed to get set label key");
		user_group = buxton_key_get_group(user_key);
		fail_if(!user_group, "Failed to get group from user key");
		group = buxton_key_get_group(key);
		fail_if(!group, "Failed to get group from key");
		fail_if(!streq(group, user_group),
			"Incorrect set label group returned");
		free(user_group);
		free(group);

		user_name = buxton_key_get_name(user_key);
		if (user_name) {
			name = buxton_key_get_name(key);
			fail_if(!name, "Failed to get name from key");
			fail_if(!streq(name, user_name),
				"Incorrect set label name returned");
			free(user_name);
			free(name);
		}
		buxton_key_free(key);
	} else {
		if (skip_check) {
			fail_if(buxton_response_status(response) != 0,
			"Set label failed");
		} else {
			fail_if(buxton_response_status(response) == 0,
			"Set label succeeded, but the client is not root");
		}
	}
}
START_TEST(buxton_set_label_check)
{
	BuxtonClient c;
	BuxtonKey group = buxton_key_create("bxt_group", NULL, "test-gdbm", STRING);
	fail_if(!group, "Failed to create key for group");
	fail_if(buxton_open(&c) == -1,
		"Open failed with daemon.");
	fail_if(buxton_create_group(c, group, NULL, NULL, true),
		"Creating group in buxton failed.");
	fail_if(buxton_set_label(c, group, "*",
				 client_set_label_test,
				 group, true),
		"Setting label for group in buxton failed.");

	BuxtonKey name = buxton_key_create("bxt_group", "bxt_name", "test-gdbm", STRING);
	fail_if(!name, "Failed to create key for name");
	fail_if(buxton_set_value(c, name, "bxt_value", NULL, NULL, true),
		"Setting label for name in buxton failed.");
	fail_if(buxton_set_label(c, name, "*",
				 client_set_label_test,
				 name, true),
		"Setting label for name in buxton failed.");

	buxton_key_free(group);
	buxton_key_free(name);
}
END_TEST

static void client_get_value_test(BuxtonResponse response, void *data)
{
	BuxtonKey key;
	char *group;
	char *name;
	char *v;
	char *value = (char *)data;

	fail_if(buxton_response_status(response) != 0,
		"Get value failed");

	key = buxton_response_key(response);
	fail_if(!key, "Failed to get key");
	group = buxton_key_get_group(key);
	fail_if(!group, "Failed to get group");
	fail_if(!streq(group, "group"),
		"Failed to get correct group");
	name = buxton_key_get_name(key);
	fail_if(!name, "Failed to get name");
	fail_if(!streq(name, "name"),
		"Failed to get correct name");
	v = buxton_response_value(response);
	printf("val=%s\n", v);
	fail_if(!v, "Failed to get value");
	fail_if(!streq(v, value),
		"Failed to get correct value");

	free(v);
	free(group);
	free(name);
	buxton_key_free(key);
}
START_TEST(buxton_get_value_for_layer_check)
{
	BuxtonClient c = NULL;
	BuxtonKey key = buxton_key_create("group", "name", "test-gdbm-user", STRING);

	fail_if(buxton_open(&c) == -1,
		"Open failed with daemon.");
	fail_if(buxton_get_value(c, key,
				 client_get_value_test,
				 "bxt_test_value", true),
		"Retrieving value from buxton gdbm backend failed.");
}
END_TEST

START_TEST(buxton_get_value_check)
{
	BuxtonClient c = NULL;

	BuxtonKey group = buxton_key_create("group", NULL, "test-gdbm", STRING);
	fail_if(!group, "Failed to create key for group");
	BuxtonKey key = buxton_key_create("group", "name", "test-gdbm", STRING);

	fail_if(buxton_open(&c) == -1,
		"Open failed with daemon.");

	fail_if(buxton_create_group(c, group, NULL, NULL, true),
		"Creating group in buxton failed.");
	fail_if(buxton_set_label(c, group, "*", NULL, NULL, true),
		"Setting group in buxton failed.");
	fail_if(buxton_set_value(c, key, "bxt_test_value2",
				 client_set_value_test, "group", true),
		"Failed to set second value.");
	buxton_key_free(group);
	buxton_key_free(key);
	key = buxton_key_create("group", "name", NULL, STRING);
	fail_if(buxton_get_value(c, key,
				 client_get_value_test,
				 "bxt_test_value2", true),
		"Retrieving value from buxton gdbm backend failed.");
	buxton_key_free(key);
}
END_TEST

START_TEST(parse_list_check)
{
	BuxtonData l3[2];
	BuxtonData l2[4];
	BuxtonData l1[3];
	_BuxtonKey key;
	BuxtonData *value = NULL;

	fail_if(parse_list(BUXTON_CONTROL_NOTIFY, 2, l1, &key, &value),
		"Parsed bad notify argument count");
	l1[0].type = INT32;
	l1[1].type = STRING;
	l1[2].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_NOTIFY, 3, l1, &key, &value),
		"Parsed bad notify type 1");
	l1[0].type = STRING;
	l1[1].type = FLOAT;
	l1[2].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_NOTIFY, 3, l1, &key, &value),
		"Parsed bad notify type 3");
	l1[0].type = STRING;
	l1[1].type = STRING;
	l1[2].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_NOTIFY, 3, l1, &key, &value),
		"Parsed bad notify type 3");
	l1[0].type = STRING;
	l1[1].type = STRING;
	l1[2].type = UINT32;
	l1[0].store.d_string = buxton_string_pack("s1");
	l1[1].store.d_string = buxton_string_pack("s2");
	l1[2].store.d_uint32 = STRING;
	fail_if(!parse_list(BUXTON_CONTROL_NOTIFY, 3, l1, &key, &value),
		"Unable to parse valid notify");
	fail_if(!streq(key.group.value, l1[0].store.d_string.value),
		"Failed to set correct notify group");
	fail_if(!streq(key.name.value, l1[1].store.d_string.value),
		"Failed to set correct notify name");
	fail_if(key.type != l1[2].store.d_uint32,
		"Failed to set correct notify type");

	fail_if(parse_list(BUXTON_CONTROL_UNNOTIFY, 2, l1, &key, &value),
		"Parsed bad unnotify argument count");
	l1[0].type = INT32;
	l1[1].type = STRING;
	l1[2].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_UNNOTIFY, 3, l1, &key, &value),
		"Parsed bad unnotify type 1");
	l1[0].type = STRING;
	l1[1].type = FLOAT;
	l1[2].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_UNNOTIFY, 3, l1, &key, &value),
		"Parsed bad unnotify type 2");
	l1[0].type = INT32;
	l1[1].type = STRING;
	l1[2].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_UNNOTIFY, 3, l1, &key, &value),
		"Parsed bad unnotify type 3");
	l1[0].type = STRING;
	l1[1].type = STRING;
	l1[2].type = UINT32;
	l1[0].store.d_string = buxton_string_pack("s3");
	l1[1].store.d_string = buxton_string_pack("s4");
	l1[2].store.d_uint32 = STRING;
	fail_if(!parse_list(BUXTON_CONTROL_UNNOTIFY, 3, l1, &key, &value),
		"Unable to parse valid unnotify");
	fail_if(!streq(key.group.value, l1[0].store.d_string.value),
		"Failed to set correct unnotify group");
	fail_if(!streq(key.name.value, l1[1].store.d_string.value),
		"Failed to set correct unnotify name");
	fail_if(key.type != l1[2].store.d_uint32,
		"Failed to set correct unnotify type");

	fail_if(parse_list(BUXTON_CONTROL_GET, 5, l2, &key, &value),
		"Parsed bad get argument count");
	l2[0].type = INT32;
	l2[1].type = STRING;
	l2[2].type = STRING;
	l2[3].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_GET, 4, l2, &key, &value),
		"Parsed bad get type 1");
	l2[0].type = STRING;
	l2[1].type = FLOAT;
	l2[2].type = STRING;
	l2[3].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_GET, 4, l2, &key, &value),
		"Parsed bad get type 2");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[2].type = BOOLEAN;
	l2[3].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_GET, 4, l2, &key, &value),
		"Parsed bad get type 3");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[2].type = STRING;
	l2[3].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_GET, 4, l2, &key, &value),
		"Parsed bad get type 4");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[2].type = STRING;
	l2[3].type = UINT32;
	l2[0].store.d_string = buxton_string_pack("s5");
	l2[1].store.d_string = buxton_string_pack("s6");
	l2[2].store.d_string = buxton_string_pack("s7");
	l2[3].store.d_uint32 = STRING;
	fail_if(!parse_list(BUXTON_CONTROL_GET, 4, l2, &key, &value),
		"Unable to parse valid get 1");
	fail_if(!streq(key.layer.value, l2[0].store.d_string.value),
		"Failed to set correct get layer 1");
	fail_if(!streq(key.group.value, l2[1].store.d_string.value),
		"Failed to set correct get group 1");
	fail_if(!streq(key.name.value, l2[2].store.d_string.value),
		"Failed to set correct get name");
	fail_if(key.type != l2[3].store.d_uint32,
		"Failed to set correct get type 1");
	l2[0].store.d_string = buxton_string_pack("s6");
	l2[1].store.d_string = buxton_string_pack("s6");
	l2[2].type = UINT32;
	l2[2].store.d_uint32 = STRING;
	fail_if(!parse_list(BUXTON_CONTROL_GET, 3, l2, &key, &value),
		"Unable to parse valid get 2");
	fail_if(!streq(key.group.value, l2[0].store.d_string.value),
		"Failed to set correct get group 2");
	fail_if(!streq(key.name.value, l2[1].store.d_string.value),
		"Failed to set correct get name 2");
	fail_if(key.type != l2[2].store.d_uint32,
		"Failed to set correct get type 2");
	l1[0].type = INT32;
	l1[1].type = STRING;
	l1[2].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_GET, 3, l1, &key, &value),
		"Parsed bad get type 5");
	l1[0].type = STRING;
	l1[1].type = FLOAT;
	l1[2].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_GET, 3, l1, &key, &value),
		"Parsed bad get type 6");
	l1[0].type = STRING;
	l1[1].type = STRING;
	l1[2].type = BOOLEAN;
	fail_if(parse_list(BUXTON_CONTROL_GET, 3, l1, &key, &value),
		"Parsed bad get type 7");

	fail_if(parse_list(BUXTON_CONTROL_SET, 1, l2, &key, &value),
		"Parsed bad set argument count");
	l2[0].type = INT32;
	l2[1].type = STRING;
	l2[2].type = STRING;
	l2[3].type = FLOAT;
	fail_if(parse_list(BUXTON_CONTROL_SET, 4, l2, &key, &value),
		"Parsed bad set type 1");
	l2[0].type = STRING;
	l2[1].type = FLOAT;
	l2[2].type = STRING;
	l2[3].type = FLOAT;
	fail_if(parse_list(BUXTON_CONTROL_SET, 4, l2, &key, &value),
		"Parsed bad set type 2");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[2].type = BOOLEAN;
	l2[3].type = FLOAT;
	fail_if(parse_list(BUXTON_CONTROL_SET, 4, l2, &key, &value),
		"Parsed bad set type 3");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[2].type = STRING;
	l2[3].type = FLOAT;
	l2[0].store.d_string = buxton_string_pack("s8");
	l2[1].store.d_string = buxton_string_pack("s9");
	l2[2].store.d_string = buxton_string_pack("s10");
	l2[3].store.d_float = 3.14F;
	fail_if(!parse_list(BUXTON_CONTROL_SET, 4, l2, &key, &value),
		"Unable to parse valid set 1");
	fail_if(!streq(key.layer.value, l2[0].store.d_string.value),
		"Failed to set correct set layer 1");
	fail_if(!streq(key.group.value, l2[1].store.d_string.value),
		"Failed to set correct set group 1");
	fail_if(!streq(key.name.value, l2[2].store.d_string.value),
		"Failed to set correct set name 1");
	fail_if(value->store.d_float != l2[3].store.d_float,
		"Failed to set correct set value 1");

	fail_if(parse_list(BUXTON_CONTROL_UNSET, 1, l2, &key, &value),
		"Parsed bad unset argument count");
	l2[0].type = INT32;
	l2[1].type = STRING;
	l2[2].type = STRING;
	l2[3].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_UNSET, 4, l2, &key, &value),
		"Parsed bad unset type 1");
	l2[0].type = STRING;
	l2[1].type = FLOAT;
	l2[2].type = STRING;
	l2[3].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_UNSET, 4, l2, &key, &value),
		"Parsed bad unset type 2");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[2].type = BOOLEAN;
	l2[3].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_UNSET, 4, l2, &key, &value),
		"Parsed bad unset type 3");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[2].type = STRING;
	l2[3].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_UNSET, 4, l2, &key, &value),
		"Parsed bad unset type 4");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[2].type = STRING;
	l2[3].type = UINT32;
	l2[0].store.d_string = buxton_string_pack("s11");
	l2[1].store.d_string = buxton_string_pack("s12");
	l2[2].store.d_string = buxton_string_pack("s13");
	l2[3].store.d_uint32 = STRING;
	fail_if(!parse_list(BUXTON_CONTROL_UNSET, 4, l2, &key, &value),
		"Unable to parse valid unset 1");
	fail_if(!streq(key.layer.value, l2[0].store.d_string.value),
		"Failed to set correct unset layer 1");
	fail_if(!streq(key.group.value, l2[1].store.d_string.value),
		"Failed to set correct unset group 1");
	fail_if(!streq(key.name.value, l2[2].store.d_string.value),
		"Failed to set correct unset name 1");
	fail_if(key.type != l2[3].store.d_uint32,
		"Failed to set correct unset type 1");

	fail_if(parse_list(BUXTON_CONTROL_SET_LABEL, 1, l2, &key, &value),
		"Parsed bad set label argument count");
	l1[0].type = INT32;
	l1[1].type = STRING;
	l1[2].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_SET_LABEL, 3, l1, &key, &value),
		"Parsed bad set label type 1");
	l1[0].type = STRING;
	l1[1].type = FLOAT;
	l1[2].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_SET_LABEL, 3, l1, &key, &value),
		"Parsed bad set label type 2");
	l1[0].type = STRING;
	l1[1].type = STRING;
	l1[2].type = BOOLEAN;
	fail_if(parse_list(BUXTON_CONTROL_SET_LABEL, 3, l1, &key, &value),
		"Parsed bad set label type 3");
	l1[0].type = STRING;
	l1[1].type = STRING;
	l1[2].type = STRING;
	l1[0].store.d_string = buxton_string_pack("s14");
	l1[1].store.d_string = buxton_string_pack("s15");
	l1[2].store.d_string = buxton_string_pack("*");
	fail_if(!parse_list(BUXTON_CONTROL_SET_LABEL, 3, l1, &key, &value),
		"Unable to parse valid set label 1");
	fail_if(!streq(key.layer.value, l1[0].store.d_string.value),
		"Failed to set correct set label layer 1");
	fail_if(!streq(key.group.value, l1[1].store.d_string.value),
		"Failed to set correct set label group 1");
	fail_if(!streq(value->store.d_string.value, l1[2].store.d_string.value),
		"Failed to set correct set label label 1");
	fail_if(key.type != STRING, "Failed to key type in set label");
	l2[0].type = INT32;
	l2[1].type = STRING;
	l2[2].type = STRING;
	l2[3].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_SET_LABEL, 4, l2, &key, &value),
		"Parsed bad set label type 4");
	l2[0].type = STRING;
	l2[1].type = FLOAT;
	l2[2].type = STRING;
	l2[3].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_SET_LABEL, 4, l2, &key, &value),
		"Parsed bad set label type 5");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[2].type = BOOLEAN;
	l2[3].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_SET_LABEL, 4, l2, &key, &value),
		"Parsed bad set label type 6");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[2].type = STRING;
	l2[3].type = UINT32;
	fail_if(parse_list(BUXTON_CONTROL_SET_LABEL, 4, l2, &key, &value),
		"Parsed bad set label type 7");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[2].type = STRING;
	l2[3].type = STRING;
	l2[0].store.d_string = buxton_string_pack("x1");
	l2[1].store.d_string = buxton_string_pack("x2");
	l2[2].store.d_string = buxton_string_pack("x3");
	l2[3].store.d_string = buxton_string_pack("x4");
	fail_if(!parse_list(BUXTON_CONTROL_SET_LABEL, 4, l2, &key, &value),
		"Unable to parse valid set label 2");
	fail_if(!streq(key.layer.value, l2[0].store.d_string.value),
		"Failed to set correct set label layer 2");
	fail_if(!streq(key.group.value, l2[1].store.d_string.value),
		"Failed to set correct set label group 2");
	fail_if(!streq(key.name.value, l2[2].store.d_string.value),
		"Failed to set correct set label name 2");
	fail_if(!streq(value->store.d_string.value, l2[3].store.d_string.value),
		"Failed to set correct set label label 2");

	fail_if(parse_list(BUXTON_CONTROL_CREATE_GROUP, 1, l3, &key, &value),
		"Parsed bad create group argument count");
	l3[0].type = INT32;
	l3[1].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_CREATE_GROUP, 2, l3, &key, &value),
		"Parsed bad create group type 1");
	l3[0].type = STRING;
	l3[1].type = FLOAT;
	fail_if(parse_list(BUXTON_CONTROL_CREATE_GROUP, 2, l3, &key, &value),
		"Parsed bad create group type 2");
	l3[0].type = STRING;
	l3[1].type = STRING;
	l3[0].store.d_string = buxton_string_pack("s16");
	l3[1].store.d_string = buxton_string_pack("s17");
	fail_if(!parse_list(BUXTON_CONTROL_CREATE_GROUP, 2, l3, &key, &value),
		"Unable to parse valid create group 1");
	fail_if(!streq(key.layer.value, l3[0].store.d_string.value),
		"Failed to set correct create group layer 1");
	fail_if(!streq(key.group.value, l3[1].store.d_string.value),
		"Failed to set correct create group group 1");
	fail_if(key.type != STRING, "Failed to key type in create group");

	fail_if(parse_list(BUXTON_CONTROL_REMOVE_GROUP, 1, l3, &key, &value),
		"Parsed bad remove group argument count");
	l3[0].type = INT32;
	l3[1].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_REMOVE_GROUP, 2, l3, &key, &value),
		"Parsed bad remove group type 1");
	l3[0].type = STRING;
	l3[1].type = FLOAT;
	fail_if(parse_list(BUXTON_CONTROL_REMOVE_GROUP, 2, l3, &key, &value),
		"Parsed bad remove group type 2");
	l3[0].type = STRING;
	l3[1].type = STRING;
	l3[0].store.d_string = buxton_string_pack("s18");
	l3[1].store.d_string = buxton_string_pack("s19");
	fail_if(!parse_list(BUXTON_CONTROL_REMOVE_GROUP, 2, l3, &key, &value),
		"Unable to parse valid remove group 1");
	fail_if(!streq(key.layer.value, l3[0].store.d_string.value),
		"Failed to set correct remove group layer 1");
	fail_if(!streq(key.group.value, l3[1].store.d_string.value),
		"Failed to set correct remove group group 1");
	fail_if(key.type != STRING, "Failed to key type in remove group");

	fail_if(parse_list(BUXTON_CONTROL_MIN, 2, l3, &key, &value),
		"Parsed bad control type 1");
}
END_TEST

START_TEST(create_group_check)
{
	_BuxtonKey key = { {0}, {0}, {0}, 0};
	client_list_item client;
	int32_t status;
	BuxtonDaemon server;
	BuxtonString clabel = buxton_string_pack("_");

	fail_if(!buxton_direct_open(&server.buxton),
		"Failed to open buxton direct connection");

	client.cred.uid = getuid();
	if (use_smack())
		client.smack_label = &clabel;
	else
		client.smack_label = NULL;
	server.buxton.client.uid = 0;

	key.layer = buxton_string_pack("test-gdbm-user");
	key.group = buxton_string_pack("daemon-check");
	key.type = STRING;
	create_group(&server, &client, &key, &status);
	fail_if(status != 0, "Failed to create group");

	key.layer = buxton_string_pack("test-gdbm");
	create_group(&server, &client, &key, &status);
	fail_if(status != 0, "Failed to create group");

	key.layer = buxton_string_pack("base");
	key.group = buxton_string_pack("tgroup");
	create_group(&server, &client, &key, &status);
	fail_if(status != 0, "Failed to create group");

	buxton_direct_close(&server.buxton);
}
END_TEST

START_TEST(remove_group_check)
{
	_BuxtonKey key = { {0}, {0}, {0}, 0};
	client_list_item client;
	int32_t status;
	BuxtonDaemon server;
	BuxtonString clabel = buxton_string_pack("_");

	fail_if(!buxton_direct_open(&server.buxton),
		"Failed to open buxton direct connection");

	client.cred.uid = getuid();
	if (use_smack())
		client.smack_label = &clabel;
	else
		client.smack_label = NULL;
	server.buxton.client.uid = 0;

	key.layer = buxton_string_pack("base");
	key.group = buxton_string_pack("tgroup");
	key.type = STRING;

	remove_group(&server, &client, &key, &status);
	fail_if(status != 0, "Failed to remove group");

	buxton_direct_close(&server.buxton);
}
END_TEST

START_TEST(set_label_check)
{
	_BuxtonKey key = { {0}, {0}, {0}, 0};
	BuxtonData value;
	client_list_item client;
	int32_t status;
	BuxtonDaemon server;
	BuxtonString clabel = buxton_string_pack("_");

	fail_if(!buxton_direct_open(&server.buxton),
		"Failed to open buxton direct connection");

	client.cred.uid = getuid();
	if (use_smack())
		client.smack_label = &clabel;
	else
		client.smack_label = NULL;
	server.buxton.client.uid = 0;
	key.layer = buxton_string_pack("test-gdbm");
	key.group = buxton_string_pack("daemon-check");
	key.type = STRING;
	value.type = STRING;
	value.store.d_string = buxton_string_pack("*");

	set_label(&server, &client, &key, &value, &status);
	fail_if(status != 0, "Failed to set label");
	buxton_direct_close(&server.buxton);
}
END_TEST

START_TEST(set_value_check)
{
	_BuxtonKey key = { {0}, {0}, {0}, 0};
	BuxtonData value;
	client_list_item client;
	int32_t status;
	BuxtonDaemon server;
	BuxtonString clabel = buxton_string_pack("_");

	fail_if(!buxton_direct_open(&server.buxton),
		"Failed to open buxton direct connection");

	client.cred.uid = getuid();
	server.buxton.client.uid = 0;

	if (use_smack())
		client.smack_label = &clabel;
	else
		client.smack_label = NULL;

	key.layer = buxton_string_pack("test-gdbm-user");
	key.group = buxton_string_pack("daemon-check");
	key.name = buxton_string_pack("name");
	value.type = STRING;
	value.store.d_string = buxton_string_pack("user-layer-value");

	set_value(&server, &client, &key, &value, &status);
	fail_if(status != 0, "Failed to set value");
	fail_if(server.buxton.client.uid != client.cred.uid, "Failed to change buxton uid");

	key.layer = buxton_string_pack("test-gdbm");
	value.store.d_string = buxton_string_pack("system-layer-value");
	set_value(&server, &client, &key, &value, &status);
	fail_if(status != 0, "Failed to set value");

	buxton_direct_close(&server.buxton);
}
END_TEST

START_TEST(get_value_check)
{
	_BuxtonKey key = { {0}, {0}, {0}, 0};
	BuxtonData *value;
	client_list_item client;
	int32_t status;
	BuxtonDaemon server;
	BuxtonString clabel = buxton_string_pack("_");

	fail_if(!buxton_direct_open(&server.buxton),
		"Failed to open buxton direct connection");

	fail_if(!buxton_cache_smack_rules(),
		"Failed to cache smack rules");
	client.cred.uid = getuid();
	if (use_smack())
		client.smack_label = &clabel;
	else
		client.smack_label = NULL;
	server.buxton.client.uid = 0;
	key.layer = buxton_string_pack("test-gdbm-user");
	key.group = buxton_string_pack("daemon-check");
	key.name = buxton_string_pack("name");
	key.type = STRING;

	value = get_value(&server, &client, &key, &status);
	fail_if(!value, "Failed to get value");
	fail_if(status != 0, "Failed to get value");
	fail_if(value->type != STRING, "Failed to get correct type");
	fail_if(!streq(value->store.d_string.value, "user-layer-value"), "Failed to get correct value");
	fail_if(server.buxton.client.uid != client.cred.uid, "Failed to change buxton uid");
	free(value);

	server.buxton.client.uid = 0;
	key.layer.value = NULL;
	key.layer.length = 0;
	value = get_value(&server, &client, &key, &status);
	fail_if(!value, "Failed to get value 2");
	fail_if(status != 0, "Failed to get value 2");
	fail_if(value->type != STRING, "Failed to get correct type 2");
	fail_if(!streq(value->store.d_string.value, "system-layer-value"), "Failed to get correct value 2");
	fail_if(server.buxton.client.uid != client.cred.uid, "Failed to change buxton uid 2");
	free(value);

	buxton_direct_close(&server.buxton);
}
END_TEST

START_TEST(register_notification_check)
{
	_BuxtonKey key = { {0}, {0}, {0}, 0};
	client_list_item client, no_client;
	BuxtonString clabel = buxton_string_pack("_");
	int32_t status;
	BuxtonDaemon server;
	uint32_t msgid;

	fail_if(!buxton_cache_smack_rules(),
		"Failed to cache smack rules");
	if (use_smack())
		client.smack_label = &clabel;
	else
		client.smack_label = NULL;
	client.cred.uid = 1002;
	fail_if(!buxton_direct_open(&server.buxton),
		"Failed to open buxton direct connection");
	server.notify_mapping = hashmap_new(string_hash_func, string_compare_func);
	fail_if(!server.notify_mapping, "Failed to allocate hashmap");

	key.group = buxton_string_pack("group");
	key.name = buxton_string_pack("name");
	key.type = STRING;
	register_notification(&server, &client, &key, 1, &status);
	fail_if(status != 0, "Failed to register notification");
	register_notification(&server, &client, &key, 1, &status);
	fail_if(status != 0, "Failed to register notification");
	//FIXME: Figure out what to do with duplicates
	key.group = buxton_string_pack("no-key");
	msgid = unregister_notification(&server, &client, &key, &status);
	fail_if(status == 0,
		"Unregistered from notifications with invalid key");
	fail_if(msgid != 0, "Got unexpected notify message id");
	key.group = buxton_string_pack("group");
	msgid = unregister_notification(&server, &no_client, &key, &status);
	fail_if(status == 0,
		"Unregistered from notifications with invalid client");
	fail_if(msgid != 0, "Got unexpected notify message id");
	msgid = unregister_notification(&server, &client, &key, &status);
	fail_if(status != 0,
		"Unable to unregister from notifications");
	fail_if(msgid != 1, "Failed to get correct notify message id");
	key.group = buxton_string_pack("key2");
	register_notification(&server, &client, &key, 0, &status);
	fail_if(status == 0, "Registered notification with key not in db");

	hashmap_free(server.notify_mapping);
	buxton_direct_close(&server.buxton);
}
END_TEST
START_TEST(buxtond_handle_message_error_check)
{
	int client, server;
	BuxtonDaemon daemon;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1;
	client_list_item cl;
	bool r;
	BuxtonArray *list = NULL;
	uint16_t control;

	setup_socket_pair(&client, &server);
	fail_if(fcntl(client, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");
	fail_if(fcntl(server, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");
	list = buxton_array_new();
	fail_if(!list, "Failed to allocate list");

	cl.fd = server;
	slabel = buxton_string_pack("_");
	cl.smack_label = &slabel;
	daemon.buxton.client.uid = 1001;
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	fail_if(!buxton_direct_open(&daemon.buxton),
		"Failed to open buxton direct connection");

	cl.data = malloc(4);
	fail_if(!cl.data, "Couldn't allocate blank message");
	cl.data[0] = 0;
	cl.data[1]= 0;
	cl.data[2] = 0;
	cl.data[3] = 0;
	size = 100;
	r = buxtond_handle_message(&daemon, &cl, size);
	fail_if(r, "Failed to detect invalid message data");
	free(cl.data);

	data1.type = STRING;
	data1.store.d_string = buxton_string_pack("group");
	r = buxton_array_add(list, &data1);
	fail_if(!r, "Failed to add element to array");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_NOTIFY, 0,
					list);
	fail_if(size == 0, "Failed to serialize message");
	control = BUXTON_CONTROL_MIN;
	memcpy(cl.data, &control, sizeof(uint16_t));
	r = buxtond_handle_message(&daemon, &cl, size);
	fail_if(r, "Failed to detect min control size");
	control = BUXTON_CONTROL_MAX;
	memcpy(cl.data, &control, sizeof(uint16_t));
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(r, "Failed to detect max control size");

	close(client);
	buxton_direct_close(&daemon.buxton);
	buxton_array_free(&list, NULL);
}
END_TEST

START_TEST(buxtond_handle_message_create_group_check)
{
	BuxtonDaemon daemon;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1, data2;
	client_list_item cl;
	bool r;
	BuxtonData *list;
	BuxtonArray *out_list1, *out_list2;
	BuxtonControlMessage msg;
	ssize_t csize;
	int client, server;
	ssize_t s;
	uint8_t buf[4096];
	uint32_t msgid;

	setup_socket_pair(&client, &server);
	fail_if(fcntl(client, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");
	fail_if(fcntl(server, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");

	cl.fd = server;
	slabel = buxton_string_pack("_");
	if (use_smack())
		cl.smack_label = &slabel;
	else
		cl.smack_label = NULL;
	cl.cred.uid = 1002;
	daemon.buxton.client.uid = 1001;
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	fail_if(!buxton_direct_open(&daemon.buxton),
		"Failed to open buxton direct connection");
	daemon.notify_mapping = hashmap_new(string_hash_func, string_compare_func);
	fail_if(!daemon.notify_mapping, "Failed to allocate hashmap");

	out_list1 = buxton_array_new();
	fail_if(!out_list1, "Failed to allocate list");
	data1.type = STRING;
	data1.store.d_string = buxton_string_pack("base");
	data2.type = STRING;
	data2.store.d_string = buxton_string_pack("tgroup");
	r = buxton_array_add(out_list1, &data1);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list1, &data2);
	fail_if(!r, "Failed to add element to array");

	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_CREATE_GROUP, 0,
					out_list1);
	fail_if(size == 0, "Failed to serialize message");
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to handle create group message");

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1, "Failed to get correct response to create group");
	fail_if(msg != BUXTON_CONTROL_STATUS,
		"Failed to get correct control type");
	fail_if(list[0].type != INT32,
		"Failed to get correct indicator type");
	fail_if(list[0].store.d_int32 != 0,
		"Failed to create group");
	fail_if(msgid != 0, "Failed to get correct message id");
	free(list);

	out_list2 = buxton_array_new();
	fail_if(!out_list2, "Failed to allocate list");
	data1.store.d_string = buxton_string_pack("base");
	data2.store.d_string = buxton_string_pack("daemon-check");
	r = buxton_array_add(out_list2, &data1);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list2, &data2);
	fail_if(!r, "Failed to add element to array");

	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_CREATE_GROUP, 1,
					out_list2);
	fail_if(size == 0, "Failed to serialize message");
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to handle create group message");

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1, "Failed to get correct response to create group");
	fail_if(msg != BUXTON_CONTROL_STATUS,
		"Failed to get correct control type");
	fail_if(list[0].type != INT32,
		"Failed to get correct indicator type");
	fail_if(list[0].store.d_int32 != 0,
		"Failed to create group");
	fail_if(msgid != 1, "Failed to get correct message id");

	free(list);
	cleanup_callbacks();
	close(client);
	hashmap_free(daemon.notify_mapping);
	buxton_direct_close(&daemon.buxton);
	buxton_array_free(&out_list1, NULL);
	buxton_array_free(&out_list2, NULL);
}
END_TEST

START_TEST(buxtond_handle_message_remove_group_check)
{
	BuxtonDaemon daemon;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1, data2;
	client_list_item cl;
	bool r;
	BuxtonData *list;
	BuxtonArray *out_list;
	BuxtonControlMessage msg;
	ssize_t csize;
	int client, server;
	ssize_t s;
	uint8_t buf[4096];
	uint32_t msgid;

	setup_socket_pair(&client, &server);
	fail_if(fcntl(client, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");
	fail_if(fcntl(server, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");

	out_list = buxton_array_new();
	fail_if(!out_list, "Failed to allocate list");
	cl.fd = server;
	slabel = buxton_string_pack("_");
	if (use_smack())
		cl.smack_label = &slabel;
	else
		cl.smack_label = NULL;
	cl.cred.uid = 1002;
	daemon.buxton.client.uid = 1001;
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	fail_if(!buxton_direct_open(&daemon.buxton),
		"Failed to open buxton direct connection");
	daemon.notify_mapping = hashmap_new(string_hash_func, string_compare_func);
	fail_if(!daemon.notify_mapping, "Failed to allocate hashmap");

	data1.type = STRING;
	data1.store.d_string = buxton_string_pack("base");
	data2.type = STRING;
	data2.store.d_string = buxton_string_pack("tgroup");
	r = buxton_array_add(out_list, &data1);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data2);
	fail_if(!r, "Failed to add element to array");

	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_REMOVE_GROUP, 0,
					out_list);
	fail_if(size == 0, "Failed to serialize message");
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to handle remove group message");

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1, "Failed to get correct response to remove group");
	fail_if(msg != BUXTON_CONTROL_STATUS,
		"Failed to get correct control type");
	fail_if(list[0].type != INT32,
		"Failed to get correct indicator type");
	fail_if(list[0].store.d_int32 != 0,
		"Failed to remove group");
	fail_if(msgid != 0, "Failed to get correct message id");

	free(list);
	cleanup_callbacks();
	close(client);
	hashmap_free(daemon.notify_mapping);
	buxton_direct_close(&daemon.buxton);
	buxton_array_free(&out_list, NULL);
}
END_TEST

START_TEST(buxtond_handle_message_set_label_check)
{
	BuxtonDaemon daemon;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1, data2, data3;
	client_list_item cl;
	bool r;
	BuxtonData *list;
	BuxtonArray *out_list;
	BuxtonControlMessage msg;
	ssize_t csize;
	int client, server;
	ssize_t s;
	uint8_t buf[4096];
	uint32_t msgid;

	setup_socket_pair(&client, &server);
	fail_if(fcntl(client, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");
	fail_if(fcntl(server, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");

	out_list = buxton_array_new();
	fail_if(!out_list, "Failed to allocate list");
	cl.fd = server;
	slabel = buxton_string_pack("_");
	if (use_smack())
		cl.smack_label = &slabel;
	else
		cl.smack_label = NULL;
	cl.cred.uid = 1002;
	daemon.buxton.client.uid = 1001;
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	fail_if(!buxton_direct_open(&daemon.buxton),
		"Failed to open buxton direct connection");
	daemon.notify_mapping = hashmap_new(string_hash_func, string_compare_func);
	fail_if(!daemon.notify_mapping, "Failed to allocate hashmap");

	data1.type = STRING;
	data1.store.d_string = buxton_string_pack("base");
	data2.type = STRING;
	data2.store.d_string = buxton_string_pack("daemon-check");
	data3.type = STRING;
	data3.store.d_string = buxton_string_pack("*");
	r = buxton_array_add(out_list, &data1);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data2);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data3);
	fail_if(!r, "Failed to add element to array");

	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_SET_LABEL, 0,
					out_list);
	fail_if(size == 0, "Failed to serialize message");
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to handle set label message");

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1, "Failed to get correct response to set label");
	fail_if(msg != BUXTON_CONTROL_STATUS,
		"Failed to get correct control type");
	fail_if(list[0].type != INT32,
		"Failed to get correct indicator type");
	fail_if(list[0].store.d_int32 != 0,
		"Failed to set label");
	fail_if(msgid != 0, "Failed to get correct message id");

	free(list);
	cleanup_callbacks();
	close(client);
	hashmap_free(daemon.notify_mapping);
	buxton_direct_close(&daemon.buxton);
	buxton_array_free(&out_list, NULL);
}
END_TEST

START_TEST(buxtond_handle_message_set_value_check)
{
	BuxtonDaemon daemon;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1, data2, data3, data4;
	client_list_item cl;
	bool r;
	BuxtonData *list;
	BuxtonArray *out_list;
	BuxtonControlMessage msg;
	ssize_t csize;
	int client, server;
	ssize_t s;
	uint8_t buf[4096];
	uint32_t msgid;

	setup_socket_pair(&client, &server);
	fail_if(fcntl(client, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");
	fail_if(fcntl(server, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");

	out_list = buxton_array_new();
	fail_if(!out_list, "Failed to allocate list");
	cl.fd = server;
	slabel = buxton_string_pack("_");
	if (use_smack())
		cl.smack_label = &slabel;
	else
		cl.smack_label = NULL;
	cl.cred.uid = 1002;
	daemon.buxton.client.uid = 1001;
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	fail_if(!buxton_direct_open(&daemon.buxton),
		"Failed to open buxton direct connection");
	daemon.notify_mapping = hashmap_new(string_hash_func, string_compare_func);
	fail_if(!daemon.notify_mapping, "Failed to allocate hashmap");

	data1.type = STRING;
	data1.store.d_string = buxton_string_pack("base");
	data2.type = STRING;
	data2.store.d_string = buxton_string_pack("daemon-check");
	data3.type = STRING;
	data3.store.d_string = buxton_string_pack("name");
	data4.type = STRING;
	data4.store.d_string = buxton_string_pack("bxt_test_value3");
	r = buxton_array_add(out_list, &data1);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data2);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data3);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data4);
	fail_if(!r, "Failed to add element to array");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_NOTIFY, 0,
					out_list);
	fail_if(size == 0, "Failed to serialize message");
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(r, "Failed to detect parse_list failure");

	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_SET, 0,
					out_list);
	fail_if(size == 0, "Failed to serialize message");
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to handle set message");

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1, "Failed to get correct response to set");
	fail_if(msg != BUXTON_CONTROL_STATUS,
		"Failed to get correct control type");
	fail_if(list[0].type != INT32,
		"Failed to get correct indicator type");
	fail_if(list[0].store.d_int32 != 0,
		"Failed to set");
	fail_if(msgid != 0, "Failed to get correct message id");

	free(list);
	cleanup_callbacks();
	close(client);
	hashmap_free(daemon.notify_mapping);
	buxton_direct_close(&daemon.buxton);
	buxton_array_free(&out_list, NULL);
}
END_TEST

START_TEST(buxtond_handle_message_get_check)
{
	int client, server;
	BuxtonDaemon daemon;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1, data2, data3, data4;
	client_list_item cl;
	bool r;
	BuxtonData *list;
	BuxtonArray *out_list;
	BuxtonArray *out_list2;
	BuxtonControlMessage msg;
	ssize_t csize;
	ssize_t s;
	uint8_t buf[4096];
	uint32_t msgid;

	setup_socket_pair(&client, &server);
	out_list = buxton_array_new();
	fail_if(!out_list, "Failed to allocate list");

	cl.fd = server;
	slabel = buxton_string_pack("_");
	if (use_smack())
		cl.smack_label = &slabel;
	else
		cl.smack_label = NULL;
	cl.cred.uid = getuid();
	daemon.buxton.client.uid = 1001;
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	fail_if(!buxton_direct_open(&daemon.buxton),
		"Failed to open buxton direct connection");

	data1.type = STRING;
	data1.store.d_string = buxton_string_pack("test-gdbm-user");
	data2.type = STRING;
	data2.store.d_string = buxton_string_pack("daemon-check");
	data3.type = STRING;
	data3.store.d_string = buxton_string_pack("name");
	data4.type = UINT32;
	data4.store.d_uint32 = STRING;
	r = buxton_array_add(out_list, &data1);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data2);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data3);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data4);
	fail_if(!r, "Failed to add element to array");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_GET, 0,
					out_list);
	fail_if(size == 0, "Failed to serialize message");
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to get message 1");

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 2, "Failed to get valid message from buffer");
	fail_if(msg != BUXTON_CONTROL_STATUS,
		"Failed to get correct control type");
	fail_if(msgid != 0, "Failed to get correct message id");
	fail_if(list[0].type != INT32, "Failed to get correct response type");
	fail_if(list[0].store.d_int32 != 0,
		"Failed to get value");
	fail_if(list[1].type != STRING, "Failed to get correct value type");
	fail_if(!streq(list[1].store.d_string.value, "user-layer-value"),
		"Failed to get correct value");

	free(list[1].store.d_string.value);
	free(list);

	out_list2 = buxton_array_new();
	fail_if(!out_list2, "Failed to allocate list 2");
	r = buxton_array_add(out_list2, &data2);
	fail_if(!r, "Failed to add element to array 2");
	r = buxton_array_add(out_list2, &data3);
	fail_if(!r, "Failed to add element to array 2");
	r = buxton_array_add(out_list2, &data4);
	fail_if(!r, "Failed to add element to array 2");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_GET, 0,
					out_list2);
	fail_if(size == 0, "Failed to serialize message 2");
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to get message 2");

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed 2");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 2, "Failed to get correct response to get 2");
	fail_if(msg != BUXTON_CONTROL_STATUS,
		"Failed to get correct control type 2");
	fail_if(msgid != 0, "Failed to get correct message id 2");
	fail_if(list[0].type != INT32, "Failed to get correct response type 2");
	fail_if(list[0].store.d_int32 != 0,
		"Failed to get value 2");
	fail_if(list[1].type != STRING, "Failed to get correct value type 2");
	fail_if(streq(list[1].store.d_string.value, "bxt_test_value2"),
		"Failed to get correct value 2");

	free(list[1].store.d_string.value);
	free(list);
	close(client);
	buxton_direct_close(&daemon.buxton);
	buxton_array_free(&out_list, NULL);
	buxton_array_free(&out_list2, NULL);
}
END_TEST

START_TEST(buxtond_handle_message_notify_check)
{
	int client, server;
	BuxtonDaemon daemon;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1, data2, data3;
	client_list_item cl;
	bool r;
	BuxtonData *list;
	BuxtonArray *out_list;
	BuxtonControlMessage msg;
	ssize_t csize;
	ssize_t s;
	uint8_t buf[4096];
	uint32_t msgid;

	setup_socket_pair(&client, &server);
	out_list = buxton_array_new();
	fail_if(!out_list, "Failed to allocate list");

	cl.fd = server;
	slabel = buxton_string_pack("_");
	if (use_smack())
		cl.smack_label = &slabel;
	else
		cl.smack_label = NULL;
	cl.cred.uid = 1002;
	daemon.buxton.client.uid = 1001;
	daemon.notify_mapping = hashmap_new(string_hash_func, string_compare_func);
	fail_if(!daemon.notify_mapping, "Failed to allocate hashmap");
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	fail_if(!buxton_direct_open(&daemon.buxton),
		"Failed to open buxton direct connection");

	data1.type = STRING;
	data1.store.d_string = buxton_string_pack("group");
	data2.type = STRING;
	data2.store.d_string = buxton_string_pack("name");
	data3.type = UINT32;
	data3.store.d_uint32 = STRING;
	r = buxton_array_add(out_list, &data1);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data2);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data3);
	fail_if(!r, "Failed to add element to array");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_NOTIFY, 0,
					out_list);
	fail_if(size == 0, "Failed to serialize message");
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to register for notification");

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1, "Failed to get correct response to notify");
	fail_if(msg != BUXTON_CONTROL_STATUS,
		"Failed to get correct control type");
	fail_if(msgid != 0, "Failed to get correct notify message id");
	fail_if(list[0].type != INT32, "Failed to get correct response type");
	fail_if(list[0].store.d_int32 != 0,
		"Failed to register notification");

	free(list);

	/* UNNOTIFY */
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_UNNOTIFY, 0,
					out_list);
	fail_if(size == 0, "Failed to serialize message");
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to unregister from notification");

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed 2");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 2, "Failed to get correct response to unnotify");
	fail_if(msg != BUXTON_CONTROL_STATUS,
		"Failed to get correct control type 2");
	fail_if(list[0].type != INT32,
		"Failed to get correct indicator type 2");
	fail_if(list[0].store.d_int32 != 0,
		"Failed to unregister for notification");
	fail_if(list[1].type != UINT32,
		"Failed to get correct unnotify message id type");
	fail_if(list[1].store.d_uint32 != 0,
		"Failed to get correct unnotify message id");
	fail_if(msgid != 0, "Failed to get correct message id 2");

	free(list);
	close(client);
	hashmap_free(daemon.notify_mapping);
	buxton_direct_close(&daemon.buxton);
	buxton_array_free(&out_list, NULL);
}
END_TEST

START_TEST(buxtond_handle_message_unset_check)
{
	int client, server;
	BuxtonDaemon daemon;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1, data2, data3, data4;
	client_list_item cl;
	bool r;
	BuxtonData *list;
	BuxtonArray *out_list;
	BuxtonControlMessage msg;
	ssize_t csize;
	ssize_t s;
	uint8_t buf[4096];
	uint32_t msgid;

	setup_socket_pair(&client, &server);
	out_list = buxton_array_new();
	fail_if(!out_list, "Failed to allocate list");

	cl.fd = server;
	slabel = buxton_string_pack("_");
	if (use_smack())
		cl.smack_label = &slabel;
	else
		cl.smack_label = NULL;
	cl.cred.uid = 1002;
	daemon.buxton.client.uid = 1001;
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	fail_if(!buxton_direct_open(&daemon.buxton),
		"Failed to open buxton direct connection");
	daemon.notify_mapping = hashmap_new(string_hash_func, string_compare_func);
	fail_if(!daemon.notify_mapping, "Failed to allocate hashmap");

	data1.type = STRING;
	data1.store.d_string = buxton_string_pack("base");
	data2.type = STRING;
	data2.store.d_string = buxton_string_pack("daemon-check");
	data3.type = STRING;
	data3.store.d_string = buxton_string_pack("name");
	data4.type = UINT32;
	data4.store.d_uint32 = STRING;
	r = buxton_array_add(out_list, &data1);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data2);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data3);
	fail_if(!r, "Failed to add element to array");
	r = buxton_array_add(out_list, &data4);
	fail_if(!r, "Failed to add element to array");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_UNSET, 0,
					out_list);
	fail_if(size == 0, "Failed to serialize message");
	r = buxtond_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to unset message");

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1, "Failed to get correct response to unset");
	fail_if(msg != BUXTON_CONTROL_STATUS,
		"Failed to get correct control type");
	fail_if(list[0].type != INT32,
		"Failed to get correct indicator type");
	fail_if(list[0].store.d_int32 != 0,
		"Failed to unset");
	fail_if(msgid != 0, "Failed to get correct message id");

	free(list);
	close(client);
	hashmap_free(daemon.notify_mapping);
	buxton_direct_close(&daemon.buxton);
	buxton_array_free(&out_list, NULL);
}
END_TEST

START_TEST(buxtond_notify_clients_check)
{
	int client, server;
	BuxtonDaemon daemon;
	_BuxtonKey key;
	BuxtonString slabel;
	BuxtonData value1, value2;
	client_list_item cl;
	int32_t status;
	bool r;
	BuxtonData *list;
	BuxtonControlMessage msg;
	ssize_t csize;
	ssize_t s;
	uint8_t buf[4096];
	uint32_t msgid;

	setup_socket_pair(&client, &server);

	cl.fd = server;
	slabel = buxton_string_pack("_");
	if (use_smack())
		cl.smack_label = &slabel;
	else
		cl.smack_label = NULL;
	cl.cred.uid = 1002;
	daemon.notify_mapping = hashmap_new(string_hash_func,
					    string_compare_func);
	fail_if(!daemon.notify_mapping, "Failed to allocate hashmap");
	fail_if(!buxton_cache_smack_rules(),
		"Failed to cache Smack rules");
	fail_if(!buxton_direct_open(&daemon.buxton),
		"Failed to open buxton direct connection");

	value1.type = STRING;
	value1.store.d_string = buxton_string_pack("dummy value");
	key.group = buxton_string_pack("dummy");
	key.name = buxton_string_pack("name");
	buxtond_notify_clients(&daemon, &cl, &key, &value1);

	value1.store.d_string = buxton_string_pack("real value");
	key.group = buxton_string_pack("daemon-check");
	key.name = buxton_string_pack("name");
	key.layer = buxton_string_pack("base");
	key.type = STRING;
	r = buxton_direct_set_value(&daemon.buxton, &key,
				    &value1, NULL);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, 0, &status);
	fail_if(status != 0,
		"Failed to register notification for notify");
	buxtond_notify_clients(&daemon, &cl, &key, &value1);

	value2.type = STRING;
	value2.store.d_string = buxton_string_pack("new value");
	buxtond_notify_clients(&daemon, &cl, &key, &value2);

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1,
		"Failed to get correct response to notify string");
	fail_if(msg != BUXTON_CONTROL_CHANGED,
		"Failed to get correct control type");
	fail_if(msgid != 0, "Failed to get correct message id");
	fail_if(list[0].type != STRING,
		"Failed to get correct notification value type string");
	fail_if(!streq(list[0].store.d_string.value, "new value"),
		"Failed to get correct notification value data string");

	free(list[0].store.d_string.value);
	free(list);

	key.group = buxton_string_pack("group");
	key.name.value = NULL;
	key.name.length = 0;
	r = buxton_direct_create_group(&daemon.buxton, &key, NULL);
	fail_if(!r, "Unable to create group");
	r = buxton_direct_set_label(&daemon.buxton, &key, &slabel);
	fail_if(!r, "Unable set group label");

	value1.type = INT32;
	value1.store.d_int32 = 1;
	value2.type = INT32;
	value2.store.d_int32 = 2;
	key.group = buxton_string_pack("group");
	key.name = buxton_string_pack("name32");
	key.type = INT32;
	r = buxton_direct_set_value(&daemon.buxton, &key,
				    &value1, NULL);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, 0, &status);
	fail_if(status != 0,
		"Failed to register notification for notify");
	buxtond_notify_clients(&daemon, &cl, &key, &value2);

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1,
		"Failed to get correct response to notify int32");
	fail_if(msg != BUXTON_CONTROL_CHANGED,
		"Failed to get correct control type");
	fail_if(msgid != 0, "Failed to get correct message id");
	fail_if(list[0].type != INT32,
		"Failed to get correct notification value type int32");
	fail_if(list[0].store.d_int32 != 2,
		"Failed to get correct notification value data int32");

	free(list);

	value1.type = UINT32;
	value1.store.d_uint32 = 1;
	value2.type = UINT32;
	value2.store.d_uint32 = 2;
	key.group = buxton_string_pack("group");
	key.name = buxton_string_pack("nameu32");
	key.type = UINT32;
	r = buxton_direct_set_value(&daemon.buxton, &key,
				    &value1, NULL);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, 0, &status);
	fail_if(status != 0,
		"Failed to register notification for notify");
	buxtond_notify_clients(&daemon, &cl, &key, &value2);

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1,
		"Failed to get correct response to notify uint32");
	fail_if(msg != BUXTON_CONTROL_CHANGED,
		"Failed to get correct control type");
	fail_if(msgid != 0, "Failed to get correct message id");
	fail_if(list[0].type != UINT32,
		"Failed to get correct notification value type uint32");
	fail_if(list[0].store.d_uint32 != 2,
		"Failed to get correct notification value data uint32");

	free(list);

	value1.type = INT64;
	value1.store.d_int64 = 2;
	value2.type = INT64;
	value2.store.d_int64 = 3;
	key.group = buxton_string_pack("group");
	key.name = buxton_string_pack("name64");
	key.type = INT64;
	r = buxton_direct_set_value(&daemon.buxton, &key,
				    &value1, NULL);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, 0, &status);
	fail_if(status != 0,
		"Failed to register notification for notify");
	buxtond_notify_clients(&daemon, &cl, &key, &value2);

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1,
		"Failed to get correct response to notify int64");
	fail_if(msg != BUXTON_CONTROL_CHANGED,
		"Failed to get correct control type");
	fail_if(msgid != 0, "Failed to get correct message id");
	fail_if(list[0].type != INT64,
		"Failed to get correct notification value type int 64");
	fail_if(list[0].store.d_int64 != 3,
		"Failed to get correct notification value data int64");

	free(list);

	value1.type = UINT64;
	value1.store.d_uint64 = 2;
	value2.type = UINT64;
	value2.store.d_uint64 = 3;
	key.group = buxton_string_pack("group");
	key.name = buxton_string_pack("nameu64");
	key.type = UINT64;
	r = buxton_direct_set_value(&daemon.buxton, &key,
				    &value1, NULL);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, 0, &status);
	fail_if(status != 0,
		"Failed to register notification for notify");
	buxtond_notify_clients(&daemon, &cl, &key, &value2);

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1,
		"Failed to get correct response to notify uint64");
	fail_if(msg != BUXTON_CONTROL_CHANGED,
		"Failed to get correct control type");
	fail_if(msgid != 0, "Failed to get correct message id");
	fail_if(list[0].type != UINT64,
		"Failed to get correct notification value type uint64");
	fail_if(list[0].store.d_uint64 != 3,
		"Failed to get correct notification value data uint64");

	free(list);

	value1.type = FLOAT;
	value1.store.d_float = 3.1F;
	value2.type = FLOAT;
	value2.store.d_float = 3.14F;
	key.group = buxton_string_pack("group");
	key.name = buxton_string_pack("namef");
	key.type = FLOAT;
	r = buxton_direct_set_value(&daemon.buxton, &key,
				    &value1, NULL);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, 0, &status);
	fail_if(status != 0,
		"Failed to register notification for notify");
	buxtond_notify_clients(&daemon, &cl, &key, &value2);

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1,
		"Failed to get correct response to notify float");
	fail_if(msg != BUXTON_CONTROL_CHANGED,
		"Failed to get correct control type");
	fail_if(msgid != 0, "Failed to get correct message id");
	fail_if(list[0].type != FLOAT,
		"Failed to get correct notification value type float");
	fail_if(list[0].store.d_float != 3.14F,
		"Failed to get correct notification value data float");

	free(list);

	value1.type = DOUBLE;
	value1.store.d_double = 3.141F;
	value2.type = DOUBLE;
	value2.store.d_double = 3.1415F;
	key.group = buxton_string_pack("group");
	key.name = buxton_string_pack("named");
	key.type = DOUBLE;
	r = buxton_direct_set_value(&daemon.buxton, &key,
				    &value1, NULL);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, 0, &status);
	fail_if(status != 0,
		"Failed to register notification for notify");
	buxtond_notify_clients(&daemon, &cl, &key, &value2);

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1,
		"Failed to get correct response to notify double");
	fail_if(msg != BUXTON_CONTROL_CHANGED,
		"Failed to get correct control type");
	fail_if(msgid != 0, "Failed to get correct message id");
	fail_if(list[0].type != DOUBLE,
		"Failed to get correct notification value type double");
	fail_if(list[0].store.d_double != 3.1415F,
		"Failed to get correct notification value data double");

	free(list);

	value1.type = BOOLEAN;
	value1.store.d_boolean = false;
	value2.type = BOOLEAN;
	value2.store.d_int32 = true;
	key.group = buxton_string_pack("group");
	key.name = buxton_string_pack("nameb");
	key.type = BOOLEAN;
	r = buxton_direct_set_value(&daemon.buxton, &key,
				    &value1, NULL);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, 0, &status);
	fail_if(status != 0,
		"Failed to register notification for notify");
	buxtond_notify_clients(&daemon, &cl, &key, &value2);

	s = read(client, buf, 4096);
	fail_if(s < 0, "Read from client failed");
	csize = buxton_deserialize_message(buf, &msg, (size_t)s, &msgid, &list);
	fail_if(csize != 1,
		"Failed to get correct response to notify bool");
	fail_if(msg != BUXTON_CONTROL_CHANGED,
		"Failed to get correct control type");
	fail_if(msgid != 0, "Failed to get correct message id");
	fail_if(list[0].type != BOOLEAN,
		"Failed to get correct notification value type bool");
	fail_if(list[0].store.d_boolean != true,
		"Failed to get correct notification value data bool");

	free(list);
	close(client);
	buxton_direct_close(&daemon.buxton);
}
END_TEST

START_TEST(identify_client_check)
{
	int sender;
	client_list_item client;
	bool r;
	int32_t msg = 5;

	setup_socket_pair(&client.fd, &sender);
	r = identify_client(&client);
	fail_if(r, "Identified client without message");

	write(sender, &msg, sizeof(int32_t));
	r = identify_client(&client);
	fail_if(!r, "Identify client failed");

	close(client.fd);
	close(sender);
}
END_TEST

START_TEST(add_pollfd_check)
{
	BuxtonDaemon daemon;
	int fd;
	short events;
	bool a;

	fd = 3;
	daemon.nfds_alloc = 0;
	daemon.accepting_alloc = 0;
	daemon.nfds = 0;
	daemon.pollfds = NULL;
	daemon.accepting = NULL;
	events = 1;
	a = true;
	add_pollfd(&daemon, fd, events, a);
	fail_if(daemon.nfds != 1, "Failed to increase nfds");
	fail_if(daemon.pollfds[0].fd != fd, "Failed to set pollfd");
	fail_if(daemon.pollfds[0].events != events, "Failed to set events");
	fail_if(daemon.pollfds[0].revents != 0, "Failed to set revents");
	fail_if(daemon.accepting[0] != a, "Failed to set accepting status");
	free(daemon.pollfds);
	free(daemon.accepting);
}
END_TEST

START_TEST(del_pollfd_check)
{
	BuxtonDaemon daemon;
	int fd;
	short events;
	bool a;

	fd = 3;
	daemon.nfds_alloc = 0;
	daemon.accepting_alloc = 0;
	daemon.nfds = 0;
	daemon.pollfds = NULL;
	daemon.accepting = NULL;
	events = 1;
	a = true;
	add_pollfd(&daemon, fd, events, a);
	fail_if(daemon.nfds != 1, "Failed to add pollfd");
	del_pollfd(&daemon, 0);
	fail_if(daemon.nfds != 0, "Failed to decrease nfds 1");

	fd = 4;
	events = 2;
	a = false;
	add_pollfd(&daemon, fd, events, a);
	fail_if(daemon.nfds != 1, "Failed to increase nfds after del");
	fail_if(daemon.pollfds[0].fd != fd, "Failed to set pollfd after del");
	fail_if(daemon.pollfds[0].events != events,
		"Failed to set events after del");
	fail_if(daemon.pollfds[0].revents != 0,
		"Failed to set revents after del");
	fail_if(daemon.accepting[0] != a,
		"Failed to set accepting status after del");
	fd = 5;
	events = 3;
	a = true;
	add_pollfd(&daemon, fd, events, a);
	del_pollfd(&daemon, 0);
	fail_if(daemon.nfds != 1, "Failed to delete fd 2");
	fail_if(daemon.pollfds[0].fd != fd, "Failed to set pollfd after del2");
	fail_if(daemon.pollfds[0].events != events,
		"Failed to set events after del2");
	fail_if(daemon.pollfds[0].revents != 0,
		"Failed to set revents after del2");
	fail_if(daemon.accepting[0] != a,
		"Failed to set accepting status after del2");
}
END_TEST

START_TEST(handle_smack_label_check)
{
	client_list_item client;
	int server;

	setup_socket_pair(&client.fd, &server);
	handle_smack_label(&client);

	close(client.fd);
	close(server);
}
END_TEST

START_TEST(terminate_client_check)
{
	client_list_item *client;
	BuxtonDaemon daemon;
	int dummy;

	client = malloc0(sizeof(client_list_item));
	fail_if(!client, "client malloc failed");
	client->smack_label = malloc0(sizeof(BuxtonString));
	fail_if(!client->smack_label, "smack label malloc failed");
	daemon.client_list = client;
	setup_socket_pair(&client->fd, &dummy);
	daemon.nfds_alloc = 0;
	daemon.accepting_alloc = 0;
	daemon.nfds = 0;
	daemon.pollfds = NULL;
	daemon.accepting = NULL;
	add_pollfd(&daemon, client->fd, 2, false);
	fail_if(daemon.nfds != 1, "Failed to add pollfd");
	client->smack_label->value = strdup("dummy");
	client->smack_label->length = 6;
	fail_if(!client->smack_label->value, "label strdup failed");

	terminate_client(&daemon, client, 0);
	fail_if(daemon.client_list, "Failed to set client list item to NULL");
	close(dummy);
}
END_TEST

START_TEST(handle_client_check)
{
	BuxtonDaemon daemon;
	int dummy;
	uint8_t buf[4096];
	uint8_t *message = NULL;
	BuxtonData data1, data2, data3, data4;
	BuxtonArray *list = NULL;
	bool r;
	size_t ret;
	uint32_t bsize;

	list = buxton_array_new();
	data1.type = STRING;
	data1.store.d_string = buxton_string_pack("test-gdbm-user");
	data2.type = STRING;
	data2.store.d_string = buxton_string_pack("daemon-check");
	data3.type = STRING;
	data3.store.d_string = buxton_string_pack("name");
	data4.type = UINT32;
	data4.store.d_uint32 = STRING;
	r = buxton_array_add(list, &data1);
	fail_if(!r, "Failed to add data to array");
	r = buxton_array_add(list, &data2);
	fail_if(!r, "Failed to add data to array");
	r = buxton_array_add(list, &data3);
	fail_if(!r, "Failed to add data to array");
	r = buxton_array_add(list, &data4);
	fail_if(!r, "Failed to add data to array");
	ret = buxton_serialize_message(&message, BUXTON_CONTROL_GET, 0, list);
	fail_if(ret == 0, "Failed to serialize string data");
	daemon.client_list = malloc0(sizeof(client_list_item));
	fail_if(!daemon.client_list, "client malloc failed");
	setup_socket_pair(&daemon.client_list->fd, &dummy);
	fcntl(daemon.client_list->fd, F_SETFL, O_NONBLOCK);
	daemon.nfds_alloc = 0;
	daemon.accepting_alloc = 0;
	daemon.nfds = 0;
	daemon.pollfds = NULL;
	daemon.accepting = NULL;
	add_pollfd(&daemon, daemon.client_list->fd, 2, false);
	fail_if(daemon.nfds != 1, "Failed to add pollfd 1");
	fail_if(handle_client(&daemon, daemon.client_list, 0), "More data available 1");
	fail_if(daemon.client_list, "Failed to terminate client with no data");
	close(dummy);

	daemon.client_list = malloc0(sizeof(client_list_item));
	fail_if(!daemon.client_list, "client malloc failed");
	setup_socket_pair(&daemon.client_list->fd, &dummy);
	fcntl(daemon.client_list->fd, F_SETFL, O_NONBLOCK);
	add_pollfd(&daemon, daemon.client_list->fd, 2, false);
	fail_if(daemon.nfds != 1, "Failed to add pollfd 2");
	write(dummy, buf, 1);
	fail_if(handle_client(&daemon, daemon.client_list, 0), "More data available 2");
	fail_if(!daemon.client_list, "Terminated client with insufficient data");
	fail_if(daemon.client_list->data, "Didn't clean up left over client data 1");

	bsize = 0;
	memcpy(message + BUXTON_LENGTH_OFFSET, &bsize, sizeof(uint32_t));
	write(dummy, message, BUXTON_MESSAGE_HEADER_LENGTH);
	fail_if(handle_client(&daemon, daemon.client_list, 0), "More data available 3");
	fail_if(daemon.client_list, "Failed to terminate client with bad size 1");
	close(dummy);

	daemon.client_list = malloc0(sizeof(client_list_item));
	fail_if(!daemon.client_list, "client malloc failed");
	setup_socket_pair(&daemon.client_list->fd, &dummy);
	fcntl(daemon.client_list->fd, F_SETFL, O_NONBLOCK);
	add_pollfd(&daemon, daemon.client_list->fd, 2, false);
	fail_if(daemon.nfds != 1, "Failed to add pollfd 3");
	bsize = BUXTON_MESSAGE_MAX_LENGTH + 1;
	memcpy(message + BUXTON_LENGTH_OFFSET, &bsize, sizeof(uint32_t));
	write(dummy, message, BUXTON_MESSAGE_HEADER_LENGTH);
	fail_if(handle_client(&daemon, daemon.client_list, 0), "More data available 4");
	fail_if(daemon.client_list, "Failed to terminate client with bad size 2");
	close(dummy);

	daemon.client_list = malloc0(sizeof(client_list_item));
	fail_if(!daemon.client_list, "client malloc failed");
	setup_socket_pair(&daemon.client_list->fd, &dummy);
	fcntl(daemon.client_list->fd, F_SETFL, O_NONBLOCK);
	add_pollfd(&daemon, daemon.client_list->fd, 2, false);
	fail_if(daemon.nfds != 1, "Failed to add pollfd 4");
	bsize = (uint32_t)ret;
	memcpy(message + BUXTON_LENGTH_OFFSET, &bsize, sizeof(uint32_t));
	write(dummy, message, ret);
	fail_if(handle_client(&daemon, daemon.client_list, 0), "More data available 5");
	fail_if(!daemon.client_list, "Terminated client with correct data length");

	for (int i = 0; i < 33; i++) {
		write(dummy, message, ret);
	}
	fail_if(!handle_client(&daemon, daemon.client_list, 0), "No more data available");
	fail_if(!daemon.client_list, "Terminated client with correct data length");
	terminate_client(&daemon, daemon.client_list, 0);
	fail_if(daemon.client_list, "Failed to remove client 1");
	close(dummy);

	//FIXME add SIGPIPE handler
	/* daemon.client_list = malloc0(sizeof(client_list_item)); */
	/* fail_if(!daemon.client_list, "client malloc failed"); */
	/* setup_socket_pair(&daemon.client_list->fd, &dummy); */
	/* fcntl(daemon.client_list->fd, F_SETFL, O_NONBLOCK); */
	/* add_pollfd(&daemon, daemon.client_list->fd, 2, false); */
	/* fail_if(daemon.nfds != 1, "Failed to add pollfd 5"); */
	/* write(dummy, message, ret); */
	/* close(dummy); */
	/* fail_if(handle_client(&daemon, daemon.client_list, 0), "More data available 6"); */
	/* fail_if(daemon.client_list, "Failed to terminate client"); */
}
END_TEST

START_TEST(buxtond_eat_garbage_check)
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
	if (pid) {		/* parent*/
		BuxtonClient c;
		FuzzContext fuzz;
		time_t start;
		bool keep_going = true;
		int fd;


		srand(0);
		bzero(&fuzz, sizeof(FuzzContext));

		daemon_pid = pid;
		usleep(250*1000);
		check_did_not_crash(daemon_pid, &fuzz);


		fail_if(time(&start) == -1, "call to time() failed");
		do {
			pid_t client;
			ssize_t bytes;
			time_t now;

			fail_if(time(&now) == -1, "call to time() failed");
			if (now - start >= fuzz_time) {
				keep_going = false;
			}

			fuzz.size = (unsigned int)rand() % 4096;
			for (int i=0; i < fuzz.size; i++) {
				fuzz.buf[i] = (uint8_t)(rand() % 255);
			}
			if ((fuzz.size >= 6) && (rand() % 4096)) {
				uint16_t control = (uint16_t)((rand() % (BUXTON_CONTROL_MAX-1)) + 1);

				/* magic */
				fuzz.buf[0] = 0x06;
				fuzz.buf[1] = 0x72;

				/* valid message type */
				memcpy((void*)(fuzz.buf + 2), (void*)(&control), sizeof(uint16_t));

				/* valid size */
				memcpy((void *)(fuzz.buf + 4), (void *)(&fuzz.size), sizeof(uint32_t));
			}
			client = fork();
			fail_if(client == -1, "couldn't fork");
			if (client == 0) {
				fd = buxton_open(&c);
				fail_if(fd == -1,
					"Open failed with daemon%s", dump_fuzz(&fuzz));


				bytes = write(fd, (void*)(fuzz.buf), fuzz.size);
				fail_if(bytes == -1, "write failed: %m%s", dump_fuzz(&fuzz));
				fail_unless(bytes == fuzz.size, "write was %d instead of %d", bytes, fuzz.size);

				buxton_close(c);
				usleep(1*1000);

				check_did_not_crash(daemon_pid, &fuzz);
				exit(0);
			} else {
				int status;
				pid_t wait = waitpid(client, &status, 0);
				fail_if(wait == -1, "waitpid failed");
				fail_unless(WIFEXITED(status), "client died");
				fuzz.iteration++;
			}
		} while (keep_going);
	} else {		/* child */
		exec_daemon();
	}
}
END_TEST

BuxtonClient c;

void cleanup(void)
{
	//used to cleanup the helper groups, values, names that are needed by other fuzzed values/labels

	fail_if(buxton_open(&c) == -1, "Cleanup: Open failed with daemon.");

	BuxtonKey key = buxton_key_create("tempgroup", NULL, "base", STRING);
	fail_if(buxton_remove_group(c, key, NULL, "tempgroup", true), "Cleanup: Error at removing");
	buxton_key_free(key);
	buxton_close(c);

}

char *random_string(int str_size)
{
	//generates random strings of maximum str_size characters
	//ignore this for now, it will be replaced by something more intelligent

	int size;
	char *str;

	size = rand() % str_size;
	str = calloc((size_t)(size + 1), sizeof(char));

	for (int i = 0; i < size ; i++) {
		str[i] = (char)(rand() % 255 + 1); //1-255, use % 25+97 to use lower-case alpha chars
	}

	return str;
}

static void SIGPIPE_handler(int signo)
{
	buxton_close(c);

	//reconnect
	fail_if(buxton_open(&c) == -1,"SIGPIPE: Open failed with daemon.");
}

START_TEST(buxtond_fuzz_commands)
{
/*	Previous fuzzer had correct MAGIC, CONTROL code, Message SIZE and randomized everything else.
 *	This only randomizes the Data Value.
 *
 *	If you want to fuzz the protocol, use the above fuzzer.
 *	If you want to fuzz the daemon, use this one. Use export BUXTON_FUZZER=NEW
 */

	daemon_pid = 0;
	pid_t pid;
	sigset_t sigset;
	struct sigaction sa;
	char *random_group, *random_layer, *random_label, *random_value, *random_name;
	int max_length = 32768;
	unlink(buxton_socket());

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigprocmask(SIG_BLOCK, &sigset, NULL);

	// since the daemon will close the connection with the client if it receives a "weird" command
	// we have to treat SIGPIPE
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIGPIPE_handler;
	sigaction(SIGPIPE, &sa, NULL);

	printf("============== CAUTION!!! Fuzzer at work =================\n");
	FILE *f = fopen("debug_check_daemon.txt", "w");
	fclose(f);

	pid = fork();
	fail_if(pid < 0, "couldn't fork");
	if (pid) {		/* parent*/
		FuzzContext fuzz;
		time_t start;
		bool keep_going = true;

		srand((unsigned int) time(NULL));
		bzero(&fuzz, sizeof(FuzzContext));

		usleep(250*1000); //wait for daemon to start

		fail_if(time(&start) == -1, "call to time() failed");
		do {
			BuxtonKey key = NULL;
			time_t now;

			fail_if(time(&now) == -1, "call to time() failed");
			if (now - start >= fuzz_time) {
				keep_going = false;
			}

			cleanup();

			/* create a random group and layer */
			random_group = random_string(max_length);
			random_layer = random_string(max_length);

			key = buxton_key_create(random_group, NULL, random_layer, STRING);
			fail_if(!key, "Failed to create key");
			fail_if(buxton_open(&c) == -1, "Open failed with daemon.");

			f = fopen("debug_check_daemon.txt", "w");
			fail_if(!f, "Unable to open file\n");
			fprintf(f, "Create group: Group: %s\t Layer: %s\n", random_group, random_layer);
			fflush(f);

			if (buxton_create_group(c, key, NULL, random_group, true)) {
				fprintf(f, "1: Group created!\n");
			} else {
				fprintf(f, "1: Group was NOT created.\n");
			}
			fflush(f);
			buxton_key_free(key);

			/* create a random group in an existing layer */
			key = buxton_key_create(random_group, NULL, "base", STRING);
			fail_if(!key, "Failed to create key");
			fprintf(f, "Create group: Group: %s\t Layer: base\n", random_group);
			fflush(f);

			if (buxton_create_group(c, key, NULL, random_group, true)) {
				fprintf(f, "1: Group created!\n");
			} else {
				fprintf(f, "1: Group was NOT created.\n");
			}
			fflush(f);
			buxton_key_free(key);

			// create a random name on random group on a random layer
			random_name = random_string(max_length);
			key = buxton_key_create(random_group, random_name, random_layer, STRING);
			fail_if(!key, "Failed to create key");

			fprintf(f, "Create name: Group: %s\t Layer: %s\t Name:%s\n", random_group, random_layer, random_name);
			fflush(f);

			if (buxton_create_group(c, key, NULL, random_group, true)) {
				fprintf(f, "2: Name created!\n");
			} else {
				fprintf(f, "2: Name was NOT created.\n");
			}
			fflush(f);
			buxton_key_free(key);

			// create a random name on existing group
			// create group
			key = buxton_key_create("tempgroup", NULL, "base", STRING);
			fail_if(!key, "Failed to create key");
			fail_if(buxton_create_group(c, key, NULL,"tempgroup", true), "Creating group in buxton failed.");
			buxton_key_free(key);

			// put name on group
			key = buxton_key_create("tempgroup", random_name, "base", STRING);
			fail_if(!key, "Failed to create key");
			fprintf(f, "Create name: Group: tempgroup\t Layer: base\t Name: %s\n", random_name);

			if (buxton_create_group(c, key, NULL, "tempgroup", true)) {
				fprintf(f, "2: Name created!\n");
			} else {
				fprintf(f, "2: Name was NOT created.\n");
			}
			fflush(f);
			buxton_key_free(key);

			// create a random value on a existing labeled group
			//create the "existing" group, plus the name
			random_value = random_string(max_length);
			BuxtonKey group = buxton_key_create("tempgroup", NULL, "base", STRING);
			fail_if(!group, "Failed to create key for group");
			key = buxton_key_create("tempgroup", "name", "base", STRING);
			fail_if(!key, "Failed to create key");

			// set the a correct label and randomized value
			fprintf(f, "Set label: Group: tgroup\t Layer: base\t Value: %s\n", random_value);
			fflush(f);
			fail_if(buxton_set_label(c, group, "*", NULL, NULL, true), "Setting label in buxton failed.");

			if (buxton_set_value(c, key, random_value, NULL, "tgroup", true)) {
				fprintf(f, "3: Value was set!\n");
			} else {
				fprintf(f, "3: Value was NOT set.\n");
			}
			fflush(f);
			buxton_key_free(group);
			buxton_key_free(key);

			//set a random label on an existing group
			random_label = random_string(3);
			group = buxton_key_create("tempgroup", NULL, "base", STRING);
			fail_if(!group, "Failed to create key for group");

			fprintf(f, "Set label: Group: tempgroup\t Layer: base\t Label: %s\n", random_label);
			if (buxton_set_label(c, group, random_label, NULL, group, true)) {
				fprintf(f, "3: Label was set!\n");
			} else {
				fprintf(f, "3: Label was NOT set.\n");
			}
			fflush(f);

			//set random value/label on name
			BuxtonKey name = buxton_key_create("tempgroup", "name", "base", STRING);
			fail_if(!name, "Failed to create key for name");

			fprintf(f, "Set label and value: Group: tempgroup\t Layer: base\t Name: name\t Value: %s\t Label: %s \n", random_value, random_label);
			if (buxton_set_value(c, name, random_value, NULL, NULL, true)) {
				fprintf(f, "4: Value on name was set!\n");
			} else {
				fprintf(f, "4: Value on name  was NOT set.\n");
			}
			free(random_value);
			fflush(f);

			if (buxton_set_label(c, name, random_label, NULL, name, true)) {
				fprintf(f, "4: Label on name was set!\n");
			} else {
				fprintf(f, "4: Label on name was NOT set.\n");
			}
			fflush(f);
			buxton_key_free(group);
			buxton_key_free(name);
			free(random_label);

			// remove name from group
			key = buxton_key_create(random_group, random_name, random_layer, STRING);
			fprintf(f, "Remove group: Group: %s\t Layer: %s\t Name:%s\n", random_group, random_layer, random_name);
			if (buxton_remove_group(c, key, NULL, random_group, true)) {
				fprintf(f, "5: Name from group was removed!\n");
			} else {
				fprintf(f, "5: Name from group was NOT removed.\n");
			}
			fflush(f);
			buxton_key_free(key);

			// remove name from existing group
			key = buxton_key_create("tempgroup", random_name, "base", STRING);
			fprintf(f, "Remove group: Group: tempgroup\t Layer: base\t Name:%s\n", random_name);
			if (buxton_remove_group(c, key, NULL, "tempgroup", true)) {
				fprintf(f, "5: Name from group was removed!\n");
			} else {
				fprintf(f, "5: Name from group was NOT removed.\n");
			}
			fflush(f);
			free(random_name);
			buxton_key_free(key);

			/* remove group from existing layer*/
			key = buxton_key_create(random_group, NULL, "base", STRING);
			fail_if(!key, "Failed to create key");
			fprintf(f, "Remove group: Group: %s\t Layer: base\n", random_group);
			if (buxton_remove_group(c, key, NULL, random_group, true)) {
				fprintf(f, "5: Group was removed!\n");
			} else {
				fprintf(f, "5: Group was NOT removed.\n");
			}
			fflush(f);
			buxton_key_free(key);

			/* remove group */
			key = buxton_key_create(random_group, NULL, random_layer, STRING);
			fail_if(!key, "Failed to create key");
			fprintf(f, "Remove group: Group: %s\t Layer: %s\n", random_group, random_layer);
			if (buxton_remove_group(c, key, NULL, random_group, true)) {
				fprintf(f, "5: Group was removed!\n");
			} else {
				fprintf(f, "5: Group was NOT removed.\n");
			}
			buxton_key_free(key);
			fflush(f);

			buxton_close(c);
			usleep(1*1000);

			fprintf(f, "5: Closed comm.\n");
			fclose(f);
			free(random_layer);
			free(random_group);

			reap_callbacks();
		} while (keep_going);
	} else {		/* child */
		exec_daemon();
	}

	usleep(3 * 1000);
	reap_callbacks();
}
END_TEST

static Suite *
daemon_suite(void)
{
	Suite *s;
	TCase *tc;
	char *fuzzer_engine;

	s = suite_create("daemon");
	tc = tcase_create("daemon test functions");
	tcase_add_checked_fixture(tc, setup, teardown);
	tcase_add_test(tc, buxton_open_check);
	tcase_add_test(tc, buxton_create_group_check);
	tcase_add_test(tc, buxton_remove_group_check);
	tcase_add_test(tc, buxton_set_value_check);
	tcase_add_test(tc, buxton_set_label_check);
	tcase_add_test(tc, buxton_get_value_for_layer_check);
	tcase_add_test(tc, buxton_get_value_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("buxton_daemon_functions");
	tcase_add_test(tc, parse_list_check);
	tcase_add_test(tc, create_group_check);
	tcase_add_test(tc, remove_group_check);
	tcase_add_test(tc, set_label_check);

	tcase_add_test(tc, set_value_check);
	tcase_add_test(tc, get_value_check);
	tcase_add_test(tc, register_notification_check);
	tcase_add_test(tc, buxtond_handle_message_error_check);
	tcase_add_test(tc, buxtond_handle_message_create_group_check);
	tcase_add_test(tc, buxtond_handle_message_remove_group_check);
	tcase_add_test(tc, buxtond_handle_message_set_label_check);
	tcase_add_test(tc, buxtond_handle_message_set_value_check);
	tcase_add_test(tc, buxtond_handle_message_get_check);
	tcase_add_test(tc, buxtond_handle_message_notify_check);
	tcase_add_test(tc, buxtond_handle_message_unset_check);
	tcase_add_test(tc, buxtond_notify_clients_check);
	tcase_add_test(tc, identify_client_check);
	tcase_add_test(tc, add_pollfd_check);
	tcase_add_test(tc, del_pollfd_check);
	tcase_add_test(tc, handle_smack_label_check);
	tcase_add_test(tc, terminate_client_check);
	tcase_add_test(tc, handle_client_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("buxton daemon evil tests");
	tcase_add_checked_fixture(tc, NULL, teardown);
	fuzzer_engine = getenv("BUXTON_FUZZER");
	if (fuzzer_engine) {
		if (strcmp(fuzzer_engine,"NEW") == 0) {
			tcase_add_test(tc, buxtond_fuzz_commands);
		} else {
			tcase_add_test(tc, buxtond_eat_garbage_check);
		}
	} else {
		tcase_add_test(tc, buxtond_eat_garbage_check);
	}
	tcase_set_timeout(tc, fuzz_time+2);
	suite_add_tcase(s, tc);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;
	char *fuzzenv;

	putenv("BUXTON_CONF_FILE=" ABS_TOP_BUILDDIR "/test/test.conf");
	putenv("BUXTON_ROOT_CHECK=0");
	fuzzenv = getenv("BUXTON_FUZZ_TIME");
	if (fuzzenv) {
		fuzz_time = atoi(fuzzenv);
	} else {
		fuzz_time = 2;
	}
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
