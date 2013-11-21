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

/**
 * \file buxton.h Buxton public header
 *
 * This is the public part of Buxton
 */

#pragma once

#include <stdbool.h>

/**
 * Valid Buxton types
 */
typedef enum {
	BUXTON_TYPES_MIN,
	BUXTON_STRING, /**<Represents type of a string value */
	BUXTON_INT32, /**<Represents type of an int32_t value */
	BUXTON_INT64, /**<Represents type of a int64_t value */
	BUXTON_FLOAT, /**<Represents type of a float value */
	BUXTON_DOUBLE, /**<Represents type of a double value */
	BUXTON_BOOLEAN, /**<Represents type of a boolean value */
	BUXTON_TYPES_MAX
} BuxtonType;

/**
 * Used for all set and get operations
 */
typedef struct BuxtonValue {

	/** The type of data stored */
	BuxtonType type;

	/* Pointer to the data */
	void* store;
} BuxtonValue;


/**
 * Get a value from within Buxton
 * Note that you must free the BuxtonValue with buxton_free()
 * @param layer The layer to retrieve data from
 * @param group Group that the key exists within
 * @param key The key you wish to retrieve
 * @returns BuxtonPointer pointer to returned data, or NULL
 */
BuxtonValue* buxton_get_value(char *layer,
			      char *group,
			      char *key);

/**
 * Set a value within Buxton
 * @param layer The layer to modify
 * @param group Group that the key exists within
 * @param key The key you wish to modify
 * @param data The data to be set
 * @returns bool True if the key was set or updated, otherwise false
 */
bool buxton_set_value(char *layer,
		      char *group,
		      char *key,
		      BuxtonValue *data);

/**
 * Unset a value from within Buxton
 * @param layer The layer to modify
 * @param group Group that the key exists within
 * @param key The key you wish to modify
 * @returns bool True if the value was unset, otherwise false
 */
bool buxton_unset_value(char *layer,
		        char *group,
		        char *key);

/**
 * Free a BuxtonValid when you are done with it
 * This function will ignore NULL pointers
 * @param p Valid BuxtonValue instance
 */
void buxton_free_value(BuxtonValue *p);

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
