#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
#include "agent_device_manager.h"
#include <string.h>
#include <stdlib.h>
#include "ipp_local_device_info.h"
#include "local_common.h"

static ipp_device_list *g_agent_device_list = NULL;
static mutex_t g_agent_device_list_data_lock;

//��ʼ���������豸�б�
void init_agent_device_list()
{
	mutex_init(&g_agent_device_list_data_lock, NULL);
}

//�ڵ������豸��������һ���豸
int agent_device_list_append(ipp_local_device *device) {
	int ret;
	mutex_lock(&g_agent_device_list_data_lock);
	ret = ipp_device_list_append(&g_agent_device_list, device);
	mutex_unlock(&g_agent_device_list_data_lock);
	return ret;
}

//�����豸SN���б���ɾ��ָ���豸����������һ���豸��ָ��
ipp_device_list *agent_device_list_remove(char *guid) 
{
	ipp_device_list *next_device;
	mutex_lock(&g_agent_device_list_data_lock);
	next_device = ipp_device_list_remove(&g_agent_device_list, guid);
	mutex_unlock(&g_agent_device_list_data_lock);
	return next_device;
}

//���ٵ������豸�б�
void agent_device_list_destroy() 
{
	ipp_device_list *next;

	mutex_lock(&g_agent_device_list_data_lock);
	for (; g_agent_device_list; g_agent_device_list = next) {
		next = g_agent_device_list->next;
		free_local_device(&g_agent_device_list->device);
		FREE_POINTER(g_agent_device_list);
	}
	mutex_unlock(&g_agent_device_list_data_lock);
	mutex_free(&g_agent_device_list_data_lock);
}

//�����豸SN���б��в��Ҷ�Ӧ�豸���������豸ָ��
ipp_device_list *agent_device_list_find(char *guid) 
{
	if (NULL != g_agent_device_list) {
		ipp_device_list *ele = NULL;
		mutex_lock(&g_agent_device_list_data_lock);
		ele = g_agent_device_list;
		for (; ele; ele = ele->next) {
			if (NULL != ele->device) {
				if (0 == strcmp(ele->device->guid, guid)) {
					mutex_unlock(&g_agent_device_list_data_lock);
					return ele;
				}
			}
		}
		mutex_unlock(&g_agent_device_list_data_lock);
		return NULL;
	}

	return NULL;
}

//��ȡ�������豸�б�
ipp_device_list *get_agent_device_list() {
	return g_agent_device_list;
}

//��ȡ�������豸�б�ĸ���
ipp_device_list *get_agent_device_list_clone() {
	if (NULL != g_agent_device_list) {
		ipp_device_list *dlist = NULL;
		ipp_device_list *ele = NULL;
		mutex_lock(&g_agent_device_list_data_lock);
		ele = g_agent_device_list;
		for (; ele; ele = ele->next) {
			if (NULL != ele->device) {
				ipp_device_list_append(&dlist, ele->device);
			}
		}
		mutex_unlock(&g_agent_device_list_data_lock);

		return dlist;
	}

	return NULL;
}

//��ȡ�������豸����
int32_t agent_device_count()
{
	int32_t count = 0;
	if (NULL != g_agent_device_list) {
		ipp_device_list *ele = g_agent_device_list;
		mutex_lock(&g_agent_device_list_data_lock);
		while(ele) {
			ele = ele->next;
			++count;
		}
		mutex_unlock(&g_agent_device_list_data_lock);
	}

	return count;
}

//�������豸�б��������
void agent_device_manager_lock()
{
	mutex_lock(&g_agent_device_list_data_lock);
}

//�������豸�б����
void agent_device_manager_unlock()
{
	mutex_unlock(&g_agent_device_list_data_lock);
}

#endif
