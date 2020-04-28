/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : 本地错误代码和错误打印宏
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _LOCAL_ERROR_CODE_H_
#define _LOCAL_ERROR_CODE_H_

#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
typedef enum
{
	local_inner_error = -1,
	local_inner_sucess = 0,
	create_mdns_sock_failed = 1,
	create_mdns_sock_SO_SNDTIMEO = 2,
	create_mdns_sock_SO_RCVTIMEO = 3,
	create_mdns_sock_SO_REUSEADDR = 4,
	create_mdns_sock_SO_REUSEPORT = 5,
	create_mdns_sock_bind = 6,
	create_mdns_sock_IP_MULTICAST_IF = 7,
	create_mdns_sock_IP_MULTICAST_TTL = 8,
	create_mdns_sock_IP_ADD_MEMBERSHIP = 9,
	create_mdns_sock_IP_MULTICAST_LOOP = 10,
	create_mdns_sock_SO_BROADCAST = 11,
	create_mdns_sock_IP_PKTINFO = 12,
	create_tcp_server_sock_failed = 13,
	create_tcp_server_sock_FIONBIO = 14,
	create_tcp_client_sock_FIONBIO = 15,
	deal_connect_FIONBIO = 16,
	create_tcp_server_sock_SO_SNDTIMEO = 17,
	create_tcp_server_sock_SO_RCVTIMEO = 18,
	create_tcp_server_sock_bind = 19,
	create_tcp_server_sock_bind_all_port = 20,
	create_tcp_server_sock_listen = 21,
	create_tcp_client_sock_failed = 22,
	create_tcp_client_sock_SO_SNDTIMEO = 23,
	create_tcp_client_sock_SO_RCVTIMEO = 24,
	recv_from_sock_select_failed = 25,
	recv_from_sock_data_too_much = 26,
	mdnsd_process_malloc_recv_buffer_failed = 27,
	copy_label_length_exceeds_buffer = 28,
	mdns_parse_rr_invalid_rr_data_len = 29,
	mdns_parse_rr_invalid_rr_data_len_AAAA = 30,
	mdns_parse_rr_data_AAAA_addr = 31,
    mdns_parse_rr_data_SRV_target = 32,
	mdns_parse_rr_unable_to_parse = 33,
	mdns_parse_rr_rr_data_len_is0 = 34,
	mdns_parse_rr_unable_to_copy_label = 35,
	mdns_parse_rr_txt_re_next = 36,
	mdns_encode_rr_unhandled_rr_type = 37,
	mdns_encode_pkt_packet_buffer_too_small = 38,
	check_buffer_head_get_head_failed = 39,
	check_buffer_head_get_info_len_failed = 40,
	control_local_device_fd_timed_out = 41,
	control_local_device_fd_not_found = 42,
	control_local_device_fd_async_not_found = 43,
	control_local_device_fd_async_failed = 44,
	client_deal_one_get_command_failed = 45,
	client_process_getpeername_failed = 46,
	client_process_recv_error = 47,
	client_process_recv_len0 = 48,
	client_process_heart_beat_timed_out = 49,
	client_process_disconnect = 50,
	client_start_create_server_info_failed = 51,
	client_start_create_sock_failed = 52,
	deal_request_create_protocol_failed = 53,
	response_local_device_fd_not_found = 54,
	server_deal_one_get_command_failed = 55,
	deal_connect_accept_failed = 56,
	server_process_getpeername_failed = 57,
	server_process_recv_error = 58,
	server_process_recv_len0 = 59,
	server_process_heart_beat_timed_out = 60,
	server_process_disconnect = 61,
	server_init_create_sock_failed = 62,
	get_device_info_get_agent_count_failed = 63,
	get_device_info_init_local_device_failed = 64,
	get_device_info_init_agent_device_failed = 65,
	get_device_info_copy_string_failed = 66,
	agent_device_online_init_device_failed = 67,
	agent_device_online_copy_string_failed = 68,
	agent_device_offline_init_device_failed = 69,
	agent_device_offline_copy_string_failed = 70,
	send_buffer_tried_too_much = 71,
	send_buffer_FD_ISSET_failed = 72,
	send_buffer_select_full = 73,
	send_buffer_select_error = 74,
	create_tcp_server_sock_SO_LINGER = 75,
	create_tcp_client_sock_SO_LINGER = 76,
	deal_connect_SO_RCVTIMEO = 77,
	create_udp_client_sock_failed = 78,
	create_udp_client_sock_FIONBIO = 79,
	create_udp_client_sock_SO_SNDTIMEO = 80,
	create_udp_client_sock_SO_RCVTIMEO = 81,
	create_udp_client_sock_bind = 82,
	create_udp_client_sock_bind_all_port = 83,
	client_init_create_sock_failed = 84,
	create_udp_server_sock_failed = 85,
	create_udp_server_sock_FIONBIO = 86,
	create_udp_server_sock_SO_SNDTIMEO = 87,
	create_udp_server_sock_SO_RCVTIMEO = 88,
	send_to_all_create_udp_server_sock = 89,
}local_error_code;

#define print_local_format_error(code, format, ...) \
	ipp_LogE_Prefix("Local error %d:"format, code, __VA_ARGS__)

#define print_local_error(code) \
	ipp_LogE_Prefix("Local error %d\r\n", code)

#define print_local_format_warnning(code, format, ...) \
	ipp_LogW_Prefix("Local error %d:"format, code, __VA_ARGS__)

#define print_local_warnning(code) \
	ipp_LogW_Prefix("Local error %d\r\n", code)

#define print_local_format_debug(code, format, ...) \
	ipp_LogD_Prefix("Local error %d:"format, code, __VA_ARGS__)

#define print_local_debug(code) \
	ipp_LogD_Prefix("Local error %d\r\n", code)
#endif

#endif