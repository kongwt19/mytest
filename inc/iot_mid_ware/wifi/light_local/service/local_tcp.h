/******************************************************************************

                  ��Ȩ���� (C), 1958-2018, �����������-IPP�ǻۼ�ͥҵ����

 ******************************************************************************
  �� �� ��   : ����
  ��    ��   : ���� 20167633
  ��������   : 2017��7��5��
  ����޸�   :
  ��������   : ����tcp���ܹ���
  �޸���ʷ   :
  1.��    ��   : 2017��7��5��
    ��    ��   : ���� 20167633
    �޸�����   : �����ļ�
 ******************************************************************************/
#ifndef _LOCAL_TCP_H_
#define _LOCAL_TCP_H_

#include "local_module_enable.h"

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
#include "local_service.h"
#include "ipp_defines.h"
#include "ipp_protocol.h"

#define ADDR_STR_LEN  22

//�豸������Ϣ
typedef enum
{
	command_get_device_info,	//��ȡ�豸��Ϣ
	command_control_device,		//�����豸
	command_event,				//�����¼�
	command_heart_beat,			//��������
	command_agent_online,		//�������豸����
	command_agent_offline,		//�������豸����
	command_agent_update,		//�������豸״̬����
	command_get_agent_info,		//��ȡ�������豸�б���Ϣ
}command_code;

//TCP����ʼͷ��Ϣ
static const int32_t TCP_HEAD = 0x2748ADAF;

#ifdef __cplusplus
extern "C" {
#endif

//tcp(server+client)ģ���ʼ��
int tcpd_init(ipp_device_type init_type);
//��ʼ��tcp��Ϣ����
ipp_protocol *create_tcp_protocol(int size);
//����tcp��Ϣ�����С
void set_tcp_protocol_size(ipp_protocol *p, int size);
//�����յ�����Ϣ����
void deal_buffer(void *par, ipp_protocol **global_protocol, char *buffer, size_t buffer_len,
				  void (*deal_one)(void *par, char *buffer, int buffer_len));
//��������
void send_heart_beat(int fd);
//��ӡ�����豸��Ϣ
void check_devices(uint8_t print_now);

#ifdef __cplusplus
}
#endif

#endif

#endif

