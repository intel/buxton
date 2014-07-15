#include "../shared/log.h"
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "buxton.h"
#include "lbuxton-simp.h"

/*DEMONSTRATION*/
int main(void)
{
	/*Create group*/
	errno = 0;
	buxtond_set_group("tg_s0", "user");
//	printf("set_group: 'tg_s0', 'user', Error number: %s.\n", strerror(errno));
	/*Test string setting*/
	srand(time(NULL));
	char * s="Watermelon";
	printf("value should be set to %s.\n", s);
	errno = 0;
	buxtond_set_string("tk_s1", s);
//	printf("set_string: 'tg_s0', 'tk_s1', Error number: %s.\n", strerror(errno));
	/*Test string getting*/
	char * sv = buxtond_get_string("tk_s1");
	printf("Got value: %s(string).\n", sv);		
//	printf("get_string: 'tk_s1', Error number: %s.\n", strerror(errno));
	/*Create group*/
	errno = 0;
	buxtond_set_group("tg_s1", "user");
//	printf("set_group: 'tg_s1', Error number: %s.\n", strerror(errno));
	/*Test int32 setting*/
	srand(time(NULL));
	int32_t i=rand()%100+1;
	printf("value should be set to %i.\n",i);
	errno = 0;
	buxtond_set_int32("tk_i32", i);
//	printf("set_int32: 'tg_s1', 'tk_i32', Error number: %s.\n", strerror(errno));

	errno = 0;
	buxtond_set_group("tg_s2", "user");
//	printf("set_group: 'tg_s2', Error number: %s.\n", strerror(errno));
	srand(time(NULL));
	int32_t i2=rand()%1000+1;
	printf("Second value should be set to %i.\n", i2);
	errno = 0;
	buxtond_set_int32("tk_i32b", i2);
//	printf("set_int32: 'tg_s2', 'tk_i32b', Error number: %s.\n", strerror(errno));
	/*Test int32 getting*/
	errno = 0;
	buxtond_set_group("tg_s1", "user");
//	printf("set_group: 'tg_s1', Error number: %s.\n", strerror(errno));
	errno = 0;
	int32_t iv = buxtond_get_int32("tk_i32");
//	printf("get_int32: 'tg_s1', 'tk_i32', Error number: %s.\n", strerror(errno));
	printf("Got value: %i(int32_t).\n", iv);
	errno = 0;
	buxtond_set_group("tg_s2", "user");
//	printf("set_group: 'tg_s2', Error number: %s.\n", strerror(errno));
	errno = 0;
	int32_t i2v = buxtond_get_int32("tk_i32b");
	printf("Got value: %i(int32_t).\n", i2v);
//	printf("get_int32: 'tg_s2', 'tk_i32b', Error number: %s.\n", strerror(errno));
	/*Test uint32 setting*/
	errno = 0;
	buxtond_set_group("tg_s3", "user");
//	printf("set_group: 'tg_s3', Error number: %s.\n", strerror(errno));
	uint32_t ui32 = rand()%50+1;
	printf("value should be set to %u.\n", ui32);
	errno = 0;
	buxtond_set_uint32("tk_ui32", ui32);
//	printf("set_uint32: 'tg_s3', 'tk_ui32', Error number: %s.\n", strerror(errno));
	/*Test uint32 getting*/
	errno = 0;
	uint32_t ui32v = buxtond_get_uint32("tk_ui32");
	printf("Got value: %i(uint32_t).\n", ui32v);
//	printf("get_uint32: 'tg_s3', 'tk_ui32', Error number: %s.\n", strerror(errno));
	/*Test  int64 setting*/
	int64_t i64 = rand()%1000+1;
	printf("value should be set to ""%"PRId64".\n", i64);
	errno = 0;
	buxtond_set_int64("tk_i64", i64);
	/*Test int64 getting*/
	errno = 0;
	int64_t i64v = buxtond_get_int64("tk_i64");
	printf("Got value: ""%"PRId64"(int64_t).\n", i64v);
//	printf("get_int64: 'tg_s3', 'tk_i64', Error number: %s.\n", strerror(errno));
	/*Test uint64 setting*/
	errno = 0;
	buxtond_set_group("tg_s0", "user");
	uint64_t ui64 = rand()%500+1;
	printf("value should be set to ""%"PRIu64".\n", ui64);
	errno = 0;
	buxtond_set_uint64("tk_ui64", ui64);
	/*Test uint64 getting*/
	errno = 0;
	uint64_t ui64v = buxtond_get_uint64("tk_ui64");
	printf("Got value: ""%"PRIu64"(uint64_t).\n", ui64v);
//	printf("get_uint64: 'tg_s0', 'tk_ui64', Error number: %s.\n", strerror(errno));
	/*Test float setting*/
	float f = rand()%9+1;
	printf("value should be set to %e.\n", f);
	errno = 0;
	buxtond_set_float("tk_f", f);
	/*Test float getting*/
	errno = 0;
	float fv = buxtond_get_float("tk_f");
	printf("Got value: %e(float).\n", fv);
//	printf("get_float: 'tg_s0', 'tk_f', Error number: %s.\n", strerror(errno));
	/*Test double setting*/
	double d = rand()%7000+1;
	printf("value should be set to %e.\n", d);
	errno = 0;
	buxtond_set_double("tk_d", d);
	/*Test double getting*/
	errno = 0;
	double dv = buxtond_get_double("tk_d");
	printf("Got value: %e(double).\n", dv);
//	printf("get_double: 'tg_s0', 'tk_f', Error number: %s.\n", strerror(errno));
	/*Test boolean setting*/
	bool b = true;
	printf("value should be set to %i.\n", b);
	errno = 0;
	buxtond_set_bool("tk_b", b);
	/*Test boolean getting*/
	errno = 0;
	bool bv = buxtond_get_bool("tk_b");
	printf("Got value: %i(bool).\n", bv);		
//	printf("get_bool: 'tg_s0', 'tk_b', Error number: %s.\n", strerror(errno));
	/*Close connection*/
	errno = 0;
	buxtond_remove_group2("tg_s1", "user");
	printf("Remove group: 'tg_s1', 'user' Error number: %s.\n", strerror(errno));
	buxtond_remove_group2("tg_s0", "user");
	buxtond_remove_group2("tg_s2", "user");
	buxtond_remove_group2("tg_s3", "user");

	return 0;
}
