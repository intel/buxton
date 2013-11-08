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
	memcpy(data+offset, &length, sizeof(size_t));
	offset += sizeof(size_t);

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
	void *copy_data = NULL;
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
	if (copy_data)
		free(copy_data);

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

	/* Empty message not permitted */
	if (n_params <= 0)
		return ret;

	if (message >= BUXTON_CONTROL_MAX || message < BUXTON_CONTROL_SET)
		return ret;

	data = malloc(sizeof(uint32_t) + sizeof(size_t) + sizeof(unsigned int));
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
		if (param->type == STRING)
			p_length = strlen(param->store.d_string)+1;
		else
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
				memcpy(data+offset, param->store.d_string, p_length);
				break;
			case BOOLEAN:
				memcpy(data+offset, &(param->store.d_boolean), p_length);
				break;
			case FLOAT:
				memcpy(data+offset, &(param->store.d_float), p_length);
				break;
			case INT:
				memcpy(data+offset, &(param->store.d_int), p_length);
				break;
			case DOUBLE:
				memcpy(data+offset, &(param->store.d_double), p_length);
				break;
			case LONG:
				memcpy(data+offset, &(param->store.d_long), p_length);
				break;
			default:
				goto fail;
		}
		offset += p_length;
		p_length = 0;
	}

	memcpy(data+BUXTON_LENGTH_OFFSET, &offset, sizeof(size_t));

	ret = offset;
	*dest = data;
fail:
	/* Clean up */
	if (ret == 0 && data)
		free(data);
	va_end(args);
end:
	return ret;
}

int buxton_deserialize_message(uint8_t *data, BuxtonControlMessage *r_message,
			       int size, BuxtonData **list)
{
	int offset = 0;
	int ret = -1;
	int min_length = BUXTON_MESSAGE_HEADER_LENGTH;
	void *copy_control = NULL;
	void *copy_message = NULL;
	void *copy_params = NULL;
	uint16_t control, message;
	unsigned int n_params, c_param;
	/* For each parameter */
	size_t p_size = 0;
	void *p_content = NULL;
	BuxtonDataType c_type;
	unsigned int c_length;
	BuxtonData *k_list = NULL;
	BuxtonData *c_data = NULL;

	assert(data);
	assert(r_message);
	assert(list);

	if (size < min_length)
		goto end;

	/* Copy the control code */
	copy_control = malloc(sizeof(uint16_t));
	if (!copy_control)
		goto end;
	memcpy(copy_control, data, sizeof(uint16_t));
	offset += sizeof(uint16_t);
	control = *(uint16_t*)copy_control;

	/* Check this is a valid buxton message */
	if (control != BUXTON_CONTROL_CODE)
		goto end;

	/* Obtain the control message */
	copy_message = malloc(sizeof(uint16_t));
	if (!copy_message)
		goto end;
	memcpy(copy_message, data+offset, sizeof(uint16_t));
	offset += sizeof(uint16_t);
	message = *(BuxtonControlMessage*)copy_message;
	/* Ensure control message is in valid range */
	if (message >= BUXTON_CONTROL_MAX)
		goto end;

	/* Skip size since our caller got this already */
	offset += sizeof(size_t);

	/* Obtain number of parameters */
	copy_params = malloc(sizeof(unsigned int));
	if (!copy_params)
		goto end;
	memcpy(copy_params, data+offset, sizeof(unsigned int));
	offset += sizeof(unsigned int);
	n_params = *(unsigned int*)copy_params;

	k_list = malloc(sizeof(BuxtonData)*n_params);

	for (c_param = 0; c_param < n_params; c_param++) {
		/* Now unpack type + length */
		memcpy(&c_type, data+offset, sizeof(BuxtonDataType));
		offset += sizeof(BuxtonDataType);

		if (c_type >= BUXTON_TYPE_MAX || c_type < STRING)
			goto end;

		/* Length */
		memcpy(&c_length, data+offset, sizeof(unsigned int));
		offset += sizeof(unsigned int);

		if (c_length <= 0)
			goto end;

		if (!p_content)
			p_content = malloc(c_length);
		else
			p_content = greedy_realloc((void**)&p_content, &p_size, c_length);

		/* If it still doesn't exist, bail */
		if (!p_content)
			goto end;
		memcpy(p_content, data+offset, c_length);

		if (!c_data)
			c_data = malloc(sizeof(BuxtonData));
		if (!c_data)
			goto end;

		switch (c_type) {
			case STRING:
				c_data->store.d_string = strndup((char*)p_content, c_length);
				if (!c_data->store.d_string)
					goto end;
				break;
			case BOOLEAN:
				c_data->store.d_boolean = *(bool*)p_content;
				break;
			case FLOAT:
				c_data->store.d_float = *(float*)p_content;
				break;
			case INT:
				c_data->store.d_int = *(int*)p_content;
				break;
			case DOUBLE:
				c_data->store.d_double = *(double*)p_content;
				break;
			case LONG:
				c_data->store.d_long = *(long*)p_content;
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

	if (copy_control)
		free(copy_control);
	if (copy_message)
		free(copy_message);
	if (copy_params)
		free(copy_params);
	if (p_content)
		free(p_content);
	if (c_data)
		free(c_data);

	return ret;
}

size_t buxton_get_message_size(uint8_t *data, int size)
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
