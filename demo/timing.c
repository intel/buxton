/*
 * This file is part of buxton.
 *
 * Copyright (C) 2013 Intel Corporation
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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "buxton.h"
#include "util.h"

#define error(...) { printf(__VA_ARGS__); }

static int iterations = 100000;

static void callback(BuxtonResponse response, void *userdata)
{
	bool *r;

	if (!userdata) {
		return;
	}

	r = (bool *)userdata;

	if (buxton_response_status(response) == 0) {
		*r = true;
	}
}

enum test_type {
	TEST_GET,
	TEST_SET,
	TEST_SET_UNSET,
	TEST_TYPE_MAX
};

enum data_type {
	TEST_INT32,
	TEST_UINT32,
	TEST_INT64,
	TEST_UINT64,
	TEST_BOOLEAN,
	TEST_STRING,
	TEST_STRING4K,
	TEST_FLOAT,
	TEST_DOUBLE,
	TEST_DATA_TYPE_MAX
};

struct testcase {
	char name[32];
	enum test_type t;
	enum data_type d;
};

#define TEST_COUNT (TEST_TYPE_MAX * TEST_DATA_TYPE_MAX)
static struct testcase testcases[TEST_COUNT] = {
	{ "set_int32",          TEST_SET,       TEST_INT32    },
	{ "get_int32",          TEST_GET,       TEST_INT32    },
	{ "set_unset_int32",    TEST_SET_UNSET, TEST_INT32    },
	{ "set_uint32",         TEST_SET,       TEST_UINT32   },
	{ "get_uint32",         TEST_GET,       TEST_UINT32   },
	{ "set_unset_uint32",   TEST_SET_UNSET, TEST_UINT32   },
	{ "set_int64",          TEST_SET,       TEST_INT64    },
	{ "get_int64",          TEST_GET,       TEST_INT64    },
	{ "set_unset_int64",    TEST_SET_UNSET, TEST_INT64    },
	{ "set_uint64",         TEST_SET,       TEST_UINT64   },
	{ "get_uint64",         TEST_GET,       TEST_UINT64   },
	{ "set_unset_uint64",   TEST_SET_UNSET, TEST_UINT64   },
	{ "set_boolean",        TEST_SET,       TEST_BOOLEAN  },
	{ "get_boolean",        TEST_GET,       TEST_BOOLEAN  },
	{ "set_unset_boolean",  TEST_SET_UNSET, TEST_BOOLEAN  },
	{ "set_string",         TEST_SET,       TEST_STRING   },
	{ "get_string",         TEST_GET,       TEST_STRING   },
	{ "set_unset_string",   TEST_SET_UNSET, TEST_STRING   },
	{ "set_string4k",       TEST_SET,       TEST_STRING4K },
	{ "get_string4k",       TEST_GET,       TEST_STRING4K },
	{ "set_unset_string4k", TEST_SET_UNSET, TEST_STRING4K },
	{ "set_float",          TEST_SET,       TEST_FLOAT    },
	{ "get_float",          TEST_GET,       TEST_FLOAT    },
	{ "set_unset_float",    TEST_SET_UNSET, TEST_FLOAT    },
	{ "set_double",         TEST_SET,       TEST_DOUBLE   },
	{ "get_double",         TEST_GET,       TEST_DOUBLE   },
	{ "set_unset_double",   TEST_SET_UNSET, TEST_DOUBLE   }
};

static BuxtonClient __client;
static BuxtonData __data;
static BuxtonKey __key;

static bool init_group(void)
{
	BuxtonKey group;
	bool r;
	bool d = false;

	group = buxton_key_create("TimingTest", NULL, "user", STRING);
	r = buxton_create_group(__client, group, callback, &d, true);

	free(group);

	return (!r && d);
}

static bool testcase_init(struct testcase *tc)
{
	char name[64] = "";
	int32_t i;
	uint32_t ui;
	int64_t i6;
	uint64_t ui6;
	bool b;
	char *string;
	float f;
	double d;
	void *value = NULL;

	switch (tc->d) {
		case TEST_INT32:
			sprintf(name, "TimingTest-%d-int32", getpid());
			__key = buxton_key_create("TimingTest", name, "user", INT32);
			i = -672;
			value = &i;
			break;
		case TEST_UINT32:
			sprintf(name, "TimingTest-%d-uint32", getpid());
			__key = buxton_key_create("TimingTest", name, "user", UINT32);
			ui = 672;
			value = &ui;
			break;
		case TEST_INT64:
			sprintf(name, "TimingTest-%d-int64", getpid());
			__key = buxton_key_create("TimingTest", name, "user", INT64);
			i6 = -672 * 672;
			value = &i6;
			break;
		case TEST_UINT64:
			sprintf(name, "TimingTest-%d-uint64", getpid());
			__key = buxton_key_create("TimingTest", name, "user", UINT64);
			ui6 = 672 * 672;
			value = &ui6;
			break;
		case TEST_BOOLEAN:
			sprintf(name, "TimingTest-%d-boolean", getpid());
			__key = buxton_key_create("TimingTest", name, "user", BOOLEAN);
			b = true;
			value = &b;
			break;
		case TEST_STRING:
			sprintf(name, "TimingTest-%d-string", getpid());
			__key = buxton_key_create("TimingTest", name, "user", STRING);
			string = "672";
			value = string;
			break;
		case TEST_STRING4K:
			sprintf(name, "TimingTest-%d-string4k", getpid());
			__key = buxton_key_create("TimingTest", name, "user", STRING);
			string = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
			value = string;
			break;
		case TEST_FLOAT:
			sprintf(name, "TimingTest-%d-float", getpid());
			__key = buxton_key_create("TimingTest", name, "user", FLOAT);
			f = (float)3.14;
			value = &f;
			break;
		case TEST_DOUBLE:
			sprintf(name, "TimingTest-%d-double", getpid());
			__key = buxton_key_create("TimingTest", name, "user", DOUBLE);
			d = 3.14;
			value = &d;
			break;
		default:
			return false;
	}

	return !buxton_set_value(__client, __key, value, callback, NULL, true);
}

static bool testcase_cleanup(struct testcase *tc)
{
	bool ret = (!buxton_set_value(__client, __key, &__data, callback, NULL, true) &&
		!buxton_unset_value(__client,  __key, callback, NULL, true));
	buxton_key_free(__key);
	return ret;
}

static bool testcase_run(struct testcase *tc)
{
	bool d = false;
	bool r, s;
	switch (tc->t) {
		case TEST_GET:
			r = buxton_get_value(__client, __key, callback, &d, true);
			return (!r && d);
		case TEST_SET:
			r = buxton_set_value(__client, __key, &__data, callback, &d, true);
			return (!r && d);
		case TEST_SET_UNSET:
			r = buxton_set_value(__client, __key, &__data, callback, &d, true);
			s = buxton_unset_value(__client, __key, callback, &d, true);
			return (!s && !r && d);
		default:
			return false;
	}
}

static bool timed_func(unsigned long long *elapsed, struct testcase *tc)
{
	struct timespec tsi, tsf;
	bool ret;

	clock_gettime(CLOCK_MONOTONIC, &tsi);
	ret = testcase_run(tc);
	clock_gettime(CLOCK_MONOTONIC, &tsf);

	*elapsed = (unsigned long long)((tsf.tv_nsec - tsi.tv_nsec) + ((tsf.tv_sec - tsi.tv_sec) * 1000000000));
	return ret;
}

static void test(struct testcase *tc)
{
	unsigned long long elapsed;
	unsigned long long total;
	unsigned long long errors = 0;
	double meansqr;
	double mean;
	double sigma;
	int i;

	total = 0;
	meansqr = 0.0;
	mean = 0.0;

	testcase_init(tc);

	for (i = 0; i < iterations; i++) {
		if (!timed_func(&elapsed, tc)) {
			errors++;
		}

		mean += (double)elapsed;
		meansqr += (double)elapsed * (double)elapsed;
		total += elapsed;
	}
	mean /= (double)iterations;
	meansqr /= (double)iterations;
	sigma = sqrt(meansqr - (mean * mean));

	testcase_cleanup(tc);

	printf("%-24s  %10.3lfus  %10.3lfus  %10llu\n",
	       tc->name, mean / 1000.0, sigma / 1000.0, errors);
}

int main(int argc, char **argv)
{
	int ret = EXIT_SUCCESS;
	int i;

	if (argc == 2) {
		iterations = atoi(argv[1]);
		if (iterations <= 0) {
			exit(EXIT_FAILURE);
		}
	} else if (argc != 1) {
		error("Usage: %s [iterations]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (!buxton_open(&__client)) {
		error("Unable to open BuxtonClient\n");
		exit(EXIT_FAILURE);
	}

	init_group();

	printf("Buxton protocol latency timing tool. Using %i iterations per test.\n", iterations);
	printf("Test Name:                   Average:        Sigma:     Errors:\n");

	for (i = 0; i < TEST_COUNT; i++)
		test(&testcases[i]);

	buxton_close(__client);
	exit(ret);
}

