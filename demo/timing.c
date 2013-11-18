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
#include <math.h>

#include "bt-daemon.h"
#include "util.h"

#define error(...) { printf(__VA_ARGS__); goto end; }

#define ITERATIONS 100000

typedef bool (*TestFunction) (BuxtonClient *client);

bool timed_func(TestFunction func, BuxtonClient *client, unsigned long long *elapsed)
{
	struct timespec tsi, tsf;
	bool ret;

	clock_gettime(CLOCK_MONOTONIC, &tsi);
	ret = func(client);
	clock_gettime(CLOCK_MONOTONIC, &tsf);

	*elapsed = (unsigned long long)((tsf.tv_nsec - tsi.tv_nsec) + ((tsf.tv_sec - tsi.tv_sec) * 1000000000));
	return ret;
}

bool set_string(BuxtonClient *client)
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

bool get_string(BuxtonClient *client)
{
	BuxtonString key;
	BuxtonData data;
	bool ret;

	key = buxton_string_pack("ValueTestTest");

	ret = buxton_client_get_value(client, &key, &data);

	return ret;
}

static bool test(TestFunction func, BuxtonClient *client)
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

	for (i = 0; i < ITERATIONS; i++) {
		if (!timed_func(func, client, &elapsed))
			return false;

		mean += (double)elapsed;
		meansqr += (double)elapsed * (double)elapsed;
		total += elapsed;
	}
	mean /= (double)ITERATIONS;
	meansqr /= (double)ITERATIONS;
	sigma = sqrt(meansqr - (mean * mean));

	printf("get_string: %i operations took %0.3lfs; Average time: %0.3lfus, sigma: %0.3lfus\n",
	       ITERATIONS, (double)total / 1000000000.0, mean / 1000.0, sigma / 1000.0);

	return true;
}

int main(int argc, char **argv)
{
	BuxtonClient client;
	int ret = EXIT_FAILURE;

	if (!buxton_client_open(&client)) {
		error("Unable to open BuxtonClient\n");
		exit(EXIT_FAILURE);
	}

	if (!get_string(&client)) {
		error("Unable to get string\n");
		exit(EXIT_FAILURE);
	}

	if (!set_string(&client)) {
		error("Unable to set string\n");
		exit(EXIT_FAILURE);
	}

	test(get_string, &client);
	test(set_string, &client);

	ret = EXIT_SUCCESS;
	buxton_client_close(&client);
end:
	return ret;
}
