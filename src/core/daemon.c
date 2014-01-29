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
#include "direct.h"
#include "log.h"
#include "util.h"
#include "buxtonlist.h"

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
	case BUXTON_CONTROL_SET_LABEL:
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
	BuxtonData response_data, kdata, mdata;
	BuxtonData *value = NULL;
	BuxtonString *key = NULL;
	BuxtonString *layer = NULL;
	BuxtonArray *out_list = NULL, *key_list = NULL;
	_cleanup_free_ uint8_t *response_store = NULL;
	uid_t uid;
	bool ret = false;
	uint64_t msgid = 0;
	uint64_t n_msgid = 0;

	assert(self);
	assert(client);

	kdata.label = buxton_string_pack("dummy");
	uid = self->buxton.client.uid;
	p_count = buxton_deserialize_message((uint8_t*)client->data, &msg, size,
					     &msgid, &list);
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
	case BUXTON_CONTROL_SET_LABEL:
		set_label(self, client, layer, key, value, &response);
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
		register_notification(self, client, key, msgid, &response);
		break;
	case BUXTON_CONTROL_UNNOTIFY:
		n_msgid = unregister_notification(self, client, key, &response);
		break;
	default:
		goto end;
	}
	/* Set a response code */
	response_data.type = INT32;
	response_data.store.d_int32 = response;
	response_data.label = buxton_string_pack("dummy");
	out_list = buxton_array_new();
	if (!out_list)
		goto end;
	if (!buxton_array_add(out_list, &response_data)) {
		buxton_log("Failed to prepare response\n");
		goto end;
	}

	switch (msg) {
	/* TODO: Use cascading switch */
	case BUXTON_CONTROL_SET:
		buxton_string_to_data(key, &kdata);
		if (!buxton_array_add(out_list, &kdata)) {
			buxton_log("Failed to prepare SET array key data\n");
			goto end;
		}
		response_len = buxton_serialize_message(&response_store,
							BUXTON_CONTROL_STATUS,
							msgid, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize set response message\n");
			goto end;
		}
		break;
	case BUXTON_CONTROL_SET_LABEL:
		buxton_string_to_data(key, &kdata);
		if (!buxton_array_add(out_list, &kdata)) {
			buxton_log("Failed to prepare SET_LABEL array key data\n");
			goto end;
		}
		response_len = buxton_serialize_message(&response_store,
							BUXTON_CONTROL_STATUS,
							msgid, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize set_label response message\n");
			goto end;
		}
		break;
	case BUXTON_CONTROL_GET:
		buxton_string_to_data(key, &kdata);
		if (!buxton_array_add(out_list, &kdata)) {
			buxton_log("Failed to prepare GET array key data\n");
			goto end;
		}
		if (!buxton_array_add(out_list, data)) {
			buxton_log("Failed to prepare GET array data\n");
			goto end;
		}
		response_len = buxton_serialize_message(&response_store,
							BUXTON_CONTROL_STATUS,
							msgid, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize get response message\n");
			goto end;
		}
		break;
	case BUXTON_CONTROL_UNSET:
		buxton_string_to_data(key, &kdata);
		if (!buxton_array_add(out_list, &kdata)) {
			buxton_log("Failed to prepare UNSET array key data\n");
			goto end;
		}
		response_len = buxton_serialize_message(&response_store,
							BUXTON_CONTROL_STATUS,
							msgid, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize unset response message\n");
			goto end;
		}
		break;
	case BUXTON_CONTROL_LIST:
		if (key_list) {
			for (i = 0; i < key_list->len; i++) {
				if (!buxton_array_add(out_list, buxton_array_get(key_list, i)))
					goto end;
			}
			buxton_array_free(&key_list, NULL);
		}
		response_len = buxton_serialize_message(&response_store,
							BUXTON_CONTROL_STATUS,
							msgid, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize list response message\n");
			goto end;
		}
		break;
	case BUXTON_CONTROL_NOTIFY:
		buxton_string_to_data(key, &kdata);
		if (!buxton_array_add(out_list, &kdata)) {
			buxton_log("Failed to prepare NOTIFY array key data\n");
			goto end;
		}
		response_len = buxton_serialize_message(&response_store,
							BUXTON_CONTROL_STATUS,
							msgid, out_list);
		if (response_len == 0) {
			buxton_log("Failed to serialize notify response message\n");
			goto end;
		}
		break;
	case BUXTON_CONTROL_UNNOTIFY:
		buxton_string_to_data(key, &kdata);
		if (!buxton_array_add(out_list, &kdata)) {
			buxton_log("Failed to prepare UNNOTIFY array key data\n");
			goto end;
		}
		mdata.label = buxton_string_pack("dummy");
		mdata.type = UINT64;
		mdata.store.d_uint64 = n_msgid;
		if (!buxton_array_add(out_list, &mdata)) {
			buxton_log("Failed to prepare NOTIFY array msgid data\n");
			goto end;
		}
		response_len = buxton_serialize_message(&response_store,
							BUXTON_CONTROL_STATUS,
							msgid, out_list);
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
	ret = _write(client->fd, response_store, response_len);
	if (ret) {
		if (msg == BUXTON_CONTROL_SET && response == BUXTON_STATUS_OK)
			bt_daemon_notify_clients(self, client, key, value);
		else if (msg == BUXTON_CONTROL_UNSET && response == BUXTON_STATUS_OK)
			bt_daemon_notify_clients(self, client, key, NULL);
	}

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
	BuxtonList *list = NULL;
	BuxtonList *elem = NULL;
	BuxtonNotification *nitem;
	_cleanup_free_ uint8_t* response = NULL;
	size_t response_len;
	BuxtonData data;
	BuxtonArray *out_list = NULL;

	assert(self);
	assert(client);
	assert(key);

	list = hashmap_get(self->notify_mapping, key->value);
	if (!list)
		return;

	data.type = STRING;
	data.label = buxton_string_pack("dummy");
	BUXTON_LIST_FOREACH(list, elem) {
		if (!elem)
			break;
		nitem = elem->data;
		int c = 1;
		__attribute__((unused)) bool unused;
		free(response);
		response = NULL;

		if (nitem->old_data && value) {
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
			case UINT32:
				c = memcmp((const void *)&(nitem->old_data->store.d_uint32),
					   (const void *)&(value->store.d_uint32),
					   sizeof(uint32_t));
				break;
			case INT64:
				c = memcmp((const void *)&(nitem->old_data->store.d_int64),
					   (const void *)&(value->store.d_int64),
					   sizeof(int64_t));
				break;
			case UINT64:
				c = memcmp((const void *)&(nitem->old_data->store.d_uint64),
					   (const void *)&(value->store.d_uint64),
					   sizeof(uint64_t));
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
		if (nitem->old_data && value)
			buxton_data_copy(value, nitem->old_data);

		data.store.d_string.value = key->value;
		data.store.d_string.length = key->length;
		out_list = buxton_array_new();
		if (!out_list)
			return;
		if (!buxton_array_add(out_list, &data)) {
			buxton_log("Failed to add notify array data\n");
			buxton_array_free(&out_list, NULL);
			return;
		}
		if (value) {
			if (!buxton_array_add(out_list, value)) {
				buxton_log("Failed to add notify array value\n");
				buxton_array_free(&out_list, NULL);
				return;
			}
		}
		response_len = buxton_serialize_message(&response,
							BUXTON_CONTROL_CHANGED,
							nitem->msgid, out_list);
		buxton_array_free(&out_list, NULL);
		if (response_len == 0) {
			buxton_log("Failed to serialize notification\n");
			return;
		}
		buxton_debug("Notification to %d of key change (%s)\n", nitem->client->fd,
			     key->value);

		unused = _write(nitem->client->fd, response, response_len);
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

	//FIXME move direct functions to daemon only file
	/* Use internal library to set value */
	if (!buxton_direct_set_value(&self->buxton, layer, key, value, client->smack_label))
		return;

	*status = BUXTON_STATUS_OK;
	buxton_debug("Daemon set value completed\n");
}

void set_label(BuxtonDaemon *self, client_list_item *client, BuxtonString *layer,
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

	/* Use internal library to set label */
	if (!buxton_direct_set_label(&self->buxton, layer, key, &value->label))
		return;

	*status = BUXTON_STATUS_OK;
	buxton_debug("Daemon set label completed\n");
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

	/* Use internal library to unset value */
	self->buxton.client.uid = client->cred.uid;
	if (!buxton_direct_unset_value(&self->buxton, layer, key, client->smack_label))
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
		if (!buxton_direct_get_value_for_layer(&self->buxton, layer, key, data, client->smack_label))
			goto fail;
	} else {
		/* Key only */
		if (!buxton_direct_get_value(&self->buxton, key, data, client->smack_label))
			goto fail;
	}
	buxton_debug("get value returned successfully from db\n");

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

void register_notification(BuxtonDaemon *self, client_list_item *client,
			   BuxtonString *key, uint64_t msgid,
			   BuxtonStatus *status)
{
	BuxtonList *n_list = NULL;
	BuxtonNotification *nitem;
	BuxtonData *old_data = NULL;
	BuxtonStatus key_status;
	char *key_name;

	assert(self);
	assert(client);
	assert(key);
	assert(status);

	*status = BUXTON_STATUS_FAILED;

	nitem = malloc0(sizeof(BuxtonNotification));
	if (!nitem)
		return;
	nitem->client = client;

	/* Store data now, cheap */
	old_data = get_value(self, client, NULL, key, &key_status);
	if (key_status != BUXTON_STATUS_OK) {
		free(nitem);
		return;
	}
	nitem->old_data = old_data;
	nitem->msgid = msgid;

	/* May be null, but will append regardless */
	n_list = hashmap_get(self->notify_mapping, key->value);

	if (!n_list) {
		key_name = strdup(key->value);
		if (!key_name) {
			free(nitem);
			return;
		}

		if (!buxton_list_append(&n_list, nitem))
			return;
		if (hashmap_put(self->notify_mapping, key_name, n_list) < 0) {
			free(key_name);
			free(nitem);
			return;
		}
	} else {
		if (!buxton_list_append(&n_list, nitem))
			return;
	}
	*status = BUXTON_STATUS_OK;
}

uint64_t unregister_notification(BuxtonDaemon *self, client_list_item *client,
			     BuxtonString *key, BuxtonStatus *status)
{
	BuxtonList *n_list = NULL;
	BuxtonList *elem = NULL;
	BuxtonNotification *nitem, *citem = NULL;
	uint64_t msgid = 0;

	assert(self);
	assert(client);
	assert(key);
	assert(status);

	*status = BUXTON_STATUS_FAILED;
	n_list = hashmap_get(self->notify_mapping, key->value);
	/* This key isn't actually registered for notifications */
	if (!n_list)
		return 0;

	BUXTON_LIST_FOREACH(n_list, elem) {
		if (!elem)
			break;
		nitem = elem->data;
		/* Find the list item for this client */
		if (nitem->client == client)
			citem = nitem;
			break;
	};

	/* Client hasn't registered for notifications on this key */
	if (!citem)
		return 0;

	msgid = citem->msgid;
	/* Remove client from notifications */
	free_buxton_data(&(citem->old_data));
	buxton_list_remove(&n_list, citem, true);

	/* If we removed the last item, remove the mapping too */
	if (!n_list)
		hashmap_remove(self->notify_mapping, key->value);

	*status = BUXTON_STATUS_OK;

	return msgid;
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

bool handle_client(BuxtonDaemon *self, client_list_item *cl, nfds_t i)
{
	ssize_t l;
	uint16_t peek;
	bool more_data = false;
	int message_limit = 32;

	assert(self);
	assert(cl);

	if (!cl->data) {
		cl->data = malloc0(BUXTON_MESSAGE_HEADER_LENGTH);
		cl->offset = 0;
		cl->size = BUXTON_MESSAGE_HEADER_LENGTH;
	}
	if (!cl->data)
		goto cleanup;
	/* client closed the connection, or some error occurred? */
	if (recv(cl->fd, cl->data, cl->size, MSG_PEEK | MSG_DONTWAIT) <= 0)
		goto terminate;

	/* need to authenticate the client? */
	if ((cl->cred.uid == 0) || (cl->cred.pid == 0)) {
		if (!identify_client(cl))
			goto terminate;

		handle_smack_label(cl);
	}

	buxton_debug("New packet from UID %ld, PID %ld\n", cl->cred.uid, cl->cred.pid);

	/* Hand off any read data */
	do {
		l = read(self->pollfds[i].fd, (cl->data) + cl->offset, cl->size - cl->offset);

		/*
		* Close clients with read errors. If there isn't more
		* data and we don't have a complete message just
		* cleanup and let the client resend their request.
		*/
		if (l < 0) {
			if (errno != EAGAIN)
				goto terminate;
			else
				goto cleanup;
		} else if (l == 0) {
			goto cleanup;
		}

		cl->offset += (size_t)l;
		if (cl->offset < BUXTON_MESSAGE_HEADER_LENGTH) {
			continue;
		}
		if (cl->size == BUXTON_MESSAGE_HEADER_LENGTH) {
			cl->size = buxton_get_message_size(cl->data, cl->offset);
			if (cl->size == 0 || cl->size > BUXTON_MESSAGE_MAX_LENGTH)
				goto terminate;
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
			goto terminate;
		}

		message_limit--;
		if (message_limit)
			continue;
		if (recv(cl->fd, &peek, sizeof(uint16_t), MSG_PEEK | MSG_DONTWAIT) > 0)
			more_data = true;
		goto cleanup;
	} while (l > 0);

cleanup:
	free(cl->data);
	cl->data = NULL;
	cl->size = BUXTON_MESSAGE_HEADER_LENGTH;
	cl->offset = 0;
	return more_data;

terminate:
	terminate_client(self, cl, i);
	return more_data;
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
