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

#include "buxton.h"
#include "buxtonarray.h"

/**
 * Magic for Buxton messages
 */
#define BUXTON_CONTROL_CODE 0x672

/**
 * Location of size in serialized message data
 */
#define BUXTON_LENGTH_OFFSET sizeof(uint32_t)

/**
 * Minimum size of serialized BuxtonData
 * 2 is the minimum number of characters in a valid SMACK label
 * 0 is the mimimum number of characters in a valid value (NULL STRING)
 */
#define BXT_MINIMUM_SIZE sizeof(BuxtonDataType) \
	+ (sizeof(uint32_t) * 2)		\
	+ 2

/**
 * Length of valid message header
 */
#define BUXTON_MESSAGE_HEADER_LENGTH sizeof(uint32_t)	\
	+ sizeof(uint32_t)
/**
 * Maximum length of valid control message
 */
#define BUXTON_MESSAGE_MAX_LENGTH 32768

/**
 * Maximum length of valid control message
 */
#define BUXTON_MESSAGE_MAX_PARAMS 16

/**
 * Serialize data internally for backend consumption
 * @param source Data to be serialized
 * @param label Label to be serialized
 * @param target Pointer to store serialized data in
 * @return a size_t value, indicating the size of serialized data
 */
size_t buxton_serialize(BuxtonData *source, BuxtonString *label,
			uint8_t **target)
	__attribute__((warn_unused_result));

/**
 * Deserialize internal data for client consumption
 * @param source Serialized data pointer
 * @param target A pointer where the deserialize data will be stored
 * @param label A pointer where the deserialize label will be stored
 */
void buxton_deserialize(uint8_t *source, BuxtonData *target,
			BuxtonString *label);

/**
 * Serialize an internal buxton message for wire communication
 * @param dest Pointer to store serialized message in
 * @param message The type of message to be serialized
 * @param msgid The message ID to be serialized
 * @param list An array of BuxtonData's to be serialized
 * @return a size_t, 0 indicates failure otherwise size of dest
 */
size_t buxton_serialize_message(uint8_t **dest,
				BuxtonControlMessage message,
				uint32_t msgid,
				BuxtonArray *list)
	__attribute__((warn_unused_result));

/**
 * Deserialize the given data into an array of BuxtonData structs
 * @param data The source data to be deserialized
 * @param r_message An empty pointer that will be set to the message type
 * @param size The size of the data being deserialized
 * @param r_msgid The message ID being deserialized
 * @param list A pointer that will be filled out as an array of BuxtonData structs
 * @return the length of the array, or -1 if deserialization failed
 */
ssize_t buxton_deserialize_message(uint8_t *data,
				  BuxtonControlMessage *r_message,
				  size_t size, uint32_t *r_msgid,
				  BuxtonData **list)
	__attribute__((warn_unused_result));

/**
 * Get size of a buxton message data stream
 * @param data The source data stream
 * @param size The size of the data stream (from read)
 * @return a size_t length of the complete message or 0
 */
size_t buxton_get_message_size(uint8_t *data, size_t size)
	__attribute__((warn_unused_result));

void include_serialize(void);

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
