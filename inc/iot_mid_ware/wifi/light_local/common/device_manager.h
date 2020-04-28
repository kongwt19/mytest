/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  文 件 名   : device_manager.h
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : 设备管理接口，对client发现的设备进行统一管理
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

#include "local_module_enable.h"
#include "ipp_local_device_info.h"
#include "ipp_defines.h"

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE

//设备列表
typedef struct device_list
{
	int fd;									//套接字
	uint32_t last_request_time;				//上次询问设备详细信息的时间
	uint32_t added_time;					//首次加入到列表的时间
	ipp_local_device *device;				//设备信息
	ipp_device_list *agent_device_list;		//其下挂的第三方设备列表
	struct device_list *next;				//下一个元素的指针
}device_list;

//初始化设备列表
void init_device_list();
//发现设备接入，按照对应fd，加入一个节点到列表，此时设备详细信息为空
int device_list_append(int fd);
//将一个第三方设备加入到一个指定sn的主设备的第三方列表中
int device_list_append_agent(char *gateway_guid, ipp_local_device *device);
//更新设备信息，适用于设备状态更新或者多次发现
IPP_BOOL update_device_info(int fd, ipp_local_device *element);
//将设备从列表中移除，并返回下一个设备的指针
device_list *device_list_remove(int fd, ipp_local_device **removed_device);
//将第三方设备从指定sn的主设备的第三方设备列表中移除
int device_list_remove_agent(char *gateway_guid, ipp_local_device *device);
//判断指定的套接字节点是否详细信息为空
IPP_BOOL is_device_list_empty(int fd);
//将已经接入的节点，但是没有详细信息的移除
IPP_BOOL device_list_remove_empty(int fd);
//根据SN查找设备，并返回指针，recursion参数指明知否需要从设备下挂的第三方列表中继续查找
device_list *device_list_find(char *guid, IPP_BOOL recursion);
//根据套接字fd查找设备
device_list *device_list_find_fd(int fd);
//根据SN查找设备的套接字fd
int device_fd_find(char *guid);
//根据SN获取设备的副本，使用者可修改
ipp_local_device *get_device_clone(char *guid);
//获取设备列表的副本，使用者可修改
ipp_device_list *get_device_list_clone();
//销毁设备列表
void device_list_destroy();
//打印设备列表
void print_device_list();
//设备列表加锁操作
void device_manager_lock();
//设备列表解锁操作
void device_manager_unlock();

#endif

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
void print_device_info(ipp_local_device *device);
#endif

#endif