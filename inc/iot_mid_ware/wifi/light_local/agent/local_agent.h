/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  文 件 名   : local_agent.h
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : 第三方设备管理接口
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _LOCAL_AGENT_H_
#define _LOCAL_AGENT_H_

#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
#include "local_service.h"
#include "ipp_defines.h"
#include "ipp_local_device_info.h"

//第三方设备上线
void agent_online(char *gateway_guid, ipp_local_device *device);
//第三方设备下线
void agent_offline(char *gateway_guid, ipp_local_device *device);
//第三方设备有状态上报
void agent_update(char* guid, int data_len, char *agent_data);
#endif

#endif