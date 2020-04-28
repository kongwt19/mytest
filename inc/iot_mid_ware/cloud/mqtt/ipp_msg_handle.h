#ifndef __IPP_MSG_HANDLE_H__
#define __IPP_MSG_HANDLE_H__

#ifndef NULL
#define NULL 0
#endif

//typedef long long  int64_t;

typedef struct {
	char     Version;
	char     FromLen;
	char     ToLen;
	char     MsgType;  //  req/res

	char*	 From;     //?????id(SN),????:32??
	char*	 To;       //?????id(SN),????:32??
	char*    Content;
    char     Reserved[4];
	
	int      Length;
	int64_t  Msgid; //??id,????????
	
}IPP_MSG;

typedef struct {
	char *data;
	int current;
	int maxsize;
}mqtt_buffer_t;


int put_ipp_msg_buffer(IPP_MSG *ipp_msg, mqtt_buffer_t *mqtt_buffer);

int get_ipp_msg_buffer(IPP_MSG *ipp_msg, char *mqtt_buffer);

void mqtt_cleanup_buffer(mqtt_buffer_t *dest);

#endif
