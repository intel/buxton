#include "../shared/log.h"
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "buxton.h"
#include "buxton-simp.h"

static BuxtonClient client=NULL;
static char _layer[256];
static char _group[256];
static const size_t MAX_STRING_LENGTH=sizeof(_layer)<sizeof(_group)?sizeof(_group):sizeof(_layer);

/*Save errno*/
int saved_errno;

void save_errno(void)
{
	saved_errno = errno;
}

/*Open client connection*/
void sbuxton_open(void)
{
	if ((buxton_open(&client)) <0 ){
		printf("Couldn't connect.\n");
		return;
	}
	printf("Connection successful.\n");
}

/*Close client connection*/
void sbuxton_close(void)
{ 
	buxton_close(client);
	printf("Connection closed\n");
}

/*Make sure client connection is open*/
void client_connection(void)
{
	/*Check if client connection is open*/
	if(!client){
		/*Open connection if needed*/
		sbuxton_open();
	}
}

/*Close an open client connection*/
void client_disconnect(void)
{
	/*Only attempt to close the client if it != NULL*/
	if(client) {
		/*Close the connection*/
		sbuxton_close();
		client = NULL;
	}
}

/*Create group callback*/
void cg_cb(BuxtonResponse response, void *data)
{
	int *status = (int *)data;
	*status = 0;
	if (buxton_response_status(response) != 0){
		printf("Failed to create group.\n");
	} else {
		printf("Created group.\n");
		*status = 1;
	}
}

/*Initialization of group*/
void buxtond_set_group(char *group, char *layer)
{
	client_connection();
	save_errno();
	/*strcpy*/
	strncpy(_layer, layer, MAX_STRING_LENGTH);
	strncpy(_group, group, MAX_STRING_LENGTH);
	BuxtonKey g = buxton_key_create(_group, NULL, _layer, STRING);
	printf("buxton key group = %s\n", buxton_key_get_group(g));
	int status;
	if (buxton_create_group(client, g, cg_cb, &status, true)){
		printf("Create group call failed.\n");
	} else {
		printf("Switched to group: %s, layer: %s.\n", buxton_key_get_group(g),
 	buxton_key_get_layer(g));
		errno = saved_errno;
	}
	buxton_key_free(g);
	client_disconnect();
}

void bs_print(vstatus *data, BuxtonResponse response)
{
	switch(data->type){
		case STRING:
		{
			char * val = data->val.sval;
			printf("Success: value has been set: %s(string). ", val);
			break;
		}
		case INT32:
		{
			int32_t val = data->val.i32val;
			printf("Success: value has been set: %d(int32_t). ", val);
			break;
		}
		case UINT32:
		{
			uint32_t val = data->val.ui32val;
			printf("Success: value has been set: %d(uint32_t). ", val);
			break;
		}
		case INT64:
		{
			int64_t val = data->val.i64val;
			printf("Success: value has been set: ""%"PRId64"(int64_t). ", val);
			break;
		}
		case UINT64:
		{
			uint64_t val = data->val.ui64val;
			printf("Success: value has been set: ""%"PRIu64"(uint64_t). ", val);
			break;
		}
		case FLOAT:
		{
			float val = data->val.fval;
			printf("Success: value has been set: %f(float). ", val);
			break;
		}
		case DOUBLE:
		{
			double val = data->val.dval;
			printf("Success: value has been set: %e(double). ", val);
			break;
		}
		case BOOLEAN:
		{
			bool val = data->val.bval;
			printf("Success: value has been set: %i(bool). ", val);
			break;
		}
		default:
		{
			printf("Data type not found\n");
			break;
		}
	}
	BuxtonKey k = buxton_response_key(response);
	printf("Key: %s, Group: %s, Layer: %s.\n", buxton_key_get_name(k), buxton_key_get_group(k), buxton_key_get_layer(k));
	buxton_key_free(k);	
}

/*buxton_set_value callback for all buxton data types*/
void bs_cb(BuxtonResponse response, void *data){
	vstatus *ret = (vstatus*)data;
	ret->status = 0;
	/*check response before switch TODO: separate function*/
	if (buxton_response_status(response)){
		printf("Failed to set value.\n");
		return;
	}
	ret->status =1;
	bs_print(ret, response);

}

void bg_cb(BuxtonResponse response, void *data)
{
	char *type;
	vstatus *ret= (vstatus *)data;
	switch (ret->type){
		case STRING:
		{
			ret->val.sval = *(char**)buxton_response_value(response);
			type = "string";
			break;
		}
		case INT32:
		{
			ret->val.i32val = *(int32_t*)buxton_response_value(response);
			type = "int32_t";
			break;
		}
		case UINT32:
		{
			ret->val.ui32val = *(uint32_t*)buxton_response_value(response);
			type = "uint32_t";
			break;
		}
		case INT64:
		{
			ret->val.i64val = *(int64_t*)buxton_response_value(response);
			type = "int64_t";
			break;
		}
		case UINT64:
		{
			ret->val.ui64val = *(uint64_t*)buxton_response_value(response);
			type = "uint64_t";
			break;
		}
		case FLOAT:
		{
			ret->val.fval = *(float*)buxton_response_value(response);
			type = "float";
			break;
		}
		case DOUBLE:
		{
			ret->val.dval = *(double*)buxton_response_value(response);
			type = "double";
			break;
		}
		case BOOLEAN:
		{
			ret->val.bval = *(bool*)buxton_response_value(response);
			type = "bool";
			break;
		}
		default:
		{
			type = "unknown";
			break;
		}
	}
	ret->status = 0;
	if (buxton_response_status(response)){
		printf("Failed to get %s. \n", type);
		return;
	}
	ret->status = 1;
}

void buxtond_set_int32(char *key, int32_t value)
{
	/*make sure client connection is open*/
	client_connection();
	/*check if a key has been created*/
	/*create key */
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT32);
	/*Return value and status*/
	vstatus ret;
	ret.type = INT32;
	ret.val.i32val = value;
	save_errno();
	/*call buxton_set_value for type INT32*/
	if (buxton_set_value(client, _key, &value, bs_cb, &ret, true)){
		printf("Set int32_t call failed.\n");
		return;
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
}

int32_t buxtond_get_int32(char *key)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT32);
	/*return value*/
	vstatus ret;
	ret.type = INT32;
	save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, bg_cb, &ret, true)){
		printf("Get int32_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
	return ret.val.i32val;
}

void buxtond_set_string(char *key, char *value )
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, STRING);
	/*Return value and status*/
	vstatus ret;
	ret.type = STRING;
	ret.val.sval = value;
	save_errno();
	/*set value*/
	if (buxton_set_value(client, _key, &value, bs_cb, &ret, true)){
		printf("Set string call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
}

char* buxtond_get_string(char *key)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, STRING);
	/*return value*/
	vstatus ret;
	ret.type = STRING;
	save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, bg_cb, &ret, true)){
		printf("Get string call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
	return ret.val.sval;
}

void buxtond_set_uint32(char *key, uint32_t value)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT32);
	/*Return value and status*/
	vstatus ret;
	ret.type = UINT32;
	ret.val.ui32val = value;
	save_errno();
	if (buxton_set_value(client,_key, &value, bs_cb, &ret, true)){
		printf("Set uint32_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
}

uint32_t buxtond_get_uint32(char *key)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT32);
	/*return value*/
	vstatus ret;
	ret.type = UINT32;
	save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, bg_cb, &ret, true)){
		printf("Get uint32_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
	return ret.val.ui32val;
}

void buxtond_set_int64(char *key, int64_t value)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT64);
	/*Return value and status*/
	vstatus ret;
	ret.type = INT64;
	ret.val.i64val = value;
	save_errno();
	if(buxton_set_value(client, _key, &value, bs_cb, &ret, true)){
		printf("Set int64_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
}

int64_t buxtond_get_int64(char *key)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, INT64);
	/*return value*/
	vstatus ret;
	ret.type = INT64;
	save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, bg_cb, &ret, true)){
		printf("Get int64_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
	return ret.val.i64val;
}

void buxtond_set_uint64(char *key, uint64_t value)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT64);
	/*Return value and status*/
	vstatus ret;
	ret.type = UINT64;
	ret.val.ui64val = value;
	save_errno();
	if(buxton_set_value(client, _key, &value, bs_cb, &ret, true)){
		printf("Set uint64_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
}

uint64_t buxtond_get_uint64(char *key)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, UINT64);
	/*return value*/
	vstatus ret;
	ret.type = UINT64;
	save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, bg_cb, &ret, true)){
		printf("Get uint64_t call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
	return ret.val.ui64val;
}

void buxtond_set_float(char *key, float value)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, FLOAT);
	/*Return value and status*/
	vstatus ret;
	ret.type = FLOAT;
	ret.val.fval = value;
	save_errno();
	if(buxton_set_value(client, _key, &value, bs_cb, &ret, true)){
		printf("Set float call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
}

float buxtond_get_float(char *key)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, FLOAT);
	/*return value*/
	vstatus ret;
	ret.type = FLOAT;
	save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, bg_cb, &ret, true)){
		printf("Get float call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
	return ret.val.fval;
}

void buxtond_set_double(char *key, double value)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, DOUBLE);
	/*Return value and status*/
	vstatus ret;
	ret.type = DOUBLE;
	ret.val.dval = value;
	save_errno();
	if(buxton_set_value(client, _key, &value, bs_cb, &ret, true)){
		printf("Set double call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
}

double buxtond_get_double(char *key)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, DOUBLE);
	/*return value*/
	vstatus ret;
	ret.type = DOUBLE;
	save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, bg_cb, &ret, true)){
		printf("Get double call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
	return ret.val.dval;
}

void buxtond_set_bool(char *key, bool value)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, BOOLEAN);
	/*Return value and status*/
	vstatus ret;
	ret.type = BOOLEAN;
	ret.val.bval = value;
	save_errno();
	if(buxton_set_value(client, _key, &value, bs_cb, &ret, true)){
		printf("Set bool call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
}

bool buxtond_get_bool(char *key)
{
	/*make sure client connection is open*/
	client_connection();
	/*create key*/
	BuxtonKey _key = buxton_key_create(_group, key, _layer, BOOLEAN);
	/*return value*/
	vstatus ret;
	ret.type = BOOLEAN;
	save_errno();
	/*get value*/
	if (buxton_get_value(client, _key, bg_cb, &ret, true)){
		printf("Get bool call failed.\n");
	}
	if (!ret.status){
		errno = EACCES;
	} else {
		errno = saved_errno;
	}
	buxton_key_free(_key);
	client_disconnect();
	return ret.val.bval;
}

/*create a client side group TODO: create BuxtonGroup type
	probably not really needed*/
BuxtonKey buxton_group_create(char *name, char *layer)
{
	/* Only helper function- not called outside the library, so doesn't need
	to connect/disconnect
	client_connection();*/
	BuxtonKey ret = buxton_key_create(name, NULL, layer, STRING);
	/*client_disconnect();*/
	return ret;
}

/*create a buxtond side group*/
void buxtond_create_group(BuxtonKey group)
{
	client_connection();
	if (buxton_create_group(client, group, NULL, NULL, true)){
		printf("Create group call failed.\n");
		buxton_key_free(group);
		client_disconnect();
		return;
	}
	/*TODO*/
	//buxton_key_free(group);
	client_disconnect();
}

/*buxton_remove_group callback and function*/
void rg_cb(BuxtonResponse response, void *data)
{
	if (buxton_response_status(response) != 0){
		printf("Failed to remove group.\n");
	} else {
		printf("Removed group.\n");
	}
}

void buxtond_remove_group(char *group_name, char *layer)
{
	client_connection();
	BuxtonKey group = buxton_group_create(group_name, layer);
	if (buxton_remove_group(client, group, rg_cb, NULL, true)){
		printf("Remove group call failed.\n");
	}
	buxton_key_free(group);
	client_disconnect();
}

/*buxton_key_free insert char key name and type*/
void buxtond_key_free(char * key_name, BuxtonDataType type)
{
	BuxtonKey k = buxton_key_create(_group, strdup(key_name), _layer, type);
	buxton_key_free(k);
}
