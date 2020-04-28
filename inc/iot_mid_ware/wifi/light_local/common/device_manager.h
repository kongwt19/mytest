/******************************************************************************

                  ��Ȩ���� (C), 1958-2018, �����������-IPP�ǻۼ�ͥҵ����

 ******************************************************************************
  �� �� ��   : device_manager.h
  �� �� ��   : ����
  ��    ��   : ���� 20167633
  ��������   : 2017��7��5��
  ����޸�   :
  ��������   : �豸����ӿڣ���client���ֵ��豸����ͳһ����
  �޸���ʷ   :
  1.��    ��   : 2017��7��5��
    ��    ��   : ���� 20167633
    �޸�����   : �����ļ�
 ******************************************************************************/
#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

#include "local_module_enable.h"
#include "ipp_local_device_info.h"
#include "ipp_defines.h"

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE

//�豸�б�
typedef struct device_list
{
	int fd;									//�׽���
	uint32_t last_request_time;				//�ϴ�ѯ���豸��ϸ��Ϣ��ʱ��
	uint32_t added_time;					//�״μ��뵽�б��ʱ��
	ipp_local_device *device;				//�豸��Ϣ
	ipp_device_list *agent_device_list;		//���¹ҵĵ������豸�б�
	struct device_list *next;				//��һ��Ԫ�ص�ָ��
}device_list;

//��ʼ���豸�б�
void init_device_list();
//�����豸���룬���ն�Ӧfd������һ���ڵ㵽�б���ʱ�豸��ϸ��ϢΪ��
int device_list_append(int fd);
//��һ���������豸���뵽һ��ָ��sn�����豸�ĵ������б���
int device_list_append_agent(char *gateway_guid, ipp_local_device *device);
//�����豸��Ϣ���������豸״̬���»��߶�η���
IPP_BOOL update_device_info(int fd, ipp_local_device *element);
//���豸���б����Ƴ�����������һ���豸��ָ��
device_list *device_list_remove(int fd, ipp_local_device **removed_device);
//���������豸��ָ��sn�����豸�ĵ������豸�б����Ƴ�
int device_list_remove_agent(char *gateway_guid, ipp_local_device *device);
//�ж�ָ�����׽��ֽڵ��Ƿ���ϸ��ϢΪ��
IPP_BOOL is_device_list_empty(int fd);
//���Ѿ�����Ľڵ㣬����û����ϸ��Ϣ���Ƴ�
IPP_BOOL device_list_remove_empty(int fd);
//����SN�����豸��������ָ�룬recursion����ָ��֪����Ҫ���豸�¹ҵĵ������б��м�������
device_list *device_list_find(char *guid, IPP_BOOL recursion);
//�����׽���fd�����豸
device_list *device_list_find_fd(int fd);
//����SN�����豸���׽���fd
int device_fd_find(char *guid);
//����SN��ȡ�豸�ĸ�����ʹ���߿��޸�
ipp_local_device *get_device_clone(char *guid);
//��ȡ�豸�б�ĸ�����ʹ���߿��޸�
ipp_device_list *get_device_list_clone();
//�����豸�б�
void device_list_destroy();
//��ӡ�豸�б�
void print_device_list();
//�豸�б��������
void device_manager_lock();
//�豸�б��������
void device_manager_unlock();

#endif

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
void print_device_info(ipp_local_device *device);
#endif

#endif