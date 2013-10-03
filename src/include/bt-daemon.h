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

#define _GNU_SOURCE

#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>

#ifndef btdaemonh
#define btdaemonh

#define BUXTON_SOCKET "/run/buxton-0"

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

_bx_export_ bool buxton_client_open(BuxtonClient *client);

_bx_export_ bool buxton_client_set_string(BuxtonClient *client, const char *layer, const char *key, const char *value);

_bx_export_ char* buxton_client_get_string(BuxtonClient *client, const char *layer, const char *key);

#endif /* btdaemonh */

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
