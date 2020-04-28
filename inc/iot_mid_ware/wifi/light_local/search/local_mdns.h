/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : mdns发现模块
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _LOCAL_MDNS_H_
#define _LOCAL_MDNS_H_

#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
#include "mdns_core.h"
#include "local_service.h"
#include "ipp_local_device_info.h"

#define FD_LIST_CNT 2

typedef struct mdnsd {
	mutex_t data_lock;				//互斥锁
	int mdns_fd[FD_LIST_CNT];		//mdns的套接字，5353端口
	int ipp_mdns_fd[FD_LIST_CNT];	//mdns的套接字，9393端口
	ipp_device_type init_type;		//初始化类型，server还是client
	rr_qlist *rr_qn;				//关注的主题
}mdnsd;

//mdns发现模块初始化
int mdnsd_init(ipp_device_type init_type);
//mdns模块的fd set
void mdnsd_fd_set(fd_set *read_set);
//周期发送request和response
void cycle_send();
//mdns过程处理
void mdnsd_process(fd_set *read_set);
//mdns停止
void mdnsd_stop();
//获取mdns流程中最大的套接字
int get_mdns_max_fd();
//使能广播通道的发现
void enable_broadcast(IPP_BOOL enable);
#endif

#endif