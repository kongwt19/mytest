#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
#include "local_mdns.h"
#include "local_socket.h"
#include "local_tcp.h"
#include "local_error_code.h"
#include <stdio.h>
#include "ipp_defines.h"
#include "local_tcp_server.h"
#include "local_tcp_client.h"
#include "ipp_local_device_info.h"
#include "local_common.h"
//#include "ipp_util.h"

typedef enum
{
	SEND_MDNS,
	SEND_IPP_MDNS,
	SEND_RESPONSE,
	SEND_QUESTION,
}MDNS_SEND_TYPE;

//��ǰserver�����Ķ˿�
extern uint16_t g_service_port;
//mdns�׽��ֹ����ṹ��
static mdnsd *g_mdns_sock = NULL;

//mdns�˿�
static const uint16_t MDNS_PORT = 5353;
//mdns��
static const char *MDNS_ADDR = "224.0.0.251";

//����mdns�˿�
static const uint16_t IPP_MDNS_PORT = 9393;
//����mdns��
static const char *IPP_MDNS_ADDR = "224.0.0.151";

//������
static const char *SERVICES_NLABEL = "chipp.tcp.local";
static const uint8_t PACKET_SIZE = 255;
static const uint8_t MAX_SEND_TIME_OUT = 30;
static float g_cur_send_time_out = 1;
static uint32_t g_last_cycle_send_time = 0;
static uint8_t g_last_cycle_send_type = 0;
static uint8_t g_mdns_max_fd = 0xff;

//ʹ�ܹ㲥ͨ���ķ���
void enable_broadcast(IPP_BOOL enable)
{
	g_mdns_max_fd = (TRUE == enable) ? 0xff : 1;
}

//�������ݰ�
int send_packet(const void *data, size_t len, int fd_list[FD_LIST_CNT], int fd_idx, MDNS_SEND_TYPE mdns_send_type) {
	if (fd_list[fd_idx] >= 0) {
		struct sockaddr_in toaddr;
		memset(&toaddr, 0, sizeof(struct sockaddr_in));
		toaddr.sin_family = AF_INET;
		toaddr.sin_port = htons((SEND_MDNS == mdns_send_type) ? MDNS_PORT : IPP_MDNS_PORT);
		if (0 == fd_idx)
			toaddr.sin_addr.s_addr = inet_addr((SEND_MDNS == mdns_send_type) ? MDNS_ADDR : IPP_MDNS_ADDR);
		else
			toaddr.sin_addr.s_addr = INADDR_BROADCAST;
		return sendto(fd_list[fd_idx], (char *)data, (int)len, 0, 
			(struct sockaddr *) &toaddr, sizeof(struct sockaddr_in));
	}
	
	return -1;
}

#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
//����ѯ����Ϣ
void send_question(int fd_list[FD_LIST_CNT], int fd_idx, MDNS_SEND_TYPE mdns_send_type)
{
	uint8_t *sendbuff = NULL;
	size_t replylen = 0;
	rr_question *qn = NULL;

	mdns_pkt *pkt = (mdns_pkt *)malloc(sizeof(mdns_pkt));
	MALLOC_ERROR_RETURN(pkt)
	memset(pkt, 0, sizeof(mdns_pkt));

	pkt->id = TRASANCTION_ID;
	pkt->flags = 0;

	qn = rr_create_ptr(create_nlabel(SERVICES_NLABEL));
	rr_qlist_append(&(pkt->rr_qn), qn);
	pkt->num_qn++;

	sendbuff = (uint8_t *)malloc(PACKET_SIZE);
	if (NULL == sendbuff) {
		ipp_LogE_Prefix("malloc:%s(%d)\r\n",__FUNCTION__,__LINE__);
		mdns_pkt_destroy(&pkt);
		return;
	}

	replylen = mdns_encode_pkt(pkt, sendbuff, PACKET_SIZE);
	mdns_pkt_destroy(&pkt);

	send_packet((char *)sendbuff, replylen, fd_list, fd_idx, mdns_send_type);
	FREE_POINTER(sendbuff);
}
#endif

#if IPP_LIGHT_LOCAL_SERVER_ENABLE
//���ͻظ���Ϣ
void send_response(int fd_list[FD_LIST_CNT], int fd_idx, MDNS_SEND_TYPE mdns_send_type)
{
	uint8_t *sendbuff = NULL;
	size_t replylen = 0;
	rr_entry *srv = NULL;

	mdns_pkt *pkt = (mdns_pkt *)malloc(sizeof(mdns_pkt));
	MALLOC_ERROR_RETURN(pkt)
	memset(pkt, 0, sizeof(mdns_pkt));

	pkt->id = TRASANCTION_ID;
	pkt->flags = MDNS_FLAG_RESP;

	srv = rr_create_srv(create_nlabel(SERVICES_NLABEL), g_service_port, create_label(g_self->guid));
	rr_list_append(&(pkt->rr_ans), srv);
	pkt->num_ans_rr++;

	sendbuff = (uint8_t *)malloc(PACKET_SIZE);
	if (NULL == sendbuff) {
		ipp_LogE_Prefix("malloc:%s(%d)\r\n",__FUNCTION__,__LINE__);
		mdns_pkt_destroy(&pkt);
		return;
	}

	replylen = mdns_encode_pkt(pkt, sendbuff, PACKET_SIZE);
	mdns_pkt_destroy(&pkt);

	send_packet((char *)sendbuff, replylen, fd_list, fd_idx, mdns_send_type);
	FREE_POINTER(sendbuff);
}
#endif

//��鵱ǰ���ֵķ����Ƿ����ע���������
int check_model(mdnsd *svr, uint8_t *name) {
	if (NULL != name) {
		rr_question *qn;

		// check if we have the records
		mutex_lock(&svr->data_lock);
		qn = rr_question_find(svr->rr_qn, name);
		if (qn == NULL) {
			mutex_unlock(&svr->data_lock);
			return 0;
		}

		mutex_unlock(&svr->data_lock);
		return 1;
	}
	
	return 0;
}

// processes the incoming MDNS packet
// returns >0 if processed, 0 otherwise
static int process_mdns_pkt(mdnsd *svr, mdns_pkt *pkt, char **target) {
	int i;
	rr_qlist *qnl;
	rr_list *ansl;

	if (NULL == pkt)
		return 0;

	if (!MDNS_FLAG_GET_OPCODE(pkt->flags) == 0)
		return 0;

	// standard query
	if ((pkt->flags & MDNS_FLAG_RESP) == 0) 
	{
		// loop through questions
		qnl = pkt->rr_qn;
		for (i = 0; i < pkt->num_qn && qnl; i++, qnl = qnl->next) {
			int printed = 0;
			rr_question *qn = qnl->qn;
			int num_check_added = check_model(svr, qn->name);
			if (num_check_added > 0) {
				if (0 == printed) {
					//ipp_LogV_Prefix("mdns query\r\n");
					printed = 1;
				}
				return num_check_added;
			}
		}
		return 0;
	} else {// answers
		// loop through questions
		ansl = pkt->rr_ans;
		for (i = 0; i < pkt->num_ans_rr && ansl; i++, ansl = ansl->next) {
			int printed = 0;
			rr_entry *ans = ansl->e;
			int num_check_added = check_model(svr, ans->name);
			if (num_check_added > 0 && RR_SRV == ans->type) {
				*target = nlabel_to_str(ans->data.SRV.target);
				if (0 == printed) {
					//ipp_LogV_Prefix("mdns response %s\r\n", *target);
					printed = 1;
				}
				return ans->data.SRV.port;
			}
		}
		return 0;
	}
}

//���ƶ����׽����������Է���request��response
void cycle_send_fd_list(int fd_list[FD_LIST_CNT], MDNS_SEND_TYPE send_type, MDNS_SEND_TYPE mdns_send_type)
{
	int i;
	for (i = 0; i < (int)ipp_min(FD_LIST_CNT, g_mdns_max_fd); ++i) {
		if (SEND_QUESTION == send_type) {
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
			send_question(fd_list, i, mdns_send_type);
#endif
		} else {
#if IPP_LIGHT_LOCAL_SERVER_ENABLE
			send_response(fd_list, i, mdns_send_type);
#endif
		}
	}
}

//���ڷ���request��response
void cycle_send()
{
	uint32_t cur_time = get_process_msec();
	if ((0 == g_last_cycle_send_time) || TRUE == check_time_out(cur_time, g_last_cycle_send_time, (uint32_t)g_cur_send_time_out * 1000))
	{
		char sended = 0;
		g_last_cycle_send_time = cur_time;
		if (0 == g_last_cycle_send_type) {
			if (g_mdns_sock->init_type & ipp_device_client) {
				cycle_send_fd_list(g_mdns_sock->mdns_fd, SEND_QUESTION, SEND_MDNS);
				cycle_send_fd_list(g_mdns_sock->ipp_mdns_fd, SEND_QUESTION, SEND_IPP_MDNS);
				sended = 1;
			}
			g_last_cycle_send_type = 1;
		}

		if (!sended) {
			if (1 == g_last_cycle_send_type) {
				if ((g_mdns_sock->init_type & ipp_device_server)) {
					cycle_send_fd_list(g_mdns_sock->mdns_fd, SEND_RESPONSE, SEND_MDNS);
					cycle_send_fd_list(g_mdns_sock->ipp_mdns_fd, SEND_RESPONSE, SEND_IPP_MDNS);
				}
			}
			g_last_cycle_send_type = 0;
		}

		g_cur_send_time_out *= 1.5;
		g_cur_send_time_out = (g_cur_send_time_out >= MAX_SEND_TIME_OUT) ? MAX_SEND_TIME_OUT : g_cur_send_time_out;
	}
}

//mdnsģ���fd set
void mdnsd_fd_set(fd_set *read_set)
{
	int i;
	for (i = 0; i < (int)ipp_min(FD_LIST_CNT, g_mdns_max_fd); ++i) {
		if (g_mdns_sock->mdns_fd[i] >= 0)
			FD_SET(g_mdns_sock->mdns_fd[i], read_set);
	}

	for (i = 0; i < (int)ipp_min(FD_LIST_CNT, g_mdns_max_fd); ++i) {
		if (g_mdns_sock->ipp_mdns_fd[i] >= 0)
			FD_SET(g_mdns_sock->ipp_mdns_fd[i], read_set);
	}
}

//��ȡmdns�����������׽���
int get_mdns_max_fd()
{
	int i;
	int mdns_max = 0;
	for (i = 0; i < (int)ipp_min(FD_LIST_CNT, g_mdns_max_fd); ++i) {
		if (g_mdns_sock->mdns_fd[i] > mdns_max)
			mdns_max = g_mdns_sock->mdns_fd[i];
	}

	for (i = 0; i < (int)ipp_min(FD_LIST_CNT, g_mdns_max_fd); ++i) {
		if (g_mdns_sock->ipp_mdns_fd[i] > mdns_max)
			mdns_max = g_mdns_sock->ipp_mdns_fd[i];
	}

	return mdns_max;
}

//�����׽�����Ĵ�������
void mdnsd_process_one_fd_list(fd_set *read_set, int fd_list[FD_LIST_CNT], MDNS_SEND_TYPE mdns_send_type) {
	int i = 0;
	for (i = 0; i < (int)ipp_min(FD_LIST_CNT, g_mdns_max_fd); ++i) {
		if (fd_list[i] >= 0 && FD_ISSET(fd_list[i], read_set)) {
			mdns_pkt *mdns;
			struct sockaddr_in fromaddr;
			socklen_t sockaddr_size = sizeof(struct sockaddr_in);
			int recvsize;
			uint8_t *pkt_buffer = (uint8_t*)malloc(PACKET_SIZE);
			if (NULL == pkt_buffer) {
				print_local_warnning(mdnsd_process_malloc_recv_buffer_failed);
				continue;
			}

			recvsize = recvfrom(fd_list[i], (char *)pkt_buffer, PACKET_SIZE, 0,
				(struct sockaddr *) &fromaddr, &sockaddr_size);
			if (recvsize > 0 && NULL != pkt_buffer) {
				mdns = mdns_parse_pkt(pkt_buffer, recvsize);
				if (mdns != NULL) {
					if (mdns->num_qn > 0)
					{
#if IPP_LIGHT_LOCAL_SERVER_ENABLE
						if (g_mdns_sock->init_type & ipp_device_server) {
							char *target = NULL;
							//����Ƿ���ˣ����ͻظ�
							if (process_mdns_pkt(g_mdns_sock, mdns, &target)) {
								send_response(fd_list, i, mdns_send_type);
							}
							FREE_POINTER(target);
						}
#endif
					} else if (mdns->num_ans_rr > 0) {
#if IPP_LIGHT_LOCAL_CLIENT_ENABLE
						if (g_mdns_sock->init_type & ipp_device_client) {
							char *target = NULL;
							uint16_t port = (uint16_t)process_mdns_pkt(g_mdns_sock, mdns, &target);
							//����յ��˶˿ڻظ�����client�˿���
							if (port && target) {
								fromaddr.sin_port = htons(port);
								client_start(&fromaddr, target);
							}
							FREE_POINTER(target);
						}
#endif
					} else {
						//ipp_LogV_Prefix("(no questions or answers in packet)\r\n");
					}

					mdns_pkt_destroy(&mdns);
				}
			}

			FREE_POINTER(pkt_buffer);
		}
	}
}

// main loop to receive, process and send out MDNS replies
// also handles MDNS service announces
//mdns���̴���
void mdnsd_process(fd_set *read_set) {
	mdnsd_process_one_fd_list(read_set, g_mdns_sock->mdns_fd, SEND_MDNS);
	mdnsd_process_one_fd_list(read_set, g_mdns_sock->ipp_mdns_fd, SEND_IPP_MDNS);
}

void init_question_model(mdnsd *svr)
{
	mutex_lock(&svr->data_lock);
	rr_qlist_append(&svr->rr_qn, rr_create_ptr(create_nlabel(SERVICES_NLABEL)));
	mutex_unlock(&svr->data_lock);
}

//mdns����ģ���׽��ֳ�ʼ��
int mdnsd_fd_init(int fd_list[FD_LIST_CNT], uint16_t mdns_port, const char *mdns_addr)
{
	if (g_mdns_max_fd >= 0) {
		fd_list[0] = create_mdns_sock(mdns_port, mdns_addr);
	} else {
		fd_list[0] = -1;
	}

	if (g_mdns_max_fd > 1) {
		fd_list[1] = create_mdns_sock(mdns_port, NULL);
	} else {
		fd_list[1] = -1;
	}

	if (fd_list[0] < 0 && fd_list[1] < 0) {
		return -1;
	}

	return 1;
}

//mdns����ģ���ʼ��
int mdnsd_init(ipp_device_type init_type) 
{
	int mdns_ret;
	int ipp_mdns_ret;

	enable_broadcast(FALSE);
	g_cur_send_time_out = 1;
	g_mdns_sock = (mdnsd *)malloc(sizeof(mdnsd));
	MALLOC_ERROR_RETURN_WTTH_RET(g_mdns_sock, -1)
		
	memset(g_mdns_sock, 0, sizeof(mdnsd));
	g_mdns_sock->init_type = init_type;
	
	mdns_ret = mdnsd_fd_init(g_mdns_sock->mdns_fd, MDNS_PORT, MDNS_ADDR);
	ipp_mdns_ret = mdnsd_fd_init(g_mdns_sock->ipp_mdns_fd, IPP_MDNS_PORT, IPP_MDNS_ADDR);
	if (mdns_ret < 0 && ipp_mdns_ret < 0) {
		FREE_POINTER(g_mdns_sock);
		return -1;
	}
	
	mutex_init(&g_mdns_sock->data_lock, NULL);
	init_question_model(g_mdns_sock);
	return 1;
}

//�ر�һ���׽���
void mdnsd_close_fd_list(int fd_list[FD_LIST_CNT])
{
	int i;
	for (i = 0; i < FD_LIST_CNT; ++i) {
		if (fd_list[i] >= 0)
			local_close_sock(fd_list[i]);
	}
}

//mdnsֹͣ
void mdnsd_stop() {
	if (NULL != g_mdns_sock) {
		mdnsd_close_fd_list(g_mdns_sock->mdns_fd);
		mdnsd_close_fd_list(g_mdns_sock->ipp_mdns_fd);

		mutex_free(&g_mdns_sock->data_lock);
		rr_qlist_destroy(&g_mdns_sock->rr_qn, 1);
		FREE_POINTER(g_mdns_sock);
	}
}

#endif
