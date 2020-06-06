//socket udp 服务端
#include "udp_ser.h"
#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include "pthread.h"
#include "defines.h"
#include "error_code.h"
#include "rex_common.h"
#include "cJSON.h"
#include "gw_service.h"
#include "biz_plf.h"
#include "platform.h"

#define MSG_LEN 512
#define DEV_NUM 128
static pthread_t udp_pid;
#define STR_LEN 16
#define SN_PREFIX "AAAACCDDEEFFF"

char *  GenerateStr(int * index_num)
{
    int flag = (*index_num);
    char * str = (char *)malloc(STR_LEN + 1);
	memset(str, 0, STR_LEN + 1);
	strcpy(str, SN_PREFIX);
	char num[4];
	bzero(num,4); 
	sprintf(num, "%03d",flag+1);
    (*index_num)++;
	strcat(str, num);
    return str;
}

int devs_online()
{
    char *sn;
    char *type;
    int index_num = 0;
    int ret = GW_ERR;
    int i = 1;
    for(; i <= DEV_NUM; i++)
    {
        sn = GenerateStr(&index_num);
        if(i <= 21)
        {
            type = "1001";
        }
        else if(i > 21 && i <= 42)
        {
            type = "1101";
        }
        else if(i > 42 && i <= 63)
        {
            type = "1102";
        }
        else if(i > 63 && i <= 84)
        {
            type = "1103";
        }
        else if(i > 84 && i <= 105)
        {
            type = "1304";
        }
        else if(i > 105 && i <= 128)
        {
            type = "1312";
        }
        LogI_Prefix("dev(%s), the type is %s online\n", sn, type);
        ret = device_join_net_report(sn, type);
        sleep_s(2);
    }
    return ret;
}

int prase_udp_meg(char *msg)
{
    LogI_Prefix("recv msg =%s\n",msg);
    cJSON *root = cJSON_Parse(msg);
	RETURN_IF_NULL(root, GW_NULL_PARAM);
    int ret = GW_ERR;
    cJSON *deviceId = NULL;
	cJSON * method_node = NULL;

    method_node = cJSON_GetObjectItem(root, "method");
	if((method_node != NULL) && (!(strcmp(method_node->valuestring, "online"))))
	{
		ret = devs_online();
	}
    else if((method_node != NULL) && (!(strcmp(method_node->valuestring, "offline"))))
    {
        deviceId = cJSON_GetObjectItem(root, "deviceId");
		RETURN_IF_NULL(deviceId, GW_NULL_PARAM);
        ret = dev_offline_callback(deviceId->valuestring);
    }
    else if((method_node != NULL) && (!(strcmp(method_node->valuestring, "status"))))
    {
        deviceId = cJSON_GetObjectItem(root, "deviceId");
		RETURN_IF_NULL(deviceId, GW_NULL_PARAM);
        cJSON * point_node = NULL;
		point_node = cJSON_GetObjectItem(root, "endpoint_id");
		RETURN_IF_NULL(point_node, GW_NULL_PARAM);
        cJSON * type_node = NULL;
		type_node = cJSON_GetObjectItem(root, "state_type");
		RETURN_IF_NULL(type_node, GW_NULL_PARAM);
        cJSON * state_node = NULL;
		state_node = cJSON_GetObjectItem(root, "state");
		RETURN_IF_NULL(state_node, GW_NULL_PARAM);
        char *data=cJSON_Print(state_node);
        ret = dev_report_callback(deviceId->valuestring, point_node->valueint, type_node->valueint, data);
        FREE_POINTER(data);
    }
    cJSON_Delete(root);
    return ret;
}

void *udp_rev_msg(void *arg)
{
    LogI_Prefix("udp_rev_msg start\n");
    //创建socket对象
    int sockfd=socket(AF_INET,SOCK_DGRAM,0);

    //创建网络通信对象
    struct sockaddr_in addr;
    addr.sin_family =AF_INET;
    addr.sin_port =htons(1324);
    addr.sin_addr.s_addr=htonl(INADDR_ANY);

    //绑定socket对象与通信链接
    int ret =bind(sockfd,(struct sockaddr*)&addr,sizeof(addr));
    if(0>ret)
    {
        LogD_Prefix("bind error\n");
    }
	
    struct sockaddr_in cli;
    socklen_t len=sizeof(cli);

    while(1)
    {
        char buf[MSG_LEN];
        int rcv_len = 0;
        do
        {
            LogI_Prefix("rcv the msg success\n");
            rcv_len = recvfrom(sockfd,&buf,sizeof(buf)-1,0,(struct sockaddr*)&cli,&len);
        } while (rcv_len <= 0);
		buf[rcv_len] = '\0';
        if(prase_udp_meg(buf) != GW_OK)
        {
            LogI_Prefix("prase the msg (%s) error\n", buf);
        }
    }
    close(sockfd);
}

void start_udp()
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&udp_pid, &attr, udp_rev_msg, NULL);
}