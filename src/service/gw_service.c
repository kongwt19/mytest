#include "defines.h"
#include "error_code.h"
#include "dev_mng.h"
#include "gw_service.h"
#include "msg_mng.h"
#include "rex_common.h"
#include "cJSON.h"
#include "dev_cfg.h"
#include "biz_plf.h"
#include "mqtt.h"
#include "iot_mid_ware.h"

static ONLINE_CALLBACK online_cb = NULL;
static OFFLINE_CALLBACK offline_cb = NULL;
static ALARM_CALLBACK alarm_cb = NULL;

void reg_online_callback(ONLINE_CALLBACK cb)
{
	online_cb = cb;
}

void reg_offline_callback(OFFLINE_CALLBACK cb)
{
	offline_cb = cb;
}

void reg_alarm_callback(ALARM_CALLBACK cb)
{
	alarm_cb = cb;
}

int report_gw_status(void)
{
	int ret = GW_ERR;
	cJSON *root = cJSON_CreateObject();

	RETURN_IF_NULL(root, GW_NULL_PARAM);

	cJSON_AddNumberToObject(root, "zigbee_num", get_child_num(CONN_TYPE_ZIGBEE));
	cJSON_AddNumberToObject(root, "dongle", 1);
	cJSON_AddNumberToObject(root, "zigbee", 1);

	char *status = cJSON_PrintUnformatted(root);
	if(NULL != status)
	{
		ret = cdc_dev_report_status(get_gw_info()->sn, status);
		FREE_POINTER(status);
	}

	cJSON_Delete(root);
	
	return ret;
}

int dev_online(DEV_INFO_S *dev, ONLINE_TYPE_E online_type)
{
	int ret = GW_ERR;
	
	RETURN_IF_NULL(dev, GW_NULL_PARAM);

	// add to dev list
	ret = add_dev(dev, TRUE);
	if(GW_OK == ret)
	{
		if(CONN_TYPE_WIFI != dev->conn_type)
		{
			// online
			if(ONLINE_JOIN_NET == online_type)
			{
				if(GW_OK != cdc_regist(dev))
				{
					return GW_ERR;
				}
			}
			if(GW_OK != cdc_slave_device_online(dev))
			{
				return GW_ERR;
			}
			mqtt_slave_online(dev->sn);
			// user callback
			if(NULL != online_cb)
			{
				online_cb(dev->sn);
			}
			report_gw_status();
		}
	}
	
	return GW_OK;
}

int dev_offline(char *sn)
{
	int ret = GW_ERR;

	RETURN_IF_NULL(sn, GW_NULL_PARAM);

	DEV_INFO_S *dev = NULL;
	dev = get_dev_info(sn);
	RETURN_IF_NULL(dev, GW_NULL_PARAM);

	if(CONN_TYPE_WIFI != dev->conn_type)
	{
		if(GW_OK !=cdc_slave_device_offline(sn))
		{
			return GW_ERR;
		}
		mqtt_slave_offline(sn);
		// user callback
		if(NULL != offline_cb)
		{
			offline_cb(sn);
		}
		report_gw_status();
	}
	
	// del from dev list
	ret = del_dev(sn);
	FREE_POINTER(dev);
	
	return ret;
}

int dev_report(char *sn, char *data_json)
{
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data_json, GW_NULL_PARAM);

	// alarm 

	// report status
	cdc_dev_report_status(sn, data_json);

	// mqtt report
	mqtt_report_status(data_json, strlen(data_json), 0);
	
	return GW_OK;
}

/**
* Handle the mqtt message to zigbee controll
* @param msg,len,resp, resp_len
* @return 
*/
int process_mqtt_msg(char *msg, int len, char **resp, int *resp_len)
{
	RETURN_IF_NULL(msg, GW_NULL_PARAM);
	RETURN_IF_NULL(resp, GW_NULL_PARAM);
	LogI_Prefix("msg is %s\r\n",msg);

	int ret = GW_ERR;
	cJSON *root = cJSON_Parse(msg);
	RETURN_IF_NULL(root, GW_NULL_PARAM);
	cJSON *sn = cJSON_GetObjectItem(root, "deviceId");
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	ret = ctl_dev(sn->valuestring, msg);
	cJSON *resp_root = cJSON_CreateObject();
	cJSON_AddNumberToObject(resp_root, "devtype", 0);
	cJSON_AddNumberToObject(resp_root, "serviceid", 6);
	cJSON_AddNumberToObject(resp_root, "cmd", 0);
	cJSON_AddNumberToObject(resp_root, "point", 0);
	cJSON_AddNumberToObject(resp_root, "ret", ret);
	cJSON_AddStringToObject(resp_root, "deviceId", sn->valuestring);
	*resp = cJSON_PrintUnformatted(resp_root);
	*resp_len = strlen(*resp);
	cJSON_Delete(root);
	cJSON_Delete(resp_root);
	
	return ret;
}

int process_http_msg(uint8_t *buf, int size, char *sn, MSG_TYPE_E type)
{
	return http_send(sn, type, (char*)buf, size);
}

