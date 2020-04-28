/******************************************************************************

                  ��Ȩ���� (C), 1958-2018, �����������-IPP�ǻۼ�ͥҵ����

 ******************************************************************************
  �� �� ��   : ����
  ��    ��   : ���� 20167633
  ��������   : 2017��7��5��
  ����޸�   :
  ��������   : mdns����ģ��
  �޸���ʷ   :
  1.��    ��   : 2017��7��5��
    ��    ��   : ���� 20167633
    �޸�����   : �����ļ�
 ******************************************************************************/
#ifndef _LOCAL_MDNS_H_
#define _LOCAL_MDNS_H_

#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
#include "mdns_core.h"
#include "local_service.h"
#include "ipp_local_device_info.h"

#define FD_LIST_CNT 2

typedef struct mdnsd {
	mutex_t data_lock;				//������
	int mdns_fd[FD_LIST_CNT];		//mdns���׽��֣�5353�˿�
	int ipp_mdns_fd[FD_LIST_CNT];	//mdns���׽��֣�9393�˿�
	ipp_device_type init_type;		//��ʼ�����ͣ�server����client
	rr_qlist *rr_qn;				//��ע������
}mdnsd;

//mdns����ģ���ʼ��
int mdnsd_init(ipp_device_type init_type);
//mdnsģ���fd set
void mdnsd_fd_set(fd_set *read_set);
//���ڷ���request��response
void cycle_send();
//mdns���̴���
void mdnsd_process(fd_set *read_set);
//mdnsֹͣ
void mdnsd_stop();
//��ȡmdns�����������׽���
int get_mdns_max_fd();
//ʹ�ܹ㲥ͨ���ķ���
void enable_broadcast(IPP_BOOL enable);
#endif

#endif