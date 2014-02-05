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

#include "buxton.h"
#include "buxtonstring.h"

/**
 * Stores values in Buxton, may have only one value
 */
typedef union BuxtonDataStore {
	BuxtonString d_string; /**<Stores a string value */
	int32_t d_int32; /**<Stores an int32_t value */
	uint32_t d_uint32; /**<Stores an uint32_t value */
	int64_t d_int64; /**<Stores a int64_t value */
	uint64_t d_uint64; /**<Stores a uint64_t value */
	float d_float; /**<Stores a float value */
	double d_double; /**<Stores a double value */
	bool d_boolean; /**<Stores a boolean value */
} BuxtonDataStore;

/**
 * Represents data in Buxton
 *
 * In Buxton we operate on all data using BuxtonData, for both set and
 * get operations. The type must be set to the type of value being set
 * in the BuxtonDataStore
 */
typedef struct BuxtonData {
	BuxtonDataType type; /**<Type of data stored */
	BuxtonDataStore store; /**<Contains one value, correlating to
			       * type */
} BuxtonData;

static inline void buxton_string_to_data(BuxtonString *s, BuxtonData *d)
{
	d->type = STRING;
	d->store.d_string.value = s->value;
	d->store.d_string.length = s->length;
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
