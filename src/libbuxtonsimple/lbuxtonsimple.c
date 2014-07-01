/*
 * This file is part of buxton.
 *
 * Copyright (C) 2014 Intel Corporation
 *
 * buxton is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the license, or (at your option) any later version.
 */
 
 /**
 * \file lbuxton.c Buxton library implementation
 */
 
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buxtonsimple.h"
#include "buxtonsimple-internals.h"
#include "log.h"
/* Max length of layer and group names  */
#define MAX_LG_LEN 256

extern BuxtonClient client;
static char _layer[MAX_LG_LEN];
static char _group[MAX_LG_LEN];
static int saved_errno;

/* Initialization of group */
void buxtond_set_group(char *group, char *layer)
{
	if (!_client_connection()) {
		errno = ENOTCONN;
		return;
	}
	saved_errno = errno;
	int status = 0;
	/* strcpy the name of the layer and group*/
	strncpy(_layer, layer, MAX_LG_LEN-1);
	strncpy(_group, group, MAX_LG_LEN-1);
	/* In case a string is longer than MAX_LG_LEN, set the last byte to null */
	_layer[MAX_LG_LEN -1] = '\0';
	_group[MAX_LG_LEN -1] = '\0';
	BuxtonKey g = buxton_key_create(_group, NULL, _layer, STRING);
	buxton_debug("buxton key group = %s\n", buxton_key_get_group(g));
	if (buxton_create_group(client, g, _cg_cb, &status, true)
		|| !status) {
		buxton_debug("Create group call failed.\n");
		errno = EBADMSG;
	} else {
		buxton_debug("Switched to group: %s, layer: %s.\n", buxton_key_get_group(g),
 	buxton_key_get_layer(g));
		errno = saved_errno;
	}
	buxton_key_free(g);
	_client_disconnect();
}

/* Set and get int32_t value for buxton key with type INT32 */
void buxtond_set_int32(char *key, int32_t value)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return;
	}
	/* create key  */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT32);
	/* return value and status */
	vstatus ret;
	ret.type = INT32;
	ret.val.i32val = value;
	saved_errno = errno;
	/* call buxton_set_value for type INT32 */
	if (buxton_set_value(client, _key, &value, _bs_cb, &ret, true)) {
		buxton_debug("Set int32_t call failed.\n");
		return;
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

int32_t buxtond_get_int32(char *key)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return -1;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT32);
	/* return value */
	vstatus ret;
	ret.type = INT32;
	saved_errno = errno;
	/* get value */
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)) {
		buxton_debug("Get int32_t call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.i32val;
}

/* Set and get char * value for buxton key with type STRING */
void buxtond_set_string(char *key, char *value )
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, STRING);
	/* return value and status */
	vstatus ret;
	ret.type = STRING;
	ret.val.sval = value;
	saved_errno = errno;
	/* set value */
	if (buxton_set_value(client, _key, &value, _bs_cb, &ret, true)) {
		buxton_debug("Set string call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

char* buxtond_get_string(char *key)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return NULL;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, STRING);
	/* return value */
	vstatus ret;
	ret.type = STRING;
	saved_errno = errno;
	/* get value */
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)) {
		buxton_debug("Get string call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.sval;
}

/* Set and get uint32_t value for buxton key with type UINT32 */
void buxtond_set_uint32(char *key, uint32_t value)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT32);
	/* return value and status */
	vstatus ret;
	ret.type = UINT32;
	ret.val.ui32val = value;
	saved_errno = errno;
	if (buxton_set_value(client,_key, &value, _bs_cb, &ret, true)) {
		buxton_debug("Set uint32_t call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

uint32_t buxtond_get_uint32(char *key)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return 0;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT32);
	/* return value */
	vstatus ret;
	ret.type = UINT32;
	saved_errno = errno;
	/* get value */
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)) {
		buxton_debug("Get uint32_t call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.ui32val;
}

/* Set and get int64_t value for buxton key with type INT64 */
void buxtond_set_int64(char *key, int64_t value)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT64);
	/* return value and status */
	vstatus ret;
	ret.type = INT64;
	ret.val.i64val = value;
	saved_errno = errno;
	if (buxton_set_value(client, _key, &value, _bs_cb, &ret, true)) {
		buxton_debug("Set int64_t call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

int64_t buxtond_get_int64(char *key)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return -1;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT64);
	/* return value */
	vstatus ret;
	ret.type = INT64;
	saved_errno = errno;
	/* get value */
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)) {
		buxton_debug("Get int64_t call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.i64val;
}

/* Set and get uint64_t value for buxton key with type UINT64 */
void buxtond_set_uint64(char *key, uint64_t value)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT64);
	/* return value and status */
	vstatus ret;
	ret.type = UINT64;
	ret.val.ui64val = value;
	saved_errno = errno;
	if (buxton_set_value(client, _key, &value, _bs_cb, &ret, true)) {
		buxton_debug("Set uint64_t call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

uint64_t buxtond_get_uint64(char *key)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return 0;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT64);
	/* return value */
	vstatus ret;
	ret.type = UINT64;
	saved_errno = errno;
	/* get value */
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)) {
		buxton_debug("Get uint64_t call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.ui64val;
}

/* Set and get float value for buxton key with type FLOAT */
void buxtond_set_float(char *key, float value)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, FLOAT);
	/* return value and status */
	vstatus ret;
	ret.type = FLOAT;
	ret.val.fval = value;
	saved_errno = errno;
	if (buxton_set_value(client, _key, &value, _bs_cb, &ret, true)) {
		buxton_debug("Set float call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

float buxtond_get_float(char *key)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return -1;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, FLOAT);
	/* return value */
	vstatus ret;
	ret.type = FLOAT;
	saved_errno = errno;
	/* get value */
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)) {
		buxton_debug("Get float call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.fval;
}

/* Set and get double value for buxton key with type DOUBLE */
void buxtond_set_double(char *key, double value)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, DOUBLE);
	/* return value and status */
	vstatus ret;
	ret.type = DOUBLE;
	ret.val.dval = value;
	saved_errno = errno;
	if (buxton_set_value(client, _key, &value, _bs_cb, &ret, true)) {
		buxton_debug("Set double call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

double buxtond_get_double(char *key)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return -1;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, DOUBLE);
	/* return value */
	vstatus ret;
	ret.type = DOUBLE;
	saved_errno = errno;
	/* get value */
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)) {
		buxton_debug("Get double call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.dval;
}

/* Set and get bool value for buxton key with type BOOLEAN */
void buxtond_set_bool(char *key, bool value)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, BOOLEAN);
	/* return value and status */
	vstatus ret;
	ret.type = BOOLEAN;
	ret.val.bval = value;
	saved_errno = errno;
	if (buxton_set_value(client, _key, &value, _bs_cb, &ret, true)) {
		buxton_debug("Set bool call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

bool buxtond_get_bool(char *key)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return false;
	}
	/* create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, BOOLEAN);
	/* return value */
	vstatus ret;
	ret.type = BOOLEAN;
	saved_errno = errno;
	/* get value */
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)) {
		buxton_debug("Get bool call failed.\n");
	}
	if (!ret.status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.bval;
}

/* Remove group given its name and layer */
void buxtond_remove_group(char *group_name, char *layer)
{
	/* make sure client connection is open */
	if (!_client_connection()) {
		errno = ENOTCONN;
		return;
	}
	saved_errno = errno;
	BuxtonKey group = _buxton_group_create(group_name, layer);
	int status;
	if (buxton_remove_group(client, group, _rg_cb, &status, true)) {
		buxton_debug("Remove group call failed.\n");
	}
	if (!status) {
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(group);
	_client_disconnect();
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
