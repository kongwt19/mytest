/*******************************************************************************************************
**                                           File description
** File Name:           call_back.h
** Description:         ����ص�����
** Creator:             xieshaobing
** Creation Date:       2017��4��20��
** Modify Log:          none
** Last Modified Date:  2017��4��20��
*******************************************************************************************************/

#ifndef	__REX_CALL_BACK_H
#define	__REX_CALL_BACK_H

/* If this is a C++ compiler, use C linkage */
#if defined(__cplusplus) 
extern "C"
{
#endif

/**
 * call_back_device_join_cb
 *     �豸�����ص�����
 * address[in]: �豸��ַ
 * reporting[in]: �ϱ�����
 * type[in]: �豸����
 * model_id[in]: Model Id
 * return: 0�ɹ�,����-1
 * note: none
 */
extern int call_back_device_join_cb(char *address, unsigned short reporting, char *type, char *model_id);

/**
 * call_back_device_leave_cb
 *     �豸�����ص�����
 * address[in]: �豸��ַ
 * return: 0�ɹ�,����-1
 * note: none
 */
extern int call_back_device_leave_cb(char *address);

/**
 * call_back_report_state_data_cb
 *     �ϱ�״̬���ݻص�����
 * address[in]: ���õ�ַ
 * endpoint_id[in]: �豸��ID��,Ĭ��Ϊ1,��·�豸��1���ε���
 * state_type[in]: ״̬����
 * state[in]: ״̬����,ΪJSON�ַ���
 * return: 0�ɹ�,����-1
 * note: none
 */
extern int call_back_report_state_data_cb(char *address, unsigned char endpoint_id, unsigned short state_type, char *
state);

/**
 * call_back_report_event_data_cb
 *     �ϱ��¼����ݻص�����
 * address[in]: ���õ�ַ
 * endpoint_id[in]: �豸��ID��,Ĭ��Ϊ1,��·�豸��1���ε���
 * event_type[in]: �¼�����
 * event[in]: �¼�����,ΪJSON�ַ���
 * return: 0�ɹ�,����-1
 * note: none
 */
extern int call_back_report_event_data_cb(char *address, unsigned char endpoint_id, unsigned short event_type, char *
event);

/**
 * call_back_report_self_defining_data_cb
 *     �ϱ��Զ������ݻص�����
 * address[in]: ���õ�ַ
 * endpoint_id[in]: �豸��ID��,Ĭ��Ϊ1,��·�豸��1���ε���
 * data[in]: �Զ�������
 * length[in]: �Զ������ݳ���
 * return: 0�ɹ�,����-1
 * note: none
 */
extern int call_back_report_self_defining_data_cb(char *address, unsigned char endpoint_id, char *data, unsigned 
short length);

/* If this is a C++ compiler, use C linkage */
#if defined(__cplusplus) 
}
#endif

#endif
/*******************************************************************************************************
**                                           End of file
*******************************************************************************************************/

