#ifndef __DEV_MNG_H__
#define __DEV_MNG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"
#include <pthread.h>
#include "defines.h"

#define MAX_DEV_NUM 200

typedef struct _zigbee_info
{
	uint64_t long_addr;
	uint16_t type;
	uint8_t node_number;
}ZIGBEE_INFO_S;

typedef struct _ble_info
{
	uint64_t reserve;
}BLE_INFO_S;

typedef struct _wifi_info
{
	uint64_t reserver;
}WIFI_INFO_S;

typedef union _child_info
{
	ZIGBEE_INFO_S 	zigbee;
	BLE_INFO_S		ble;
	WIFI_INFO_S		wifi;
}CHILD_INFO_U;

typedef enum _conn_type
{
	CONN_TYPE_ZIGBEE = 0,
	CONN_TYPE_BLE,
	CONN_TYPE_WIFI,

	CONN_TYPE_BUTT
}CONN_TYPE_E;

typedef struct _devInfo
{
	char sn[SN_LEN];                    //device's serial number
    char ip[IPV4_LEN];                  //device's ip address
	char mac[MAC_LEN];                  //device's mac address
	char product_id[PRODUCT_ID_LEN];    //device's product id
    char gw_sn[SN_LEN];             	//master's serial number
    char soft_ver[VER_LEN];      		//device's software ver
	BOOL is_master;                  	//master or slave
	CONN_TYPE_E conn_type;                    //connect type between slave and master
	CHILD_INFO_U child_info;
	int overtime;
	int maxtime;
}DEV_INFO_S;

typedef struct _devNode
{
	LIST_HEAD_T dev_head;
	DEV_INFO_S dev_info;
	char *dev_snapshot;
}DEV_NODE_S;

typedef struct _devListMng
{
	LIST_HEAD_T	dev_list_head;
	int dev_num;
	int max_dev_num;
	int zigbee_num;
	int ble_num;
	int wifi_num;
	pthread_mutex_t dev_list_mutex;
}DEV_LIST_MNG_S;

/**
* Add a new device to list
* @param dev
* @return 
*/
int add_dev(DEV_INFO_S *dev, BOOL save_file);
/**
* Delte a device from list
* @param sn, device serial number
* @return 
*/
int del_dev(char *sn);
/**
* Get info of a device from the list by sn
* @param sn, device serial number
* @return DEV_INFO_S *, free the memory by yourself
*/
DEV_INFO_S *get_dev_info(char *sn);
/**
* Get snapshot of a device from the list by sn
* @param sn, device serial number
* @return char *, free the memory by yourself
*/
char *get_dev_snapshot(char *sn);
/**
* Get info of a device from the list by sn
* @param sn, device serial number
* @param snapshot, device snapshot
* @return 
*/
int update_dev_snapshot(char *sn, char *snapshot);
/**
* Copy a dev list from manager
* @param 
* @return 
*/
int get_dev_list(CONN_TYPE_E conn_type, DEV_INFO_S **dev_list, int *dev_num);
/**
* Count the child number of a connection type 
* @param child_type, refer to @CONN_TYPE_E
* @return 
*/
int get_child_num(CONN_TYPE_E child_type);
/**
* Set gateway information 
* @param gw, refer to @DEV_INFO_S
* @return 
*/
void set_gw_info(DEV_INFO_S *gw);
/**
* Get gateway information 
* @param gw, refer to @DEV_INFO_S
* @return 
*/
DEV_INFO_S *get_gw_info(void);
/**
* Initialize the device list
* @param
* @return 
*/
int init_dev_list(int size);

#ifdef __cplusplus
}
#endif

#endif/*__DEV_MNG_H__*/
