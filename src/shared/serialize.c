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

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "serialize.h"
#include "../include/bt-daemon.h"

bool buxton_serialize(BuxtonData *source, uint8_t **target)
{
	unsigned int length;
	unsigned int size;
	unsigned int offset = 0;
	uint8_t *data = NULL;
	bool ret = false;

	if (!source)
		goto end;

	/* DataType + length field */
	size = sizeof(BuxtonDataType) + sizeof(unsigned int);

	/* Total size will be different for string data */
	switch (source->type) {
		case STRING:
			length = strlen(source->store.d_string) + 1;
			break;
		default:
			length = sizeof(source->store);
			break;
	}

	size += length;

	/* Allocate memory big enough to hold all information */
	data = malloc(size);
	if (!data)
		goto end;

	/* Write the entire BuxtonDataType to the first block */
	memcpy(data, &(source->type), sizeof(BuxtonDataType));
	offset += sizeof(BuxtonDataType);

	/* Write out the length of the data field */
	memcpy(data+offset, &length, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	/* Write the data itself */
	switch (source->type) {
		case STRING:
			memcpy(data+offset, source->store.d_string, length);
			break;
		case BOOLEAN:
			memcpy(data+offset, &(source->store.d_boolean), length);
			break;
		case FLOAT:
			memcpy(data+offset, &(source->store.d_float), length);
			break;
		case INT:
			memcpy(data+offset, &(source->store.d_int), length);
			break;
		case DOUBLE:
			memcpy(data+offset, &(source->store.d_double), length);
			break;
		case LONG:
			memcpy(data+offset, &(source->store.d_long), length);
		default:
			goto end;
	}

	*target = data;
	ret = true;
end:
	if (!ret && data)
		free(data);

	return ret;
}

bool buxton_deserialize(uint8_t *source, BuxtonData *target)
{
	void *copy_type = NULL;
	void *copy_length = NULL;
	void *copy_data = NULL;
	unsigned int offset = 0;
	unsigned int length = 0;
	unsigned int len;
	BuxtonDataType type;
	bool ret = false;

	len = malloc_usable_size(source);

	if (len < BXT_MINIMUM_SIZE)
		return false;

	/* firstly, retrieve the BuxtonDataType */
	copy_type = malloc(sizeof(BuxtonDataType));
	if (!copy_type)
		goto end;
	memcpy(copy_type, source, sizeof(BuxtonDataType));
	type = *(BuxtonDataType*)copy_type;
	offset += sizeof(BuxtonDataType);

	/* Now retrieve the length */
	copy_length = malloc(sizeof(unsigned int));
	if (!copy_length)
		goto end;
	memcpy(copy_length, source+offset, sizeof(unsigned int));
	length = *(unsigned int*)copy_length;
	offset += sizeof(unsigned int);

	/* Retrieve the remainder of the data */
	copy_data = malloc(length);
	if (!copy_data)
		goto end;
	memcpy(copy_data, source+offset, length);

	switch (type) {
		case STRING:
			/* User must free the string */
			target->store.d_string = strdup((char*)copy_data);
			if (!target->store.d_string)
				goto end;
			break;
		case BOOLEAN:
			target->store.d_boolean = *(bool*)copy_data;
			break;
		case FLOAT:
			target->store.d_float = *(float*)copy_data;
			break;
		case INT:
			target->store.d_int = *(int*)copy_data;
			break;
		case DOUBLE:
			target->store.d_double = *(double*)copy_data;
			break;
		case LONG:
			target->store.d_long = *(long*)copy_data;
			break;
		default:
			goto end;
	}

	/* Successful */
	target->type = type;
	ret = true;

end:
	/* Cleanup */
	if (copy_type)
		free(copy_type);
	if (copy_length)
		free(copy_length);
	if (copy_data)
		free(copy_data);

	return ret;
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
