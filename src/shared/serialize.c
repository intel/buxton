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

#include <errno.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "buxton.h"
#include "log.h"
#include "serialize.h"
#include "smack.h"
#include "util.h"


size_t buxton_serialize(BuxtonData *source, BuxtonString *label,
			uint8_t **target)
{
	size_t length;
	size_t size;
	size_t offset = 0;
	uint8_t *data = NULL;
	size_t ret = 0;

	assert(source);
	assert(target);

	/* DataType + length field */
	size = sizeof(BuxtonDataType) + (sizeof(uint32_t) * 2) + label->length;

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
	if (!data) {
		abort();
	}

	/* Write the entire BuxtonDataType to the first block */
	memcpy(data, &(source->type), sizeof(BuxtonDataType));
	offset += sizeof(BuxtonDataType);

	/* Write out the length of the label field */
	memcpy(data+offset, &(label->length), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	/* Write out the length of the data field */
	memcpy(data+offset, &length, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	/* Write out the label field */
	memcpy(data+offset, label->value, label->length);
	offset += label->length;

	/* Write the data itself */
	switch (source->type) {
	case STRING:
		memcpy(data+offset, source->store.d_string.value, length);
		break;
	case INT32:
		memcpy(data+offset, &(source->store.d_int32), sizeof(int32_t));
		break;
	case UINT32:
		memcpy(data+offset, &(source->store.d_uint32), sizeof(uint32_t));
		break;
	case INT64:
		memcpy(data+offset, &(source->store.d_int64), sizeof(int64_t));
		break;
	case UINT64:
		memcpy(data+offset, &(source->store.d_uint64), sizeof(uint64_t));
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
		abort();
	}

	ret = size;
	*target = data;

	assert(ret >= BXT_MINIMUM_SIZE);

	return ret;
}

void buxton_deserialize(uint8_t *source, BuxtonData *target,
			BuxtonString *label)
{
	size_t offset = 0;
	size_t length = 0;
	BuxtonDataType type;

	assert(source);
	assert(target);
	assert(label);

	/* Retrieve the BuxtonDataType */
	type = *(BuxtonDataType*)source;
	offset += sizeof(BuxtonDataType);

	/* Retrieve the length of the label */
	label->length = *(uint32_t*)(source+offset);
	offset += sizeof(uint32_t);

	/* Retrieve the length of the value */
	length = *(uint32_t*)(source+offset);
	offset += sizeof(uint32_t);

	/* Retrieve the label */
	label->value = malloc(label->length);
	if (label->length > 0 && !label->value) {
		abort();
	}
	memcpy(label->value, source+offset, label->length);
	offset += label->length;

	switch (type) {
	case STRING:
		/* User must free the string */
		target->store.d_string.value = malloc(length);
		if (!target->store.d_string.value) {
			abort();
		}
		memcpy(target->store.d_string.value, source+offset, length);
		target->store.d_string.length = (uint32_t)length;
		break;
	case INT32:
		target->store.d_int32 = *(int32_t*)(source+offset);
		break;
	case UINT32:
		target->store.d_uint32 = *(uint32_t*)(source+offset);
		break;
	case INT64:
		target->store.d_int64 = *(int64_t*)(source+offset);
		break;
	case UINT64:
		target->store.d_uint64 = *(uint64_t*)(source+offset);
		break;
	case FLOAT:
		target->store.d_float = *(float*)(source+offset);
		break;
	case DOUBLE:
		memcpy(&target->store.d_double, source + offset, sizeof(double));
		break;
	case BOOLEAN:
		target->store.d_boolean = *(bool*)(source+offset);
		break;
	default:
		buxton_debug("Invalid BuxtonDataType: %lu\n", type);
		abort();
	}

	/* Successful */
	target->type = type;
}

size_t buxton_serialize_message(uint8_t **dest, BuxtonControlMessage message,
				uint32_t msgid, BuxtonArray *list)
{
	uint16_t i = 0;
	uint8_t *data = NULL;
	size_t ret = 0;
	size_t offset = 0;
	size_t size = 0;
	size_t curSize = 0;
	uint16_t control, msg;

	assert(dest);

	buxton_debug("Serializing message...\n");

	if (list->len > BUXTON_MESSAGE_MAX_PARAMS) {
		errno = EINVAL;
		return ret;
	}

	if (message >= BUXTON_CONTROL_MAX || message < BUXTON_CONTROL_SET) {
		errno = EINVAL;
		return ret;
	}

	/*
	 * initial size =
	 * control code + control message (uint16_t * 2) +
	 * message size (uint32_t) +
	 * message id (uint32_t) +
	 * param count (uint32_t)
	 */
	data = malloc0(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
		       sizeof(uint32_t));
	if (!data) {
		errno = ENOMEM;
		goto end;
	}

	control = BUXTON_CONTROL_CODE;
	memcpy(data, &control, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	msg = (uint16_t)message;
	memcpy(data+offset, &msg, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	/* Save room for final size */
	offset += sizeof(uint32_t);

	memcpy(data+offset, &msgid, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	/* Now write the parameter count */
	memcpy(data+offset, &(list->len), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	size = offset;

	/* Deal with parameters */
	BuxtonData *param;
	size_t p_length = 0;
	for (i=0; i < list->len; i++) {
		param = buxton_array_get(list, i);
		if (!param) {
			errno = EINVAL;
			goto fail;
		}

		switch (param->type) {
		case STRING:
			p_length = param->store.d_string.length;
			break;
		case INT32:
			p_length = sizeof(int32_t);
			break;
		case UINT32:
			p_length = sizeof(uint32_t);
			break;
		case INT64:
			p_length = sizeof(int64_t);
			break;
		case UINT64:
			p_length = sizeof(uint64_t);
			break;
		case FLOAT:
			p_length = sizeof(float);
			break;
		case DOUBLE:
			p_length = sizeof(double);
			break;
		case BOOLEAN:
			p_length = sizeof(bool);
			break;
		default:
			errno = EINVAL;
			buxton_log("Invalid parameter type %lu\n", param->type);
			goto fail;
		};

		buxton_debug("offset: %lu\n", offset);
		buxton_debug("value length: %lu\n", p_length);

		/* Need to allocate enough room to hold this data */
		size += sizeof(uint16_t) + sizeof(uint32_t) + p_length;

		if (curSize < size) {
			if (!(data = greedy_realloc((void**)&data, &curSize, size))) {
				errno = ENOMEM;
				goto fail;
			}
			memzero(data+offset, size - offset);
		}

		/* Copy data type */
		memcpy(data+offset, &(param->type), sizeof(uint16_t));
		offset += sizeof(uint16_t);

		/* Write out the length of value */
		memcpy(data+offset, &p_length, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		switch (param->type) {
		case STRING:
			memcpy(data+offset, param->store.d_string.value, p_length);
			break;
		case INT32:
			memcpy(data+offset, &(param->store.d_int32), sizeof(int32_t));
			break;
		case UINT32:
			memcpy(data+offset, &(param->store.d_uint32), sizeof(uint32_t));
			break;
		case INT64:
			memcpy(data+offset, &(param->store.d_int64), sizeof(int64_t));
			break;
		case UINT64:
			memcpy(data+offset, &(param->store.d_uint64), sizeof(uint64_t));
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
			/* already tested this above, can't get here
			 * normally */
			assert(0);
		};
		offset += p_length;
		p_length = 0;
	}

	memcpy(data+BUXTON_LENGTH_OFFSET, &offset, sizeof(uint32_t));

	ret = offset;
	*dest = data;

fail:
	/* Clean up */
	if (ret == 0) {
		free(data);
	}
end:
	buxton_debug("Serializing returned:%lu\n", ret);
	return ret;
}

ssize_t buxton_deserialize_message(uint8_t *data,
				  BuxtonControlMessage *r_message,
				  size_t size, uint32_t *r_msgid,
				  BuxtonData **list)
{
	size_t offset = 0;
	ssize_t ret = -1;
	size_t min_length = BUXTON_MESSAGE_HEADER_LENGTH;
	uint16_t control, message;
	size_t n_params, c_param, c_length;
	BuxtonDataType c_type = 0;
	BuxtonData *k_list = NULL;
	BuxtonData c_data;
	uint32_t msgid;

	assert(data);
	assert(r_message);
	assert(list);

	buxton_debug("Deserializing message...\n");
	buxton_debug("size=%lu\n", size);

	if (size < min_length) {
		errno = EINVAL;
		goto end;
	}

	/* Copy the control code */
	control = *(uint16_t*)data;
	offset += sizeof(uint16_t);

	/* Check this is a valid buxton message */
	if (control != BUXTON_CONTROL_CODE) {
		errno = EINVAL;
		goto end;
	}

	/* Obtain the control message */
	message = *(BuxtonControlMessage*)(data+offset);
	offset += sizeof(uint16_t);

	/* Ensure control message is in valid range */
	if (message <= BUXTON_CONTROL_MIN || message >= BUXTON_CONTROL_MAX) {
		errno = EINVAL;
		goto end;
	}

	/* Skip size since our caller got this already */
	offset += sizeof(uint32_t);

	/* Obtain the message id */
	msgid = *(uint32_t*)(data+offset);
	offset += sizeof(uint32_t);

	/* Obtain number of parameters */
	n_params = *(uint32_t*)(data+offset);
	offset += sizeof(uint32_t);
	buxton_debug("total params: %d\n", n_params);

	if (n_params > BUXTON_MESSAGE_MAX_PARAMS) {
		errno = EINVAL;
		goto end;
	}

	k_list = malloc0(sizeof(BuxtonData)*n_params);
	if (n_params && !k_list) {
		errno = ENOMEM;
		goto end;
	}

	memzero(&c_data, sizeof(BuxtonData));

	for (c_param = 0; c_param < n_params; c_param++) {
		buxton_debug("param: %d\n", c_param + 1);
		buxton_debug("offset=%lu\n", offset);
		/* Don't read past the end of the buffer */
		if (offset + sizeof(uint16_t) + sizeof(uint32_t) > size) {
			errno = EINVAL;
			goto end;
		}

		/* Now unpack type */
		memcpy(&c_type, data+offset, sizeof(uint16_t));
		offset += sizeof(uint16_t);

		if (c_type >= BUXTON_TYPE_MAX || c_type <= BUXTON_TYPE_MIN) {
			errno = EINVAL;
			goto end;
		}

		/* Retrieve the length of the value */
		c_length = *(uint32_t*)(data+offset);
		if (c_length == 0 && c_type != STRING) {
			errno = EINVAL;
			goto end;
		}
		offset += sizeof(uint32_t);
		buxton_debug("value length: %lu\n", c_length);

		/* Don't try to read past the end of our buffer */
		if (offset + c_length > size) {
			errno = EINVAL;
			goto end;
		}

		switch (c_type) {
		case STRING:
			if (c_length) {
				c_data.store.d_string.value = malloc(c_length);
				if (!c_data.store.d_string.value) {
					errno = ENOMEM;
					goto end;
				}
				memcpy(c_data.store.d_string.value, data+offset, c_length);
				c_data.store.d_string.length = (uint32_t)c_length;
				if (c_data.store.d_string.value[c_length-1] != 0x00) {
					errno = EINVAL;
					buxton_debug("buxton_deserialize_message(): Garbage message\n");
					free(c_data.store.d_string.value);
					goto end;
				}
			} else {
				c_data.store.d_string.value = NULL;
				c_data.store.d_string.length = 0;
			}
			break;
		case INT32:
			c_data.store.d_int32 = *(int32_t*)(data+offset);
			break;
		case UINT32:
			c_data.store.d_uint32 = *(uint32_t*)(data+offset);
			break;
		case INT64:
			c_data.store.d_int64 = *(int64_t*)(data+offset);
			break;
		case UINT64:
			c_data.store.d_uint64 = *(uint64_t*)(data+offset);
			break;
		case FLOAT:
			c_data.store.d_float = *(float*)(data+offset);
			break;
		case DOUBLE:
			memcpy(&c_data.store.d_double, data + offset, sizeof(double));
			break;
		case BOOLEAN:
			c_data.store.d_boolean = *(bool*)(data+offset);
			break;
		default:
			errno = EINVAL;
			goto end;
		}
		c_data.type = c_type;
		k_list[c_param] = c_data;
		memzero(&c_data, sizeof(BuxtonData));
		offset += c_length;
	}
	*r_message = message;
	*r_msgid = msgid;
	if (n_params == 0) {
		*list = NULL;
		free(k_list);
		k_list = NULL;
	} else {
		*list = k_list;
	}
	ret = (ssize_t)n_params;
end:
	if (ret <= 0) {
		free(k_list);
	}

	buxton_debug("Deserializing returned:%i\n", ret);
	return ret;
}

size_t buxton_get_message_size(uint8_t *data, size_t size)
{
	size_t r_size;

	assert(data);

	if (size < BUXTON_MESSAGE_HEADER_LENGTH) {
		return 0;
	}

	r_size = *(uint32_t*)(data + BUXTON_LENGTH_OFFSET);

	if (r_size < BUXTON_MESSAGE_HEADER_LENGTH) {
		return 0;
	}

	return r_size;
}

void include_serialize(void)
{
	;
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
