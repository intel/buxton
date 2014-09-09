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
#include <string.h>

#include "buxtonarray.h"
#include "util.h"

/* size of initial allocation in count of items */
#define INITIAL_ARRAY_ALLOC  8  /* MUST be a power of 2!!! */

BuxtonArray *buxton_array_new(void)
{
	BuxtonArray *ret = NULL;
	/* If this fails, we simply return NULL and allow the user
	 * to deal with the error */
	ret = calloc(1, sizeof(BuxtonArray));
	return ret;
}

bool buxton_array_add(BuxtonArray *array,
		      void *data)
{
	uint16_t len, alen;
	void **p;

	if (!array || !data) {
		return false;
	}

	p = array->data;
	len = array->len;
	if (!(len + 1)) {
		return false;
	}

	/*
	* Resize the array if needed to hold at least one more pointer
	*
	* ALGO: the current length is scanned and the reallocation
	* occurs only if it is a power of 2, doubling the allocated size.
	* Also, a minimal allocation count is managed
	*/
	if (!(len & (len - 1)) && (!len || len >= INITIAL_ARRAY_ALLOC)) {
		alen = MAX(INITIAL_ARRAY_ALLOC, 2 * len);
		p = realloc(p, alen * sizeof * p);
		if (!p) {
			return false;
		}
		/* this following erasing isn't strictly needed */
		memset(p + len + 1, 0, (alen - len - 1) * sizeof * p);
		array->data = p;
	}

	/* Store the data at the end of the array that is updated */
	assert(p);
	p[len] = data;
	array->len = len + 1;

	return true;
}

void *buxton_array_get(BuxtonArray *array, uint16_t index)
{
	if (!array) {
		return NULL;
	}
	if (index >= array->len) {
		return NULL;
	}
	return array->data[index];
}

void buxton_array_free(BuxtonArray **array,
		       buxton_free_func free_method)
{
	int i;
	if (!array || !*array) {
		return;
	}

	if (free_method) {
		/* Call the free_method on all members */
		for (i = 0; i < (*array)->len; i++) {
			free_method((*array)->data[i]);
		}
	}
	/* Ensure this array is indeed gone. */
	free((*array)->data);
	free(*array);
	*array = NULL;
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
