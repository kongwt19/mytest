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
#include "biz_plf.h"
#include "scene_panel.h"
#include "dev_cfg.h"

static int prase_scene_key_info(char *sn, uint32_t ep_id, char *data);
static int prase_scene_switch_info(char *sn, uint32_t ep_id, char *data);
static void setSensor(DEV_INFO_S *ptDevice, const char *sn, const char *productID, uint16_t type, uint64_t laddr);
static int prase_light_info(char *sn, uint32_t ep_id, char *data);
static int prase_plug_info(char *sn, uint32_t ep_id, char *data);
static int prase_curtain_info(char *sn, uint32_t ep_id, char *data);
static void set_network_value(int val);
static int get_network_value(void);
//static int creat_net_flag = 0;

static int g_network = -1;
static int timescnt = 0;
static uint32_t timestamp = 0;
static uint32_t join_time = 0;

/**
 * init_rex_sdk
 * SDK启动函数
 * return: 0成功,否则-1
 * note: none
 */
#ifndef REX_ENABLE
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
    	//return GW_ERR;
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
    }
    //开始运行SDK
    if(rex_start() != 0)
    {
        LogE_Prefix("Failed to rex_start \r\n");
	    return GW_ERR;
    }

    reg_dev_ctl_callback(CONN_TYPE_ZIGBEE, ctl_dev_adapt);

	return GW_OK;
}
#endif
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
#ifndef REX_ENABLE
int device_data_report(char *sn, uint32_t ep_id, uint16_t data_type, char *data)
{
	int ret = GW_OK;
	DEV_INFO_S *ptr = NULL;

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data, GW_NULL_PARAM);

	if(data_type == NET_INFO)
	{
		//creat_net_flag = GW_NET_FAIL;
		report_gw_status();
	}

	//sn地址存在则上报zigbee数据，不存在则不上报
	ptr = get_dev_info(sn);
	if(ptr != NULL)
	{
		if(ptr->offline_flag == 1)
		{
			LogI_Prefix("device %s is offline to online!\r\n", ptr->sn);
			cdc_slave_device_online(ptr);
		}
		ptr->offline_flag = 0;
		ptr->overtime = ptr->maxtime;
		update_dev(ptr);
		LogD_Prefix("device report data: sn = %s, data_type = 0x%04X, data = %s\r\n", sn, data_type, data);
		switch(data_type)
		{
			case PLUG_INFO:
			{
				//插座上报
				ret = prase_plug_info(sn, ep_id, data);
				break;
			}
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
			case SCENE_KEY:
			{
				/* 场景面板按键上报 */
				ret = prase_scene_key_info(sn, ep_id, data);
				break;
			}
			case SCENE_SWITCH:
			{
				/* 场景面板开关上报 */
				ret = prase_scene_switch_info(sn, ep_id, data);
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
		LogE_Prefix("sn: %s is no exist\r\n", sn);
		return GW_ERR;
	}
	
	FREE_POINTER(ptr);
	
	return ret;
}
#endif
/**
 * call_back_report_state_data_cb
 * 下发控制数据函数
 * data[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
int ctl_zig_dev(char *sn, uint32_t ep_id, USER_CONTROL_OPTION_E type, DItem pDes)
{
	unsigned short cmd = 0;
	char *dev_id = sn;
	char conmand_message[JSON_MAX_LEN];

	memset(conmand_message, 0, JSON_MAX_LEN);

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(pDes, GW_NULL_PARAM);

	switch(type)
	{
		case user_join_net:
		{
			dev_id = "0000000000000000";
			cmd = ZIGBEE_JOIN_NET;
			if((pDes->times > 255) || (pDes->times <= 0))
			{
				pDes->times = 240;
			}
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Duration\": \"%d\"}", pDes->times);
			join_time = pDes->times;
			set_network_value(JOIN_NETWORK_OPEN);
			dev_zigbee_callback(TRUE);
			report_gw_status();
			break;
		}
		case user_stop_net:
		{
			dev_id = "0000000000000000";
			cmd = ZIGBEE_JOIN_NET;
			pDes->times = 0;
			snprintf(conmand_message, JSON_MAX_LEN, "{\"Duration\": \"%d\"}", pDes->times);
			set_network_value(JOIN_NETWORK_CLOSE);
			dev_zigbee_callback(FALSE);
			report_gw_status();
			break;
		}
		case user_creat_net:
		{
			dev_id = "0000000000000000";
			cmd = ZIGBEE_CREAT_NET_S;
			snprintf(conmand_message, JSON_MAX_LEN, "{}");
			set_network_value(JOIN_NETWORK_CLOSE);
			break;
		}
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
			dev_id = "0000000000000000";
			cmd = ZIGBEE_CLEAR_DEVICE;
			snprintf(conmand_message, JSON_MAX_LEN, "{}");
			break;
		}
		default:
			LogE_Prefix("Zigbee command type is not exist\r\n");
			return GW_ERR;
	}

	LogI_Prefix("begin to send command_data:\r\n  sn = %s\r\n  ep_id = %d\r\n  cmd = %d\r\n  conmand_message = %s\r\n", sn, ep_id, cmd, conmand_message);
#ifndef REX_ENABLE
	rex_send_command_data(dev_id, ep_id, cmd, conmand_message);
#endif
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
	MALLOC_ERROR_RETURN_WTTH_RET(parameters, GW_MALLOR_FAIL);
	memset(parameters, 0, sizeof(DEV_INFO_S));

	switch(dev_type)
	{
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
		case DEV_SCENE_PANEL:
		{
			productID = DEV_PRODUCT_SCENE;
			break;
		}
		default:
			break;
	}
	RETURN_IF_NULL(productID, GW_NULL_PARAM);
	setSensor(parameters, sn, productID, dev_type, 0);
	LogI_Prefix("device join net:sn = %s ,productID = %s ,dev_type = 0x%04X\r\n", parameters->sn, parameters->product_id, parameters->child_info.zigbee.type);

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
		LogI_Prefix("the device is online after offline\r\n");
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

	LogI_Prefix("the device: %s is offline\r\n", sn);
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
#ifndef REX_ENABLE
int zigbee_network_query(void)
{
	LogD_Prefix("g_network = %d\r\n", g_network);
	return g_network;
}
#endif

int set_network_sta(void)
{
	if(get_network_value() == 0)
	{
		/* 入网后开始计时 */
		if(get_ssecond() >= timestamp+join_time)
        {
        	timestamp = get_ssecond();
        	timescnt++;
        	LogD_Prefix("==============join network start, timescnt = %d, systemtime = %d==============\r\n", timescnt,timestamp);
        }
		if(timescnt > 1)
		{
			timescnt = 0;
			timestamp = 0;
			set_network_value(JOIN_NETWORK_CLOSE);
			report_gw_status();
			LogI_Prefix("==========JOIN_NETWORK_CLOSE: %d========\r\n", g_network);
		}
    }
	else
	{
		timescnt = 0;
		timestamp = 0;
	}
	
	
    return GW_OK;
}

static void set_network_value(int val)
{
	g_network = val;
}

static int get_network_value(void)
{
	return g_network;
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
	AGENT_STRNCPY(ptDevice->sn, sn, strlen(sn));
	AGENT_STRNCPY(ptDevice->product_id, productID, strlen(productID));
	ptDevice->child_info.zigbee.type = type;
	ptDevice->child_info.zigbee.long_addr = laddr;
	string_processor_hex_str_to_byte(sn, strlen(sn), mac);
	snprintf(ptDevice->mac, sizeof(ptDevice->mac), "%02x:%02x:%02x:%02x:%02x:%02x", \
				mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
	ptDevice->conn_type = CONN_TYPE_ZIGBEE;
	AGENT_STRNCPY(ptDevice->soft_ver, "1.1.01", strlen("1.1.01"));
	AGENT_STRNCPY(ptDevice->gw_sn, get_gw_info()->sn, strlen(get_gw_info()->sn));
	AGENT_STRNCPY(ptDevice->ip,get_gw_info()->ip , strlen(get_gw_info()->ip));
	ptDevice->maxtime = VC_MAX_TIME;
	ptDevice->overtime= ptDevice->maxtime;
	ptDevice->offline_flag = 0;
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
				snprintf(json_value, JSON_MAX_LEN, "{\"light_val\":0,\"devstate\":%d,\"point\":%d}", istatus, ep_id);
				LogD_Prefix("light_info: sn = %s,json_value = %s  \n", sn, json_value);
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
 * prase_plug_info
 * 解析插座开关数据函数
 * sn[in]: 设备地址
 * data[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
static int prase_plug_info(char *sn, uint32_t ep_id, char *data)
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
				snprintf(json_value, JSON_MAX_LEN, "{\"devpower\":0,\"devstate\":%d}", istatus);
				LogD_Prefix("plug_info: sn = %s,json_value = %s  \n", sn, json_value);
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
				snprintf(json_value, JSON_MAX_LEN, "{\"devstate\":%d,\"level\":%d,\"point\":%d}", \
								istatus, ilevel, ep_id);
				LogD_Prefix("curtain_info: sn = %s,json_value = %s  \n", sn, json_value);
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
 * 解析窗场景面板按键数据函数
 * sn[in]: 设备地址
 * data[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
static int prase_scene_key_info(char *sn, uint32_t ep_id, char *data)
{
	uint8_t ikeyvalue = 0;
	cJSON *root, *keyvalue;
	char json_value[JSON_MAX_LEN];

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data, GW_NULL_PARAM);

	root = cJSON_Parse(data);
	if(root)
	{
		keyvalue = cJSON_GetObjectItem(root, "KeyValue");
		if(keyvalue)
		{
			if(0 == strcmp(keyvalue->string, "KeyValue"))
			{
				ikeyvalue = atoi(keyvalue->valuestring);
				memset(json_value, 0, JSON_MAX_LEN);
				snprintf(json_value, JSON_MAX_LEN, "{\"devid\":\"%s\",\"KeyValue\":%d,\"point\":%d}", sn, ikeyvalue, ep_id);
				LogD_Prefix("The json_value of prase_scene_key_info is %s\r\n", json_value);
				//调场景控制
				execute_scene_panel(sn, ikeyvalue);
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
 * prase_scene_switch_info
 * 解析窗场景面板开关数据函数
 * sn[in]: 设备地址
 * data[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
static int prase_scene_switch_info(char *sn, uint32_t ep_id, char *data)
{
	uint8_t iworkmode = 0;
	cJSON *root, *workmode;
	char json_value[JSON_MAX_LEN];

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data, GW_NULL_PARAM);

	root = cJSON_Parse(data);
	if(root)
	{
		workmode = cJSON_GetObjectItem(root, "WorkMode");
		if(workmode)
		{
			if(0 == strcmp(workmode->string, "WorkMode"))
			{
				iworkmode = atoi(workmode->valuestring);
				memset(json_value, 0, JSON_MAX_LEN);
				if(iworkmode == 10)
				{
					iworkmode = 1;
				}
				snprintf(json_value, JSON_MAX_LEN, "{\"devstate\":%d,\"point\":%d}", iworkmode, ep_id);
				LogD_Prefix("The json_value of prase_scene_switch_info is %s\r\n", json_value);
				dev_report(sn, json_value);
				//调场景控制
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
 * get_joinnet_state
 * 获取入网或删除网关json数据
 * data[in]: json数据
 * sn[in]: 设备地址
 * return: 0成功,否则-1
 * note: none
 */

int get_joinnet_state(cJSON *data, char *sn)
{
	cJSON *method;
	cJSON *time;
	int ev_point;
	int cmd_type;
	DataItem pDes;
	memset(&pDes,0,sizeof(DataItem));
    method = cJSON_GetObjectItem(data, "method");
	time = cJSON_GetObjectItem(data, "time");
	if(method!=NULL)
	{
		if( time !=NULL )
		{
			LogD_Prefix("method->valuestring is %s\r\n",method->valuestring);
			if(0 == strcmp(method->valuestring, "start_zigbee"))
			{
				cmd_type = user_join_net;
			}
			else if(0 == strcmp(method->valuestring, "stop_zigbee"))
			{
				cmd_type = user_stop_net;
			}
			else
			{
				LogD_Prefix("Control Zigbee cmd is not found,return\r\n");
				return GW_ERR;
			}

		/*	if(creat_net_flag == GW_NET_SUCCESS)
			{
				pDes.times = time->valueint;
				creat_net_flag = GW_NET_FAIL;
				cmd = user_creat_net;
				ev_point = 1;
				ctl_zig_dev(sn, ev_point, cmd, &pDes);
				ctl_zig_dev(sn, ev_point, cmd, &pDes);
			}*/

			LogD_Prefix("time->valueint is %d\r\n",time->valueint);
			pDes.times = time->valueint;
			ev_point = 1;
			LogD_Prefix("Send msf to Control Zigbee Device1: sn = %s, ev_point = %d, cmd_type = %d, pDes = %d\r\n", sn, ev_point, cmd_type, pDes.times);
			ctl_zig_dev(sn, ev_point, cmd_type, &pDes);
		}
		else
		{
			if(0 == strcmp(method->valuestring ,"delete_gateway"))
			{
				ev_point = 1;
				LogD_Prefix("Send msf to Control Zigbee Device1: sn = %s, ev_point = %d, cmd_type = %d, pDes = %d\r\n", sn, ev_point, cmd_type, pDes.times);
				LogI_Prefix("Delete gateway data!\n");
				clear_child_by_type(CONN_TYPE_ZIGBEE);
				erase_dev_cfg();
				//creat_net_flag = GW_NET_SUCCESS;
				ctl_zig_dev(sn, ev_point, user_creat_net, &pDes);
			}
		}
	}
	return GW_OK;
}

/**
 * get_light_state
 * 获取控制灯json数据
 * data[in]: json数据
 * sn[in]: 设备地址
 * return: 0成功,否则-1
 * note: none
 */
int get_light_state(cJSON *data, char *sn)
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
	    ctl_zig_dev(sn, ev_point, cmd_type, &pDes);
	}		
	return GW_OK;
}

/**
 * get_plug_state
 * 获取控制插座json数据
 * data[in]: json数据
 * sn[in]: 设备地址
 * return: 0成功,否则-1
 * note: none
 */
int get_plug_state(cJSON *data, char *sn)
{
	cJSON *method;
	cJSON *order;
	int ev_point;
	int cmd_type;
	DataItem pDes;
	
	memset(&pDes,0,sizeof(DataItem));
	method = cJSON_GetObjectItem(data, "method");
	order = cJSON_GetObjectItem(data, "order");
	if((method!=NULL)  && (order != NULL))
	{
		LogD_Prefix("method->valuestring is %s\r\n", method->valuestring);
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
					return GW_OK;
			}
		}
	    
		ev_point = 1;
		LogD_Prefix("Send msf to Control Zigbee Device1: sn = %s, ev_point = %d, cmd_type = %d, pDes = %d\r\n", sn, ev_point, cmd_type, pDes.state);
	    ctl_zig_dev(sn, ev_point, cmd_type, &pDes);
	}
	
	return GW_OK;
}

/**
 * get_curtain_state
 * 获取控制窗帘json数据
 * data[in]: json数据
 * sn[in]: 设备地址
 * return: 0成功,否则-1
 * note: none
 */
int get_curtain_state(cJSON *data, char *sn)
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
			return GW_ERR;
		}

		ev_point = point->valueint;
		LogD_Prefix("Send msf to Control Zigbee Device1: sn = %s, ev_point = %d, cmd_type = %d, pDes = %d\r\n", sn, ev_point, cmd_type, pDes.state);
		ctl_zig_dev(sn, ev_point, cmd_type, &pDes);
	}

	return GW_OK;
}

/**
 * ctl_dev_clear_net
 * 清除单个设备的zigbee网络
 * sn[in]: 设备地址
 * return: 无
 * note: none
 */
#ifndef REX_ENABLE
void ctl_dev_clear_net(char *sn)
{
	int ev_point;
	int cmd_type;
	DataItem pDes;
	memset(&pDes,0,sizeof(DataItem));
	cmd_type = user_clear_device;
	pDes.state = 255;
	ev_point = 1;
	LogD_Prefix("Send msf to Control Zigbee Device1: sn = %s, ev_point = %d, cmd_type = %d, pDes = %d\r\n", sn, ev_point, cmd_type, pDes.state);
	ctl_zig_dev(sn, ev_point, cmd_type, &pDes);
}
#endif
/**
 * ctl_all_dev_clear_net
 * 清除所有设备的zigbee网络
 * sn[in]: 设备地址
 * return: 无
 * note: none
 */
void ctl_all_dev_clear_net(void)
{
	char *sn = NULL;
	sn = "FFFFFFFFFFFFFFFF";
	int ev_point;
	int cmd_type;
	DataItem pDes;
	memset(&pDes,0,sizeof(DataItem));
	cmd_type = user_clear_all_devices;
	pDes.state = 255;
	ev_point = 255;
	LogD_Prefix("Send msf to Control Zigbee Device1: sn = %s, ev_point = %d, cmd_type = %d, pDes = %d\r\n", sn, ev_point, cmd_type, pDes.state);
	ctl_zig_dev(sn, ev_point, cmd_type, &pDes);
}

/**
 * ctl_dev_adapt
 * 控制设备命令
 * dev_type: 设备类型
 * sn[in]: 设备地址
 * cmd[in]: 命令
 * return: 0成功,否则-1
 * note: none
 */
int ctl_dev_adapt(uint16_t dev_type, char *sn, char *cmd)
{
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(cmd, GW_NULL_PARAM);

	cJSON *root = cJSON_Parse(cmd);
  	RETURN_IF_NULL(root, GW_NULL_PARAM);
	cJSON *point;
	cJSON *option;

	point= cJSON_GetObjectItem(root, "point");
	option = cJSON_GetObjectItem(root, "option");
	if((option!=NULL) && (point !=NULL))
	{
		if(0 == strcmp(option->valuestring, "devDel"))
		{
			ctl_dev_clear_net(sn);
		}
	}
	else
	{
		LogD_Prefix(" dev_type is 0x%04x!!!!!!!!!!!!!!!!!!!!!!!!\r\n", dev_type);
	    switch(dev_type)
	   	{
	   		case DEV_GATEWAY:
				get_joinnet_state(root, sn);
				break;
	   		case DEV_ONEWAY_LIGHT:
			case DEV_TWOWAY_LIGHT:
			case DEV_THREEWAY_LIGHT:
			case DEV_FOURWAY_LIGHT:
				get_light_state(root, sn);
				break;
			case DEV_ONEWAY_PLUG:
			case DEV_TWOWAY_PLUG:
			case DEV_THREEWAY_PLUG:
			case DEV_FOURWAY_PLUG:
				get_plug_state(root, sn);
				break;
			case DEV_ONEWAY_CURTAIN:
			case DEV_TWOWAY_CURTAIN:
			case DEV_THREEWAY_CURTAIN:
			case DEV_FOURWAY_CURTAIN:
			case DEV_FIVEWAY_CURTAIN:
			case DEV_SIXWAY_CURTAIN:
				get_curtain_state(root,sn);
				break;
			default:
				LogE_Prefix("Zigbee device type is not exist\r\n");
				break;
	   	}
	}

	cJSON_Delete(root);

	return GW_OK;
}


