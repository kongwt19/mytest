/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : 本地tcp功能管理
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _LOCAL_TCP_H_
#define _LOCAL_TCP_H_

#include "local_module_enable.h"

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
#include "local_service.h"
#include "ipp_defines.h"
#include "ipp_protocol.h"

#define ADDR_STR_LEN  22

//设备控制消息
typedef enum
{
	command_get_device_info,	//获取设备信息
	command_control_device,		//控制设备
	command_event,				//发送事件
	command_heart_beat,			//发送心跳
	command_agent_online,		//第三方设备上线
	command_agent_offline,		//第三方设备下线
	command_agent_update,		//第三方设备状态更新
	command_get_agent_info,		//获取第三方设备列表信息
}command_code;

//TCP的起始头信息
static const int32_t TCP_HEAD = 0x2748ADAF;

#ifdef __cplusplus
extern "C" {
#endif

//tcp(server+client)模块初始化
int tcpd_init(ipp_device_type init_type);
//初始化tcp消息缓存
ipp_protocol *create_tcp_protocol(int size);
//设置tcp消息缓存大小
void set_tcp_protocol_size(ipp_protocol *p, int size);
//处理收到的消息缓存
void deal_buffer(void *par, ipp_protocol **global_protocol, char *buffer, size_t buffer_len,
				  void (*deal_one)(void *par, char *buffer, int buffer_len));
//发送心跳
void send_heart_beat(int fd);
//打印所有设备信息
void check_devices(uint8_t print_now);

#ifdef __cplusplus
}
#endif

#endif

#endif

