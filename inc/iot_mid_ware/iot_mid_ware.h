#ifndef __IOT_MID_WARE_H__
#define __IOT_MID_WARE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_mng.h"

#define MAX_CONN_TYPE_SUPPORT   5

typedef int (*CTL_DEV_CALLBACK)(uint16_t dev_type, char *sn, char *cmd);

typedef struct _ctl_func
{
    CONN_TYPE_E conn_type;
    CTL_DEV_CALLBACK ctl_func;
}CTL_FUNC_S;

void reg_dev_ctl_callback(CONN_TYPE_E conn_type, CTL_DEV_CALLBACK cb);

int ctl_dev(char *sn, char *cmd);

void set_mqtt_server(const char *ip, const short port);

void set_cdc_server(const char *ip, const short port);

void set_ippinte_server(const char *ip, const short port);

void get_mqtt_server(char *ip, short *port);

void get_cdc_server(char *ip, short *port);

void get_ippinte_server(char *ip, short *port);

int init_iot_mid_ware(void);

int deinit_iot_mid_ware(void);

#ifdef __cplusplus
}
#endif

#endif/*__IOT_MID_WARE_H__*/

