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
 * \file lbuxton-simp.h Buxton public header
 *
 *
 * \copyright Copyright (C) 2013 Intel corporation
 * \par License
 * GNU Lesser General Public License 2.1
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
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
void save_errno(void);
void sbuxton_open(void);
void sbuxton_close(void);
void client_connection(void);
void client_disconnect(void);
void cg_cb(BuxtonResponse response, void *data);
void buxtond_set_group(char *group, char *layer);
void bs_print(vstatus *data, BuxtonResponse response);
void bs_cb(BuxtonResponse response, void *data);
void bg_cb(BuxtonResponse response, void *data);
void bsi32_cb(BuxtonResponse response, void *data);
void buxtond_set_int32(char *key, int32_t value);
void bgi32_cb(BuxtonResponse response, void *data);
int32_t buxtond_get_int32(char *key);
void bss_cb(BuxtonResponse response, void *data);
void buxtond_set_string(char *key, char *value );
void bgs_cb(BuxtonResponse response, void *data);
char* buxtond_get_string(char *key);
void bsui32_cb(BuxtonResponse response, void *data);
void buxtond_set_uint32(char *key, uint32_t value);
void bgui32_cb(BuxtonResponse response, void *data);
uint32_t buxtond_get_uint32(char *key);
void bsi64_cb(BuxtonResponse response, void *data);
void buxtond_set_int64(char *key, int64_t value);
void bgi64_cb(BuxtonResponse response, void *data);
int64_t buxtond_get_int64(char *key);
void bsui64_cb(BuxtonResponse response, void *data);
void buxtond_set_uint64(char *key, uint64_t value);
void bgui64_cb(BuxtonResponse response, void *data);
uint64_t buxtond_get_uint64(char *key);
void bsf_cb(BuxtonResponse response, void *data);
void buxtond_set_float(char *key, float value);
void bgf_cb(BuxtonResponse response, void *data);
float buxtond_get_float(char *key);
void bsd_cb(BuxtonResponse response, void *data);
void buxtond_set_double(char *key, double value);
void bgd_cb(BuxtonResponse response, void *data);
double buxtond_get_double(char *key);
void bsb_cb(BuxtonResponse response, void *data);
void buxtond_set_bool(char *key, bool value);
void bgb_cb(BuxtonResponse response, void *data);
bool buxtond_get_bool(char *key);
BuxtonKey buxton_group_create(char *name, char *layer);
void buxtond_create_group(BuxtonKey group);
BuxtonKey buxtond_create_group2(char *group_name, char *layer);
void rg_cb(BuxtonResponse response, void *data);
void buxtond_remove_group(BuxtonKey group);
void buxtond_remove_group2(char *group_name, char *layer);
void buxtond_key_free(char * key_name, BuxtonDataType type);


