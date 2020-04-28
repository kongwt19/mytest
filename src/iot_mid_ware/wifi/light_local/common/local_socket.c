#include "local_module_enable.h"
#include "local_socket.h"
#include "local_error_code.h"
#include <stdint.h>
#include <stdio.h>
#include "ipp_defines.h"
#include "ipp_protocol.h"
#include "local_common.h"

#define RECV_BUFFER_SIZE 1024
static const uint16_t MAX_RECV_BUFFER_SIZE = 0;
static const int MAX_PORT = 10000;

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE ||IPP_LIGHT_LOCAL_CLIENT_ENABLE)
//将套接字添加到套接字列表
int peer_info_list_append(peer_info **head, peer_info *element) {
	peer_info *node = (peer_info *)malloc(sizeof(peer_info));
	MALLOC_ERROR_RETURN_WTTH_RET(node, -1)

	node->fd = element->fd;
	node->last_recv_heart_beat_time = get_process_msec();
	node->last_send_heart_beat_time = 0;
	node->peer_addr = element->peer_addr;
	node->peer_port = element->peer_port;
	node->udp_peer_port = 0;
	node->next = NULL;

	if (*head == NULL) {
		*head = node;
	} else {
		peer_info *e = *head, *tail_e = NULL;
		for (; e; e = e->next) {
			// already in list - don't add
			if (node->fd == e->fd || 
				(0 == memcmp(&node->peer_addr, &e->peer_addr, sizeof(struct in_addr)) && node->peer_port == e->peer_port)) {
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

//将套接字从套接字列表中删除
peer_info *peer_info_list_remove(peer_info **head, peer_info *info) {
	peer_info *le = *head, *pe = NULL;
	for (; le; le = le->next) {
		if (info->fd == le->fd || 
			(0 == memcmp(&info->peer_addr, &le->peer_addr, sizeof(struct in_addr)) && info->peer_port == le->peer_port)) {
			if (pe == NULL) {
				*head = le->next;
				FREE_POINTER(le);
				return *head;
			} else {
				pe->next = le->next;
				FREE_POINTER(le);
				return pe->next;
			}
		}
		pe = le;
	}
	return NULL;
}

//销毁套接字列表
void peer_info_list_destroy(peer_info **head) {
	peer_info *next;

	for (; *head; *head = next) {
		next = (*head)->next;
		FREE_POINTER(*head);
	}
}

//根据ip地址和端口号查找server信息节点
peer_info *peer_info_list_find(peer_info *server_list, struct in_addr server_addr, uint16_t port) {
	peer_info *ele = server_list;
	for (; ele; ele = ele->next) {
		if (0 == memcmp(&server_addr, &ele->peer_addr, sizeof(struct in_addr)) && port == ele->peer_port) {
			return ele;
		}
	}
	return NULL;
}
#endif

//设置套接字发送超时
int set_send_timeout(int fd, int second) {
	struct timeval tv;
	tv.tv_sec = second;
	tv.tv_usec = 0;
	return setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
}

//设置套接字接收超时
int set_recv_timeout(int fd, int second) {
	struct timeval tv;
	tv.tv_sec = second;
	tv.tv_usec = 0;
	return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
}

//设置关闭的行为
int set_so_linger(int fd) {
	struct linger so_linger;
	so_linger.l_onoff = 1;
	so_linger.l_linger = 0;
	return setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&so_linger, sizeof(so_linger));
}

//设置为非阻塞
int set_no_blocking(int fd)
{
	int r = -1;
#ifdef WIN32
	unsigned long ul = 1;
#else
	int flags = -1;
#endif

#ifdef WIN32
	if((r = ioctlsocket(fd, FIONBIO, &ul)) < 0)  {
		//print_local_format_error(create_tcp_server_sock_FIONBIO, "%d\r\n", get_sock_err());
		return r;
	}
#else
	if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
		//print_local_format_error(create_tcp_server_sock_F_GETFL, "%d\r\n", flags);
		return flags;
	}

	if ((r = fcntl(fd, F_SETFL, flags | O_NONBLOCK)) < 0) {
		//print_local_format_error(create_tcp_server_sock_F_SETFL, "%d\r\n", r);
		return r;
	}
#endif

	return r;
}

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
//创建server需要的tcp套接字
int create_tcp_server_sock(uint16_t *server_port, uint8_t listen_cnt) {
	int sd = (int)socket(PF_INET, SOCK_STREAM, 0);
	int r = -1;

	struct sockaddr_in serveraddr;

	if (sd < 0) {
		print_local_error(create_tcp_server_sock_failed);
		return sd;
	}

	//设置为非阻塞
	if (set_no_blocking(sd) < 0) {
		print_local_format_warnning(create_tcp_server_sock_FIONBIO, "%d\r\n", get_sock_err());
	}

	//设置发送超时
	if (set_send_timeout(sd, 3) < 0) {
		print_local_format_warnning(create_tcp_server_sock_SO_SNDTIMEO, "%d\r\n", get_sock_err());
	}

	//设置接收超时
	if (set_recv_timeout(sd, 8) < 0) {
		print_local_format_warnning(create_tcp_server_sock_SO_RCVTIMEO, "%d\r\n", get_sock_err());
	}

	//if (set_so_linger(sd) < 0) {
	//	print_local_format_warnning(create_tcp_server_sock_SO_LINGER, "%d\r\n", get_sock_err());
	//}

	/* bind to an address */
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	for (;*server_port < MAX_PORT; ++(*server_port))
	{
		serveraddr.sin_port = htons(*server_port);
		if ((r = bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0) {
			print_local_format_warnning(create_tcp_server_sock_bind, "%d\r\n", get_sock_err());
		} else {
			break;
		}
	}

	if (*server_port >= MAX_PORT) {
		print_local_format_error(create_tcp_server_sock_bind_all_port, "%d\r\n", get_sock_err());
		return r;
	}

	if ((r = listen(sd, listen_cnt)) < 0) {
		print_local_format_error(create_tcp_server_sock_listen, "%d\r\n", get_sock_err());
		return r;
	}  

	return sd;
}

//创建server需要的udp套接字
int create_udp_server_sock()
{
	int sd = (int)socket(PF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		print_local_error(create_udp_server_sock_failed);
		return sd;
	}

	//设置为非阻塞
	if (set_no_blocking(sd) < 0) {
		print_local_format_warnning(create_udp_server_sock_FIONBIO, "%d\r\n", get_sock_err());
	}

	//设置发送超时
	if (set_send_timeout(sd, 3) < 0) {
		print_local_format_warnning(create_udp_server_sock_SO_SNDTIMEO, "%d\r\n", get_sock_err());
	}

	//设置接收超时
	if (set_recv_timeout(sd, 8) < 0) {
		print_local_format_warnning(create_udp_server_sock_SO_RCVTIMEO, "%d\r\n", get_sock_err());
	}

	//if (set_so_linger(sd) < 0) {
	//	print_local_format_warnning(create_tcp_client_sock_SO_LINGER, "%d\r\n", get_sock_err());
	//}

	return sd;
}
#endif

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
//创建client需要的tcp套接字
int create_tcp_client_sock() {
	int sd = (int)socket(PF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		print_local_error(create_tcp_client_sock_failed);
		return sd;
	}

	//设置为非阻塞
	//if (set_no_blocking(sd) < 0) {
	//	print_local_format_warnning(create_tcp_client_sock_FIONBIO, "%d\r\n", get_sock_err());
	//}

	//设置发送超时
	if (set_send_timeout(sd, 3) < 0) {
		print_local_format_warnning(create_tcp_client_sock_SO_SNDTIMEO, "%d\r\n", get_sock_err());
	}

	//设置接收超时
	if (set_recv_timeout(sd, 8) < 0) {
		print_local_format_warnning(create_tcp_client_sock_SO_RCVTIMEO, "%d\r\n", get_sock_err());
	}

	//if (set_so_linger(sd) < 0) {
	//	print_local_format_warnning(create_tcp_client_sock_SO_LINGER, "%d\r\n", get_sock_err());
	//}

	return sd;
}

//创建client需要的udp套接字
int create_udp_client_sock(uint16_t *client_port)
{
	int sd = (int)socket(PF_INET, SOCK_DGRAM, 0);
	int r = -1;

	struct sockaddr_in serveraddr;
	if (sd < 0) {
		print_local_error(create_udp_client_sock_failed);
		return sd;
	}

	//设置为非阻塞
	if (set_no_blocking(sd) < 0) {
		print_local_format_warnning(create_udp_client_sock_FIONBIO, "%d\r\n", get_sock_err());
	}

	//设置发送超时
	if (set_send_timeout(sd, 3) < 0) {
		print_local_format_warnning(create_udp_client_sock_SO_SNDTIMEO, "%d\r\n", get_sock_err());
	}

	//设置接收超时
	if (set_recv_timeout(sd, 8) < 0) {
		print_local_format_warnning(create_udp_client_sock_SO_RCVTIMEO, "%d\r\n", get_sock_err());
	}

	//if (set_so_linger(sd) < 0) {
	//	print_local_format_warnning(create_tcp_client_sock_SO_LINGER, "%d\r\n", get_sock_err());
	//}

	/* bind to an address */
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	for (;*client_port < MAX_PORT; ++(*client_port))
	{
		serveraddr.sin_port = htons(*client_port);
		if ((r = bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0) {
			print_local_format_warnning(create_udp_client_sock_bind, "%d\r\n", get_sock_err());
		} else {
			break;
		}
	}

	if (*client_port >= MAX_PORT) {
		print_local_format_error(create_udp_client_sock_bind_all_port, "%d\r\n", get_sock_err());
		return r;
	}

	return sd;
}
#endif

#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
//创建mdns需要的udp套接字
int create_mdns_sock(uint16_t mdns_port, const char *mdns_addr) {
	int sd = (int)socket(PF_INET, SOCK_DGRAM, 0);
	int r = -1;
	int on = 1;
	struct sockaddr_in serveraddr;
	unsigned char ttl = 120;

	if (sd < 0) {
		print_local_error(create_mdns_sock_failed);
		return sd;
	}

	//设置发送超时
	if (set_send_timeout(sd, 3) < 0) {
		print_local_format_warnning(create_mdns_sock_SO_SNDTIMEO, "%d\r\n", get_sock_err());
	}

	//设置接收超时
	if (set_recv_timeout(sd, 8) < 0) {
		print_local_format_warnning(create_mdns_sock_SO_RCVTIMEO, "%d\r\n", get_sock_err());
	}

	//设置端口复用
	if ((r = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) < 0) {
		print_local_format_warnning(create_mdns_sock_SO_REUSEADDR, "%d\r\n", get_sock_err());
	}

#ifdef __APPLE__
	on = 1;
	if ((r = setsockopt(sd, SOL_SOCKET, SO_REUSEPORT,(char*)&on, sizeof(on))) < 0) {
		print_local_format_warnning(create_mdns_sock_SO_REUSEPORT, "%d\r\n", get_sock_err());
	}
#endif

	/* bind to an address */
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(mdns_port);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);	/* receive multicast */
	if ((r = bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0) {
		print_local_format_error(create_mdns_sock_bind, "%d\r\n", get_sock_err());
		return r;
	}

	if (NULL != mdns_addr) {
		struct ip_mreq mreq;
		memset(&mreq, 0, sizeof(struct ip_mreq));
		mreq.imr_interface.s_addr = INADDR_ANY;
		if ((r = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char*) &mreq.imr_interface.s_addr, sizeof(mreq.imr_interface.s_addr))) < 0)  {
			print_local_format_error(create_mdns_sock_IP_MULTICAST_IF, "%d\r\n", get_sock_err());
			return r;
		}

		if ((r = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, (char*) &ttl, sizeof(ttl))) < 0) {
			print_local_format_error(create_mdns_sock_IP_MULTICAST_TTL, "%d\r\n", get_sock_err());
			return r;
		}

		// add membership to receiving socket
		mreq.imr_multiaddr.s_addr = inet_addr(mdns_addr);
		if ((r = setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mreq, sizeof(mreq))) < 0) {
			print_local_format_error(create_mdns_sock_IP_ADD_MEMBERSHIP, "%d\r\n", get_sock_err());
			return r;
		}

		on = 1;
		// enable loopback in case someone else needs the data
		if ((r = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *) &on, sizeof(on))) < 0) {
			print_local_format_error(create_mdns_sock_IP_MULTICAST_LOOP, "%d\r\n", get_sock_err());
			return r;
		}
	} else {
		const int opt = -1;
		if ((r = setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt))) < 0) {
			print_local_format_error(create_mdns_sock_SO_BROADCAST, "%d\r\n", get_sock_err());
			return r;
		}
	}

#ifdef IP_PKTINFO
	on = 1;
	if ((r = setsockopt(sd, IPPROTO_IP, IP_PKTINFO, (char *) &on, sizeof(on))) < 0) {
		print_local_format_error(create_mdns_sock_IP_PKTINFO, "%d\r\n", get_sock_err());
		return r;
	}
#endif

	return sd;
}

//在指定的套接字上发送buffer
int send_buffer(int s, char *buffer, int size)
{
	int sent = 0;
	int nTryTime = 0;
	const int maxTryTime = 3;

	while (sent < size)
	{
		int ret = 0;
		fd_set fd_write;
		struct timeval tv = {3, 0};

		FD_ZERO(&fd_write);
		FD_SET(s, &fd_write);
		ret = select(s + 1, NULL, &fd_write, NULL, &tv);

		//尝试发送次数超过上限，则认为发送异常
		if (nTryTime > maxTryTime) {
			print_local_error(send_buffer_tried_too_much);
			return -1;
		}

		if (ret > 0)
		{
			if (FD_ISSET(s, &fd_write)) {
				int tmpres = send(s, buffer + sent, size - sent, 0);
				if (tmpres <= 0)
					return -1;

				sent += tmpres;
			} else {
				print_local_error(send_buffer_FD_ISSET_failed);
				return -1;
			}
		} else if (0 == ret){
			print_local_error(send_buffer_select_full);
			return 0;
		} else {
			print_local_format_error(send_buffer_select_error, "%d\r\n", get_sock_err());
			return -1;
		}

		++nTryTime;
	}

	return 1;
}

//从指定socket获取数据，函数内部出现异常，不会释放buffer，尽量返回可处理的字节，所以无论函数返回什么，使用者都一定要释放buffer
int recv_from_sock(int s, char **recv_buffer)
{
	uint32_t tatal_len = 0;
	
	*recv_buffer = NULL;
	while (tatal_len <= MAX_RECV_BUFFER_SIZE || 0 == MAX_RECV_BUFFER_SIZE) 
	{
		int select_ret = 0;
		fd_set sockfd_set;
		struct timeval tv = {3, 0};
		
		FD_ZERO(&sockfd_set);
		FD_SET(s, &sockfd_set);
		select_ret = select(s + 1, &sockfd_set, NULL, NULL, &tv);
		if (select_ret > 0 && FD_ISSET(s, &sockfd_set)) {
			char *buffer = (char *)malloc(RECV_BUFFER_SIZE);
			int recv_len = 0;

			memset(buffer, 0, RECV_BUFFER_SIZE);
			recv_len = recv(s, buffer, RECV_BUFFER_SIZE, 0);
			if (0 == recv_len) {
				FREE_POINTER(buffer);
				return 0;
			} else if (recv_len < 0) {
				FREE_POINTER(buffer);
				return (tatal_len > 0) ? tatal_len : -1;
			} else {
				if (NULL == *recv_buffer) {
					*recv_buffer = (char *)malloc(recv_len);
					MALLOC_ERROR_RETURN_WTTH_RET(*recv_buffer, -1);
				} else {
					char *recv_buffer_temp = (char *)realloc(*recv_buffer, tatal_len + recv_len);
					MALLOC_ERROR_RETURN_WTTH_RET(recv_buffer_temp,-1)
					*recv_buffer = recv_buffer_temp;
				}
				memcpy(*recv_buffer + tatal_len, buffer, recv_len);
				tatal_len += recv_len;

				if (recv_len < RECV_BUFFER_SIZE) {
					FREE_POINTER(buffer);
					return (tatal_len > 0) ? tatal_len : -1;
				}
			}

			FREE_POINTER(buffer);
		} else if (select_ret < 0){
			print_local_format_warnning(recv_from_sock_select_failed, "%d\r\n", select_ret);
			return (tatal_len > 0) ? tatal_len : -1;
		} else {
			return (tatal_len > 0) ? tatal_len : -1;
		}
	}

	print_local_warnning(recv_from_sock_data_too_much);
	return (tatal_len > 0) ? tatal_len : -1;
}

//关闭套接字
int local_close_sock(int s) {
	local_shutdown_sock(s);
#ifdef _WIN32
	return closesocket(s);
#else
	return close(s);
#endif
}

//停止套接字
int local_shutdown_sock(int s) {
#ifdef _WIN32
	return shutdown(s, SD_BOTH);
#else
	return shutdown(s, SHUT_RDWR);
#endif
}

//获取socket的错误
int get_sock_err()
{
#ifdef _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

#endif
