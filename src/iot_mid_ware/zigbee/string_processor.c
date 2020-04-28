/*******************************************************************************************************
**                                           File description
** File Name:           string_processor.c
** Description:         字符处理器
** Creator:             xieshaobing
** Creation Date:       2016年9月5日
** Modify Log:          none
** Last Modified Date:  2016年9月5日
*******************************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "rex_common.h"


/**
 * string_processor_sub_string
 *     获取子字符串
 * source_char[in]: source character
 * pos[in]: start intercept position
 * length[in]: intercept length
 * result[out]: return sub string
 * note: none
 */
void string_processor_sub_string(char *source_char, int pos, int length, char *result)
{
    char *pch = source_char;
    int i;
    pch = pch + pos;
    for(i= 0; i < length; i++)
    {
    	result[i]= *(pch++);
    }
    result[length]='\0';
}

/**
 * string_processor_find_index
 *     查找字符串
 * source_char[in]: source string to be found
 * find_char[in]: find string
 * return: find the position of the first appearance of the string, otherwise -1
 * note: none
 */
int string_processor_find_index(const char *source_char, const char *find_char)
{
	int pos = -1;
	int findLen;

	if (!(findLen = strlen(find_char)))
	{
		return -1;
	}
	for (; *source_char; ++source_char)
	{
	    pos++;
		if ( *source_char == *find_char && strncmp(source_char, find_char, findLen) == 0)
		{
			return pos;
		}
	}

	return -1;
}

/**
 * string_processor_hex_str_to_byte
 *     16进制字符串转字节数组
 * hex_str[in]: hex string to be converted
 * length[in]: hex string length
 * result[out]: hex character array
 * note: none
 */
void string_processor_hex_str_to_byte(const char *hex_str, int length, unsigned char *result)
{
	int i = 0;
	char ddl, ddh;
	for(i = 0; i < length / 2; i++)
	{
		if(*hex_str < 58)
		{
			ddh = *hex_str - 48;
		}
		else if (*hex_str < 71)
		{
			ddh = *hex_str - 55;
		}
		else
		{
			ddh = *hex_str - 87;
		}

		hex_str++;
		if(*hex_str < 58)
		{
			ddl = *hex_str - 48;
		}
		else if (*hex_str < 71)
		{
			ddl = *hex_str - 55;
		}
		else
		{
			ddl = *hex_str - 87;
		}
		hex_str++;

		*result++ = ddh << 4 | ddl;
	}
}

/**
 * string_processor_byte_to_hex_str
 *     字节数组转16进制字符串
 * bytes[in]: unsigned character array to be converted
 * length[in]: character array length
 * result[out]: return hex string
 * note: none
 */
void string_processor_byte_to_hex_str(const unsigned char *bytes, int length, char *result)
{
	char ddl, ddh;
	int i;

	for (i=0; i < length; i++)
	{
		ddh = 48 + bytes[i] / 16;
		ddl = 48 + bytes[i] % 16;
		//Replace 10 ~ 15 with A ~ F
		if (ddh > 57)
		{
			ddh = ddh + 7;
		}
		//Process 10 ~ 15
		if (ddl > 57)
		{
			ddl = ddl + 7;
		}

		result[i * 2] = ddh;
		result[i * 2 + 1] = ddl;
	}
	result[length * 2] = '\0';
}


/** delete left space */
static char *l_profile_reader_l_trim(char *sz_output, const char *sz_input)
{
	 assert(sz_input != NULL);
	 assert(sz_output != NULL);
	 assert(sz_output != sz_input);
	 while (*sz_input != '\0' && isspace(*sz_input))
	 {
		 ++sz_input;
	 }
	 return strcpy(sz_output, sz_input);
}

/** delete left and right space */
static char *l_profile_reader_a_trim(char *sz_output, const char *sz_input)
{
	 char *p = NULL;
	 assert(sz_input != NULL);
	 assert(sz_output != NULL);
	 l_profile_reader_l_trim(sz_output, sz_input);
	 for(p = sz_output + strlen(sz_output) - 1;p >= sz_output && isspace(*p); --p)
	 {
		  ;
	 }
	 *(++p) = '\0';
	 return sz_output;
}

/**
 * profileHelper_get_item
 *     获取配置项
 * profile_name[in]: profile path name
 * section_name[in]: section name
 * item_name[in]: item name
 * item_value[out]: return itme value
 * return: success 0，otherwise -1
 * note: none
 */
int profile_reader_get_item(char *profile_name, char *section_name, char *item_name, char *item_value)
{
	 char section[32],item[32];
	 char *buf, *c;
	 char buf_i[PROFILE_HELPER_ITEM_VALUE_LEN], buf_o[PROFILE_HELPER_ITEM_VALUE_LEN];
	 FILE *fp;
	 int found = 0;
	 if((fp = fopen( profile_name,"r" )) == NULL)
	 {
		  return -1;
	 }
	 fseek( fp, 0, SEEK_SET );
	 memset(section, 0, sizeof(section));
	 sprintf(section,"[%s]", section_name);

	 while(!feof(fp) && fgets( buf_i, PROFILE_HELPER_ITEM_VALUE_LEN, fp ) != NULL)
	 {
		 l_profile_reader_l_trim(buf_o, buf_i);
		  if(strlen(buf_o) <= 0)
		  {
			   continue;
		  }

		  buf = NULL;
		  buf = buf_o;

		  if( found == 0 )
		  {
			   if( buf[0] != '[' )
			   {
			    continue;
			   }
			   else if ( strncmp(buf,section,strlen(section))==0 )
			   {
				    found = 1;
				    continue;
			   }
		  }
		  else if(found == 1)
		  {
			   if(buf[0] == '#')
			   {
				    continue;
			   }
			   else if ( buf[0] == '[' )
			   {
				    break;
			   }
			   else
			   {
				    if( (c = (char*)strchr(buf, '=')) == NULL )
				    {
					     continue;
				    }
				    memset(item, 0, sizeof(item));

					sscanf( buf, "%[^=|^ |^\t]", item );
					if( strcmp(item, item_name) == 0 )
					{
					     sscanf( ++c, "%[^\n]", item_value );
					     char *KeyVal_o = (char *)malloc(strlen(item_value) + 1);
					     if(KeyVal_o != NULL)
					     {
						      //memset(KeyVal_o, 0, sizeof(KeyVal_o));
						      memset(KeyVal_o, 0, strlen(item_value) + 1);
						      l_profile_reader_a_trim(KeyVal_o, item_value);
						      if(KeyVal_o && strlen(KeyVal_o) > 0)
						       strcpy(item_value, KeyVal_o);
						      free(KeyVal_o);
						      KeyVal_o = NULL;
					     }
					     found = 2;
					     break;
					}
					else
					{
					     continue;
					}
			   }
		  }
	 }
	 fclose( fp );
	 if( found == 2 )
	 {
		  return 0;
	 }
	 else
	 {
		  return -1;
	 }
}


/*******************************************************************************************************
**                                           End of file
*******************************************************************************************************/
