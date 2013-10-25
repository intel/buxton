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
 * \file protocol.h Internal header
 * This file is used internally by buxton to provide functionality
 * used for the wire protocol
 */
#pragma once

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include "bt-daemon.h"

/**
 * Buxton Status Codes
 */
typedef enum BuxtonStatus {
	BUXTON_STATUS_OK = 0, /**<Operation succeeded */
	BUXTON_STATUS_FAILED /**<Operation failed */
} BuxtonStatus;
 
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
