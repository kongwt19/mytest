#include "scene_panel.h"
#include "iot_mid_ware.h"
#include "light_local.h"
#include "mqtt.h"
#include "rex_common.h"
#include "biz_plf.h"
#include "gw_service.h"

static PANEL_LIST_MNG_S panel_list_mng;
static PANEL_INFO_S panel_info;
static METHOD_TRANSFER get_method_transfer[] = 
{
	{"打开灯", 	 "lightOn"},
	{"关闭灯", 	 "lightOff"},
	{"打开插座",	 "socketOn"},
	{"关闭插座", "socketOff"},
	{"打开窗帘", "winOpen"},
	{"关闭窗帘",	 "winClose"},
};

void panel_list_lock(void)
{
	pthread_mutex_lock(&panel_list_mng.panel_list_mutex);
}

void panel_list_unlock(void)
{
	pthread_mutex_unlock(&panel_list_mng.panel_list_mutex);
}

int init_panel_list(void)
{	
	memset(&panel_list_mng, 0, sizeof(panel_list_mng));
	INIT_LIST_HEAD(&panel_list_mng.panel_list_head);
	if(-1 == pthread_mutex_init(&panel_list_mng.panel_list_mutex, 0))
	{
		LogE_Prefix("pthread_mutex_init failed\r\n");
		return GW_ERR;
	}
	
	LogI_Prefix("Scene panel list initialized\r\n");
	return GW_OK;
}

void free_panel_node(PANEL_NODE_S ** panel_node) {
	if (*panel_node != NULL) {
		FREE_POINTER((*panel_node)->panel_info.cmd);
		FREE_POINTER(*panel_node);
	}
}

int delete_panel(char *deviceId, int point)
{
	RETURN_IF_NULL(deviceId, GW_NULL_PARAM);
	PANEL_NODE_S *node = NULL;
	LIST_HEAD_T * node_next = NULL;
	if(!list_empty(&panel_list_mng.panel_list_head))
	{
		node = list_entry((&panel_list_mng.panel_list_head)->next, typeof(*node), panel_head);
		RETURN_IF_NULL(node, GW_NULL_PARAM);
		while((&node->panel_head) != (&panel_list_mng.panel_list_head))
		{
			node_next = node->panel_head.next;
			if((!strcmp(node->panel_info.devid, deviceId)) && (node->panel_info.point == point))
			{
				panel_list_lock();
				LogI_Prefix("Del panel(%s) from list, point is %d, sn is %s, addDevid is %s, cmd is %s\r\n", \
				deviceId, point, node->panel_info.sn, node->panel_info.addDevid, node->panel_info.cmd);
				list_del(&node->panel_head);
				free_panel_node(&node);
				panel_list_unlock();
			}
			node = list_entry(node_next, typeof(*node), panel_head);
			RETURN_IF_NULL(node, GW_NULL_PARAM);
		}
	}
	else
	{
		LogI_Prefix("panel list is empty when delete\r\n");
	}
	return GW_OK;
}

int add_node(PANEL_INFO_S *p_info)
{
	RETURN_IF_NULL(p_info, GW_ERR);
	int ret = GW_ERR;
	PANEL_NODE_S *node = NULL;
	
	panel_list_lock();
	node = (PANEL_NODE_S *)malloc(sizeof(PANEL_NODE_S));
	if(NULL != node)
	{
		memset(node, 0, sizeof(PANEL_NODE_S));
		INIT_LIST_HEAD(&node->panel_head);
		memcpy(&node->panel_info, p_info, sizeof(PANEL_INFO_S));
		list_add(&node->panel_head, &panel_list_mng.panel_list_head);
		LogI_Prefix("========Add node to list, gw_sn:(%s) Point: (%d) cmd:(%s)\n", p_info->sn, p_info->point, p_info->cmd);
		ret = GW_OK;	
	}
	else
	{
		LogE_Prefix("Malloc memory failed");
	}
	panel_list_unlock();
	return ret;
}

void init_scene_panel()
{
	
	LogD_Prefix("init_scene_panel start...\r\n");
	init_panel_list();
	memset(&panel_list_mng, 0, sizeof(panel_list_mng));
	INIT_LIST_HEAD(&panel_list_mng.panel_list_head);
	int ret = -1;
	do
	{
		ret = pthread_mutex_init(&panel_list_mng.panel_list_mutex, 0);
	} while (-1 == ret);

	DEV_INFO_S *dev_list = NULL;
	int scene_panel_num = 0;
	get_type_list(DEV_SCENE_PANEL, &dev_list, &scene_panel_num);
	LogI_Prefix("scene_panel_num = %d\r\n", scene_panel_num);
	if(0 != scene_panel_num)
	{
		LogI_Prefix("scene panel num is %d\r\n", scene_panel_num);
		int i, j;
		for(i=0; i<scene_panel_num; i++)
		{
			for(j=1; j <= MAX_POINT; j++)
			{
				PANEL_INFO_S * p = NULL;
				char * check_cmd = NULL;
				do
				{
					ret = ippinte_scene_panel_get_by_point(dev_list[i].sn, j, &check_cmd);
				} while (GW_OK != ret);
				
				if(NULL != check_cmd)
				{
    				cJSON *root = NULL, *json = NULL;
					root = cJSON_Parse(check_cmd);  
					if (NULL != root)
    				{
						int array_size = cJSON_GetArraySize(root);
						int cnt = 0;
						for(;cnt < array_size; cnt++)
						{
							json= cJSON_GetArrayItem(root, cnt);
							cJSON *dev_id=NULL, *ad_point=NULL, *link_id=NULL, *men=NULL, *vlu=NULL;
							dev_id = cJSON_GetObjectItem(json, "addDeviceId");
							ad_point= cJSON_GetObjectItem(json, "addPoint");
							link_id= cJSON_GetObjectItem(json, "linkerId");
							men = cJSON_GetObjectItem(json, "mename");
							vlu = cJSON_GetObjectItem(json, "value");

							if((NULL != dev_id) && (NULL != ad_point) && (NULL != link_id) && (NULL != men) && (NULL != vlu))
							{
								do
								{
									ret = cmd_transfer_point(dev_list[i].sn, j, json, &p); 
									LogD_Prefix("cmd_transfer_point ret: %d\r\n",ret);
								} while (GW_OK != ret);
								do
								{
									ret = add_node(p);
									LogD_Prefix("add_node ret: %d\r\n",ret);
								} while (GW_OK != ret);
							}
							else
							{
								LogE_Prefix("some json key not exist\r\n");
							}
						}
    				}
					else
					{
						LogE_Prefix("cJSON_Parse failed!!\r\n");
					}
					FREE_POINTER(p);
					cJSON_Delete(root);
				}
				else
				{
					LogE_Prefix("scene panel %s point %d has no scene info\r\n",dev_list[i].sn, j);
				}
			}
		}
		LogD_Prefix("init_scene_panel end\r\n");
	}
	else
	{
		LogE_Prefix("dev list has no scene panel, init_scene_panel filed!\n");
	}
	FREE_POINTER(dev_list);
}

int cmd_transfer_point(char * deviceId, int point, cJSON * dev_json, PANEL_INFO_S ** dev_panel){
	RETURN_IF_NULL(deviceId, GW_NULL_PARAM);
	RETURN_IF_NULL(dev_json, GW_NULL_PARAM);
	RETURN_IF_NULL(dev_panel, GW_NULL_PARAM);
	cJSON * addDeviceId_node = NULL;
	cJSON * mename_node = NULL;
	cJSON * value_node = NULL;
	cJSON * point_node = NULL;
	cJSON * linker_node = NULL;
	char * addDeviceId = NULL;
	char * mename = NULL;
	char * method = NULL;
	int i = 0;
	int value = 0;
	int add_point = 0;
	char * json_data = NULL;
	char * sn = NULL;
	PANEL_INFO_S * dev_panel_info = NULL;

	addDeviceId_node = cJSON_GetObjectItem(dev_json, "addDeviceId");
	RETURN_IF_NULL(addDeviceId_node, GW_ERR);
	addDeviceId = addDeviceId_node->valuestring;
	RETURN_IF_NULL(addDeviceId, GW_ERR);

	mename_node = cJSON_GetObjectItem(dev_json, "mename");
	RETURN_IF_NULL(mename_node, GW_ERR);
	mename = mename_node->valuestring;
	RETURN_IF_NULL(mename, GW_ERR);

	linker_node = cJSON_GetObjectItem(dev_json, "linkerId");
	RETURN_IF_NULL(linker_node, GW_ERR);
    sn = linker_node->valuestring;
	RETURN_IF_NULL(sn, GW_ERR);

	point_node = cJSON_GetObjectItem(dev_json, "addPoint");
	add_point = point_node->valueint;

	for(i = 0; i < sizeof(get_method_transfer)/sizeof(get_method_transfer[0]); i++)
	{
		if(strstr(mename,get_method_transfer[i].old_method) != NULL)
		{
			method = get_method_transfer[i].new_method;
			break;
		}
	}
    RETURN_IF_NULL(method, GW_ERR);
    
	//make cmd
	cJSON *root = cJSON_CreateObject();
	RETURN_IF_NULL(root, GW_ERR);
	cJSON_AddStringToObject(root, "deviceId", addDeviceId);
	cJSON_AddNumberToObject(root, "point", add_point);

    //open curtain by percent
	if(!strcmp(method,"winOpen"))
	{
		value_node = cJSON_GetObjectItem(dev_json, "value");
		if(value_node ==  NULL){
			LogE_Prefix("winOpen's value node is null,sn=%s,devid=%s,point=%d\r\n", sn, deviceId, point);
			cJSON_Delete(root);
			return GW_ERR;
		}
		value = value_node->valueint;
		if(value < 100)
		{
			method = "winPercent";
			cJSON_AddNumberToObject(root, "percent", value);
		}
		
	}
	cJSON_AddStringToObject(root, "method", method);
    json_data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	RETURN_IF_NULL(json_data, GW_ERR);

    //make panel node
	dev_panel_info = (PANEL_INFO_S *)malloc(sizeof(PANEL_INFO_S));
	if(dev_panel_info == NULL)
	{
		LogE_Prefix("dev_panel_info malloc is NULL,sn=%s,devid=%s,point=%d\r\n", sn, deviceId, point);
		FREE_POINTER(json_data);
		return GW_ERR;
	}
	memset(dev_panel_info, 0, sizeof(PANEL_INFO_S));
	strncpy(dev_panel_info->sn, sn, strlen(sn));
	strncpy(dev_panel_info->devid, deviceId, strlen(deviceId));
	strncpy(dev_panel_info->addDevid, addDeviceId, strlen(addDeviceId));

	dev_panel_info->point = point;
    dev_panel_info->cmd = (char *)malloc(strlen(json_data));
	if (dev_panel_info->cmd == NULL) {
		LogE_Prefix("cmd malloc is NULL,sn=%s,devid=%s,point=%d\r\n", sn, deviceId, point);
		FREE_POINTER(json_data);
		FREE_POINTER(dev_panel_info);
		return GW_ERR;
	}
	strncpy(dev_panel_info->cmd, json_data, strlen(json_data));
	FREE_POINTER(json_data);
    *dev_panel = dev_panel_info;
    return GW_OK;
}

int batch_add_panel(char * deviceId, int point, char *old_cmd){
	RETURN_IF_NULL(deviceId, GW_NULL_PARAM);
	if(old_cmd == NULL)
	{
		LogI_Prefix("deviceList is NULL when add panel\r\n");
		return GW_OK;
	}

    int i = 0, result = 0;
	cJSON * dev_node = NULL;
	PANEL_INFO_S * dev_panel_info = NULL;
	cJSON * devicelist_node = cJSON_Parse(old_cmd);
	if(devicelist_node == NULL)
	{
         cJSON_Delete(devicelist_node);
	     LogE_Prefix("deviceList is NULL when parse cJSON\r\n");
		 return GW_ERR;
	}
    if(devicelist_node->type != cJSON_Array)
	{
		cJSON_Delete(devicelist_node);
		LogE_Prefix("devicelist is not array\r\n");
		return GW_ERR;
	}
	
	int length = cJSON_GetArraySize(devicelist_node);
	for(i = 0; i < length; ++i)
	{
		dev_node = cJSON_GetArrayItem(devicelist_node, i);
		if(dev_node->type != cJSON_Object)
		{
			cJSON_Delete(devicelist_node);
			LogE_Prefix("dev_node's type is not obj\r\n");
			return GW_ERR;
		}
		result = cmd_transfer_point(deviceId, point, dev_node, &dev_panel_info);
		if(dev_panel_info == NULL)
		{
			cJSON_Delete(devicelist_node);
			LogE_Prefix("dev_panel_info is NULL,%s-%d\r\n", __func__, __LINE__);
			return GW_ERR;
		}
		if(result != GW_OK)
		{
			cJSON_Delete(devicelist_node);
			FREE_POINTER(dev_panel_info);
			LogE_Prefix("cmd_transfer_point error,%s-%d\r\n", __func__, __LINE__);
			return result;
		}
		result = add_node(dev_panel_info);
		FREE_POINTER(dev_panel_info);
		if(result != GW_OK)
		{
			cJSON_Delete(devicelist_node);
			LogE_Prefix("add_node error,%s-%d\r\n", __func__, __LINE__);
			return result;
		}
	}

	cJSON_Delete(devicelist_node);
	return GW_OK;
}

void * update_scene_panel_body(void * arg)
{
	RETURN_IF_NULL(arg, NULL);
	PANEL_INFO_S * panel_info = (PANEL_INFO_S *)arg;
	RETURN_IF_NULL(panel_info->devid, NULL);
	char deviceId[SN_LEN];
	memset(deviceId, 0, strlen(deviceId));
	strncpy(deviceId, panel_info->devid, strlen(panel_info->devid));
	int point = panel_info->point;
	char * old_cmd = NULL;

	int ippinte_ret = GW_ERR;
	int delete_ret = GW_ERR;
	int add_ret = GW_ERR;
	do
	{
		ippinte_ret = ippinte_scene_panel_get_by_point(deviceId, point, &old_cmd);
		if(ippinte_ret != GW_OK){
			continue;
		}
		//delete
        delete_ret = delete_panel(deviceId, point);
		if(delete_ret != GW_OK){
			FREE_POINTER(old_cmd);
			continue;
		}
		//add
		add_ret = batch_add_panel(deviceId, point, old_cmd);
		FREE_POINTER(old_cmd);
	}while((ippinte_ret != GW_OK) || (delete_ret != GW_OK) || (add_ret != GW_OK));
	return NULL;
}

int update_scene_panel(char *deviceId, int point)
{
	RETURN_IF_NULL(deviceId, GW_NULL_PARAM);
	pthread_t pthread_id;
	memset(&panel_info, 0, sizeof(panel_info));
	strncpy(panel_info.devid, deviceId, strlen(deviceId));
	panel_info.point = point;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	int result = pthread_create(&pthread_id, &attr, update_scene_panel_body, &panel_info);
	int ret = (result == 0)?GW_OK:GW_ERR;
	return ret;
}

void execute_scene_panel(char *deviceId, int keyval)
{
	PANEL_NODE_S *node = NULL;
	char *cmd = NULL;
	char *gw_id = NULL;
	char *adddevid = NULL;

	panel_list_lock();
	if(!list_empty(&panel_list_mng.panel_list_head))
	{
		list_for_each_entry(node, (&panel_list_mng.panel_list_head), panel_head)
		{
			if(!strcmp(node->panel_info.devid, deviceId) && (node->panel_info.point == keyval))
			{
				gw_id = node->panel_info.sn;
				cmd = node->panel_info.cmd;
				adddevid = node->panel_info.addDevid;
				DEV_INFO_S *dev_info = get_dev_info(gw_id);
				CONTINUE_IF_NULL(gw_id);
				CONTINUE_IF_NULL(cmd);
				CONTINUE_IF_NULL(adddevid);
				if(!strcmp(gw_id, get_gw_info()->sn))
				{
					if(ctl_dev(adddevid, cmd) == GW_ERR)//本网关下的控制
					{
						LogE_Prefix("control the %s faild, the cmd is %s\n", adddevid, cmd);
					}
				}
				else if(strcmp(gw_id, get_gw_info()->sn) && dev_info != NULL && dev_info->conn_type == CONN_TYPE_WIFI)
				{
					int len = strlen(cmd);
					if(local_ctrl_dev(adddevid, cmd, len) == GW_ERR)//本地控制
					{
						LogE_Prefix("control the local dev %s faild, the cmd is %s\n", adddevid, cmd);
					}
				}
				else
				{
					int topic_len = strlen(gw_id) + 4 + 1;
					char * cmdTopic = (char *)malloc(topic_len);
					if(cmdTopic == NULL)
					{
						FREE_POINTER(cmdTopic);
						LogE_Prefix("control cmdTopic malloc is NULL");
						continue;
					}
					snprintf(cmdTopic, topic_len, "d/%s/i", gw_id);
					mqtt_send_contrl(gw_id, cmd, strlen(cmd), cmdTopic);//mqtt控制 增加mqtt接口进行发送消息
					FREE_POINTER(cmdTopic);
				}
			}
		}
	}
	else
	{
		LogI_Prefix("panel list is empty\n");
	}
	panel_list_unlock();
}