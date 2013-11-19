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

#include <assert.h>
#include <stdlib.h>

#include "log.h"
#include "protocol.h"
#include "util.h"

bool buxton_wire_get_response(BuxtonClient *self, BuxtonControlMessage *msg,
			      BuxtonArray **list)
{
	assert(self);
	assert(msg);
	assert(list);

	ssize_t l;
	_cleanup_free_ uint8_t *response = NULL;
	BuxtonArray *r_list = NULL;
	BuxtonControlMessage r_msg = BUXTON_CONTROL_MIN;
	size_t offset = 0;
	size_t size = BUXTON_MESSAGE_HEADER_LENGTH;

	response = malloc0(BUXTON_MESSAGE_HEADER_LENGTH);
	if (!response)
		return false;

	do {
		l = read(self->fd, response + offset, size - offset);
		if (l <= 0)
			return false;
		offset += (size_t)l;
		if (offset < BUXTON_MESSAGE_HEADER_LENGTH)
			continue;
		if (size == BUXTON_MESSAGE_HEADER_LENGTH) {
			size = buxton_get_message_size(response, offset);
			if (size == 0 || size > BUXTON_MESSAGE_MAX_LENGTH)
				return false;
		}
		if (size != BUXTON_MESSAGE_HEADER_LENGTH) {
			response = realloc(response, size);
			if (!response)
				return false;
		}
		if (size != offset)
			continue;

		if (!buxton_deserialize_message(response, &r_msg, size, &r_list))
			return false;
		if (r_msg != BUXTON_CONTROL_STATUS || BD(r_list, 0)->type != INT32) {
			buxton_log("Critical error: Invalid response\n");
			return false;
		}
		break;
	} while (true);
	assert(r_msg > BUXTON_CONTROL_MIN);
	assert(r_msg < BUXTON_CONTROL_MAX);
	*msg = r_msg;
	*list = r_list;

	return true;
}

bool buxton_wire_set_value(BuxtonClient *self, BuxtonString *layer_name, BuxtonString *key,
			   BuxtonData *value)
{
	assert(self);
	assert(layer_name);
	assert(key);
	assert(value);
	assert(value->label.value);

	_cleanup_free_ uint8_t *send = NULL;
	size_t send_len = 0;
	BuxtonControlMessage r_msg;
	_cleanup_free_ BuxtonArray *r_list = NULL;
	BuxtonData d_layer;
	BuxtonData d_key;

	buxton_string_to_data(layer_name, &d_layer);
	buxton_string_to_data(key, &d_key);
	d_layer.label = buxton_string_pack("dummy");
	d_key.label = buxton_string_pack("dummy");

	/* Attempt to serialize our send message */
	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_SET, 3,
					    &d_layer, &d_key, value);
	if (send_len == 0)
		return false;

	/* Now write it off */
	write(self->fd, send, send_len);

	/* Gain response */
	if (!buxton_wire_get_response(self, &r_msg, &r_list))
		return false;
	if (r_list->len > 0 && BD(r_list, 0)->store.d_int32 == BUXTON_STATUS_OK)
		return true;

	return false;
}

bool buxton_wire_get_value(BuxtonClient *self, BuxtonString *layer_name, BuxtonString *key,
			   BuxtonData *value)
{
	assert(self);
	assert(key);
	assert(value);

	bool ret = false;
	size_t send_len = 0;
	int i;
	_cleanup_free_ uint8_t *send = NULL;
	BuxtonControlMessage r_msg;
	BuxtonArray *r_list = NULL;
	BuxtonData d_layer;
	BuxtonData d_key;

	buxton_string_to_data(key, &d_key);
	d_key.label = buxton_string_pack("dummy");

	/* Attempt to serialize our send message */
	if (layer_name != NULL) {
		buxton_string_to_data(layer_name, &d_layer);
		d_layer.label = buxton_string_pack("dummy");
		send_len = buxton_serialize_message(&send, BUXTON_CONTROL_GET, 2,
						    &d_layer, &d_key);
	} else {
		send_len = buxton_serialize_message(&send, BUXTON_CONTROL_GET, 1,
						    &d_key);
	}

	if (send_len == 0)
		goto end;
	/* Now write it off */
	write(self->fd, send, send_len);

	/* Gain response */
	if (!buxton_wire_get_response(self, &r_msg, &r_list))
		goto end;
	if (r_list->len == 2 && BD(r_list, 0)->store.d_int32 == BUXTON_STATUS_OK)
		ret = true;
	else
		goto end;

	/* Now copy the data over for the user */
	buxton_data_copy(BD(r_list, 1), value);

end:
	/* Todo: Use utility cleanup */
	if (r_list) {
		for (i=0; i < r_list->len; i++) {
			free(BD(r_list, i)->label.value);
			if (BD(r_list, i)->type == STRING)
				free(BD(r_list, i)->store.d_string.value);
		}
		buxton_array_free(&r_list, NULL);
	}
	return ret;
}

bool buxton_wire_register_notification(BuxtonClient *self, BuxtonString *key)
{
	assert(self);
	assert(key);

	bool ret = false;
	uint8_t *send = NULL;
	size_t send_len = 0;
	BuxtonControlMessage r_msg;
	BuxtonArray *r_list = NULL;
	BuxtonData d_key;

	buxton_string_to_data(key, &d_key);
	d_key.label = buxton_string_pack("dummy");

	/* Attempt to serialize our send message */
	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_NOTIFY, 1,
					    &d_key);
	if (send_len == 0)
		goto end;
	/* Now write it off */
	write(self->fd, send, send_len);

	/* Gain response */
	if (!buxton_wire_get_response(self, &r_msg, &r_list))
		goto end;
	if (r_list->len> 0 && BD(r_list, 0)->store.d_int32 == BUXTON_STATUS_OK)
		ret = true;
end:
	free(send);
	buxton_array_free(&r_list, NULL);

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
