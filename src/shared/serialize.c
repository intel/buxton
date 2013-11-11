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
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bt-daemon.h"
#include "serialize.h"
#include "util.h"
#include "log.h"

size_t buxton_serialize(BuxtonData *source, uint8_t **target)
{
	size_t length;
	size_t size;
	size_t offset = 0;
	uint8_t *data = NULL;
	size_t ret = 0;

	assert(source);
	assert(target);

	/* DataType + length field */
	size = sizeof(BuxtonDataType) + sizeof(size_t);

	/* Total size will be different for string data */
	switch (source->type) {
		case STRING:
			length = source->store.d_string.length;
			break;
		default:
			length = sizeof(source->store);
			break;
	}

	size += length;

	/* Allocate memory big enough to hold all information */
	data = malloc0(size);
	if (!data)
		goto end;

	/* Write the entire BuxtonDataType to the first block */
	memcpy(data, &(source->type), sizeof(BuxtonDataType));
	offset += sizeof(BuxtonDataType);

	/* Write out the length of the data field */
	memcpy(data+offset, &length, sizeof(size_t));
	offset += sizeof(size_t);

	/* Write the data itself */
	switch (source->type) {
		case STRING:
			memcpy(data+offset, source->store.d_string.value, length);
			break;
		case BOOLEAN:
			memcpy(data+offset, &(source->store.d_boolean), sizeof(bool));
			break;
		case FLOAT:
			memcpy(data+offset, &(source->store.d_float), sizeof(float));
			break;
		case INT:
			memcpy(data+offset, &(source->store.d_int), sizeof(int32_t));
			break;
		case DOUBLE:
			memcpy(data+offset, &(source->store.d_double), sizeof(double));
			break;
		case LONG:
			memcpy(data+offset, &(source->store.d_long), sizeof(int64_t));
			break;
		default:
			goto end;
	}

	*target = data;
	ret = size;
end:
	if (ret < BXT_MINIMUM_SIZE && data)
		free(data);

	return ret;
}

bool buxton_deserialize(uint8_t *source, BuxtonData *target)
{
	size_t offset = 0;
	size_t length = 0;
	BuxtonDataType type;
	bool ret = false;

	assert(source);
	assert(target);

	/* firstly, retrieve the BuxtonDataType */
	memcpy(&type, source, sizeof(BuxtonDataType));
	offset += sizeof(BuxtonDataType);

	/* Now retrieve the length */
	memcpy(&length, source+offset, sizeof(size_t));
	offset += sizeof(size_t);

	switch (type) {
		case STRING:
			/* User must free the string */
			target->store.d_string.value = malloc(length);
			if (!target->store.d_string.value)
				goto end;
			memcpy(target->store.d_string.value, source+offset, length);
			target->store.d_string.length = length;
			break;
		case BOOLEAN:
			target->store.d_boolean = *(bool*)(source+offset);
			break;
		case FLOAT:
			target->store.d_float = *(float*)(source+offset);
			break;
		case INT:
			target->store.d_int = *(int32_t*)(source+offset);
			break;
		case DOUBLE:
			target->store.d_double = *(double*)(source+offset);
			break;
		case LONG:
			target->store.d_long = *(int64_t*)(source+offset);
			break;
		default:
			buxton_debug("Invalid BuxtonDataType: %lu\n", type);
			goto end;
	}

	/* Successful */
	target->type = type;
	ret = true;

end:
	return ret;
}

size_t buxton_serialize_message(uint8_t **dest, BuxtonControlMessage message,
			      unsigned int n_params, ...)
{
	va_list args;
	int i = 0;
	uint8_t *data;
	size_t ret = 0;
	size_t offset = 0;
	size_t size = 0;
	size_t curSize = 0;
	uint16_t control, msg;

	assert(dest);

	buxton_debug("Serializing message...\n");

	/* Empty message not permitted */
	if (n_params <= 0)
		return ret;

	if (message >= BUXTON_CONTROL_MAX || message < BUXTON_CONTROL_SET)
		return ret;

	data = malloc0(sizeof(uint32_t) + sizeof(size_t) + sizeof(unsigned int));
	if (!data)
		goto end;

	control = BUXTON_CONTROL_CODE;
	memcpy(data, &control, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	msg = (uint16_t)message;
	memcpy(data+offset, &msg, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	/* Save room for final size */
	offset += sizeof(size_t);

	/* Now write the parameter count */
	memcpy(data+offset, &n_params, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	size = offset;

	/* Deal with parameters */
	va_start(args, n_params);
	BuxtonData *param;
	unsigned int p_length = 0;
	for (i=0; i<n_params; i++) {
		/* Every parameter must be a BuxtonData. */
		param = va_arg(args, BuxtonData*);
		if (!param)
			goto fail;
		if (param->type == STRING) {
			//FIXME - this assert should likely go away
			assert(param->store.d_string.value);
			p_length = param->store.d_string.length;
		} else
			p_length = sizeof(param->store);

		/* Need to allocate enough room to hold this data */
		size += sizeof(BuxtonDataType) + sizeof(unsigned int);
		size += p_length;

		if (curSize < size) {
			if (!(data = greedy_realloc((void**)&data, &curSize, size)))
				goto fail;
		}

		/* Begin copying */
		memcpy(data+offset, &(param->type), sizeof(BuxtonDataType));
		offset += sizeof(BuxtonDataType);

		/* Length of following data */
		memcpy(data+offset, &p_length, sizeof(unsigned int));
		offset += sizeof(unsigned int);

		switch (param->type) {
			case STRING:
				memcpy(data+offset, param->store.d_string.value, p_length);
				break;
			case BOOLEAN:
				memcpy(data+offset, &(param->store.d_boolean), sizeof(bool));
				break;
			case FLOAT:
				memcpy(data+offset, &(param->store.d_float), sizeof(float));
				break;
			case INT:
				memcpy(data+offset, &(param->store.d_int), sizeof(int32_t));
				break;
			case DOUBLE:
				memcpy(data+offset, &(param->store.d_double), sizeof(double));
				break;
			case LONG:
				memcpy(data+offset, &(param->store.d_long), sizeof(int64_t));
				break;
			default:
				buxton_log("Invalid parameter type %lu\n", param->type);
				goto fail;
		};
		offset += p_length;
		p_length = 0;
	}

	memcpy(data+BUXTON_LENGTH_OFFSET, &offset, sizeof(size_t));

	ret = offset;
	*dest = data;
fail:
	/* Clean up */
	if (ret == 0)
		free(data);
	va_end(args);
end:
	buxton_debug("Serializing returned:%lu\n", ret);
	return ret;
}

int buxton_deserialize_message(uint8_t *data, BuxtonControlMessage *r_message,
			       int size, BuxtonData **list)
{
	int offset = 0;
	int ret = -1;
	int min_length = BUXTON_MESSAGE_HEADER_LENGTH;
	uint16_t control, message;
	unsigned int n_params, c_param;
	BuxtonDataType c_type;
	unsigned int c_length;
	BuxtonData *k_list = NULL;
	BuxtonData *c_data = NULL;

	assert(data);
	assert(r_message);
	assert(list);

	buxton_debug("Deserializing message...\n");

	if (size < min_length)
		goto end;

	/* Copy the control code */
	control = *(uint16_t*)data;
	offset += sizeof(uint16_t);

	/* Check this is a valid buxton message */
	if (control != BUXTON_CONTROL_CODE)
		goto end;

	/* Obtain the control message */
	message = *(BuxtonControlMessage*)(data+offset);
	offset += sizeof(uint16_t);

	/* Ensure control message is in valid range */
	if (message >= BUXTON_CONTROL_MAX)
		goto end;

	/* Skip size since our caller got this already */
	offset += sizeof(size_t);

	/* Obtain number of parameters */
	n_params = *(unsigned int*)(data+offset);
	offset += sizeof(unsigned int);

	k_list = malloc(sizeof(BuxtonData)*n_params);
	if (!k_list)
		goto end;

	for (c_param = 0; c_param < n_params; c_param++) {
		/* Now unpack type + length */
		memcpy(&c_type, data+offset, sizeof(BuxtonDataType));
		offset += sizeof(BuxtonDataType);

		if (c_type >= BUXTON_TYPE_MAX || c_type < STRING)
			goto end;

		/* Length */
		c_length = *(unsigned int*)(data+offset);
		offset += sizeof(unsigned int);

		if (c_length <= 0)
			goto end;

		if (!c_data)
			c_data = malloc0(sizeof(BuxtonData));
		if (!c_data)
			goto end;

		switch (c_type) {
			case STRING:
				c_data->store.d_string.value = malloc(c_length);
				if (!c_data->store.d_string.value)
					goto end;
				memcpy(c_data->store.d_string.value, data+offset, c_length);
				c_data->store.d_string.length = c_length;
				break;
			case BOOLEAN:
				c_data->store.d_boolean = *(bool*)(data+offset);
				break;
			case FLOAT:
				c_data->store.d_float = *(float*)(data+offset);
				break;
			case INT:
				c_data->store.d_int = *(int*)(data+offset);
				break;
			case DOUBLE:
				c_data->store.d_double = *(double*)(data+offset);
				break;
			case LONG:
				c_data->store.d_long = *(long*)(data+offset);
				break;
			default:
				goto end;
		}
		c_data->type = c_type;
		k_list[c_param] = *c_data;
		c_data = NULL;
		offset += c_length;
	}
	*r_message = message;
	*list = k_list;
	ret = n_params;
end:

	free(c_data);

	buxton_debug("Deserializing returned:%i\n", ret);
	return ret;
}

size_t buxton_get_message_size(uint8_t *data, size_t size)
{
	size_t r_size;

	assert(data);

	if (size < BUXTON_MESSAGE_HEADER_LENGTH)
		return 0;

	memcpy(&r_size, data+BUXTON_LENGTH_OFFSET, sizeof(size_t));

	if (r_size < BUXTON_MESSAGE_HEADER_LENGTH)
		return 0;

	return r_size;
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
