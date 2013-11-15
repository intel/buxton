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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "bt-daemon.h"
#include "util.h"

#define error(...) { printf(__VA_ARGS__); goto end; }

typedef bool (*TestFunction) (BuxtonClient *client);

bool timed_func(TestFunction func, BuxtonClient *client, unsigned long long *total)
{
	struct timespec tsi, tsf;
	unsigned long long elapsed;
	bool ret;

	clock_gettime(CLOCK_MONOTONIC, &tsi);
	ret = func(client);
	clock_gettime(CLOCK_MONOTONIC, &tsf);

	elapsed = (unsigned long long)(tsf.tv_nsec - tsi.tv_nsec);
	elapsed += *total;

	*total = elapsed;
	return ret;
}

bool set_value_test(BuxtonClient *client)
{
	BuxtonString layer, key, value;
	BuxtonData data;
	bool ret;

	layer = buxton_string_pack("base");
	key = buxton_string_pack("ValueTestTest");
	value = buxton_string_pack("SomeRandomString");
	buxton_string_to_data(&value, &data);
	data.label = buxton_string_pack("_");

	ret = buxton_client_set_value(client, &layer, &key, &data);
	return ret;
}

int main(int argc, char **argv)
{
	BuxtonClient client;
	int i;
	int ret = EXIT_FAILURE;
	unsigned long long total;
	long num_tests = 0;
	float average;

	if (!buxton_client_open(&client))
		error("Unable to open BuxtonClient\n");

	if (!set_value_test(&client))
		error("Unable to set string\n");

	/* 10,000 set operations using wire protocol */
	num_tests = 10000;
	for (i=0; i < num_tests; i++) {
		if (!timed_func(set_value_test, &client, &total))
			error("Unable to set string\n");
	}
	total /= 1000000;
	average = ((float)total) / ((float)num_tests);
	printf("set_value_test: %ld operations took %lldms; Average time: %lfms\n", num_tests, total, average);

	ret = EXIT_SUCCESS;
	buxton_client_close(&client);
end:
	return ret;
}
