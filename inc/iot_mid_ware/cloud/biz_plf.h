/*
 * biz_plf.h
 *
 *  Created on: 2020年4月8日
 *      Author: han
 */
#include "dev_mng.h"
#include "msg_mng.h"

#ifndef JSON_DEVICE_FUNCTION_H_
#define JSON_DEVICE_FUNCTION_H_
#define MAX_RESPONSE_SIZE 1024
#define URL_SIZE 100
#define POST "1"  
#define GET "0"
#define HTTP_REQ_SUCCESS "1000"

#define DEV_TYPE 1

#ifdef IPP2
#define IPP "2.0"
#define KEY "key2.0"
#else
#define IPP "3.0"
#define KEY "key3.0"
#endif

typedef struct _func_uri
{
	MSG_TYPE_E type;
	const char *uri;
}HTTP_FUNC_URL_S;
//业务云接口
int ippinte_alarm(char *deviceID, char* alarmInfo, int64_t alarmNumber);
int ippinte_get_alarm_switch(char *devID);
int ippinte_set_alarm_switch(char *devID, int onoff);
int ippinte_scene_panel_report(char *info, char *sn, int len);
int ippinte_scene_panel_get_by_point(char *devid, int point, char ** scene_panel_list);

//设备云接口
int cdc_slave_device_online(DEV_INFO_S *dev);
int cdc_slave_device_offline(char *slaveId);
int cdc_dev_report_status(char *sn, char *data_json, BOOL multi);
int cdc_regist(DEV_INFO_S *dev);
int cdc_update(DEV_INFO_S *dev);
char * cdc_get_token(char *devID, char **token);
int http_send(char *sn, MSG_TYPE_E msg_type, char *content, int content_len);
int http_send_msg(char *sn, MSG_TYPE_E msg_type, char *content, int content_len, BOOL async_send);

#endif
