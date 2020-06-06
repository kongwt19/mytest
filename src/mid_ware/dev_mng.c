#include "defines.h"
#include "error_code.h"
#include "dev_mng.h"
#include "dev_cfg.h"
#include "biz_plf.h"

static DEV_INFO_S gw_info;
static DEV_LIST_MNG_S dev_list_mng;

void dev_list_lock(void)
{
	pthread_mutex_lock(&dev_list_mng.dev_list_mutex);
}

void dev_list_unlock(void)
{
	pthread_mutex_unlock(&dev_list_mng.dev_list_mutex);
}

/**
* Update the device data to list
* @param dev
* @return 
*/
int update_dev(DEV_INFO_S *dev)
{
	int ret = GW_ERR;
	DEV_NODE_S *node = NULL;

	RETURN_IF_NULL(dev, GW_NULL_PARAM);

	dev_list_lock();
	if(!list_empty(&dev_list_mng.dev_list_head))
	{
		list_for_each_entry(node, &dev_list_mng.dev_list_head, dev_head)
		{
			if(!strcmp(node->dev_info.sn, dev->sn))
			{
				memcpy(&node->dev_info, dev, sizeof(DEV_INFO_S));
				LogI_Prefix("Up device(%s) data in list, %d in list\n", dev->sn, dev_list_mng.dev_num);
				ret = GW_OK;
			}
		}
	}
	dev_list_unlock();
	
	return ret;
}


/**
* Add a new device to list
* @param dev
* @return 
*/
int add_dev(DEV_INFO_S *dev, BOOL save_file)
{
	BOOL exists = FALSE;
	int ret = GW_ERR;
	DEV_NODE_S *node = NULL;

	RETURN_IF_NULL(dev, GW_NULL_PARAM);

	dev_list_lock();
	if(!list_empty(&dev_list_mng.dev_list_head))
	{
		list_for_each_entry(node, &dev_list_mng.dev_list_head, dev_head)
		{
			if(!strcmp(node->dev_info.sn, dev->sn))
			{
				//memcpy(&node->dev_info, dev, sizeof(DEV_INFO_S));
				LogI_Prefix("Modify device(%s) in list, %d in list\n", dev->sn, dev_list_mng.dev_num);
				ret = GW_OK;
				exists = TRUE;
				if(save_file)
				{
					// Modify config file
				}
				break;
			}
		}
	}
	if(FALSE == exists)
	{
		node = NULL;
		node = (DEV_NODE_S *)malloc(sizeof(DEV_NODE_S));
		if(NULL != node)
		{
			memset(node, 0, sizeof(DEV_NODE_S));
			node->dev_snapshot = NULL;
			INIT_LIST_HEAD(&node->dev_head);
			memcpy(&node->dev_info, dev, sizeof(DEV_INFO_S));
			
			switch(node->dev_info.conn_type)
			{
				case CONN_TYPE_ZIGBEE:
					if(dev_list_mng.zigbee_num < dev_list_mng.max_zigbee_num)
					{
						dev_list_mng.zigbee_num ++;
						if(save_file)
						{
							save_dev_info(dev);
						}
						list_add(&node->dev_head, &dev_list_mng.dev_list_head);
						ret = GW_OK;
					}
					else
					{
						FREE_POINTER(node);
					}
					break;
				case CONN_TYPE_WIFI:
					dev_list_mng.wifi_num ++;
					list_add(&node->dev_head, &dev_list_mng.dev_list_head);
					ret = GW_OK;
					break;
				case CONN_TYPE_BLE:
					if(dev_list_mng.ble_num < dev_list_mng.max_ble_num)
					{
						dev_list_mng.ble_num ++;
						//Save ble device information
						list_add(&node->dev_head, &dev_list_mng.dev_list_head);
						ret = GW_OK;
					}
					else
					{
						FREE_POINTER(node);
					}
					break;
				default:
					FREE_POINTER(node);
					break;
			}
			if(GW_OK == ret)
			{
				dev_list_mng.dev_num  = dev_list_mng.zigbee_num + dev_list_mng.ble_num + dev_list_mng.wifi_num;
				LogI_Prefix("Add device(%s) to list, (%d(total):%d(zigbee)-%d(ble)-%d(wifi)) devices in list\n", dev->sn, dev_list_mng.dev_num, dev_list_mng.zigbee_num,
							dev_list_mng.ble_num, dev_list_mng.wifi_num);
			}
			else
			{
				LogE_Prefix("Failed to add device(%s)\n", dev->sn);
			}
		}
		else
		{
			LogE_Prefix("Malloc memory failed, drop device %s\n", dev->sn);
		}
	}
	dev_list_unlock();
	
	return ret;
}

/**
* Delte a device from list
* @param sn, device serial number
* @return 
*/
int del_dev(char *sn)
{
	DEV_NODE_S *node = NULL;
	
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	
	dev_list_lock();
	if(!list_empty(&dev_list_mng.dev_list_head))
	{
		list_for_each_entry(node, &dev_list_mng.dev_list_head, dev_head)
		{
			if(!strcmp(node->dev_info.sn, sn))
			{
				del_dev_info(sn,node,&dev_list_mng.dev_list_head);
				list_del(&node->dev_head);
				switch(node->dev_info.conn_type)
				{
					case CONN_TYPE_ZIGBEE:
						dev_list_mng.zigbee_num --;
						break;
					case CONN_TYPE_WIFI:
						dev_list_mng.wifi_num --;
						break;
					case CONN_TYPE_BLE:
						dev_list_mng.ble_num --;
						break;
					default:
						break;
				}
				dev_list_mng.dev_num  = dev_list_mng.zigbee_num + dev_list_mng.ble_num + dev_list_mng.wifi_num;
				FREE_POINTER(node->dev_snapshot);
				FREE_POINTER(node);
				LogI_Prefix("Del device(%s) from list, (%d(total):%d(zigbee)-%d(ble)-%d(wifi)) devices in list\n", sn, dev_list_mng.dev_num, 
							dev_list_mng.zigbee_num, dev_list_mng.ble_num, dev_list_mng.wifi_num);

				// Del dev info from configuration file
				// Erase & Write
				break;
			}
		}
	}
	else
	{
		LogI_Prefix("Dev list is empty, %s not found\n", sn);
	}
	dev_list_unlock();
	
	return GW_OK;
}

/**
* Get info of a device from the list by sn
* @param sn, device serial number
* @return DEV_INFO_S *, free the memory by yourself
*/
DEV_INFO_S *get_dev_info(char *sn)
{
	DEV_NODE_S *node = NULL;
	DEV_INFO_S *info = NULL;
	
	RETURN_NULL_IF_NULL(sn);
	
	dev_list_lock();
	if(!list_empty(&dev_list_mng.dev_list_head))
	{
		list_for_each_entry(node, &dev_list_mng.dev_list_head, dev_head)
		{
			if(!strcmp(node->dev_info.sn, sn))
			{
				info = (DEV_INFO_S*)malloc(sizeof(DEV_INFO_S));
				if(NULL != info)
				{
					memset(info, 0, sizeof(DEV_INFO_S));
					memcpy(info, &node->dev_info, sizeof(DEV_INFO_S));
				}
				break;
			}
		}
	}
	else
	{
		LogI_Prefix("Dev list is empty, %s not found\n", sn);
	}
	dev_list_unlock();

	return info;
}

/**
* Get snapshot of a device from the list by sn
* @param sn, device serial number
* @return char *, free the memory by yourself
*/
char *get_dev_snapshot(char *sn)
{
	DEV_NODE_S *node = NULL;
	char *snapshot = NULL;
	
	RETURN_NULL_IF_NULL(sn);
	
	dev_list_lock();
	if(!list_empty(&dev_list_mng.dev_list_head))
	{
		list_for_each_entry(node, &dev_list_mng.dev_list_head, dev_head)
		{
			if(!strcmp(node->dev_info.sn, sn))
			{
				snapshot = (char*)malloc(strlen(node->dev_snapshot) + 1);
				if(NULL != snapshot)
				{
					memset(snapshot, 0, strlen(node->dev_snapshot) + 1);
					strcpy(snapshot, node->dev_snapshot);
				}
				break;
			}
		}
	}
	else
	{
		LogI_Prefix("Dev list is empty, %s not found\n", sn);
	}
	dev_list_unlock();

	return snapshot;
}

/**
* Get info of a device from the list by sn
* @param sn, device serial number
* @param snapshot, device snapshot
* @return 
*/
int update_dev_snapshot(char *sn, char *snapshot)
{
	DEV_NODE_S *node = NULL;
	
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(snapshot, GW_NULL_PARAM);
	
	dev_list_lock();
	if(!list_empty(&dev_list_mng.dev_list_head))
	{
		list_for_each_entry(node, &dev_list_mng.dev_list_head, dev_head)
		{
			if(!strcmp(node->dev_info.sn, sn))
			{
				FREE_POINTER(node->dev_snapshot);
				node->dev_snapshot = (char*)malloc(strlen(snapshot) + 1);
				if(NULL != node->dev_snapshot)
				{
					memset(node->dev_snapshot, 0, strlen(snapshot) + 1);
					strcpy(node->dev_snapshot, snapshot);
				}
				break;
			}
		}
	}
	else
	{
		LogI_Prefix("Dev list is empty, %s not found\n", sn);
	}
	dev_list_unlock();

	return GW_OK;
}

/**
* Copy a dev list from manager
* @param 
* @return 
*/
int get_dev_list(CONN_TYPE_E conn_type, DEV_INFO_S **dev_list, int *dev_num)
{
	int i = 0;
	int ret = GW_ERR;
	DEV_NODE_S *node = NULL;
	DEV_INFO_S *devs = NULL;
	int num = 0;
	
	RETURN_IF_NULL(dev_list, GW_NULL_PARAM);
	RETURN_IF_NULL(dev_num, GW_NULL_PARAM);
	
	dev_list_lock();
	num = get_child_num(conn_type);
	if(0 != num)
	{
		devs = (DEV_INFO_S *)malloc(sizeof(DEV_INFO_S) * num);
		if(NULL != devs)
		{
			memset(devs, 0, sizeof(DEV_INFO_S) * num);
			list_for_each_entry(node, &dev_list_mng.dev_list_head, dev_head)
			{
				if(conn_type == node->dev_info.conn_type)
				{
					memcpy(devs + i, &node->dev_info, sizeof(DEV_INFO_S));
					i ++;
				}
			}
			ret = GW_OK;
			*dev_list = devs;
			*dev_num = num;
		}
	}
	dev_list_unlock();

	return ret;
}
/**
* Copy a dev list from manager with type
* @param 
* @return 
*/
int get_type_list(uint16_t type, DEV_INFO_S **type_list, int *type_num)
{
	int i = 0;
	int ret = GW_ERR;
	DEV_NODE_S *node = NULL;
	DEV_INFO_S *devs = NULL;
	int num = 0;
	
	RETURN_IF_NULL(type_list, GW_NULL_PARAM);
	RETURN_IF_NULL(type_num, GW_NULL_PARAM);
	
	dev_list_lock();
	num = get_type_num(type);
	if(0 != num)
	{  
		devs = (DEV_INFO_S *)malloc(sizeof(DEV_INFO_S) * num);
		if(NULL != devs)
		{
			memset(devs, 0, sizeof(DEV_INFO_S) * num);
			list_for_each_entry(node, &dev_list_mng.dev_list_head, dev_head)
			{
				if(type == node->dev_info.child_info.zigbee.type)
				{
					memcpy(devs + i, &node->dev_info, sizeof(DEV_INFO_S));
					i ++;
				}
			}
			ret = GW_OK;
			*type_list = devs;
			*type_num = num;
		}
	}
	dev_list_unlock();

	return ret;
}
/**
* Count the number of a zigbee type 
* @param type, refer to @uint16_t
* @return 
*/
int get_type_num(uint16_t type)
{
	int type_num = 0;
	DEV_NODE_S *node = NULL;
	list_for_each_entry(node, &dev_list_mng.dev_list_head, dev_head)
	{
		if(type == node->dev_info.child_info.zigbee.type)
		{
			type_num ++;
		}
	}
	return type_num;	
}

/**
* Count the child number of a connection type 
* @param child_type, refer to @CONN_TYPE_E
* @return 
*/
int get_child_num(CONN_TYPE_E child_type)
{
	int child_num = 0;

	switch(child_type)
	{
		case CONN_TYPE_ZIGBEE:
			child_num = dev_list_mng.zigbee_num;
			break;
		case CONN_TYPE_WIFI:
			child_num = dev_list_mng.wifi_num;
			break;
		case CONN_TYPE_BLE:
			child_num = dev_list_mng.ble_num;
			break;
		case CONN_TYPE_BUTT:
		default:
			break;
	}

	return child_num;	
}

/**
* Set gateway information 
* @param gw, refer to @DEV_INFO_S
* @return 
*/
void set_gw_info(DEV_INFO_S *gw)
{
	RETURN_VOID_IF_NULL(gw);
	memset(&gw_info, 0, sizeof(gw_info));
	memcpy(&gw_info, gw, sizeof(gw_info));
}
/**
* Get gateway information 
* @param gw, refer to @DEV_INFO_S
* @return 
*/
DEV_INFO_S *get_gw_info(void)
{
	return &gw_info;
}

/**
* check  dev list return the offline dev
* @param 
* @return 
*/
void get_offline_dev(char *sn, DEV_NODE_S **dev_nd, uint16_t uncheck_dev_type)
{
	DEV_NODE_S *nd = NULL;
	dev_list_lock();
	if(!list_empty(&dev_list_mng.dev_list_head))
	{
		list_for_each_entry(nd, &dev_list_mng.dev_list_head, dev_head)
		{				
			if(nd->dev_info.overtime > 0)
			{
				nd->dev_info.overtime -= 1;
				//LogD_Prefix("the device id is %016lX,offline count= %d\r\n", nd->dev_info.child_info.zigbee.long_addr,nd->dev_info.overtime);
			}
			
			if(nd->dev_info.child_info.zigbee.type!= uncheck_dev_type)
			{
				if (nd->dev_info.overtime == 0 && nd->dev_info.offline_flag == 0)
				{
					LogD_Prefix("the device id is %016lX\r\n", nd->dev_info.child_info.zigbee.long_addr);
					memset(sn, 0, SN_LEN);
					AGENT_STRNCPY(sn, nd->dev_info.sn, strlen(nd->dev_info.sn));
					//nd->dev_info.offline_flag =1;
					break;
				}
			}
		}
	}
	*dev_nd = nd;
	dev_list_unlock();
}

/**
* Initialize the device list
* @param
* @return 
*/
int init_dev_list(int max_zigbee, int max_ble)
{	
	memset(&dev_list_mng, 0, sizeof(dev_list_mng));
	dev_list_mng.dev_num = 0;
	dev_list_mng.zigbee_num = 0;
	dev_list_mng.ble_num = 0;
	dev_list_mng.wifi_num = 0;
	dev_list_mng.max_zigbee_num = (max_zigbee > MAX_ZIGBEE_NUM)?MAX_ZIGBEE_NUM:max_zigbee;
	dev_list_mng.max_ble_num = (max_ble > MAX_BLE_NUM)?MAX_BLE_NUM:max_ble;
	INIT_LIST_HEAD(&dev_list_mng.dev_list_head);
	if(-1 == pthread_mutex_init(&dev_list_mng.dev_list_mutex, 0))
	{
		return GW_ERR;
	}

	// Read dev list from config file
	read_dev_info();
	
	LogI_Prefix("Device list initialized\n");
	
	return GW_OK;
}

int clear_child_by_type(CONN_TYPE_E child_type)
{
	DEV_NODE_S *node = NULL;

	LogD_Prefix("Clear child list(%d) begin\n", child_type);
	dev_list_lock();
	while(0 != get_child_num(child_type))
	{
		list_for_each_entry(node, &dev_list_mng.dev_list_head, dev_head)
		{
			if(child_type == node->dev_info.conn_type)
			{
				LogD_Prefix("Clear device(%s-%d)\n", node->dev_info.sn, node->dev_info.conn_type);
				list_del(&node->dev_head);
				FREE_POINTER(node->dev_snapshot);
				FREE_POINTER(node);
				switch(child_type)
				{
					case CONN_TYPE_ZIGBEE:
						dev_list_mng.zigbee_num --;
						dev_list_mng.dev_num --;
						break;
					case CONN_TYPE_BLE:
						dev_list_mng.ble_num --;
						dev_list_mng.dev_num --;
						break;
					case CONN_TYPE_WIFI:
						dev_list_mng.wifi_num --;
						dev_list_mng.dev_num --;
						break;
					default:
						break;
				}
				break;
			}
		}
	}
	dev_list_unlock();
	LogD_Prefix("Clear dev list end, (%d(total):%d(zigbee)-%d(ble)-%d(wifi)) devices in list\n", dev_list_mng.dev_num, 
				dev_list_mng.zigbee_num, dev_list_mng.ble_num, dev_list_mng.wifi_num);

	return GW_OK;
}

/*main(void)
{
	init_dev_list(32, 16);
	DEV_INFO_S info;
	memset(&info, 0, sizeof(DEV_INFO_S));
	for(int i = 100000;i < 100035;i ++)
	{
		sprintf(info.sn, "%d", i);
		info.conn_type = CONN_TYPE_ZIGBEE;
		add_dev(&info, TRUE);
	}
	for(int i = 200000;i < 200020;i ++)
	{
		sprintf(info.sn, "%d", i);
		info.conn_type = CONN_TYPE_BLE;
		add_dev(&info, TRUE);
	}
	for(int i = 300000;i < 300020;i ++)
	{
		sprintf(info.sn, "%d", i);
		info.conn_type = CONN_TYPE_WIFI;
		add_dev(&info, TRUE);
	}
}*/

