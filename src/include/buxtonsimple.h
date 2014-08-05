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


/*Buxton Simple API Methods*/
/**
 * Creates a group if it does not exist and uses that group for all following get and set calls
 * If the group already exists, it will be used for all following get and set calls
 * Group and layer names longer than 256 bits will be truncated
 * @param group A group name that is a string (char *)
 * @param layer A layer name that is a string (char *)
 */
_bx_export_ void sbuxton_set_group(char *group, char *layer);
/** 
 * Buxton set int32_t sets an int32_t value for a given key
 * @param key A key name that is a string (char *)
 * @param value A 32-bit integer value
 */
_bx_export_ void sbuxton_set_int32(char *key, int32_t value);
/** 
 * Buxton get int32_t gets an int32_t value from a given key
 * @param key A key name that is a string (char *)
 * @return A 32-bit integer value
 */
_bx_export_ int32_t sbuxton_get_int32(char *key);
/** 
 * Buxton set string sets a string (char *) value for a given key
 * @param key A key name that is a string (char *)
 * @param value A string (char *) value
 */
_bx_export_ void sbuxton_set_string(char *key, char *value );
/** 
 * Buxton get string gets a string (char *) value from a given key
 * @param key A key name that is a string (char *)
 * @return A string (char *) value
 */
_bx_export_ char* sbuxton_get_string(char *key);
/** 
 * Buxton set uint32_t sets a uint32_t value for a given key
 * @param key A key name that is a string (char *)
 * @param value A 32-bit unsigned integer value
 */
_bx_export_ void sbuxton_set_uint32(char *key, uint32_t value);
/** 
 * Buxton get uint32_t gets an uint32_t value from a given key
 * @param key A key name that is a string (char *)
 * @return A 32-bit unsigned integer value
 */
_bx_export_ uint32_t sbuxton_get_uint32(char *key);
/** 
 * Buxton set int64_t sets an int64_t value for a given key
 * @param key A key name that is a string (char *)
 * @param value A 64-bit integer value
 */
_bx_export_ void sbuxton_set_int64(char *key, int64_t value);
/** 
 * Buxton get int64_t gets an int64_t value from a given key
 * @param key A key name that is a string (char *)
 * @return A 64-bit integer value
 */
_bx_export_ int64_t sbuxton_get_int64(char *key);
/** 
 * Buxton set uint64_t sets a uint64_t value for a given key
 * @param key A key name that is a string (char *)
 * @param value A 64-bit unsigned integer value
 */
_bx_export_ void sbuxton_set_uint64(char *key, uint64_t value);
/** 
 * Buxton get uint64_t gets a uint64_t value from a given key
 * @param key A key name that is a string (char *)
 * @return A 64-bit unsigned integer value
 */
_bx_export_ uint64_t sbuxton_get_uint64(char *key);
/** 
 * Buxton set float sets a floating point value for a given key
 * @param key A key name that is a string (char *)
 * @param value A floating point value
 */
_bx_export_ void sbuxton_set_float(char *key, float value);
/** 
 * Buxton get float gets a floating point value from a given key
 * @param key A key name that is a string (char *)
 * @return A floating point value
 */
_bx_export_ float sbuxton_get_float(char *key);
/** 
 * Buxton set double sets a double value for a given key
 * @param key A key name that is a string (char *)
 * @param value A double value
 */
_bx_export_ void sbuxton_set_double(char *key, double value);
/** 
 * Buxton get double gets a double value from a given key
 * @param key A key name that is a string (char *)
 * @return A double value
 */
_bx_export_ double sbuxton_get_double(char *key);
/** 
 * Buxton set bool sets a boolean value for a given key
 * @param key A key name that is a string (char *)
 * @param value A boolean value
 */
_bx_export_ void sbuxton_set_bool(char *key, bool value);
/** 
 * Buxton get bool gets a boolean value from a given key
 * @param key A key name that is a string (char *)
 * @return A boolean value
 */
_bx_export_ bool sbuxton_get_bool(char *key);
/**
 * Removes a group and clears all of the key value pairs in that group
 * @param group_name A group name that is a string (char *)
 * @param layer A layer name that is a string (char *)
 */
_bx_export_ void sbuxton_remove_group(char *group_name, char *layer);
