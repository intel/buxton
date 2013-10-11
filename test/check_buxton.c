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
#include "../src/include/bt-daemon.h"

START_TEST(buxton_client_open_check)
{
	BuxtonClient c;
	fail_if(buxton_client_open(&c) == true,
		"Connection opened without daemon.");
}
END_TEST

START_TEST(buxton_direct_open_check)
{
	BuxtonClient c;
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
}
END_TEST

START_TEST(buxton_client_set_value_check)
{
	BuxtonClient c;
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
	BuxtonData data;
	data.type = STRING;
	data.store.d_string = "bxt_test_value";
	fail_if(buxton_client_set_value(&c, "test-gdbm", "bxt_test", &data) == false,
		"Setting value in buxton directly failed.");
}
END_TEST

START_TEST(buxton_memory_backend_check)
{
	BuxtonClient c;
	fail_if(buxton_direct_open(&c) == false,
		"Direct open failed without daemon.");
	BuxtonData data, result;
	data.type = STRING;
	data.store.d_string = "bxt_test_value";
	fail_if(buxton_client_set_value(&c, "temp", "bxt_mem_test", &data) == false,
		"Setting value in buxton memory backend directly failed.");
	fail_if(buxton_client_get_value(&c, "temp", "bxt_mem_test", &data) == false,
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
	tcase_add_test(tc, buxton_client_open_check);

	tcase_add_test(tc, buxton_direct_open_check);

	tcase_add_test(tc, buxton_client_set_value_check);

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
