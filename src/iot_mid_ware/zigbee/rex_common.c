#include "rex_common.h"
#include "rex_export_gateway.h"
#include "rex_export_type.h"
#include "rex_call_back.h"
#include "rex_uart.h"
#include "platform.h"
#include "cJSON.h"
#include "dev_mng.h"
#include "error_code.h"
#include "gw_service.h"
#include "string_processor.h"
#include "iot_mid_ware.h"


static void setSensor(DEV_INFO_S *ptDevice, const char *sn, const char *productID, uint16_t type, uint64_t laddr);
static int prase_net_info(char *sn, uint32_t ep_id, char *data);
static int prase_light_info(char *sn, uint32_t ep_id, char *data);
static int prase_curtain_info(char *sn, uint32_t ep_id, char *data);
static int prase_air_info(char *sn, uint32_t ep_id, char *data);
static int prase_network_online(char *sn, uint32_t ep_id, char *data);

static int g_network = -1;



/**
 * init_rex_sdk
 * SDK启动函数
 * return: 0成功,否则-1
 * note: none
 */
int init_rex_sdk(char *name)
{
	RETURN_IF_NULL(name, GW_NULL_PARAM);
	
	//SDK初始化处理
	if(rex_init_process(MAIN_LOG_LEVEL, MAIN_LOG_MAX_SIZE, MAIN_TRACK_LEVEL) != 0)
	{
	    LogE_Prefix("Failed to rex_init \r\n");
		return GW_ERR;
	}
    //设置串口
	if(rex_set_serial_port(name, MAIN_SERIAL_BAUD_RATE) != 0)
	{
		LogE_Prefix("Failed to rex_set_serial \r\n");
		return GW_ERR;
	}
    //设置OTA镜像文件路径
    if(rex_set_ota_image_file_path(MAIN_OTA_IMAGE_FILE_PATH) != 0)
    {
        LogE_Prefix("Failed to rex_set_ota \r\n");
    	return GW_ERR;
    }
    //设置设备入网回调函数
    if(rex_set_callback(REX_DEVICE_JOIN, call_back_device_join_cb) != 0)
    {
        LogE_Prefix("Failed to device_join_cb \r\n");
    	return GW_ERR;
    }
    //设置设备离网回调函数
    if(rex_set_callback(REX_DEVICE_LEAVE, call_back_device_leave_cb) != 0)
    {
        LogE_Prefix("Failed to device_leave_cb \r\n");
    	return GW_ERR;
    }
	//设置上报状态数据回调函数
    if(rex_set_callback(REX_REPORT_STATE_DATA, call_back_report_state_data_cb) != 0)
    {
        LogE_Prefix("Failed to report_state_data_cb \r\n");
    	return GW_ERR;
    }
	//设置上报事件数据回调函数
    if(rex_set_callback(REX_REPORT_EVENT_DATA, call_back_report_event_data_cb) != 0)
    {
        LogE_Prefix("Failed to report_event_data_cb \r\n");
    	return GW_ERR;
    }
	//设置上报自定义数据回调函数
    if(rex_set_callback(REX_REPORT_SELF_DEFINING_DATA, call_back_report_self_defining_data_cb) != 0)
    {
        LogE_Prefix("Failed to defining_data_cb \r\n");
    	return GW_ERR;
    }
    reg_dev_ctl_callback(CONN_TYPE_ZIGBEE, ctl_dev_adapt);
    //开始运行SDK
    if(rex_start() != 0)
    {
        LogE_Prefix("Failed to rex_start \r\n");
	    return GW_ERR;
    }

	return GW_OK;
}

/**
 * device_data_report
 * 构建上报数据函数
 * sn[in]: 设备地址
 * ep_id[in]: 设备端ID号,默认为1,多路设备从1依次递增
 * data_type[in]: 状态种类
 * data[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
int device_data_report(char *sn, uint32_t ep_id, uint16_t data_type, char *data)
{
	int ret = GW_OK;
	DEV_INFO_S *ptr = NULL;

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data, GW_NULL_PARAM);

	//sn地址存在则上报zigbee数据，不存在则不上报
	ptr = get_dev_info(sn);
	if(ptr != NULL)
	{
		ptr->overtime = ptr->maxtime;
		LogD_Prefix("device report data: sn = %s, data_type = 0x%04X, data = %s\r\n", sn, data_type, data);
		switch(data_type)
		{
			case NET_INFO:
			{
	     		//网关上线
	 			ret = prase_net_info(sn, ep_id, data);
				break;
			}
			case PLUG_INFO:
			case LIGHT_INFO:
			{
				//灯开关上报
				ret = prase_light_info(sn, ep_id, data);
				break;
			}
			case CURTAIN_INFO:
			{
				//窗帘上报
				ret = prase_curtain_info(sn, ep_id, data);
				break;
			}
			case AIR_INFO:
			{
				//面板空调上报
				ret = prase_air_info(sn, ep_id, data);
				break;
			}
			case ONLINE_EVENT:
			{
				/* 断电上线 */
				ret = device_online_report(sn, ONLINE_LONG);
				break;
			}
			default:
				break;
		}
	}
	else
	{
		/* 网关第一次上线加入链表 */
		if(data_type == NET_INFO)
		{
			LogD_Prefix("======gateway is first online======\r\n");
			ret = prase_network_online(sn, ep_id, data);
		}
		else
		{
			LogE_Prefix("sn: %s is no exist\r\n", sn);
			return GW_ERR;
		}
	}
	
	FREE_POINTER(ptr);
	
	return ret;
}

/**
 * call_back_report_state_data_cb
 * 下发控制数据函数
 * data[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
int controlZigbeeDevice(char *sn, uint32_t ep_id, USER_CONTROL_OPTION_E type, DItem pDes)
{
	unsigned short cmd = 0;
	char *dev_id = sn;
	char conmand_message[JSON_MAX_LEN];

	memset(conmand_message, 0, JSON_MAX_LEN);

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(pDes, GW_NULL_PARAM);

	switch(type)
	{
		case user_get_net:
		{
			dev_id="0000000000000000";
			cmd = ZIGBEE_GET_NET;
			snprintf(conmand_message, JSON_MAX_LEN, "{}");
			break;
		}
		case user_join_net:
		{
			dev_id="0000000000000000";
			cmd = ZIGBEE_JOIN_NET;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Duration\": \"%d\"}", pDes->times);
			break;
		}
		case user_creat_net:
		{
			dev_id="0000000000000000";
			cmd = ZIGBEE_CREAT_NET;
			snprintf(conmand_message, JSON_MAX_LEN, "{}");
			break;
		}
		//case USER_SET_FACTOR:
		case user_curtain_on:
		case user_curtain_close:
		case user_curtain_stop:
		{
			cmd = ZIGBEE_CURTAIN_SET_OPERATE;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Operate\":\"%d\"}", pDes->state);
			break;
		}	
		case user_curtain_goto_percent:
		{
			cmd = ZIGBEE_CURTAIN_SET_LEVEL;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Level\":\"%d\"}", pDes->level);
			break;
		}
		case user_light_turn_on:
		case user_light_turn_off:
		{
			cmd = ZIGBEE_LIGHT_SET_STATE;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"State\":\"%d\"}", pDes->state);
			break;
		}
		case user_light_change_level:
		{
			cmd = ZIGBEE_LIGHT_SET_LEVEL;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Level\":\"%d\"}", pDes->level);
			break;
		}
		case user_plug_turn_on:
		case user_plug_turn_off:
		{
			cmd = ZIGBEE_PLUG_SET_STATE;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"State\":\"%d\"}", pDes->state);
			break;
		}
		case user_clear_device:
		{
			cmd = ZIGBEE_CLEAR_DEVICE;
			snprintf(conmand_message, JSON_MAX_LEN, "{}");
			break;
		}
		case user_clear_all_devices:
		{

			dev_id="FFFFFFFFFFFFFFFF";
			cmd = ZIGBEE_CLEAR_DEVICE;
			snprintf(conmand_message, JSON_MAX_LEN, "{}");
			break;
		}
		case user_creat_groupid:
		{
			cmd = ZIGBEE_MANAGE_GROUID;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Action\":\"A\",\"GroupId\":\"%d\"}", pDes->groud_id);
			break;
		}
		case user_delete_grouid:
		{
			cmd = ZIGBEE_MANAGE_GROUID;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Action\":\"D\",\"GroupId\":\"%d\"}", pDes->groud_id);
			break;
		}
		case user_delete_all_grouid:
		{
			cmd = ZIGBEE_MANAGE_GROUID;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Action\":\"C\",\"GroupId\":\"\"}");
			break;
		}
		case user_query_grouid:
		{
			cmd = ZIGBEE_MANAGE_GROUID;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Action\":\"Q\",\"GroupId\":\"\"}");
			break;
		}
		case user_creat_scene:
		{
			cmd = ZIGBEE_MANAGE_SCENE;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Action\":\"A\",\"GroupId\":\"%d\",\"SceneId\":\"%d\"}", pDes->groud_id, pDes->scene_id);
			break;
		}
		case user_delete_scene:
		{
			cmd = ZIGBEE_MANAGE_SCENE;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Action\":\"D\",\"GroupId\":\"%d\",\"SceneId\":\"%d\"}", pDes->groud_id, pDes->scene_id);
			break;
		}
		case user_delete_all_scene:
		{
			cmd = ZIGBEE_MANAGE_SCENE;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Action\":\"C\",\"GroupId\":\"%d\",\"SceneId\":\"\"}", pDes->groud_id);
			break;
		}
		case user_run_scene:
		{
			cmd = ZIGBEE_MANAGE_SCENE;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Action\":\"E\",\"GroupId\":\"%d\",\"SceneId\":\"%d\"}", pDes->groud_id, pDes->scene_id);
			break;
		}
		case user_query_scene:
		{
			cmd = ZIGBEE_MANAGE_SCENE;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Action\":\"Q\",\"GroupId\":\"%d\",\"SceneId\":\"\"}", pDes->groud_id);
			break;
		}
		case user_device_info:
		case user_device_status:
		case user_device_bind:
		case user_call_device:
		case user_sync_clock:
		default:
			break;
	}

	LogD_Prefix("begin to send command_data:\r\n  sn = %s\r\n  ep_id = %d\r\n  cmd = %d\r\n  conmand_message = %s\r\n", sn, ep_id, cmd, conmand_message);
	
	rex_send_command_data(dev_id, ep_id, cmd, conmand_message);

	return GW_OK;
}

/**
 * device_online_report
 * 设备上报入网消息函数Started dev_offline
 * sn[in]: 设备地址
 * type[in]: 设备类型
 * return: 0成功,否则-1
 * note: none
 */
int device_join_net_report(char *sn, char *type)
{
	DEV_INFO_S *parameters = NULL;
	char *productID = NULL;

	unsigned short dev_type = 0;
	unsigned char dev_type_byte[2] = {0, 0};

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(type, GW_NULL_PARAM);

	string_processor_hex_str_to_byte((char const *)type, 4, dev_type_byte);
	dev_type = dev_type_byte[0] << 8 | dev_type_byte[1];

	parameters = (DEV_INFO_S*)malloc(sizeof(DEV_INFO_S));
	RETURN_IF_NULL(parameters, GW_MALLOR_FAIL);
	memset(parameters, 0, sizeof(DEV_INFO_S));

	switch(dev_type)
	{
		case DEV_GATEWAY:
		{
			productID = DEV_PRODUCT_GATEWAY;
			break;
		}
		case DEV_ONEWAY_PLUG:
		{
			productID = DEV_PRODUCT_ONEWAY_PLUG;
			break;
		}
		case DEV_TWOWAY_PLUG:
		case DEV_THREEWAY_PLUG:
		case DEV_FOURWAY_PLUG:
		{
			break;
		}
		case DEV_ONEWAY_LIGHT:
		{
			productID = DEV_PRODUCT_ONEWAY_LIGHT;
			break;
		}
		case DEV_TWOWAY_LIGHT:
		{
			productID = DEV_PRODUCT_TWOWAY_LIGHT;
			break;
		}
		case DEV_THREEWAY_LIGHT:
		{
			productID = DEV_PRODUCT_THREEWAY_LIGHT;
			break;
		}
		case DEV_FOURWAY_LIGHT:
		{
			break;
		}
		case DEV_ONEWAY_CURTAIN:
		{
			productID = DEV_PRODUCT_ONEWAY_CURTAIN;
			break;
		}
		case DEV_TWOWAY_CURTAIN:
		case DEV_THREEWAY_CURTAIN:
		case DEV_FOURWAY_CURTAIN:
		case DEV_FIVEWAY_CURTAIN:
		case DEV_SIXWAY_CURTAIN:
		{
			break;
		}
		case DEV_SIXWAY_AIR:
		{
			productID = DEV_PRODUCT_SIXWAY_AIR;
			break;
		}
		default:
			break;
	}
	RETURN_IF_NULL(productID, GW_NULL_PARAM);
	setSensor(parameters, sn, productID, dev_type, 0);
	LogD_Prefix("device join net:sn = %s ,productID = %s ,dev_type = 0x%04X\r\n", parameters->sn, parameters->product_id, parameters->child_info.zigbee.type);

	//设备入网上报
	dev_online(parameters, ONLINE_JOIN_NET);

	FREE_POINTER(parameters);

	return GW_OK;
}

/**
 * device_online_report
 * 设备上报上线消息函数
 * sn[in]: 设备地址
 * type[in]: 设备类型
 * return: 0成功,否则-1
 * note: none
 */
int device_online_report(char *sn, DEVICE_ONLINE_TYPE_E type)
{
	DEV_INFO_S *tmp = NULL;
	
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	
	if(type == ONLINE_CHANG)  //超出距离后上线
	{
		
	}
	else if(type == ONLINE_LONG)  //断电后上线
	{
		tmp = get_dev_info(sn);
		RETURN_IF_NULL(tmp, GW_NULL_PARAM);
		LogD_Prefix("the device is online after offline\r\n");
		dev_online(tmp, ONLINE_AFTER_OFFLINE);
		FREE_POINTER(tmp);
	}
	return GW_OK;
}

/**
 * device_offline_report
 * 设备上报离网消息函数
 * sn[in]: 设备地址
 * return: 0成功,否则-1
 * note: none
 */
int device_offline_report(char *sn)
{
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	LogD_Prefix("device_offline is: %s\n", sn);
	//设备离网上报
	dev_offline(sn);
	
	return GW_OK;
}

/**
 * device_offline_report
 * 查询zigbee是否组网函数
 * return: 0创建网络,否则-1
 * note: none
 */
int zigbee_network_query(void)
{
	int ret = 0;
	char *dev_id = "0000000000000000";
	char cmd[JSON_MAX_LEN];

	memset(cmd, 0, JSON_MAX_LEN);
	snprintf(cmd, JSON_MAX_LEN, "{}");

	if(GW_OK == rex_send_command_data(dev_id, 1, ZIGBEE_GET_NET, cmd))
	{
		sleep_ms(1 * 1000);
		LogD_Prefix("query zigbee net is working\n");
		ret = g_network;
	}

	return ret;
}

/**
 * setSensor
 * 构建数据函数
 * return: 0成功,否则-1
 * note: none
 */
static void setSensor(DEV_INFO_S *ptDevice, const char *sn, const char *productID, uint16_t type, uint64_t laddr)
{
	uint8_t mac[6]={0};
	memset(ptDevice, 0, sizeof(DEV_INFO_S));
	agent_strncpy(ptDevice->sn, sn, strlen(sn));
	agent_strncpy(ptDevice->product_id, productID, strlen(productID));
	ptDevice->child_info.zigbee.type = type;
	ptDevice->child_info.zigbee.long_addr = laddr;
	string_processor_hex_str_to_byte(sn, strlen(sn), mac);
	snprintf(ptDevice->mac, sizeof(ptDevice->mac), "%02x:%02x:%02x:%02x:%02x:%02x", \
				mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
	ptDevice->conn_type = CONN_TYPE_ZIGBEE;
	agent_strncpy(ptDevice->soft_ver, "1.1.01", strlen("1.1.01"));
	agent_strncpy(ptDevice->gw_sn, get_gw_info()->sn, strlen(get_gw_info()->sn));
	agent_strncpy(ptDevice->ip,get_gw_info()->ip , strlen(get_gw_info()->ip));
	ptDevice->maxtime = VC_MAX_TIME;
	ptDevice->overtime= ptDevice->maxtime;
}

/**
 * prase_network_online
 * 解析网络信息上线函数
 * sn[in]: 设备地址
 * data[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
static int prase_network_online(char *sn, uint32_t ep_id, char *data)
{
	uint64_t panid = 0;
	cJSON *root;
	cJSON *pid;
	DEV_INFO_S *ptr = NULL;
	char *pro_id = NULL;
	
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data, GW_NULL_PARAM);

	root = cJSON_Parse(data);
	if(root)
	{
		ptr = (DEV_INFO_S*)malloc(sizeof(DEV_INFO_S));
		RETURN_IF_NULL(ptr, GW_MALLOR_FAIL);
		memset(ptr, 0, sizeof(DEV_INFO_S));

		pid = cJSON_GetObjectItem(root, "ExtPanId");
		if(pid)
		{
			if(0 == strcmp(pid->string, "ExtPanId"))
			{
				panid = atol(pid->valuestring);
				pro_id = DEV_PRODUCT_GATEWAY;
				setSensor(ptr, sn, pro_id, DEV_GATEWAY, panid);
				add_dev(ptr, 0);
				LogD_Prefix("============== device online is ok ==============\n ");
				//dev_online(ptr, ONLINE_JOIN_NET);
			}
		}
	}

	FREE_POINTER(ptr);
	cJSON_Delete(root);

	return GW_OK;
}

/**
 * prase_net_info
 * 解析网络信息数据函数
 * sn[in]: 设备地址
 * data[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
static int prase_net_info(char *sn, uint32_t ep_id, char *data)
{
	uint8_t ch = 0;
	cJSON *root;
	cJSON *channel;

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data, GW_NULL_PARAM);

	root = cJSON_Parse(data);
	if(root)
	{
		channel = cJSON_GetObjectItem(root, "Channel");
		if(channel)
		{
			if(0 == strcmp(channel->string, "Channel"))
			{
				ch = atol(channel->valuestring);
				if(ch == 255)
					g_network = -1;//zigbee没有创建网路
				else
					g_network = 0;//zigbee创建网路
			}
		}
	}
	else
	{
    	LogE_Prefix("Failed to load json command[%s]\r\n", data);
    	cJSON_Delete(root);
    	return GW_ERR;
	}

	LogD_Prefix("============== device g_network is %d ==============\n ", g_network);
	
	cJSON_Delete(root);
		
	return GW_OK;
}

/**
 * prase_light_info
 * 解析灯开关数据函数
 * sn[in]: 设备地址
 * data[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
static int prase_light_info(char *sn, uint32_t ep_id, char *data)
{
	uint8_t istatus = 0;
	cJSON *root, *state;
	char json_value[JSON_MAX_LEN];

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data, GW_NULL_PARAM);

	root = cJSON_Parse(data);
	if(root)
	{
		state = cJSON_GetObjectItem(root, "State");
		if(state)
		{
			if(0 == strcmp(state->string, "State"))
			{
				istatus = atoi(state->valuestring);
				memset(json_value, 0, JSON_MAX_LEN);
				snprintf(json_value, JSON_MAX_LEN, "{\"devid\":\"%s\",\"power\":%d,\"point\":%d}", sn, istatus, ep_id);
				LogD_Prefix("dev_report:sn = %s, json_value = %s\r\n", sn, json_value);
				dev_report(sn, json_value);
			}
		}
		cJSON_Delete(root);
		return GW_OK;
	}
	else
	{
    	LogE_Prefix("Failed to load json command[%s]\r\n", data);
    	cJSON_Delete(root);
    	return GW_ERR;
	}
}

/**
 * prase_curtain_info
 * 解析窗帘数据函数
 * sn[in]: 设备地址
 * data[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
static int prase_curtain_info(char *sn, uint32_t ep_id, char *data)
{
	uint8_t istatus = 0, ilevel = 0;
	cJSON *root, *state;
	char json_value[JSON_MAX_LEN];

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data, GW_NULL_PARAM);

	root = cJSON_Parse(data);
	if(root)
	{
		state = cJSON_GetObjectItem(root, "Level");
		if(state)
		{
			if(0 == strcmp(state->string, "Level"))
			{
				ilevel = atoi(state->valuestring);
				if(ilevel > 0)
					istatus = 1;
				else
					istatus = 0;
				memset(json_value, 0, JSON_MAX_LEN);
				snprintf(json_value, JSON_MAX_LEN, "{\"devid\":\"%s\",\"power\":%d,\"level\":%d,\"point\":%d}", \
								sn, istatus, ilevel, ep_id);
				LogD_Prefix("dev_report:sn = %s, json_value = %s\r\n", sn, json_value);	
				dev_report(sn, json_value);
			}
		}
		cJSON_Delete(root);
		return GW_OK;
	}
	else
	{
    	LogE_Prefix("Failed to load json command[%s]\r\n", data);
    	cJSON_Delete(root);
    	return GW_ERR;
	}
}

/**
 * prase_air_info
 * 解析面板空调数据函数
 * sn[in]: 设备地址
 * data[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
static int prase_air_info(char *sn, uint32_t ep_id, char *data)
{
	uint8_t istatus = 0;
	float tmp =  0;
	cJSON *root, *state;
	char json_value[JSON_MAX_LEN];

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data, GW_NULL_PARAM);

	root = cJSON_Parse(data);
	if(root)
	{
		if(strstr(data, "TargetTemperature"))
		{
			state = cJSON_GetObjectItem(root, "TargetTemperature");
			if(state)
			{
				if(0 == strcmp(state->string, "TargetTemperature"))
				{
					tmp = atof(state->valuestring);
					memset(json_value, 0, JSON_MAX_LEN);
					snprintf(json_value, JSON_MAX_LEN, "{\"devid\":\"%s\",\"temperature\":%f,\"point\":%d}", sn, tmp, ep_id);
					LogD_Prefix("dev_report:sn = %s, json_value = %s\r\n", sn, json_value);
					dev_report(sn, json_value);
				}
			}
		}
		else if(strstr(data, "WorkMode"))
		{
			state = cJSON_GetObjectItem(root, "WorkMode");
			if(state)
			{
				if(0 == strcmp(state->string, "WorkMode"))
				{
					istatus = atoi(state->valuestring);
					if(istatus == 10)
						istatus = 1;
					memset(json_value, 0, JSON_MAX_LEN);
					snprintf(json_value, JSON_MAX_LEN, "{\"devid\":\"%s\",\"power\":%d,\"point\":%d}", sn, istatus, ep_id);
					LogD_Prefix("dev_report:sn = %s, json_value = %s\r\n", sn, json_value);
					dev_report(sn, json_value);
				}
			}
		}
		cJSON_Delete(root);
		return GW_OK;
	}
	else
	{
    	LogE_Prefix("Failed to load json command[%s]\r\n", data);
    	cJSON_Delete(root);
    	return GW_ERR;
	}
}

int get_joinnet_state(cJSON *data,char *sn)
{
	cJSON *method;
	cJSON *time;
	int ev_point;
	int cmd_type;
	DataItem pDes;

	memset(&pDes,0,sizeof(DataItem));
    method = cJSON_GetObjectItem(data, "method");
	time = cJSON_GetObjectItem(data, "time");
	if((method!=NULL) && (time !=NULL))
	{
		LogD_Prefix("method->valuestring is %s\r\n", method->valuestring);
		if(0 == strcmp(method->valuestring, "start_zigbee"))
		{
			cmd_type = user_join_net;
		}

		LogD_Prefix("time->valueint is %d\r\n",time->valueint);
		pDes.times = time->valueint;
		ev_point = 1;
		LogD_Prefix("Send msf to Control Zigbee Device1: sn = %s, ev_point = %d, cmd_type = %d, pDes = %d\r\n", sn, ev_point, cmd_type, pDes.times);
		controlZigbeeDevice(sn, ev_point, cmd_type, &pDes);
	}
	return GW_OK;
}

int get_light_state(cJSON *data,char *sn)
{
	cJSON *method;
	cJSON *point;
	int ev_point;
	int cmd_type;
	DataItem pDes;

	memset(&pDes,0,sizeof(DataItem));
	method = cJSON_GetObjectItem(data, "method");
	point= cJSON_GetObjectItem(data, "point");
	if((method!=NULL) && (point !=NULL))
	{
		LogD_Prefix("method->valuestring is %s\r\n", method->valuestring);
		if(0 == strcmp(method->valuestring, "lightOn"))
		{
			cmd_type = user_light_turn_on;
			pDes.state = 1;
		}
		else if(0 == strcmp(method->valuestring, "lightOff"))
		{
			cmd_type = user_light_turn_off;
			pDes.state = 0;
		}
		else
		{
			cmd_type = user_clear_device;
			pDes.state = 255;
		}
		
		ev_point = point->valueint;
		LogD_Prefix("Send msf to Control Zigbee Device1: sn = %s, ev_point = %d, cmd_type = %d, pDes = %d\r\n", sn, ev_point, cmd_type, pDes.state);
	    controlZigbeeDevice(sn, ev_point, cmd_type, &pDes);
	}		
	return GW_OK;
}

int get_plug_state(cJSON *data,char *sn)
{
	cJSON *method;
	cJSON *point;
	cJSON *order;
	int ev_point;
	int cmd_type;
	DataItem pDes;
	
	memset(&pDes,0,sizeof(DataItem));
	method = cJSON_GetObjectItem(data, "method");
	point= cJSON_GetObjectItem(data, "point");
	order = cJSON_GetObjectItem(data, "order");
	if((method!=NULL) && (point !=NULL) && (order != NULL))
	{
		LogI_Prefix("method->valuestring is %s\r\n", method->valuestring);
		if(0 == strcmp(method->valuestring, "socketOnOff"))
		{
			switch(order->valueint)
			{
				case 0:
					cmd_type = user_plug_turn_off;
					pDes.state = 0;
					break;
				case 1:
					cmd_type = user_plug_turn_on;
					pDes.state = 1;
					break;
				case 255:
					cmd_type = user_clear_device;
					pDes.state = 255;
					break;
				default:
					LogD_Prefix("Control Zigbee cmd is not found,return\r\n");
					return GW_OK ;
					break;
			}
		}
	    
		ev_point = point->valueint;
		LogD_Prefix("Send msf to Control Zigbee Device1: sn = %s, ev_point = %d, cmd_type = %d, pDes = %d\r\n", sn, ev_point, cmd_type, pDes.state);
	    controlZigbeeDevice(sn, ev_point, cmd_type, &pDes);
	}
	
	return GW_OK;
}

int get_curtain_state(cJSON *data,char *sn)
{
	 cJSON *method;
	 cJSON *point;
	 int ev_point;
	 int cmd_type;
	 DataItem pDes;
	 
	 memset(&pDes,0,sizeof(DataItem));
	 method = cJSON_GetObjectItem(data, "method");
	 point= cJSON_GetObjectItem(data, "point");
	 if((method!=NULL) && (point !=NULL))
 	 {
		 LogI_Prefix("method->valuestring is %s\r\n", method->valuestring);
		 if(0 == strcmp(method->valuestring, "winOpen"))
		 {
			cmd_type = user_curtain_on;
			pDes.state = 0;
		 }
		 else if(0 == strcmp(method->valuestring, "winClose"))
		 {
			cmd_type = user_curtain_close;
			pDes.state = 1;
		 }
		 else if(0 == strcmp(method->valuestring, "winStop"))
		 {
			cmd_type = user_curtain_stop;
			pDes.state = 2;
		 }
		 else if(0 == strcmp(method->valuestring, "winPercent"))
		 {
			cmd_type = user_curtain_goto_percent;
			cJSON *percent;
			percent= cJSON_GetObjectItem(data, "percent");
			if(percent !=NULL)
			{
				pDes.level = percent->valueint;			
			}
		 }
		 else
		 {
			LogD_Prefix("Control Zigbee cmd is not found,return\r\n");
			return GW_OK ;
		 }
		 
		ev_point = point->valueint;
		if(cmd_type != user_curtain_goto_percent)
		{
			LogD_Prefix("Send msf to Control Zigbee Device1: sn = %s, ev_point = %d, cmd_type = %d, pDes = %d\r\n", sn, ev_point, cmd_type, pDes.state);
			controlZigbeeDevice(sn, ev_point, cmd_type, &pDes);
		}
		else
		{
			LogD_Prefix("Send msf to Control Zigbee Device1: sn = %s, ev_point = %d, cmd_type = %d, pDes = %d\r\n", sn, ev_point, cmd_type, pDes.level);
			controlZigbeeDevice(sn, ev_point, cmd_type, &pDes);
						
		}
	}		
	return GW_OK;
}

int ctl_dev_adapt(uint16_t dev_type, char *sn, char *cmd)
{
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(cmd, GW_NULL_PARAM);

	cJSON *root = cJSON_Parse(cmd);
  	RETURN_IF_NULL(root, GW_NULL_PARAM);
	LogD_Prefix(" dev_type is 0x%04x!!!!!!!!!!!!!!!!!!!!!!!!\r\n",dev_type);
   
    switch(dev_type)
   	{
   		case DEV_GATEWAY:
			get_joinnet_state(root, sn);
			break;
   		case DEV_ONEWAY_LIGHT:
		case DEV_TWOWAY_LIGHT:
		case DEV_THREEWAY_LIGHT:
		case DEV_FOURWAY_LIGHT:
			get_light_state(root,sn);
			break;
		case DEV_ONEWAY_PLUG:
		case DEV_TWOWAY_PLUG:
		case DEV_THREEWAY_PLUG:
		case DEV_FOURWAY_PLUG:
			get_plug_state(root,sn);
			break;
		case DEV_ONEWAY_CURTAIN:
		case DEV_TWOWAY_CURTAIN:
		case DEV_THREEWAY_CURTAIN:
		case DEV_FOURWAY_CURTAIN:
		case DEV_FIVEWAY_CURTAIN:
		case DEV_SIXWAY_CURTAIN:
			get_curtain_state(root,sn);
			break;
		case DEV_SIXWAY_AIR:
		case DEV_SCENE_PANEL:
			//get_scene_state(root,sn);
			break;
		default:
			LogD_Prefix("Zigbee type is not exist\r\n");
			break;
   	}

	cJSON_Delete(root);

	return GW_OK;
}


