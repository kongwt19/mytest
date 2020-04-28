/******************************************************************************

                  ��Ȩ���� (C), 1958-2018, �����������-IPP�ǻۼ�ͥҵ����

 ******************************************************************************
  �� �� ��   : agent_device_manager.h
  �� �� ��   : ����
  ��    ��   : ���� 20167633
  ��������   : 2017��7��5��
  ����޸�   :
  ��������   : �������豸���������豸��ΪServerʱ����ʵ���¹ҵĵ������豸
  �޸���ʷ   :
  1.��    ��   : 2017��7��5��
    ��    ��   : ���� 20167633
    �޸�����   : �����ļ�
 ******************************************************************************/
#ifndef _AGENT_DEVICE_MANAGER_H_
#define _AGENT_DEVICE_MANAGER_H_

#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
#include "ipp_defines.h"
#include "ipp_local_device_info.h"

//��ʼ���������豸�б�
void init_agent_device_list();
//�ڵ������豸��������һ���豸
int agent_device_list_append(ipp_local_device *device);
//�����豸SN���б���ɾ��ָ���豸����������һ���豸��ָ��
ipp_device_list *agent_device_list_remove(char *guid);
//�����豸SN���б��в��Ҷ�Ӧ�豸���������豸ָ��
ipp_device_list *agent_device_list_find(char *guid);
//��ȡ�������豸�б�ĸ�����ʹ���߿ɶ������ݽ����޸�
ipp_device_list *get_agent_device_list_clone();
//��ȡ�������豸�б��˴�ֱ�ӷ��ص������豸�б�ָ�룬ʹ������������ݽ����޸�
ipp_device_list *get_agent_device_list();
//��ȡ�������豸����
int32_t agent_device_count();
//���ٵ������豸�б�
void agent_device_list_destroy();
//�������豸�б��������
void agent_device_manager_lock();
//�������豸�б����
void agent_device_manager_unlock();

#endif

#endif
