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

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#if (__GNUC__ >= 4)
/* Export symbols */
#    define _bx_export_ __attribute__ ((visibility("default")))
#else
#  define _bx_export_
#endif
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

/*Buxton Simple API Methods*/

/**
 * Saves errno at the beginning of methods that could change errno 
 * errno can be set by the programmer above a method call and checked after to see if method succeeds
 */
void save_errno(void);
/**
 * Opens client connection when called by client_connection()
 */
void sbuxton_open(void);
/**
 * Closes client connections when called by client_disconnect()
 */
void sbuxton_close(void);
/**
 * Checks for client connection and calls sbuxton_open if client connection is not open
 */
void client_connection(void);
/**
 * Checks for client connections and calls sbuxton_close if client connection is open
 */
void client_disconnect(void);
/**
 * Create group callback
 * @param response BuxtonResponse
 * @param data A void pointer that points to data passed in by buxton_create_group
 */
void cg_cb(BuxtonResponse response, void *data);
/**
 * Creates a group if it does not exist and uses that group for all following get and set calls
 * If the group already exists, it will be used for all following get and set calls
 * @param group A group name that is a string (char *)
 * @param layer A layer name that is a string (char *)
 */
_bx_export_ void buxtond_set_group(char *group, char *layer);
/**
 * Prints the value that has been set along with the key name, group, and layer (when in debug mode)
 * @param data Pointer to structure with the value to be set, its type, and a status to be set on success
 * @param response A BuxtonResponse used to get and print the key name, group, and layer  
 */
void bs_print(vstatus *data, BuxtonResponse response);
/** 
 * Buxton set value callback checks buxton_response_status and calls bs_print
 * @param response A BuxtonResponse that is used to see if value has been set properly  
 * @param data A void pointer to a vstatus structure with status that will be set
 */
void bs_cb(BuxtonResponse response, void *data);
/**
 * Buxton get value callback
 * @param response A BuxtonResponse used to get the value and check status (buxton_response_status)
 * @param data A void pointer to a vstatus structure with status and value that will be set
 */
void bg_cb(BuxtonResponse response, void *data);
/**
 * Buxton set int32_t callback
 * @param response A BuxtonResponse used to see if setting the value fails and to get key information
 * @param data A void pointer to a 32-bit integer value
 */
void bsi32_cb(BuxtonResponse response, void *data);
/** 
 * Buxton set int32_t sets an int32_t value for a given key
 * @param key A key name that is a string (char *)
 * @param value A 32-bit integer value
 */
_bx_export_ void buxtond_set_int32(char *key, int32_t value);
/**
 * Buxton get int32_t callback 
 * @param response A BuxtonResponse used to see if getting the value fails and to get key information
 * @param data A void pointer to a 32-bit integer where the value will be stored
 */
void bgi32_cb(BuxtonResponse response, void *data);
/** 
 * Buxton get int32_t gets an int32_t value from a given key
 * @param key A key name that is a string (char *)
 * @return A 32-bit integer value
 */
_bx_export_ int32_t buxtond_get_int32(char *key);
/**
 * Buxton set string callback
 * @param response A BuxtonResponse used to see if setting the value fails and to get key information
 * @param data A void pointer to a string (char *) value
 */
void bss_cb(BuxtonResponse response, void *data);
/** 
 * Buxton set string sets a string (char *) value for a given key
 * @param key A key name that is a string (char *)
 * @param value A string (char *) value
 */
_bx_export_ void buxtond_set_string(char *key, char *value );
/**
 * Buxton get string callback 
 * @param response A BuxtonResponse used to see if getting the value fails and to get key information
 * @param data A void pointer to a string (char *) where the value will be stored
 */
void bgs_cb(BuxtonResponse response, void *data);
/** 
 * Buxton get string gets a string (char *) value from a given key
 * @param key A key name that is a string (char *)
 * @return A string (char *) value
 */
_bx_export_ char* buxtond_get_string(char *key);
/**
 * Buxton set uint32_t callback
 * @param response A BuxtonResponse used to see if setting the value fails and to get key information
 * @param data A void pointer to a 32-bit unsigned integer value
 */
void bsui32_cb(BuxtonResponse response, void *data);
/** 
 * Buxton set uint32_t sets a uint32_t value for a given key
 * @param key A key name that is a string (char *)
 * @param value A 32-bit unsigned integer value
 */
_bx_export_ void buxtond_set_uint32(char *key, uint32_t value);
/**
 * Buxton get uint32_t callback 
 * @param response A BuxtonResponse used to see if getting the value fails and to get key information
 * @param data A void pointer to a 32-bit unsigned integer where the value will be stored
 */
void bgui32_cb(BuxtonResponse response, void *data);
/** 
 * Buxton get uint32_t gets an uint32_t value from a given key
 * @param key A key name that is a string (char *)
 * @return A 32-bit unsigned integer value
 */
_bx_export_ uint32_t buxtond_get_uint32(char *key);
/**
 * Buxton set int64_t callback
 * @param response A BuxtonResponse used to see if setting the value fails and to get key information
 * @param data A void pointer to a 64-bit integer value
 */
void bsi64_cb(BuxtonResponse response, void *data);
/** 
 * Buxton set int64_t sets an int64_t value for a given key
 * @param key A key name that is a string (char *)
 * @param value A 64-bit integer value
 */
_bx_export_ void buxtond_set_int64(char *key, int64_t value);
/**
 * Buxton get int64_t callback 
 * @param response A BuxtonResponse used to see if getting the value fails and to get key information
 * @param data A void pointer to a 64-bit integer where the value will be stored
 */
void bgi64_cb(BuxtonResponse response, void *data);
/** 
 * Buxton get int64_t gets an int64_t value from a given key
 * @param key A key name that is a string (char *)
 * @return A 64-bit integer value
 */
_bx_export_ int64_t buxtond_get_int64(char *key);
/**
 * Buxton set uint64_t callback
 * @param response A BuxtonResponse used to see if setting the value fails and to get key information
 * @param data A void pointer to a 64-bit unsigned integer value
 */
void bsui64_cb(BuxtonResponse response, void *data);
/** 
 * Buxton set uint64_t sets a uint64_t value for a given key
 * @param key A key name that is a string (char *)
 * @param value A 64-bit unsigned integer value
 */
_bx_export_ void buxtond_set_uint64(char *key, uint64_t value);
/**
 * Buxton get uint64_t callback 
 * @param response A BuxtonResponse used to see if getting the value fails and to get key information
 * @param data A void pointer to a 64-bit unsigned integer where the value will be stored
 */
void bgui64_cb(BuxtonResponse response, void *data);
/** 
 * Buxton get uint64_t gets a uint64_t value from a given key
 * @param key A key name that is a string (char *)
 * @return A 64-bit unsigned integer value
 */
_bx_export_ uint64_t buxtond_get_uint64(char *key);
/**
 * Buxton set float callback
 * @param response A BuxtonResponse used to see if setting the value fails and to get key information
 * @param data A void pointer to a floating point value
 */
void bsf_cb(BuxtonResponse response, void *data);
/** 
 * Buxton set float sets a floating point value for a given key
 * @param key A key name that is a string (char *)
 * @param value A floating point value
 */
_bx_export_ void buxtond_set_float(char *key, float value);
/**
 * Buxton get float callback 
 * @param response A BuxtonResponse used to see if getting the value fails and to get key information
 * @param data A void pointer to a floating point where the value will be stored
 */
void bgf_cb(BuxtonResponse response, void *data);
/** 
 * Buxton get float gets a floating point value from a given key
 * @param key A key name that is a string (char *)
 * @return A floating point value
 */
_bx_export_ float buxtond_get_float(char *key);
/**
 * Buxton set double callback
 * @param response A BuxtonResponse used to see if setting the value fails and to get key information
 * @param data A void pointer to a double value
 */
void bsd_cb(BuxtonResponse response, void *data);
/** 
 * Buxton set double sets a double value for a given key
 * @param key A key name that is a string (char *)
 * @param value A double value
 */
_bx_export_ void buxtond_set_double(char *key, double value);
/**
 * Buxton get double callback 
 * @param response A BuxtonResponse used to see if getting the value fails and to get key information
 * @param data A void pointer to a double where the value will be stored
 */
void bgd_cb(BuxtonResponse response, void *data);
/** 
 * Buxton get double gets a double value from a given key
 * @param key A key name that is a string (char *)
 * @return A double value
 */
_bx_export_ double buxtond_get_double(char *key);
/**
 * Buxton set bool callback
 * @param response A BuxtonResponse used to see if setting the value fails and to get key information
 * @param data A void pointer to a boolean value
 */
void bsb_cb(BuxtonResponse response, void *data);
/** 
 * Buxton set bool sets a boolean value for a given key
 * @param key A key name that is a string (char *)
 * @param value A boolean value
 */
_bx_export_ void buxtond_set_bool(char *key, bool value);
/**
 * Buxton get bool callback 
 * @param response A BuxtonResponse used to see if getting the value fails and to get key information
 * @param data A void pointer to a boolean where the value will be stored
 */
void bgb_cb(BuxtonResponse response, void *data);
/** 
 * Buxton get bool gets a boolean value from a given key
 * @param key A key name that is a string (char *)
 * @return A boolean value
 */
_bx_export_ bool buxtond_get_bool(char *key);
/**
 * Removes a group and clears all of the key value pairs in that group
 * @param group_name A group name that is a string (char *)
 * @param layer A layer name that is a string (char *)
 */
_bx_export_ void buxtond_remove_group2(char *group_name, char *layer);


/* TODO: remove functions below from library.
 * Rename buxtond_remove_group2.
 * Take the 'd' out of buxtond for all functions? Maybe use sbuxton instead?
 */
BuxtonKey buxton_group_create(char *name, char *layer);
void buxtond_create_group(BuxtonKey group);
BuxtonKey buxtond_create_group2(char *group_name, char *layer);
void rg_cb(BuxtonResponse response, void *data);
void buxtond_remove_group(BuxtonKey group);
void buxtond_key_free(char * key_name, BuxtonDataType type);

