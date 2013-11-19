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
#include "log.h"
#include "serialize.h"
#include "smack.h"
#include "util.h"


size_t buxton_serialize(BuxtonData *source, uint8_t **target)
{
	size_t length;
	size_t size;
	size_t offset = 0;
	uint8_t *data = NULL;
	size_t ret = 0;

	assert(source);
	assert(source->label.value);
	assert(target);

	/* DataType + length field */
	size = sizeof(BuxtonDataType) + (sizeof(size_t) * 2) + source->label.length;

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

	/* Write out the length of the label field */
	memcpy(data+offset, &(source->label.length), sizeof(size_t));
	offset += sizeof(size_t);

	/* Write out the length of the data field */
	memcpy(data+offset, &length, sizeof(size_t));
	offset += sizeof(size_t);

	/* Write out the label field */
	memcpy(data+offset, source->label.value, source->label.length);
	offset += source->label.length;

	/* Write the data itself */
	switch (source->type) {
		case STRING:
			memcpy(data+offset, source->store.d_string.value, length);
			break;
		case INT32:
			memcpy(data+offset, &(source->store.d_int32), sizeof(int32_t));
			break;
		case INT64:
			memcpy(data+offset, &(source->store.d_int64), sizeof(int64_t));
			break;
		case FLOAT:
			memcpy(data+offset, &(source->store.d_float), sizeof(float));
			break;
		case DOUBLE:
			memcpy(data+offset, &(source->store.d_double), sizeof(double));
			break;
		case BOOLEAN:
			memcpy(data+offset, &(source->store.d_boolean), sizeof(bool));
			break;
		default:
			goto end;
	}

	*target = data;
	ret = size;
end:
	if (ret < BXT_MINIMUM_SIZE)
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

	/* Retrieve the BuxtonDataType */
	type = *(BuxtonDataType*)source;
	offset += sizeof(BuxtonDataType);

	/* Retrieve the length of the label */
	target->label.length = *(size_t*)(source+offset);
	offset += sizeof(size_t);

	/* Retrieve the length of the value */
	length = *(size_t*)(source+offset);
	offset += sizeof(size_t);

	/* Retrieve the label */
	target->label.value = malloc(target->label.length);
	if (!target->label.value)
		goto end;
	memcpy(target->label.value, source+offset, target->label.length);
	offset += target->label.length;

	switch (type) {
		case STRING:
			/* User must free the string */
			target->store.d_string.value = malloc(length);
			if (!target->store.d_string.value)
				goto end;
			memcpy(target->store.d_string.value, source+offset, length);
			target->store.d_string.length = length;
			break;
		case INT32:
			target->store.d_int32 = *(int32_t*)(source+offset);
			break;
		case INT64:
			target->store.d_int64 = *(int64_t*)(source+offset);
			break;
		case FLOAT:
			target->store.d_float = *(float*)(source+offset);
			break;
		case DOUBLE:
			target->store.d_double = *(double*)(source+offset);
			break;
		case BOOLEAN:
			target->store.d_boolean = *(bool*)(source+offset);
			break;
		default:
			buxton_debug("Invalid BuxtonDataType: %lu\n", type);
			goto end;
	}

	/* Successful */
	target->type = type;
	ret = true;

end:
	if (!ret) {
		free(target->label.value);
		target->label.value = NULL;
	}
	return ret;
}

size_t buxton_serialize_message(uint8_t **dest, BuxtonControlMessage message,
			      size_t n_params, ...)
{
	va_list args;
	int i = 0;
	uint8_t *data = NULL;
	size_t ret = 0;
	size_t offset = 0;
	size_t size = 0;
	size_t curSize = 0;
	uint16_t control, msg;

	assert(dest);

	buxton_debug("Serializing message...\n");

	if (n_params == 0 || n_params > BUXTON_MESSAGE_MAX_PARAMS)
		return ret;

	if (message >= BUXTON_CONTROL_MAX || message < BUXTON_CONTROL_SET)
		return ret;

	data = malloc0(sizeof(uint32_t) + sizeof(size_t) + sizeof(size_t));
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
	memcpy(data+offset, &n_params, sizeof(size_t));
	offset += sizeof(size_t);

	size = offset;

	/* Deal with parameters */
	va_start(args, n_params);
	BuxtonData *param;
	size_t p_length = 0;
	for (i=0; i<n_params; i++) {
		/* Every parameter must be a BuxtonData. */
		param = va_arg(args, BuxtonData*);
		if (!param)
			goto fail;

		//FIXME - this assert should likely go away
		assert(param->label.value);

		if (param->type == STRING) {
			//FIXME - this assert should likely go away
			assert(param->store.d_string.value);
			p_length = param->store.d_string.length;
		} else {
			p_length = sizeof(param->store);
		}

		/* Need to allocate enough room to hold this data */
		size += sizeof(BuxtonDataType) + (sizeof(size_t) * 2)
			+ param->label.length
			+ p_length;

		if (curSize < size) {
			if (!(data = greedy_realloc((void**)&data, &curSize, size)))
				goto fail;
		}

		/* Copy data type */
		memcpy(data+offset, &(param->type), sizeof(BuxtonDataType));
		offset += sizeof(BuxtonDataType);

		/* Write out the length of the label field */
		memcpy(data+offset, &(param->label.length), sizeof(size_t));
		offset += sizeof(size_t);

		/* Write out the length of value */
		memcpy(data+offset, &p_length, sizeof(size_t));
		offset += sizeof(size_t);

		/* Write out the label field */
		memcpy(data+offset, param->label.value, param->label.length);
		offset += param->label.length;

		switch (param->type) {
			case STRING:
				memcpy(data+offset, param->store.d_string.value, p_length);
				break;
			case INT32:
				memcpy(data+offset, &(param->store.d_int32), sizeof(int32_t));
				break;
			case INT64:
				memcpy(data+offset, &(param->store.d_int64), sizeof(int64_t));
				break;
			case FLOAT:
				memcpy(data+offset, &(param->store.d_float), sizeof(float));
				break;
			case DOUBLE:
				memcpy(data+offset, &(param->store.d_double), sizeof(double));
				break;
			case BOOLEAN:
				memcpy(data+offset, &(param->store.d_boolean), sizeof(bool));
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

bool buxton_deserialize_message(uint8_t *data, BuxtonControlMessage *r_message,
			       size_t size, BuxtonArray **list)
{
	size_t offset = 0;
	bool ret = false;
	size_t min_length = BUXTON_MESSAGE_HEADER_LENGTH;
	uint16_t control, message;
	size_t n_params, c_param, c_length;
	BuxtonDataType c_type;
	BuxtonArray *k_list = NULL;
	BuxtonData *c_data = NULL;

	assert(data);
	assert(r_message);
	assert(list);

	buxton_debug("Deserializing message...\n");
	buxton_debug("size=%lu\n", size);

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
	n_params = *(size_t*)(data+offset);
	offset += sizeof(size_t);
	buxton_debug("total params: %d\n", n_params);

	if (n_params > BUXTON_MESSAGE_MAX_PARAMS)
		goto end;

	k_list = buxton_array_new();
	if (!k_list)
		goto end;

	for (c_param = 0; c_param < n_params; c_param++) {
		buxton_debug("param: %d\n", c_param + 1);
		buxton_debug("offset=%lu\n", offset);
		/* Don't read past the end of the buffer */
		if (offset + sizeof(BuxtonDataType) + (sizeof(size_t) * 2) >= size)
			goto end;

		/* Now unpack type */
		memcpy(&c_type, data+offset, sizeof(BuxtonDataType));
		offset += sizeof(BuxtonDataType);

		if (c_type >= BUXTON_TYPE_MAX || c_type < STRING)
			goto end;

		c_data = malloc0(sizeof(BuxtonData));
		if (!c_data)
			goto end;

		/* Retrieve the length of the label */
		c_data->label.length = *(size_t*)(data+offset);
		if (c_data->label.length < 2)
			goto end;
		offset += sizeof(size_t);
		buxton_debug("label length: %lu\n", c_data->label.length);

		if (c_data->label.length > SMACK_LABEL_LEN)
			goto end;

		/* Retrieve the length of the value */
		c_length = *(size_t*)(data+offset);
		if (c_length == 0)
			goto end;
		offset += sizeof(size_t);
		buxton_debug("value length: %lu\n", c_length);

		/* Don't try to read past the end of our buffer */
		if (offset + c_length + c_data->label.length > size)
			goto end;

		/* Retrieve the label */
		c_data->label.value = malloc(c_data->label.length);
		if (!c_data->label.value)
			goto end;
		memcpy(c_data->label.value, data+offset, c_data->label.length);
		offset += c_data->label.length;

		switch (c_type) {
			case STRING:
				c_data->store.d_string.value = malloc(c_length);
				if (!c_data->store.d_string.value)
					goto end;
				memcpy(c_data->store.d_string.value, data+offset, c_length);
				c_data->store.d_string.length = c_length;
				if (c_data->store.d_string.value[c_length-1] != 0x00) {
					buxton_debug("buxton_deserialize_message(): Garbage message\n");
					goto end;
				}
				break;
			case INT32:
				c_data->store.d_int32 = *(int32_t*)(data+offset);
				break;
			case INT64:
				c_data->store.d_int64 = *(int64_t*)(data+offset);
				break;
			case FLOAT:
				c_data->store.d_float = *(float*)(data+offset);
				break;
			case DOUBLE:
				c_data->store.d_double = *(double*)(data+offset);
				break;
			case BOOLEAN:
				c_data->store.d_boolean = *(bool*)(data+offset);
				break;
			default:
				goto end;
		}
		c_data->type = c_type;
		buxton_array_add(k_list, c_data);
		c_data = NULL;
		offset += c_length;
	}
	*r_message = message;
	*list = k_list;
	ret = true;
end:
	if (c_data)
		free(c_data->label.value);
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
