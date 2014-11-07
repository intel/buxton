/*
 * This file is part of buxton.
 *
 * Copyright (C) 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "buxtonsimple.h"

/* DEMONSTRATION */
int main(void)
{
	/* Create group */
	errno = 0;
	sbuxton_set_group("tg_s0", "user");
	printf("set_group: 'tg_s0', 'user', Error number: %s.\n", strerror(errno));

	/* Test string setting */
	srand((unsigned)time(NULL));
	char * s="Watermelon";
	printf("value should be set to %s.\n", s);
	errno = 0;
	sbuxton_set_string("tk_s1", s);
	printf("set_string: 'tg_s0', 'tk_s1', Error number: %s.\n", strerror(errno));

	/* Test string getting */
	char * sv = sbuxton_get_string("tk_s1");
	printf("Got value: %s(string).\n", sv);
	printf("get_string: 'tk_s1', Error number: %s.\n", strerror(errno));
	free(sv);

	/* Create group */
	errno = 0;
	sbuxton_set_group("tg_s1", "user");
	printf("set_group: 'tg_s1', Error number: %s.\n", strerror(errno));

	/* Test int32 setting */
	srand((unsigned)time(NULL));
	int32_t i=rand() % 100 + 1;
	printf("value should be set to %i.\n",i);
	errno = 0;
	sbuxton_set_int32("tk_i32", i);
	printf("set_int32: 'tg_s1', 'tk_i32', Error number: %s.\n", strerror(errno));

	/* Create group */
	errno = 0;
	sbuxton_set_group("tg_s2", "user");
	printf("set_group: 'tg_s2', Error number: %s.\n", strerror(errno));

	/* Test int32 setting */
	srand((unsigned)time(NULL));
	int32_t i2=rand() % 1000 + 1;
	printf("Second value should be set to %i.\n", i2);
	errno = 0;
	sbuxton_set_int32("tk_i32b", i2);
	printf("set_int32: 'tg_s2', 'tk_i32b', Error number: %s.\n", strerror(errno));

	/* Test int32 getting */
	/* Change group */
	errno = 0;
	sbuxton_set_group("tg_s1", "user");
	printf("set_group: 'tg_s1', Error number: %s.\n", strerror(errno));
	errno = 0;
	/* Get int32 */
	int32_t iv = sbuxton_get_int32("tk_i32");
	printf("get_int32: 'tg_s1', 'tk_i32', Error number: %s.\n", strerror(errno));
	printf("Got value: %i(int32_t).\n", iv);
	errno = 0;
	/* Change group */
	sbuxton_set_group("tg_s2", "user");
	printf("set_group: 'tg_s2', Error number: %s.\n", strerror(errno));
	errno = 0;
	/* Get int32 */
	int32_t i2v = sbuxton_get_int32("tk_i32b");
	printf("Got value: %i(int32_t).\n", i2v);
	printf("get_int32: 'tg_s2', 'tk_i32b', Error number: %s.\n", strerror(errno));

	/* Create group */
	errno = 0;
	sbuxton_set_group("tg_s3", "user");
	printf("set_group: 'tg_s3', Error number: %s.\n", strerror(errno));

	/* Test uint32 setting */
	uint32_t ui32 = (uint32_t) rand() % 50 + 1;
	printf("value should be set to %u.\n", ui32);
	errno = 0;
	sbuxton_set_uint32("tk_ui32", ui32);
	printf("set_uint32: 'tg_s3', 'tk_ui32', Error number: %s.\n", strerror(errno));
	/* Test uint32 getting */
	errno = 0;
	uint32_t ui32v = sbuxton_get_uint32("tk_ui32");
	printf("Got value: %i(uint32_t).\n", ui32v);
	printf("get_uint32: 'tg_s3', 'tk_ui32', Error number: %s.\n", strerror(errno));

	/* Test  int64 setting */
	int64_t i64 = rand() % 1000 + 1;
	printf("value should be set to ""%"PRId64".\n", i64);
	errno = 0;
	sbuxton_set_int64("tk_i64", i64);
	/* Test int64 getting */
	errno = 0;
	int64_t i64v = sbuxton_get_int64("tk_i64");
	printf("Got value: ""%"PRId64"(int64_t).\n", i64v);
	printf("get_int64: 'tg_s3', 'tk_i64', Error number: %s.\n", strerror(errno));

	/* Change group */
	errno = 0;
	sbuxton_set_group("tg_s0", "user");

	/* Test uint64 setting */
	uint64_t ui64 = (uint64_t) rand() % 500 + 1;
	printf("value should be set to ""%"PRIu64".\n", ui64);
	errno = 0;
	sbuxton_set_uint64("tk_ui64", ui64);
	/* Test uint64 getting */
	errno = 0;
	uint64_t ui64v = sbuxton_get_uint64("tk_ui64");
	printf("Got value: ""%"PRIu64"(uint64_t).\n", ui64v);
	printf("get_uint64: 'tg_s0', 'tk_ui64', Error number: %s.\n", strerror(errno));

	/* Test float setting */
	float f = (float) (rand() % 9 + 1);
	printf("value should be set to %e.\n", f);
	errno = 0;
	sbuxton_set_float("tk_f", f);
	/* Test float getting */
	errno = 0;
	float fv = sbuxton_get_float("tk_f");
	printf("Got value: %e(float).\n", fv);
	printf("get_float: 'tg_s0', 'tk_f', Error number: %s.\n", strerror(errno));

	/* Test double setting */
	double d = rand() % 7000 + 1;
	printf("value should be set to %e.\n", d);
	errno = 0;
	sbuxton_set_double("tk_d", d);
	/* Test double getting */
	errno = 0;
	double dv = sbuxton_get_double("tk_d");
	printf("Got value: %e(double).\n", dv);
	printf("get_double: 'tg_s0', 'tk_f', Error number: %s.\n", strerror(errno));

	/* Test boolean setting */
	bool b = true;
	printf("value should be set to %i.\n", b);
	errno = 0;
	sbuxton_set_bool("tk_b", b);
	/* Test boolean getting */
	errno = 0;
	bool bv = sbuxton_get_bool("tk_b");
	printf("Got value: %i(bool).\n", bv);		
	printf("get_bool: 'tg_s0', 'tk_b', Error number: %s.\n", strerror(errno));

	/* Remove groups */
	errno = 0;
	sbuxton_remove_group("tg_s1", "user");
	printf("remove_group: 'tg_s1', 'user', Error number: %s.\n", strerror(errno));
	errno = 0;
	sbuxton_remove_group("tg_s0", "user");
	printf("remove_group: 'tg_s0', 'user', Error number: %s.\n", strerror(errno));
	errno = 0;
	sbuxton_remove_group("tg_s2", "user");
	printf("remove_group: 'tg_s2', 'user', Error number: %s.\n", strerror(errno));
	errno = 0;
	sbuxton_remove_group("tg_s3", "user");
	printf("remove_group: 'tg_s3', 'user', Error number: %s.\n", strerror(errno));

	return 0;
}
