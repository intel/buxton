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
 * \file buxton-simp.h Buxton public header
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

/**
 * Structure with possible data types for key values and status for buxton_response_status
 * For setting a value, the caller stores the value to be set and the BuxtonDataType before callback
 * For getting a value, the caller stores the BuxtonDataType before callback
 * The buxton get callback puts the value that should be returned into the structure
 * The buxton set callback accesses the structure for the value to be set and prints it(in debug mode)
 * status is used to check for success or failure of an operation
 * status is set to 0 on failure and 1 on success(set when checking buxton_response_status)
 */
typedef struct vstatus {
	int status;
	BuxtonDataType type;
	union {
		char * sval;
		int32_t i32val;
		uint32_t ui32val;
		int64_t i64val;
		uint64_t ui64val;
		float fval;
		double dval;
		bool bval;
	} val;
} vstatus;
/**
 * Saves errno at the beginning of methods that could change errno 
 * errno can be set by the programmer above a method call and checked after to see if method succeeds
 */
void _save_errno(void);
/**
 * Opens client connection when called by client_connection()
 */
void _sbuxton_open(void);
/**
 * Closes client connections when called by client_disconnect()
 */
void _sbuxton_close(void);
/**
 * Checks for client connection and calls sbuxton_open if client connection is not open
 */
void _client_connection(void);
/**
 * Checks for client connections and calls sbuxton_close if client connection is open
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


