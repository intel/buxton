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
#include <stdint.h>
#include <string.h>
#include <attr/xattr.h>

#include "daemon.h"
#include "log.h"
#include "smack.h"
#include "util.h"

bool parse_list(BuxtonControlMessage msg, size_t count, BuxtonData *list,
		       BuxtonString **key, BuxtonString **layer, BuxtonData **value)
{
	switch (msg) {
	case BUXTON_CONTROL_SET:
		if (count != 3)
			return false;
		if (list[0].type != STRING || list[1].type != STRING)
			return false;
		*layer = &(list[0].store.d_string);
		*key = &(list[1].store.d_string);
		*value = &(list[2]);
		break;
	case BUXTON_CONTROL_GET:
		if (count == 2) {
			if(list[0].type != STRING || list[1].type != STRING)
				return false;
			*layer = &(list[0].store.d_string);
			*key = &(list[1].store.d_string);
		} else if (count == 1) {
			if (list[0].type != STRING)
				return false;
			*key = &(list[0].store.d_string);
		} else {
			return false;
		}
		break;
	case BUXTON_CONTROL_LIST:
		if (count != 1)
			return false;
		if (list[0].type != STRING)
			return false;
		*layer = &(list[0].store.d_string);
		break;
	case BUXTON_CONTROL_UNSET:
		if (count != 2)
			return false;
		if (list[0].type != STRING || list[1].type != STRING)
			return false;
		*layer = &(list[0].store.d_string);
		*key = &(list[1].store.d_string);
		break;
	case BUXTON_CONTROL_NOTIFY:
		if (count != 1)
			return false;
		if (list[0].type != STRING)
			return false;
		*key = &(list[0].store.d_string);
		break;
	case BUXTON_CONTROL_UNNOTIFY:
		if (count != 1)
			return false;
		if (list[0].type != STRING)
			return false;
		*key = &(list[0].store.d_string);
		break;
	default:
		return false;
		break;
	}

	return true;
}

bool bt_daemon_handle_message(BuxtonDaemon *self, client_list_item *client, size_t size)
{
	BuxtonControlMessage msg;
	BuxtonStatus response;
	BuxtonData *list = NULL;
	_cleanup_buxton_data_ BuxtonData *data = NULL;
	uint16_t i;
	size_t p_count;
	size_t response_len;
	BuxtonData response_data;
	BuxtonData *value = NULL;
	BuxtonString *key = NULL;
	BuxtonString *layer = NULL;
	BuxtonArray *out_list = NULL, *key_list = NULL;
	_cleanup_free_ uint8_t *response_store = NULL;
	uid_t uid;
	bool ret = false;

	assert(self);
	assert(client);

	uid = self->buxton.client.uid;
	p_count = buxton_deserialize_message((uint8_t*)client->data, &msg, size, &list);
	if (p_count == 0) {
		/* Todo: terminate the client due to invalid message */
		buxton_debug("Failed to deserialize message\n");
		goto end;
	}

	/* Check valid range */
	if (msg <= BUXTON_CONTROL_MIN || msg >= BUXTON_CONTROL_MAX)
		goto end;

	if (!parse_list(msg, p_count, list, &key, &layer, &value))
		goto end;

	/* use internal function from bt-daemon */
	switch (msg) {
	case BUXTON_CONTROL_SET:
		set_value(self, client, layer, key, value, &response);
		break;
	case BUXTON_CONTROL_GET:
		data = get_value(self, client, layer, key, &response);
		break;
	case BUXTON_CONTROL_UNSET:
		unset_value(self, client, layer, key, &response);
		break;
	case BUXTON_CONTROL_LIST:
		key_list = list_keys(self, client, layer, &response);
		break;
	case BUXTON_CONTROL_NOTIFY:
		register_notification(self, client, key, &response);
		break;
	case BUXTON_CONTROL_UNNOTIFY:
		unregister_notification(self, client, key, &response);
		break;
	default:
		goto end;
	}
	/* Set a response code */
	response_data.type = INT32;
	response_data.store.d_int32 = response;
	response_data.label = buxton_string_pack("dummy");
	out_list = buxton_array_new();
	if (!buxton_array_add(out_list, &response_data)) {
		buxton_log("Failed to prepare response\n");
		goto end;
	}

	switch (msg) {
	/* TODO: Use cascading switch */
	case BUXTON_CONTROL_SET:
		response_len = buxton_serialize_message(&response_store, BUXTON_CONTROL_STATUS, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize set response message\n");
			goto end;
		}
		break;
	case BUXTON_CONTROL_GET:
		if (!buxton_array_add(out_list, data)) {
			buxton_log("Failed to prepare GET array data\n");
			goto end;
		}
		response_len = buxton_serialize_message(&response_store, BUXTON_CONTROL_STATUS, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize get response message\n");
			goto end;
		}
		break;
	case BUXTON_CONTROL_UNSET:
		response_len = buxton_serialize_message(&response_store, BUXTON_CONTROL_STATUS, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize unset response message\n");
			goto end;
		}
		break;
	case BUXTON_CONTROL_LIST:
		if (key_list) {
			for (i = 0; i < key_list->len; i++) {
				buxton_array_add(out_list, buxton_array_get(key_list, i));
			}
			buxton_array_free(&key_list, NULL);
		}
		response_len = buxton_serialize_message(&response_store, BUXTON_CONTROL_STATUS, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize list response message\n");
			goto end;
		}
		break;
	case BUXTON_CONTROL_NOTIFY:
		response_len = buxton_serialize_message(&response_store, BUXTON_CONTROL_STATUS, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize notify response message\n");
			goto end;
		}
		break;
	case BUXTON_CONTROL_UNNOTIFY:
		response_len = buxton_serialize_message(&response_store, BUXTON_CONTROL_STATUS, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize unnotify response message\n");
			goto end;
		}
		break;
	default:
		goto end;
	}

	buxton_array_free(&out_list, NULL);

	/* Now write the response */
	write(client->fd, response_store, response_len);
	ret = true;

end:
	/* Restore our own UID */
	self->buxton.client.uid = uid;
	if (list) {
		for (i=0; i < p_count; i++) {
			free(list[i].label.value);
			if (list[i].type == STRING)
				free(list[i].store.d_string.value);
		}
		free(list);
	}
	return ret;
}

void bt_daemon_notify_clients(BuxtonDaemon *self, client_list_item *client, BuxtonString *key, BuxtonData *value)
{
	notification_list_item *list = NULL;
	notification_list_item *nitem;
	_cleanup_free_ uint8_t* response = NULL;
	size_t response_len;
	BuxtonData data;
	BuxtonArray *out_list = NULL;

	assert(self);
	assert(client);
	assert(key);
	assert(value);

	list = hashmap_get(self->notify_mapping, key->value);
	if (!list)
		return;

	data.type = STRING;
	data.label = buxton_string_pack("dummy");
	LIST_FOREACH(item, nitem, list) {
		int c = 1;
		free(response);
		response = NULL;

		if (nitem->old_data) {
			switch (value->type) {
			case STRING:
				c = memcmp((const void *)
					   (nitem->old_data->store.d_string.value),
					   (const void *)(value->store.d_string.value),
					   value->store.d_string.length);
				break;
			case INT32:
				c = memcmp((const void *)&(nitem->old_data->store.d_int32),
					   (const void *)&(value->store.d_int32),
					   sizeof(int32_t));
				break;
			case INT64:
				c = memcmp((const void *)&(nitem->old_data->store.d_int64),
					   (const void *)&(value->store.d_int64),
					   sizeof(int64_t));
				break;
			case FLOAT:
				c = memcmp((const void *)&(nitem->old_data->store.d_float),
					   (const void *)&(value->store.d_float),
					   sizeof(float));
				break;
			case DOUBLE:
				c = memcmp((const void *)&(nitem->old_data->store.d_double),
					   (const void *)&(value->store.d_double),
					   sizeof(double));
				break;
			case BOOLEAN:
				c = memcmp((const void *)&(nitem->old_data->store.d_boolean),
					   (const void *)&(value->store.d_boolean),
					   sizeof(bool));
				break;
			default:
				buxton_log("Internal state corruption: Notification data type invalid\n");
				return;
			}
		}

		if (!c) {
			continue;
		}
		if (nitem->old_data && (nitem->old_data->type == STRING))
			free(nitem->old_data->store.d_string.value);
		free(nitem->old_data);

		nitem->old_data = malloc0(sizeof(BuxtonData));
		if (nitem->old_data)
			buxton_data_copy(value, nitem->old_data);

		data.store.d_string.value = key->value;
		data.store.d_string.length = key->length;
		out_list = buxton_array_new();
		if (!buxton_array_add(out_list, &data) ||
			!buxton_array_add(out_list, value)) {
			buxton_log("Failed to prepare array data\n");
			return;
		}
		response_len = buxton_serialize_message(&response,
			BUXTON_CONTROL_CHANGED, out_list);
		buxton_array_free(&out_list, NULL);
		if (response_len == 0) {
			buxton_log("Failed to serialize notification\n");
			return;
		}
		buxton_debug("Notification to %d of key change (%s)\n", nitem->client->fd,
			     key->value);
		write(nitem->client->fd, response, response_len);
	}
}

void set_value(BuxtonDaemon *self, client_list_item *client, BuxtonString *layer,
		      BuxtonString *key, BuxtonData *value, BuxtonStatus *status)
{

	assert(self);
	assert(client);
	assert(layer);
	assert(key);
	assert(value);
	assert(status);

	*status = BUXTON_STATUS_FAILED;

	buxton_debug("Daemon setting [%s][%s][%s]\n",
		     layer->value,
		     key->value,
		     value->label.value);

	self->buxton.client.uid = client->cred.uid;

	if (!buxton_check_write_access(&self->buxton,
				       layer,
				       key,
				       value,
				       client->smack_label)) {
		*status = BUXTON_STATUS_FAILED;
		return;
	}

	/* Use internal library to set value */
	if (!buxton_direct_set_value(&self->buxton, layer, key, value)) {
		*status = BUXTON_STATUS_FAILED;
		return;
	}

	*status = BUXTON_STATUS_OK;
	buxton_debug("Daemon set value completed\n");
}


void unset_value(BuxtonDaemon *self, client_list_item *client,
		 BuxtonString *layer, BuxtonString *key,
		 BuxtonStatus *status)
{
	assert(self);
	assert(client);
	assert(key);
	assert(status);

	*status = BUXTON_STATUS_FAILED;

	buxton_debug("Daemon unsetting [%s][%s]\n",
		     layer->value,
		     key->value);

	if (!buxton_check_write_access(&self->buxton,
				       layer,
				       key,
				       NULL,
				       client->smack_label)) {
		*status = BUXTON_STATUS_FAILED;
		return;
	}
	/* Use internal library to unset value */
	self->buxton.client.uid = client->cred.uid;
	if (!buxton_direct_unset_value(&self->buxton, layer, key))
		return;

	buxton_debug("unset value returned successfully from db\n");

	*status = BUXTON_STATUS_OK;
	buxton_debug("Daemon unset value completed\n");
}

BuxtonData *get_value(BuxtonDaemon *self, client_list_item *client, BuxtonString *layer,
		      BuxtonString *key, BuxtonStatus *status)
{
	BuxtonData *data = NULL;

	assert(self);
	assert(client);
	assert(key);
	assert(status);

	*status = BUXTON_STATUS_FAILED;

	data = malloc0(sizeof(BuxtonData));
	if (!data)
		goto end;

	if (layer) {
		buxton_debug("Daemon getting [%s][%s]\n", layer->value,
			     key->value);
	} else {
		buxton_debug("Daemon getting [%s]\n", key->value);
	}
	self->buxton.client.uid = client->cred.uid;
	/* Attempt to retrieve key */
	if (layer) {
		/* Layer + key */
		if (!buxton_direct_get_value_for_layer(&self->buxton, layer, key, data))
			goto fail;
	} else {
		/* Key only */
		if (!buxton_direct_get_value(&self->buxton, key, data))
			goto fail;
	}
	buxton_debug("get value returned successfully from db\n");

	/* TODO: Need to move this check to
	 * buxton_direct_get_value_for_layer() so that we
	 * can do per-layer access checks.
	 */
	if (!buxton_check_read_access(&(self->buxton.client),
				      layer,
				      key,
				      data,
				      client->smack_label)) {
		goto fail;
	}
	buxton_debug("SMACK check succeeded for get_value\n");

	*status = BUXTON_STATUS_OK;
	goto end;
fail:
	buxton_debug("get value failed\n");
	free(data);
	data = NULL;
end:

	return data;
}

BuxtonArray *list_keys(BuxtonDaemon *self, client_list_item *client,
		       BuxtonString *layer, BuxtonStatus *status)
{
	BuxtonArray *ret_list = NULL;
	assert(self);
	assert(client);
	assert(layer);
	assert(status);

	*status = BUXTON_STATUS_FAILED;
	if (buxton_direct_list_keys(&self->buxton, layer, &ret_list))
		*status = BUXTON_STATUS_OK;
	return ret_list;
}

void register_notification(BuxtonDaemon *self, client_list_item *client, BuxtonString *key,
			   BuxtonStatus *status)
{
	notification_list_item *n_list = NULL;
	notification_list_item *nitem;
	BuxtonData *old_data = NULL;
	BuxtonStatus key_status;
	char *key_name;

	assert(self);
	assert(client);
	assert(key);
	assert(status);

	*status = BUXTON_STATUS_FAILED;

	nitem = malloc0(sizeof(notification_list_item));
	if (!nitem)
		return;
	LIST_INIT(notification_list_item, item, nitem);
	nitem->client = client;

	/* Store data now, cheap */
	old_data = get_value(self, client, NULL, key, &key_status);
	if (key_status != BUXTON_STATUS_OK) {
		free(nitem);
		return;
	}
	nitem->old_data = old_data;

	n_list = hashmap_get(self->notify_mapping, key->value);
	if (!n_list) {
		key_name = strdup(key->value);
		if (!key_name) {
			free(nitem);
			return;
		}

		if (hashmap_put(self->notify_mapping, key_name, nitem) < 0) {
			free(key_name);
			free(nitem);
			return;
		}
	} else {
		LIST_PREPEND(notification_list_item, item, n_list, nitem);
	}
	*status = BUXTON_STATUS_OK;
}

void unregister_notification(BuxtonDaemon *self, client_list_item *client,
			     BuxtonString *key, BuxtonStatus *status)
{
	notification_list_item *n_list = NULL;
	notification_list_item *nitem, *citem = NULL;
	int length = 0;

	assert(self);
	assert(client);
	assert(key);
	assert(status);

	*status = BUXTON_STATUS_FAILED;
	n_list = hashmap_get(self->notify_mapping, key->value);
	/* This key isn't actually registered for notifications */
	if (!n_list)
		return;

	LIST_FOREACH(item, nitem, n_list) {
		/* Find the list item for this client */
		if (nitem->client == client)
			citem = nitem;
		length++;
	};

	/* Client hasn't registered for notifications on this key */
	if (!citem)
		return;

	/* Remove client from notifications */
	LIST_REMOVE(notification_list_item, item, n_list, citem);
	free_buxton_data(&(citem->old_data));
	citem->client = NULL;
	free(citem);

	/* If we removed the last item, remove the mapping too */
	if (length == 1)
		hashmap_remove(self->notify_mapping, key->value);

	*status = BUXTON_STATUS_OK;
}

bool identify_client(client_list_item *cl)
{
	/* Identity handling */
	ssize_t nr;
	int data;
	struct msghdr msgh;
	struct iovec iov;
	__attribute__((unused)) struct ucred *ucredp;
	struct cmsghdr *cmhp;
	socklen_t len = sizeof(struct ucred);
	int on = 1;

	assert(cl);

	union {
		struct cmsghdr cmh;
		char control[CMSG_SPACE(sizeof(struct ucred))];
	} control_un;

	setsockopt(cl->fd, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));

	control_un.cmh.cmsg_len = CMSG_LEN(sizeof(struct ucred));
	control_un.cmh.cmsg_level = SOL_SOCKET;
	control_un.cmh.cmsg_type = SCM_CREDENTIALS;

	msgh.msg_control = control_un.control;
	msgh.msg_controllen = sizeof(control_un.control);

	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	iov.iov_base = &data;
	iov.iov_len = sizeof(int);

	msgh.msg_name = NULL;
	msgh.msg_namelen = 0;

	nr = recvmsg(cl->fd, &msgh, MSG_PEEK | MSG_DONTWAIT);
	if (nr == -1)
		return false;

	cmhp = CMSG_FIRSTHDR(&msgh);

	if (cmhp == NULL || cmhp->cmsg_len != CMSG_LEN(sizeof(struct ucred)))
		return false;

	if (cmhp->cmsg_level != SOL_SOCKET || cmhp->cmsg_type != SCM_CREDENTIALS)
		return false;

	ucredp = (struct ucred *) CMSG_DATA(cmhp);

	if (getsockopt(cl->fd, SOL_SOCKET, SO_PEERCRED, &cl->cred, &len) == -1)
		return false;

	return true;
}

void add_pollfd(BuxtonDaemon *self, int fd, short events, bool a)
{
	assert(self);
	assert(fd >= 0);

	if (!greedy_realloc((void **) &(self->pollfds), &(self->nfds_alloc),
	    (size_t)((self->nfds + 1) * (sizeof(struct pollfd))))) {
		buxton_log("realloc(): %m\n");
		exit(EXIT_FAILURE);
	}
	if (!greedy_realloc((void **) &(self->accepting), &(self->accepting_alloc),
	    (size_t)((self->nfds + 1) * (sizeof(self->accepting))))) {
		buxton_log("realloc(): %m\n");
		exit(EXIT_FAILURE);
	}
	self->pollfds[self->nfds].fd = fd;
	self->pollfds[self->nfds].events = events;
	self->pollfds[self->nfds].revents = 0;
	self->accepting[self->nfds] = a;
	self->nfds++;

	buxton_debug("Added fd %d to our poll list (accepting=%d)\n", fd, a);
}

void del_pollfd(BuxtonDaemon *self, nfds_t i)
{
	assert(self);
	assert(i < self->nfds);

	buxton_debug("Removing fd %d from our list\n", self->pollfds[i].fd);

	if (i != (self->nfds - 1)) {
		memmove(&(self->pollfds[i]),
			&(self->pollfds[i + 1]),
			/*
			 * nfds < max int because of kernel limit of
			 * fds. i + 1 < nfds because of if and assert
			 * so the casts below are always >= 0 and less
			 * than long unsigned max int so no loss of
			 * precision.
			 */
			(size_t)(self->nfds - i - 1) * sizeof(struct pollfd));
		memmove(&(self->accepting[i]),
			&(self->accepting[i + 1]),
			(size_t)(self->nfds - i - 1) * sizeof(self->accepting));
	}
	self->nfds--;
}

static void handle_smack_label(client_list_item *cl)
{
	socklen_t slabel_len = 1;
	char *buf = NULL;
	BuxtonString *slabel = NULL;
	int ret;

	ret = getsockopt(cl->fd, SOL_SOCKET, SO_PEERSEC, NULL, &slabel_len);
	/* libsmack ignores ERANGE here, so we ignore it too */
	if (ret < 0 && errno != ERANGE) {
		switch (errno) {
		case ENOPROTOOPT:
			/* If Smack is not enabled, do not set the client label */
			cl->smack_label = NULL;
			return;
		default:
			buxton_log("getsockopt(): %m\n");
			exit(EXIT_FAILURE);
		}
	}

	slabel = malloc0(sizeof(BuxtonString));
	if (!slabel) {
		buxton_log("malloc0() for BuxtonString: %m\n");
		exit(EXIT_FAILURE);
	}

	/* already checked slabel_len positive above */
	buf = malloc0((size_t)slabel_len + 1);
	if (!buf) {
		buxton_log("malloc0() for string value: %m\n");
		exit(EXIT_FAILURE);
	}

	ret = getsockopt(cl->fd, SOL_SOCKET, SO_PEERSEC, buf, &slabel_len);
	if (ret < 0) {
		buxton_log("getsockopt(): %m\n");
		exit(EXIT_FAILURE);
	}

	slabel->value = buf;
	slabel->length = (uint32_t)slabel_len;

	buxton_debug("getsockopt(): label=\"%s\"\n", slabel->value);

	cl->smack_label = slabel;
}

/**
 * Handle a client connection
 * @param cl The currently activate client
 * @param i The currently active file descriptor
 */
void handle_client(BuxtonDaemon *self, client_list_item *cl, nfds_t i)
{
	ssize_t l;

	assert(self);
	assert(cl);

	if (!cl->data) {
		cl->data = malloc0(BUXTON_MESSAGE_HEADER_LENGTH);
		cl->offset = 0;
		cl->size = BUXTON_MESSAGE_HEADER_LENGTH;
	}
	if (!cl->data)
		return;
	/* client closed the connection, or some error occurred? */
	if (recv(cl->fd, cl->data, cl->size, MSG_PEEK | MSG_DONTWAIT) <= 0) {
		terminate_client(self, cl, i);
		return;
	}

	/* need to authenticate the client? */
	if ((cl->cred.uid == 0) || (cl->cred.pid == 0)) {
		if (!identify_client(cl)) {
			terminate_client(self, cl, i);
			return;
		}

		handle_smack_label(cl);
	}

	buxton_debug("New packet from UID %ld, PID %ld\n", cl->cred.uid, cl->cred.pid);

	/* Hand off any read data */
	/*
	 * TODO: Need to handle partial messages, read total message
	 * size of the data, keep reading until we get that amount.
	 * Probably need a timer to stop waiting and just move to the
	 * next client at some point as well.
	 */
	do {
		l = read(self->pollfds[i].fd, (cl->data) + cl->offset, cl->size - cl->offset);
		if (l < 0) {
			terminate_client(self, cl, i);
			return;
		}
		else if (l == 0)
			break;
		cl->offset += (size_t)l;
		if (cl->offset < BUXTON_MESSAGE_HEADER_LENGTH) {
			continue;
		}
		if (cl->size == BUXTON_MESSAGE_HEADER_LENGTH) {
			cl->size = buxton_get_message_size(cl->data, cl->offset);
			if (cl->size == 0 || cl->size > BUXTON_MESSAGE_MAX_LENGTH)
				goto cleanup;
		}
		if (cl->size != BUXTON_MESSAGE_HEADER_LENGTH) {
			cl->data = realloc(cl->data, cl->size);
			if (!cl->data)
				goto cleanup;
		}
		if (cl->size != cl->offset)
			continue;
		if (!bt_daemon_handle_message(self, cl, cl->size)) {
			buxton_log("Communication failed with client %d\n", cl->fd);
			terminate_client(self, cl, i);
			return;
		}

		/* reset in case there are more messages */
		cl->data = realloc(cl->data, BUXTON_MESSAGE_HEADER_LENGTH);
		if (!cl->data)
			goto cleanup;
		cl->size = BUXTON_MESSAGE_HEADER_LENGTH;
		cl->offset = 0;
	} while (l > 0);

	/* Not done with this message so don't cleanup */
	if (l == 0 && cl->offset < cl->size)
		return;
cleanup:
	free(cl->data);
	cl->data = NULL;
	cl->size = BUXTON_MESSAGE_HEADER_LENGTH;
	cl->offset = 0;
}

void terminate_client(BuxtonDaemon *self, client_list_item *cl, nfds_t i)
{
	del_pollfd(self, i);
	close(cl->fd);
	if (cl->smack_label)
		free(cl->smack_label->value);
	free(cl->smack_label);
	free(cl->data);
	buxton_debug("Closed connection from fd %d\n", cl->fd);
	LIST_REMOVE(client_list_item, item, self->client_list, cl);
	free(cl);
	cl = NULL;
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
