#include "local_module_enable.h"
#include "device_manager.h"
#include <string.h>
#include <stdlib.h>
#include "ipp_local_device_info.h"
#include "local_common.h"
//#include "ipp_util.h"

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE

static device_list *g_device_list = NULL;
static mutex_t g_device_list_data_lock;
static const uint8_t MAX_INFO_WAIT_TIME = 60;

//��ʼ���豸�б�
void init_device_list()
{
	mutex_init(&g_device_list_data_lock, NULL);
}

//��ʼ��һ���豸����ڵ�
device_list *init_device_list_node(int fd, ipp_local_device *profile)
{
	device_list *node = (device_list *)malloc(sizeof(device_list));
	MALLOC_ERROR_RETURN_WTTH_RET(node, NULL)

	node->fd = fd;
	node->last_request_time = 0;
	node->added_time = get_process_msec();
	node->device = profile;
	node->agent_device_list = NULL;
	node->next = NULL;

	return node;
}

//�����豸���룬���ն�Ӧfd������һ���ڵ㵽�б�����ʱ�豸��ϸ��ϢΪ��
int device_list_append(int fd) {
	device_list *node = init_device_list_node(fd, NULL);
	MALLOC_ERROR_RETURN_WTTH_RET(node, -1)

	mutex_lock(&g_device_list_data_lock);
	if (g_device_list == NULL) {
		g_device_list = node;
	} else {
		device_list *e = g_device_list, *tail_e = NULL;
		for (; e; e = e->next) {
			// already in list - don't add
			if (e->fd == fd) {
				FREE_POINTER(node);
				mutex_unlock(&g_device_list_data_lock);
				return 0;
			}
			if (e->next == NULL)
				tail_e = e;
		}

		if (NULL != tail_e)
			tail_e->next = node;
		else
			FREE_POINTER(node);
	}
	mutex_unlock(&g_device_list_data_lock);
	return 1;
}

//��һ���������豸���뵽һ��ָ��sn�����豸�ĵ������б���
int device_list_append_agent(char *gateway_guid, ipp_local_device *device)
{
	mutex_lock(&g_device_list_data_lock);
	if (g_device_list == NULL) {
		mutex_unlock(&g_device_list_data_lock);
		return 0;
	} else {
		device_list *e = g_device_list;
		for (; e; e = e->next) {
			//�ҵ����豸
			if (NULL != e->device && 0 == strcmp(e->device->guid, gateway_guid)) {
				//���������豸�ҵ������豸���¹��б���
				if (ipp_device_list_append(&e->agent_device_list, device) > 0) {

					ipp_LogI_Prefix("###############################\r\n");
					ipp_LogI_Prefix("Add agent:\r\n");
					print_device_info(device);
					ipp_LogI_Prefix("###############################\r\n");

					mutex_unlock(&g_device_list_data_lock);
					return 1;
				}
				
				break;
			}
		}
	}
	mutex_unlock(&g_device_list_data_lock);
	return 0;
}

//�ж��豸�Ƿ����
IPP_BOOL contains_device(int fd, ipp_local_device *element) {
	if (NULL == element)
		return FALSE;

	if (NULL != g_device_list) {
		device_list *ele = NULL;
		mutex_lock(&g_device_list_data_lock);
		ele = g_device_list;
		for (; ele; ele = ele->next) {
			//�豸�Ѿ�����ϸ��Ϣ�������fd
			if (NULL != ele->device && 0 == strcmp(ele->device->guid, element->guid)) {
				ipp_LogV_Prefix("%s update!\r\n", ele->device->guid);
				free_local_device(&ele->device);
				ele->fd = fd;
				ele->device = get_device_clone_from_other(element);
				mutex_unlock(&g_device_list_data_lock);
				return TRUE;
			}
		}
		mutex_unlock(&g_device_list_data_lock);
		return FALSE;
	}

	return FALSE;
}

//�����豸��Ϣ���������豸״̬���»��߶�η���
IPP_BOOL update_device_info(int fd, ipp_local_device *element)
{
	if (NULL != g_device_list) {
		device_list *ele = NULL;
		if (TRUE == contains_device(fd, element)) {
			return FALSE;
		}

		mutex_lock(&g_device_list_data_lock);
		ele = g_device_list;
		for (; ele; ele = ele->next) {
			//�Ѿ����ڵ�ǰ�׽��ֽڵ㣬������豸��Ϣ�������ߴ��豸
			if (ele->fd == fd) {
				free_local_device(&ele->device);
				ele->device = get_device_clone_from_other(element);
				ipp_LogI_Prefix("###############################\r\n");
				ipp_LogI_Prefix("Add device:\r\n");
				print_device_info(element);
				ipp_LogI_Prefix("###############################\r\n");
				mutex_unlock(&g_device_list_data_lock);
				return TRUE;
			}
		}
		mutex_unlock(&g_device_list_data_lock);
	}

	return FALSE;
}

//�ж�ָ�����׽��ֽڵ��Ƿ���ϸ��ϢΪ��
IPP_BOOL is_device_list_empty(int fd)
{
	if (NULL != g_device_list) {
		device_list *ele = NULL;
		mutex_lock(&g_device_list_data_lock);
		ele = g_device_list;
		for (; ele; ele = ele->next) {
			if (ele->fd == fd && NULL != ele->device && 0 != strcmp("",ele->device->guid)) {
				mutex_unlock(&g_device_list_data_lock);
				return FALSE;
			}
		}
		mutex_unlock(&g_device_list_data_lock);
	}

	return TRUE;
}

//ɾ���׽��ֽڵ��£��豸��ϸ��ϢΪ�յĽڵ�
IPP_BOOL device_list_remove_empty(int fd)
{
	if (NULL != g_device_list) {
		device_list *le = NULL, *pe = NULL;
		mutex_lock(&g_device_list_data_lock);
		le = g_device_list, pe = NULL;
		while (le) {
			device_list *next = le->next;
			if (fd == le->fd) {
				if ((NULL == le->device || 0 == strcmp("", le->device->guid)) &&
					(TRUE == check_time_out(get_process_msec(), le->added_time, MAX_INFO_WAIT_TIME * 1000))) 
				{
					if (pe == NULL) {
						g_device_list = next;
					} else {
						pe->next = next;
					}

					ipp_LogV_Prefix("clear sock:%d\r\n", le->fd);
					print_device_info(le->device);
					free_local_device(&le->device);
					free_local_device_list(&le->agent_device_list);
					FREE_POINTER(le);

					mutex_unlock(&g_device_list_data_lock);
					return TRUE;
				}
			}

			pe = le;
			le = next;
		}
		mutex_unlock(&g_device_list_data_lock);
	}

	return FALSE;
}

//ɾ�����нڵ��У��׽���Ϊfd�Ľڵ㣬��������һ���豸
//����removed_deviceָ���ֵĵ�һ���豸��ϸ��Ϣ(������������������ϣ����Ե�һ������Ψһһ��)
device_list *device_list_remove(int fd, ipp_local_device **removed_device) 
{
	*removed_device = NULL;

	if (NULL != g_device_list) {
		device_list *le = NULL, *pe = NULL, *re = NULL;
		mutex_lock(&g_device_list_data_lock);
		le = g_device_list, pe = NULL;
		while (le) {
			device_list *next = le->next;
			if (le->fd == fd) {
				ipp_device_list *next_agent = NULL;

				if (NULL == pe) {
					g_device_list = next;
				} else {
					pe->next = next;
				}

				if (NULL != le->device && NULL == *removed_device) {
					ipp_LogI_Prefix("###############################\r\n");
					ipp_LogI_Prefix("Remove device:\r\n");
					*removed_device = get_device_clone_from_other(le->device);
					print_device_info(le->device);
					ipp_LogI_Prefix("###############################\r\n");
				}
				
				free_local_device(&le->device);

				for (; le->agent_device_list; le->agent_device_list = next_agent) {
					next_agent = le->agent_device_list->next;

					if (NULL != le->agent_device_list->device) {
						ipp_LogI_Prefix("###############################\r\n");
						ipp_LogI_Prefix("Remove agent:\r\n");
						print_device_info(le->agent_device_list->device);
						free_local_device(&le->agent_device_list->device);
						ipp_LogI_Prefix("###############################\r\n");
					}
					FREE_POINTER(le->agent_device_list);
				}
				FREE_POINTER(le);

				re = next;
				//mutex_unlock(&g_device_list_data_lock);
				//return next;
			} else {
				pe = le;
			}

			le = next;
		}
		mutex_unlock(&g_device_list_data_lock);
		return re;
	}

	return NULL;
}

//���������豸��ָ��sn�����豸�ĵ������豸�б����Ƴ�
int device_list_remove_agent(char *gateway_guid, ipp_local_device *device)
{
	mutex_lock(&g_device_list_data_lock);
	if (g_device_list == NULL) {
		mutex_unlock(&g_device_list_data_lock);
		return 0;
	} else {
		device_list *e = g_device_list;
		for (; e; e = e->next) {
			if (NULL != e->device) {
				if (0 == strcmp(e->device->guid, gateway_guid)) {
					if (NULL != ipp_device_list_find(&e->agent_device_list, device->guid)) {
						ipp_LogI_Prefix("###############################\r\n");
						ipp_LogI_Prefix("Remove agent:\r\n");
						print_device_info(device);
						ipp_LogI_Prefix("###############################\r\n");

						ipp_device_list_remove(&e->agent_device_list, device->guid);
					}

					mutex_unlock(&g_device_list_data_lock);
					return 1;
				}
			}
		}
	}
	mutex_unlock(&g_device_list_data_lock);
	return 0;
}

//�����豸�б�
void device_list_destroy() 
{
	device_list *next;

	mutex_lock(&g_device_list_data_lock);
	for (; g_device_list; g_device_list = next) {
		next = g_device_list->next;
		free_local_device(&g_device_list->device);
		free_local_device_list(&g_device_list->agent_device_list);
		FREE_POINTER(g_device_list);
	}
	mutex_unlock(&g_device_list_data_lock);
	mutex_free(&g_device_list_data_lock);
}

//����SN�����豸��������ָ�룬recursion����ָ��֪����Ҫ���豸�¹ҵĵ������б��м������ҡ�
device_list *device_list_find(char *guid, IPP_BOOL recursion) 
{
	if (NULL != g_device_list) {
		device_list *ele = NULL;
		mutex_lock(&g_device_list_data_lock);
		ele = g_device_list;
		for (; ele; ele = ele->next) {
			ipp_device_list *agent_list = ele->agent_device_list;

			if (NULL != ele->device) {
				if (0 == strcmp(ele->device->guid, guid)) {
					mutex_unlock(&g_device_list_data_lock);
					return ele;
				}
			}

			//�����ӵ������б��в���
			if (TRUE == recursion) {
				for (; agent_list; agent_list = agent_list->next) {
					if (NULL != agent_list->device) {
						if (0 == strcmp(agent_list->device->guid, guid)) {
							mutex_unlock(&g_device_list_data_lock);
							return ele;
						}
					}
				}
			}
		}
		mutex_unlock(&g_device_list_data_lock);
		return NULL;
	}

	return NULL;
}

//�����׽���fd�����豸
device_list *device_list_find_fd(int fd)
{
	if (NULL != g_device_list) {
		device_list *ele = NULL;
		mutex_lock(&g_device_list_data_lock);
		ele = g_device_list;
		for (; ele; ele = ele->next) {
			if (ele->fd == fd) {
				mutex_unlock(&g_device_list_data_lock);
				return ele;
			}
		}
		mutex_unlock(&g_device_list_data_lock);
		return NULL;
	}

	return NULL;
}

//����SN�����豸���׽���fd
int device_fd_find(char *guid)
{
	if (NULL != g_device_list) {
		device_list *l;
		int fd;
		mutex_lock(&g_device_list_data_lock);
		l = device_list_find(guid, TRUE);
		if (NULL == l) {
			mutex_unlock(&g_device_list_data_lock);
			return -1;
		}

		fd = l->fd;
		mutex_unlock(&g_device_list_data_lock);
		return fd;
	}

	return -1;
}

//����SN��ȡ�豸�ĸ�����ʹ���߿��޸�
ipp_local_device *get_device_clone(char *guid)
{
	if (NULL != g_device_list) {
		ipp_local_device *p = NULL;
		device_list *ele = NULL;
		mutex_lock(&g_device_list_data_lock);

		ele = g_device_list;
		for (; ele; ele = ele->next) {
			ipp_device_list *agent_list = ele->agent_device_list;

			if (NULL != ele->device) {
				if (0 == strcmp(ele->device->guid, guid)) {
					p = get_device_clone_from_other(ele->device);
					mutex_unlock(&g_device_list_data_lock);
					return p;
				}
			}

			for (; agent_list; agent_list = agent_list->next) {
				if (NULL != agent_list->device) {
					if (0 == strcmp(agent_list->device->guid, guid)) {
						p = get_device_clone_from_other(agent_list->device);
						mutex_unlock(&g_device_list_data_lock);
						return p;
					}
				}
			}
		}

		mutex_unlock(&g_device_list_data_lock);
		return p;
	}

	return NULL;
}

//��ȡ�豸�б��ĸ�����ʹ���߿��޸�
ipp_device_list *get_device_list_clone() {
	if (NULL != g_device_list) {
		ipp_device_list *dlist = NULL;
		device_list *ele = NULL;
		mutex_lock(&g_device_list_data_lock);
		ele = g_device_list;
		for (; ele; ele = ele->next) {
			ipp_device_list *agent_list = ele->agent_device_list;

			if (NULL != ele->device) {
				ipp_device_list_append(&dlist, ele->device);
			}

			for (; agent_list; agent_list = agent_list->next) {
				if (NULL != agent_list->device) {
					ipp_device_list_append(&dlist, agent_list->device);
				}
			}
		}
		mutex_unlock(&g_device_list_data_lock);

		return dlist;
	}

	return NULL;
}

//��ӡ�豸�б�
void print_device_list()
{
	if (NULL != g_device_list) {
		device_list *ele = NULL;
		int i = 1;

		ipp_LogV("Device list:\r\n");
		mutex_lock(&g_device_list_data_lock);
		ele = g_device_list;
		for (; ele; ele = ele->next, ++i) {
			ipp_device_list *agent_list = ele->agent_device_list;
			int j = 1;

			if (NULL != ele->device) {
				ipp_LogV("  %d:socket = %d, SN = %s\r\n", i, ele->fd, ele->device->guid);
			} else {
				ipp_LogV("  %d:socket = %d\r\n", i, ele->fd);
			}

			for (; agent_list; agent_list = agent_list->next, ++j) {
				if (NULL != agent_list->device) {
					ipp_LogV("  %d:SN = %s\r\n", j, agent_list->device->guid);
				}
			}
		}
		mutex_unlock(&g_device_list_data_lock);
	}
}

//�豸�б���������
void device_manager_lock()
{
	mutex_lock(&g_device_list_data_lock);
}

//�豸�б���������
void device_manager_unlock()
{
	mutex_unlock(&g_device_list_data_lock);
}

#endif

void print_device_info(ipp_local_device *device)
{
	if (NULL != device) {
		ipp_LogV("  %s\r\n", device->guid);
		ipp_LogV("  %s\r\n", device->product_id);
		ipp_LogV("  %s\r\n", device->software_ver);
		ipp_LogV("  %d\r\n", device->device_type);
		ipp_LogV("  %s\r\n", device->mac);
		ipp_LogV("  %d\r\n", device->connet_type);
		ipp_LogV("  %s\r\n", device->name);
		ipp_LogV("  %d\r\n", device->is_master);
		ipp_LogV("  %s\r\n", device->gateway_guid);
		ipp_LogV("  %d\r\n", device->reserved_len);
	}
}
