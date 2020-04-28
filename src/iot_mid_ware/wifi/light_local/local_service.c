#include "local_module_enable.h"
#include "local_mdns.h"
#include "local_tcp.h"
#include "local_service.h"
#include "device_manager.h"
#include "local_socket.h"
#include "local_tcp_server.h"
#include "local_tcp_client.h"
#include "local_agent.h"
#include "local_handler_manager.h"
#include "ipp_defines.h"
#include "local_common.h"
//#include "ipp_util.h"
#include <stdio.h>

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
typedef enum
{
	thread_run_stat_stopped,
	thread_run_stat_running,
	thread_run_stat_stopping,
	thread_run_stat_deleted,
}thread_run_stat;

ipp_local_device *g_self = NULL;
static const char *LOCAL_MDNS_THREAD = "local_mdns_thread";

static uint8_t g_mdns_stop_flag = thread_run_stat_stopped;
static uint8_t g_server_stop_flag = thread_run_stat_stopped;
static uint8_t g_client_stop_flag = thread_run_stat_stopped;

static const uint8_t CHECK_DEVICES_TIME_OUT = 60;
static uint32_t g_last_check_devices_time = 0;
#endif

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
static const char *LOCAL_SERVER_THREAD = "local_server_thread";
#endif

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
static const char *LOCAL_CLIENT_THREAD = "local_client_thread";
#endif

static const char *LIGHT_LOCAL_VER = "1.0.0.3";

//-----------------------------------------�ص������ע��-----------------------------------------------

void reg_local_event_handler(char *cbName, void *param, event_handler handler)
{
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	reg_client_event_handler(cbName, param, handler);
#endif
}

void reg_local_device_online_handler(char *cbName, void *param, device_info_handler handler)
{
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	reg_client_device_online_handler(cbName, param, handler);
#endif
}

void reg_local_device_offline_handler(char *cbName, void *param, device_info_handler handler)
{
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	reg_client_device_offline_handler(cbName, param, handler);
#endif
}

void reg_local_message_handler(void *param, request_handler handler)
{
#if IPP_LIGHT_LOCAL_SERVER_ENABLE
	reg_server_message_handler(param, handler);
#endif
}

void reg_local_agent_message_handler(void *param, request_handler handler)
{
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
	reg_server_agent_message_handler(param, handler);
#endif
}

//-----------------------------------------��ʼ����ֹͣ-----------------------------------------------
void local_service_init(ipp_local_device *profile)
{
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
	free_local_device(&g_self);
	g_self = get_device_clone_from_other(profile);
#endif
}

void local_service_deinit()
{
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
	free_local_device(&g_self);
#endif
}

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
void local_mdns_loop(void *arg)
{
	detach_thread();
	ipp_LogI("local_mdns start\r\n");

	while (thread_run_stat_running == g_mdns_stop_flag) 
	{
		uint32_t curtime = 0;

		fd_set sockfd_set;
		struct timeval tv = {1, 0};
		int mdns_fd_max = get_mdns_max_fd();

		FD_ZERO(&sockfd_set);
		mdnsd_fd_set(&sockfd_set);
		cycle_send();

		select(mdns_fd_max + 1, &sockfd_set, NULL, NULL, &tv);
		mdnsd_process(&sockfd_set);

		curtime = get_process_msec();
		if (TRUE == check_time_out(curtime,g_last_check_devices_time, CHECK_DEVICES_TIME_OUT * 1000)) {
			g_last_check_devices_time = curtime;
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
			print_client_sock_list();
#endif
#if IPP_LIGHT_LOCAL_SERVER_ENABLE
			print_server_sock_list();
#endif
		}
		//ipp_LogV("local_service_loop\r\n");
	}

	ipp_LogI("local_mdns end\r\n");
	g_mdns_stop_flag = thread_run_stat_deleted;
	delete_thread(NULL);
}
#endif

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
void local_server_loop(void *arg)
{
	detach_thread();
	ipp_LogI("local_server start\r\n");

	while (thread_run_stat_running == g_server_stop_flag) 
	{
		fd_set sockfd_set;
		struct timeval tv = {1, 0};
		int server_max_fd = get_server_max_fd();

		FD_ZERO(&sockfd_set);
		server_fd_set(&sockfd_set);
		select(server_max_fd + 1, &sockfd_set, NULL, NULL, &tv);
		server_process(&sockfd_set);
		//ipp_LogV("local_service_loop\r\n");
	}

	ipp_LogI("local_server end\r\n");
	g_server_stop_flag = thread_run_stat_deleted;
	delete_thread(NULL);
}
#endif

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
void local_client_loop(void *arg)
{
	detach_thread();
	ipp_LogI("local_client start\r\n");

	while (thread_run_stat_running == g_client_stop_flag) 
	{
		fd_set sockfd_set;
		struct timeval tv = {1, 0};
		int client_max_fd = get_client_max_fd();

		FD_ZERO(&sockfd_set);
		client_fd_set(&sockfd_set);
		select(client_max_fd + 1, &sockfd_set, NULL, NULL, &tv);
		client_process(&sockfd_set);
		//ipp_LogV("local_service_loop\r\n");
	}

	ipp_LogI("local_client end\r\n");
	g_client_stop_flag = thread_run_stat_deleted;
	delete_thread(NULL);
}
#endif

void localservice_destory()
{
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
	mdnsd_stop();
	g_mdns_stop_flag = thread_run_stat_stopped;
#endif

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
	close_server();
	g_server_stop_flag = thread_run_stat_stopped;
#endif

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	close_client();
	g_client_stop_flag = thread_run_stat_stopped;
#endif
}

void localservice_start(ipp_device_type init_type, int server_stack_size, int client_stack_size)
{
	ipp_LogV("ls start:s=%d,a=%d,c=%d;t=%d,v=%s\r\n",
		IPP_LIGHT_LOCAL_SERVER_ENABLE, IPP_LIGHT_LOCAL_AGENT_ENABLE, IPP_LIGHT_LOCAL_CLIENT_ENABLE,
		init_type, LIGHT_LOCAL_VER);

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
	if (thread_run_stat_stopped == g_mdns_stop_flag &&
		thread_run_stat_stopped == g_server_stop_flag &&
		thread_run_stat_stopped == g_client_stop_flag) {
		thread_t tid;
		ipp_LogV("########################\r\n");
		ipp_LogV("g_self:\r\n");
		print_device_info(g_self);
		ipp_LogV("########################\r\n");

		if (tcpd_init(init_type) <= 0) {
			localservice_destory();
			return;
		}

		if (mdnsd_init(init_type) <= 0) {
			localservice_destory();
			return;
		}

		g_mdns_stop_flag = thread_run_stat_running;
		if (THREAD_OK != create_thread((THREAD_VOID (*)(void *))local_mdns_loop, 
			LOCAL_MDNS_THREAD, 1024, NULL, 2, &tid, NULL)) {
			ipp_LogE_Prefix("%s failed!\r\n", LOCAL_MDNS_THREAD);
			g_mdns_stop_flag = thread_run_stat_stopped;
			localservice_destory();
		}

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
		g_server_stop_flag = thread_run_stat_running;
		if (THREAD_OK != create_thread((THREAD_VOID (*)(void *))local_server_loop, 
			LOCAL_SERVER_THREAD, server_stack_size, NULL, 1, &tid, NULL)) {
			g_server_stop_flag = thread_run_stat_stopped;
			ipp_LogE_Prefix("%s failed!\r\n", LOCAL_SERVER_THREAD);
			localservice_destory();
		}
#endif

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
		g_client_stop_flag = thread_run_stat_running;
		if (THREAD_OK != create_thread((THREAD_VOID (*)(void *))local_client_loop, 
			LOCAL_CLIENT_THREAD, client_stack_size, NULL, 0, &tid, NULL)) {
			g_client_stop_flag = thread_run_stat_stopped;
			ipp_LogE_Prefix("%s failed!\r\n", LOCAL_CLIENT_THREAD);
			localservice_destory();
		}
#endif
	}
#endif
}

void localservice_stop()
{
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
	if (thread_run_stat_stopped != g_mdns_stop_flag) {
		g_mdns_stop_flag = thread_run_stat_stopping;

		while (thread_run_stat_deleted != g_mdns_stop_flag)
			sleep_ms(100);
	}
#endif

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
	if (thread_run_stat_stopped != g_server_stop_flag) {
		g_server_stop_flag = thread_run_stat_stopping;

		while (thread_run_stat_deleted != g_server_stop_flag)
			sleep_ms(100);
	}
#endif

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	if (thread_run_stat_stopped != g_client_stop_flag) {
		g_client_stop_flag = thread_run_stat_stopping;

		while (thread_run_stat_deleted != g_client_stop_flag)
			sleep_ms(100);
	}
#endif

	localservice_destory();
}

//-----------------------------------------�豸�����Ϳ���-----------------------------------------------

ipp_local_device *get_local_device(char *guid)
{
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	return get_device_clone(guid);
#else
	return NULL;
#endif
}

ipp_device_list *get_local_device_list()
{
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	return get_device_list_clone();
#else
	return NULL;
#endif
}

local_control_result control_local_device(char *guid, char* command, int command_len, char **response, int *rep_len)
{
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	int fd = device_fd_find(guid);
	if (fd >= 0) {
		local_error_code err = control_local_device_fd(fd, guid, command, command_len, response, rep_len);
		switch (err)
		{
		case local_inner_sucess:
			return control_success;
		case control_local_device_fd_timed_out:
			return control_timed_out;
		case control_local_device_fd_not_found:
			return device_not_exist;
		default:
			return control_failed;
		}
	} else {
		ipp_LogE_Prefix("not found %s\r\n", guid);
		return device_not_exist;
	}
#else
	return control_failed;
#endif
}

IPP_BOOL control_all_local_devices(char *command, int command_len)
{
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	IPP_BOOL CallRet = TRUE;
	ipp_device_list *dlist = get_local_device_list();
	ipp_device_list *ele = dlist;
	for (; ele; ele = ele->next) {
		char *reply = NULL;
		int replyLen;
		if (FALSE == control_local_device(ele->device->guid, command, command_len, &reply, &replyLen)) {
			CallRet = FALSE;
		}
		FREE_POINTER(reply);
	}
	free_local_device_list(&dlist);
	return CallRet;
#else
	return FALSE;
#endif
}

local_control_result control_local_device_async(char *guid, char *command, int command_len, response_handler handler, void *param)
{
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	int fd = device_fd_find(guid);
	if (fd >= 0) {
		local_error_code err = control_local_device_fd_async(fd, guid, command, command_len, handler, param);
		switch (err)
		{
		case local_inner_sucess:
			return control_success;
		case control_local_device_fd_not_found:
			return device_not_exist;
		default:
			return control_failed;
		}
	} else {
		ipp_LogE_Prefix("not found %s\r\n", guid);
		return device_not_exist;
	}
#else
	return control_failed;
#endif
}

IPP_BOOL control_all_local_device_async(char *command, int command_len)
{
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	IPP_BOOL CallRet = TRUE;
	ipp_device_list *dlist = get_local_device_list();
	ipp_device_list *ele = dlist;
	for (; ele; ele = ele->next) {
		if (FALSE == control_local_device_async(ele->device->guid, command, command_len, NULL, NULL)) {
			CallRet = FALSE;
		}
	}
	free_local_device_list(&dlist);
	return CallRet;
#else
	return FALSE;
#endif
}

IPP_BOOL send_local_event(char *guid, char* command, int command_len)
{
#if IPP_LIGHT_LOCAL_SERVER_ENABLE
	return server_send_event(guid, command, command_len);
#else
	return FALSE;
#endif
}

//-----------------------------------------�������豸�ӿ�-----------------------------------------------
void local_agent_online(char *gateway_guid, ipp_local_device *device)
{
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
	agent_online(gateway_guid, device);
#endif
}

void local_agent_offline(char *gateway_guid, ipp_local_device *device)
{
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
	agent_offline(gateway_guid, device);
#endif
}

void local_agent_update(char* guid, int data_len, char *agent_data)
{
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE && IPP_LIGHT_LOCAL_AGENT_ENABLE)
	agent_update(guid, data_len, agent_data);
#endif
}

//-----------------------------------------�����ӿ�(DEBUG)-----------------------------------------------

//�ͻ��˱����socket�б�
void print_local_client_sock_list()
{
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
	print_client_sock_list(1);
#endif
}

//����˱����socket�б�
void print_local_server_sock_list()
{
#if IPP_LIGHT_LOCAL_SERVER_ENABLE
	print_server_sock_list(1);
#endif
}
