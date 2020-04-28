/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : 本地client功能管理
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _LOCAL_TCP_CLIENT_H_
#define _LOCAL_TCP_CLIENT_H_

#include "local_module_enable.h"

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE

#include "local_service.h"
#include "local_tcp.h"
#include "local_error_code.h"

//client功能初始化
int client_init();
//client开始连接一个server
int client_start(struct sockaddr_in *server_addr, char *target);
//获取client中最大的套接字
int get_client_max_fd();
//client模块的的fd set
void client_fd_set(fd_set *read_set);
//client处理流程
void client_process(fd_set *read_set);
//关闭client功能模块
void close_client();
//打印client的套接字和所连接的套接字列表
void print_client_sock_list();
//控制设备
local_error_code control_local_device_fd(int fd, char *sn, char* command, int command_len, char **response, int *rep_len);
//异步控制设备
local_error_code control_local_device_fd_async(int fd, char *sn, char* command, int command_len, response_handler handler, void *param);

#endif

#endif