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
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "backend.h"
#include "buxtonlist.h"
#include "check_utils.h"
#include "hashmap.h"
#include "log.h"
#include "serialize.h"
#include "smack.h"
#include "util.h"
#include "configurator.h"

#ifdef NDEBUG
#error "re-run configure with --enable-debug"
#endif

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

static inline void array_free_fun(void *p)
{
	free(p);
}

START_TEST(array_check)
{
	BuxtonArray *array = NULL;
	char *value;
	char *element;
	void *f;
	bool r;

	array = buxton_array_new();
	fail_if(array == NULL, "Failed to allocate memory for BuxtonArray");
	element = strdup("test");
	fail_if(!element, "Failed to allocate memory for array item");
	r = buxton_array_add(NULL, element);
	fail_if(r, "Added element to NULL array");
	r = buxton_array_add(array, NULL);
	fail_if(r, "Added NULL element to array");
	r = buxton_array_add(array, element);
	fail_if(r  == false, "Failed to add element to BuxtonArray");
	fail_if(array->len != 1,
		"Failed to get correct value for number of elements in array");

	f = buxton_array_get(NULL, 0);
	fail_if(f, "Got value from NULL array");
	f = buxton_array_get(array, (uint16_t)(array->len + 1));
	fail_if(f, "Got value from index bigger than maximum index");
	value = (char *)buxton_array_get(array, 0);

	fail_if(value == NULL,
		"Failed to get value from BuxtonArray");

	fail_if(strcmp(value, "test") != 0,
		"Failed to retrieve the stored value");

	buxton_array_free(&array, array_free_fun);
	fail_if(array != NULL,
		"Failed to free BuxtonArray");
}
END_TEST

START_TEST(list_check)
{
	BuxtonList *list = NULL;
	int i;
	char *tmp = NULL;
	char *head = "<head of the list>";
	char *head2 = "<prepend should appear before head now>";
	char *data = "<middle element to be removed>";

	/* Append a million strings. Results in about 3 million allocs
	 * due to asprintf, calloc of node, etc */
	int DEFAULT_SIZE = (10*1000)*100;
	for (i = 0; i <= DEFAULT_SIZE; i++) {
		if (i == 5) {
			fail_if(buxton_list_append(&list, data) == false,
				"Failed to append to BuxtonList");
		} else {
			asprintf(&tmp, "i #%d", i);
			fail_if(buxton_list_prepend(&list, tmp) == false,
				"Failed to prepend to BuxtonList");
		}
	}

	fail_if(list->size != DEFAULT_SIZE, "List size invalid");

	/* Prepend head */
	fail_if(buxton_list_prepend(&list, head) != true, "Prepend head failed");
	fail_if(list->size != DEFAULT_SIZE+1, "Prepended head size invalid");

	/* Prepend head2 */
	fail_if(buxton_list_prepend(&list, head2) != true, "Prepend head2 failed");
	fail_if(list->size != DEFAULT_SIZE+2, "Prepended head2 size invalid");

	/* Remove from middle */
	fail_if(buxton_list_remove(&list, data, false) != true,
		"List removal from middle failed");
	fail_if(list->size != DEFAULT_SIZE+1, "List middle removal size invalid");

	/* Remove from end */
	fail_if(buxton_list_remove(&list, tmp, true) != true,
		"List tail removal failed");
	fail_if(list->size != DEFAULT_SIZE, "List tail removal size invalid");

	fail_if(buxton_list_append(&list, "newend") != true,
		"List new tail append failed");
	fail_if(list->size != DEFAULT_SIZE+1, "List new tail size invalid");
	fail_if(buxton_list_remove(&list, "newend", false) != true,
		"List new tail removal failed");
	fail_if(list->size != DEFAULT_SIZE,
		"List new tail size invalid (post removal)");

	/* Fake remove */
	fail_if(buxton_list_remove(&list, "nonexistent", false) == true,
		"List non existent removal should fail");
	fail_if(list->size != DEFAULT_SIZE,
		"List size invalid after no change");

	/* Remove head */
	fail_if(buxton_list_remove(&list, head, false) == false,
		"List remove head failed");
	fail_if(buxton_list_remove(&list, head2, false) == false,
		"List remove head2 failed");
	fail_if(list->size != DEFAULT_SIZE-2,
		"List post heads removal size invalid");

	buxton_list_free_all(&list);
}
END_TEST

START_TEST(get_layer_path_check)
{
	BuxtonLayer layer;
	char *path = NULL;
	char *real_path = NULL;
	int r;

	memzero(&layer, sizeof(BuxtonLayer));
	layer.name = buxton_string_pack("path-test");
	layer.type = LAYER_SYSTEM;
	r = asprintf(&real_path, "%s/%s", buxton_db_path(), "path-test.db");
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
	r = asprintf(&real_path, "%s/%s", buxton_db_path(), "user-path-test-1000.db");
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
	original.store.d_string = buxton_string_pack("test-data-copy");
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy string type");
	fail_if(!copy.store.d_string.value, "Failed to copy string data");
	fail_if((strcmp(original.store.d_string.value, copy.store.d_string.value) != 0),
		"Incorrectly copied string data");
	if (copy.store.d_string.value)
		free(copy.store.d_string.value);

	original.type = INT32;
	original.store.d_int32 = INT_MAX;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy int32 type");
	fail_if(original.store.d_int32 != copy.store.d_int32,
		"Failed to copy int32 data");

	original.type = UINT32;
	original.store.d_uint32 = UINT_MAX;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy uint32 type");
	fail_if(original.store.d_uint32 != copy.store.d_uint32,
		"Failed to copy int32 data");

	original.type = INT64;
	original.store.d_int64 = LLONG_MAX;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy int64 type");
	fail_if(original.store.d_int64 != copy.store.d_int64,
		"Failed to copy int64 data");

	original.type = UINT64;
	original.store.d_uint64 = ULLONG_MAX;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy uint64 type");
	fail_if(original.store.d_uint64 != copy.store.d_uint64,
		"Failed to copy uint64 data");

	original.type = FLOAT;
	original.store.d_float = 3.14F;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy float type");
	fail_if(original.store.d_float != copy.store.d_float,
		"Failed to copy float data");

	original.type = DOUBLE;
	original.store.d_double = 3.1415;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy double type");
	fail_if(original.store.d_double != copy.store.d_double,
		"Failed to copy double data");

	original.type = BOOLEAN;
	original.store.d_boolean = true;
	buxton_data_copy(&original, &copy);
	fail_if(copy.type != original.type,
		"Failed to copy boolean type");
	fail_if(original.store.d_boolean != copy.store.d_boolean,
		"Failed to copy boolean data");

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

	type = UINT32;
	fail_if(strcmp(buxton_type_as_string(type), "uint32_t") != 0,
		"Failed to get string of UINT32 type");

	type = INT64;
	fail_if(strcmp(buxton_type_as_string(type), "int64_t") != 0,
		"Failed to get string of INT64 type");

	type = UINT64;
	fail_if(strcmp(buxton_type_as_string(type), "uint64_t") != 0,
		"Failed to get string of UINT64 type");

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

START_TEST(_write_check)
{
	int in, out;
	uint8_t buf[10];

	setup_socket_pair(&in, &out);
	fail_if(fcntl(in, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");
	fail_if(fcntl(out, F_SETFL, O_NONBLOCK),
		"Failed to set socket to non blocking");

	buf[0] = 1;
	fail_if(!_write(out, buf, 1), "Failed to write 1 byte");
}
END_TEST

START_TEST(buxton_db_serialize_check)
{
	BuxtonData dsource, dtarget;
	uint8_t *packed = NULL;
	BuxtonString lsource, ltarget;

	dsource.type = STRING;
	lsource = buxton_string_pack("label");
	dsource.store.d_string = buxton_string_pack("test-string");
	fail_if(buxton_serialize(&dsource, &lsource, &packed) == false,
		"Failed to serialize string data");
	buxton_deserialize(packed, &dtarget, &ltarget);
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for string");
	fail_if(strcmp(lsource.value, ltarget.value) != 0,
		"Source and destination string labels differ");
	fail_if(strcmp(dsource.store.d_string.value, dtarget.store.d_string.value) != 0,
		"Source and destination string data differ");
	free(packed);
	free(ltarget.value);
	if (dtarget.store.d_string.value)
		free(dtarget.store.d_string.value);

	dsource.type = INT32;
	dsource.store.d_int32 = INT_MAX;
	fail_if(buxton_serialize(&dsource, &lsource, &packed) == false,
		"Failed to serialize int32 data");
	buxton_deserialize(packed, &dtarget, &ltarget);
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for int32");
	fail_if(strcmp(lsource.value, ltarget.value) != 0,
		"Source and destination int32 labels differ");
	fail_if(dsource.store.d_int32 != dtarget.store.d_int32,
		"Source and destination int32 data differ");
	free(ltarget.value);
	free(packed);

	dsource.type = UINT32;
	dsource.store.d_uint32 = UINT_MAX;
	fail_if(buxton_serialize(&dsource, &lsource, &packed) == false,
		"Failed to serialize uint32 data");
	buxton_deserialize(packed, &dtarget, &ltarget);
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for uint32");
	fail_if(strcmp(lsource.value, ltarget.value) != 0,
		"Source and destination uint32 labels differ");
	fail_if(dsource.store.d_uint32 != dtarget.store.d_uint32,
		"Source and destination uint32 data differ");
	free(ltarget.value);
	free(packed);

	dsource.type = INT64;
	dsource.store.d_int64 = LONG_MAX;
	fail_if(buxton_serialize(&dsource, &lsource, &packed) == false,
		"Failed to serialize int64 data");
	buxton_deserialize(packed, &dtarget, &ltarget);
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for int64");
	fail_if(strcmp(lsource.value, ltarget.value) != 0,
		"Source and destination int64 labels differ");
	fail_if(dsource.store.d_int64 != dtarget.store.d_int64,
		"Source and destination int64 data differ");
	free(ltarget.value);
	free(packed);

	dsource.type = UINT64;
	dsource.store.d_uint64 = ULLONG_MAX;
	fail_if(buxton_serialize(&dsource, &lsource, &packed) == false,
		"Failed to serialize uint64 data");
	buxton_deserialize(packed, &dtarget, &ltarget);
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for uint64");
	fail_if(strcmp(lsource.value, ltarget.value) != 0,
		"Source and destination uint64 labels differ");
	fail_if(dsource.store.d_uint64 != dtarget.store.d_uint64,
		"Source and destination uint64 data differ");
	free(ltarget.value);
	free(packed);

	dsource.type = FLOAT;
	dsource.store.d_float = 3.14F;
	fail_if(buxton_serialize(&dsource, &lsource, &packed) == false,
		"Failed to serialize float data");
	buxton_deserialize(packed, &dtarget, &ltarget);
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for float");
	fail_if(strcmp(lsource.value, ltarget.value) != 0,
		"Source and destination float labels differ");
	fail_if(dsource.store.d_float != dtarget.store.d_float,
		"Source and destination float data differ");
	free(ltarget.value);
	free(packed);

	dsource.type = DOUBLE;
	dsource.store.d_double = 3.1415;
	fail_if(buxton_serialize(&dsource, &lsource, &packed) == false,
		"Failed to serialize double data");
	buxton_deserialize(packed, &dtarget, &ltarget);
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for double");
	fail_if(strcmp(lsource.value, ltarget.value) != 0,
		"Source and destination double labels differ");
	fail_if(dsource.store.d_double != dtarget.store.d_double,
		"Source and destination double data differ");
	free(ltarget.value);
	free(packed);

	dsource.type = BOOLEAN;
	dsource.store.d_boolean = true;
	fail_if(buxton_serialize(&dsource, &lsource, &packed) == false,
		"Failed to serialize boolean data");
	buxton_deserialize(packed, &dtarget, &ltarget);
	fail_if(dsource.type != dtarget.type,
		"Source and destination type differ for boolean");
	fail_if(strcmp(lsource.value, ltarget.value) != 0,
		"Source and destination boolean labels differ");
	free(ltarget.value);
	free(packed);
}
END_TEST

START_TEST(buxton_message_serialize_check)
{
	BuxtonControlMessage csource;
	BuxtonControlMessage ctarget;
	BuxtonData dsource1, dsource2;
	uint16_t control, message;
	BuxtonData *dtarget = NULL;
	uint8_t *packed = NULL;
	BuxtonArray *list = NULL;
	BuxtonArray *list2 = NULL;
	size_t ret;
	size_t pcount;
	bool r;
	uint32_t msource;
	uint32_t mtarget;

	list = buxton_array_new();
	fail_if(!list, "Failed to allocate list");
	dsource1.type = STRING;
	dsource1.store.d_string = buxton_string_pack("test-key");
	csource = BUXTON_CONTROL_GET;
	msource = 0;
	r = buxton_array_add(list, &dsource1);
	fail_if(!r, "Failed to add element to array");
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(ret == 0, "Failed to serialize string data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) != 1,
		"Failed to deserialize string data");
	fail_if(ctarget != csource, "Failed to get correct control message for string");
	fail_if(mtarget != msource,
		"Failed to get correct message id for string");
	fail_if(dsource1.type != dtarget[0].type,
		"Source and destination type differ for string");
	fail_if(strcmp(dsource1.store.d_string.value, dtarget[0].store.d_string.value) != 0,
		"Source and destination string data differ");
	free(packed);
	if (dtarget) {
		if (dtarget[0].store.d_string.value) {
			free(dtarget[0].store.d_string.value);
		}
		free(dtarget);
	}

	dsource1.type = INT32;
	dsource1.store.d_int32 = INT_MAX;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(ret == 0, "Failed to serialize int32 data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) != 1,
		"Failed to deserialize int data");
	fail_if(ctarget != csource, "Failed to get correct control message for int32");
	fail_if(mtarget != msource,
		"Failed to get correct message id for int32");
	fail_if(dsource1.type != dtarget[0].type,
		"Source and destination type differ for int32");
	fail_if(dsource1.store.d_int32 != dtarget[0].store.d_int32,
		"Source and destination int32 data differ");
	free(packed);
	free(dtarget);

	dsource1.type = UINT32;
	dsource1.store.d_uint32 = UINT_MAX;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(ret == 0, "Failed to serialize uint32 data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) != 1,
		"Failed to deserialize uint32 data");
	fail_if(ctarget != csource, "Failed to get correct control message for uint32");
	fail_if(mtarget != msource,
		"Failed to get correct message id for uint32");
	fail_if(dsource1.type != dtarget[0].type,
		"Source and destination type differ for uint32");
	fail_if(dsource1.store.d_uint32 != dtarget[0].store.d_uint32,
		"Source and destination uint32 data differ");
	free(packed);
	free(dtarget);

	dsource1.type = INT64;
	dsource1.store.d_int64 = LONG_MAX;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(ret == 0, "Failed to serialize long data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) != 1,
		"Failed to deserialize long data");
	fail_if(ctarget != csource, "Failed to get correct control message for long");
	fail_if(mtarget != msource,
		"Failed to get correct message id for long");
	fail_if(dsource1.type != dtarget[0].type,
		"Source and destination type differ for long");
	fail_if(dsource1.store.d_int64 != dtarget[0].store.d_int64,
		"Source and destination long data differ");
	free(packed);
	free(dtarget);

	dsource1.type = UINT64;
	dsource1.store.d_uint64 = ULLONG_MAX;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(ret == 0, "Failed to serialize uint64 data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) != 1,
		"Failed to deserialize uint64 data");
	fail_if(ctarget != csource, "Failed to get correct control message for uint64");
	fail_if(mtarget != msource,
		"Failed to get correct message id for uint64");
	fail_if(dsource1.type != dtarget[0].type,
		"Source and destination type differ for uint64");
	fail_if(dsource1.store.d_uint64 != dtarget[0].store.d_uint64,
		"Source and destination uint64 data differ");
	free(packed);
	free(dtarget);

	dsource1.type = FLOAT;
	dsource1.store.d_float = 3.14F;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(ret == 0, "Failed to serialize float data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) != 1,
		"Failed to deserialize float data");
	fail_if(ctarget != csource, "Failed to get correct control message for float");
	fail_if(mtarget != msource,
		"Failed to get correct message id for float");
	fail_if(dsource1.type != dtarget[0].type,
		"Source and destination type differ for float");
	fail_if(dsource1.store.d_float != dtarget[0].store.d_float,
		"Source and destination float data differ");
	free(packed);
	free(dtarget);

	dsource1.type = DOUBLE;
	dsource1.store.d_double = 3.1415;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(ret == 0, "Failed to serialize double data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) != 1,
		"Failed to deserialize double data");
	fail_if(ctarget != csource, "Failed to get correct control message for double");
	fail_if(mtarget != msource,
		"Failed to get correct message id for double");
	fail_if(dsource1.type != dtarget[0].type,
		"Source and destination type differ for double");
	fail_if(dsource1.store.d_double != dtarget[0].store.d_double,
		"Source and destination double data differ");
	free(packed);
	free(dtarget);

	dsource1.type = BOOLEAN;
	dsource1.store.d_boolean = true;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(ret == 0, "Failed to serialize boolean data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) != 1,
		"Failed to deserialize boolean data");
	fail_if(ctarget != csource, "Failed to get correct control message for boolean");
	fail_if(mtarget != msource,
		"Failed to get correct message id for boolean");
	fail_if(dsource1.type != dtarget[0].type,
		"Source and destination type differ for boolean");
	fail_if(dsource1.store.d_boolean != dtarget[0].store.d_boolean,
		"Source and destination boolean data differ");
	free(packed);
	free(dtarget);

	dsource1.type = INT32;
	dsource1.store.d_int32 = 1;
	dsource2.type = INT32;
	dsource2.store.d_int32 = 2;
	csource = BUXTON_CONTROL_STATUS;
	r = buxton_array_add(list, &dsource2);
	fail_if(!r, "Failed to add element to array");
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(ret == 0, "Failed to serialize 2arg data");
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) != 2,
		"Failed to deserialize 2arg data");
	fail_if(ctarget != csource, "Failed to get correct control message for 2arg");
	fail_if(mtarget != msource,
		"Failed to get correct message id for 2arg");
	fail_if(dsource1.type != dtarget[0].type,
		"1 Source and destination type differ for 2arg");
	fail_if(dsource1.store.d_int32 != dtarget[0].store.d_int32,
		"1 Source and destination differ for 2arg data");
	fail_if(dsource2.type != dtarget[1].type,
		"2 Source and destination type differ for 2arg");
	fail_if(dsource2.store.d_int32 != dtarget[1].store.d_int32,
		"2 Source and destination differ for 2arg data");
	free(packed);
	free(dtarget);

	list2 = buxton_array_new();
	fail_if(!list, "Failed to allocate list");
	list2->len = 0;
	dsource1.type = STRING;
	dsource1.store.d_string = buxton_string_pack("test-key");
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, msource, list2);
	fail_if(ret == 0, "Unable to serialize with 0 element list");

	list2->len = BUXTON_MESSAGE_MAX_PARAMS + 1;
	ret = buxton_serialize_message(&packed, csource, msource, list2);
	fail_if(ret != 0, "Serialized with too many parameters");

	list2->len = 0;
	r = buxton_array_add(list2, &dsource1);
	fail_if(!r, "Failed to add element to array");
	list2->len = 2;
	ret = buxton_serialize_message(&packed, csource, msource, list2);
	fail_if(ret != 0, "Serialized with incorrect parameter count");
	list2->len = 0;

	dsource1.type = -1;
	dsource1.store.d_string = buxton_string_pack("test-key");
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(ret != 0, "Serialized with bad data type");

	dsource1.type = STRING;
	dsource1.store.d_string = buxton_string_pack("test-key");
	csource = -1;
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(ret != 0, "Serialized with bad message type");

	dsource1.type = INT32;
	dsource1.store.d_int32 = INT_MAX;
	csource = BUXTON_CONTROL_GET;
	ret = buxton_serialize_message(&packed, csource, msource, list);
	fail_if(buxton_deserialize_message(packed, &ctarget,
					   BUXTON_MESSAGE_HEADER_LENGTH - 1,
					   &mtarget, &dtarget) >= 0,
		"Deserialized message with too small a length data");

	/* don't read past end of buffer check */
	fail_if(buxton_deserialize_message(packed, &ctarget,
					   (sizeof(uint32_t) * 3)
					   + sizeof(uint32_t)
					   + sizeof(uint16_t)
					   + (sizeof(uint32_t) * 2),
					   &mtarget, &dtarget) >= 0,
		"Deserialized message size smaller than minimum data length");

	control = 0x0000;
	memcpy(packed, &control, sizeof(uint16_t));
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) >= 0,
		"Deserialized message with invalid control");
	free(packed);

	ret = buxton_serialize_message(&packed, csource, msource, list);
	message = BUXTON_CONTROL_MIN;
	memcpy(packed+sizeof(uint16_t), &message, sizeof(uint16_t));
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) >= 0,
		"Deserialized message with invalid control");
	free(packed);

	ret = buxton_serialize_message(&packed, csource, msource, list);
	message = BUXTON_CONTROL_MAX;
	memcpy(packed+sizeof(uint16_t), &message, sizeof(uint16_t));
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) >= 0,
		"Deserialized message with invalid control");
	free(packed);

	ret = buxton_serialize_message(&packed, csource, msource, list);
	pcount = 0;
	memcpy(packed+(2 * sizeof(uint32_t)+sizeof(uint32_t)), &pcount, sizeof(uint32_t));
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) < 0,
		"Unable to deserialize message with 0 BuxtonData");
	free(packed);

	ret = buxton_serialize_message(&packed, csource, msource, list);
	pcount = BUXTON_MESSAGE_MAX_PARAMS + 1;
	memcpy(packed+(2 * sizeof(uint32_t)+sizeof(uint32_t)), &pcount, sizeof(uint32_t));
	fail_if(buxton_deserialize_message(packed, &ctarget, ret, &mtarget,
					   &dtarget) >= 0,
		"Unable to deserialize message with 0 BuxtonData");
	free(packed);

	buxton_array_free(&list, NULL);
	buxton_array_free(&list2, NULL);
}
END_TEST

START_TEST(buxton_get_message_size_check)
{
	BuxtonControlMessage csource;
	BuxtonData dsource;
	uint8_t *packed = NULL;
	BuxtonArray *list = NULL;
	size_t ret;
	bool r;

	list = buxton_array_new();
	fail_if(!list, "Failed to allocate list");
	dsource.type = STRING;
	dsource.store.d_string = buxton_string_pack("test-key");
	csource = BUXTON_CONTROL_GET;
	r = buxton_array_add(list, &dsource);
	fail_if(!r, "Failed to add element to array");
	ret = buxton_serialize_message(&packed, csource, 0, list);
	fail_if(ret == 0, "Failed to serialize string data for size");
	fail_if(ret != buxton_get_message_size(packed, ret),
		"Failed to get correct message size");
	fail_if(buxton_get_message_size(packed, BUXTON_MESSAGE_HEADER_LENGTH - 1) != 0,
		"Got size even though message smaller than the minimum");

	free(packed);
	buxton_array_free(&list, NULL);
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

	tc = tcase_create("hashmap_functions");
	tcase_add_test(tc, hashmap_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("array_functions");
	tcase_add_test(tc, array_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("list_functions");
	tcase_add_test(tc, list_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("util_functions");
	tcase_add_test(tc, get_layer_path_check);
	tcase_add_test(tc, buxton_data_copy_check);
	tcase_add_test(tc, buxton_type_as_string_check);
	tcase_add_test(tc, _write_check);
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

	putenv("BUXTON_CONF_FILE=" ABS_TOP_BUILDDIR "/test/test.conf");
	s = shared_lib_suite();
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
