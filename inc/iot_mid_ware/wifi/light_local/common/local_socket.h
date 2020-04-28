/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : 本地网络套接字管理，创建套接字、接收数据、关闭套接字等
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _LOCAL_SOCKET_H_
#define _LOCAL_SOCKET_H_

#include "local_module_enable.h"
#include "platform.h"
#include <stdint.h>

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)

typedef struct peer_info
{
	int fd;								//套接字
	uint32_t last_recv_heart_beat_time; //上次接收到心跳的时间
	uint32_t last_send_heart_beat_time;	//上次发送心跳的时间
	struct in_addr peer_addr;			//对端地址信息
	uint16_t peer_port;					//对端的端口信息
	uint16_t udp_peer_port;				//对端的udp端口信息
	struct peer_info *next;				//下一个元素
}peer_info;

//将server信息添加到server列表中
int peer_info_list_append(peer_info **head, peer_info *element);
//将server信息从server列表中删除
peer_info *peer_info_list_remove(peer_info **head, peer_info *info);
//销毁server列表
void peer_info_list_destroy(peer_info **head);
//根据ip地址和端口号查找server信息节点
peer_info *peer_info_list_find(peer_info *server_list, struct in_addr server_addr, uint16_t port);

#endif

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
//创建server需要的tcp套接字
int create_tcp_server_sock(uint16_t *server_port, uint8_t listen_cnt);
//创建server需要的udp套接字
int create_udp_server_sock();
#endif

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
//创建client需要的tcp套接字
int create_tcp_client_sock();
//创建client需要的udp套接字
int create_udp_client_sock(uint16_t *client_port);
#endif

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
//创建mdns需要的udp套接字
int create_mdns_sock(uint16_t mdns_port, const char *mdns_addr);
//在指定的套接字上发送buffer
int send_buffer(int s, char *buffer, int len);
//从指定socket获取数据，函数内部出现异常，不会释放buffer，尽量返回可处理的字节，所以无论函数返回什么，使用者都一定要释放buffer
int recv_from_sock(int s, char **buffer);
//关闭套接字
int local_close_sock(int s);
//停止套接字
int local_shutdown_sock(int s);
//获取socket的错误
int get_sock_err();
//设置套接字发送超时
int set_send_timeout(int fd, int second);
//设置套接字接收超时
int set_recv_timeout(int fd, int second);
//设置关闭的行为
int set_so_linger(int fd);
//设为非阻塞模式
int set_no_blocking(int fd);
#endif

#endif
