#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ipp_defines.h"
#include "ipp_protocol.h"
#include "ipp_protocol_helper.h"

#define PROTOCOL_BUFFER_TO_ASCII_CHANGE_VAL 'A'

int16_t swap_int16(int16_t s)
{
	return ((((s) >> 8) & 0x00FFL ) |
		( ((s) << 8) & 0xFF00L ) );
}

int32_t swap_int32(int32_t l)
{
	return ((((l) >> 24) & 0x000000FFL ) |
	( ((l) >>  8) & 0x0000FF00L ) |
	( ((l) <<  8) & 0x00FF0000L ) |
	( ((l) << 24) & 0xFF000000L ) );
}

int64_t swap_int64(int64_t ll)
{
	return ((((ll) >> 56) & 0x00000000000000FFLL ) |
	( ((ll) >> 40) & 0x000000000000FF00LL ) |
	( ((ll) >> 24) & 0x0000000000FF0000LL ) |
	( ((ll) >>  8) & 0x00000000FF000000LL ) |
	( ((ll) <<  8) & 0x000000FF00000000LL ) |
	( ((ll) << 24) & 0x0000FF0000000000LL ) |
	( ((ll) << 40) & 0x00FF000000000000LL ) |
	( ((ll) << 56) & 0xFF00000000000000LL ) );
}

//由于目前手机端，强终端等已经是采用小端字节序，所以为了更好地兼容，
//以后的各种弱终端平台，只要是用到ipp_protocol的，均采用小端字节序，如果是SDK开发者自己封装buffer，需要自行处理大小端。
//返回1，为大端；反之，为小端；
IPP_BOOL check_big_endian()
{
	union
	{
		unsigned int  a;
		unsigned char b;
	}c;
	c.a = 1;

	return (1 == c.b) ? FALSE : TRUE;
}

//16Bit，本地字节序转换成小端字节序
//host to little short
int16_t ipp_h2ls(int16_t val)
{
	return check_big_endian() ?  swap_int16(val) : val;
}

//16Bit，小端字节序转换成本地字节序
int16_t ipp_l2hs(int16_t val)
{
	return ipp_h2ls(val);
}

//32Bit，本地字节序转换成小端字节序
int32_t ipp_h2ll(int32_t val)
{
	return check_big_endian() ?  swap_int32(val) : val;
}

//32Bit，小端字节序转换成本地字节序
int32_t ipp_l2hl(int32_t val)
{
	return ipp_h2ll(val);
}

//64Bit，本地字节序转换成小端字节序
int64_t ipp_h2lll(int64_t val)
{
	return check_big_endian() ?  swap_int64(val) : val;
}

//64Bit，小端字节序转换成本地字节序
int64_t ipp_l2hll(int64_t val)
{
	return ipp_h2lll(val);
}

//float，本地字节序转换成小端字节序
int32_t ipp_h2lf(float val)
{
	int32_t temp_val;
	int32_t return_val;
	temp_val =  (int32_t)(val); //temp_val =  *(int32_t*)(&val); 
	return_val = check_big_endian() ?  swap_int32(temp_val) : temp_val;
	return return_val;
}

//float，小端字节序转换成本地字节序
float ipp_l2hf(int32_t val)
{
	const int32_t temp_val = check_big_endian() ?  swap_int32(val) : val;
	float return_val;
	return_val =  (float)(temp_val); //*((int32_t*)&return_val) = temp_val;
	return return_val;
}

//double，本地字节序转换成小端字节序
int64_t ipp_h2ld(double val)
{
	int64_t temp_val;
	int64_t return_val;
	temp_val =  (int64_t)(val); //temp_val = *(int64_t*)(&val);
	return_val = check_big_endian() ?  swap_int64(temp_val) : temp_val;
	return return_val;
}

//double，小端字节序转换成本地字节序
double ipp_l2hd(int64_t val)
{
	const int64_t temp_val = check_big_endian() ?  swap_int64(val) : val;
	double return_val;
	return_val =  (double)(temp_val); //*((int64_t*)&return_val) = temp_val;
	return return_val;
}

// 检查IPP Protocol Buffer合法性
int32_t check_protocol(ipp_protocol* protocol) {
	if (NULL == protocol) {
		return protocol_is_null;
	}
	if (NULL == protocol->buffer) {
		return protocol_buffer_is_null;
	}

	return protocol_success;
}

// 检查IPP Protocol Buffer合法性
int32_t check_protocol_size(ipp_protocol* protocol, int32_t data_size) {
	if (NULL == protocol) {
		ipp_LogE("protocol is null\r\n");
		return protocol_is_null;
	}
	if (NULL == protocol->buffer) {
		ipp_LogE("protocol->buffer is null\r\n");
		return protocol_buffer_is_null;
	}
	if (data_size > (protocol->buffer_size - protocol->buffer_index)) {
		ipp_LogE("data_size is too big\r\n");
		return protocol_buffer_not_enough;
	}

	return protocol_success;
}

// 转换buffer到ascii串
int32_t get_ascii_from_protocol(ipp_protocol* protocol, char** ascii_string) {
	int i;
	char high_half_byte;
	char low_half_byte;
	uint8_t byte_value;
	int32_t error_code = check_protocol(protocol);
	if (protocol_success != error_code) {
		return error_code;
	}

	(*ascii_string) = (char*)malloc((protocol->buffer_content_size) * 2 + 1);
	if (NULL == (*ascii_string)) {
		ipp_LogE_Prefix("malloc:%s(%d)-%s\r\n",__FILE__,__LINE__,__FUNCTION__);
		return protocol_alloc_failed;
	}

	for (i = 0; i < protocol->buffer_content_size; ++i) {
		byte_value = protocol->buffer[i];
		low_half_byte = (byte_value & 0x0F) + PROTOCOL_BUFFER_TO_ASCII_CHANGE_VAL;
		high_half_byte = ((byte_value >> 4) & 0x0F) + PROTOCOL_BUFFER_TO_ASCII_CHANGE_VAL;
		(*ascii_string)[i * 2 + 0] = high_half_byte;
		(*ascii_string)[i * 2 + 1] = low_half_byte;
	}

	(*ascii_string)[(protocol->buffer_content_size) * 2] = '\0';

	return protocol_success;
}

// 转换ascii串到buffer
int32_t get_protocol_from_ascii(char* ascii_string, ipp_protocol** protocol) {
	int i;
	char high_half_byte;
	char low_half_byte;
	uint8_t byte_value;
	int ascii_string_len;
	if (NULL == ascii_string) {
		return protocol_buffer_is_null;
	}

	ascii_string_len = (int)strlen(ascii_string);
	if ((ascii_string_len % 2 != 0)) {
		return protocol_buffer_size_error;
	}

	(*protocol) = create_protocol_size(ascii_string_len / 2);

	if (NULL == (*protocol))
	{
		ipp_LogE_Prefix("malloc:%s(%d)-%s\r\n",__FILE__,__LINE__,__FUNCTION__);
		return protocol_alloc_failed;
	}

	for (i = 0; i < (*protocol)->buffer_size; ++i) {
		high_half_byte = ascii_string[i * 2 + 0];
		low_half_byte = ascii_string[i * 2 + 1];
		byte_value = (low_half_byte - PROTOCOL_BUFFER_TO_ASCII_CHANGE_VAL) |
			((high_half_byte - PROTOCOL_BUFFER_TO_ASCII_CHANGE_VAL) << 4);
		put_char((*protocol), byte_value);
	}

	reset_protocol_index((*protocol));
	return protocol_success;
}
