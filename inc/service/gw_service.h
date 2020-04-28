#ifndef __GW_SERVICE_H__
#define __GW_SERVICE_H__

#include "dev_mng.h"
#include "msg_mng.h"

typedef enum _online_type
{
	ONLINE_JOIN_NET = 0,
	ONLINE_SHORT_CHANGE,
	ONLINE_AFTER_OFFLINE,
	ONLINE_AFTER_REBOOT,
	
	ON_LINT_BUTT
}ONLINE_TYPE_E;

typedef void(*ONLINE_CALLBACK)(char *sn);
typedef void(*OFFLINE_CALLBACK)(char *sn);
typedef void(*ALARM_CALLBACK)(char *sn, char *alarm);

void reg_online_callback(ONLINE_CALLBACK cb);

void reg_offline_callback(OFFLINE_CALLBACK cb);

void reg_alarm_callback(ALARM_CALLBACK cb);

int dev_online(DEV_INFO_S *dev, ONLINE_TYPE_E online_type);

int dev_offline(char *sn);

int dev_report(char *sn, char *data_json);

int process_mqtt_msg(char *msg, int len, char **resp, int *resp_len);

int process_http_msg(uint8_t *buf, int size, char *sn, MSG_TYPE_E type);

#endif /*__GW_SERVICE_H__*/
