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

#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

#include "protocol.h"
#include "log.h"

void bt_daemon_handle_message(BuxtonDaemon *self, client_list_item *client, int size)
{
	BuxtonControlMessage msg;
	BuxtonStatus response;
	BuxtonData *list = NULL;
	BuxtonData *data = NULL;
	int p_count, i;
	int response_len;
	BuxtonData response_data;
	uint8_t *response_store;


	if (!buxton_deserialize_message((uint8_t*)client->data, &msg, size, &list)) {
		buxton_debug("Failed to deserialize message\n");
		goto end;
	}

	/* Check valid range */
	if (msg < BUXTON_CONTROL_SET || msg > BUXTON_CONTROL_MAX)
		goto end;

	/* use internal function from bt-daemon */
	switch (msg) {
		case BUXTON_CONTROL_SET:
			data = self->set_value(client, list, &response);
			break;
		case BUXTON_CONTROL_GET:
			data = self->get_value(client, list, &response);
			break;
		default:
			goto end;
	}
	/* Set a response code */
	response_data.type = INT;
	response_data.store.d_int = response;

	/* Prepare a data response */
	if (data) {
		/* Returning data from inside buxton */
		if (!buxton_serialize_message(&response_store, BUXTON_CONTROL_STATUS, 2, &response_data, data)) {
			buxton_log("Failed to serialize 2 parameter response message\n");
			goto end;
		}
	} else {
		if (!buxton_serialize_message(&response_store, BUXTON_CONTROL_STATUS, 1, &response_data)) {
			buxton_log("Failed to serialize single parameter response message\n");
			goto end;
		}
	}

	/* Now write the response */
	response_len = malloc_usable_size(response_store);
	write(client->fd, response_store, response_len);

end:
	if (response_store)
		free(response_store);
	if (data && data->type == STRING)
		free(data->store.d_string);
	if (list) {
		for (i=0; i<p_count; i++) {
			if (list[i].type == STRING)
				free(list[i].store.d_string);
		}
		free(list);
	}
}

int buxton_wire_get_response(BuxtonClient *client, BuxtonControlMessage *msg,
			      BuxtonData **list)
{
	assert(client);
	assert(msg);
	assert(list);

	char response[256];
	int l;
	BuxtonData *r_list = NULL;
	BuxtonControlMessage r_msg;
	uint8_t *data = NULL;;
	int count = -1;

	while ((l = read(client->fd, &response, 256)) > 0) {
		if ((count = buxton_deserialize_message(response, &r_msg, l, &r_list)) < 0)
			goto end;
		if (r_msg != BUXTON_CONTROL_STATUS && r_list[0].type != INT) {
			buxton_log("Critical error: Invalid response\n");
			goto end;
		}
		break;
	}
	*msg = r_msg;
	*list = r_list;
end:
	return count;
}

bool buxton_wire_set_value(BuxtonClient *client, char *layer, char *key,
			   BuxtonData *value)
{
	assert(client);
	assert(layer);
	assert(key);
	assert(value);

	bool ret = false;
	int count;
	uint16_t *send = NULL;
	int send_len = 0;
	BuxtonControlMessage r_msg;
	BuxtonData *r_list = NULL;
	BuxtonData d_layer, d_key;

	d_layer.type = STRING;
	d_layer.store.d_string = layer;
	d_key.type = STRING;
	d_key.store.d_string = key;

	/* Attempt to serialize our send message */
	if (!buxton_serialize_message(&send, BUXTON_CONTROL_SET, 3,
		&d_layer, &d_key, value))
		goto end;
	/* Now write it off */
	send_len = malloc_usable_size(send);
	write(client->fd, send, send_len);

	/* Gain response */
	count = buxton_wire_get_response(client, &r_msg, &r_list);;
	if (count > 0 && r_list[0].store.d_int == BUXTON_STATUS_OK)
		ret = true;
end:
	if (send)
		free(send);
	if (r_list)
		free(r_list);

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
