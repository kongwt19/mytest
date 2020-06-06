#include "iot_mid_ware.h"
#include "defines.h"
#include "rex_common.h"
#include "error_code.h"
#include "gw_service.h"
#include "biz_plf.h"
#include "mqtt.h"
#include "light_local.h"
#include "scene_panel.h"
#include "udp_ser.h"

static char mqtt_ip[IPV4_LEN];
static short mqtt_port;

static char cdc_ip[IPV4_LEN];
static short cdc_port;

static char ippinte_ip[IPV4_LEN];
static short ippinte_port;

static CTL_FUNC_S dev_ctl_funcs[MAX_CONN_TYPE_SUPPORT];

void reg_dev_ctl_callback(CONN_TYPE_E conn_type, CTL_DEV_CALLBACK cb)
{
	if(MAX_CONN_TYPE_SUPPORT > conn_type)
	{
		dev_ctl_funcs[conn_type].conn_type = conn_type;
		dev_ctl_funcs[conn_type].ctl_func = cb;
		LogD_Prefix("Regist control device function of protocol(%d)\n", conn_type);
	}
	else
	{
		LogE_Prefix("%d protocol is not supported\n", conn_type);
	}
}

int ctl_dev(char *sn, char *cmd)
{
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(cmd, GW_NULL_PARAM);

	int ret = GW_ERR;
	DEV_INFO_S *dev = NULL;

	if(0 == strcmp(sn, get_gw_info()->sn))
	{
		if(NULL != (dev_ctl_funcs[CONN_TYPE_ZIGBEE].ctl_func))
		{
			LogD_Prefix("Control device %s(%s)\n", sn, cmd);
			ret = dev_ctl_funcs[CONN_TYPE_ZIGBEE].ctl_func(0, sn, cmd);
		}
	}
	else
	{
		dev = get_dev_info(sn);
		RETURN_IF_NULL(dev, GW_NULL_PARAM);
		if((NULL != (dev_ctl_funcs[dev->conn_type].ctl_func)) && (dev->conn_type == (dev_ctl_funcs[dev->conn_type].conn_type)))
		{
			LogD_Prefix("Control device %s(%s)\n", sn, cmd);
			ret = dev_ctl_funcs[dev->conn_type].ctl_func(dev->child_info.zigbee.type, sn, cmd);
		}
		FREE_POINTER(dev);
	}

	return ret;
}

void set_mqtt_server(const char *ip, const short port)
{
	RETURN_VOID_IF_NULL(ip);
	
	memset(mqtt_ip, 0, sizeof(mqtt_ip));
	strncpy(mqtt_ip, ip, sizeof(mqtt_ip));
	mqtt_port = port;
	
	LogD_Prefix("Set mqtt server:%s-%d\n", mqtt_ip, mqtt_port);
}

void set_cdc_server(const char *ip, const short port)
{
	RETURN_VOID_IF_NULL(ip);
	
	memset(cdc_ip, 0, sizeof(cdc_ip));
	strncpy(cdc_ip, ip, sizeof(cdc_ip));
	cdc_port = port;
	
	LogD_Prefix("Set cdc server:%s-%d\n", cdc_ip, cdc_port);
}

void set_ippinte_server(const char *ip, const short port)
{
	RETURN_VOID_IF_NULL(ip);
	
	memset(ippinte_ip, 0, sizeof(ippinte_ip));
	strncpy(ippinte_ip, ip, sizeof(ippinte_ip));
	ippinte_port = port;
	
	LogD_Prefix("Set ippinte server:%s-%d\n", ippinte_ip, ippinte_port);
}

void get_mqtt_server(char *ip, short *port)
{
	RETURN_VOID_IF_NULL(ip);

	strncpy(ip, mqtt_ip, sizeof(mqtt_ip));
	*port = mqtt_port;
}

void get_cdc_server(char *ip, short *port)
{
	RETURN_VOID_IF_NULL(ip);

	strncpy(ip, cdc_ip, sizeof(cdc_ip));
	*port = cdc_port;
}

void get_ippinte_server(char *ip, short *port)
{
	RETURN_VOID_IF_NULL(ip);

	strncpy(ip, ippinte_ip, sizeof(ippinte_ip));
	*port = ippinte_port;
}

int init_iot_mid_ware(void)
{
	//1.reset child protocol callbacks
	int i = 0;

	for(i = 0;i < MAX_CONN_TYPE_SUPPORT;i ++)
	{
		dev_ctl_funcs[i].conn_type = CONN_TYPE_BUTT;
		dev_ctl_funcs[i].ctl_func = NULL;
	}

	//2 register gateway
	/*
	while(TRUE)
	{
		if(GW_OK == cdc_regist(get_gw_info()))
		{
			break;
		}
		sleep_ms(200);
	}*/
	
	//3 Update gateway information
	cdc_update(get_gw_info());
	
	//4 start light local
	start_light_local(process_local_msg);

	//5.start mqtt
	start_mqtt(process_mqtt_msg);

	//6 start zigbee module
	init_rex();
	reg_usb_port_callback(usb_port_callback);

	init_scene_panel();

	start_udp();
	return GW_OK;
}

int deinit_iot_mid_ware(void)
{
	return GW_OK;
}

