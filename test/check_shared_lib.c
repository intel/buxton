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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iniparser.h>
#include "../src/shared/log.h"
#include "../src/shared/hashmap.h"

START_TEST(log_write_check)
{
	char log_file[] = "log-check-stderr-file";
	char log_msg[] = "Log test";
	char *log_read = malloc(strlen(log_msg));
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

	map = hashmap_new(string_hash_func, string_compare_func);
	hashmap_put(map, "test", "passed");

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

Suite *
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
