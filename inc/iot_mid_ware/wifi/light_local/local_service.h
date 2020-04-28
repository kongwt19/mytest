/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : 轻量级本地统一接口
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _C_LOCAL_SERVICE_H_
#define _C_LOCAL_SERVICE_H_

#include <stdint.h>
#include "ipp_defines.h"
#include "ipp_local_device_info.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ipp_local_device *g_self;

//本地设备控制结果
typedef enum
{
	control_success = 0,
	device_not_exist,
	control_failed,
	control_timed_out,
}local_control_result;

//-----------------------------------------回调定义和注册-----------------------------------------------
//服务端收到客户端的控制指令的处理回调函数
typedef ipp_protocol *(*request_handler)(void* param, char *sn, char *msg, int32_t len);

//客户端异步控制服务端后，收到服务端返回的消息处理回调函数
typedef void (*response_handler)(void* param, char *sn, int32_t return_len, char *return_buffer);

//客户端收到服务端的事件消息处理回调函数
typedef void (*event_handler)(void *param, char *sn, int32_t data_len, char *data);

//客户端收到服务端上下线回调函数
typedef void (*device_info_handler)(void *param, ipp_local_device *device);

//注册事件监听回调函数
void reg_local_event_handler(char *cbName, void *param, event_handler handler);
//注册设备上线监听回调函数
void reg_local_device_online_handler(char *cbName, void *param, device_info_handler handler);
//注册设备下线监听回调函数
void reg_local_device_offline_handler(char *cbName, void *param, device_info_handler handler);
//注册收到控制自身指令时的处理回调函数
void reg_local_message_handler(void *param, request_handler handler);
//注册收到控制第三方设备指令时的处理回调函数
void reg_local_agent_message_handler(void *param, request_handler handler);

//-----------------------------------------初始化和停止-----------------------------------------------
//初始化，填入设备信息
void local_service_init(ipp_local_device *profile);
//销毁资源
void local_service_deinit();
//启动轻量级本地
void localservice_start(ipp_device_type init_type, int server_stack_size, int client_stack_size);
//停止轻量级本地
void localservice_stop();

//-----------------------------------------设备管理和控制-----------------------------------------------
//获取设备的副本，需要使用者用free_local_device销毁获取的副本
ipp_local_device *get_local_device(char *guid);
//获取设备列表副本，需要使用者用free_local_device_list销毁获副本
ipp_device_list *get_local_device_list();
//控制设备
local_control_result control_local_device(char *guid, char* command, int command_len, char **response, int *rep_len);
//控制所有设备
IPP_BOOL control_all_local_devices(char *command, int command_len);
//异步控制设备，设备回复后，会自动调用回掉函数handler
local_control_result control_local_device_async(char *guid, char* command, int command_len, response_handler handler, void *param);
//异步控制所有设备
IPP_BOOL control_all_local_device_async(char *command, int command_len);
//发送本地事件，所有客户端能收到
IPP_BOOL send_local_event(char *guid, char* command, int command_len);

//-----------------------------------------第三方设备接口-----------------------------------------------
//第三方设备上线，此动作会将设备加入列表，并广播上线消息
void local_agent_online(char *gateway_guid, ipp_local_device *device);
//第三方设备下线，此动作会将设备移除列表，并广播下线消息
void local_agent_offline(char *gateway_guid, ipp_local_device *device);
//第三方设备数据上报，此动作会广播数据
void local_agent_update(char* guid, int data_len, char *agent_data);

//-----------------------------------------其他接口(DEBUG)-----------------------------------------------
//客户端保存的socket列表
void print_local_client_sock_list();

//服务端保存的socket列表
void print_local_server_sock_list();

#ifdef __cplusplus
}
#endif

#endif

