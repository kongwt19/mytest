#include "local_module_enable.h"
#include "local_handler_manager.h"
#include "ipp_defines.h"

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
typedef struct ipp_device_info_cb
{
	char *name;
	void *param;
	device_info_handler handler;
	struct ipp_device_info_cb *next;
}ipp_device_info_cb;

typedef struct ipp_device_event_cb
{
	char *name;
	void *param;
	event_handler handler;
	struct ipp_device_event_cb *next;
}ipp_device_event_cb;


static ipp_device_info_cb *g_device_info_online_cb = NULL;
static ipp_device_info_cb *g_device_info_offline_cb = NULL;
static ipp_device_event_cb *g_device_event_cb = NULL;
#endif

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
static request_handler g_server_message_handler = NULL;
static void *g_server_message_param = NULL;
#endif

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
static request_handler g_server_agent_message_handler = NULL;
static void *g_server_agent_message_param = NULL;
#endif

#define CB_APPEND(cb_type) \
cb_type *node = (cb_type *)malloc(sizeof(cb_type));\
MALLOC_ERROR_RETURN_WTTH_RET(node, -1)\
	\
if (NULL == element->name || NULL == element->handler)\
	return 0;\
	\
node->name = (char *)malloc(strlen(element->name) + 1);\
if (NULL == node->name) {\
	ipp_LogE_Prefix("malloc:%s(%d)\r\n",__FUNCTION__,__LINE__);\
	FREE_POINTER(node);\
	return -1;\
}\
strncpy(node->name, element->name, strlen(element->name) + 1);\
node->param = element->param;\
node->handler = element->handler;\
node->next = NULL;\
	\
if (*head == NULL) {\
	*head = node;\
} else {\
	cb_type *e = *head, *tail_e = NULL;\
	for (; e; e = e->next) {\
		if (0 == strcmp(node->name, e->name)) {\
			FREE_POINTER(node->name);\
			FREE_POINTER(node);\
			return 0;\
		}\
		if (e->next == NULL)\
			tail_e = e;\
	}\
	\
	if (NULL != tail_e)\
		tail_e->next = node;\
	else\
		FREE_POINTER(node);\
}\
return 1

#define CB_DESTROY(cb_type) \
cb_type *next;\
	\
for (; *head; *head = next) {\
	next = (*head)->next;\
	FREE_POINTER((*head)->name);\
	FREE_POINTER(*head);\
}

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
int device_info_cb_append(ipp_device_info_cb **head, ipp_device_info_cb *element) {
	CB_APPEND(ipp_device_info_cb);
}

void device_info_cb_destroy(ipp_device_info_cb **head) {
	CB_DESTROY(ipp_device_info_cb);
}

int device_event_cb_append(ipp_device_event_cb **head, ipp_device_event_cb *element) {
	CB_APPEND(ipp_device_event_cb);
}

void device_event_cb_destroy(ipp_device_event_cb **head) {
	CB_DESTROY(ipp_device_event_cb);
}

void reg_client_device_online_handler(char *cbName, void *param, device_info_handler handler)
{
	ipp_device_info_cb cb;
	cb.name = cbName;
	cb.param = param;
	cb.handler = handler;
	device_info_cb_append(&g_device_info_online_cb, &cb);
}

void reg_client_device_offline_handler(char *cbName, void *param, device_info_handler handler)
{
	ipp_device_info_cb cb;
	cb.name = cbName;
	cb.param = param;
	cb.handler = handler;
	device_info_cb_append(&g_device_info_offline_cb, &cb);
}

void reg_client_event_handler(char *cbName, void *param, event_handler handler)
{
	ipp_device_event_cb cb;
	cb.name = cbName;
	cb.param = param;
	cb.handler = handler;
	device_event_cb_append(&g_device_event_cb, &cb);
}

void call_device_online_handler(ipp_local_device* profile)
{
	ipp_device_info_cb *ele = g_device_info_online_cb;
	for (; ele; ele = ele->next) {
		ele->handler(ele->param, profile);
	}
}

void call_device_offline_handler(ipp_local_device* profile)
{
	ipp_device_info_cb *ele = g_device_info_offline_cb;
	for (; ele; ele = ele->next) {
		ele->handler(ele->param, profile);
	}
}

void call_event_handler(char *sn, int32_t dataLen, char *datas)
{
	ipp_device_event_cb *ele = g_device_event_cb;
	for (; ele; ele = ele->next) {
		ele->handler(ele->param, sn, dataLen, datas);
	}
}

void device_handler_destroy()
{
	device_info_cb_destroy(&g_device_info_online_cb);
	device_info_cb_destroy(&g_device_info_offline_cb);
	device_event_cb_destroy(&g_device_event_cb);
}
#endif

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
ipp_protocol *call_message_handler(void *param, request_handler handler, char *guid, ipp_protocol *protocol) 
{
	if (NULL != handler) {
		return handler(param, guid, protocol->buffer + protocol->buffer_index, 
			protocol->buffer_content_size - protocol->buffer_index);
	}

	return NULL;
}

void reg_server_message_handler(void *param, request_handler handler)
{
	g_server_message_param = param;
	g_server_message_handler = handler;
}

ipp_protocol *call_server_message_handler(char *guid, ipp_protocol *protocol)
{
	return call_message_handler(g_server_message_param, g_server_message_handler, 
		guid, protocol);
}
#endif

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
void reg_server_agent_message_handler(void *param, request_handler handler)
{
	g_server_agent_message_param = param;
	g_server_agent_message_handler = handler;
}

ipp_protocol *call_server_agent_message_handler(char *guid, ipp_protocol *protocol)
{
	return call_message_handler(g_server_agent_message_param, g_server_agent_message_handler, 
		guid, protocol);
}
#endif
