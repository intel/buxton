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

#include "backend.h"
#include "bt-daemon.h"
#include "check_utils.h"
#include "protocol.h"
#include "serialize.h"
#include "util.h"

#ifdef NDEBUG
	#error "re-run configure with --enable-debug"
#endif

START_TEST(buxton_direct_open_check)
{
	BuxtonControl c;
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
	buxton_direct_close(&c);
}
END_TEST

START_TEST(buxton_direct_set_value_check)
{
	BuxtonControl c;
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
	BuxtonData data;
	BuxtonString layer = buxton_string_pack("test-gdbm");
	BuxtonString key = buxton_string_pack("bxt_test");

	c.client.uid = getuid();
	data.type = STRING;
	data.label = buxton_string_pack("label");
	data.store.d_string = buxton_string_pack("bxt_test_value");
	fail_if(buxton_direct_set_value(&c, &layer, &key, &data) == false,
		"Setting value in buxton directly failed.");
	buxton_direct_close(&c);
}
END_TEST

START_TEST(buxton_direct_get_value_for_layer_check)
{
	BuxtonControl c;
	BuxtonData result;
	BuxtonString layer = buxton_string_pack("test-gdbm");
	BuxtonString key = buxton_string_pack("bxt_test");

	c.client.uid = getuid();
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
	fail_if(buxton_direct_get_value_for_layer(&c, &layer, &key, &result) == false,
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
	buxton_direct_close(&c);
}
END_TEST

START_TEST(buxton_direct_get_value_check)
{
	BuxtonControl c;
	BuxtonData data, result;
	BuxtonString layer = buxton_string_pack("test-gdbm-user");
	BuxtonString key = buxton_string_pack("bxt_test");
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");

	c.client.uid = getuid();
	data.type = STRING;
	data.label = buxton_string_pack("label2");
	data.store.d_string = buxton_string_pack("bxt_test_value2");
	fail_if(data.store.d_string.value == NULL,
		"Failed to allocate test string.");
	fail_if(buxton_direct_set_value(&c, &layer, &key, &data) == false,
		"Failed to set second value.");
	fail_if(buxton_direct_get_value(&c, &key, &result) == false,
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
	buxton_direct_close(&c);
}
END_TEST

START_TEST(buxton_memory_backend_check)
{
	BuxtonControl c;
	BuxtonString layer = buxton_string_pack("temp");
	BuxtonString key = buxton_string_pack("bxt_mem_test");
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
	BuxtonData data, result;

	c.client.uid = getuid();
	data.type = STRING;
	data.label = buxton_string_pack("label");
	data.store.d_string = buxton_string_pack("bxt_test_value");
	fail_if(buxton_direct_set_value(&c, &layer, &key, &data) == false,
		"Setting value in buxton memory backend directly failed.");
	fail_if(buxton_direct_get_value_for_layer(&c, &layer, &key, &result) == false,
		"Retrieving value from buxton memory backend directly failed.");
	fail_if(!streq(result.label.value, "label"),
		"Buxton memory returned a different label to that set.");
	fail_if(!streq(result.store.d_string.value, "bxt_test_value"),
		"Buxton memory returned a different value to that set.");
	buxton_direct_close(&c);
}
END_TEST

START_TEST(buxton_key_check)
{
	char *group = "group";
	char *name = "name";
	BuxtonString *key;
	char *g;
	char *n;
	char bkey[5];

	key = buxton_make_key(group, name);
	fail_if(!key, "Failed to create buxton key");
	g = buxton_get_group(key);
	n = buxton_get_name(key);
	fail_if(!g, "Failed to get group from key");
	fail_if(!n, "Failed to get name from key");
	fail_if(!streq(g, group), "Got different group back from key");
	fail_if(!streq(n, name), "Got different name back from key");

	fail_if(buxton_make_key(NULL, name), "Got key back with invalid group");
	free(key->value);
	free(key);
	key = buxton_make_key(group, NULL);
	fail_if(!key, "Failed to create buxton key with empty name");

	free(key->value);
	key->value = NULL;
	fail_if(buxton_get_group(NULL), "Got group back with invalid key1");
	fail_if(buxton_get_group(key), "Got group back with invalid key2");
	key->value = bkey;
	key->length = 5;
	bkey[0] = 0;
	bkey[1] = 'a';
	bkey[2] = 'a';
	bkey[3] = 'a';
	bkey[4] = 'a';
	fail_if(buxton_get_group(key), "Got group back with invalid key3");

	key->value = NULL;
	fail_if(buxton_get_name(NULL), "Got name back with invalid key1");
	fail_if(buxton_get_name(key), "Got name back with invalid key2");

	key->value = bkey;
	bkey[0] = 'a';
	bkey[1] = 'a';
	bkey[2] = 'a';
	bkey[3] = 'a';
	bkey[4] = 'a';
	fail_if(buxton_get_name(key), "Got name back with invalid key3");
	bkey[0] = 'a';
	bkey[1] = 'a';
	bkey[2] = 'a';
	bkey[3] = 'a';
	bkey[4] = 0;
	fail_if(buxton_get_name(key), "Got name back with invalid key4");

}
END_TEST

START_TEST(buxton_group_label_check)
{
	BuxtonControl c;
	BuxtonData result;
	BuxtonString layer = buxton_string_pack("test-gdbm");
	BuxtonString *key = buxton_make_key("test-group", NULL);
	BuxtonString label = buxton_string_pack("System");

	c.client.uid = getuid();
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
	fail_if(buxton_direct_set_label(&c, &layer, key, &label) == false,
		"Failed to set group label.");
	fail_if(buxton_direct_get_value_for_layer(&c, &layer, key, &result) == false,
		"Retrieving group label failed.");
	fail_if(!streq("System", result.label.value),
		"Retrieved group label is incorrect.");

	free(key->value);
	free(key);
	buxton_direct_close(&c);
}
END_TEST

START_TEST(buxton_name_label_check)
{
	BuxtonControl c;
	BuxtonData data, result;
	BuxtonString layer, label;
	BuxtonString *group, *key;

	/* create the group first, and validate the label */
	layer = buxton_string_pack("test-gdbm");
	group = buxton_make_key("group-foo", NULL);
	label = buxton_string_pack("System");

	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
	fail_if(buxton_direct_set_label(&c, &layer, group, &label) == false,
		"Failed to set group label.");
	fail_if(buxton_direct_get_value_for_layer(&c, &layer, group, &result) == false,
		"Retrieving group label failed.");
	fail_if(!streq("System", result.label.value),
		"Retrieved group label is incorrect.");
	free(group->value);
	free(group);
	if (result.label.value)
		free(result.label.value);
	if (result.store.d_string.value)
		free(result.store.d_string.value);

	/* then create the name (key), and validate the label */
	key = buxton_make_key("group-foo", "name-foo");
	data.type = STRING;
	data.label = buxton_string_pack("System");
	data.store.d_string = buxton_string_pack("value1-foo");
	fail_if(buxton_direct_set_value(&c, &layer, key, &data) == false,
		"Failed to set key name-foo.");
	fail_if(buxton_direct_get_value_for_layer(&c, &layer, key, &result) == false,
		"Failed to get value for name-foo.");
	fail_if(!streq("value1-foo", result.store.d_string.value),
		"Retrieved key value is incorrect.");
	fail_if(!streq("System", result.label.value),
		"Retrieved key label is incorrect.");
	if (result.label.value)
		free(result.label.value);
	if (result.store.d_string.value)
		free(result.store.d_string.value);

	/* modify the same key, with a new value, and validate the label */
	data.store.d_string = buxton_string_pack("value2-foo");
	fail_if(buxton_direct_set_value(&c, &layer, key, &data) == false,
		"Failed to modify key name-foo.");
	fail_if(buxton_direct_get_value_for_layer(&c, &layer, key, &result) == false,
		"Failed to get new value for name-foo.");
	fail_if(!streq("value2-foo", result.store.d_string.value),
		"New key value is incorrect.");
	fail_if(!streq("System", result.label.value),
		"Key label has been modified.");
	free(key->value);
	free(key);
	if (result.label.value)
		free(result.label.value);
	if (result.store.d_string.value)
		free(result.store.d_string.value);

	buxton_direct_close(&c);
}
END_TEST

START_TEST(buxton_wire_get_response_check)
{
	BuxtonClient client;
	BuxtonArray *out_list = NULL;
	BuxtonData *list = NULL;
	BuxtonControlMessage msg;
	pid_t pid;
	int server;

	setup_socket_pair(&(client.fd), &server);
	out_list = buxton_array_new();

	pid = fork();
	if (pid == 0) {
		/* child (server) */
		uint8_t *dest = NULL;
		size_t size;
		BuxtonData data;
		close(client.fd);
		data.type = INT32;
		data.store.d_int32 = 0;
		data.label = buxton_string_pack("dummy");
		fail_if(!buxton_array_add(out_list, &data),
			"Failed to add response to array");
		size = buxton_serialize_message(&dest, BUXTON_CONTROL_STATUS,
						0, out_list);
		buxton_array_free(&out_list, NULL);
		fail_if(size == 0, "Failed to serialize message");
		write(server, dest, size);
		close(server);
		exit(EXIT_SUCCESS);
	} else if (pid == -1) {
		/* error */
		fail("Failed to fork for response check");
	} else {
		/* parent (client) */
		close(server);
		fail_if(buxton_wire_get_response(&client, &msg, &list, NULL) != 1,
			"Failed to properly handle response");
		fail_if(msg != BUXTON_CONTROL_STATUS,
			"Failed to get correct control message type");
		fail_if(list[0].type != INT32,
			"Failed to get correct data type from message");
		fail_if(!(list[0].label.value),
			"Failed to get label from message");
		fail_if(!streq(list[0].label.value, "dummy"),
			"Failed to get correct label from message");
		fail_if(list[0].store.d_int32 != 0,
			"Failed to get correct data value from message");
		free(list);
		close(client.fd);
	}
}
END_TEST

START_TEST(buxton_wire_set_value_check)
{
	BuxtonClient client;
	pid_t pid;
	int server;
	BuxtonArray *list = NULL;

	setup_socket_pair(&(client.fd), &server);
	list = buxton_array_new();

	pid = fork();
	if (pid == 0) {
		/* child (server) */
		uint8_t *dest = NULL;
		size_t size;
		BuxtonData data;
		uint8_t buf[4096];
		ssize_t r;

		close(client.fd);
		data.type = INT32;
		data.store.d_int32 = 0;
		data.label = buxton_string_pack("dummy");
		fail_if(!buxton_array_add(list, &data),
			"Failed to add response to array");
		size = buxton_serialize_message(&dest, BUXTON_CONTROL_STATUS, 0,
						list);
		buxton_array_free(&list, NULL);
		fail_if(size == 0, "Failed to serialize message");
		r = read(server, buf, 4096);
		fail_if(r < 0, "Read failed from buxton_wire_set_value");
		write(server, dest, size);
		close(server);
		free(dest);
		exit(EXIT_SUCCESS);
	} else if (pid == -1) {
		/* error */
		fail("Failed to fork for set check");
	} else {
		/* parent (client) */
		BuxtonString layer_name, key;
		BuxtonData value;

		close(server);
		layer_name = buxton_string_pack("layer");
		key = buxton_string_pack("key");
		value.type = STRING;
		value.label = buxton_string_pack("label");
		value.store.d_string = buxton_string_pack("value");
		fail_if(buxton_wire_set_value(&client, &layer_name, &key, &value, NULL) != true,
			"Failed to properly set value");
		close(client.fd);
	}
}
END_TEST

START_TEST(buxton_wire_get_value_check)
{
	BuxtonClient client;
	pid_t pid;
	int server;
	BuxtonArray *list = NULL;

	setup_socket_pair(&(client.fd), &server);
	list = buxton_array_new();

	pid = fork();
	if (pid == 0) {
		/* child (server) */
		uint8_t *dest = NULL;
		size_t size;
		BuxtonData data1, data2, data3;
		uint8_t buf[4096];
		ssize_t r;

		close(client.fd);
		data1.type = INT32;
		data1.store.d_int32 = 0;
		data1.label = buxton_string_pack("dummy");
		data2.type = STRING;
		data2.store.d_string = buxton_string_pack("key");
		data2.label = buxton_string_pack("dummy");
		data3.type = INT32;
		data3.store.d_int32 = 1;
		data3.label = buxton_string_pack("label");
		fail_if(!buxton_array_add(list, &data1),
			"Failed to add first element to response");
		fail_if(!buxton_array_add(list, &data2),
			"Failed to add first element to response");
		fail_if(!buxton_array_add(list, &data3),
			"Failed to add first element to response");
		size = buxton_serialize_message(&dest, BUXTON_CONTROL_STATUS, 0,
						list);
		buxton_array_free(&list, NULL);
		fail_if(size == 0, "Failed to serialize message");
		r = read(server, buf, 4096);
		fail_if(r < 0, "Read failed from buxton_wire_get_value");
		write(server, dest, size);
		close(server);
		free(dest);
		exit(EXIT_SUCCESS);
	} else if (pid == -1) {
		/* error */
		fail("Failed to fork for get check");
	} else {
		/* parent (client) */
		BuxtonString layer_name, key;
		BuxtonData value;

		close(server);
		layer_name = buxton_string_pack("layer");
		key = buxton_string_pack("key");
		fail_if(buxton_wire_get_value(&client, &layer_name, &key, &value, NULL) != true,
			"Failed to properly get value");
		fail_if(value.type != INT32, "Failed to get value's correct type");
		fail_if(value.store.d_int32 != 1, "Failed to get value's correct value");
		close(client.fd);
		free(value.label.value);
	}
}
END_TEST

static Suite *
buxton_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("buxton");

	tc = tcase_create("buxton_client_lib_functions");
	tcase_add_test(tc, buxton_direct_open_check);
	tcase_add_test(tc, buxton_direct_set_value_check);
	tcase_add_test(tc, buxton_direct_get_value_for_layer_check);
	tcase_add_test(tc, buxton_direct_get_value_check);
	tcase_add_test(tc, buxton_memory_backend_check);
	tcase_add_test(tc, buxton_key_check);
	tcase_add_test(tc, buxton_group_label_check);
	tcase_add_test(tc, buxton_name_label_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("buxton_protocol_functions");
	tcase_add_test(tc, buxton_wire_get_response_check);
	tcase_add_test(tc, buxton_wire_set_value_check);
	tcase_add_test(tc, buxton_wire_get_value_check);
	suite_add_tcase(s, tc);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	putenv("BUXTON_CONF_FILE=" ABS_TOP_BUILDDIR "/test/test.conf");
	s = buxton_suite();
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
