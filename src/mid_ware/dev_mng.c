#include "defines.h"
#include "error_code.h"
#include "dev_mng.h"
#include "cJSON.h"
#include "rex_common.h"
#include "dev_cfg.h"
#include "gw_service.h"
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
		if(dev_list_mng.dev_num < dev_list_mng.max_dev_num)
		{
			node = NULL;
			node = (DEV_NODE_S *)malloc(sizeof(DEV_NODE_S));
			if(NULL != node)
			{
				memset(node, 0, sizeof(DEV_NODE_S));
				node->dev_snapshot = NULL;
				INIT_LIST_HEAD(&node->dev_head);
				
				if(save_file)
				{
					save_dev_info(dev);
				}
				memcpy(&node->dev_info, dev, sizeof(DEV_INFO_S));
				list_add(&node->dev_head, &dev_list_mng.dev_list_head);
				
				switch(node->dev_info.conn_type)
				{
					case CONN_TYPE_ZIGBEE:
						dev_list_mng.zigbee_num ++;
						break;
					case CONN_TYPE_WIFI:
						dev_list_mng.wifi_num ++;
						break;
					case CONN_TYPE_BLE:
						dev_list_mng.ble_num ++;
						break;
					default:
						break;
				}
				dev_list_mng.dev_num  = dev_list_mng.zigbee_num + dev_list_mng.ble_num;
				LogI_Prefix("Add device(%s) to list, %d in list\n", dev->sn, dev_list_mng.dev_num);
				ret = GW_OK;
				
			}
			else
			{
				LogE_Prefix("Malloc memory failed, drop device %s\n", dev->sn);
			}
		}
		else
		{
			LogE_Prefix("Device list is full, drop device %s\n", dev->sn);
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
				dev_list_mng.dev_num  = dev_list_mng.zigbee_num + dev_list_mng.ble_num;
				FREE_POINTER(node->dev_snapshot);
				FREE_POINTER(node);
				LogI_Prefix("Del device(%s) from list, %d in list\n", sn, dev_list_mng.dev_num);

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

void *offline_check(void *arg)
{
	DEV_NODE_S *nd = NULL;
	char sn[SN_LEN];
	memset(sn, 0, SN_LEN);
    while(1)
    {
		sleep_ms(1000);
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
				
				if(nd->dev_info.child_info.zigbee.type!= DEV_GATEWAY)
				{
		        	if (nd->dev_info.overtime == 0 && nd->dev_info.offline_flag == 0)
		            {
		        		LogD_Prefix("the device id is %016lX\r\n", nd->dev_info.child_info.zigbee.long_addr);
						memset(sn, 0, SN_LEN);
						agent_strncpy(sn, nd->dev_info.sn, strlen(nd->dev_info.sn));
						//nd->dev_info.offline_flag =1;
						break;
		        	}
				}
            }
        }

		dev_list_unlock();
		if(strlen(sn)!=0 && NULL != nd)
		{	
			nd->dev_info.offline_flag =1;
			LogD_Prefix("A device is offline, the device sn is %s\r\n", sn);
			cdc_slave_device_offline(sn);
			memset(sn, 0, SN_LEN);
		}
		
		
    }
    return NULL;
}
int de_offline_check(void)
{
    LogI_Prefix("Start de_offline_check!!\n");
	pthread_attr_t check;
	pthread_t check_threadId;
	pthread_attr_init(&check);
	pthread_attr_setdetachstate(&check, PTHREAD_CREATE_DETACHED);
	pthread_create(&check_threadId, &check, offline_check, NULL);
	return GW_OK;
}

/**
* Initialize the device list
* @param
* @return 
*/
int init_dev_list(int size)
{	
	memset(&dev_list_mng, 0, sizeof(dev_list_mng));
	dev_list_mng.dev_num = 0;
	dev_list_mng.zigbee_num = 0;
	dev_list_mng.ble_num = 0;
	dev_list_mng.wifi_num = 0;
	dev_list_mng.max_dev_num = (size > MAX_DEV_NUM)?MAX_DEV_NUM:size;
	INIT_LIST_HEAD(&dev_list_mng.dev_list_head);
	if(-1 == pthread_mutex_init(&dev_list_mng.dev_list_mutex, 0))
	{
		return GW_ERR;
	}

	// Read dev list from config file
	read_dev_info();
	de_offline_check();
	
	LogI_Prefix("Device list initialized\n");
	
	return GW_OK;
}

int reinit_dev_list(void)
{
	DEV_NODE_S *node = NULL;
	LogI_Prefix("Start deinit_dev_list\n");
	dev_list_lock();
	
	if(!list_empty(&dev_list_mng.dev_list_head))
	{
		LogI_Prefix(" dev_list_mng.dev_num is %d\n", dev_list_mng.dev_num);
		for(int i = 0; i < dev_list_mng.dev_num; i++)
		{
			list_for_each_entry(node, &dev_list_mng.dev_list_head, dev_head)
			{
				if(DEV_GATEWAY!=node->dev_info.child_info.zigbee.type)
				{
					ctl_dev_clear_net(node->dev_info.sn);
				}
				list_del(&node->dev_head);
				FREE_POINTER(node->dev_snapshot);
				FREE_POINTER(node);
				break;
			}
		}
		dev_list_mng.dev_num = 0;
		dev_list_mng.zigbee_num = 0;
		dev_list_mng.ble_num = 0;
		dev_list_mng.wifi_num = 0;
		LogI_Prefix("Deinit_dev_list is ok\n");
	}
	else
	{
		LogI_Prefix("Dev list is empty\n");
	}
	dev_list_unlock();
	return GW_OK;
}

/*int main(void)
{
	init_dev_list(100);
	DEV_INFO_S info;
	memset(&info, 0, sizeof(DEV_INFO_S));
	for(int i = 100000;i < 100120;i ++)
	{
		sprintf(info.sn, "%d", i);
		add_dev(&info, TRUE);
	}
	DEV_INFO_S *tmp = NULL;
	for(int i = 100000;i < 100100;i ++)
	{
		sprintf(info.sn, "%d", i);
		tmp = get_dev_info(info.sn);
		printf("Get dev %s\n", tmp->sn);
		free(tmp);
		tmp = NULL;
	}
	for(int i = 100000;i < 100100;i ++)
	{
		sprintf(info.sn, "%d", i);
		update_dev_snapshot(info.sn, info.sn);
	}
	char *shot = NULL;
	for(int i = 100000;i < 100100;i ++)
	{
		sprintf(info.sn, "%d", i);
		shot = get_dev_snapshot(info.sn);
		printf("Get dev snapshot %s\n", shot);
		free(shot);
		shot = NULL;
	}
	for(int i = 100110;i >= 100000;i --)
	{
		sprintf(info.sn, "%d", i);
		del_dev(info.sn);
	}
}*/

