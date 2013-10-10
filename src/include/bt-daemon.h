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

#pragma once

#define _GNU_SOURCE

#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>

#if (__GNUC__ >= 4)
/* Export symbols */
#    define _bx_export_ __attribute__ ((visibility("default")))
#else
#  define _bx_export_
#endif

typedef struct BuxtonClient {
	int fd;
	bool direct;
	pid_t pid;
} BuxtonClient;

/* Buxton data types */
typedef enum BuxtonDataType {
	STRING,
	BOOLEAN,
	FLOAT,
	INT,
	DOUBLE,
	LONG,
} BuxtonDataType;

typedef union BuxtonDataStore {
	char *d_string;
	bool d_boolean;
	float d_float;
	int d_int;
	double d_double;
	long d_long;
} BuxtonDataStore;

typedef struct BuxtonData {
	BuxtonDataType type;
	BuxtonDataStore store;
} BuxtonData;

/* Buxton API Methods */
_bx_export_ bool buxton_client_open(BuxtonClient *client);

_bx_export_ bool buxton_client_set_value(BuxtonClient *client, const char *layer, const char *key, BuxtonData *data);

_bx_export_ bool buxton_client_get_value(BuxtonClient *client, const char *layer, const char *key, BuxtonData *data);

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
