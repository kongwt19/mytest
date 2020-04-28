#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
#include "local_tcp.h"
#include "local_tcp_client.h"
#include "local_tcp_server.h"
#include "local_socket.h"
#include "local_error_code.h"
#include "device_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//#include "ipp_util.h"
#include "local_common.h"

//tcp(server+client)ģ���ʼ��
int tcpd_init(ipp_device_type init_type) {
#if IPP_LIGHT_LOCAL_SERVER_ENABLE
	if (init_type & ipp_device_server) {
		if (server_init() < 0) {
			close_server();
			return -1;
		}
	}
#endif

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	if (init_type & ipp_device_client) {
		if (client_init() < 0) {
#if IPP_LIGHT_LOCAL_SERVER_ENABLE
			close_server();
#endif
			close_client();
			return -1;
		}
	}
#endif

	return 1;
}

//��ʼ��tcp��Ϣ����
ipp_protocol *create_tcp_protocol(int size)
{
	ipp_protocol *p = create_protocol_size(size + sizeof(int32_t) * 2);
	MALLOC_ERROR_RETURN_WTTH_RET(p, NULL);

	if (protocol_success != put_int32(p, TCP_HEAD) ||
		protocol_success != put_int32(p, size)) {
		return NULL;
	}

	return p;
}

//����tcp��Ϣ�����С
void set_tcp_protocol_size(ipp_protocol *p, int size)
{
	int32_t indexTemp = p->buffer_index;
	int32_t content_sizeTemp = p->buffer_content_size;
	p->buffer_index = sizeof(int32_t);
	put_int32(p, size);

	p->buffer_index = indexTemp;
	p->buffer_content_size = content_sizeTemp;
}

//У��buffer�Ƿ���tcp��ʼͷ
IPP_BOOL check_buffer_head(char *buffer, int buffer_len)
{
	int32_t head;
	ipp_protocol *protocol;
	if (buffer_len < sizeof(TCP_HEAD))
		return FALSE;

	protocol = create_protocol_with_buffer(buffer, buffer_len);
	if (protocol_success != get_int32(protocol, &head)) {
		print_local_error(check_buffer_head_get_head_failed);
		return FALSE;
	}

	FREE_POINTER(protocol);
	return (TCP_HEAD == head);
}

//��ȡһ��tcp��Ϣ�ĳ�����Ϣ
int32_t get_info_len(char *buffer, int buffer_len)
{
	int info_len;
	ipp_protocol *protocol;
	if (buffer_len < sizeof(int32_t))
		return -1;

	protocol = create_protocol_with_buffer(buffer, buffer_len);
	if (protocol_success != get_int32(protocol, &info_len)) {
		print_local_error(check_buffer_head_get_info_len_failed);
		return -1;
	}

	FREE_POINTER(protocol);
	return info_len;
}

//�����յ�����Ϣ����
void deal_buffer(void *par, ipp_protocol **global_protocol, char *buffer, size_t buffer_len,
				 void (*deal_one)(void *par, char *buffer, int buffer_len))
{
	size_t i = 0;

	//ȫ�ֻ�������ͷ��ʶ��ʼ������һ�������쳣
	if (NULL != *global_protocol) {
		if (FALSE == check_buffer_head((*global_protocol)->buffer, 
			get_protocol_content_size(*global_protocol))) {
				free_protocol(global_protocol);
		}
	}

	while (i < buffer_len)
	{
		int info_len = -1;
		//������ʼ,�ӵ�ȫ�ֻ�����
		//���ȫ�ֻ���Ϊ�գ�����Ȼ�������ֽ�
		if (FALSE == check_buffer_head(buffer + i, (int)(buffer_len - i))) {
			if (NULL != *global_protocol) {
				int last_info_len = get_info_len((*global_protocol)->buffer + sizeof(TCP_HEAD), 
					get_protocol_content_size(*global_protocol) - sizeof(TCP_HEAD));
				put_char(*global_protocol, *(buffer + i));
				if (last_info_len == get_protocol_content_size(*global_protocol) - sizeof(TCP_HEAD) - sizeof(int32_t)) {
					//����һ�����������ݰ�
					deal_one(par, (*global_protocol)->buffer + sizeof(TCP_HEAD) + sizeof(int32_t),
						get_protocol_content_size(*global_protocol) - sizeof(TCP_HEAD) - sizeof(int32_t));

					//�����꣬�ͷ�ȫ�ֻ���
					free_protocol(global_protocol);
				}
			}
			++i;
			continue;
		}

		//�µ���ʼλ�ã�����֮ǰ��ȫ�ֻ���
		free_protocol(global_protocol);

		//����ʼ������ʣ�������Ѿ������������ȫ�ֻ���
		if (i + sizeof(TCP_HEAD) + sizeof(int32_t) > buffer_len) {
			if (NULL == *global_protocol) {
				*global_protocol = create_protocol_size((int32_t)(buffer_len - i));
				MALLOC_ERROR_RETURN(*global_protocol);
			}

			put_raw(*global_protocol, buffer + i, (int32_t)(buffer_len - i));
			return;
		}

		//����ʼ��bufferʣ��ռ��㹻�����ǻ�ȡ��Ϣ���ȳ����쳣��ֱ�ӷ��أ�������������
		info_len = get_info_len(buffer + i + sizeof(TCP_HEAD), (int)(buffer_len - i - sizeof(TCP_HEAD)));
		if (info_len <= 0)
			return;

		//���Ȼ�ȡ����������ʣ�����ݲ���һ���������������ȫ�ֻ���
		if (i + sizeof(TCP_HEAD) + sizeof(int32_t) + info_len > buffer_len) {
			if (NULL == *global_protocol) {
				*global_protocol = create_protocol_size((int32_t)(buffer_len - i));
				MALLOC_ERROR_RETURN(*global_protocol);
			}

			put_raw(*global_protocol, buffer + i, (int32_t)(buffer_len - i));
			return;
		}

		//�����������ܻ�ȡ�������ݰ�������д���
		deal_one(par, buffer + i + sizeof(TCP_HEAD) + sizeof(int32_t), info_len);
		i += sizeof(TCP_HEAD) + sizeof(int32_t) + info_len;
	}
}

//��������
void send_heart_beat(int fd)
{
	ipp_protocol *p = create_tcp_protocol(sizeof(int32_t));
	MALLOC_ERROR_RETURN(p);

	if (protocol_success == put_int32(p, command_heart_beat)) {
		send_buffer(fd, p->buffer, get_protocol_content_size(p));
	}
	
	free_protocol(&p);
}
#endif
