/******************************************************************************

                  ��Ȩ���� (C), 1958-2018, �����������-IPP�ǻۼ�ͥҵ����

 ******************************************************************************
  �� �� ��   : ����
  ��    ��   : ���� 20167633
  ��������   : 2017��7��5��
  ����޸�   :
  ��������   : ����Server���ܹ���
  �޸���ʷ   :
  1.��    ��   : 2017��7��5��
    ��    ��   : ���� 20167633
    �޸�����   : �����ļ�
 ******************************************************************************/
#ifndef _LOCAL_TCP_SERVER_H_
#define _LOCAL_TCP_SERVER_H_

#include "local_module_enable.h"

#if IPP_LIGHT_LOCAL_SERVER_ENABLE

#include "local_service.h"
#include "ipp_defines.h"
#include "ipp_local_device_info.h"

//serverģ���ʼ��
int server_init();
//��ȡserverģ������fd
int get_server_max_fd();
//serverģ���fd set
void server_fd_set(fd_set *read_set);
//server��������
void server_process(fd_set *read_set);
//�ر�serverģ��
void close_server();
//�����¼���Ϣ������client�����յ�
IPP_BOOL server_send_event(char *guid, char *eve, int eve_len);
//���͹㲥��Ϣ������client�����յ�
void server_send_broadcast(int32_t command_id, char *data, int data_len);
//��ӡ����server������client��Ϣ
void print_server_sock_list();

#endif

#endif