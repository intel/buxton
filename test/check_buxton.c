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

#include <check.h>
#include <stdlib.h>
#include "bt-daemon.h"
#include "src/shared/backend.h"

#define PACK(s) ((BuxtonString){(s), strlen(s) + 1})

START_TEST(buxton_direct_open_check)
{
	BuxtonClient c;
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
}
END_TEST

START_TEST(buxton_direct_set_value_check)
{
	BuxtonClient c;
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
	BuxtonData data;
	BuxtonString layer = PACK("test-gdbm");
	BuxtonString key = PACK("bxt_test");
	data.type = STRING;
	data.store.d_string = PACK("bxt_test_value");
	fail_if(buxton_client_set_value(&c, &layer, &key, &data) == false,
		"Setting value in buxton directly failed.");
}
END_TEST

START_TEST(buxton_direct_get_value_for_layer_check)
{
	BuxtonClient c;
	BuxtonData result;
	BuxtonString layer = PACK("test-gdbm");
	BuxtonString key = PACK("bxt_test");
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
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

START_TEST(buxton_direct_get_value_check)
{
	BuxtonClient c;
	BuxtonData data, result;
	BuxtonString layer = PACK("test-gdbm-user");
	BuxtonString key = PACK("bxt_test");
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");

	data.type = STRING;
	data.store.d_string = PACK("bxt_test_value2");
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

START_TEST(buxton_memory_backend_check)
{
	BuxtonClient c;
	BuxtonString layer = PACK("temp");
	BuxtonString key = PACK("bxt_mem_test");
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
	BuxtonData data, result;
	data.type = STRING;
	data.store.d_string = PACK("bxt_test_value");
	fail_if(buxton_client_set_value(&c, &layer, &key, &data) == false,
		"Setting value in buxton memory backend directly failed.");
	fail_if(buxton_client_get_value_for_layer(&c, &layer, &key, &data) == false,
		"Retrieving value from buxton memory backend failed.");
}
END_TEST

Suite *
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

	suite_add_tcase(s, tc);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

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
