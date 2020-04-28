/******************************************************************************

                  ��Ȩ���� (C), 1958-2018, �����������-IPP�ǻۼ�ͥҵ����

 ******************************************************************************
  �� �� ��   : ����
  ��    ��   : ���� 20167633
  ��������   : 2017��7��5��
  ����޸�   :
  ��������   : ���ػص�����
  �޸���ʷ   :
  1.��    ��   : 2017��7��5��
    ��    ��   : ���� 20167633
    �޸�����   : �����ļ�
 ******************************************************************************/
#ifndef _LOCAL_HANDLER_MANAGER_H_
#define _LOCAL_HANDLER_MANAGER_H_

#include "local_module_enable.h"
#include "ipp_local_device_info.h"
#include "local_service.h"

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
//ע��client���յ��¼��Ĵ���ص�
void reg_client_event_handler(char *cbName, void *param, event_handler handler);
//ע���豸���ߴ���ص�
void reg_client_device_online_handler(char *cbName, void *param, device_info_handler handler);
//ע���豸���ߴ���ص�
void reg_client_device_offline_handler(char *cbName, void *param, device_info_handler handler);
//�豸�ص��б�����
void device_handler_destroy();
//���������豸���ߴ���ص�
void call_device_online_handler(ipp_local_device* profile);
//���������豸���ߴ���ص�
void call_device_offline_handler(ipp_local_device* profile);
//���������豸�յ��¼��Ĵ���ص�
void call_event_handler(char *sn, int32_t data_len, char *data);
#endif

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
//ע�������յ�������Ϣ�Ļص�
void reg_server_message_handler(void *param, request_handler handler);
//�������з�����յ���Ϣ�Ŀ��ƻص�
ipp_protocol *call_server_message_handler(char *guid, ipp_protocol *protocol);
#endif

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
//ע�������յ��������豸���Ƶ���Ϣ�ص�
void reg_server_agent_message_handler(void *param, request_handler handler);
//�������з�����յ��������豸������Ϣ�Ļص�
ipp_protocol *call_server_agent_message_handler(char *guid, ipp_protocol *protocol);
#endif

#endif