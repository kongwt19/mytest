/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : mdns核心功能
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _LOCAL_MDNS_CORE_H_
#define _LOCAL_MDNS_CORE_H_

#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const uint16_t TRASANCTION_ID = 0xFEDC;
static const uint8_t DEFAULT_TTL = 120;

#define MALLOC_ZERO_STRUCT(x, type) \
	x = (type *)malloc(sizeof(type)); \
	memset(x, 0, sizeof(type));

#define DECL_MALLOC_ZERO_STRUCT(x, type) \
	type * MALLOC_ZERO_STRUCT(x, type)

#define DECL_STRUCT(x, type) \
	type * x;


#define MDNS_FLAG_RESP 	(1 << 15)	// Query=0 / Response=1
#define MDNS_FLAG_AA	(1 << 10)	// Authoritative
#define MDNS_FLAG_TC	(1 <<  9)	// TrunCation
#define MDNS_FLAG_RD	(1 <<  8)	// Recursion Desired
#define MDNS_FLAG_RA	(1 <<  7)	// Recursion Available
#define MDNS_FLAG_Z		(1 <<  6)	// Reserved (zero)

#define MDNS_FLAG_GET_RCODE(x)	(x & 0x0F)
#define MDNS_FLAG_GET_OPCODE(x)	((x >> 11) & 0x0F)

// gets the PTR target name, either from "name" member or "entry" member
#define MDNS_RR_GET_PTR_NAME(rr)  (rr->data.PTR.name != NULL ? rr->data.PTR.name : rr->data.PTR.entry->name)

typedef enum {
	RR_A		= 0x01,
	RR_PTR		= 0x0C,
	RR_TXT		= 0x10,
	RR_AAAA		= 0x1C,
	RR_SRV		= 0x21,
	RR_NSEC		= 0x2F,
	RR_ANY		= 0xFF,
} rr_type;

typedef struct rr_data_srv {
	uint16_t priority;
	uint16_t weight;
	uint16_t port;
	uint8_t *target;	// host
}rr_data_srv;

typedef struct rr_data_txt {
	struct rr_data_txt *next;
	uint8_t *txt;
}rr_data_txt;

typedef struct rr_data_nsec {
	//uint8_t *name;	// same as record

	// NSEC occupies the 47th bit, 5 bytes
	//uint8_t bitmap_len;	// = 5
	uint8_t bitmap[5];	// network order: first byte contains LSB
}rr_data_nsec;

typedef struct rr_data_ptr {
	uint8_t *name;		// NULL if entry is to be used
    struct rr_entry *entry;
}rr_data_ptr;

typedef struct rr_data_a {
	uint32_t addr;
}rr_data_a;

typedef struct rr_data_aaaa {
	struct in6_addr *addr;
}rr_data_aaaa;

typedef struct rr_question {
	uint8_t *name;
	rr_type type;
	uint16_t rr_class;
	char qu;
}rr_question;

typedef struct rr_qlist {
    rr_question *qn;
	struct rr_qlist *next;
}rr_qlist;

typedef struct rr_entry {
	uint8_t *name;
	rr_type type;
	uint32_t ttl;
	// for use in Questions only
	char unicast_query;
	// for use in Answers only
	char cache_flush;
	uint16_t rr_class;
	// RR data
	union {
		rr_data_nsec NSEC;
		rr_data_srv  SRV;
		rr_data_txt  TXT;
		rr_data_ptr  PTR;
		rr_data_a    A;
		rr_data_aaaa AAAA;
	} data;
}rr_entry;

typedef struct rr_list {
	rr_entry *e;
	struct rr_list *next;
}rr_list;

typedef struct mdns_pkt {
	uint16_t id;	// transaction ID
	uint16_t flags;
	uint16_t num_qn;
	uint16_t num_ans_rr;
	uint16_t num_auth_rr;
	uint16_t num_add_rr;

	rr_qlist *rr_qn;		// questions
 	rr_list *rr_ans;		// answer RRs
 	rr_list *rr_auth;	// authority RRs
	rr_list *rr_add;		// additional RRs
}mdns_pkt;

mdns_pkt *mdns_parse_pkt(uint8_t *pkt_buf, size_t pkt_len);

void mdns_init_reply(mdns_pkt *pkt, uint16_t id);
size_t mdns_encode_pkt(mdns_pkt *answer, uint8_t *pkt_buf, size_t pkt_len);

void mdns_pkt_destroy(mdns_pkt **p);
rr_question *rr_question_find(rr_qlist *rr_list, uint8_t *name);

int rr_list_append(rr_list **rr_head, rr_entry *rr);
int rr_qlist_append(rr_qlist **rr_head, rr_question *rr);
rr_entry *rr_list_remove(rr_list **rr_head, rr_entry *rr);
void rr_list_destroy(rr_list **rr, char destroy_items);
void rr_qlist_destroy(rr_qlist **rr, char destroy_items);

rr_question *rr_create_ptr(uint8_t *name);
rr_entry *rr_create_srv(uint8_t *name, uint16_t port, uint8_t *target);

const char *rr_get_type_name(rr_type type);

uint8_t *create_label(const char *txt);
uint8_t *create_nlabel(const char *name);
char *nlabel_to_str(const uint8_t *name);
uint8_t *dup_label(const uint8_t *label);

// compares 2 names
int cmp_nlabel(const uint8_t *L1, const uint8_t *L2);

#endif

#endif
