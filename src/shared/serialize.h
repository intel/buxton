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
 * \file serialize.h Internal header
 * This file is used internally by buxton to provide serialization
 * functionality
 */
#pragma once

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <stdint.h>

#include "bt-daemon.h"

/**
 * Minimum size of serialized BuxtonData
 */
#define BXT_MINIMUM_SIZE sizeof(BuxtonDataType) + (sizeof(int)*2)

/**
 * Serialize data internally for backend consumption
 * @param source Data to be serialized
 * @param dest Pointer to store serialized data in
 * @return a boolean value, indicating success of the operation
 */
bool buxton_serialize(BuxtonData *source, uint8_t** dest);

/**
 * Deserialize internal data for client consumption
 * @param source Serialized data pointer
 * @param dest A pointer where the deserialize data will be stored
 * @return a boolean value, indicating success of the operation
 */
bool buxton_deserialize(uint8_t *source, BuxtonData *dest);

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
