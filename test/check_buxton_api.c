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

#include "buxton.h"
#include "buxtonarray.h"
#include "buxtondata.h"
#include "buxtonresponse.h"
#include "src/libbuxton/lbuxton.c"
#include "check_utils.h"
#include "log.h"

#ifdef NDEBUG
#error "re-run configure with --enable-debug"
#endif

START_TEST(buxton_response_value_type_check)
{
	BuxtonData d1;
	BuxtonData d2;
	BuxtonArray *a = NULL;
	_BuxtonResponse r;

	fail_if(buxton_response_value_type(NULL) != -1,
		"Returned invalid type for NULL response");
	d1.type = BUXTON_TYPE_STRING;
	a = buxton_array_new();
	fail_if(!buxton_array_add(a, &d1), "Array add failed 1");
	r.data = a;
	r.type = BUXTON_CONTROL_SET;
	fail_if(buxton_response_value_type(&r) != -1,
		"Returned invalid type for SET response");
	r.type = BUXTON_CONTROL_CHANGED;
	fail_if(buxton_response_value_type(&r) != BUXTON_TYPE_STRING,
		"Returned incorrect type for changed response");
	d2.type = BUXTON_TYPE_FLOAT;
	fail_if(!buxton_array_add(a, &d2), "Array add failed 2");
	r.type = BUXTON_CONTROL_GET;
	fail_if(buxton_response_value_type(&r) != BUXTON_TYPE_FLOAT,
		"Returned incorrect type for get response");
	buxton_array_free(&a, NULL);
	r.data = NULL;
	fail_if(buxton_response_value_type(&r) != -1,
		"Returned invalid type for NULL array");
}
END_TEST

static Suite *
daemon_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("buxton public api function");
	tc = tcase_create("api no daemon function tests");
	tcase_add_test(tc, buxton_response_value_type_check);
	suite_add_tcase(s, tc);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	putenv("BUXTON_CONF_FILE=" ABS_TOP_BUILDDIR "/test/test.conf");
	putenv("BUXTON_ROOT_CHECK=0");
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
