#ifndef __GW_SERVICE_H__
#define __GW_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_mng.h"
#include "msg_mng.h"
#include "rex_uart.h"

typedef enum _online_type
{
	ONLINE_JOIN_NET = 0,
	ONLINE_SHORT_CHANGE,
	ONLINE_AFTER_OFFLINE,
	ONLINE_AFTER_REBOOT,
	
	ON_LINT_BUTT
}ONLINE_TYPE_E;

typedef void(*ONLINE_CALLBACK)(char *sn, char *product_id);
typedef void(*OFFLINE_CALLBACK)(char *sn, char *product_id);
typedef void(*ALARM_CALLBACK)(char *sn, char *alarm);
typedef void(*ZIGBEE_CALLBACK)(int flag);

//Stub for x86_64 linux debug 
#ifndef ZIGBEE_ENABLE
int zigbee_network_query(void);
int usb_dev_query(void);
int init_rex(void);
void reg_usb_port_callback(USB_PORT_CHECK cb);
#endif

void usb_port_callback(USB_PORT_STA_E sta);

void reg_online_callback(ONLINE_CALLBACK cb);

void reg_offline_callback(OFFLINE_CALLBACK cb);

void reg_alarm_callback(ALARM_CALLBACK cb);

void reg_zigbee_callback(ZIGBEE_CALLBACK cb);

int dev_online(DEV_INFO_S *dev, ONLINE_TYPE_E online_type);

int dev_offline(char *sn);

int dev_report(char *sn, char *data_json);

int dev_offline_callback(char *sn);

void dev_zigbee_callback(int flag);

int dev_report_callback(char *sn, uint32_t ep_id, uint16_t data_type, char *data);

int report_gw_status(void);

int dev_offline_check(void);

int process_mqtt_msg(char *msg, int len, char **resp, int *resp_len);

int process_local_msg(char *msg, int len, char **resp, int *resp_len);

int process_http_msg(uint8_t *buf, int size, char *sn, MSG_TYPE_E type);

#ifdef __cplusplus
}
#endif


#endif /*__GW_SERVICE_H__*/
