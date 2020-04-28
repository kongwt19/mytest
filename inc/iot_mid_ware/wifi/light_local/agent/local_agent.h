/******************************************************************************

                  ��Ȩ���� (C), 1958-2018, �����������-IPP�ǻۼ�ͥҵ����

 ******************************************************************************
  �� �� ��   : local_agent.h
  �� �� ��   : ����
  ��    ��   : ���� 20167633
  ��������   : 2017��7��5��
  ����޸�   :
  ��������   : �������豸����ӿ�
  �޸���ʷ   :
  1.��    ��   : 2017��7��5��
    ��    ��   : ���� 20167633
    �޸�����   : �����ļ�
 ******************************************************************************/
#ifndef _LOCAL_AGENT_H_
#define _LOCAL_AGENT_H_

#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
#include "local_service.h"
#include "ipp_defines.h"
#include "ipp_local_device_info.h"

//�������豸����
void agent_online(char *gateway_guid, ipp_local_device *device);
//�������豸����
void agent_offline(char *gateway_guid, ipp_local_device *device);
//�������豸��״̬�ϱ�
void agent_update(char* guid, int data_len, char *agent_data);
#endif

#endif