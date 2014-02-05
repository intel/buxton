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

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include "buxtonarray.h"
#include "buxtonkey.h"

/**
 * Represents daemon's reply to client
 */
typedef struct BuxtonResponse {
	BuxtonArray *data; /**<Array containing BuxtonData elements */
	BuxtonControlMessage type; /**<Type of message in the response */
	_BuxtonKey *key; /**<Key used by client to make the request */
} _BuxtonResponse;

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
