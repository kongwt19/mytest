/******************************************************************************

                  ��Ȩ���� (C), 1958-2018, �����������-IPP�ǻۼ�ͥҵ����

 ******************************************************************************
  �� �� ��   : ����
  ��    ��   : ���� 20167633
  ��������   : 2017��7��5��
  ����޸�   :
  ��������   : �����豸�б����
  �޸���ʷ   :
  1.��    ��   : 2017��7��5��
    ��    ��   : ���� 20167633
    �޸�����   : �����ļ�
 ******************************************************************************/
#ifndef _IPP_LOCAL_DEVICE_INFO_
#define _IPP_LOCAL_DEVICE_INFO_

#include <stdint.h>
#include "ipp_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

//�豸���Խṹ��
typedef struct _ipp_local_device
{
	char *guid;				//�豸Ψһ��ʶ
	char *product_id;		//�豸��Ʒ��ʶ
	char *software_ver;		//�豸����汾��
	int32_t device_type;	//�豸����
	char *mac;				//�豸MAC��ַ
	int8_t connet_type;		//��ȡ��������:������zigbee
	char *name;				//�豸����(�ǳ�)
	int8_t is_master;		//�Ƿ������豸
	char *gateway_guid;		//�������ر�ʶ��������������Ϊ�ջ��������guid
	int8_t *reserved;		//Ԥ���ֶΣ����Զ���
	int32_t reserved_len;	//Ԥ���ֶγ���
}ipp_local_device;

//�豸�б�ṹ��
typedef struct ipp_device_list
{
	ipp_local_device *device;
	struct ipp_device_list *next;
}ipp_device_list;

//SDK����ʱ���豸��ɫ
typedef enum
{
	ipp_device_client  = 0x1,	//����Ϊ�ͻ���
	ipp_device_server  = 0x2,	//����Ϊ�����
	ipp_device_both = ipp_device_client | ipp_device_server,//ͬʱ��Ϊ�ͻ��˺ͷ����
	ipp_device_invalid,
}ipp_device_type;

/*
  ��������	create_local_device
  ���ܣ�	����һ�������豸��Ϣ�ṹ�壨�ڲ��Ѿ���ʼ����
  ��Σ�	��
  ���Σ�	��
  ����ֵ��	ipp_device_profile *  �豸��Ϣ�ṹ�壬��Ҫʹ����ʹ��free_local_device�ͷ�
*/
ipp_local_device *create_local_device();

/*
  ��������	init_local_device
  ���ܣ�	��һ�������豸��Ϣ��ʼ���������ֶ�����ΪĬ��ֵ
  ��Σ�	ipp_local_device *profile  �����豸��Ϣ�ṹ��
  ���Σ�	��
  ����ֵ��	��
*/
void init_local_device(ipp_local_device *profile);

/*
  ��������	init_local_device_from_protocol
  ���ܣ�	��һ�������豸��Ϣ��ʼ����һ����Ϣ�����н�������
  ��Σ�	ipp_local_device *profile  �����豸��Ϣ�ṹ�壬��Ž�������������
			ipp_protocol *protocol     ��Ϣ����
  ���Σ�	��
  ����ֵ��	1���ɹ���-1��ʧ��
*/
int init_local_device_from_protocol(ipp_local_device *profile, ipp_protocol *protocol);

/*
  ��������	free_device_info
  ���ܣ�	�ͷ��豸��Ϣ�ṹ��
  ��Σ�	ipp_device_profile *profile
  ���Σ�	��
  ����ֵ��	��
*/
void free_local_device(ipp_local_device **profile);

/*
  ��������	free_local_device_list
  ���ܣ�	�ͷ��豸�б�
  ��Σ�	ipp_device_list *profile_list
  ���Σ�	��
  ����ֵ��	��
*/
void free_local_device_list(ipp_device_list **profile_list);

/*
  ��������	get_device_clone_from_other
  ���ܣ�	���һ���豸��Ϣ����������ֵ��Ҫʹ�����ͷ�
  ��Σ�	ipp_local_device *other
  ���Σ�	��
  ����ֵ��	ipp_local_device	�豸��Ϣ����
*/
ipp_local_device *get_device_clone_from_other(ipp_local_device *other);

/*
  ��������	ipp_device_list_append
  ���ܣ�	��һ���豸��ӵ��豸�б���
  ��Σ�	ipp_device_list **head		�豸�б�ͷָ��
			ipp_local_device *element	Ҫ��ӵ��豸��Ϣ
  ���Σ�	��
  ����ֵ��	int							��ӳɹ�����1�����򷵻�<=0
*/
int ipp_device_list_append(ipp_device_list **head, ipp_local_device *element);

/*
  ��������	ipp_device_list_remove
  ���ܣ�	��һ���豸��Ӵ��豸�б����Ƴ�
  ��Σ�	ipp_device_list **head		�豸�б�ͷָ��
			char *guidt					Ҫ�Ƴ����豸��Ϣ��sn
  ���Σ�	��
  ����ֵ��	ipp_device_list*			��һ���豸��ָ��
*/
ipp_device_list *ipp_device_list_remove(ipp_device_list **head, char *guid);

/*
  ��������	ipp_device_list_find
  ���ܣ�	����SN���豸�б��в����豸
  ��Σ�	ipp_device_list **head		�豸�б�ͷָ��
			char *guid					Ҫ���ҵ��豸��Ϣ��sn
  ���Σ�	��
  ����ֵ��	ipp_local_device *			�ҵ����豸ָ��
*/
ipp_local_device *ipp_device_list_find(ipp_device_list **head, char *guid);

/*
  ��������	get_device_info_protocol_len
  ���ܣ�	�����豸��Ϣ�ṹ���ȡ���ʹ��豸��Ϣ��Ҫ�Ļ��泤��
  ��Σ�	ipp_local_device *device	�豸��Ϣ�ṹ��
  ���Σ�	��
  ����ֵ��	int							��Ҫ�Ļ��泤��
*/
int get_device_info_protocol_len(ipp_local_device *device);

/*
  ��������	fill_to_protocol
  ���ܣ�	���豸��Ϣ��䵽������
  ��Σ�	ipp_protocol *p				����
			ipp_local_device *device	�豸��Ϣ
  ���Σ�	��
  ����ֵ��	int32_t						����������
*/
int32_t fill_to_protocol(ipp_protocol *p, ipp_local_device *device);

#ifdef __cplusplus
}
#endif

#endif
