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
#include "pthread.h"
#include "scene_panel.h"

#define TIME 6

static pthread_t time_pid;
static int flag = USB_ADD;
static int status = USB_ADD;

static ONLINE_CALLBACK online_cb;
static OFFLINE_CALLBACK offline_cb;
static ALARM_CALLBACK alarm_cb;
static ZIGBEE_CALLBACK zigbee_cb;


//Stub for x86_64 linux debug 
#ifndef ZIGBEE_ENABLE
int zigbee_network_query(void)
{
	return GW_OK;
}

int usb_dev_query(void)
{
	return GW_OK;
}

int init_rex(void)
{
	return GW_OK;
}
void reg_usb_port_callback(USB_PORT_CHECK cb)
{
	return;
}
#endif

void dev_list_handle()
{
	DEV_INFO_S *dev_list = NULL;
	int dev_num = 0;
	get_dev_list(CONN_TYPE_ZIGBEE, &dev_list, &dev_num);

	if(dev_num != 0)
	{
		LogE_Prefix("zigbee device num is %d!\n", dev_num);

		int i;
		for(i = 0; i < dev_num; i++)
		{
			if(status == USB_ADD)
			{
				if(cdc_slave_device_online(&dev_list[i]) != 0)
				{
					LogE_Prefix("device(%s) online failed!\n", dev_list[i].sn);
				}
			}
			else
			{
				if(cdc_slave_device_offline(dev_list[i].sn) != 0)
				{
					LogE_Prefix("device(%s) offline failed!\n", dev_list[i].sn);
				}
			}
		}
	}
	else
	{
		LogI_Prefix("dev list has no zigbee dev!\n");
	}
	FREE_POINTER(dev_list);
}

void *time_func(void *arg)
{
	LogI_Prefix("time_func start!\n");
	int tmp_flag = flag;
	int time  = TIME;
	while(time > 0)
	{
		if(tmp_flag != flag)
		{
			tmp_flag = flag;
			time = TIME;
		}
		else
		{
			time--;
			sleep_s(1);
		}
	}

	if(tmp_flag != status)
	{
		status = tmp_flag;
		dev_list_handle();
	}
	time_pid = 0;
	LogI_Prefix("time_func end!\n");
	return NULL;
}

void usb_port_callback(USB_PORT_STA_E sta)
{ 	
	flag = sta;
	if( time_pid == 0)
	{
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&time_pid, &attr, time_func, NULL);
	}
}

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

void reg_zigbee_callback(ZIGBEE_CALLBACK cb)
{
	zigbee_cb = cb;
}

int dev_offline_callback(char *sn)
{
	DEV_INFO_S *dev = NULL;
	if(!(dev = get_dev_info((char *)sn)))
	{
		LogE_Prefix("%s\n", "Do not find the device!");
		return GW_HTTP_FAIL;
	}
	int ret = cdc_slave_device_offline(sn);
	if(NULL != offline_cb)
	{
		offline_cb(dev->sn, dev->product_id);
	}
	FREE_POINTER(dev);
	return ret;
}

void dev_zigbee_callback(int flag)
{
	if(NULL != zigbee_cb)
	{
		zigbee_cb(flag);
	}
}

int dev_report_callback(char *sn, uint32_t ep_id, uint16_t data_type, char *data)
{
	int ret = device_data_report(sn, ep_id, data_type, data);
	if(NULL != alarm_cb)
	{
		alarm_cb(sn, data);
	}
	return ret;
}

int report_gw_status(void)
{
	int ret = GW_ERR;
	cJSON *root = cJSON_CreateObject();
	RETURN_IF_NULL(root, GW_NULL_PARAM);
	cJSON *status = cJSON_CreateObject();
	if(NULL != status)
	{
		int zigbee_num = get_child_num(CONN_TYPE_ZIGBEE);
		int dongle_status  = (GW_OK == usb_dev_query())?1:0;
		int zigbee_status = (GW_OK == zigbee_network_query())?1:0;
		cJSON_AddStringToObject(root, "devid", get_gw_info()->sn);
		cJSON_AddNumberToObject(root, "serviceid", 2);
		cJSON_AddNumberToObject(root, "cmd", 0);
		cJSON_AddNumberToObject(root, "devtype", 0);
		cJSON_AddStringToObject(root, "productid", get_gw_info()->product_id);
		cJSON_AddNumberToObject(status, "zigbee_num", zigbee_num);
		cJSON_AddNumberToObject(status, "dongle", dongle_status);
		cJSON_AddNumberToObject(status, "zigbee", zigbee_status);
		cJSON_AddItemToObject(root, "status", status);
		char *data_json = cJSON_PrintUnformatted(status);
		if(NULL != data_json)
		{
			ret = cdc_dev_report_status(get_gw_info()->sn, data_json, FALSE);
			FREE_POINTER(data_json);
			data_json = cJSON_PrintUnformatted(root);
			if(NULL != data_json)
			{
				// mqtt report
				mqtt_report_status(data_json, strlen(data_json), 0);
				FREE_POINTER(data_json);
			}
		}
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
				online_cb(dev->sn, dev->product_id);
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
			offline_cb(sn, dev->product_id);
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
	BOOL multi = FALSE;
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data_json, GW_NULL_PARAM);
	
	DEV_INFO_S *dev = get_dev_info(sn);
	RETURN_IF_NULL(dev, GW_NULL_PARAM);

	// alarm 

	// report status
	if(CONN_TYPE_ZIGBEE == dev->conn_type)
	{
		if(DEV_TWOWAY_LIGHT == dev->child_info.zigbee.type || DEV_THREEWAY_LIGHT == dev->child_info.zigbee.type ||
		   DEV_FOURWAY_LIGHT == dev->child_info.zigbee.type || DEV_TWOWAY_CURTAIN == dev->child_info.zigbee.type ||
		   DEV_SCENE_PANEL == dev->child_info.zigbee.type)
		{
			multi = TRUE;
		}
	}
	cdc_dev_report_status(sn, data_json, multi);

	// mqtt report
	cJSON *root = cJSON_CreateObject();
	RETURN_IF_NULL(root, GW_NULL_PARAM);
	cJSON *status = cJSON_Parse(data_json);
	if(NULL != status)
	{
		cJSON_AddStringToObject(root, "devid", dev->sn);
		cJSON_AddNumberToObject(root, "serviceid", 2);
		cJSON_AddNumberToObject(root, "cmd", 0);
		cJSON_AddNumberToObject(root, "devtype", dev->conn_type);
		cJSON_AddStringToObject(root, "productid", dev->product_id);
		cJSON_AddItemToObject(root, "status", status);
		char *data = cJSON_PrintUnformatted(root);
		if(NULL != data)
		{
			mqtt_report_status(data, strlen(data), 0);
			FREE_POINTER(data);
		}
		FREE_POINTER(dev);
	}
	cJSON_Delete(root);
	FREE_POINTER(dev);
	
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
	cJSON *root = cJSON_Parse(msg);
	RETURN_IF_NULL(root, GW_NULL_PARAM);
	int ret = GW_ERR;
	int point = 0;
	cJSON *sn = NULL;
	cJSON * option_node = NULL;
	cJSON * point_node = NULL;
	cJSON *status = NULL;
	char *masterchange = NULL;
	cJSON *change = NULL;
	cJSON *msg_info = NULL;
	cJSON *slave = NULL;
	char *dev_msg = NULL;
	cJSON *dev_info = NULL;
	cJSON *devId = NULL;

	status = cJSON_GetObjectItem(root, "status");
	if(NULL != status)
	{
		masterchange = cJSON_PrintUnformatted(status);
		if(NULL != masterchange)
		{	
			change = cJSON_Parse(masterchange);
			RETURN_IF_NULL(change, GW_NULL_PARAM);
			msg_info = cJSON_GetObjectItem(change, "msg");
			if(!strcmp(msg_info->valuestring,"masterchange"))
			{
				slave = cJSON_GetObjectItem(change, "slave");
				if(NULL != slave)
				{
					dev_msg = cJSON_PrintUnformatted(slave);
					if(NULL != dev_msg)
					{
						dev_info = cJSON_Parse(dev_msg);
						RETURN_IF_NULL(dev_info, GW_NULL_PARAM);
						devId = cJSON_GetObjectItem(dev_info, "devId");
						if(NULL != devId)
						{
							del_dev(devId->valuestring);
							ctl_dev_clear_net(devId->valuestring);
						}
						sn = cJSON_GetObjectItem(dev_info, "devId");
						cJSON_Delete(dev_info);
						FREE_POINTER(dev_msg);
					}
				}
			}
			cJSON_Delete(change);
			FREE_POINTER(masterchange);
		}
	}
	else
	{
	    option_node = cJSON_GetObjectItem(root, "option");
		if((option_node != NULL) && (!(strcmp(option_node->valuestring, "update"))))
		{//scene panel update
		    sn = cJSON_GetObjectItem(root, "devId");
			RETURN_IF_NULL(sn, GW_NULL_PARAM);
			point_node = cJSON_GetObjectItem(root, "point");
			RETURN_IF_NULL(point_node, GW_NULL_PARAM);
			point = point_node->valueint;
			ret = update_scene_panel(sn->valuestring, point);
		}
		else
		{//dev ctl
			if((option_node != NULL) && (!(strcmp(option_node->valuestring, "devDel"))))
			{//delete device
		    	sn = cJSON_GetObjectItem(root, "devId");
				RETURN_IF_NULL(sn, GW_NULL_PARAM);
				//del_dev(sn->valuestring);
			}
			else
			{
				sn = cJSON_GetObjectItem(root, "deviceId");
				RETURN_IF_NULL(sn, GW_NULL_PARAM);
			}
			ret = ctl_dev(sn->valuestring, msg);
		}
	}
    cJSON *resp_root = cJSON_CreateObject();
	cJSON_AddNumberToObject(resp_root, "devtype", 0);
	cJSON_AddNumberToObject(resp_root, "serviceid", 6);
	cJSON_AddNumberToObject(resp_root, "cmd", 0);
	cJSON_AddNumberToObject(resp_root, "point", 0);
	cJSON_AddNumberToObject(resp_root, "ret", ret);
	if(NULL!= sn)
	{
		cJSON_AddStringToObject(resp_root, "deviceId", sn->valuestring);
	}
	*resp = cJSON_PrintUnformatted(resp_root);
	*resp_len = strlen(*resp);
	cJSON_Delete(root);
	cJSON_Delete(resp_root);
	return ret;
}

/**
* Handle the local message to zigbee controll
* @param msg,len,resp, resp_len
* @return 
*/
int process_local_msg(char *msg, int len, char **resp, int *resp_len)
{
	return process_mqtt_msg(msg, len, resp, resp_len);
}

int process_http_msg(uint8_t *buf, int size, char *sn, MSG_TYPE_E type)
{
	return http_send(sn, type, (char*)buf, size);
}

