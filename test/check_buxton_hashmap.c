/*
 * This file is part of buxton.
 *
 * Copyright (C) 2014 Intel Corporation
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
#include <malloc.h>
#include <stdlib.h>

#include "buxtonhashmap.h"
#include "buxtonhashmap.c"
#include "check_utils.h"
#include "log.h"

#ifdef NDEBUG
#error "re-run configure with --enable-debug"
#endif

static inline void _bmap_free(const void *p)
{
	free((void*)p);
}

START_TEST(buxton_hashmap_new_check)
{
	BuxtonHashmap *map = buxton_hashmap_new(NULL, NULL);
	fail_if(!map, "Failed to allocate map");
	fail_if(buxton_hashmap_size(map) != 0, "Failed to allocate empty map");

	buxton_hashmap_free(map);
}
END_TEST

START_TEST(buxton_hashmap_complex_check)
{
	BuxtonHashmap *map;
	char *value;
	HashValue hvalue;
	bool r;
	uint count = 0;
	char *kval, *kkey;

	map = buxton_hashmap_new_full(string_compare, string_hash, NULL, NULL);
	fail_if(map == NULL, "Failed to allocate hashmap");
	r = buxton_hashmap_put(&map, "test", "passed");
	fail_if(r != true, "Failed to add element to hashmap");

	value = buxton_hashmap_get(map, "test");

	fail_if(value == NULL,
		"Failed to get value from hashmap");

	fail_if(strcmp(value, "passed") != 0,
		"Failed to retrieve the put value");

	fail_if(!buxton_hashmap_contains(map, "test"),
		"Failed to find item in hashmap");

	buxton_hashmap_remove(map, "test");
	fail_if(buxton_hashmap_size(map) != 0,
		"Failed to remove item from hashmap");

	buxton_hashmap_free(map);

	map = buxton_hashmap_new(NULL, NULL);
	fail_if(map == NULL, "Failed to allocate hashmap");

	for (uint i = 0; i < 1000; i++) {
		r = buxton_hashmap_put(&map, HASH_KEY(i), HASH_VALUE(i));
		fail_if(r == false, "Failed to add item to hashmap");
	}

	BUXTON_HASHMAP_FOREACH(map, iter, key, v) {
		if (key) { /* Prevent unused warnings */ }
		count += 1;
	}
	fail_if(count != 1000, "Failed to iterate all items");
	fail_if(count != buxton_hashmap_size(map), "Failed to match hashmap size to count");

	buxton_hashmap_free(map);

	/* Simple comparison test */
	fail_if(simple_compare(HASH_KEY(10), HASH_KEY(10)) != true,
		"Failed to perform simple_compare");

	/* Forced hash collision */
	map = buxton_hashmap_new_internal(2, simple_compare, simple_hash, NULL, NULL, false);

	for (uint i = 0; i < 10; i++) {
		r = buxton_hashmap_put(&map, HASH_KEY(i), HASH_VALUE(i));
		fail_if(r != true, "Failed to add item to hashmap");
	}
	for (uint i = 0; i < 10; i++) {
		hvalue = buxton_hashmap_get(map, HASH_KEY(i));
		fail_if(UNHASH_VALUE(hvalue) != i,
			"Value returned by hashmap incorrect");
	}
	count = 0;
	BUXTON_HASHMAP_FOREACH(map, iter2, key2, v2) {
		if (key2) { /* Prevent unused warnings */ }
		count += 1;
	}
	fail_if(count != 10, "Failed to iterate all items #2");
	fail_if(count != buxton_hashmap_size(map), "Failed to match hashmap size to count #2");
	/* Hash collision removal */
	r = buxton_hashmap_remove(map, HASH_KEY(5));
	fail_if(r != true, "Failed to remove collided item from hashmap");
	buxton_hashmap_free(map);

	/* Check cleanups . */
	map = buxton_hashmap_new_internal(2, string_compare, string_hash, _bmap_free, _bmap_free, false);
	for (uint i = 0; i < 10; i++) {
		asprintf(&kkey, "key: %i", i);
		asprintf(&kval, "value: %i", i);
		r = buxton_hashmap_put(&map, kkey, kval);
		fail_if(r != true, "Failed to add item to hashmap");
	}
	/* Overwrite one value to force collison paths */
	asprintf(&kval, "value: check");
	r = buxton_hashmap_put(&map, "key: 6", kval);
	fail_if(r != true, "Failed to replace value in hashmap");

	r = buxton_hashmap_remove(map, "key: 5");
	fail_if(r != true, "Failed to remove item from hashmap");

	/* Removal of head test */
	r = buxton_hashmap_remove(map, "key: 0");
	fail_if(r != true, "Failed to remove head item from hashmap");

	buxton_hashmap_free(map);

	/* Check we don't lock in iteration */
	BUXTON_HASHMAP_FOREACH(NULL, iter3, key3, v4) {
		fail_if(key3, "Foreach should never get here in hashmap");
	}
}
END_TEST

static Suite *
buxton_hashmap_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("buxton_hashmap");
	tc = tcase_create("buxton_hashmap_functions");
	tcase_add_test(tc, buxton_hashmap_new_check);
	tcase_add_test(tc, buxton_hashmap_complex_check);
	suite_add_tcase(s, tc);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	putenv("BUXTON_CONF_FILE=" ABS_TOP_BUILDDIR "/test/test.conf");
	s = buxton_hashmap_suite();
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
