/*
 * This file is part of buxton.
 *
 * Copyright (C) 2014 Intel Corporation
 *
 * Buxton is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the license, or (at your option) any later version.
 */

/**
 * \file buxtonsimple.h Buxton public header
 *
 *
 * \copyright Copyright (C) 2014 Intel corporation
 * \par License
 * GNU Lesser General Public License 2.1
 */

#include "buxton.h"
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#if (__GNUC__ >= 4)
/* Export symbols */
#    define _bx_export_ __attribute__ ((visibility("default")))
#else
#  define _bx_export_
#endif

#pragma once

/* Compiler will think that some variables are unused due to use of macros
 * to print them out, i.e. via buxton_debug. As a more logical approach to
 * defining them as as __attribute__ ((unused)) we define a more intuitive
 * label for them.
 */
#define _bxt_used_ __attribute__ ((unused))

/**
 * @struct vstatus
 * @brief Structure with possible data types for key values and status for buxton_response_status
 * For setting a value, the caller stores the value to be set and the BuxtonDataType before callback
 * For getting a value, the caller stores the BuxtonDataType before callback
 * The buxton get callback puts the value that should be returned into the structure
 * The buxton set callback accesses the structure for the value to be set and prints it(in debug mode)
 * status is used to check for success or failure of an operation
 * status is set to 0 on failure and 1 on success(set when checking buxton_response_status)
 * @var vstatus::status
 * Member 'status' contains a boolean value to indicate whether an operation failed or succeeded
 * @var vstatus::type
 * Member 'type' contains a BuxtonDataType that holds the type of value being set or gotten
 * @var vstatus::sval
 * Member 'sval' a string that contains the value for (set) or from (get) a BUXTON_TYPE_STRING key
 * @var vstatus::i32val
 * Member 'i32val' a 32-bit integer that contains the value for or from an BUXTON_TYPE_INT32 key
 * @var vstatus::ui32val
 * Member 'ui32val' an unsigned 32-bit integer that contains the value for or from a BUXTON_TYPE_UINT32 key
 * @var vstatus::i64val
 * Member 'i64val' a 64-bit integer that contains the value for or from an BUXTON_TYPE_INT64 key
 * @var vstatus::ui64val
 * Member 'ui64val' an unsigned 64-bit integer that contains the value for or from a BUXTON_TYPE_UINT64 key
 * @var vstatus::fval
 * Member 'fval' a floating point that contains the value for or from a BUXTON_TYPE_FLOAT key
 * @var vstatus::dval
 * Member 'dval' a double that contains the value for or from a BUXTON_TYPE_DOUBLE key
 * @var vstatus::bval
 * Member 'bval' a boolean that contains the value for or from a BUXTON_TYPE_BOOLEAN key
 */
typedef struct vstatus {
	int status;
	BuxtonDataType type;
	union {
		char *sval;
		int32_t i32val;
		uint32_t ui32val;
		int64_t i64val;
		uint64_t ui64val;
		float fval;
		double dval;
		bool bval;
	} val;
} vstatus;

extern BuxtonClient client;

/**
 * Checks for client connection and opens it if client connection is not open
 * @return Returns 1 on success and 0 on failure
 */
int _client_connection(void);

/**
 * Checks for client connections and closes it if client connection is open
 */
void _client_disconnect(void);

/**
 * Create group callback
 * @param response BuxtonResponse
 * @param data A void pointer that points to data passed in by buxton_create_group
 */
void _cg_cb(BuxtonResponse response, void *data);

/**
 * Prints the value that has been set along with the key name, group, and layer (when in debug mode)
 * @param data Pointer to structure with the value to be set, its type, and a status to be set on success
 * @param response A BuxtonResponse used to get and print the key name, group, and layer
 */
void _bs_print(vstatus *data, BuxtonResponse response);

/**
 * Buxton set value callback checks buxton_response_status and calls bs_print
 * @param response A BuxtonResponse that is used to see if value has been set properly
 * @param data A void pointer to a vstatus structure with status that will be set
 */
void _bs_cb(BuxtonResponse response, void *data);

/**
 * Buxton get value callback
 * @param response A BuxtonResponse used to get the value and check status (buxton_response_status)
 * @param data A void pointer to a vstatus structure with status and value that will be set
 */
void _bg_cb(BuxtonResponse response, void *data);

/**
 * Creates a BuxtonKey internally for buxtond_remove_group to remove
 * @param name A group name that is a string (char *)
 * @param layer A layer name that is a string (char *)
 * @return A BuxtonKey that is a group
 */
BuxtonKey _buxton_group_create(char *name, char *layer);

/**
 * Remove group callback
 * @param response A BuxtonResponse
 * @param data A void pointer
 */
void _rg_cb(BuxtonResponse response, void *data);

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
