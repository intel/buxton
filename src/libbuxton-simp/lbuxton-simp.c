#include "../shared/log.h"
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "buxton-simp.h"
#include "buxton-simp-internals.h"

extern BuxtonClient client;
static char _layer[256];
static char _group[256];
extern int saved_errno;
static const size_t MAX_STRING_LENGTH=sizeof(_layer)<sizeof(_group)?sizeof(_group):sizeof(_layer);

/*Initialization of group*/
void buxtond_set_group(char *group, char *layer)
{
	_client_connection();
	_save_errno();
	/*strcpy*/
	strncpy(_layer, layer, MAX_STRING_LENGTH);
	strncpy(_group, group, MAX_STRING_LENGTH);
	BuxtonKey g = buxton_key_create(_group, NULL, _layer, STRING);
	printf("buxton key group = %s\n", buxton_key_get_group(g));
	int status;
	if (buxton_create_group(client, g, _cg_cb, &status, true)){
		printf("Create group call failed.\n");
	} else {
		printf("Switched to group: %s, layer: %s.\n", buxton_key_get_group(g),
 	buxton_key_get_layer(g));
		errno = saved_errno;
	}
	buxton_key_free(g);
	_client_disconnect();
}


void buxtond_set_int32(char *key, int32_t value)
{
	/*make sure client connection is open*/
	_client_connection();
	/*check if a key has been created*/
	/*create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT32);
	/*Return value and status*/
	vstatus ret;
	ret.type = INT32;
	ret.val.i32val = value;
	_save_errno();
	/*call buxton_set_value for type INT32*/
	if (buxton_set_value(client, _key, &value, _bs_cb, &ret, true)){
		printf("Set int32_t call failed.\n");
		return;
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

int32_t buxtond_get_int32(char *key)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT32);
	/*return value*/
	vstatus ret;
	ret.type = INT32;
	_save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)){
		printf("Get int32_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.i32val;
}

void buxtond_set_string(char *key, char *value )
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, STRING);
	/*Return value and status*/
	vstatus ret;
	ret.type = STRING;
	ret.val.sval = value;
	_save_errno();
	/*set value*/
	if (buxton_set_value(client, _key, &value, _bs_cb, &ret, true)){
		printf("Set string call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

char* buxtond_get_string(char *key)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, STRING);
	/*return value*/
	vstatus ret;
	ret.type = STRING;
	_save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)){
		printf("Get string call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.sval;
}

void buxtond_set_uint32(char *key, uint32_t value)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT32);
	/*Return value and status*/
	vstatus ret;
	ret.type = UINT32;
	ret.val.ui32val = value;
	_save_errno();
	if (buxton_set_value(client,_key, &value, _bs_cb, &ret, true)){
		printf("Set uint32_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

uint32_t buxtond_get_uint32(char *key)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT32);
	/*return value*/
	vstatus ret;
	ret.type = UINT32;
	_save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)){
		printf("Get uint32_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.ui32val;
}

void buxtond_set_int64(char *key, int64_t value)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT64);
	/*Return value and status*/
	vstatus ret;
	ret.type = INT64;
	ret.val.i64val = value;
	_save_errno();
	if(buxton_set_value(client, _key, &value, _bs_cb, &ret, true)){
		printf("Set int64_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

int64_t buxtond_get_int64(char *key)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT64);
	/*return value*/
	vstatus ret;
	ret.type = INT64;
	_save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)){
		printf("Get int64_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.i64val;
}

void buxtond_set_uint64(char *key, uint64_t value)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT64);
	/*Return value and status*/
	vstatus ret;
	ret.type = UINT64;
	ret.val.ui64val = value;
	_save_errno();
	if(buxton_set_value(client, _key, &value, _bs_cb, &ret, true)){
		printf("Set uint64_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

uint64_t buxtond_get_uint64(char *key)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT64);
	/*return value*/
	vstatus ret;
	ret.type = UINT64;
	_save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)){
		printf("Get uint64_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.ui64val;
}

void buxtond_set_float(char *key, float value)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, FLOAT);
	/*Return value and status*/
	vstatus ret;
	ret.type = FLOAT;
	ret.val.fval = value;
	_save_errno();
	if(buxton_set_value(client, _key, &value, _bs_cb, &ret, true)){
		printf("Set float call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

float buxtond_get_float(char *key)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, FLOAT);
	/*return value*/
	vstatus ret;
	ret.type = FLOAT;
	_save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)){
		printf("Get float call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.fval;
}

void buxtond_set_double(char *key, double value)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, DOUBLE);
	/*Return value and status*/
	vstatus ret;
	ret.type = DOUBLE;
	ret.val.dval = value;
	_save_errno();
	if(buxton_set_value(client, _key, &value, _bs_cb, &ret, true)){
		printf("Set double call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

double buxtond_get_double(char *key)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, DOUBLE);
	/*return value*/
	vstatus ret;
	ret.type = DOUBLE;
	_save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)){
		printf("Get double call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.dval;
}

void buxtond_set_bool(char *key, bool value)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, BOOLEAN);
	/*Return value and status*/
	vstatus ret;
	ret.type = BOOLEAN;
	ret.val.bval = value;
	_save_errno();
	if(buxton_set_value(client, _key, &value, _bs_cb, &ret, true)){
		printf("Set bool call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
}

bool buxtond_get_bool(char *key)
{
	/*make sure client connection is open*/
	_client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, BOOLEAN);
	/*return value*/
	vstatus ret;
	ret.type = BOOLEAN;
	_save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, _bg_cb, &ret, true)){
		printf("Get bool call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	_client_disconnect();
	return ret.val.bval;
}

void buxtond_remove_group(char *group_name, char *layer)
{
	_client_connection();
	BuxtonKey group = _buxton_group_create(group_name, layer);
	if (buxton_remove_group(client, group, _rg_cb, NULL, true)){
		printf("Remove group call failed.\n");
	}
	buxton_key_free(group);
	_client_disconnect();
}

