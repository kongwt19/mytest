/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : 本地回调管理
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _LOCAL_HANDLER_MANAGER_H_
#define _LOCAL_HANDLER_MANAGER_H_

#include "local_module_enable.h"
#include "ipp_local_device_info.h"
#include "local_service.h"

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
//注册client接收到事件的处理回调
void reg_client_event_handler(char *cbName, void *param, event_handler handler);
//注册设备上线处理回调
void reg_client_device_online_handler(char *cbName, void *param, device_info_handler handler);
//注册设备下线处理回调
void reg_client_device_offline_handler(char *cbName, void *param, device_info_handler handler);
//设备回调列表销毁
void device_handler_destroy();
//调用所有设备上线处理回调
void call_device_online_handler(ipp_local_device* profile);
//调用所有设备下线处理回调
void call_device_offline_handler(ipp_local_device* profile);
//调用所有设备收到事件的处理回调
void call_event_handler(char *sn, int32_t data_len, char *data);
#endif

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
//注册服务端收到控制消息的回调
void reg_server_message_handler(void *param, request_handler handler);
//调用所有服务端收到消息的控制回调
ipp_protocol *call_server_message_handler(char *guid, ipp_protocol *protocol);
#endif

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
//注册服务端收到第三方设备控制的消息回调
void reg_server_agent_message_handler(void *param, request_handler handler);
//调用所有服务端收到第三方设备控制消息的回调
ipp_protocol *call_server_agent_message_handler(char *guid, ipp_protocol *protocol);
#endif

#endif