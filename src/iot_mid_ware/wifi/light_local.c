#include "local_service.h"
#include "rex_common.h"
#include "dev_mng.h"
#include "gw_service.h"
#include "error_code.h"
#include "iot_mid_ware.h"

void local_online_callback(void *param, ipp_local_device *device)
{
	DEV_INFO_S dev;
	memset(&dev, 0, sizeof(dev));
	strncpy(dev.sn, device->guid, sizeof(dev.sn));
	
	strncpy(dev.product_id, device->product_id, sizeof(dev.product_id));
	strncpy(dev.mac, device->mac, sizeof(dev.mac));
	dev.is_master = TRUE;
	dev.conn_type = CONN_TYPE_WIFI;
	strncpy(dev.gw_sn, (char *)device->gateway_guid, sizeof(dev.gw_sn));
	strncpy(dev.soft_ver, device->software_ver, sizeof(dev.soft_ver));
	
	LogI_Prefix("The function 'local_online_callback' is called!\n");
	dev_online(&dev, ONLINE_JOIN_NET);
}

void local_offline_callback(void *param, ipp_local_device *device)
{
	DEV_INFO_S dev;
	memset(&dev, 0, sizeof(dev));
	strncpy(dev.sn, device->guid, sizeof(dev.sn));
	strncpy(dev.product_id, device->product_id, sizeof(dev.product_id));
	strncpy(dev.mac, device->mac, sizeof(dev.mac));
	dev.is_master = TRUE;
	dev.conn_type = CONN_TYPE_WIFI;
	strncpy(dev.gw_sn, (char *)device->gateway_guid, sizeof(dev.gw_sn));
	strncpy(dev.soft_ver, device->software_ver, sizeof(dev.soft_ver));
	
	LogI_Prefix("The function 'local_offline_callback' is called!\n");
	dev_offline(dev.sn);
}

void local_update_callback(void *param, char *sn, int32_t data_len, char *data)
{
	LogI_Prefix("The function 'local_update_callback' is called!\n");
	dev_report(sn, data);
}

ipp_protocol  *local_self_msg_callback(void* param, char *sn, char *msg, int32_t len)
{
	DEV_INFO_S *dev = NULL;
	dev = get_dev_info(sn);

	//ctl_dev_adapt(dev->conn_type, sn, msg);
	LogI_Prefix("The function 'local_self_msg_callback' called!\n");
	//��ӡmsg
	LogD_Prefix("print parameter ' msg':%s\n", msg);
	
	FREE_POINTER(dev);
   	return NULL;
}

ipp_protocol  *local_agent_msg_callback(void* param, char *sn, char *msg, int32_t len)
{
    return local_self_msg_callback(param, sn, msg, len);
}

void start_localservice_module()
{
	ipp_local_device profile; 

	memset(&profile, 0, sizeof(ipp_local_device));
	
	profile.guid = "MY2w3DRvmacaT0402test008"; 
	profile.product_id = "SLIFE_GW0001-7406593c56";
	profile.software_ver = "V0.0.1";
	profile.device_type = 0;
	profile.connet_type = 0;
	profile.name = "TV Gateway";
	profile.mac = "A1:B1:C1:D1:E1:F1";
	profile.is_master = 1;
	profile.gateway_guid = "MY2w3DRvmacaT0402test008";
	
	local_service_init(&profile);
	reg_local_device_online_handler((char *)"local_online", NULL, local_online_callback);
	reg_local_device_offline_handler((char *)"local_offline", NULL, local_offline_callback);
	reg_local_event_handler((char *)"local_update", NULL, local_update_callback);
	reg_local_message_handler(NULL, local_self_msg_callback);
	reg_local_agent_message_handler(NULL, local_agent_msg_callback);

	localservice_start(ipp_device_both, 4 * 1024, 0);

	return;
}