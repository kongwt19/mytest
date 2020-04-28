#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ipp_msg_handle.h"
#include "platform.h"
#include "defines.h"

#define  IPP_BUFFER_MAX_LEN   1024
int mqtt_init_buffer(mqtt_buffer_t *dest, int size)
{
	if (NULL == dest)
	{
		LogE_Prefix("mqtt init buffer is NULL\r\n");
		return -1;
	}

	dest->current = 0;
	dest->maxsize = sizeof(char) * size;
	dest->data = (char *)malloc(dest->maxsize);

	if (NULL == dest->data)
	{
		LogE_Prefix("Error: Malloc for pBuf->data failed\r\n");
		return -1;
	}
	memset(dest->data, '\0', dest->maxsize);
	
	return 0;
}

int mqtt_get_avilable_bytes(mqtt_buffer_t *dest)
{
	return (dest->maxsize / sizeof(char)) - dest->current;
}

int check_buffer(mqtt_buffer_t *dest, int src_size)
{
	int avilable_char_size = mqtt_get_avilable_bytes(dest);

	if (src_size > avilable_char_size)
	{
		int new_size = dest->maxsize * 2 + sizeof(char);
		char *temp = (char *)malloc(new_size);
		if (temp == NULL)
		{
			LogE_Prefix("mqtt_add_data_to_buffer extend buffer malloc error\r\n");
			return -1;
		}

		memset(temp, '\0', new_size);
		memcpy(temp, dest->data, dest->current);
		FREE_POINTER(dest->data);
		dest->maxsize = new_size;
		dest->data = temp;
	}
	return 0;
}

void mqtt_cleanup_buffer(mqtt_buffer_t *dest)
{
	FREE_POINTER(dest->data);
	dest->current = 0;
	return;
}


int put_char_buffer(mqtt_buffer_t *dest, char *src)
{
	if (dest == NULL)
	{
		LogE_Prefix("Error: mqtt buffer is NULL\r\n");
		return -1;
	}
	int src_size = sizeof(char);

	check_buffer(dest, src_size);

	memcpy(dest->data + dest->current, src, src_size);
	dest->current += src_size;

	if (mqtt_get_avilable_bytes(dest) > 1)
	{
		dest->data[dest->current] = '\0';
	}

	return 0;
}

int put_int_buffer(mqtt_buffer_t *dest, int *src)
{
	if (dest == NULL)
	{
		LogE_Prefix("Error: mqtt buffer is NULL\r\n");
		return -1;
	}
	int src_size = sizeof(int);

	check_buffer(dest, src_size);

	memcpy(dest->data + dest->current, src, src_size);
	dest->current += src_size;

	if (mqtt_get_avilable_bytes(dest) > 1)
	{
		dest->data[dest->current] = '\0';
	}

	return 0;
}


int put_int64_buffer(mqtt_buffer_t *dest, int64_t *src)
{
	if (dest == NULL)
	{
		LogE_Prefix("Error: mqtt buffer is NULL\r\n");
		return -1;
	}
	int src_size = sizeof(int64_t);

	check_buffer(dest, src_size);

	memcpy(dest->data + dest->current, src, src_size);
	dest->current += src_size;

	if (mqtt_get_avilable_bytes(dest) > 1)
	{
		dest->data[dest->current] = '\0';
	}

	return 0;
}

int put_str_buffer(mqtt_buffer_t *dest, char *src, int src_size)
{
	if (dest == NULL)
	{
		LogE_Prefix("Error: mqtt buffer is NULL\r\n");
		return -1;
	}

	check_buffer(dest, src_size);
	memcpy(dest->data + dest->current, src, src_size);
	dest->current += src_size;

	if (mqtt_get_avilable_bytes(dest) > 1)
	{
		dest->data[dest->current] = '\0';
	}

	return 0;
}

int put_array_buffer(mqtt_buffer_t *dest, char *src, int src_size)
{
	if (dest == NULL)
	{
		LogE_Prefix("Error: mqtt buffer is NULL\r\n");
		return -1;
	}

	check_buffer(dest, src_size);
	memcpy(dest->data + dest->current, src, src_size);
	dest->current += src_size;

	if (mqtt_get_avilable_bytes(dest) > 1)
	{
		dest->data[dest->current] = '\0';
	}

	return 0;
}

/*  ????pp???buffer  */
int put_ipp_msg_buffer(IPP_MSG *ipp_msg, mqtt_buffer_t *mqtt_buffer)
{
	if (NULL == ipp_msg || NULL == mqtt_buffer)
	{
		LogE_Prefix("%s,%d:  ipp_mess or mqtt_buffer is NULL\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (mqtt_init_buffer(mqtt_buffer, IPP_BUFFER_MAX_LEN ) < 0)
	{
		LogE_Prefix("mqtt_init_buffer error\r\n");
		return -1;
	}


	put_char_buffer(mqtt_buffer, &ipp_msg->Version);

	put_char_buffer(mqtt_buffer, &ipp_msg->FromLen);


    if (ipp_msg->FromLen  < 0 || ipp_msg->FromLen > IPP_BUFFER_MAX_LEN)
	{
	    LogE_Prefix("ipp_msg->FromLen is  illegal!\r\n");
		return -1;
    }
	else
	{
		put_str_buffer(mqtt_buffer, ipp_msg->From, ipp_msg->FromLen );
	}

	put_char_buffer(mqtt_buffer, &ipp_msg->ToLen);

	
    if (ipp_msg->ToLen < 0 || ipp_msg->ToLen > IPP_BUFFER_MAX_LEN)
	{
	    LogE_Prefix("ipp_msg->ToLen is  illegal!\r\n");
		return -1;
    }
	else
	{
		put_str_buffer(mqtt_buffer, ipp_msg->To, ipp_msg->ToLen);
	}

	put_int64_buffer(mqtt_buffer, &ipp_msg->Msgid);

	put_char_buffer(mqtt_buffer, &ipp_msg->MsgType);

	//if (strcmp(ipp_msg->Retain, ""))
	{
		put_array_buffer(mqtt_buffer, ipp_msg->Reserved, sizeof(ipp_msg->Reserved));
	}

	put_int_buffer(mqtt_buffer, &ipp_msg->Length);

    	
    if (ipp_msg->Length < 0 || ipp_msg->Length > IPP_BUFFER_MAX_LEN)
	{
	    LogE_Prefix("ipp_msg->Length is  illegal!\r\n");
		return -1;
    }
	else
	{
		put_str_buffer(mqtt_buffer, ipp_msg->Content, ipp_msg->Length);
	}

	return 0;
}

int get_char_buffer(char *dest, char *mqtt_buffer, int* current)
{
	memcpy(dest, mqtt_buffer + *current, sizeof(char));
	*current += sizeof(char);
	return 0;
}

int get_int_buffer(int *dest, char *mqtt_buffer, int* current)
{
	memcpy(dest, mqtt_buffer + *current, sizeof(int));
	*current += sizeof(int);
	return 0;
}

int get_int64_buffer(int64_t *dest, char *mqtt_buffer, int* current)
{
	memcpy(dest, mqtt_buffer + *current, sizeof(int64_t));
	*current += sizeof(int64_t);
	return 0;
}

int get_str_buffer(char *dest, int dest_len, char *mqtt_buffer, int* current)
{
	if(NULL == dest)
    {
 	   LogI_Prefix("dest is NULL!\r\n");
 	   return -1;
    }

	memcpy(dest, mqtt_buffer + *current, dest_len );
	*current += dest_len;
	dest[dest_len] = '\0';
	return 0;
}

int get_array_buffer(char *dest, int dest_len, char *mqtt_buffer, int* current)
{
	memcpy(dest, mqtt_buffer + *current, dest_len );
	*current += dest_len;
	return 0;
}

int get_ipp_msg_buffer(IPP_MSG *ipp_msg, char *mqtt_buffer)
{
	if (NULL == ipp_msg || NULL == mqtt_buffer)
	{
	    if(NULL == ipp_msg)
	    {
			LogI_Prefix("NULL == ipp_msg");
		}
		if(NULL == mqtt_buffer)
	    {
			LogI_Prefix("NULL == mqtt_buffer");
		}
		LogE_Prefix("%s,%d: ipp_msg or mqtt_buffer is NULL\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	int current = 0;
	get_char_buffer(&ipp_msg->Version, mqtt_buffer, &current);

	get_char_buffer(&ipp_msg->FromLen, mqtt_buffer, &current);


	if (0 == ipp_msg->FromLen)
	{
		ipp_msg->From = "";
	}
	else if (ipp_msg->FromLen  < 0 || ipp_msg->FromLen > IPP_BUFFER_MAX_LEN)
	{
	    LogE_Prefix("ipp_msg->FromLen is  illegal!\r\n");
		return -1;
	}
	else
	{
		ipp_msg->From = (char *)malloc(ipp_msg->FromLen + 1);
		if (NULL == ipp_msg->From)
		{
			LogE_Prefix("ipp_msg->From malloc buffer error\r\n");
			return -1;
		}
		memset(ipp_msg->From , 0 , ipp_msg->FromLen + 1);
	
		get_str_buffer(ipp_msg->From, ipp_msg->FromLen, mqtt_buffer, &current);
	}

	get_char_buffer(&ipp_msg->ToLen, mqtt_buffer, &current);


	if (0 == ipp_msg->ToLen)
	{
		ipp_msg->To = "";
	}
	else if (ipp_msg->ToLen < 0 || ipp_msg->ToLen > IPP_BUFFER_MAX_LEN)
	{
	    FREE_POINTER(ipp_msg->From);
	    LogE_Prefix("ipp_msg->ToLen is  illegal!\r\n");
		return -1;
    }
	else
	{
		ipp_msg->To = (char *)malloc(ipp_msg->ToLen + 1);
		if (NULL == ipp_msg->To)
		{
		    FREE_POINTER(ipp_msg->From);
			LogE_Prefix("ipp_msg->From malloc buffer error\r\n");
			return -1;
		}
        memset(ipp_msg->To , 0 , ipp_msg->ToLen + 1);
		
		get_str_buffer(ipp_msg->To, ipp_msg->ToLen, mqtt_buffer, &current);
	}

	get_int64_buffer(&ipp_msg->Msgid, mqtt_buffer, &current);
    
    get_char_buffer(&ipp_msg->MsgType, mqtt_buffer, &current);
	
	get_array_buffer(ipp_msg->Reserved, sizeof(ipp_msg->Reserved), mqtt_buffer, &current);

	get_int_buffer(&ipp_msg->Length, mqtt_buffer, &current);
     
	if (0 == ipp_msg->Length)
	{
		ipp_msg->Content = "";
	}
    else if (ipp_msg->Length < 0 || ipp_msg->Length > IPP_BUFFER_MAX_LEN)
	{
	    FREE_POINTER(ipp_msg->From);
		FREE_POINTER(ipp_msg->To);
	    LogE_Prefix("ipp_msg->Length is  illegal!\r\n");
		return -1;
    }
	else
	{
		ipp_msg->Content = (char *)malloc(ipp_msg->Length + 1);
		if (NULL == ipp_msg->Content)
		{
		    FREE_POINTER(ipp_msg->From);
			FREE_POINTER(ipp_msg->To);
			LogE_Prefix("ipp_msg->From malloc buffer error\r\n");
			return -1;
		}
        memset(ipp_msg->Content , 0 , ipp_msg->Length + 1);
		
		get_str_buffer(ipp_msg->Content, ipp_msg->Length, mqtt_buffer, &current);
	}

	return 0;
}
