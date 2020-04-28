/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : 本地Server功能管理
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _LOCAL_TCP_SERVER_H_
#define _LOCAL_TCP_SERVER_H_

#include "local_module_enable.h"

#if IPP_LIGHT_LOCAL_SERVER_ENABLE

#include "local_service.h"
#include "ipp_defines.h"
#include "ipp_local_device_info.h"

//server模块初始化
int server_init();
//获取server模块最大的fd
int get_server_max_fd();
//server模块的fd set
void server_fd_set(fd_set *read_set);
//server处理流程
void server_process(fd_set *read_set);
//关闭server模块
void close_server();
//发送事件消息，所有client均能收到
IPP_BOOL server_send_event(char *guid, char *eve, int eve_len);
//发送广播消息，所有client均能收到
void server_send_broadcast(int32_t command_id, char *data, int data_len);
//打印连接server的所有client信息
void print_server_sock_list();

#endif

#endif