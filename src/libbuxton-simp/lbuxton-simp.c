#include <poll.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "buxton.h"

static BuxtonClient client=NULL;
static int fd =-1;
static struct pollfd pfd[1];
static int r;
/*Open client connection*/
void sbuxton_open(void)
{
	if ((fd= buxton_open(&client)) <0 ){
		printf("Couldn't connect.\n");
		return;
	}
	printf("Connection successful.\n");
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
/*Public poll timeout setter*/
/*Polling*/
void run_poll(int timeout)
{
	pfd[0].fd = fd;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;
	r = poll(pfd, 1, timeout);
	
	if (r <= 0){
		printf("poll error\n");
		return;
	}
	if (!buxton_client_handle_response(client)){
		printf("bad response from daemon\n");
		return;
	}
}
/*buxton_set_value callback and function for int32_t*/
void bsi32_cb(BuxtonResponse response, void *data)
{
	int32_t *ret=(int32_t*)data;
	if (buxton_response_status(response)){
		printf("Failed to set int32_t.\n");
		return;
	}
	printf("Value has been set: %i(int32_t)\n", *ret);
}
void buxtond_set_int32(BuxtonKey key, int32_t value)
{
	/*make sure client connection is open*/
	client_connection();
	/*check if a key has been created*/
	/*create a key if needed*/
	/*call buxton_set_value for type INT32i*/
	if (buxton_set_value(client, key, &value, bsi32_cb, &value, true)){
		printf("Set int32_t call failed.\n");
	}
}
/*buxton_set_value callback and function for string*/
void bss_cb(BuxtonResponse response, void *data)
{
	char **ret=(char**)data;
	if (buxton_response_status(response)){
		printf("Failed to set string.\n");
		return;
	}
	printf("Value has been set: %s(string)\n", *ret);
}
void buxtond_set_string(BuxtonKey key, char *value )
{
	/*make sure client connection is open*/
	client_connection();
	if (buxton_set_value(client, key, &value, bss_cb, &value, true)){
		printf("Set string call failed.\n");
	}
}
/*buxton_set_value callback and function for uint32_t*/
void bsui32_cb(BuxtonResponse response, void *data)
{
	uint32_t *ret=(uint32_t*)data;
	if (buxton_response_status(response)){
		printf("Failed to set uint32_t.\n");
		return;
	}
	printf("Value has been set: %i(uint32_t)\n", *ret);
}
void buxtond_set_uint32(BuxtonKey key, uint32_t value)
{
	/*make sure client connection is open*/
	client_connection();
	if (buxton_set_value(client, key, &value, bsui32_cb, &value, true)){
		printf("Set uint32_t call failed.\n");
	}
}
/*buxton_set_value callback and function for int64_t*/
void bsi64_cb(BuxtonResponse response, void *data)
{
	int64_t *ret = (int64_t*)data;
	if (buxton_response_status(response)){
		printf("Failed to set int64_t.\n");
		return;
	}
	printf("Value has been set: ""%"PRId64"(int64_t)\n", *ret);
}
void buxtond_set_int64(BuxtonKey key, int64_t value)
{
	/*make sure client connection is open*/
	client_connection();
	if(buxton_set_value(client, key, &value, bsi64_cb, &value, true)){
		printf("Set int64_t call failed.\n");
	}
}
/*buxton_set_value callback and function for uint64_t*/
void bsui64_cb(BuxtonResponse response, void *data)
{
	uint64_t *ret = (uint64_t*)data;
	if (buxton_response_status(response)){
		printf("Failed to set uint64_t.\n");
		return;
	}
	printf("Value has been set: ""%"PRIu64"(uint64_t)\n", *ret);
}
void buxtond_set_uint64(BuxtonKey key, uint64_t value)
{
	/*make sure client connection is open*/
	client_connection();
	if(buxton_set_value(client, key, &value, bsui64_cb, &value, true)){
		printf("Set uint64_t call failed.\n");
	}
}
/*buxton_set_value callback and function for float*/
void bsf_cb(BuxtonResponse response, void *data)
{
	float *ret = (float*)data;
	if (buxton_response_status(response)){
		printf("Failed to set float.\n");
		return;
	}
	printf("Value has been set: %f(float)\n", *ret);
}
void buxtond_set_float(BuxtonKey key, float value)
{
	/*make sure client connection is open*/
	client_connection();
	if(buxton_set_value(client, key, &value, bsf_cb, &value, true)){
		printf("Set float call failed.\n");
	}
}
/*buxton_set_value callback and function for double*/
void bsd_cb(BuxtonResponse response, void *data)
{
	double *ret = (double*)data;
	if (buxton_response_status(response)){
		printf("Failed to set double.\n");
		return;
	}
	printf("Value has been set: %e(double)\n", *ret);
}
void buxtond_set_double(BuxtonKey key, double value)
{
	/*make sure client connection is open*/
	client_connection();
	if(buxton_set_value(client, key, &value, bsd_cb, &value, true)){
		printf("Set double call failed.\n");
	}
}
/*buxton_set_value callback and function for boolean*/
void bsb_cb(BuxtonResponse response, void *data)
{
	bool *ret = (bool*)data;
	if(buxton_response_status(response)){
		printf("Failed to set boolean.\n");
		return;
	}
	printf("Value has been set: %i(bool)\n", *ret);
}
void buxtond_set_bool(BuxtonKey key, bool value)
{
	/*make sure client connection is open*/
	client_connection();
	if(buxton_set_value(client, key, &value, bsb_cb, &value, true)){
		printf("Set bool call failed.\n");
	}
}
/*create a client side group TODO: create BuxtonGroup type*/
BuxtonKey buxton_group_create(char *name, char *layer)
{
	client_connection();
	BuxtonKey ret = buxton_key_create(name, NULL, layer, STRING);
	return ret;
}
/*create a buxtond side group*/
void buxtond_create_group(BuxtonKey group)
{
	client_connection();
	if (buxton_create_group(client, group, NULL, NULL, true)){
		printf("Create group call failed.\n");
		buxton_key_free(group);
		return;
	}
	/*TODO*/
	//buxton_key_free(group);
}
BuxtonKey buxtond_create_group2(char *group_name, char *layer)
{
	client_connection();
	BuxtonKey group = buxton_key_create(group_name, NULL, layer, STRING);
	if (buxton_create_group(client, group, NULL, NULL, true)){
		printf("Create group call failed.\n");
		buxton_key_free(group);
		return;
	}
	return group;
	/*Buxton free key? TODO*/
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
void buxtond_remove_group(BuxtonKey group)
{
	client_connection();
	if (buxton_remove_group(client, group, rg_cb, NULL, true)){
		printf("Remove group call failed.\n");
	}
}
void buxtond_remove_group2(char *group_name, char *layer)
{
	client_connection();
	BuxtonKey group = buxton_group_create(group_name, layer);
	if (buxton_remove_group(client, group, rg_cb, NULL, true)){
		printf("Remove group call failed.\n");
	}
}
/*buxton_get_value callback and function*/

/*buxton_notifications*/
void buxtond_register_notification(BuxtonKey key)
{
	if (buxton_register_notificatio(client, key, brn_cb, &status, true)){
		printf("Buxton register notification call failed.\n");
		return;
	}
	//run_poll();
}
/*DEMONSTRATION*/
int main(void)
{
	/*Create group*/
	BuxtonKey sGroup = buxtond_create_group2("tg_s1", "user");
	//buxtond_create_group(sGroup);
	/*Test string setting*/
	char * s="Watermelon";
	printf("Key should be set to %s.\n", s);
	BuxtonKey sKey = buxton_key_create("tg_s1", "tk_s1", "user", STRING);
	buxtond_set_string(sKey, s);
	buxton_key_free(sKey);
	/*Test int32 setting*/
	srand(time(NULL));
	int32_t i=rand()%100+1;
	printf("Key should be set to %i.\n",i);
	BuxtonKey i32key = buxton_key_create("tg_s1", "tk_i32", "user", INT32);
	buxtond_set_int32(i32key, i);
	buxton_key_free(i32key);
	/*Test uint32 setting*/
	uint32_t ui32 = rand()%50+1;
	printf("Key should be set to %u.\n", ui32);
	BuxtonKey ui32key = buxton_key_create("tg_s1", "tk_ui32", "user", UINT32);
	buxtond_set_uint32(ui32key, ui32);
	buxton_key_free(ui32key);
	/*Test  int64 setting*/
	int64_t i64 = rand()%1000+1;
	printf("Key should be set to ""%"PRId64".\n", i64);
	BuxtonKey i64key = buxton_key_create("tg_s1", "tk_i64", "user", INT64);
	buxtond_set_int64(i64key, i64);
	buxton_key_free(i64key);
	/*Test uint64 setting*/
	uint64_t ui64 = rand()%500+1;
	printf("Key should be set to ""%"PRIu64".\n", ui64);
	BuxtonKey ui64key = buxton_key_create("tg_s1", "tk_ui64", "user", UINT64);
	buxtond_set_uint64(ui64key, ui64);
	buxton_key_free(ui64key);
	/*Test float setting*/
	float f = rand()%9+1;
	printf("Key should be set to %e.\n", f);
	BuxtonKey fkey = buxton_key_create("tg_s1", "tk_f", "user", FLOAT);
	buxtond_set_float(fkey, f);
	buxton_key_free(fkey);
	/*Test double setting*/
	double d = rand()%7000+1;
	printf("Key should be set to %e.\n", d);
	BuxtonKey dkey = buxton_key_create("tg_s1", "tk_d", "user", DOUBLE);
	buxtond_set_double(dkey, d);
	buxton_key_free(dkey);
	/*Test boolean setting*/
	bool b = true;
	printf("Key should be set to %i.\n", b);
	BuxtonKey bkey = buxton_key_create("tg_s1", "tk_b", "user", BOOLEAN);
	buxtond_set_bool(bkey, b);
	buxton_key_free(bkey);
	/*Close connection*/
	buxtond_remove_group2("tg_s1", "user");
	buxton_close(client);
}
