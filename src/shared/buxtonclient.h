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

/**
 * \file client.h Internal header
 * This file is used internally by buxton to provide functionality
 * used for buxtonctl
 */
#pragma once

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdbool.h>

/**
 * Used to communicate with Buxton
 */
typedef struct BuxtonClient {
	int fd; /**<The file descriptor for the connection */
	bool direct; /**<Only used for direction connections */
	pid_t pid; /**<Process ID, used within libbuxton */
	uid_t uid; /**<User ID of currently using user */
} _BuxtonClient;

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
