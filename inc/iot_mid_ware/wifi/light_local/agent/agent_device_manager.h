/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  文 件 名   : agent_device_manager.h
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : 第三方设备管理，管理设备作为Server时，其实际下挂的第三方设备
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _AGENT_DEVICE_MANAGER_H_
#define _AGENT_DEVICE_MANAGER_H_

#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
#include "ipp_defines.h"
#include "ipp_local_device_info.h"

//初始化第三方设备列表
void init_agent_device_list();
//在第三方设备类表中添加一个设备
int agent_device_list_append(ipp_local_device *device);
//根据设备SN从列表中删除指定设备，并返回下一个设备的指针
ipp_device_list *agent_device_list_remove(char *guid);
//根据设备SN从列表中查找对应设备，并返回设备指针
ipp_device_list *agent_device_list_find(char *guid);
//获取第三方设备列表的副本，使用者可对其内容进行修改
ipp_device_list *get_agent_device_list_clone();
//获取第三方设备列表，此处直接返回第三方设备列表指针，使用者勿对其内容进行修改
ipp_device_list *get_agent_device_list();
//获取第三方设备个数
int32_t agent_device_count();
//销毁第三方设备列表
void agent_device_list_destroy();
//第三方设备列表操作加锁
void agent_device_manager_lock();
//第三方设备列表解锁
void agent_device_manager_unlock();

#endif

#endif
