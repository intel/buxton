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

#pragma once

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * Utility to easily access members of type BuxtonData in an array
 * @param x a BuxtonArray
 * @param y index of the data in the array
 */
#define BD(x,y)  ((BuxtonData*)x->data[y])

/**
 * A dynamic array
 */
typedef struct BuxtonArray {
	void **data; /**<Dynamic array contents */
	uint len; /**<Length of the array */
} BuxtonArray;

/**
 * Valid function prototype for 'free' method
 * @param p Pointer to free
 */
typedef void (*buxton_free_func) (void* p);

/**
 * Create a new BuxtonArray
 * @returns BuxtonArray a newly allocated BuxtonArray
 */
BuxtonArray *buxton_array_new(void);

/**
 * Append data to BuxtonArray
 * @param array Valid BuxtonArray
 * @param data Pointer to add to this array
 * @returns bool true if the data was added to the array
 */
bool buxton_array_add(BuxtonArray *array,
		      void *data);

/**
 * Determine if an array contains the given data
 * @param array Valid BuxtonArray
 * @param data Pointer to test
 * @returns bool true if array contains the given pointer
 */
bool buxton_array_has(BuxtonArray *array,
		      void *data);

/**
 * Remove data from the array
 * @param array valid BuxtonArray
 * @param data Pointer to remove from this array
 * @param free_method Function to call if you wish to free the data, or NULL
 * @return bool true if the data was removed from the array
 */
bool buxton_array_remove(BuxtonArray *array,
			 void *data,
			 buxton_free_func free_method);

/**
 * Free an array, and optionally its members
 * @param array valid BuxtonArray reference
 * @param free_func Function to call to free all members, or NULL to leave allocated
 */
void buxton_array_free(BuxtonArray **array,
		       buxton_free_func free_method);

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
