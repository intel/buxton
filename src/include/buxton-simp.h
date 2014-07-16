/*
 * This file is part of buxton.
 *
 * Copyright (C) 2014 Intel Corporation
 *
 * buxton is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
	};
} vstatus;

/*TODO: add descriptions to all the function w/ parameters and return values*/
_bx_export_ void save_errno(void);
_bx_export_ void sbuxton_open(void);
_bx_export_ void sbuxton_close(void);
_bx_export_ void client_connection(void);
_bx_export_ void client_disconnect(void);
_bx_export_ void cg_cb(BuxtonResponse response, void *data);
_bx_export_ void buxtond_set_group(char *group, char *layer);
_bx_export_ void bs_print(vstatus *data, BuxtonResponse response);
_bx_export_ void bs_cb(BuxtonResponse response, void *data);
_bx_export_ void bg_cb(BuxtonResponse response, void *data);
_bx_export_ void bsi32_cb(BuxtonResponse response, void *data);
_bx_export_ void buxtond_set_int32(char *key, int32_t value);
_bx_export_ void bgi32_cb(BuxtonResponse response, void *data);
_bx_export_ int32_t buxtond_get_int32(char *key);
_bx_export_ void bss_cb(BuxtonResponse response, void *data);
_bx_export_ void buxtond_set_string(char *key, char *value );
_bx_export_ void bgs_cb(BuxtonResponse response, void *data);
_bx_export_ char* buxtond_get_string(char *key);
_bx_export_ void bsui32_cb(BuxtonResponse response, void *data);
_bx_export_ void buxtond_set_uint32(char *key, uint32_t value);
_bx_export_ void bgui32_cb(BuxtonResponse response, void *data);
_bx_export_ uint32_t buxtond_get_uint32(char *key);
_bx_export_ void bsi64_cb(BuxtonResponse response, void *data);
_bx_export_ void buxtond_set_int64(char *key, int64_t value);
_bx_export_ void bgi64_cb(BuxtonResponse response, void *data);
_bx_export_ int64_t buxtond_get_int64(char *key);
_bx_export_ void bsui64_cb(BuxtonResponse response, void *data);
_bx_export_ void buxtond_set_uint64(char *key, uint64_t value);
_bx_export_ void bgui64_cb(BuxtonResponse response, void *data);
_bx_export_ uint64_t buxtond_get_uint64(char *key);
_bx_export_ void bsf_cb(BuxtonResponse response, void *data);
_bx_export_ void buxtond_set_float(char *key, float value);
_bx_export_ void bgf_cb(BuxtonResponse response, void *data);
_bx_export_ float buxtond_get_float(char *key);
_bx_export_ void bsd_cb(BuxtonResponse response, void *data);
_bx_export_ void buxtond_set_double(char *key, double value);
_bx_export_ void bgd_cb(BuxtonResponse response, void *data);
_bx_export_ double buxtond_get_double(char *key);
_bx_export_ void bsb_cb(BuxtonResponse response, void *data);
_bx_export_ void buxtond_set_bool(char *key, bool value);
_bx_export_ void bgb_cb(BuxtonResponse response, void *data);
_bx_export_ bool buxtond_get_bool(char *key);
_bx_export_ BuxtonKey buxton_group_create(char *name, char *layer);
_bx_export_ void buxtond_create_group(BuxtonKey group);
_bx_export_ BuxtonKey buxtond_create_group2(char *group_name, char *layer);
_bx_export_ void rg_cb(BuxtonResponse response, void *data);
_bx_export_ void buxtond_remove_group(BuxtonKey group);
_bx_export_ void buxtond_remove_group2(char *group_name, char *layer);
_bx_export_ void buxtond_key_free(char * key_name, BuxtonDataType type);


