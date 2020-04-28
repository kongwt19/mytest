/******************************************************************************

                  ��Ȩ���� (C), 1958-2018, �����������-IPP�ǻۼ�ͥҵ����

 ******************************************************************************
  �� �� ��   : ����
  ��    ��   : ���� 20167633
  ��������   : 2017��7��5��
  ����޸�   :
  ��������   : ����������ͳһ�ӿ�
  �޸���ʷ   :
  1.��    ��   : 2017��7��5��
    ��    ��   : ���� 20167633
    �޸�����   : �����ļ�
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

//�����豸���ƽ��
typedef enum
{
	control_success = 0,
	device_not_exist,
	control_failed,
	control_timed_out,
}local_control_result;

//-----------------------------------------�ص������ע��-----------------------------------------------
//������յ��ͻ��˵Ŀ���ָ��Ĵ���ص�����
typedef ipp_protocol *(*request_handler)(void* param, char *sn, char *msg, int32_t len);

//�ͻ����첽���Ʒ���˺��յ�����˷��ص���Ϣ����ص�����
typedef void (*response_handler)(void* param, char *sn, int32_t return_len, char *return_buffer);

//�ͻ����յ�����˵��¼���Ϣ����ص�����
typedef void (*event_handler)(void *param, char *sn, int32_t data_len, char *data);

//�ͻ����յ�����������߻ص�����
typedef void (*device_info_handler)(void *param, ipp_local_device *device);

//ע���¼������ص�����
void reg_local_event_handler(char *cbName, void *param, event_handler handler);
//ע���豸���߼����ص�����
void reg_local_device_online_handler(char *cbName, void *param, device_info_handler handler);
//ע���豸���߼����ص�����
void reg_local_device_offline_handler(char *cbName, void *param, device_info_handler handler);
//ע���յ���������ָ��ʱ�Ĵ���ص�����
void reg_local_message_handler(void *param, request_handler handler);
//ע���յ����Ƶ������豸ָ��ʱ�Ĵ���ص�����
void reg_local_agent_message_handler(void *param, request_handler handler);

//-----------------------------------------��ʼ����ֹͣ-----------------------------------------------
//��ʼ���������豸��Ϣ
void local_service_init(ipp_local_device *profile);
//������Դ
void local_service_deinit();
//��������������
void localservice_start(ipp_device_type init_type, int server_stack_size, int client_stack_size);
//ֹͣ����������
void localservice_stop();

//-----------------------------------------�豸����Ϳ���-----------------------------------------------
//��ȡ�豸�ĸ�������Ҫʹ������free_local_device���ٻ�ȡ�ĸ���
ipp_local_device *get_local_device(char *guid);
//��ȡ�豸�б�������Ҫʹ������free_local_device_list���ٻ񸱱�
ipp_device_list *get_local_device_list();
//�����豸
local_control_result control_local_device(char *guid, char* command, int command_len, char **response, int *rep_len);
//���������豸
IPP_BOOL control_all_local_devices(char *command, int command_len);
//�첽�����豸���豸�ظ��󣬻��Զ����ûص�����handler
local_control_result control_local_device_async(char *guid, char* command, int command_len, response_handler handler, void *param);
//�첽���������豸
IPP_BOOL control_all_local_device_async(char *command, int command_len);
//���ͱ����¼������пͻ������յ�
IPP_BOOL send_local_event(char *guid, char* command, int command_len);

//-----------------------------------------�������豸�ӿ�-----------------------------------------------
//�������豸���ߣ��˶����Ὣ�豸�����б����㲥������Ϣ
void local_agent_online(char *gateway_guid, ipp_local_device *device);
//�������豸���ߣ��˶����Ὣ�豸�Ƴ��б����㲥������Ϣ
void local_agent_offline(char *gateway_guid, ipp_local_device *device);
//�������豸�����ϱ����˶�����㲥����
void local_agent_update(char* guid, int data_len, char *agent_data);

//-----------------------------------------�����ӿ�(DEBUG)-----------------------------------------------
//�ͻ��˱����socket�б�
void print_local_client_sock_list();

//����˱����socket�б�
void print_local_server_sock_list();

#ifdef __cplusplus
}
#endif

#endif

