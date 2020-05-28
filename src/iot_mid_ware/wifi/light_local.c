#include "local_service.h"
#include "rex_common.h"
#include "dev_mng.h"
#include "gw_service.h"
#include "error_code.h"
#include "iot_mid_ware.h"
#include "light_local.h"

static LOCAL_MSG_PROC_FUNC local_msg_proc_func = NULL;

void local_online_callback(void *param, ipp_local_device *device)
{
	RETURN_VOID_IF_NULL(device);
	RETURN_VOID_IF_NULL(device->guid);
	RETURN_VOID_IF_NULL(device->mac);
	RETURN_VOID_IF_NULL(device->product_id);
	RETURN_VOID_IF_NULL(device->gateway_guid);
	RETURN_VOID_IF_NULL(device->software_ver);
	DEV_INFO_S dev;
	memset(&dev, 0, sizeof(dev));
	strncpy(dev.sn, device->guid, sizeof(dev.sn));
	
	strncpy(dev.product_id, device->product_id, sizeof(dev.product_id));
	strncpy(dev.mac, device->mac, sizeof(dev.mac));
	dev.is_master = device->is_master;
	dev.conn_type = CONN_TYPE_WIFI;
	strncpy(dev.gw_sn, (char *)device->gateway_guid, sizeof(dev.gw_sn));
	strncpy(dev.soft_ver, device->software_ver, sizeof(dev.soft_ver));
	LogI_Prefix("Local device join net:is_master = %d, conn_type = %d, sn = %s, product_id = %s, mac = %s, soft_ver = %s, gw_sn = %s\r\n",   \
	dev.is_master, dev.conn_type, dev.sn, dev.product_id, dev.mac, dev.soft_ver, dev.gw_sn);
	
	dev_online(&dev, ONLINE_JOIN_NET);
}

void local_offline_callback(void *param, ipp_local_device *device)
{
	RETURN_VOID_IF_NULL(device);
	RETURN_VOID_IF_NULL(device->guid);
	LogI_Prefix("Local device offline:guid = %s\r\n", device->guid);
	dev_offline(device->guid);
}

void local_update_callback(void *param, char *sn, int32_t data_len, char *data)
{
	/*RETURN_VOID_IF_NULL(data);
	RETURN_VOID_IF_NULL(sn);
	if(0 >= data_len)
	{
		LogE_Prefix("%s\r\n", "date_len illegal");
		return;
	}

	char * data_str = (char *)malloc(data_len);
	if(NULL == data_str)
	{
		LogE_Prefix("%s\n", "data str malloc failed\r\n");
		return ;
	}
	memset(data_str, 0, data_len);
	strncpy(data_str, data, data_len);
	LogI_Prefix("update recv msg: %s\r\n", data_str);
	dev_report(sn, data_str);
	FREE_POINTER(data_str);*/
}

ipp_protocol  *local_self_msg_callback(void* param, char *sn, char *msg, int32_t len)
{
	RETURN_NULL_IF_NULL(sn);
	RETURN_NULL_IF_NULL(msg);

	LogI_Prefix("===============Local Recieved MSG===============\n");
	LogI_Prefix("Local control msg: sn = %s, msg = %s, len = %d\n", sn, msg, len);

	char *resp = NULL;
	int resp_len = 0;
	ipp_protocol *resp_protocol = NULL;

	if(NULL != local_msg_proc_func)
	{
		local_msg_proc_func(msg, len, &resp, &resp_len);
		if(NULL != resp)
		{
			resp_protocol = create_protocol();
			if(NULL != resp_protocol)
			{
				put_raw(resp_protocol, resp, resp_len);
			}
			FREE_POINTER(resp);
		}
	}

   	return resp_protocol;
}

ipp_protocol  *local_agent_msg_callback(void* param, char *sn, char *msg, int32_t len)
{
    return NULL;
}

int local_ctrl_dev(char *sn, char *msg, int len)
{
	char *resp = NULL;
	int resp_len = 0;
	local_control_result ret = control_local_device(sn, msg, len, &resp, &resp_len);
	if(NULL != resp)
	{
		LogI_Prefix("Local control response:%s\n", resp);
		FREE_POINTER(resp);
	}

	return (control_success == ret)?GW_OK:GW_ERR;
}

void start_light_local(LOCAL_MSG_PROC_FUNC cb)
{
	RETURN_VOID_IF_NULL(cb);
	
	DEV_INFO_S *gw = get_gw_info();
	RETURN_VOID_IF_NULL(gw);

	local_msg_proc_func = cb;

	ipp_local_device profile; 
	memset(&profile, 0, sizeof(ipp_local_device));
	profile.guid = gw->sn; 
	profile.product_id = gw->product_id;
	profile.software_ver = gw->soft_ver;
	profile.device_type = gw->conn_type;
	profile.connet_type = gw->conn_type;
	profile.name = "TV GW";
	profile.mac = gw->mac;
	profile.is_master = 1;
	profile.gateway_guid = gw->sn;
	
	local_service_init(&profile);
	reg_local_device_online_handler((char *)"local_online", NULL, local_online_callback);
	reg_local_device_offline_handler((char *)"local_offline", NULL, local_offline_callback);
	reg_local_event_handler((char *)"local_update", NULL, local_update_callback);
	reg_local_message_handler(NULL, local_self_msg_callback);
	reg_local_agent_message_handler(NULL, local_agent_msg_callback);

	localservice_start(ipp_device_both, 4 * 1024, 0);
}