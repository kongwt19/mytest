#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
#include "local_agent.h"
#include "local_tcp.h"
#include "local_tcp_server.h"
#include "agent_device_manager.h"
#include "ipp_protocol.h"

//�������豸����
void agent_online(char *gateway_guid, ipp_local_device *device)
{
	ipp_protocol *p;
	if (NULL == gateway_guid)
		return;

	p = create_protocol_size((int)sizeof(int32_t) + (int)strlen(gateway_guid) + 
		get_device_info_protocol_len(device));
	MALLOC_ERROR_RETURN(p);

	if (protocol_success != put_string(p, gateway_guid) ||
		protocol_success != fill_to_protocol(p, device)) 
	{
		free_protocol(&p);
		return;
	}
	
	//���������豸���뵽����ĵ������豸�б�
	if (agent_device_list_append(device) <= 0) {
		free_protocol(&p);
		return;
	}

	//֪ͨ����client�����豸����
	server_send_broadcast(command_agent_online, p->buffer, p->buffer_content_size);
	free_protocol(&p);
}

//�������豸����
void agent_offline(char *gateway_guid, ipp_local_device *device)
{
	ipp_protocol *p;
	if (NULL == gateway_guid)
		return;

	p = create_protocol_size((int)sizeof(int32_t) + (int)strlen(gateway_guid) + 
		get_device_info_protocol_len(device));
	MALLOC_ERROR_RETURN(p);

	if (protocol_success != put_string(p, gateway_guid) ||
		protocol_success != fill_to_protocol(p, device)) 
	{
		free_protocol(&p);
		return;
	}

	//������������豸�б����Ƴ�
	agent_device_list_remove(device->guid);

	//֪ͨclient�е������豸����
	server_send_broadcast(command_agent_offline, p->buffer, p->buffer_content_size);
	free_protocol(&p);
}

//�������豸״̬�ϱ�
void agent_update(char* guid, int data_len, char *agent_data)
{
	if (NULL == guid)
		return;

	//֪ͨclient�豸�����ϱ�
	server_send_event(guid, agent_data, data_len);
}
#endif
