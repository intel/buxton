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

#include "protocol.h"
#include "serialize.h"
#include "log.h"

void bt_daemon_handle_message(BuxtonDaemon *self, client_list_item *client)
{
	BuxtonControlMessage msg;
	BuxtonStatus response;
	BuxtonData *list = NULL;
	BuxtonData *data = NULL;
	int p_count, i;
	int response_len;
	BuxtonData response_data;
	uint8_t *response_store;


	if (!buxton_deserialize_message((uint8_t*)client->data, &msg, &list)) {
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
