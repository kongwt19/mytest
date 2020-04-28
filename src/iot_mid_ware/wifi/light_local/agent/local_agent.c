#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
#include "local_agent.h"
#include "local_tcp.h"
#include "local_tcp_server.h"
#include "agent_device_manager.h"
#include "ipp_protocol.h"

//第三方设备上线
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
	
	//将第三方设备加入到自身的第三方设备列表
	if (agent_device_list_append(device) <= 0) {
		free_protocol(&p);
		return;
	}

	//通知所有client，有设备上线
	server_send_broadcast(command_agent_online, p->buffer, p->buffer_content_size);
	free_protocol(&p);
}

//第三方设备下线
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

	//从自身第三方设备列表中移除
	agent_device_list_remove(device->guid);

	//通知client有第三方设备下线
	server_send_broadcast(command_agent_offline, p->buffer, p->buffer_content_size);
	free_protocol(&p);
}

//第三方设备状态上报
void agent_update(char* guid, int data_len, char *agent_data)
{
	if (NULL == guid)
		return;

	//通知client设备数据上报
	server_send_event(guid, agent_data, data_len);
}
#endif
