/******************************************************************************

                  ��Ȩ���� (C), 1958-2018, �����������-IPP�ǻۼ�ͥҵ����

 ******************************************************************************
  �� �� ��   : ����
  ��    ��   : ���� 20167633
  ��������   : 2017��7��5��
  ����޸�   :
  ��������   : ���������׽��ֹ��������׽��֡��������ݡ��ر��׽��ֵ�
  �޸���ʷ   :
  1.��    ��   : 2017��7��5��
    ��    ��   : ���� 20167633
    �޸�����   : �����ļ�
 ******************************************************************************/
#ifndef _LOCAL_SOCKET_H_
#define _LOCAL_SOCKET_H_

#include "local_module_enable.h"
#include "platform.h"
#include <stdint.h>

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)

typedef struct peer_info
{
	int fd;								//�׽���
	uint32_t last_recv_heart_beat_time; //�ϴν��յ�������ʱ��
	uint32_t last_send_heart_beat_time;	//�ϴη���������ʱ��
	struct in_addr peer_addr;			//�Զ˵�ַ��Ϣ
	uint16_t peer_port;					//�Զ˵Ķ˿���Ϣ
	uint16_t udp_peer_port;				//�Զ˵�udp�˿���Ϣ
	struct peer_info *next;				//��һ��Ԫ��
}peer_info;

//��server��Ϣ��ӵ�server�б���
int peer_info_list_append(peer_info **head, peer_info *element);
//��server��Ϣ��server�б���ɾ��
peer_info *peer_info_list_remove(peer_info **head, peer_info *info);
//����server�б�
void peer_info_list_destroy(peer_info **head);
//����ip��ַ�Ͷ˿ںŲ���server��Ϣ�ڵ�
peer_info *peer_info_list_find(peer_info *server_list, struct in_addr server_addr, uint16_t port);

#endif

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
//����server��Ҫ��tcp�׽���
int create_tcp_server_sock(uint16_t *server_port, uint8_t listen_cnt);
//����server��Ҫ��udp�׽���
int create_udp_server_sock();
#endif

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
//����client��Ҫ��tcp�׽���
int create_tcp_client_sock();
//����client��Ҫ��udp�׽���
int create_udp_client_sock(uint16_t *client_port);
#endif

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
//����mdns��Ҫ��udp�׽���
int create_mdns_sock(uint16_t mdns_port, const char *mdns_addr);
//��ָ�����׽����Ϸ���buffer
int send_buffer(int s, char *buffer, int len);
//��ָ��socket��ȡ���ݣ������ڲ������쳣�������ͷ�buffer���������ؿɴ�����ֽڣ��������ۺ�������ʲô��ʹ���߶�һ��Ҫ�ͷ�buffer
int recv_from_sock(int s, char **buffer);
//�ر��׽���
int local_close_sock(int s);
//ֹͣ�׽���
int local_shutdown_sock(int s);
//��ȡsocket�Ĵ���
int get_sock_err();
//�����׽��ַ��ͳ�ʱ
int set_send_timeout(int fd, int second);
//�����׽��ֽ��ճ�ʱ
int set_recv_timeout(int fd, int second);
//���ùرյ���Ϊ
int set_so_linger(int fd);
//��Ϊ������ģʽ
int set_no_blocking(int fd);
#endif

#endif
