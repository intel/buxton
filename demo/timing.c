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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "bt-daemon.h"
#include "util.h"

#define error(...) { printf(__VA_ARGS__); }

static int iterations = 100000;

typedef bool (*TestFunction) (BuxtonClient *client);

enum test_type {
	TEST_GET,
	TEST_SET,
	TEST_SET_UNSET
};

enum data_type {
	TEST_INT32,
	TEST_STRING,
	TEST_STRING4K
};

struct testcase {
	char name[32];
	enum test_type t;
	enum data_type d;
};

#define TEST_COUNT 9
struct testcase testcases[TEST_COUNT] = {
	{ "set_int32",          TEST_SET,       TEST_INT32    },
	{ "get_int32",          TEST_GET,       TEST_INT32    },
	{ "set_unset_int32",    TEST_SET_UNSET, TEST_INT32    },
	{ "set_string",         TEST_SET,       TEST_STRING   },
	{ "get_string",         TEST_GET,       TEST_STRING   },
	{ "set_unset_string",   TEST_SET_UNSET, TEST_STRING   },
	{ "set_string4k",       TEST_SET,       TEST_STRING4K },
	{ "get_string4k",       TEST_GET,       TEST_STRING4K },
	{ "set_unset_string4k", TEST_SET_UNSET, TEST_STRING4K }
};

static BuxtonClient __client;
static BuxtonString *__key;
static BuxtonString __layer;
static BuxtonData __data;

static bool testcase_init(struct testcase *tc)
{
	BuxtonString value;
	char name[64] = "";

	__layer = buxton_string_pack("base");
	switch (tc->d) {
		case TEST_INT32:
			sprintf(name, "TimingTest-%d-int32", getpid());
			__data.type = INT32;
			__data.store.d_int32 = 672;
			break;
		case TEST_STRING:
			sprintf(name, "TimingTest-%d-string", getpid());
			value = buxton_string_pack("672");
			buxton_string_to_data(&value, &__data);
			break;
		case TEST_STRING4K:
			sprintf(name, "TimingTest-%d-string4k", getpid());
			value = buxton_string_pack("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
			buxton_string_to_data(&value, &__data);
			break;
		default:
			return false;
	}
	__key = buxton_make_key("TimingTest", name);
	__data.label = buxton_string_pack("*");

	return buxton_client_set_value(&__client, &__layer, __key, &__data);
}

static bool testcase_cleanup(struct testcase *tc)
{
	return (buxton_client_set_value(&__client, &__layer, __key, &__data) &&
		buxton_client_unset_value(&__client, &__layer, __key));
}

static bool testcase_run(struct testcase *tc)
{
	switch (tc->t) {
		case TEST_GET:
			return buxton_client_get_value(&__client, __key, &__data);
		case TEST_SET:
			return buxton_client_set_value(&__client, &__layer, __key, &__data);
		case TEST_SET_UNSET:
			return (buxton_client_set_value(&__client, &__layer, __key, &__data) &&
				buxton_client_unset_value(&__client, &__layer, __key));
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

static bool test(struct testcase *tc)
{
	unsigned long long elapsed;
	unsigned long long total;
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
			fprintf(stderr, "Test %s failed\n", tc->name);
			return false;
		}

		mean += (double)elapsed;
		meansqr += (double)elapsed * (double)elapsed;
		total += elapsed;
	}
	mean /= (double)iterations;
	meansqr /= (double)iterations;
	sigma = sqrt(meansqr - (mean * mean));

	testcase_cleanup(tc);

	printf("%-24s  %10.3lfus  %10.3lfus\n",
	       tc->name, mean / 1000.0, sigma / 1000.0);

	return true;
}

int main(int argc, char **argv)
{
	int ret = EXIT_SUCCESS;
	int i;

	if (argc == 2) {
		iterations = atoi(argv[1]);
		if (iterations <= 0)
			exit(EXIT_FAILURE);
	} else if (argc != 1) {
		error("Usage: %s [iterations]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (!buxton_client_open(&__client)) {
		error("Unable to open BuxtonClient\n");
		exit(EXIT_FAILURE);
	}

	printf("Buxton protocol latency timing tool. Using %i iterations per test.\n", iterations);
	printf("Test Name:                   Average:        Sigma:\n");

	for (i = 0; i < TEST_COUNT; i++) {
		if (!test(&testcases[i]))
			break;
	}

	buxton_client_close(&__client);
	exit(ret);
}

