#include "local_module_enable.h"
#if IPP_LIGHT_LOCAL_SERVER_ENABLE
#include "local_tcp_server.h"
#include "local_socket.h"
#include "local_error_code.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//#include "ipp_util.h"
#include "local_tcp.h"
#include "ipp_protocol.h"
#include "local_common.h"
#include "local_handler_manager.h"
#include "agent_device_manager.h"
#include "device_manager.h"

typedef struct server_sock_info {
	mutex_t data_lock;		//������
	peer_info *client_list;	//client��Ϣ�б�
	int fd;					//�׽���
}server_sock_info;

static server_sock_info *g_server_sock_info = NULL;
static const uint8_t LISTEN_CNT = 12;
static const uint8_t MAX_HEART_BEAT_TIME = 30;
static const uint16_t GET_AGENT_MAX_LEN = 512;
static ipp_protocol *g_server_recv_protocol = NULL;
uint16_t g_service_port = 9797;

//��ȡserverģ������fd
int get_server_max_fd()
{
	if (g_server_sock_info) {
		peer_info *pclient = g_server_sock_info->client_list;
		int max_fd = g_server_sock_info->fd;

		for (; pclient; pclient = pclient->next) {
			if (pclient->fd > max_fd)
				max_fd = pclient->fd;
		}

		return max_fd;
	}
	
	return 0;
}

//serverģ���fd set
void server_fd_set(fd_set *read_set)
{
	if (g_server_sock_info && g_server_sock_info->fd >= 0)
		FD_SET(g_server_sock_info->fd, read_set);

	if (g_server_sock_info) {
		peer_info *pclient_list = g_server_sock_info->client_list;
		for (; pclient_list; pclient_list = pclient_list->next) {
			FD_SET(pclient_list->fd, read_set);
		}
	}
}

//��ӡ����server������client��Ϣ
void print_server_sock_list()
{
	ipp_LogV("########### Server Info #########\r\n");
	if (g_server_sock_info && g_server_sock_info->fd >= 0)
		ipp_LogV("  self fd:%d\r\n", g_server_sock_info->fd);

	if (g_server_sock_info) {
		peer_info *pclient_list = NULL;
		mutex_lock(&g_server_sock_info->data_lock);
		pclient_list = g_server_sock_info->client_list;
		for (; pclient_list; pclient_list = pclient_list->next) {
			ipp_LogV("  fd:%d\r\n", pclient_list->fd);
		}
		mutex_unlock(&g_server_sock_info->data_lock);
	}
}

//�ظ�client�ĵ������豸�б���Ϣ��ȡ����
void get_agent_info_response(int fd)
{
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
	ipp_device_list *dlist = NULL;
	ipp_device_list *ele = NULL;

	int agent_once_cnt = 0;
	ipp_protocol *pTemp = create_protocol_size(GET_AGENT_MAX_LEN * 2);
	MALLOC_ERROR_RETURN(pTemp);

	agent_device_manager_lock();
	dlist = get_agent_device_list();
	for (ele = dlist; ele; ele = ele->next) {
		++agent_once_cnt;
		if (protocol_success != fill_to_protocol(pTemp, ele->device)) {
			agent_device_manager_unlock();
			free_protocol(&pTemp);
			return;
		}

		if (pTemp->buffer_content_size >= GET_AGENT_MAX_LEN || NULL == ele->next) {
			ipp_protocol *p = create_tcp_protocol(0);
			if (NULL == p) {
				ipp_LogE_Prefix("malloc:%s(%d)\r\n",__FUNCTION__,__LINE__);
				agent_device_manager_unlock();
				free_protocol(&pTemp);
				return;
			}

			if (protocol_success != put_int32(p, command_get_agent_info) || //���������
				protocol_success != put_string(p, (NULL == g_self->guid ? "" : g_self->guid)) || //gateway sn
				protocol_success != put_int32(p, agent_once_cnt)){    //���agent�б�����
				agent_device_manager_unlock();
				free_protocol(&p);
				free_protocol(&pTemp);
				return;
			}
			
			set_tcp_protocol_size(p, (int)(sizeof(int32_t) + //������
				sizeof(int32_t) + (NULL == g_self->guid ? 0 : strlen(g_self->guid)) + //gateway sn
				sizeof(int32_t)	+ //agent�б�����
				get_protocol_content_size(pTemp))
				);
			send_buffer(fd, p->buffer, get_protocol_content_size(p));
			free_protocol(&p);

			send_buffer(fd, pTemp->buffer, get_protocol_content_size(pTemp));
			pTemp->buffer_index = 0;
			pTemp->buffer_content_size = 0;

			agent_once_cnt = 0;
		}
	}

	agent_device_manager_unlock();
	free_protocol(&pTemp);
#endif
}

//�ظ�client���豸��Ϣ��ȡ����
void get_deviceinfo_response(int fd, uint16_t client_udp_port)
{
	ipp_protocol *p = NULL;

	//���¶Զ˵�UDP�˿ڣ�����udp send eventʱ�Ķ˿ں�
	peer_info *pclient_list = g_server_sock_info->client_list;
	mutex_lock(&g_server_sock_info->data_lock);
	for (; pclient_list; pclient_list = pclient_list->next) {
		if (pclient_list->fd == fd) {
			pclient_list->udp_peer_port = client_udp_port;
			break;
		}
	}
	mutex_unlock(&g_server_sock_info->data_lock);

	p = create_tcp_protocol(sizeof(int32_t) + 
		get_device_info_protocol_len(g_self)
		);
	MALLOC_ERROR_RETURN(p);

	if (protocol_success != put_int32(p, command_get_device_info) || //���������
		protocol_success != fill_to_protocol(p, g_self)) { 			 //���������Ϣ
		free_protocol(&p);
		return;
	}

	send_buffer(fd, p->buffer, get_protocol_content_size(p));
	free_protocol(&p);

	//���͵������豸�б�
	get_agent_info_response(fd);
}

//��ָ�����׽��ֽ�����Ϣ�ظ�
IPP_BOOL response_local_device_fd(int fd, char* command, int command_len)
{
	if (fd >= 0) {
		ipp_protocol *p = create_tcp_protocol(sizeof(int32_t) + (NULL == command ? 0 : command_len));
		MALLOC_ERROR_RETURN_WTTH_RET(p, FALSE);

		if (protocol_success != put_int32(p, command_control_device) ||
			protocol_success != put_raw(p, command, command_len)) {
			free_protocol(&p);
			return FALSE;
		}

		send_buffer(fd, p->buffer, get_protocol_content_size(p));
		free_protocol(&p);

		return TRUE;
	} else {
		print_local_format_error(response_local_device_fd_not_found, "%d\r\n", fd);
		return FALSE;
	}
}

//�����յ���������Ϣ
void deal_request(int fd, ipp_protocol *protocol, uint32_t sync_id, char *guid)
{
	ipp_protocol *p = NULL;
	ipp_protocol *reply_protocol = NULL;

	char *temp_reply_protocol_buffer = NULL;
	int32_t temp_reply_protocol_buffer_len = 0;

	//guid����g_self��guid����Ϊ�����豸
	if (0 == strcmp(guid, g_self->guid)) {
		reply_protocol = call_server_message_handler(guid, protocol);
	} else {
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
		ipp_device_list *agent = NULL;
		IPP_BOOL device_exist = FALSE;
		agent_device_manager_lock();
		agent = agent_device_list_find(guid);
		device_exist = 
			(NULL != agent && NULL != agent->device && 0 == strcmp(guid, agent->device->guid)) ? TRUE : FALSE;
		agent_device_manager_unlock();

		if (TRUE == device_exist)
			reply_protocol = call_server_agent_message_handler(guid, protocol);
#endif
	}

	temp_reply_protocol_buffer = (NULL == reply_protocol ? NULL : reply_protocol->buffer);
	temp_reply_protocol_buffer_len = (NULL == reply_protocol ? 0 : reply_protocol->buffer_content_size);

	p = create_protocol_size((int32_t)(sizeof(sync_id) + 
		sizeof(int32_t) + (NULL == guid ? 0 : strlen(guid)) + 
		temp_reply_protocol_buffer_len));
	if(NULL == p) {
		print_local_error(deal_request_create_protocol_failed);
		free_protocol(&reply_protocol);
		return;
	}

	if (protocol_success != put_int32(p, sync_id) || 
		protocol_success != put_string(p, (char *)(NULL == guid ? "" : guid)) ||
		protocol_success != put_raw(p, temp_reply_protocol_buffer, temp_reply_protocol_buffer_len)) 
	{
		free_protocol(&reply_protocol);
		free_protocol(&p);
		return;
	}
	free_protocol(&reply_protocol);

	response_local_device_fd(fd, p->buffer, p->buffer_content_size);
	free_protocol(&p);
}

//�����յ��Ŀͻ��˵�������Ϣ
void deal_client_heart_beat(peer_info *pclient)
{
	pclient->last_recv_heart_beat_time = get_process_msec();
	send_heart_beat(pclient->fd);
}

//����һ��������յ�������
void server_deal_one(void *par, char *buffer, int buffer_len)
{
	int32_t command;
	ipp_protocol *protocol;
	peer_info *pclient = (peer_info *)par;
	if (buffer_len < sizeof(int32_t))
		return;

	protocol = create_protocol_with_buffer(buffer, buffer_len);
	if (protocol_success != get_int32(protocol, &command)) {
		print_local_error(server_deal_one_get_command_failed);
		FREE_POINTER(protocol);
		return;
	}

	switch (command)
	{
	case command_get_device_info: {
			uint16_t port;
			int32_t ret = get_int16(protocol, (int16_t*)&port);
			if (protocol_success != ret) {
				break;
			}

			get_deviceinfo_response(pclient->fd, port);
		}
		break;
	case command_control_device: {
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

			deal_request(pclient->fd, protocol, sync_id, guid);
			FREE_POINTER(guid);
		}
		break;
	case command_heart_beat:
		deal_client_heart_beat(pclient);
		break;
	default:
		
		break;
	}

	FREE_POINTER(protocol);
}

//����˵ȴ�����
void deal_connect(fd_set *read_set)
{
	if (g_server_sock_info) {
		if (g_server_sock_info->fd >= 0 && FD_ISSET(g_server_sock_info->fd, read_set)) {
			//���µ���������
			struct sockaddr_in client_address;
			socklen_t address_len = sizeof(struct sockaddr_in);
			int client_sock_fd = (int)accept(g_server_sock_info->fd, (struct sockaddr *)&client_address, &address_len);
			if (client_sock_fd >= 0) {
				peer_info p_info;
				p_info.fd = client_sock_fd;
				p_info.peer_addr = client_address.sin_addr;
				p_info.peer_port = client_address.sin_port;

				ipp_LogD_Prefix("new fd = %d\r\n", client_sock_fd);

				if (set_recv_timeout(client_sock_fd, 5) < 0)
					print_local_format_warnning(deal_connect_SO_RCVTIMEO, "%d\r\n", get_sock_err());

				if (set_no_blocking(client_sock_fd) < 0)
					print_local_format_warnning(deal_connect_FIONBIO, "%d\r\n", get_sock_err());

				peer_info_list_append(&g_server_sock_info->client_list, &p_info);
			} else {
				print_local_format_error(deal_connect_accept_failed, "%d\r\n", get_sock_err());
			}
		}
	}
}

//�ͻ��˹���
void deal_command(fd_set *read_set)
{
	if (g_server_sock_info)
	{
		peer_info *pclient_list = NULL;

		for (pclient_list = g_server_sock_info->client_list; pclient_list; ) 
		{
			//����ĳ���ͻ��˹�������Ϣ
			char str_ip[ADDR_STR_LEN];
			IPP_BOOL need_remove = FALSE;
			memset(str_ip, 0, sizeof(str_ip));
			ipp_ntop(AF_INET, &pclient_list->peer_addr, str_ip, sizeof(str_ip));

			if (FD_ISSET(pclient_list->fd, read_set)) {
				char *buffer = NULL;
				int byte_num = recv_from_sock(pclient_list->fd, &buffer);
				if (byte_num > 0 && NULL != buffer) {
					ipp_LogV_Prefix("%s:recv %d from %s:%d\r\n", "Server", byte_num, str_ip, pclient_list->peer_port);
					deal_buffer(pclient_list, &g_server_recv_protocol, buffer, byte_num, server_deal_one);
				} else if(byte_num < 0) {
					need_remove = TRUE;
					print_local_format_error(server_process_recv_error, "%d\r\n", get_sock_err());
				} else {
					int sockerr = get_sock_err();
					if (EINTR != sockerr && EAGAIN != sockerr) {
						need_remove = TRUE;
						print_local_format_error(server_process_recv_len0, "%d\r\n", sockerr);
					}
				}
				FREE_POINTER(buffer);
			}

			if (FALSE == need_remove) {
				if (TRUE == check_time_out(get_process_msec(), pclient_list->last_recv_heart_beat_time, 
						MAX_HEART_BEAT_TIME * 1000)) {
					need_remove = TRUE;
					print_local_format_debug(server_process_heart_beat_timed_out, "%s:%d\r\n", str_ip, pclient_list->peer_port);
				}
			}
			
			if (TRUE == need_remove) {
				mutex_lock(&g_server_sock_info->data_lock);

				print_local_format_debug(server_process_disconnect, "%s:%d\r\n", str_ip, pclient_list->peer_port);
				local_close_sock(pclient_list->fd);
				FD_CLR(pclient_list->fd, read_set);
				pclient_list = peer_info_list_remove(&g_server_sock_info->client_list, pclient_list);

				mutex_unlock(&g_server_sock_info->data_lock);
			} else {
				pclient_list = pclient_list->next;
			}
		}
	}
}

//����˴�������
void server_process(fd_set *read_set)
{
	deal_connect(read_set);
	deal_command(read_set);
}

//����˳�ʼ��
int server_init()
{
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
	init_agent_device_list();
#endif
	if (NULL == g_server_sock_info) {
		g_server_sock_info = (server_sock_info *)malloc(sizeof(server_sock_info));
		MALLOC_ERROR_RETURN_WTTH_RET(g_server_sock_info, -1)

		g_server_sock_info->fd = create_tcp_server_sock(&g_service_port, LISTEN_CNT);
		g_server_sock_info->client_list = NULL;
		mutex_init(&g_server_sock_info->data_lock, NULL);
	}

	if (g_server_sock_info->fd < 0) {
		print_local_error(server_init_create_sock_failed);
		return -1;
	}

	return 1;
}

//�رշ����
void close_server()
{
	if (g_server_sock_info){
		if (g_server_sock_info->fd >= 0)
			local_close_sock(g_server_sock_info->fd);

		mutex_free(&g_server_sock_info->data_lock);
		peer_info_list_destroy(&g_server_sock_info->client_list);
		FREE_POINTER(g_server_sock_info);
	}

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
	agent_device_list_destroy();
#endif
}

IPP_BOOL send_to_all(char *buffer, int len)
{
	ipp_protocol *protocol = create_protocol();
	peer_info *pclient_list = g_server_sock_info->client_list;
	int fd_cnt = 0;
	int udp_fd = create_udp_server_sock();

	if (udp_fd < 0) {
		free_protocol(&protocol);
		print_local_error(send_to_all_create_udp_server_sock);
		return FALSE;
	}

	if (NULL == protocol) {
		local_close_sock(udp_fd);
		return FALSE;
	}

	mutex_lock(&g_server_sock_info->data_lock);
	for (; pclient_list; pclient_list = pclient_list->next) {
		if (pclient_list->udp_peer_port > 0) {
			put_raw(protocol, &pclient_list->peer_addr, sizeof(pclient_list->peer_addr));
			put_int16(protocol, pclient_list->udp_peer_port);
			++fd_cnt;
		}
	}
	mutex_unlock(&g_server_sock_info->data_lock);
	reset_protocol_index(protocol);

	for (;fd_cnt > 0; --fd_cnt) {
		uint16_t port = 0;
		struct sockaddr_in toaddr;
		memset(&toaddr, 0, sizeof(struct sockaddr_in));

		get_raw(protocol, (void *)&toaddr.sin_addr, sizeof(toaddr.sin_addr));
		get_int16(protocol, (int16_t *)&port);

		toaddr.sin_family = AF_INET;
		toaddr.sin_port = htons(port);

		sendto(udp_fd, buffer, len, 0, (struct sockaddr *) &toaddr, sizeof(struct sockaddr_in));
	}

	local_close_sock(udp_fd);
	free_protocol(&protocol);
	return TRUE;
}

//����˷����¼�
IPP_BOOL server_send_event(char *guid, char *eve, int eve_len)
{
	if (g_server_sock_info) {
		IPP_BOOL ret = FALSE;
		ipp_protocol *p = create_protocol_size((int)(sizeof(int32_t) + //������
			sizeof(int32_t) + (NULL == guid ? 0 : strlen(guid)) + //sn
			(NULL == eve ? 0 : eve_len)));

		if (NULL == p) {
			return FALSE;
		}

		if (protocol_success != put_int32(p, command_event) ||
			protocol_success != put_string(p, (char *)(NULL == guid ? "" : guid)) ||
			protocol_success != put_raw(p, eve, eve_len)) 
		{
			free_protocol(&p);
			return FALSE;
		}

		ret = send_to_all(p->buffer, get_protocol_content_size(p));

		free_protocol(&p);
		return ret;
	}

	return FALSE;
}

//����˷��͹㲥��Ϣ
void server_send_broadcast(int32_t command_id, char *data, int data_len)
{
	if (g_server_sock_info) {
		ipp_protocol *p = create_tcp_protocol((int)(sizeof(int32_t) + //������
			(NULL == data ? 0 : data_len)));

		if (NULL == p) {
			return;
		}

		if (protocol_success != put_int32(p, command_id) ||
			protocol_success != put_raw(p, data, data_len))
		{
			free_protocol(&p);
			return;
		}

		send_to_all(p->buffer, get_protocol_content_size(p));
		free_protocol(&p);
	}
}

#endif
