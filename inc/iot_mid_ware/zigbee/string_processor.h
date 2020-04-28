/*******************************************************************************************************
**                                           File description
** File Name:           string_processor.h
** Description:         字符串处理器
** Creator:             xieshaobing
** Creation Date:       2016年9月5日
** Modify Log:          none
** Last Modified Date:  2016年9月5日
*******************************************************************************************************/

#ifndef __INC_STRING_HELPER_H_
#define __INC_STRING_HELPER_H_

/* If this is a C++ compiler, use C linkage */
#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * string_processor_sub_string
 *     获取子字符串
 * source_char[in]: source character
 * pos[in]: start intercept position
 * length[in]: intercept length
 * result[out]: return sub string
 * note: none
 */
extern void string_processor_sub_string(char *source_char, int pos, int length, char *result);

/**
 * string_processor_find_index
 *     查找字符串
 * source_char[in]: source string to be found
 * find_char[in]: find string
 * return: find the position of the first appearance of the string, otherwise -1
 * note: none
 */
extern int string_processor_find_index(const char *source_char, const char *find_char);

/**
 * string_processor_hex_str_to_byte
 *     16进制字符串转字节数组
 * hex_str[in]: hex string to be converted
 * length[in]: hex string length
 * result[out]: hex character array
 * note: none
 */
extern void string_processor_hex_str_to_byte(const char *hex_str, int length, unsigned char *result);

/**
 * string_processor_byte_to_hex_str
 *     字节数组转16进制字符串
 * bytes[in]: unsigned character array to be converted
 * length[in]: character array length
 * result[out]: return hex string
 * note: none
 */
extern void string_processor_byte_to_hex_str(const unsigned char *bytes, int length, char *result);

extern int profile_reader_get_item(char *profile_name, char *section_name, char *item_name, char *item_value);


/* If this is a C++ compiler, use C linkage */
#if defined(__cplusplus)
}
#endif

#endif
/*******************************************************************************************************
**                                           End of file
*******************************************************************************************************/
