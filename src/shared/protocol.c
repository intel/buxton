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

int buxton_wire_get_response(BuxtonClient *client, BuxtonControlMessage *msg,
			      BuxtonData **list)
{
	assert(client);
	assert(msg);
	assert(list);

	uint8_t *response;
	int l;
	BuxtonData *r_list = NULL;
	BuxtonControlMessage r_msg;
	int count = -1;
	size_t offset = 0;
	size_t size = BUXTON_MESSAGE_HEADER_LENGTH;

	response = malloc(BUXTON_MESSAGE_HEADER_LENGTH);

	while ((l = read(client->fd, response + offset, size - offset)) > 0) {
		offset += l;
		buxton_log("offset=%lu : bmhl=%d\n", offset, BUXTON_MESSAGE_HEADER_LENGTH);
		if (offset < BUXTON_MESSAGE_HEADER_LENGTH)
			continue;
		if (size == BUXTON_MESSAGE_HEADER_LENGTH) {
			size = buxton_get_message_size(response, offset);
			buxton_log("size=%lu\n", size);
			if (size == 0 || size > BUXTON_MESSAGE_MAX_LENGTH)
				goto end;
		}
		if (size != BUXTON_MESSAGE_HEADER_LENGTH) {
			response = realloc(response, size);
			if (!response)
				goto end;
		}
		if (size != offset)
			continue;

		count = buxton_deserialize_message(response, &r_msg, l, &r_list);
		if (count < 0)
			goto end;
		if (r_msg != BUXTON_CONTROL_STATUS || r_list[0].type != INT) {
			buxton_log("Critical error: Invalid response\n");
			goto end;
		}
		break;
	}
	*msg = r_msg;
	*list = r_list;
end:
	if (response)
		free(response);

	return count;
}

bool buxton_wire_set_value(BuxtonClient *client, const char *layer_name, const char *key,
			   BuxtonData *value)
{
	assert(client);
	assert(layer_name);
	assert(key);
	assert(value);

	bool ret = false;
	int count;
	uint8_t *send = NULL;
	int send_len = 0;
	BuxtonControlMessage r_msg;
	BuxtonData *r_list = NULL;
	BuxtonData d_layer, d_key;

	d_layer.type = STRING;
	d_layer.store.d_string = (char *)layer_name;
	d_key.type = STRING;
	d_key.store.d_string = (char *)key;

	/* Attempt to serialize our send message */
	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_SET, 3,
					    &d_layer, &d_key, value);
	if (send_len == 0)
		goto end;
	/* Now write it off */
	write(client->fd, send, send_len);

	/* Gain response */
	count = buxton_wire_get_response(client, &r_msg, &r_list);
	if (count > 0 && r_list[0].store.d_int == BUXTON_STATUS_OK)
		ret = true;
end:
	if (send)
		free(send);
	if (r_list)
		free(r_list);

	return ret;
}

bool buxton_wire_get_value(BuxtonClient *client, const char *layer_name, const char *key,
			   BuxtonData *value)
{
	assert(client);
	assert(key);
	assert(value);

	bool ret = false;
	int count, i;
	uint8_t *send = NULL;
	int send_len = 0;
	BuxtonControlMessage r_msg;
	BuxtonData *r_list = NULL;
	BuxtonData d_layer, d_key;

	/* Optional layer */
	if (layer_name != NULL) {
		d_layer.type = STRING;
		d_layer.store.d_string = (char *)layer_name;
	}
	d_key.type = STRING;
	d_key.store.d_string = (char *)key;

	/* Attempt to serialize our send message */
	if (layer_name != NULL)
		send_len = buxton_serialize_message(&send, BUXTON_CONTROL_GET, 2,
						    &d_layer, &d_key);
	else
		send_len = buxton_serialize_message(&send, BUXTON_CONTROL_GET, 1, &d_key);

	if (send_len == 0)
		goto end;
	/* Now write it off */
	write(client->fd, send, send_len);

	/* Gain response */
	count = buxton_wire_get_response(client, &r_msg, &r_list);
	if (count == 2 && r_list[0].store.d_int == BUXTON_STATUS_OK)
		ret = true;
	else
		goto end;

	/* Now copy the data over for the user */
	buxton_data_copy(&r_list[1], value);

end:
	if (send)
		free(send);
	if (r_list) {
		for (i=0; i < count; i++) {
			if (r_list[i].type == STRING)
				free(r_list[i].store.d_string);
		}
		free(r_list);
	}
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
