/******************************************************************************

                  ��Ȩ���� (C), 1958-2018, �����������-IPP�ǻۼ�ͥҵ����

 ******************************************************************************
  �� �� ��   : ����
  ��    ��   : ���� 20167633
  ��������   : 2017��7��5��
  ����޸�   :
  ��������   : ����client���ܹ���
  �޸���ʷ   :
  1.��    ��   : 2017��7��5��
    ��    ��   : ���� 20167633
    �޸�����   : �����ļ�
 ******************************************************************************/
#ifndef _LOCAL_TCP_CLIENT_H_
#define _LOCAL_TCP_CLIENT_H_

#include "local_module_enable.h"

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE

#include "local_service.h"
#include "local_tcp.h"
#include "local_error_code.h"

//client���ܳ�ʼ��
int client_init();
//client��ʼ����һ��server
int client_start(struct sockaddr_in *server_addr, char *target);
//��ȡclient�������׽���
int get_client_max_fd();
//clientģ��ĵ�fd set
void client_fd_set(fd_set *read_set);
//client��������
void client_process(fd_set *read_set);
//�ر�client����ģ��
void close_client();
//��ӡclient���׽��ֺ������ӵ��׽����б�
void print_client_sock_list();
//�����豸
local_error_code control_local_device_fd(int fd, char *sn, char* command, int command_len, char **response, int *rep_len);
//�첽�����豸
local_error_code control_local_device_fd_async(int fd, char *sn, char* command, int command_len, response_handler handler, void *param);

#endif

#endif