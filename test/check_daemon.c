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
#include "check_utils.h"
#include "daemon.h"
#include "log.h"
#include "smack.h"
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

	fail_if(parse_list(BUXTON_CONTROL_NOTIFY, 2, l1, &key, &layer, &value),
		"Parsed bad notify argument count");
	l1[0].type = INT32;
	fail_if(parse_list(BUXTON_CONTROL_NOTIFY, 1, l1, &key, &layer, &value),
		"Parsed bad notify type");
	l1[0].type = STRING;
	l1[0].store.d_string = buxton_string_pack("s1");
	fail_if(!parse_list(BUXTON_CONTROL_NOTIFY, 1, l1, &key, &layer, &value),
		"Unable to parse valid notify");
	fail_if(!streq(key->value, l1[0].store.d_string.value),
		"Failed to set correct notify key");

	fail_if(parse_list(BUXTON_CONTROL_UNNOTIFY, 2, l1, &key, &layer, &value),
		"Parsed bad unnotify argument count");
	l1[0].type = INT32;
	fail_if(parse_list(BUXTON_CONTROL_UNNOTIFY, 1, l1, &key, &layer, &value),
		"Parsed bad unnotify type");
	l1[0].type = STRING;
	l1[0].store.d_string = buxton_string_pack("s1");
	fail_if(!parse_list(BUXTON_CONTROL_UNNOTIFY, 1, l1, &key, &layer, &value),
		"Unable to parse valid unnotify");
	fail_if(!streq(key->value, l1[0].store.d_string.value),
		"Failed to set correct unnotify key");

	fail_if(parse_list(BUXTON_CONTROL_GET, 3, l2, &key, &layer, &value),
		"Parsed bad get argument count");
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
	l2[0].type = INT32;
	fail_if(parse_list(BUXTON_CONTROL_GET, 1, l2, &key, &layer, &value),
		"Unable to parse valid get 2");

	fail_if(parse_list(BUXTON_CONTROL_SET, 1, l3, &key, &layer, &value),
		"Parsed bad set argument count");
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

	fail_if(parse_list(BUXTON_CONTROL_UNSET, 1, l2, &key, &layer, &value),
		"Parsed bad unset argument count");
	l2[0].type = INT32;
	l2[1].type = STRING;
	fail_if(parse_list(BUXTON_CONTROL_UNSET, 2, l2, &key, &layer, &value),
		"Parsed bad unset type 1");
	l2[0].type = STRING;
	l2[1].type = INT32;
	fail_if(parse_list(BUXTON_CONTROL_UNSET, 2, l2, &key, &layer, &value),
		"Parsed bad unset type 2");
	l2[0].type = STRING;
	l2[1].type = STRING;
	l2[0].store.d_string = buxton_string_pack("s6");
	l2[1].store.d_string = buxton_string_pack("s7");
	fail_if(!parse_list(BUXTON_CONTROL_UNSET, 2, l2, &key, &layer, &value),
		"Unable to parse valid get 1");
	fail_if(!streq(layer->value, l2[0].store.d_string.value),
		"Failed to set correct unset layer 1");
	fail_if(!streq(key->value, l2[1].store.d_string.value),
		"Failed to set correct unset key 1");
}
END_TEST

START_TEST(set_value_check)
{
	BuxtonString layer, key;
	BuxtonData value;
	client_list_item client;
	BuxtonStatus status;
	BuxtonDaemon server;
	BuxtonString clabel;

	fail_if(!buxton_direct_open(&server.buxton),
		"Failed to open buxton direct connection");

	client.cred.uid = getuid();
	client.smack_label = &clabel;
	server.buxton.client.uid = 0;
	layer = buxton_string_pack("test-gdbm");
	key = buxton_string_pack("key");
	value.type = FLOAT;
	value.store.d_float = 3.14F;
	value.label = buxton_string_pack("label");

	set_value(&server, &client, &layer, &key, &value, &status);
	fail_if(status != BUXTON_STATUS_OK, "Failed to set value");
	fail_if(server.buxton.client.uid != client.cred.uid, "Failed to change buxton uid");
	buxton_client_close(&server.buxton.client);
}
END_TEST

START_TEST(get_value_check)
{
	BuxtonString layer, key;
	BuxtonData *value;
	client_list_item client;
	BuxtonStatus status;
	BuxtonDaemon server;
	BuxtonString clabel;

	fail_if(!buxton_direct_open(&server.buxton),
		"Failed to open buxton direct connection");

	client.cred.uid = getuid();
	client.smack_label = &clabel;
	server.buxton.client.uid = 0;
	layer = buxton_string_pack("test-gdbm");
	key = buxton_string_pack("key");

	value = get_value(&server, &client, &layer, &key, &status);
	fail_if(!value, "Failed to get value");
	fail_if(status != BUXTON_STATUS_OK, "Failed to get value");
	fail_if(value->type != FLOAT, "Failed to get correct type");
	fail_if(!value->label.value, "Failed to get label");
	fail_if(!streq(value->label.value, "label"), "Failed to get correct label");
	fail_if(value->store.d_float != 3.14F, "Failed to get correct value");
	fail_if(server.buxton.client.uid != client.cred.uid, "Failed to change buxton uid");
	free(value->label.value);
	free(value);

	server.buxton.client.uid = 0;
	value = get_value(&server, &client, NULL, &key, &status);
	fail_if(!value, "Failed to get value 2");
	fail_if(status != BUXTON_STATUS_OK, "Failed to get value 2");
	fail_if(value->type != FLOAT, "Failed to get correct type 2");
	fail_if(!value->label.value, "Failed to get label 2");
	fail_if(!streq(value->label.value, "label"), "Failed to get correct label 2");
	fail_if(value->store.d_float != 3.14F, "Failed to get correct value 2");
	fail_if(server.buxton.client.uid != client.cred.uid, "Failed to change buxton uid 2");
	free(value->label.value);
	free(value);

	buxton_client_close(&server.buxton.client);
}
END_TEST

START_TEST(register_notification_check)
{
	BuxtonString key;
	client_list_item client, no_client;
	BuxtonString clabel;
	BuxtonStatus status;
	BuxtonDaemon server;

	client.smack_label = &clabel;
	fail_if(!buxton_direct_open(&server.buxton),
		"Failed to open buxton direct connection");
	server.notify_mapping = hashmap_new(string_hash_func, string_compare_func);
	fail_if(!server.notify_mapping, "Failed to allocate hashmap");

	key = buxton_string_pack("key");
	register_notification(&server, &client, &key, &status);
	fail_if(status != BUXTON_STATUS_OK, "Failed to register notification");
	register_notification(&server, &client, &key, &status);
	fail_if(status != BUXTON_STATUS_OK, "Failed to register notification");
	//FIXME: Figure out what to do with duplicates
	key = buxton_string_pack("no-key");
	unregister_notification(&server, &client, &key, &status);
	fail_if(status != BUXTON_STATUS_FAILED,
		"Unregistered from notifications with invalid key");
	key = buxton_string_pack("key");
	unregister_notification(&server, &no_client, &key, &status);
	fail_if(status != BUXTON_STATUS_FAILED,
		"Unregistered from notifications with invalid client");
	unregister_notification(&server, &client, &key, &status);
	fail_if(status != BUXTON_STATUS_OK,
		"Unable to unregister from notifications");
	key = buxton_string_pack("key2");
	register_notification(&server, &client, &key, &status);
	fail_if(status == BUXTON_STATUS_OK, "Registered notification with key not in db");

	hashmap_free(server.notify_mapping);
	buxton_client_close(&server.buxton.client);
}
END_TEST

START_TEST(bt_daemon_handle_message_error_check)
{
	int client, server;
	BuxtonDaemon daemon;
	BuxtonString *string;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1;
	client_list_item cl;
	bool r;
	uint16_t control;

	setup_socket_pair(&client, &server);

	cl.fd = server;
	slabel = buxton_string_pack("_");
	cl.smack_label = &slabel;
	daemon.buxton.client.uid = 1001;
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	buxton_direct_open(&daemon.buxton);

	cl.data = malloc(4);
	fail_if(!cl.data, "Couldn't allocate blank message");
	cl.data[0] = 0;
	cl.data[1]= 0;
	cl.data[2] = 0;
	cl.data[3] = 0;
	size = 100;
	r = bt_daemon_handle_message(&daemon, &cl, size);
	fail_if(r, "Failed to detect invalid message data");
	free(cl.data);

	data1.type = STRING;
	string = buxton_make_key("group", "name");
	fail_if(!string, "Failed to allocate key");
	data1.store.d_string.value = string->value;
	data1.store.d_string.length = string->length;
	data1.label = buxton_string_pack("dummy");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_NOTIFY, 1,
					&data1);
	fail_if(size == 0, "Failed to serialize message");
	control = BUXTON_CONTROL_MIN;
	memcpy(cl.data, &control, sizeof(uint16_t));
	r = bt_daemon_handle_message(&daemon, &cl, size);
	fail_if(r, "Failed to detect min control size");
	control = BUXTON_CONTROL_MAX;
	memcpy(cl.data, &control, sizeof(uint16_t));
	r = bt_daemon_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(r, "Failed to detect max control size");
	free(string);

	close(client);
	buxton_direct_close(&daemon.buxton);
}
END_TEST

START_TEST(bt_daemon_handle_message_set_check)
{
	BuxtonDaemon daemon;
	BuxtonString *string;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1, data2, data3;
	client_list_item cl;
	bool r;
	BuxtonClient bclient;
	BuxtonData *list;
	BuxtonControlMessage msg;
	size_t csize;
	int client, server;

	setup_socket_pair(&client, &server);
	cl.fd = server;
	slabel = buxton_string_pack("_");
	cl.smack_label = &slabel;
	daemon.buxton.client.uid = 1001;
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	buxton_direct_open(&daemon.buxton);

	data1.type = STRING;
	string = buxton_make_key("group", "name");
	fail_if(!string, "Failed to allocate key");
	data1.label = buxton_string_pack("base/sample/key");
	data1.store.d_string = buxton_string_pack("base");
	data2.type = STRING;
	data2.store.d_string.value = string->value;
	data2.store.d_string.length = string->length;
	data2.label = buxton_string_pack("base/sample/key");
	data3.type = INT32;
	data3.store.d_int32 = 1;
	data3.label = buxton_string_pack("base/sample/key");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_NOTIFY, 3,
					&data1, &data2, &data3);
	fail_if(size == 0, "Failed to serialize message");
	r = bt_daemon_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(r, "Failed to detect parse_list failure");

	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_SET, 3,
					&data1, &data2, &data3);
	fail_if(size == 0, "Failed to serialize message");
	r = bt_daemon_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to set message");
	free(string);

	bclient.fd = client;
	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 1, "Failed to get correct response to set");
	fail_if(list[0].store.d_int32 != BUXTON_STATUS_OK,
		"Failed to set");
	free(list[0].label.value);
	free(list);

	close(client);
	buxton_direct_close(&daemon.buxton);
}
END_TEST

START_TEST(bt_daemon_handle_message_get_check)
{
	int client, server;
	BuxtonDaemon daemon;
	BuxtonString *string;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1, data2;
	client_list_item cl;
	bool r;
	BuxtonClient bclient;
	BuxtonData *list;
	BuxtonControlMessage msg;
	size_t csize;

	setup_socket_pair(&client, &server);

	cl.fd = server;
	slabel = buxton_string_pack("_");
	cl.smack_label = &slabel;
	daemon.buxton.client.uid = 1001;
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	buxton_direct_open(&daemon.buxton);

	data1.type = STRING;
	string = buxton_make_key("group", "name");
	fail_if(!string, "Failed to allocate key");
	data1.label = buxton_string_pack("dummy");
	data1.store.d_string = buxton_string_pack("base");
	data2.type = STRING;
	data2.store.d_string.value = string->value;
	data2.store.d_string.length = string->length;
	data2.label = buxton_string_pack("dummy");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_GET, 2,
					&data1, &data2);
	fail_if(size == 0, "Failed to serialize message");
	r = bt_daemon_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to get message 1");

	bclient.fd = client;
	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 2, "Failed to get correct response to get 1");
	fail_if(list[0].store.d_int32 != BUXTON_STATUS_OK,
		"Failed to get 1");
	fail_if(list[1].store.d_int32 != 1,
		"Failed to get correct value 1");
	free(list[0].label.value);
	free(list[1].label.value);
	free(list);

	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_GET, 1,
					&data2);
	fail_if(size == 0, "Failed to serialize message");
	r = bt_daemon_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to get message 2");
	free(string);

	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 2, "Failed to get correct response to get 2");
	fail_if(list[0].store.d_int32 != BUXTON_STATUS_OK,
		"Failed to get 2");
	fail_if(list[1].store.d_int32 != 1,
		"Failed to get correct value 2");
	free(list[0].label.value);
	free(list[1].label.value);
	free(list);

	close(client);
	buxton_direct_close(&daemon.buxton);
}
END_TEST

START_TEST(bt_daemon_handle_message_notify_check)
{
	int client, server;
	BuxtonDaemon daemon;
	BuxtonString *string;
	BuxtonString slabel;
	size_t size;
	BuxtonData data2;
	client_list_item cl;
	bool r;
	BuxtonClient bclient;
	BuxtonData *list;
	BuxtonControlMessage msg;
	size_t csize;

	setup_socket_pair(&client, &server);

	cl.fd = server;
	slabel = buxton_string_pack("_");
	cl.smack_label = &slabel;
	daemon.buxton.client.uid = 1001;
	daemon.notify_mapping = hashmap_new(string_hash_func, string_compare_func);
	fail_if(!daemon.notify_mapping, "Failed to allocate hashmap");
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	buxton_direct_open(&daemon.buxton);

	string = buxton_make_key("group", "name");
	fail_if(!string, "Failed to allocate key");
	data2.type = STRING;
	data2.store.d_string.value = string->value;
	data2.store.d_string.length = string->length;
	data2.label = buxton_string_pack("dummy");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_NOTIFY, 1,
					&data2);
	fail_if(size == 0, "Failed to serialize message");
	r = bt_daemon_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to register for notification");
	free(data2.store.d_string.value);
	free(string);

	bclient.fd = client;
	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 1, "Failed to get correct response to notify");
	fail_if(list[0].store.d_int32 != BUXTON_STATUS_OK,
		"Failed to register notification");
	free(list[0].label.value);
	free(list);

	/* UNNOTIFY */
	string = buxton_make_key("group", "name");
	fail_if(!string, "Failed to allocate key");
	data2.type = STRING;
	data2.store.d_string.value = string->value;
	data2.store.d_string.length = string->length;
	data2.label = buxton_string_pack("dummy");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_UNNOTIFY, 1,
					&data2);
	fail_if(size == 0, "Failed to serialize message");
	r = bt_daemon_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to unregister from notification");
	free(data2.store.d_string.value);
	free(string);

	bclient.fd = client;
	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 1, "Failed to get correct response to unnotify");
	fail_if(list[0].store.d_int32 != BUXTON_STATUS_OK,
		"Failed to unregister from notification");
	free(list[0].label.value);
	free(list);

	close(client);
	hashmap_free(daemon.notify_mapping);
	buxton_direct_close(&daemon.buxton);
}
END_TEST

START_TEST(bt_daemon_handle_message_unset_check)
{
	int client, server;
	BuxtonDaemon daemon;
	BuxtonString *string;
	BuxtonString slabel;
	size_t size;
	BuxtonData data1, data2;
	client_list_item cl;
	bool r;
	BuxtonClient bclient;
	BuxtonData *list;
	BuxtonControlMessage msg;
	size_t csize;

	setup_socket_pair(&client, &server);

	cl.fd = server;
	slabel = buxton_string_pack("_");
	cl.smack_label = &slabel;
	daemon.buxton.client.uid = 1001;
	fail_if(!buxton_cache_smack_rules(), "Failed to cache Smack rules");
	buxton_direct_open(&daemon.buxton);

	data1.type = STRING;
	string = buxton_make_key("group", "name");
	fail_if(!string, "Failed to allocate key");
	data1.label = buxton_string_pack("dummy");
	data1.store.d_string = buxton_string_pack("base");
	data2.type = STRING;
	data2.store.d_string.value = string->value;
	data2.store.d_string.length = string->length;
	data2.label = buxton_string_pack("dummy");
	size = buxton_serialize_message(&cl.data, BUXTON_CONTROL_UNSET,
					2, &data1, &data2);
	fail_if(size == 0, "Failed to serialize message");
	r = bt_daemon_handle_message(&daemon, &cl, size);
	free(cl.data);
	fail_if(!r, "Failed to unset message");
	free(string);

	bclient.fd = client;
	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 1, "Failed to get correct response to unset");
	fail_if(list[0].store.d_int32 != BUXTON_STATUS_OK,
		"Failed to unset");
	free(list[0].label.value);
	free(list);

	close(client);
	buxton_direct_close(&daemon.buxton);
}
END_TEST

START_TEST(bt_daemon_notify_clients_check)
{
	int client, server;
	BuxtonDaemon daemon;
	BuxtonString key;
	BuxtonString slabel;
	BuxtonString layer;
	BuxtonString *string;
	BuxtonData value1, value2;
	client_list_item cl;
	BuxtonStatus status;
	bool r;
	BuxtonClient bclient;
	BuxtonData *list;
	BuxtonControlMessage msg;
	size_t csize;

	setup_socket_pair(&client, &server);

	cl.fd = server;
	slabel = buxton_string_pack("_");
	cl.smack_label = &slabel;
	daemon.notify_mapping = hashmap_new(string_hash_func,
					    string_compare_func);
	fail_if(!daemon.notify_mapping, "Failed to allocate hashmap");
	fail_if(!buxton_cache_smack_rules(),
		"Failed to cache Smack rules");
	string = buxton_make_key("group", "name");
	fail_if(!string, "Failed to allocate key");
	buxton_direct_open(&daemon.buxton);

	value1.type = STRING;
	value1.store.d_string = buxton_string_pack("dummy value");
	value1.label = buxton_string_pack("dummy");
	key = buxton_string_pack("dummy key");
	bt_daemon_notify_clients(&daemon, &cl, &key, &value1);

	value1.store.d_string = buxton_string_pack("real value");
	value1.label = buxton_string_pack("base/sample/key");
	key.value = string->value;
	key.length = string->length;
	layer = buxton_string_pack("base");
	r = buxton_direct_set_value(&daemon.buxton, &layer, &key,
				    &value1);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, &status);
	fail_if(status != BUXTON_STATUS_OK,
		"Failed to register notification for notify");
	bt_daemon_notify_clients(&daemon, &cl, &key, &value1);

	value2.type = STRING;
	value2.store.d_string = buxton_string_pack("new value");
	value2.label = buxton_string_pack("dummy2");
	bt_daemon_notify_clients(&daemon, &cl, &key, &value2);
	free(string->value);
	free(string);

	bclient.fd = client;
	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 2,
		"Failed to get correct response to notify string");
	fail_if(!streq(get_group(&list[0].store.d_string), "group"),
		"Failed to get correct notification group string");
	fail_if(!streq(get_name(&list[0].store.d_string), "name"),
		"Failed to get correct notification name string");
	fail_if(!streq(list[0].label.value, "dummy"),
		"Failed to get correct notification key label string");
	fail_if(!streq(list[1].label.value, "dummy2"),
		"Failed to get correct notification value label string");
	fail_if(list[1].type != STRING,
		"Failed to get correct notification value type string");
	fail_if(!streq(list[1].store.d_string.value, "new value"),
		"Failed to get correct notification value data string");
	free(list[0].label.value);
	free(list[0].store.d_string.value);
	free(list[1].label.value);
	free(list[1].store.d_string.value);
	free(list);

	value1.type = INT32;
	value1.store.d_int32 = 1;
	value2.type = INT32;
	value2.store.d_int32 = 2;
	string = buxton_make_key("group32", "name32");
	fail_if(!string, "Failed to allocate key");
	key.value = string->value;
	key.length = string->length;
	layer = buxton_string_pack("base");
	r = buxton_direct_set_value(&daemon.buxton, &layer, &key,
				    &value1);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, &status);
	fail_if(status != BUXTON_STATUS_OK,
		"Failed to register notification for notify");
	bt_daemon_notify_clients(&daemon, &cl, &key, &value2);
	free(string->value);
	free(string);

	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 2,
		"Failed to get correct response to notify int32");
	fail_if(!streq(get_group(&list[0].store.d_string), "group32"),
		"Failed to get correct notification group int32");
	fail_if(!streq(get_name(&list[0].store.d_string), "name32"),
		"Failed to get correct notification name int32");
	fail_if(!streq(list[0].label.value, "dummy"),
		"Failed to get correct notification key label int32");
	fail_if(!streq(list[1].label.value, "dummy2"),
		"Failed to get correct notification value label int32");
	fail_if(list[1].type != INT32,
		"Failed to get correct notification value type int32");
	fail_if(list[1].store.d_int32 != 2,
		"Failed to get correct notification value data int32");
	free(list[0].label.value);
	free(list[0].store.d_string.value);
	free(list[1].label.value);
	free(list);

	value1.type = INT64;
	value1.store.d_int64 = 2;
	value2.type = INT64;
	value2.store.d_int64 = 3;
	string = buxton_make_key("group64", "name64");
	fail_if(!string, "Failed to allocate key");
	key.value = string->value;
	key.length = string->length;
	layer = buxton_string_pack("base");
	r = buxton_direct_set_value(&daemon.buxton, &layer, &key,
				    &value1);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, &status);
	fail_if(status != BUXTON_STATUS_OK,
		"Failed to register notification for notify");
	bt_daemon_notify_clients(&daemon, &cl, &key, &value2);
	free(string->value);
	free(string);

	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 2,
		"Failed to get correct response to notify int64");
	fail_if(!streq(get_group(&list[0].store.d_string), "group64"),
		"Failed to get correct notification group int64");
	fail_if(!streq(get_name(&list[0].store.d_string), "name64"),
		"Failed to get correct notification name int64");
	fail_if(!streq(list[0].label.value, "dummy"),
		"Failed to get correct notification key label int64");
	fail_if(!streq(list[1].label.value, "dummy2"),
		"Failed to get correct notification value label int64");
	fail_if(list[1].type != INT64,
		"Failed to get correct notification value type int 64");
	fail_if(list[1].store.d_int64 != 3,
		"Failed to get correct notification value data int64");
	free(list[0].label.value);
	free(list[0].store.d_string.value);
	free(list[1].label.value);
	free(list);

	value1.type = FLOAT;
	value1.store.d_float = 3.1F;
	value2.type = FLOAT;
	value2.store.d_float = 3.14F;
	string = buxton_make_key("groupf", "namef");
	fail_if(!string, "Failed to allocate key");
	key.value = string->value;
	key.length = string->length;
	layer = buxton_string_pack("base");
	r = buxton_direct_set_value(&daemon.buxton, &layer, &key,
				    &value1);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, &status);
	fail_if(status != BUXTON_STATUS_OK,
		"Failed to register notification for notify");
	bt_daemon_notify_clients(&daemon, &cl, &key, &value2);
	free(string->value);
	free(string);

	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 2,
		"Failed to get correct response to notify float");
	fail_if(!streq(get_group(&list[0].store.d_string), "groupf"),
		"Failed to get correct notification group float");
	fail_if(!streq(get_name(&list[0].store.d_string), "namef"),
		"Failed to get correct notification name float");
	fail_if(!streq(list[0].label.value, "dummy"),
		"Failed to get correct notification key label float");
	fail_if(!streq(list[1].label.value, "dummy2"),
		"Failed to get correct notification value label float");
	fail_if(list[1].type != FLOAT,
		"Failed to get correct notification value type float");
	fail_if(list[1].store.d_float != 3.14F,
		"Failed to get correct notification value data float");
	free(list[0].label.value);
	free(list[0].store.d_string.value);
	free(list[1].label.value);
	free(list);

	value1.type = DOUBLE;
	value1.store.d_double = 3.141F;
	value2.type = DOUBLE;
	value2.store.d_double = 3.1415F;
	string = buxton_make_key("groupd", "named");
	fail_if(!string, "Failed to allocate key");
	key.value = string->value;
	key.length = string->length;
	layer = buxton_string_pack("base");
	r = buxton_direct_set_value(&daemon.buxton, &layer, &key,
				    &value1);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, &status);
	fail_if(status != BUXTON_STATUS_OK,
		"Failed to register notification for notify");
	bt_daemon_notify_clients(&daemon, &cl, &key, &value2);
	free(string->value);
	free(string);

	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 2,
		"Failed to get correct response to notify double");
	fail_if(!streq(get_group(&list[0].store.d_string), "groupd"),
		"Failed to get correct notification group double");
	fail_if(!streq(get_name(&list[0].store.d_string), "named"),
		"Failed to get correct notification name double");
	fail_if(!streq(list[0].label.value, "dummy"),
		"Failed to get correct notification key label double");
	fail_if(!streq(list[1].label.value, "dummy2"),
		"Failed to get correct notification value label double");
	fail_if(list[1].type != DOUBLE,
		"Failed to get correct notification value type double");
	fail_if(list[1].store.d_double != 3.1415F,
		"Failed to get correct notification value data double");
	free(list[0].label.value);
	free(list[0].store.d_string.value);
	free(list[1].label.value);
	free(list);

	value1.type = BOOLEAN;
	value1.store.d_boolean = false;
	value2.type = BOOLEAN;
	value2.store.d_int32 = true;
	string = buxton_make_key("groupb", "nameb");
	fail_if(!string, "Failed to allocate key");
	key.value = string->value;
	key.length = string->length;
	layer = buxton_string_pack("base");
	r = buxton_direct_set_value(&daemon.buxton, &layer, &key,
				    &value1);
	fail_if(!r, "Failed to set value for notify");
	register_notification(&daemon, &cl, &key, &status);
	fail_if(status != BUXTON_STATUS_OK,
		"Failed to register notification for notify");
	bt_daemon_notify_clients(&daemon, &cl, &key, &value2);
	free(string->value);
	free(string);

	csize = buxton_wire_get_response(&bclient, &msg, &list);
	fail_if(csize != 2,
		"Failed to get correct response to notify bool");
	fail_if(!streq(get_group(&list[0].store.d_string), "groupb"),
		"Failed to get correct notification group bool");
	fail_if(!streq(get_name(&list[0].store.d_string), "nameb"),
		"Failed to get correct notification name bool");
	fail_if(!streq(list[0].label.value, "dummy"),
		"Failed to get correct notification key label bool");
	fail_if(!streq(list[1].label.value, "dummy2"),
		"Failed to get correct notification value label bool");
	fail_if(list[1].type != BOOLEAN,
		"Failed to get correct notification value type bool");
	fail_if(list[1].store.d_boolean != true,
		"Failed to get correct notification value data bool");
	free(list[0].label.value);
	free(list[0].store.d_string.value);
	free(list[1].label.value);
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
	tcase_add_test(tc, set_value_check);
	tcase_add_test(tc, get_value_check);
	tcase_add_test(tc, register_notification_check);
	tcase_add_test(tc, bt_daemon_handle_message_error_check);
	tcase_add_test(tc, bt_daemon_handle_message_set_check);
	tcase_add_test(tc, bt_daemon_handle_message_get_check);
	tcase_add_test(tc, bt_daemon_handle_message_notify_check);
	tcase_add_test(tc, bt_daemon_handle_message_unset_check);
	tcase_add_test(tc, bt_daemon_notify_clients_check);
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
