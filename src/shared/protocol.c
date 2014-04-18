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
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

#include "buxtonclient.h"
#include "buxtonkey.h"
#include "buxtonresponse.h"
#include "buxtonstring.h"
#include "hashmap.h"
#include "log.h"
#include "protocol.h"
#include "util.h"

#define TIMEOUT 3

static pthread_mutex_t callback_guard = PTHREAD_MUTEX_INITIALIZER;
static Hashmap *callbacks = NULL;
static Hashmap *notify_callbacks = NULL;
static volatile uint32_t _msgid = 0;

struct notify_value {
	void *data;
	BuxtonCallback cb;
	struct timeval tv;
	BuxtonControlMessage type;
	_BuxtonKey *key;
};

static uint32_t get_msgid(void)
{
	return __sync_fetch_and_add(&_msgid, 1);
}

bool setup_callbacks(void)
{
	bool r = false;
	int s;

	s = pthread_mutex_lock(&callback_guard);
	if (s) {
		return false;
	}

	if (callbacks && notify_callbacks) {
		goto unlock;
	}

	callbacks = hashmap_new(trivial_hash_func, trivial_compare_func);
	if (!callbacks) {
		goto unlock;
	}

	notify_callbacks = hashmap_new(trivial_hash_func, trivial_compare_func);
	if (!notify_callbacks) {
		hashmap_free(callbacks);
		goto unlock;
	}

	r = true;

unlock:
	s = pthread_mutex_unlock(&callback_guard);
	if (s) {
		return false;
	}

	return r;
}

void cleanup_callbacks(void)
{
	struct notify_value *nvi;
	Iterator it;
#if UINTPTR_MAX == 0xffffffffffffffff
	uint64_t hkey;
#else
	uint32_t hkey;
#endif

	(void)pthread_mutex_lock(&callback_guard);

	if (callbacks) {
		HASHMAP_FOREACH_KEY(nvi, hkey, callbacks, it) {
			(void)hashmap_remove(callbacks, (void *)hkey);
			key_free(nvi->key);
			free(nvi);
		}
		hashmap_free(callbacks);
	}
	callbacks = NULL;

	if (notify_callbacks) {
		HASHMAP_FOREACH_KEY(nvi, hkey, notify_callbacks, it) {
			(void)hashmap_remove(notify_callbacks, (void *)hkey);
			key_free(nvi->key);
			free(nvi);
		}
		hashmap_free(notify_callbacks);
	}
	notify_callbacks = NULL;

	(void)pthread_mutex_unlock(&callback_guard);
}

void run_callback(BuxtonCallback callback, void *data, size_t count,
		  BuxtonData *list, BuxtonControlMessage type, _BuxtonKey *key)
{
	BuxtonArray *array = NULL;
	_BuxtonResponse response;

	if (!callback) {
		goto out;
	}

	array = buxton_array_new();
	if (!array) {
		goto out;
	}

	for (int i = 0; i < count; i++)
		if (!buxton_array_add(array, &list[i])) {
			goto out;
		}

	response.type = type;
	response.data = array;
	response.key = key;
	callback(&response, data);

out:
	buxton_array_free(&array, NULL);
}

void reap_callbacks(void)
{
	struct notify_value *nvi;
	struct timeval tv;
	Iterator it;
#if UINTPTR_MAX == 0xffffffffffffffff
	uint64_t hkey;
#else
	uint32_t hkey;
#endif

	(void)gettimeofday(&tv, NULL);

	/* remove timed out callbacks */
	HASHMAP_FOREACH_KEY(nvi, hkey, callbacks, it) {
		if (tv.tv_sec - nvi->tv.tv_sec > TIMEOUT) {
			(void)hashmap_remove(callbacks, (void *)hkey);
			key_free(nvi->key);
			free(nvi);
		}
	}
}

bool send_message(_BuxtonClient *client, uint8_t *send, size_t send_len,
		  BuxtonCallback callback, void *data, uint32_t msgid,
		  BuxtonControlMessage type, _BuxtonKey *key)
{
	struct notify_value *nv;
	_BuxtonKey *k = NULL;
	int s;
	bool r = false;

	nv = malloc0(sizeof(struct notify_value));
	if (!nv) {
		goto fail;
	}

	if (key) {
		k = malloc0(sizeof(_BuxtonKey));
		if (!k) {
			goto fail;
		}
		if (!buxton_key_copy(key, k)) {
			goto fail;
		}
	}

	(void)gettimeofday(&nv->tv, NULL);
	nv->cb = callback;
	nv->data = data;
	nv->type = type;
	nv->key = k;

	s = pthread_mutex_lock(&callback_guard);
	if (s) {
		goto fail;
	}

	reap_callbacks();

#if UINTPTR_MAX == 0xffffffffffffffff
	s = hashmap_put(callbacks, (void *)((uint64_t)msgid), nv);
#else
	s = hashmap_put(callbacks, (void *)msgid, nv);
#endif
	(void)pthread_mutex_unlock(&callback_guard);

	if (s < 1) {
		buxton_debug("Error adding callback for msgid: %llu\n", msgid);
		goto fail;
	}

	/* Now write it off */
	if (!_write(client->fd, send, send_len)) {
		buxton_debug("Write failed for msgid: %llu\n", msgid);
		r = false;
	} else {
		r = true;
	}

	return r;

fail:
	free(nv);
	key_free(k);
	return false;
}

void handle_callback_response(BuxtonControlMessage msg, uint32_t msgid,
			      BuxtonData *list, size_t count)
{
	struct notify_value *nv;

	/* use notification callbacks for notification messages */
	if (msg == BUXTON_CONTROL_CHANGED) {
#if UINTPTR_MAX == 0xffffffffffffffff
		nv = hashmap_get(notify_callbacks, (void *)((uint64_t)msgid));
#else
		nv = hashmap_get(notify_callbacks, (void *)msgid);
#endif
		if (!nv) {
			return;
		}

		run_callback((BuxtonCallback)(nv->cb), nv->data, count, list,
			     BUXTON_CONTROL_CHANGED, nv->key);
		return;
	}

#if UINTPTR_MAX == 0xffffffffffffffff
	nv = hashmap_remove(callbacks, (void *)((uint64_t)msgid));
#else
	nv = hashmap_remove(callbacks, (void *)msgid);
#endif
	if (!nv) {
		return;
	}

	if (nv->type == BUXTON_CONTROL_NOTIFY) {
		if (list[0].type == INT32 &&
		    list[0].store.d_int32 == 0) {
#if UINTPTR_MAX == 0xffffffffffffffff
			if (hashmap_put(notify_callbacks, (void *)((uint64_t)msgid), nv)
#else
			if (hashmap_put(notify_callbacks, (void *)msgid, nv)
#endif
			    >= 0) {
				return;
			}
		}
	} else if (nv->type == BUXTON_CONTROL_UNNOTIFY) {
		if (list[0].type == INT32 &&
		    list[0].store.d_int32 == 0) {
			(void)hashmap_remove(notify_callbacks,
#if UINTPTR_MAX == 0xffffffffffffffff
					     (void *)((uint64_t)list[2].store.d_uint32));
#else
					     (void *)list[2].store.d_uint32);
#endif

			return;
		}
	}

	/* callback should be run on notfiy or unnotify failure */
	/* and on any other server message we are waiting for */
	run_callback((BuxtonCallback)(nv->cb), nv->data, count, list, nv->type,
		     nv->key);

	key_free(nv->key);
	free(nv);
}

ssize_t buxton_wire_handle_response(_BuxtonClient *client)
{
	ssize_t l;
	_cleanup_free_ uint8_t *response = NULL;
	BuxtonData *r_list = NULL;
	BuxtonControlMessage r_msg = BUXTON_CONTROL_MIN;
	ssize_t count = 0;
	size_t offset = 0;
	size_t size = BUXTON_MESSAGE_HEADER_LENGTH;
	uint32_t r_msgid;
	int s;
	ssize_t handled = 0;

	s = pthread_mutex_lock(&callback_guard);
	if (s) {
		return 0;
	}
	reap_callbacks();
	(void)pthread_mutex_unlock(&callback_guard);

	response = malloc0(BUXTON_MESSAGE_HEADER_LENGTH);
	if (!response) {
		return 0;
	}

	do {
		l = read(client->fd, response + offset, size - offset);
		if (l <= 0) {
			return handled;
		}
		offset += (size_t)l;
		if (offset < BUXTON_MESSAGE_HEADER_LENGTH) {
			continue;
		}
		if (size == BUXTON_MESSAGE_HEADER_LENGTH) {
			size = buxton_get_message_size(response, offset);
			if (size == 0 || size > BUXTON_MESSAGE_MAX_LENGTH) {
				return -1;
			}
		}
		if (size != BUXTON_MESSAGE_HEADER_LENGTH) {
			response = realloc(response, size);
			if (!response) {
				return -1;
			}
		}
		if (size != offset) {
			continue;
		}

		count = buxton_deserialize_message(response, &r_msg, size, &r_msgid, &r_list);
		if (count < 0) {
			goto next;
		}

		if (!(r_msg == BUXTON_CONTROL_STATUS && r_list && r_list[0].type == INT32)
		    && !(r_msg == BUXTON_CONTROL_CHANGED)) {
			handled++;
			buxton_log("Critical error: Invalid response\n");
			goto next;
		}

		s = pthread_mutex_lock(&callback_guard);
		if (s) {
			goto next;
		}

		handle_callback_response(r_msg, r_msgid, r_list, (size_t)count);

		(void)pthread_mutex_unlock(&callback_guard);
		handled++;

	next:
		if (r_list) {
			for (int i = 0; i < count; i++) {
				if (r_list[i].type == STRING) {
					free(r_list[i].store.d_string.value);
				}
			}
			free(r_list);
		}

		/* reset for next possible message */
		size = BUXTON_MESSAGE_HEADER_LENGTH;
		offset = 0;
	} while (true);
}

int buxton_wire_get_response(_BuxtonClient *client)
{
	struct pollfd pfd[1];
	int r;
	ssize_t processed;

	pfd[0].fd = client->fd;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;
	r = poll(pfd, 1, 5000);

	if (r == 0) {
		return -ETIME;
	} else if (r < 0) {
		return -errno;
	}

	processed = buxton_wire_handle_response(client);

	return (int)processed;
}

bool buxton_wire_set_value(_BuxtonClient *client, _BuxtonKey *key, void *value,
			   BuxtonCallback callback, void *data)
{
	_cleanup_free_ uint8_t *send = NULL;
	bool ret = false;
	size_t send_len = 0;
	BuxtonArray *list = NULL;
	BuxtonData d_layer;
	BuxtonData d_group;
	BuxtonData d_name;
	BuxtonData d_value;
	uint32_t msgid = get_msgid();

	buxton_string_to_data(&key->layer, &d_layer);
	buxton_string_to_data(&key->group, &d_group);
	buxton_string_to_data(&key->name, &d_name);
	d_value.type = key->type;
	switch (key->type) {
	case STRING:
		d_value.store.d_string.value = (char *)value;
		d_value.store.d_string.length = (uint32_t)strlen((char *)value) + 1;
		break;
	case INT32:
		d_value.store.d_int32 = *(int32_t *)value;
		break;
	case INT64:
		d_value.store.d_int64 = *(int64_t *)value;
		break;
	case UINT32:
		d_value.store.d_uint32 = *(uint32_t *)value;
		break;
	case UINT64:
		d_value.store.d_uint64 = *(uint64_t *)value;
		break;
	case FLOAT:
		d_value.store.d_float = *(float *)value;
		break;
	case DOUBLE:
		memcpy(&d_value.store.d_double, value, sizeof(double));
		break;
	case BOOLEAN:
		d_value.store.d_boolean = *(bool *)value;
		break;
	default:
		break;
	}

	list = buxton_array_new();
	if (!buxton_array_add(list, &d_layer)) {
		buxton_log("Failed to add layer to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_group)) {
		buxton_log("Failed to add group to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_name)) {
		buxton_log("Failed to add name to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_value)) {
		buxton_log("Failed to add value to set_value array\n");
		goto end;
	}

	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_SET, msgid, list);

	if (send_len == 0) {
		goto end;
	}


	if (!send_message(client, send, send_len, callback, data, msgid,
			  BUXTON_CONTROL_SET, key)) {
		goto end;
	}

	ret = true;

end:
	buxton_array_free(&list, NULL);
	return ret;
}

bool buxton_wire_set_label(_BuxtonClient *client,
			   _BuxtonKey *key, BuxtonString *value,
			   BuxtonCallback callback, void *data)
{
	assert(client);
	assert(key);
	assert(value);

	_cleanup_free_ uint8_t *send = NULL;
	bool ret = false;
	size_t send_len = 0;
	BuxtonArray *list = NULL;
	BuxtonData d_layer;
	BuxtonData d_group;
	BuxtonData d_name;
	BuxtonData d_value;
	uint32_t msgid = get_msgid();

	buxton_string_to_data(&key->layer, &d_layer);
	buxton_string_to_data(&key->group, &d_group);
	buxton_string_to_data(value, &d_value);

	list = buxton_array_new();
	if (!buxton_array_add(list, &d_layer)) {
		buxton_log("Failed to add layer to set_label array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_group)) {
		buxton_log("Failed to add group to set_label array\n");
		goto end;
	}
	if (key->name.value) {
		buxton_string_to_data(&key->name, &d_name);
		if (!buxton_array_add(list, &d_name)) {
			buxton_log("Failed to add name to set_label array\n");
			goto end;
		}
	}
	if (!buxton_array_add(list, &d_value)) {
		buxton_log("Failed to add value to set_label array\n");
		goto end;
	}

	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_SET_LABEL, msgid, list);

	if (send_len == 0) {
		goto end;
	}

	if (!send_message(client, send, send_len, callback, data, msgid,
			  BUXTON_CONTROL_SET_LABEL, key)) {
		goto end;
	}

	ret = true;

end:
	buxton_array_free(&list, NULL);
	return ret;
}

bool buxton_wire_create_group(_BuxtonClient *client, _BuxtonKey *key,
			      BuxtonCallback callback, void *data)
{
	assert(client);
	assert(key);

	_cleanup_free_ uint8_t *send = NULL;
	bool ret = false;
	size_t send_len = 0;
	BuxtonArray *list = NULL;
	BuxtonData d_layer;
	BuxtonData d_group;
	uint32_t msgid = get_msgid();

	buxton_string_to_data(&key->layer, &d_layer);
	buxton_string_to_data(&key->group, &d_group);

	list = buxton_array_new();
	if (!buxton_array_add(list, &d_layer)) {
		buxton_log("Failed to add layer to create_group array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_group)) {
		buxton_log("Failed to add group to create_group array\n");
		goto end;
	}

	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_CREATE_GROUP, msgid, list);

	if (send_len == 0) {
		goto end;
	}

	if (!send_message(client, send, send_len, callback, data, msgid,
			  BUXTON_CONTROL_CREATE_GROUP, key)) {
		goto end;
	}

	ret = true;

end:
	buxton_array_free(&list, NULL);
	return ret;
}

bool buxton_wire_remove_group(_BuxtonClient *client, _BuxtonKey *key,
			      BuxtonCallback callback, void *data)
{
	assert(client);
	assert(key);

	_cleanup_free_ uint8_t *send = NULL;
	bool ret = false;
	size_t send_len = 0;
	BuxtonArray *list = NULL;
	BuxtonData d_layer;
	BuxtonData d_group;
	uint32_t msgid = get_msgid();

	buxton_string_to_data(&key->layer, &d_layer);
	buxton_string_to_data(&key->group, &d_group);

	list = buxton_array_new();
	if (!buxton_array_add(list, &d_layer)) {
		buxton_log("Failed to add layer to remove_group array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_group)) {
		buxton_log("Failed to add group to remove_group array\n");
		goto end;
	}

	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_REMOVE_GROUP, msgid, list);

	if (send_len == 0) {
		goto end;
	}

	if (!send_message(client, send, send_len, callback, data, msgid,
			  BUXTON_CONTROL_REMOVE_GROUP, key)) {
		goto end;
	}

	ret = true;

end:
	buxton_array_free(&list, NULL);
	return ret;
}

bool buxton_wire_get_value(_BuxtonClient *client, _BuxtonKey *key,
			   BuxtonCallback callback, void *data)
{
	bool ret = false;
	size_t send_len = 0;
	_cleanup_free_ uint8_t *send = NULL;
	BuxtonArray *list = NULL;
	BuxtonData d_layer;
	BuxtonData d_group;
	BuxtonData d_name;
	BuxtonData d_type;
	uint32_t msgid = get_msgid();

	buxton_string_to_data(&key->group, &d_group);
	buxton_string_to_data(&key->name, &d_name);
	d_type.type = UINT32;
	d_type.store.d_int32 = key->type;

	list = buxton_array_new();
	if (key->layer.value) {
		buxton_string_to_data(&key->layer, &d_layer);
		if (!buxton_array_add(list, &d_layer)) {
			buxton_log("Unable to prepare get_value message\n");
			goto end;
		}
	}
	if (!buxton_array_add(list, &d_group)) {
		buxton_log("Failed to add group to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_name)) {
		buxton_log("Failed to add name to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_type)) {
		buxton_log("Failed to add type to set_value array\n");
		goto end;
	}

	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_GET, msgid, list);

	if (send_len == 0) {
		goto end;
	}

	if (!send_message(client, send, send_len, callback, data, msgid,
			  BUXTON_CONTROL_GET, key)) {
		goto end;
	}

	ret = true;

end:
	buxton_array_free(&list, NULL);
	return ret;
}

bool buxton_wire_unset_value(_BuxtonClient *client,
			     _BuxtonKey *key,
			     BuxtonCallback callback,
			     void *data)
{
	assert(client);
	assert(key);

	_cleanup_free_ uint8_t *send = NULL;
	size_t send_len = 0;
	BuxtonArray *list = NULL;
	BuxtonData d_group;
	BuxtonData d_name;
	BuxtonData d_layer;
	BuxtonData d_type;
	bool ret = false;
	uint32_t msgid = get_msgid();

	buxton_string_to_data(&key->group, &d_group);
	buxton_string_to_data(&key->name, &d_name);
	buxton_string_to_data(&key->layer, &d_layer);
	d_type.type = UINT32;
	d_type.store.d_int32 = key->type;

	list = buxton_array_new();
	if (!buxton_array_add(list, &d_layer)) {
		buxton_log("Failed to add layer to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_group)) {
		buxton_log("Failed to add group to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_name)) {
		buxton_log("Failed to add name to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_type)) {
		buxton_log("Failed to add type to set_value array\n");
		goto end;
	}

	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_UNSET,
					    msgid, list);

	if (send_len == 0) {
		goto end;
	}

	if (!send_message(client, send, send_len, callback, data, msgid,
			  BUXTON_CONTROL_UNSET, key)) {
		goto end;
	}

	ret = true;

end:
	buxton_array_free(&list, NULL);
	return ret;
}

bool buxton_wire_list_keys(_BuxtonClient *client,
			   BuxtonString *layer,
			   BuxtonCallback callback,
			   void *data)
{
	assert(client);
	assert(layer);

	_cleanup_free_ uint8_t *send = NULL;
	size_t send_len = 0;
	BuxtonArray *list = NULL;
	BuxtonData d_layer;
	bool ret = false;
	uint32_t msgid = get_msgid();

	buxton_string_to_data(layer, &d_layer);

	list = buxton_array_new();
	if (!buxton_array_add(list, &d_layer)) {
		buxton_log("Unable to add layer to list_keys array\n");
		goto end;
	}

	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_LIST, msgid,
					    list);

	if (send_len == 0) {
		goto end;
	}

	if (!send_message(client, send, send_len, callback, data, msgid,
			  BUXTON_CONTROL_LIST, NULL)) {
		goto end;
	}

	ret = true;

end:
	buxton_array_free(&list, NULL);

	return ret;
}

bool buxton_wire_register_notification(_BuxtonClient *client,
				       _BuxtonKey *key,
				       BuxtonCallback callback,
				       void *data)
{
	assert(client);
	assert(key);

	_cleanup_free_ uint8_t *send = NULL;
	size_t send_len = 0;
	BuxtonArray *list = NULL;
	BuxtonData d_group;
	BuxtonData d_name;
	BuxtonData d_type;
	bool ret = false;
	uint32_t msgid = get_msgid();

	buxton_string_to_data(&key->group, &d_group);
	buxton_string_to_data(&key->name, &d_name);
	d_type.type = UINT32;
	d_type.store.d_int32 = key->type;

	list = buxton_array_new();
	if (!buxton_array_add(list, &d_group)) {
		buxton_log("Failed to add group to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_name)) {
		buxton_log("Failed to add name to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_type)) {
		buxton_log("Failed to add type to set_value array\n");
		goto end;
	}

	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_NOTIFY, msgid,
					    list);

	if (send_len == 0) {
		goto end;
	}

	if (!send_message(client, send, send_len, callback, data, msgid,
			  BUXTON_CONTROL_NOTIFY, key)) {
		goto end;
	}

	ret = true;

end:
	buxton_array_free(&list, NULL);
	return ret;
}

bool buxton_wire_unregister_notification(_BuxtonClient *client,
					 _BuxtonKey *key,
					 BuxtonCallback callback,
					 void *data)
{
	assert(client);
	assert(key);

	_cleanup_free_ uint8_t *send = NULL;
	size_t send_len = 0;
	BuxtonArray *list = NULL;
	BuxtonData d_group;
	BuxtonData d_name;
	BuxtonData d_type;
	bool ret = false;
	uint32_t msgid = get_msgid();

	buxton_string_to_data(&key->group, &d_group);
	buxton_string_to_data(&key->name, &d_name);
	d_type.type = UINT32;
	d_type.store.d_int32 = key->type;

	list = buxton_array_new();
	if (!buxton_array_add(list, &d_group)) {
		buxton_log("Failed to add group to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_name)) {
		buxton_log("Failed to add name to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_type)) {
		buxton_log("Failed to add type to set_value array\n");
		goto end;
	}

	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_UNNOTIFY,
					    msgid, list);

	if (send_len == 0) {
		goto end;
	}

	if (!send_message(client, send, send_len, callback, data, msgid,
			  BUXTON_CONTROL_UNNOTIFY, key)) {
		goto end;
	}

	ret = true;

end:
	buxton_array_free(&list, NULL);
	return ret;
}

void include_protocol(void)
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
