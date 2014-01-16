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
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

#include "hashmap.h"
#include "log.h"
#include "protocol.h"
#include "util.h"

#define TIMEOUT 3

static pthread_mutex_t callback_guard = PTHREAD_MUTEX_INITIALIZER;
static Hashmap *callbacks = NULL;
static Hashmap *notify_callbacks = NULL;
static volatile uint64_t _msgid = 0;

struct notify_value {
	struct timeval tv;
	BuxtonCallback cb;
};

static uint64_t get_msgid(void)
{
	return __sync_fetch_and_add(&_msgid, 1);
}

bool setup_callbacks(void)
{
	bool r = false;
	int s;

	s = pthread_mutex_lock(&callback_guard);
	if (s)
		return false;

	if (callbacks && notify_callbacks)
		goto unlock;

	callbacks = hashmap_new(trivial_hash_func, trivial_compare_func);
	if (!callbacks)
		goto unlock;

	notify_callbacks = hashmap_new(trivial_hash_func, trivial_compare_func);
	if (!notify_callbacks) {
		hashmap_free(callbacks);
		goto unlock;
	}

	r = true;

unlock:
	s = pthread_mutex_unlock(&callback_guard);
	if (s)
		return false;

	return r;
}

void cleanup_callbacks(void)
{
	(void)pthread_mutex_lock(&callback_guard);

	if (callbacks)
		hashmap_free(callbacks);
	callbacks = NULL;

	if (notify_callbacks)
		hashmap_free(notify_callbacks);
	notify_callbacks = NULL;

	(void)pthread_mutex_unlock(&callback_guard);
}

void run_callback(BuxtonCallback callback, size_t count, BuxtonData *list)
{
	BuxtonArray *array = NULL;

	if (!callback)
		goto out;

	array = buxton_array_new();
	if (!array)
		goto out;

	for (int i = 0; i < count; i++)
		if (!buxton_array_add(array, &list[i]))
			goto out;

	callback(array);

out:
	buxton_array_free(&array, NULL);
}

bool send_message(BuxtonClient *self, uint8_t *send, size_t send_len,
		  BuxtonCallback callback, uint64_t msgid)
{
	ssize_t wr;
	struct notify_value *nv;
	int s;
	bool r = false;

	nv = malloc0(sizeof(struct notify_value));
	if (!nv)
		goto end;

	(void)gettimeofday(&nv->tv, NULL);
	nv->cb = callback;

	s = pthread_mutex_lock(&callback_guard);
	if (s)
		goto end;

	s = hashmap_put(callbacks, (void *)msgid, nv);
	(void)pthread_mutex_unlock(&callback_guard);

	if (s < 1) {
		buxton_debug("Error adding callback for msgid: %llu\n", msgid);
		free(nv);
		goto end;
	}

	/* Now write it off */
	wr = write(self->fd, send, send_len);
	if (wr < 0)
		goto end;

	r = true;

end:
	return r;
}

size_t buxton_wire_get_response(BuxtonClient *self, BuxtonControlMessage *msg,
				BuxtonData **list, BuxtonCallback callback)
{
	assert(self);
	assert(msg);
	assert(list);

	ssize_t l;
	_cleanup_free_ uint8_t *response = NULL;
	BuxtonData *r_list = NULL;
	BuxtonControlMessage r_msg = BUXTON_CONTROL_MIN;
	size_t count = 0;
	size_t offset = 0;
	size_t size = BUXTON_MESSAGE_HEADER_LENGTH;
	uint64_t r_msgid, hkey;
	struct notify_value *nv, *nvi;
	struct timeval tv;
	int s;
	Iterator it;

	response = malloc0(BUXTON_MESSAGE_HEADER_LENGTH);
	if (!response)
		return 0;

	do {
		l = read(self->fd, response + offset, size - offset);
		if (l <= 0)
			return 0;
		offset += (size_t)l;
		if (offset < BUXTON_MESSAGE_HEADER_LENGTH)
			continue;
		if (size == BUXTON_MESSAGE_HEADER_LENGTH) {
			size = buxton_get_message_size(response, offset);
			if (size == 0 || size > BUXTON_MESSAGE_MAX_LENGTH)
				return 0;
		}
		if (size != BUXTON_MESSAGE_HEADER_LENGTH) {
			response = realloc(response, size);
			if (!response)
				return 0;
		}
		if (size != offset)
			continue;

		count = buxton_deserialize_message(response, &r_msg, size, &r_msgid, &r_list);
		if (count == 0)
			return 0;
		if (!(r_msg == BUXTON_CONTROL_STATUS && r_list[0].type == INT32)
		    && !(r_msg == BUXTON_CONTROL_CHANGED &&
			 r_list[0].type == STRING)) {
			buxton_log("Critical error: Invalid response\n");
			return 0;
		}
		break;
	} while (true);
	assert(r_msg > BUXTON_CONTROL_MIN);
	assert(r_msg < BUXTON_CONTROL_MAX);

	(void)gettimeofday(&tv, NULL);

	s = pthread_mutex_lock(&callback_guard);
	if (s)
		return 0;

	nv = hashmap_remove(callbacks, (void *)r_msgid);
	if (nv) {
		run_callback((BuxtonCallback)(nv->cb), count, r_list);
		free(nv);
	}

	HASHMAP_FOREACH_KEY(nvi, hkey, callbacks, it) {
		if (tv.tv_sec - nvi->tv.tv_sec > TIMEOUT) {
			(void)hashmap_remove(callbacks, (void *)hkey);
			free(nvi);
		}
	}

	(void)pthread_mutex_unlock(&callback_guard);

	*msg = r_msg;
	*list = r_list;

	return count;
}

bool buxton_wire_set_value(BuxtonClient *self, BuxtonString *layer_name,
			   BuxtonString *key, BuxtonData *value,
			   BuxtonCallback callback)
{
	assert(self);
	assert(layer_name);
	assert(key);
	assert(value);
	assert(value->label.value);

	_cleanup_free_ uint8_t *send = NULL;
	bool ret = false;
	size_t count;
	size_t send_len = 0;
	BuxtonControlMessage r_msg;
	_cleanup_free_ BuxtonData *r_list = NULL;
	BuxtonArray *list = NULL;
	BuxtonData d_layer;
	BuxtonData d_key;
	uint64_t msgid = get_msgid();

	buxton_string_to_data(layer_name, &d_layer);
	buxton_string_to_data(key, &d_key);
	d_layer.label = buxton_string_pack("dummy");
	d_key.label = buxton_string_pack("dummy");

	list = buxton_array_new();
	if (!buxton_array_add(list, &d_layer)) {
		buxton_log("Failed to add layer to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_key)) {
		buxton_log("Failed to add key to set_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, value)) {
		buxton_log("Failed to add value to set_value array\n");
		goto end;
	}
	/* Attempt to serialize our send message */
	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_SET, msgid, list);
	if (send_len == 0)
		goto end;

	if (!send_message(self, send, send_len, callback, msgid))
		goto end;

	/* Gain response */
	count = buxton_wire_get_response(self, &r_msg, &r_list, callback);
	if (count > 0 && r_list[0].store.d_int32 == BUXTON_STATUS_OK)
		ret = true;

end:
	buxton_array_free(&list, NULL);
	return ret;
}

bool buxton_wire_get_value(BuxtonClient *self, BuxtonString *layer_name,
			   BuxtonString *key, BuxtonData *value,
			   BuxtonCallback callback)
{
	assert(self);
	assert(key);
	assert(value);

	bool ret = false;
	size_t count = 0;
	size_t send_len = 0;
	int i;
	_cleanup_free_ uint8_t *send = NULL;
	BuxtonControlMessage r_msg;
	BuxtonData *r_list = NULL;
	BuxtonArray *list = NULL;
	BuxtonData d_layer;
	BuxtonData d_key;
	uint64_t msgid = get_msgid();

	buxton_string_to_data(key, &d_key);
	d_key.label = buxton_string_pack("dummy");

	/* Attempt to serialize our send message */
	list = buxton_array_new();
	if (layer_name != NULL) {
		buxton_string_to_data(layer_name, &d_layer);
		d_layer.label = buxton_string_pack("dummy");
		if (!buxton_array_add(list, &d_layer)) {
			buxton_log("Unable to prepare get_value message\n");
			goto end;
		}
	}
	if (!buxton_array_add(list, &d_key)) {
		buxton_log("Unable to prepare get_value message\n");
		goto end;
	}

	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_GET, msgid, list);

	if (send_len == 0)
		goto end;

	if (!send_message(self, send, send_len, callback, msgid))
		goto end;

	/* Gain response */
	count = buxton_wire_get_response(self, &r_msg, &r_list, callback);
	if (count == 3 && r_list[0].store.d_int32 == BUXTON_STATUS_OK)
		ret = true;
	else
		goto end;

	/* Now copy the data over for the user */
	buxton_data_copy(&r_list[2], value);

end:
	if (r_list) {
		for (i=0; i < count; i++) {
			free(r_list[i].label.value);
			if (r_list[i].type == STRING)
				free(r_list[i].store.d_string.value);
		}
		free(r_list);
	}
	buxton_array_free(&list, NULL);
	return ret;
}

bool buxton_wire_unset_value(BuxtonClient *self,
			     BuxtonString *layer_name,
			     BuxtonString *key,
			     BuxtonCallback callback)
{
	assert(self);
	assert(layer_name);
	assert(key);

	size_t count;
	_cleanup_free_ uint8_t *send = NULL;
	size_t send_len = 0;
	BuxtonControlMessage r_msg;
	_cleanup_free_ BuxtonData *r_list = NULL;
	BuxtonArray *list = NULL;
	BuxtonData d_key, d_layer;
	bool ret = false;
	uint64_t msgid = get_msgid();

	buxton_string_to_data(key, &d_key);
	d_key.label = buxton_string_pack("dummy");
	buxton_string_to_data(layer_name, &d_layer);
	d_layer.label = buxton_string_pack("dummy");

	/* Attempt to serialize our send message */
	list = buxton_array_new();
	if (!buxton_array_add(list, &d_layer)) {
		buxton_log("Unable to add layer to get_value array\n");
		goto end;
	}
	if (!buxton_array_add(list, &d_key)) {
		buxton_log("Unable to add key to get_value array\n");
		goto end;
	}
	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_UNSET,
					    msgid, list);

	if (send_len == 0)
		goto end;

	if (!send_message(self, send, send_len, callback, msgid))
		goto end;

	/* Gain response */
	count = buxton_wire_get_response(self, &r_msg, &r_list, callback);
	if (count > 0 && r_list[0].store.d_int32 == BUXTON_STATUS_OK)
		ret = true;

end:
	buxton_array_free(&list, NULL);
	return ret;
}

bool buxton_wire_list_keys(BuxtonClient *self,
			   BuxtonString *layer,
			   BuxtonArray **array,
			   BuxtonCallback callback)
{
	assert(self);
	assert(layer);

	size_t count;
	_cleanup_free_ uint8_t *send = NULL;
	size_t send_len = 0;
	BuxtonControlMessage r_msg;
	_cleanup_free_ BuxtonData *r_list = NULL;
	BuxtonArray *list = NULL;
	BuxtonArray *ret_list = NULL;
	int i;
	BuxtonData d_layer;
	bool ret = false;
	uint64_t msgid = get_msgid();

	buxton_string_to_data(layer, &d_layer);
	d_layer.label = buxton_string_pack("dummy");

	/* Attempt to serialize our send message */
	list = buxton_array_new();
	if (!buxton_array_add(list, &d_layer)) {
		buxton_log("Unable to add layer to list_keys array\n");
		goto end;
	}
	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_LIST, msgid,
					    list);
	if (send_len == 0)
		goto end;

	if (!send_message(self, send, send_len, callback, msgid))
		goto end;

	buxton_array_free(&list, NULL);

	/* Gain response */
	count = buxton_wire_get_response(self, &r_msg, &r_list, callback);
	if (!(count > 0 && r_list[0].store.d_int32 == BUXTON_STATUS_OK))
		goto end;

	ret_list = buxton_array_new();
	for (i = 1; i < count; i ++) {
		if (!buxton_array_add(ret_list, &r_list[i])) {
			buxton_log("Unable to send list-keys response\n");
			goto end;
		}
	}
	*array = ret_list;
	ret = true;

end:
	if (!ret)
		buxton_array_free(&ret_list, NULL);

	return ret;
}

bool buxton_wire_register_notification(BuxtonClient *self,
				       BuxtonString *key,
				       BuxtonCallback callback)
{
	assert(self);
	assert(key);

	size_t count;
	_cleanup_free_ uint8_t *send = NULL;
	size_t send_len = 0;
	BuxtonControlMessage r_msg;
	_cleanup_free_ BuxtonData *r_list = NULL;
	BuxtonArray *list = NULL;
	BuxtonData d_key;
	bool ret = false;
	uint64_t msgid = get_msgid();

	buxton_string_to_data(key, &d_key);
	d_key.label = buxton_string_pack("dummy");

	/* Attempt to serialize our send message */
	list = buxton_array_new();
	if (!buxton_array_add(list, &d_key)) {
		buxton_log("Unable to add key to register_notification array\n");
		goto end;
	}
	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_NOTIFY, msgid,
					    list);
	if (send_len == 0)
		goto end;

	if (!send_message(self, send, send_len, callback, msgid))
		goto end;

	/* Gain response */
	count = buxton_wire_get_response(self, &r_msg, &r_list, callback);
	if (count > 0 && r_list[0].store.d_int32 == BUXTON_STATUS_OK)
		ret = true;

end:
	buxton_array_free(&list, NULL);
	return ret;
}

bool buxton_wire_unregister_notification(BuxtonClient *self,
					 BuxtonString *key,
					 BuxtonCallback callback)
{
	assert(self);
	assert(key);

	size_t count;
	_cleanup_free_ uint8_t *send = NULL;
	size_t send_len = 0;
	BuxtonControlMessage r_msg;
	_cleanup_free_ BuxtonData *r_list = NULL;
	BuxtonArray *list = NULL;
	BuxtonData d_key;
	bool ret = false;
	uint64_t msgid = get_msgid();

	buxton_string_to_data(key, &d_key);
	d_key.label = buxton_string_pack("dummy");

	/* Attempt to serialize our send message */
	list = buxton_array_new();
	if (!buxton_array_add(list, &d_key)) {
		buxton_log("Unable to add key to unregister_notification array\n");
		goto end;
	}
	send_len = buxton_serialize_message(&send, BUXTON_CONTROL_UNNOTIFY,
					    msgid, list);

	if (send_len == 0)
		goto end;

	if (!send_message(self, send, send_len, callback, msgid))
		goto end;

	/* Gain response */
	count = buxton_wire_get_response(self, &r_msg, &r_list, callback);
	if (count > 0 && r_list[0].store.d_int32 == BUXTON_STATUS_OK)
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
