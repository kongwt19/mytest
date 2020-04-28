/******************************************************************************

版权所有 (C), 1958-2015, 长虹软服中心-云服-数字家庭

******************************************************************************
文 件 名   : ipp_protocol.c
版 本 号   : 初稿
作    者   : 陈梁 20167633
生成日期   : 2015年10月13日
最近修改   :
功能描述   : ipp协议组装类
函数列表   : 略
修改历史   :
1.日    期   : 2015年10月13日
作    者   : 陈梁 20167633
修改内容   : 创建文件
******************************************************************************/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "ipp_defines.h"
#include "ipp_protocol.h"
#include "ipp_protocol_helper.h"

#define PUT_INT_BUFFER_NUMBER_ARRAY(numberType, putFunc)\
int i = 0;\
int32_t error_code = put_int32(protocol, array_size);\
if (protocol_success != error_code) {\
	return error_code;\
}\
\
for (;i < array_size; ++i)\
{\
	error_code = putFunc(protocol, array[i]);\
	if (protocol_success != error_code)\
	{\
		return error_code;\
	}\
}\
\
return protocol_success


#define GET_FROM_BUFFER_NUMBER(numberType, transFunc) \
numberType temp;\
int32_t error_code = get_raw(protocol, &temp, sizeof(numberType));\
if (protocol_success != error_code) {\
	return error_code;\
}\
\
*data = transFunc(temp);\
return protocol_success

#define GET_FROM_BUFFER_NUMBER_ARRAY(numberType, getFunc) \
int i = 0;\
int32_t data_size = sizeof(numberType);\
int32_t error_code = get_int32(protocol, array_size);\
if (protocol_success != error_code) {\
	return error_code;\
}\
\
error_code = check_protocol_size(protocol, *array_size);\
if (protocol_success != error_code) {\
	return error_code;\
}\
*array = (numberType*)malloc(data_size * (*array_size));\
if (NULL == (*array)) {\
	ipp_LogE_Prefix("malloc:%s(%d)-%s\r\n",__FILE__,__LINE__,__FUNCTION__);\
	return protocol_alloc_failed;\
}\
\
for (i = 0; i < (*array_size); ++i) {\
	error_code = getFunc(protocol, *array + i);\
	if (protocol_success != error_code) {\
		return error_code;\
	}\
}\
\
return protocol_success

//-----------------------------------------内部函数-----------------------------------------//
// 增加Buffer
int32_t increase_protocol(ipp_protocol* protocol, int32_t increase_size) {
	char *pTemp = NULL;
	int32_t new_len = 0;
	if (protocol->buffer_size >= protocol->buffer_index + increase_size)
		return protocol_success;

	if (increase_rule_double == protocol->increase_rule) {
		new_len = (protocol->buffer_size * 2 >= protocol->buffer_index + increase_size) ?
			protocol->buffer_size * 2 : protocol->buffer_index + increase_size;
	} else {
		new_len = protocol->buffer_index + increase_size;
	}
	pTemp = (char*)malloc(new_len);

	if (NULL == pTemp) {
		ipp_LogE_Prefix("malloc:%s(%d)-%s\r\n",__FILE__,__LINE__,__FUNCTION__);
		return protocol_alloc_failed;
	}

	memcpy(pTemp, protocol->buffer, protocol->buffer_size);
	FREE_POINTER(protocol->buffer);
	protocol->buffer = pTemp;
	protocol->buffer_size = new_len;

	return protocol_success;
}

int32_t put_array(ipp_protocol* protocol, void* array, int32_t ele_size, int32_t array_size) {
	int32_t error_code = put_int32(protocol, array_size);
	if (protocol_success != error_code) {
		return error_code;
	}

	return put_raw(protocol, array, ele_size * array_size);
}

int32_t get_array(ipp_protocol* protocol, void** array, int32_t ele_size, int32_t* array_size) {
	int32_t error_code = get_int32(protocol, array_size);
	if (protocol_success != error_code) {
		return error_code;
	}

	if (*array_size < 0)
		return protocol_buffer_size_error;

	if (0 == *array_size)
		return protocol_success;

	error_code = check_protocol_size(protocol, *array_size);
	if (protocol_success != error_code) {
		return error_code;
	}

	*array = (char*)malloc(ele_size * (*array_size));
	if (NULL == (*array)) {
		ipp_LogE_Prefix("malloc:%s(%d)-%s\r\n",__FILE__,__LINE__,__FUNCTION__);
		return protocol_alloc_failed;
	}

	error_code = get_raw(protocol, *array, ele_size * (*array_size));
	if (protocol_success != error_code) {
		return error_code;
	}

	return protocol_success;
}

//-----------------------------------------外部函数-----------------------------------------//
// 创建IPP Protocol Buffer
ipp_protocol* create_protocol(void) {
	return create_protocol_size(PROTOCOL_BUFFER_INIT_SIZE);
}

ipp_protocol* create_protocol_rule(protocol_increase_rule rule)
{
	ipp_protocol* protocol = create_protocol();
	if (NULL != protocol)
		protocol->increase_rule = rule;
	return protocol;
}

// 创建IPP Protocol Buffer指定大小
ipp_protocol* create_protocol_size(int32_t size) {
	ipp_protocol* protocol = (ipp_protocol *)malloc(sizeof(ipp_protocol));
	if (NULL == protocol) {
		ipp_LogE_Prefix("malloc:%s(%d)-%s\r\n",__FILE__,__LINE__,__FUNCTION__);
		return NULL;
	}

	protocol->buffer = (char*)malloc(size);
	protocol->buffer_content_size = 0;
	protocol->increase_rule = increase_rule_exactly;
	protocol->buffer_size = size;
	protocol->buffer_index = 0;
	if (NULL == protocol->buffer) {
		ipp_LogE_Prefix("malloc:%s(%d)-%s\r\n",__FILE__,__LINE__,__FUNCTION__);
		FREE_POINTER(protocol);
		return NULL;
	}
	memset(protocol->buffer, 0, size);

	return protocol;
}

ipp_protocol* create_protocol_size_rule(int32_t size, protocol_increase_rule rule)
{
	ipp_protocol* protocol = create_protocol_size(size);
	if (NULL != protocol)
		protocol->increase_rule = rule;
	return protocol;
}

ipp_protocol* create_protocol_with_buffer(char *buffer, int32_t buffer_size)
{
	ipp_protocol* protocol = (ipp_protocol *)malloc(sizeof(ipp_protocol));
	if (NULL == protocol) {
		ipp_LogE_Prefix("malloc:%s(%d)-%s\r\n",__FILE__,__LINE__,__FUNCTION__);
		return NULL;
	}

	protocol->buffer = buffer;
	protocol->buffer_content_size = buffer_size;
	protocol->increase_rule = increase_rule_exactly;
	protocol->buffer_size = buffer_size;
	protocol->buffer_index = 0;
	if (NULL == protocol->buffer) {
		ipp_LogE_Prefix("malloc:%s(%d)-%s\r\n",__FILE__,__LINE__,__FUNCTION__);
		FREE_POINTER(protocol);
		return NULL;
	}

	return protocol;
}

// 克隆IPP Protocol Buffer
ipp_protocol* clone_protocol(ipp_protocol* protocol) {
	ipp_protocol* clone = NULL;
	if (NULL == protocol) {
		return NULL;
	}
	clone = create_protocol_size_rule(protocol->buffer_size, protocol->increase_rule);
	if (NULL == clone) {
		ipp_LogE_Prefix("malloc:%s(%d)-%s\r\n",__FILE__,__LINE__,__FUNCTION__);
		return NULL;
	}
	clone->buffer_content_size = protocol->buffer_content_size;
	clone->buffer_size = protocol->buffer_size;
	clone->buffer_index = 0;
	memcpy(clone->buffer, protocol->buffer, protocol->buffer_size);

	return clone;
}

// 释放IPP Protocol Buffer
void free_protocol(ipp_protocol** protocol) {
	if (*protocol != NULL) {
		FREE_POINTER((*protocol)->buffer);
		FREE_POINTER(*protocol);
	}
}

void reset_protocol_index(ipp_protocol* protocol)
{
	protocol->buffer_index = 0;
}

int32_t get_protocol_content_size(ipp_protocol* protocol)
{
	return protocol->buffer_content_size;
}

int32_t get_protocol_size(ipp_protocol* protocol)
{
	return protocol->buffer_size;
}

int32_t put_raw(ipp_protocol* protocol, void* data, int32_t data_size) {
	int32_t error_code;

	if (data_size < 0)
		return protocol_buffer_size_error;

	if (NULL == data || 0 == data_size)
		return protocol_success;

	error_code = check_protocol(protocol);
	if (protocol_success != error_code) {
		return error_code;
	}

	error_code = increase_protocol(protocol, data_size);
	if (protocol_success != error_code) {
		return error_code;
	}

	memcpy(protocol->buffer + protocol->buffer_index, data, data_size);
	protocol->buffer_index += data_size;
	protocol->buffer_content_size += data_size;

	return protocol_success;
}

int32_t get_raw(ipp_protocol* protocol, void* data, int32_t data_size) {
	int32_t error_code;;

	if (NULL == data || data_size < 0)
		return protocol_buffer_size_error;

	if (0 == data_size)
		return protocol_success;

	error_code = check_protocol_size(protocol, data_size);
	if (protocol_success != error_code) {
		return error_code;
	}

	memcpy(data, protocol->buffer + protocol->buffer_index, data_size);
	protocol->buffer_index += data_size;

	return protocol_success;
}

// 在IPP Protocol Buffer放入int8数据
int32_t put_char(ipp_protocol* protocol, char data) {
	return put_raw(protocol, &data, sizeof(data));
}

int32_t put_int16(ipp_protocol* protocol, int16_t data) {
	int16_t dataTrans = ipp_h2ls(data);
	return put_raw(protocol, &dataTrans, sizeof(dataTrans));
}

int32_t put_int32(ipp_protocol* protocol, int32_t data) {
	int32_t dataTrans = ipp_h2ll(data);
	return put_raw(protocol, &dataTrans, sizeof(dataTrans));
}

int32_t put_int64(ipp_protocol* protocol, int64_t data) {
	int64_t dataTrans = ipp_h2lll(data);
	return put_raw(protocol, &dataTrans, sizeof(dataTrans));
}

int32_t put_float(ipp_protocol* protocol, float data) {
	int32_t dataTrans = ipp_h2lf(data);
	return put_raw(protocol, &dataTrans, sizeof(dataTrans));
}

int32_t put_double(ipp_protocol* protocol, double data) {
	int64_t dataTrans = ipp_h2ld(data);
	return put_raw(protocol, &dataTrans, sizeof(dataTrans));
}

int32_t put_string(ipp_protocol* protocol, char* data) {
	return put_array(protocol, data, sizeof(char), (int32_t)strlen(data));
}

// 在IPP Protocol Buffer放入数据数组
int32_t put_char_array(ipp_protocol* protocol, char* array, int32_t array_size) {
	if (NULL == array)
		return put_array(protocol, array, 0, 0);
	else
		return put_array(protocol, array, sizeof(array[0]), array_size);
}

int32_t put_int16_array(ipp_protocol* protocol, int16_t* array, int32_t array_size) {
	PUT_INT_BUFFER_NUMBER_ARRAY(int16, put_int16);
}

int32_t put_int32_array(ipp_protocol* protocol, int32_t* array, int32_t array_size) {
	PUT_INT_BUFFER_NUMBER_ARRAY(int32, put_int32);
}

int32_t put_int64_array(ipp_protocol* protocol, int64_t* array, int32_t array_size) {
	PUT_INT_BUFFER_NUMBER_ARRAY(int64, put_int64);
}

int32_t put_float_array(ipp_protocol* protocol, float* array, int32_t array_size) {
	PUT_INT_BUFFER_NUMBER_ARRAY(float, put_float);
}

int32_t put_double_array(ipp_protocol* protocol, double* array, int32_t array_size) {
	PUT_INT_BUFFER_NUMBER_ARRAY(double, put_double);
}

int32_t put_string_array(ipp_protocol* protocol, char** array, int32_t array_size) {
	PUT_INT_BUFFER_NUMBER_ARRAY(string, put_string);
}

// 从IPP Protocol Buffer获取int8数据
int32_t get_char(ipp_protocol* protocol, char* data) {
	return get_raw(protocol, data, sizeof(char));
}

int32_t get_int16(ipp_protocol* protocol, int16_t* data) {
	GET_FROM_BUFFER_NUMBER(int16_t, ipp_l2hs);
}

int32_t get_int32(ipp_protocol* protocol, int32_t* data) {
	GET_FROM_BUFFER_NUMBER(int32_t, ipp_l2hl);
}

int32_t get_int64(ipp_protocol* protocol, int64_t* data) {
	GET_FROM_BUFFER_NUMBER(int64_t, ipp_l2hll);
}

int32_t get_float(ipp_protocol* protocol, float* data) {
	GET_FROM_BUFFER_NUMBER(int32_t, ipp_l2hf);
}

int32_t get_double(ipp_protocol* protocol, double* data) {
	GET_FROM_BUFFER_NUMBER(int64_t, ipp_l2hd);
}

int32_t get_string(ipp_protocol* protocol, char** data) 
{
	int32_t array_size = 0;
	int32_t error_code = get_int32(protocol, &array_size);
	if (protocol_success != error_code) {
		return error_code;
	}

	if (array_size < 0)
		return protocol_buffer_size_error;

	error_code = check_protocol_size(protocol, array_size);
	if (protocol_success != error_code) {
		return error_code;
	}

	*data = (char*)malloc(array_size + 1);
	if (NULL == *data) {
		ipp_LogE("Error:%s(%d)-%s-array_size = %d, \r\n" ,__FILE__,__LINE__,__FUNCTION__, array_size);
		return protocol_alloc_failed;
	}

	error_code = get_raw(protocol, *data, array_size);
	if (protocol_success != error_code) {
		return error_code;
	}

	*(*data + array_size) = '\0';
	return protocol_success;
}

// 从IPP Protocol Buffer获取数据数组
int32_t get_char_array(ipp_protocol* protocol, char** array, int32_t* array_size) {
	return get_array(protocol, (void **)array, sizeof(char), array_size);
}

int32_t get_int16_array(ipp_protocol* protocol, int16_t** array, int32_t* array_size) {
	GET_FROM_BUFFER_NUMBER_ARRAY(int16_t, get_int16);
}

int32_t get_int32_array(ipp_protocol* protocol, int32_t** array, int32_t* array_size) {
	GET_FROM_BUFFER_NUMBER_ARRAY(int32_t, get_int32);
}

int32_t get_int64_array(ipp_protocol* protocol, int64_t** array, int32_t* array_size) {
	GET_FROM_BUFFER_NUMBER_ARRAY(int64_t, get_int64);
}

int32_t get_float_array(ipp_protocol* protocol, float** array, int32_t* array_size) {
	GET_FROM_BUFFER_NUMBER_ARRAY(float, get_float);
}

int32_t get_double_array(ipp_protocol* protocol, double** array, int32_t* array_size) {
	GET_FROM_BUFFER_NUMBER_ARRAY(double, get_double);
}

int32_t get_string_array(ipp_protocol* protocol, char*** array, int32_t* array_size)  {
	GET_FROM_BUFFER_NUMBER_ARRAY(char*, get_string);
}

/*--------------------------------------功能类接口--------------------------------------*/
void print_protocol(ipp_protocol* protocol)
{
	int i = 0;
	for (i = 0; i < protocol->buffer_content_size; ++i)
	{
		ipp_LogV("%c", protocol->buffer[i]);
	}

	ipp_LogV("\r\n");
}