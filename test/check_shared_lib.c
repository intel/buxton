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
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iniparser.h>
#include <limits.h>

#include "backend.h"
#include "constants.h"
#include "hashmap.h"
#include "log.h"
#include "serialize.h"
#include "smack.h"
#include "util.h"
#include "array.h"

START_TEST(log_write_check)
{
	char log_file[] = "log-check-stderr-file";
	char log_msg[] = "Log test";
	_cleanup_free_ char *log_read = malloc(strlen(log_msg));
	fail_if(log_read == NULL,
		"Failed to allocate space for reading the log");

	int old_stderr = fileno(stderr);
	fail_if(old_stderr == -1, "Failed to get fileno for stderr");

	int dup_stderr = dup(old_stderr);
	fail_if(dup_stderr == -1, "Failed to dup stderr");

	FILE *new_stderr = freopen(log_file, "w", stderr);
	fail_if(new_stderr == NULL, "Failed to reopen stderr");

	buxton_log(log_msg);
	fail_if(fflush(stderr) != 0, "Failed to flush stderr");

	FILE *read_test = fopen(log_file, "r");
	fail_if(read_test == NULL, "Failed to open stderr file for reading");

	size_t len = fread(log_read, 1, strlen(log_msg), read_test);
	fail_if(len != strlen(log_msg), "Failed to read entire log message");
	fail_if(strncmp(log_msg, log_read, strlen(log_msg)) != 0,
		"Failed to write log message correctly");

	fclose(stderr);
	fclose(read_test);
	stderr = fdopen(dup_stderr, "w");
}
END_TEST

START_TEST(ini_parse_check)
{
	char ini_good[] = "test/test-pass.ini";
	char ini_bad[] = "test/test-fail.ini";
	dictionary *ini = NULL;

        ini = iniparser_load(ini_good);
	fail_if(ini == NULL,
		"Failed to parse ini file");

	iniparser_dump(ini, stdout);
	iniparser_freedict(ini);

        ini = iniparser_load(ini_bad);
	fail_if(ini != NULL,
		"Failed to catch bad ini file");

	iniparser_dump(ini, stdout);
	iniparser_freedict(ini);
}
END_TEST

START_TEST(hashmap_check)
{
	Hashmap *map;
	char *value;
	int r;

	map = hashmap_new(string_hash_func, string_compare_func);
	fail_if(map == NULL, "Failed to allocated hashmap");
	r = hashmap_put(map, "test", "passed");
	fail_if(r < 0, "Failed to add element to hashmap");

	value = hashmap_get(map, "test");

	fail_if(value == NULL,
		"Failed to get value from hashmap");

	fail_if(strcmp(value, "passed") != 0,
		"Failed to retrieve the put value");

	hashmap_remove(map, "test");
	fail_if(hashmap_isempty(map) != true,
		"Failed to remove item from hashmap");

	hashmap_free(map);
}
END_TEST

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
}
END_TEST

START_TEST(get_layer_path_check)
{
	BuxtonLayer layer;
	char *path = NULL;
	char *real_path = NULL;
	int r;

	memset(&layer, 0, sizeof(BuxtonLayer));
	layer.name = buxton_string_pack("path-test");
	layer.type = LAYER_SYSTEM;
	r = asprintf(&real_path, "%s/%s", DB_PATH, "path-test.db");
	fail_if(r == -1, "Failed to set real path for system layer");

	path = get_layer_path(&layer);
	fail_if(path == NULL, "Failed to get path for system layer");
	fail_if(strcmp(path, real_path) != 0,
		"Failed to set correct system path");

	free(path);
	free(real_path);

	layer.name = buxton_string_pack("user-path-test");
	layer.type = LAYER_USER;
	layer.uid = 1000;
	r = asprintf(&real_path, "%s/%s", DB_PATH, "user-path-test-1000.db");
	fail_if(r == -1, "Failed to set real path for user layer");

	path = get_layer_path(&layer);
	fail_if(path == NULL, "Failed to get path for user layer");
	fail_if(strcmp(path, real_path) != 0,
		"Failed to set correct user path");

	free(path);
	free(real_path);

	layer.name = buxton_string_pack("bad-type-test");
	layer.type = -1;
	fail_if(get_layer_path(&layer) != NULL,
		"Invalid layer type didn't return failure");
}
END_TEST

START_TEST(buxton_data_copy_check)
{
	BuxtonData original, copy;

	original.type = STRING;
	original.label = buxton_string_pack("label");
	original.store.d_string = buxton_string_pack("test-data-copy");
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy string type");
	fail_if(!copy.store.d_string.value, "Failed to copy string data");
	fail_if((strcmp(original.store.d_string.value, copy.store.d_string.value) != 0),
		"Incorrectly copied string data");
	if (copy.store.d_string.value)
		free(copy.store.d_string.value);
	if (copy.label.value)
		free(copy.label.value);

	original.type = INT32;
	original.store.d_int32 = INT_MAX;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy int32 type");
	fail_if(!streq(original.label.value, copy.label.value),
		"Incorrectly copied int32 label");
	fail_if(original.store.d_int32 != copy.store.d_int32,
		"Failed to copy int32 data");
	if (copy.label.value)
		free(copy.label.value);

	original.type = INT64;
	original.store.d_int64 = LONG_MAX;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy long type");
	fail_if(!streq(original.label.value, copy.label.value),
		"Incorrectly copied long label");
	fail_if(original.store.d_int64 != copy.store.d_int64,
		"Failed to copy long data");
	if (copy.label.value)
		free(copy.label.value);

	original.type = FLOAT;
	original.store.d_float = 3.14F;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy float type");
	fail_if(!streq(original.label.value, copy.label.value),
		"Incorrectly copied float label");
	fail_if(original.store.d_float != copy.store.d_float,
		"Failed to copy float data");
	if (copy.label.value)
		free(copy.label.value);

	original.type = DOUBLE;
	original.store.d_double = 3.1415;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy double type");
	fail_if(!streq(original.label.value, copy.label.value),
		"Incorrectly copied double label");
	fail_if(original.store.d_double != copy.store.d_double,
		"Failed to copy double data");
	if (copy.label.value)
		free(copy.label.value);

	original.type = BOOLEAN;
	original.store.d_boolean = true;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy boolean type");
	fail_if(!streq(original.label.value, copy.label.value),
		"Incorrectly copied boolean label");
	fail_if(original.store.d_boolean != copy.store.d_boolean,
		"Failed to copy boolean data");
	if (copy.label.value)
		free(copy.label.value);

	original.type = -1;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type || copy.store.d_string.value, "Copied invalid data");
}
END_TEST

START_TEST(buxton_type_as_string_check)
{
	BuxtonDataType type;

	type = STRING;
	fail_if(strcmp(buxton_type_as_string(type), "string") != 0,
		"Failed to get string of STRING type");

	type = INT32;
	fail_if(strcmp(buxton_type_as_string(type), "int32_t") != 0,
		"Failed to get string of INT32 type");

	type = INT64;
	fail_if(strcmp(buxton_type_as_string(type), "int64_t") != 0,
		"Failed to get string of INT64 type");

	type = FLOAT;
	fail_if(strcmp(buxton_type_as_string(type), "float") != 0,
		"Failed to get string of FLOAT type");

	type = DOUBLE;
	fail_if(strcmp(buxton_type_as_string(type), "double") != 0,
		"Failed to get string of DOUBLE type");

	type = BOOLEAN;
	fail_if(strcmp(buxton_type_as_string(type), "boolean") != 0,
		"Failed to get string of BOOLEAN type");
}
END_TEST

START_TEST(buxton_db_serialize_check)
{
	BuxtonData dsource, dtarget;
	uint8_t *packed = NULL;

	dsource.type = STRING;
	dsource.label = buxton_string_pack("label");
	dsource.store.d_string = buxton_string_pack("test-string");
	fail_if(buxton_serialize(&dsource, &packed) == false,
		"Failed to serialize string data");
	fail_if(buxton_deserialize(packed, &dtarget) == false,
		"Failed to deserialize string data");
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for string");
	fail_if(strcmp(dsource.label.value, dtarget.label.value) != 0,
		"Source and destination string labels differ");
	fail_if(strcmp(dsource.store.d_string.value, dtarget.store.d_string.value) != 0,
		"Source and destination string data differ");
	free(packed);
	if (dtarget.store.d_string.value)
		free(dtarget.store.d_string.value);

	dsource.type = INT32;
	dsource.store.d_int32 = INT_MAX;
	fail_if(buxton_serialize(&dsource, &packed) == false,
		"Failed to serialize int data");
	fail_if(buxton_deserialize(packed, &dtarget) == false,
		"Failed to deserialize int data");
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for int");
	fail_if(strcmp(dsource.label.value, dtarget.label.value) != 0,
		"Source and destination int labels differ");
	fail_if(dsource.store.d_int32 != dtarget.store.d_int32,
		"Source and destination int data differ");
	free(packed);

	dsource.type = INT64;
	dsource.store.d_int64 = LONG_MAX;
	fail_if(buxton_serialize(&dsource, &packed) == false,
		"Failed to serialize long data");
	fail_if(buxton_deserialize(packed, &dtarget) == false,
		"Failed to deserialize long data");
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for long");
	fail_if(strcmp(dsource.label.value, dtarget.label.value) != 0,
		"Source and destination long labels differ");
	fail_if(dsource.store.d_int64 != dtarget.store.d_int64,
		"Source and destination long data differ");
	free(packed);

	dsource.type = FLOAT;
	dsource.store.d_float = 3.14F;
	fail_if(buxton_serialize(&dsource, &packed) == false,
		"Failed to serialize float data");
	fail_if(buxton_deserialize(packed, &dtarget) == false,
		"Failed to deserialize float data");
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for float");
	fail_if(strcmp(dsource.label.value, dtarget.label.value) != 0,
		"Source and destination float labels differ");
	fail_if(dsource.store.d_float != dtarget.store.d_float,
		"Source and destination float data differ");
	free(packed);

	dsource.type = DOUBLE;
	dsource.store.d_double = 3.1415;
	fail_if(buxton_serialize(&dsource, &packed) == false,
		"Failed to serialize double data");
	fail_if(buxton_deserialize(packed, &dtarget) == false,
		"Failed to deserialize double data");
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for double");
	fail_if(strcmp(dsource.label.value, dtarget.label.value) != 0,
		"Source and destination double labels differ");
	fail_if(dsource.store.d_double != dtarget.store.d_double,
		"Source and destination double data differ");
	free(packed);

	//FIXME add boolean serialize check

	dsource.type = -1;
	dsource.store.d_boolean = true;
	fail_if(buxton_serialize(&dsource, &packed) == true,
		"Able to serialize invalid type data");
}
END_TEST

START_TEST(buxton_message_serialize_check)
{
	BuxtonControlMessage csource;
	BuxtonControlMessage ctarget;
	BuxtonData dsource;
	BuxtonData *dtarget = NULL;
	uint8_t *packed = NULL;
	size_t ret;

	dsource.type = STRING;
	dsource.label = buxton_string_pack("label");
	dsource.store.d_string = buxton_string_pack("test-key");
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, 1, &dsource);
	fail_if(ret == 0, "Failed to serialize string data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &dtarget) != 1,
		"Failed to deserialize string data");
	fail_if(ctarget != csource, "Failed to get correct control message for string");
	fail_if(dsource.type != dtarget[0].type,
		"Source and destination type differ for string");
	fail_if(strcmp(dsource.store.d_string.value, dtarget[0].store.d_string.value) != 0,
		"Source and destination string data differ");
	free(packed);
	if (dtarget) {
		if (dtarget[0].store.d_string.value) {
			free(dtarget[0].store.d_string.value);
		}
		free(dtarget);
	}

	dsource.type = INT32;
	dsource.store.d_int32 = INT_MAX;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, 1, &dsource);
	fail_if(ret == 0, "Failed to serialize int data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &dtarget) != 1,
		"Failed to deserialize int data");
	fail_if(ctarget != csource, "Failed to get correct control message for int");
	fail_if(dsource.type != dtarget[0].type,
		"Source and destination type differ for int");
	fail_if(dsource.store.d_int32 != dtarget[0].store.d_int32,
		"Source and destination int data differ");
	free(packed);
	free(dtarget);

	dsource.type = INT64;
	dsource.store.d_int64 = LONG_MAX;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, 1, &dsource);
	fail_if(ret == 0, "Failed to serialize long data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &dtarget) != 1,
		"Failed to deserialize long data");
	fail_if(ctarget != csource, "Failed to get correct control message for long");
	fail_if(dsource.type != dtarget[0].type,
		"Source and destination type differ for long");
	fail_if(dsource.store.d_int64 != dtarget[0].store.d_int64,
		"Source and destination long data differ");
	free(packed);
	free(dtarget);

	dsource.type = FLOAT;
	dsource.store.d_float = 3.14F;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, 1, &dsource);
	fail_if(ret == 0, "Failed to serialize float data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &dtarget) != 1,
		"Failed to deserialize float data");
	fail_if(ctarget != csource, "Failed to get correct control message for float");
	fail_if(dsource.type != dtarget[0].type,
		"Source and destination type differ for float");
	fail_if(dsource.store.d_float != dtarget[0].store.d_float,
		"Source and destination float data differ");
	free(packed);
	free(dtarget);

	dsource.type = DOUBLE;
	dsource.store.d_double = 3.1415;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, 1, &dsource);
	fail_if(ret == 0, "Failed to serialize double data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &dtarget) != 1,
		"Failed to deserialize double data");
	fail_if(ctarget != csource, "Failed to get correct control message for double");
	fail_if(dsource.type != dtarget[0].type,
		"Source and destination type differ for double");
	fail_if(dsource.store.d_double != dtarget[0].store.d_double,
		"Source and destination double data differ");
	free(packed);
	free(dtarget);

	dsource.type = BOOLEAN;
	dsource.store.d_boolean = true;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, 1, &dsource);
	fail_if(ret == 0, "Failed to serialize boolean data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &dtarget) != 1,
		"Failed to deserialize boolean data");
	fail_if(ctarget != csource, "Failed to get correct control message for boolean");
	fail_if(dsource.type != dtarget[0].type,
		"Source and destination type differ for boolean");
	fail_if(dsource.store.d_boolean != dtarget[0].store.d_boolean,
		"Source and destination boolean data differ");
	free(packed);
	free(dtarget);

	dsource.type = STRING;
	dsource.store.d_string = buxton_string_pack("test-key");
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, 2, &dsource);
	fail_if(ret != 0, "Serialized with incorrect parameter count");

	dsource.type = -1;
	dsource.store.d_string = buxton_string_pack("test-key");
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, 1, &dsource);
	fail_if(ret != 0, "Serialized with bad data type");

	dsource.type = STRING;
	dsource.store.d_string = buxton_string_pack("test-key");
	csource = -1;
	ret = buxton_serialize_message(&packed, csource, 1, &dsource);
	fail_if(ret != 0, "Serialized with bad message type");
}
END_TEST

START_TEST(buxton_get_message_size_check)
{
	BuxtonControlMessage csource;
	BuxtonData dsource;
	uint8_t *packed = NULL;
	size_t ret;

	dsource.type = STRING;
	dsource.label = buxton_string_pack("label");
	dsource.store.d_string = buxton_string_pack("test-key");
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, 1, &dsource);
	fail_if(ret == 0, "Failed to serialize string data for size");
	fail_if(ret != buxton_get_message_size(packed, ret),
		"Failed to get correct message size");
	fail_if(buxton_get_message_size(packed, BUXTON_MESSAGE_HEADER_LENGTH - 1) != 0,
		"Got size even though message smaller than the minimum");

	free(packed);
}
END_TEST


START_TEST(buxton_array_check)
{
	BuxtonArray *array = NULL;
	BuxtonString one, two;
	char *three = NULL;
	BuxtonString *test = NULL;
	bool ret = false;

	array = buxton_array_new();
	fail_if(array == NULL, "Failed to allocate new BuxtonArray");

	one = buxton_string_pack("one");
	two = buxton_string_pack("two");
	asprintf(&three, "three");

	ret = buxton_array_add(array, &one);
	fail_if(ret == false, "Failed to add string 1 to BuxtonArray");
	ret = buxton_array_add(array, &two);
	fail_if(ret == false, "Failed to add string 2 to BuxtonArray");
	ret = buxton_array_add(array, three);
	fail_if(ret == false, "Failed to add string 3 (pointer) to BuxtonArray");

	fail_if(array->len != 3, "BuxtonArray doesn't contain 3 elements");
	ret = buxton_array_remove(array, three, &free);
	fail_if(ret  == false, "Failed to remove and free pointer");

	ret = buxton_array_remove(array, &one, NULL);
	fail_if(ret == false, "Failed to remove string 1 from BuxtonArray");
	fail_if(array->len != 1, "BuxtonArray doesn't contain only 1 element");

	test = (BuxtonString*)array->data[0];
	fail_if(!test, "Null pointer in array");
	fail_if(!streq(test->value, two.value), "Corrupted data in BuxtonArray");

	buxton_array_free(&array, NULL);
	fail_if(array != NULL, "Failed to correctly free BuxtonArray");
}
END_TEST

static Suite *
shared_lib_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("shared_lib");
	tc = tcase_create("log_functions");
	tcase_add_test(tc, log_write_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("ini_functions");
	tcase_add_test(tc, ini_parse_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("hashmap_functions");
	tcase_add_test(tc, hashmap_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("array_functions");
	tcase_add_test(tc, buxton_array_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("smack_access_functions");
	tcase_add_test(tc, smack_access_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("util_functions");
	tcase_add_test(tc, get_layer_path_check);
	tcase_add_test(tc, buxton_data_copy_check);
	tcase_add_test(tc, buxton_type_as_string_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("buxton_serialize_functions");
	tcase_add_test(tc, buxton_db_serialize_check);
	tcase_add_test(tc, buxton_message_serialize_check);
	tcase_add_test(tc, buxton_get_message_size_check);
	suite_add_tcase(s, tc);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = shared_lib_suite();
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
