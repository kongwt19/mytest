#include "local_module_enable.h"
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
#include "local_tcp_client.h"
#include "local_socket.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ipp_defines.h"
//#include "ipp_util.h"
#include "ipp_protocol.h"
#include "local_tcp.h"
#include "ipp_local_device_info.h"
#include "device_manager.h"
#include "local_handler_manager.h"
#include "local_common.h"

typedef struct client_sock_info
{
	int fd;								//UDP����event�׽���
	peer_info *server_list;				//server��Ϣ
	mutex_t data_lock;					//������
}client_sock_info;

typedef struct client_response_node
{
	uint32_t sync_id;					//��Ϣid
	char **response;					//�ظ�������
	int *resp_len;						//�ظ����ݳ���
	response_handler handler;			//�ظ����ݵĴ����ص�
	void *param;						//�����ص������Ĳ���
	struct client_response_node *next;	//��һ��Ԫ��
}client_response_node;

typedef struct client_response_queue
{
	client_response_node *response_queue;	//�ظ����ݽڵ�
	mutex_t queue_lock;						//������
}client_response_queue;

static client_sock_info *g_client_sock_info = NULL;
static ipp_protocol *g_client_recv_protocol = NULL;
static client_response_queue *g_client_response_queue = NULL;
static const uint8_t HEART_BEAT_CYCLE = 15;
static const uint8_t MAX_HEART_BEAT_TIME = 30;
static const uint8_t MAX_RESPONSE_WAIT_TIME = 12;
static const uint16_t CLIENT_UDP_BUFFER_LEN = 1024;
uint16_t g_client_port = 9898;

//��ȡclient�������׽���
int get_client_max_fd()
{
	if (g_client_sock_info) {
		peer_info *pserver = g_client_sock_info->server_list;
		int max_fd = g_client_sock_info->fd;

		for (; pserver; pserver = pserver->next) {
			if (pserver->fd > max_fd)
				max_fd = pserver->fd;
		}

		return max_fd;
	}

	return 0;
}

//clientģ��ĵ�fd set
void client_fd_set(fd_set *read_set)
{
	if (g_client_sock_info && g_client_sock_info->fd >= 0)
		FD_SET(g_client_sock_info->fd, read_set);

	if (g_client_sock_info) {
		peer_info *pserver_list = g_client_sock_info->server_list;
		for (; pserver_list; pserver_list = pserver_list->next) {
			FD_SET(pserver_list->fd, read_set);
		}
	}
}

//��ӡclient���׽��ֺ������ӵ��׽����б�
void print_client_sock_list()
{
	ipp_LogV("########### Client Info #########\r\n");
	print_device_list();

	ipp_LogV("Client sock list:\r\n");
	if (g_client_sock_info && g_client_sock_info->fd >= 0)
		ipp_LogV("  self fd:%d\r\n", g_client_sock_info->fd);

	if (g_client_sock_info) {
		peer_info *pserver_list = NULL;
		mutex_lock(&g_client_sock_info->data_lock);
		pserver_list = g_client_sock_info->server_list;
		for (; pserver_list; pserver_list = pserver_list->next) {
			ipp_LogV("  fd:%d\r\n", pserver_list->fd);
		}
		mutex_unlock(&g_client_sock_info->data_lock);
	}
}

//����ظ���Ϣƥ���б����Ѿ���ʱ�Ľڵ�
void response_queue_clear(client_response_node **head)
{
	uint32_t cur_time = get_process_msec();
	client_response_node *le = *head, *pe = NULL;
	while (le) {
		client_response_node *next = le->next;
		if (TRUE == check_time_out(cur_time, le->sync_id, MAX_RESPONSE_WAIT_TIME * 1000)) {
			if (pe == NULL) {
				*head = next;
			} else {
				pe->next = next;
			}

			if (NULL != le->response)
				FREE_POINTER(*le->response);

			FREE_POINTER(le);
		} else {
			pe = le;
		}

		le = next;
	}
}

//�ڻظ���Ϣƥ���б�������һ���ڵ�
int response_queue_append(client_response_node **head, client_response_node *element) {
	client_response_node *node = (client_response_node *)malloc(sizeof(client_response_node));
	MALLOC_ERROR_RETURN_WTTH_RET(node, -1)

	if (NULL == element->response && NULL == element->handler)
		return 0;

	node->sync_id = element->sync_id;
	node->response = element->response;
	node->resp_len = element->resp_len;
	node->handler = element->handler;
	node->param = element->param;
	node->next = NULL;

	if (*head == NULL) {
		*head = node;
	} else {
		client_response_node *e = *head, *tail_e = NULL;
		for (; e; e = e->next) {
			if (node->sync_id == e->sync_id) {
				FREE_POINTER(node);
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
	return 1;
}

//�ӻظ���Ϣƥ���б���ɾ��ָ���ڵ�
client_response_node *response_queue_remove(client_response_node **head, uint32_t sync_id, IPP_BOOL free_response) {
	client_response_node *le = *head, *pe = NULL;
	for (; le; le = le->next) {
		if (le->sync_id == sync_id) {
			if (pe == NULL) {
				*head = le->next;
				if (TRUE == free_response && NULL != le->response)
					FREE_POINTER(*le->response);

				FREE_POINTER(le);
				return *head;
			} else {
				pe->next = le->next;
				if (TRUE == free_response && NULL != le->response)
					FREE_POINTER(*le->response);

				FREE_POINTER(le);
				return pe->next;
			}
		}
		pe = le;
	}
	return NULL;
}

//���ٻظ���Ϣƥ���б�
void response_queue_destroy(client_response_node **head) {
	client_response_node *next;

	for (; *head; *head = next) {
		next = (*head)->next;
		if (NULL != (*head)->response)
			FREE_POINTER(*((*head)->response));
		FREE_POINTER(*head);
	}
}

//�ӻظ���Ϣƥ���б��в���ָ���ڵ�
client_response_node *response_queue_find(client_response_node *server_list, uint32_t sync_id) {
	client_response_node *ele = server_list;
	for (; ele; ele = ele->next) {
		if (ele->sync_id == sync_id) {
			return ele;
		}
	}
	return NULL;
}

//�ӽ��յ��Ļ��������л�ȡ�豸��Ϣ
static void get_device_info(int fd, ipp_protocol *protocol)
{
	ipp_local_device *d = create_local_device();
	MALLOC_ERROR_RETURN(d);

	if (init_local_device_from_protocol(d, protocol) < 0) {
		print_local_error(get_device_info_init_local_device_failed);
		free_local_device(&d);
		return;
	}

	//�����豸�б�
	if (TRUE == update_device_info(fd, d)) {
		call_device_online_handler(d);
	}
		
	free_local_device(&d);
}

//�ӽ��յ��Ļ��������л�ȡ�������豸�б�
static void get_agent_info(int fd, char *gateway_guid, ipp_protocol *protocol)
{
	int agent_count = 0;
	int agent_idx = 0;
	//��ȡ�������豸�б�
	if (protocol_success == get_int32(protocol, &agent_count)) {
		for ( ; agent_idx < agent_count; ++agent_idx) {
			ipp_local_device *agent = create_local_device();
			if (NULL == agent) {
				ipp_LogE_Prefix("malloc:%s(%d)\r\n",__FUNCTION__,__LINE__);
				return;
			}

			if (init_local_device_from_protocol(agent, protocol) < 0) {
				print_local_error(get_device_info_init_agent_device_failed);
				free_local_device(&agent);
				return;
			}

			if (NULL == agent->gateway_guid || 0 == strcmp("", agent->gateway_guid)) {
				if (copy_string(&agent->gateway_guid, gateway_guid) < 0) {
					print_local_error(get_device_info_copy_string_failed);
					free_local_device(&agent);
					return;
				}
			}

			if (device_list_append_agent(gateway_guid, agent) > 0)
				call_device_online_handler(agent);

			free_local_device(&agent);
		}
	} else {
		print_local_error(get_device_info_get_agent_count_failed);
	}
}

//�������豸����
static void agent_device_online(char *gateway_guid, ipp_protocol *protocol)
{
	ipp_local_device *agent = create_local_device();
	MALLOC_ERROR_RETURN(agent);

	if (init_local_device_from_protocol(agent, protocol) < 0) {
		print_local_error(agent_device_online_init_device_failed);
		free_local_device(&agent);
		return;
	}

	if (NULL == agent->gateway_guid || 0 == strcmp("", agent->gateway_guid)) {
		if (copy_string(&agent->gateway_guid, gateway_guid) < 0) {
			print_local_error(agent_device_online_copy_string_failed);
			free_local_device(&agent);
			return;
		}
	}

	if (device_list_append_agent(gateway_guid, agent) > 0)
		call_device_online_handler(agent);

	free_local_device(&agent);
}

//�������豸����
static void agent_device_offline(char *gateway_guid, ipp_protocol *protocol)
{
	ipp_local_device *agent = create_local_device();
	MALLOC_ERROR_RETURN(agent);

	if (init_local_device_from_protocol(agent, protocol) < 0) {
		print_local_error(agent_device_offline_init_device_failed);
		free_local_device(&agent);
		return;
	}

	if (NULL == agent->gateway_guid || 0 == strcmp("", agent->gateway_guid)) {
		if (copy_string(&agent->gateway_guid, gateway_guid) < 0) {
			print_local_error(agent_device_offline_copy_string_failed);
			free_local_device(&agent);
			return;
		}
	}

	if (device_list_remove_agent(gateway_guid, agent) > 0)
		call_device_offline_handler(agent);

	free_local_device(&agent);
}

//���ؿ����豸
uint32_t control_local_device_common(int fd, char *sn, char* command, int command_len, 
	char **response, int *rep_len, response_handler handler, void *param)
{
	if (NULL != sn && NULL != g_client_response_queue) {
		uint32_t sync_id = get_process_msec();
		client_response_node node;
		ipp_protocol *p = create_tcp_protocol(sizeof(int32_t) + sizeof(int32_t) + 
			sizeof(int32_t) + (int)strlen(sn) +
			(NULL == command ? 0 : command_len)
			);
		MALLOC_ERROR_RETURN_WTTH_RET(p, FALSE);
		
		if (protocol_success != put_int32(p, command_control_device) ||
			protocol_success != put_int32(p, sync_id) ||
			protocol_success != put_string(p, sn) ||
			protocol_success != put_raw(p, command, command_len)) {
			free_protocol(&p);
			return 0;
		}

		node.sync_id = sync_id;
		node.response = response;
		node.resp_len = rep_len;
		node.handler = handler;
		node.param = param;

		//����֮ǰ����һ��ƥ��ظ���Ϣ���Ա��ڸ��յ������ݽ���ƥ�����ж��Ƿ��յ��˶�Ӧ�Ļظ�
		mutex_lock(&g_client_response_queue->queue_lock);
		response_queue_append(&g_client_response_queue->response_queue, &node);
		mutex_unlock(&g_client_response_queue->queue_lock);

		if (send_buffer(fd, p->buffer, get_protocol_content_size(p)) > 0) {
			free_protocol(&p);
			return sync_id;
		} else {
			free_protocol(&p);
			return 0;
		}
	}

	return 0;
}

//�����׽����������豸
local_error_code control_local_device_fd(int fd, char *sn, char* command, int command_len, char **response, int *rep_len)
{
	if (NULL == response || NULL == rep_len)
		return local_inner_error;

	*response = NULL;
	*rep_len = 0;
	if (fd >= 0 && NULL != g_client_response_queue) {
		uint32_t sync_id = control_local_device_common(fd, sn, command, command_len, response, rep_len, NULL, NULL);
		if (sync_id > 0) {
			uint32_t begintime = get_process_msec();
			while (NULL == *response && 
				FALSE == check_time_out(get_process_msec(), begintime, MAX_RESPONSE_WAIT_TIME * 1000)) {
				sleep_ms(100);
			}
		}

		mutex_lock(&g_client_response_queue->queue_lock);
		response_queue_remove(&g_client_response_queue->response_queue, sync_id, FALSE);
		mutex_unlock(&g_client_response_queue->queue_lock);

		if (NULL != *response) {
			return local_inner_sucess;
		} else {
			print_local_format_warnning(control_local_device_fd_timed_out, "%d\r\n", fd);
			return control_local_device_fd_timed_out;
		}
	} else {
		print_local_format_error(control_local_device_fd_not_found, "%d\r\n", fd);
		return control_local_device_fd_not_found;
	}
}

//�����׽����첽�����豸
local_error_code control_local_device_fd_async(int fd, char *sn, char* command, int command_len, response_handler handler, void *param)
{
	if (fd >= 0 && NULL != g_client_response_queue) {
		uint32_t sync_id = control_local_device_common(fd, sn, command, command_len, NULL, 0, handler, param);
		if (sync_id <= 0) {
			mutex_lock(&g_client_response_queue->queue_lock);
			response_queue_remove(&g_client_response_queue->response_queue, sync_id, TRUE);
			mutex_unlock(&g_client_response_queue->queue_lock);

			print_local_format_warnning(control_local_device_fd_async_failed, "%d\r\n", fd);
			return control_local_device_fd_async_failed;
		}

		return local_inner_sucess;
	} else {
		print_local_format_error(control_local_device_fd_async_not_found, "%d\r\n", fd);
		return control_local_device_fd_async_not_found;
	}
}

//�����ظ���Ϣ
void deal_response(ipp_protocol* protocol, uint32_t sync_id, char* guid)
{
	device_list *dev_list = NULL;
	IPP_BOOL device_exist = FALSE;

	device_manager_lock();
	dev_list = device_list_find(guid, TRUE);
	device_exist = 
		(NULL != dev_list && NULL != dev_list->device && 0 == strcmp(guid, dev_list->device->guid)) ? TRUE : FALSE;
	device_manager_unlock();

	if (TRUE == device_exist && NULL != g_client_response_queue) {
		client_response_node *node = NULL;
		mutex_lock(&g_client_response_queue->queue_lock);
		node = response_queue_find(g_client_response_queue->response_queue, sync_id);
		//�ҵ�ƥ�����Ϣ
		if (NULL != node) {
			//��������ص���Ϊ�գ�����ûص�
			if (NULL != node->handler) {
				node->handler(node->param, guid, 
					protocol->buffer_content_size - protocol->buffer_index, protocol->buffer + protocol->buffer_index 
					);
				response_queue_remove(&g_client_response_queue->response_queue, sync_id, TRUE);
			} else {
				//��������ص���Ϊ�գ��򿽱�����
				if (NULL != node->response)
					FREE_POINTER(*node->response);

				*node->resp_len = protocol->buffer_content_size - protocol->buffer_index;
				if (*node->resp_len > 0) {
					*node->response = (char *)malloc(*node->resp_len);
					memcpy(*node->response, protocol->buffer + protocol->buffer_index, *node->resp_len);
				}
			}
		}
		mutex_unlock(&g_client_response_queue->queue_lock);
	}
}

void deal_event(char *sn, ipp_protocol *p)
{
	call_event_handler(sn, p->buffer_content_size - p->buffer_index, p->buffer + p->buffer_index);
}

//����server������
void deal_server_heart_beat(peer_info *pserver)
{
	pserver->last_recv_heart_beat_time = get_process_msec();
}

//����һ���ͻ����յ�����Ϣ
void client_deal_one(void *par, char *buffer, int buffer_len)
{
	int32_t command;
	ipp_protocol *protocol;
	if (buffer_len < sizeof(int32_t))
		return;

	protocol = create_protocol_with_buffer(buffer, buffer_len);
	if (protocol_success != get_int32(protocol, &command)) {
		print_local_error(client_deal_one_get_command_failed);
		FREE_POINTER(protocol);
		return;
	}

	switch (command)
	{
	case command_get_device_info: {
			peer_info *pserver = (peer_info *)par;
			get_device_info(pserver->fd, protocol);
		}
		break;
	case command_get_agent_info:{
			peer_info *pserver = (peer_info *)par;
			char *guid = NULL;
			int32_t ret = get_string(protocol, &guid);
			if (protocol_success != ret) {
				FREE_POINTER(guid);
				break;
			}

			get_agent_info(pserver->fd, guid, protocol);
			FREE_POINTER(guid);
		}
		break;
	case command_control_device:{
			uint32_t sync_id;
			char *guid = NULL;
			int32_t ret = get_int32(protocol, (int32_t*)&sync_id);
			if (protocol_success != ret) {
				break;
			}

			ret = get_string(protocol, &guid);
			if (protocol_success != ret) {
				FREE_POINTER(guid);
				break;
			}

			deal_response(protocol, sync_id, guid);
			FREE_POINTER(guid);
		}
		break;
	case command_event: {
			char *guid = NULL;
			int32_t ret = get_string(protocol, &guid);
			if (protocol_success != ret) {
				FREE_POINTER(guid);
				break;
			}

			deal_event(guid, protocol);
			FREE_POINTER(guid);
		}
		break;
	case command_heart_beat:{
			peer_info *pserver = (peer_info *)par;
			deal_server_heart_beat(pserver);
		}
		break;
	case command_agent_online: {
			char *gateway_guid = NULL;
			if (protocol_success == get_string(protocol, &gateway_guid))
				agent_device_online(gateway_guid, protocol);

			FREE_POINTER(gateway_guid);
		}
		break;
	case command_agent_offline: {
			char *gateway_guid = NULL;
			if (protocol_success == get_string(protocol, &gateway_guid))
				agent_device_offline(gateway_guid, protocol);

			FREE_POINTER(gateway_guid);
		}
		break;
	default:
		
		break;
	}

	FREE_POINTER(protocol);
}

void deal_udp_event(fd_set *read_set)
{
	if (g_client_sock_info) {
		if (g_client_sock_info->fd >= 0 && FD_ISSET(g_client_sock_info->fd, read_set)) {
			char *buffer = (char *)malloc(CLIENT_UDP_BUFFER_LEN);
			struct sockaddr_in addr;
			socklen_t len = sizeof(struct sockaddr_in);
			int recv_len = recvfrom(g_client_sock_info->fd, buffer, CLIENT_UDP_BUFFER_LEN, 0, (struct sockaddr *)&addr, &len);
			if (recv_len > 0) {
				char str_ip[ADDR_STR_LEN];
				ipp_ntop(addr.sin_family, &addr.sin_addr, str_ip, sizeof(str_ip));
				ipp_LogV_Prefix("%s:recv %d from %s:%d\r\n", "Client UDP", recv_len, str_ip, addr.sin_port);
				client_deal_one(NULL, buffer, recv_len);
			}
			FREE_POINTER(buffer);
		}
	}
}

//���ڷ�������
void cycle_send_heart_beat(peer_info *sinfo)
{
	uint32_t cur_time = get_process_msec();
	if ((0 == sinfo->last_send_heart_beat_time) || 
		TRUE == check_time_out(cur_time, sinfo->last_send_heart_beat_time, HEART_BEAT_CYCLE * 1000))
	{
		sinfo->last_send_heart_beat_time = cur_time;
		send_heart_beat(sinfo->fd);
	}
}

//������Ϊ�ͻ��˵�����
void client_process(fd_set *read_set)
{
	deal_udp_event(read_set);

	if (NULL != g_client_response_queue) 
	{
		//�Ƴ���ʱ�Ŀ�������
		mutex_lock(&g_client_response_queue->queue_lock);
		response_queue_clear(&g_client_response_queue->response_queue);
		mutex_unlock(&g_client_response_queue->queue_lock);
	}

	if (NULL != g_client_sock_info) 
	{
		peer_info *pserver_list = NULL;
		for (pserver_list = g_client_sock_info->server_list; pserver_list; ) 
		{
			//����ĳ���ͻ��˹�������Ϣ
			char str_ip[ADDR_STR_LEN];
			IPP_BOOL need_remove = FALSE;
			memset(str_ip, 0, sizeof(str_ip));
			ipp_ntop(AF_INET, &pserver_list->peer_addr, str_ip, sizeof(str_ip));
			
			//ɾ����ʱ��Ϊ�յ��豸�б�
			if (TRUE == device_list_remove_empty(pserver_list->fd)) {
				need_remove = is_device_list_empty(pserver_list->fd);
			}

			if (FALSE == need_remove) {
				if (FD_ISSET(pserver_list->fd, read_set)) {
					char *buffer = NULL;
					int byte_num = recv_from_sock(pserver_list->fd, &buffer);
					if (byte_num > 0 && NULL != buffer) {
						ipp_LogV_Prefix("%s:recv %d from %s:%d\r\n", "Client", byte_num, str_ip, pserver_list->peer_port);
						deal_buffer(pserver_list, &g_client_recv_protocol, buffer, byte_num, client_deal_one);
					} else if(byte_num < 0) {
						need_remove = TRUE;
						print_local_format_error(client_process_recv_error, "%d\r\n", get_sock_err());
					} else {
						int sockerr = get_sock_err();
						if (EINTR != sockerr && EAGAIN != sockerr) {
							need_remove = TRUE;
							print_local_format_error(client_process_recv_len0, "%d\r\n", sockerr);
						}
					}
					FREE_POINTER(buffer);
				} 
			}

			if (FALSE == need_remove) {
				if (TRUE == check_time_out(get_process_msec(), pserver_list->last_recv_heart_beat_time, 
						MAX_HEART_BEAT_TIME * 1000)) {
					need_remove = TRUE;
					print_local_format_debug(client_process_heart_beat_timed_out, "%s:%d\r\n", str_ip, pserver_list->peer_port);
				}
			}

			if (TRUE == need_remove) {
				ipp_local_device *removed_device = NULL;
				mutex_lock(&g_client_sock_info->data_lock);

				print_local_format_debug(client_process_disconnect, "%s:%d\r\n", str_ip, pserver_list->peer_port);
				local_close_sock(pserver_list->fd);
				device_list_remove(pserver_list->fd, &removed_device);
				FD_CLR(pserver_list->fd, read_set);
				pserver_list = peer_info_list_remove(&g_client_sock_info->server_list, pserver_list);

				mutex_unlock(&g_client_sock_info->data_lock);

				if (NULL != removed_device) {				
					call_device_offline_handler(removed_device);
					free_local_device(&removed_device);
				}
			} else {
				cycle_send_heart_beat(pserver_list);
				pserver_list = pserver_list->next;
			}	
		}
	}
}

//clientģ���ʼ��
int client_init()
{
	init_device_list();

	if (NULL == g_client_sock_info) {
		g_client_sock_info = (client_sock_info *)malloc(sizeof(client_sock_info));
		MALLOC_ERROR_RETURN_WTTH_RET(g_client_sock_info, -1)

		g_client_sock_info->fd = create_udp_client_sock(&g_client_port);
		g_client_sock_info->server_list = NULL;
		mutex_init(&g_client_sock_info->data_lock, NULL);
	}

	if (NULL == g_client_response_queue) {
		g_client_response_queue = (client_response_queue *)malloc(sizeof(client_response_queue));
		MALLOC_ERROR_RETURN_WTTH_RET(g_client_response_queue, -1)

		g_client_response_queue->response_queue = NULL;
		mutex_init(&g_client_response_queue->queue_lock, NULL);
	}

	if (g_client_sock_info->fd < 0) {
		print_local_error(client_init_create_sock_failed);
		return -1;
	}

	return 1;
}

//��ȡ�豸��Ϣ����
void get_deviceinfo_request(int fd)
{
	device_list *device = NULL;
	device_manager_lock();
	device = device_list_find_fd(fd);
	if (NULL != device) {
		uint32_t curtime = get_process_msec();
		if (0 == device->last_request_time || 
				TRUE == check_time_out(curtime, device->last_request_time,1 * 1000)) {
			ipp_protocol *p = create_tcp_protocol(sizeof(int32_t) + sizeof(g_client_port));
			device->last_request_time = curtime;

			if (NULL == p) {
				ipp_LogE_Prefix("malloc:%s(%d)\r\n",__FUNCTION__,__LINE__);
				device_manager_unlock();
				return;
			}

			if (protocol_success == put_int32(p, command_get_device_info) &&
				protocol_success == put_int16(p, g_client_port))
				send_buffer(fd, p->buffer, get_protocol_content_size(p));

			free_protocol(&p);
		}
	}
	device_manager_unlock();
}

//client��ʼ����һ��server
int client_start(struct sockaddr_in *server_addr, char *target)
{
	mutex_lock(&g_client_sock_info->data_lock);
	//�豸�б���û�вŽ�����һ��
	if (NULL == device_list_find(target, FALSE)) {
		char str_ip[ADDR_STR_LEN];
		peer_info *s_info = peer_info_list_find(g_client_sock_info->server_list, 
			server_addr->sin_addr, server_addr->sin_port);
		if (NULL != s_info) {
			device_list *device = NULL;
			device_manager_lock();
			device = device_list_find_fd(s_info->fd);

			//�������ӣ������б���û���豸�ڵ㣬��ֱ��ѯ��һ��
			if (NULL == device) {
				device_list_append(s_info->fd);
				get_deviceinfo_request(s_info->fd);
			} else {
				//�������ӣ��б������豸�ڵ㣬�����豸��ϢΪ�գ���ֱ��ѯ��һ��
				if (NULL == device->device) {
					get_deviceinfo_request(s_info->fd);
				}
			}
			device_manager_unlock();

			mutex_unlock(&g_client_sock_info->data_lock);
			return 1;
		}

		s_info = (peer_info *)malloc(sizeof(peer_info));
		if (NULL == s_info) {
			print_local_error(client_start_create_server_info_failed);
			mutex_unlock(&g_client_sock_info->data_lock);
			return -1;
		}

		s_info->fd = create_tcp_client_sock();
		if (s_info->fd < 0) {
			print_local_error(client_start_create_sock_failed);
			FREE_POINTER(s_info);
			mutex_unlock(&g_client_sock_info->data_lock);
			return -1;
		}

		s_info->peer_addr = server_addr->sin_addr;
		s_info->peer_port = server_addr->sin_port;
		s_info->next = NULL;

		ipp_ntop(AF_INET, &server_addr->sin_addr, str_ip, sizeof(str_ip));
		if (connect(s_info->fd, (struct sockaddr *)server_addr, sizeof(struct sockaddr_in)) >= 0) {
			peer_info_list_append(&g_client_sock_info->server_list, s_info);
			ipp_LogD_Prefix("connect (%s:%d)\r\n", str_ip, ntohs(server_addr->sin_port));

			device_list_append(s_info->fd);
			get_deviceinfo_request(s_info->fd);
			FREE_POINTER(s_info);

			mutex_unlock(&g_client_sock_info->data_lock);
			return 1;
		} else {
			ipp_LogE_Prefix("connect (%s:%d) failed!err:%d\r\n", str_ip, ntohs(server_addr->sin_port), get_sock_err());
			local_close_sock(s_info->fd);
			FREE_POINTER(s_info);

			mutex_unlock(&g_client_sock_info->data_lock);
			return -1;
		}
	}

	mutex_unlock(&g_client_sock_info->data_lock);
	return 0;
}

//�ر�client����ģ��
void close_client()
{
	if (g_client_sock_info) {
		peer_info *pserver_list = NULL;

		if (g_client_sock_info->fd >= 0)
			local_close_sock(g_client_sock_info->fd);

		pserver_list = g_client_sock_info->server_list;
		for (; pserver_list; pserver_list = pserver_list->next) {
			if (pserver_list->fd >= 0)
				local_close_sock(pserver_list->fd);
		}
		peer_info_list_destroy(&g_client_sock_info->server_list);
		mutex_free(&g_client_sock_info->data_lock);
		FREE_POINTER(g_client_sock_info);
	}

	device_list_destroy();
}

#endif
