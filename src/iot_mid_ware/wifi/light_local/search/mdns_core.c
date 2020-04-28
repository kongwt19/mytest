#include "local_module_enable.h"
#if (IPP_LIGHT_LOCAL_SERVER_ENABLE || IPP_LIGHT_LOCAL_CLIENT_ENABLE)
#include "mdns_core.h"
#include "local_error_code.h"
#include "ipp_defines.h"
#include "platform.h"

typedef struct name_comp {
	uint8_t *label;	// label
	size_t pos;		// position in msg

	struct name_comp *next;
}name_comp;

// ----- label functions -----
// duplicates a label
uint8_t *dup_label(const uint8_t *label) {
	int len = *label + 1;
	uint8_t *newlabel;
	if (len > 63)
		return NULL;
	newlabel = (uint8_t *)malloc(len + 1);
	MALLOC_ERROR_RETURN_WTTH_RET(newlabel, NULL)

	strncpy((char *) newlabel, (char *) label, len);
	newlabel[len] = '\0';
	return newlabel;
}

// returns a human-readable name label in dotted form
char *nlabel_to_str(const uint8_t *name) {
	char *label, *labelp;
	const uint8_t *p;
	int idx = 0;
	int len = (int)strlen((char *)name);
	if (len <= 0 || NULL == name)
		return NULL;

	label = (char *)malloc(len);
	MALLOC_ERROR_RETURN_WTTH_RET(label, NULL)
	memset(label, '\0', len);

	labelp = label;
	for (p = name; *p; p++) {
		int block_len = *p;
		if (block_len > len - idx - 1)
			break;

		strncpy(labelp, (char *) p + 1, block_len);
		labelp += block_len;
		*labelp = '.';
		labelp++;

		p += block_len;
		idx += block_len;
	}

	*(label + len - 1) = '\0';
	return label;
}

// returns the length of a label field
// does NOT uncompress the field, so it could be as small as 2 bytes
// or 1 for the root
static size_t label_len(uint8_t *pkt_buf, size_t pkt_len, size_t off) {
	uint8_t *p;
	uint8_t *e = pkt_buf + pkt_len;
	size_t len = 0;

	for (p = pkt_buf + off; p < e; p++) {
		if (*p == 0) {
			return len + 1;
		} else if ((*p & 0xC0) == 0xC0) {
			return len + 2;
		} else {
			len += *p + 1;
			p += *p;
		}
	}

	return len;
}

// creates a label
uint8_t *create_label(const char *txt) {
	int len;
	uint8_t *s;

	if (NULL == txt)
		return NULL;

	len = (int)strlen(txt);

	s = (uint8_t *)malloc(len + 2);
	MALLOC_ERROR_RETURN_WTTH_RET(s, NULL)

	s[0] = (uint8_t)len;
	strncpy((char *) s + 1, txt, len);
	s[len + 1] = '\0';

	return s;
}

// creates a uncompressed name label given a DNS name like "apple.b.com"
uint8_t *create_nlabel(const char *name) {
	char *label;
	char *p, *e, *lenpos;
	int len = 0;

	if (NULL == name)
		return NULL;

	len = (int)strlen(name);
	label = (char *)malloc(len + 1 + 1);
	MALLOC_ERROR_RETURN_WTTH_RET(label, NULL)

	strncpy((char *) label + 1, name, len);

	p = label;
	e = p + len;
	lenpos = p;

	while (p < e) {
		char *dot = (char *)memchr(p + 1, '.', e - p - 1);
		*lenpos = 0;

		if (dot == NULL)
			dot = e + 1;
		*lenpos = (char)(dot - p - 1);

		p = dot;
		lenpos = dot;
	}

	label[len + 1] = '\0';
	return (uint8_t *) label;
}

// copies a label from the buffer into a newly-allocated string
static uint8_t *copy_label(uint8_t *pkt_buf, size_t pkt_len, size_t off) {
	int len;

	if (off > pkt_len)
		return NULL;
	
	len = pkt_buf[off] + 1;
	if (off + len > pkt_len) {
		print_local_error(copy_label_length_exceeds_buffer);
		return NULL;
	}

	return dup_label(pkt_buf + off);
}

// uncompresses a name
static uint8_t *uncompress_nlabel(uint8_t *pkt_buf, size_t pkt_len, size_t off) {
	uint8_t *p;
	uint8_t *e = pkt_buf + pkt_len;
	size_t len = 0;
	char *str, *sp;
	if (off >= pkt_len)
		return NULL;

	// calculate length of uncompressed label
	for (p = pkt_buf + off; *p && p < e; p++) {
		size_t llen = 0;
		if ((*p & 0xC0) == 0xC0) {
			uint8_t *p2 = pkt_buf + (((p[0] & ~0xC0) << 8) | p[1]);
			llen = *p2 + 1;
			p = p2 + llen - 1;
		} else {
			llen = *p + 1;
			p += llen - 1;
		}
		len += llen;
	}

	str = sp = (char *)malloc(len + 1);
	MALLOC_ERROR_RETURN_WTTH_RET(str, NULL)

	// FIXME: must merge this with above code
	for (p = pkt_buf + off; *p && p < e; p++) {
		size_t llen = 0;
		if ((*p & 0xC0) == 0xC0) {
			uint8_t *p2 = pkt_buf + (((p[0] & ~0xC0) << 8) | p[1]);
			llen = *p2 + 1;
			strncpy(sp, (char *) p2, llen);
			p = p2 + llen - 1;
		} else {
			llen = *p + 1;
			strncpy(sp, (char *) p, llen);
			p += llen - 1;
		}
		sp += llen;
	}
	*sp = '\0';

	return (uint8_t *) str;
}

int cmp_nlabel(const uint8_t *L1, const uint8_t *L2) {
	return strcmp((char *) L1, (char *) L2);
}

// ----- RR list & group functions -----
void rr_quetion_destroy(rr_question **rr) {
	if (NULL == *rr)
		return;

	FREE_POINTER((*rr)->name);
	FREE_POINTER(*rr);
}

void rr_entry_destroy(rr_entry **rr) {
	if (NULL == *rr)
		return;

	// check rr_type and free data elements
	switch ((*rr)->type) {
		case RR_PTR: {
				FREE_POINTER((*rr)->data.PTR.name);
				if (NULL != (*rr)->data.PTR.entry)
					rr_entry_destroy(&(*rr)->data.PTR.entry);
			}
			break;

		case RR_TXT: {
				rr_data_txt *txt_rec = &(*rr)->data.TXT;
				while (txt_rec) {
					rr_data_txt *next = txt_rec->next;
					FREE_POINTER(txt_rec->txt);

					// if it wasn't part of the struct
					if (txt_rec != &(*rr)->data.TXT)
						FREE_POINTER(txt_rec);

					txt_rec = next;
				}
			}
			break;

		case RR_SRV:
			FREE_POINTER((*rr)->data.SRV.target);
			break;

		case RR_AAAA:
			FREE_POINTER((*rr)->data.AAAA.addr);
			break;

		default:
			// nothing to free
			break;
	}

	FREE_POINTER((*rr)->name);
	FREE_POINTER((*rr));
}

// destroys an RR list (and optionally, items)
void rr_qlist_destroy(rr_qlist **rr, char destroy_items) {
	rr_qlist *rr_next;

	for (; *rr; *rr = rr_next) {
		rr_next = (*rr)->next;
		if (destroy_items)
			rr_quetion_destroy(&(*rr)->qn);
		FREE_POINTER(*rr);
	}
}

// destroys an RR list (and optionally, items)
void rr_list_destroy(rr_list **rr, char destroy_items) {
	rr_list *rr_next;

	for (; *rr; *rr = rr_next) {
		rr_next = (*rr)->next;
		if (destroy_items)
			rr_entry_destroy(&(*rr)->e);
		FREE_POINTER(*rr);
	}
}

rr_entry *rr_list_remove(rr_list **rr_head, rr_entry *rr) {
	rr_list *le = *rr_head, *pe = NULL;
	for (; le; le = le->next) {
		if (le->e == rr) {
			if (pe == NULL) {
				*rr_head = le->next;
				FREE_POINTER(le);
				return rr;
			} else {
				pe->next = le->next;
				FREE_POINTER(le);
				return rr;
			}
		}
		pe = le;
	}
	return NULL;
}

// appends an rr_entry to an RR list
// if the RR is already in the list, it will not be added
// RRs are compared by memory location - not its contents
// return value of 0 means item not added
int rr_list_append(rr_list **rr_head, rr_entry *rr) {
	rr_list *node = (rr_list *)malloc(sizeof(rr_list));
	MALLOC_ERROR_RETURN_WTTH_RET(node, -1)

	node->e = rr;
	node->next = NULL;

	if (*rr_head == NULL) {
		*rr_head = node;
	} else {
		rr_list *e = *rr_head, *tail_e = NULL;
		for (; e; e = e->next) {
			// already in list - don't add
			if (e->e == rr) {
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

#define FILL_RR_ENTRY(rr, _name, _type)	\
	rr->name = _name;			\
	rr->type = (rr_type)_type;			\
	rr->ttl  = DEFAULT_TTL;		\
	rr->cache_flush = 1;		\
	rr->rr_class  = 1;

rr_entry *rr_create_srv(uint8_t *name, uint16_t port, uint8_t *target) {
	DECL_MALLOC_ZERO_STRUCT(rr, rr_entry);
	FILL_RR_ENTRY(rr, name, RR_SRV);
	rr->data.SRV.port = port;
	rr->data.SRV.target = target;
	return rr;
}

rr_question *rr_create_ptr(uint8_t *name) {
	DECL_MALLOC_ZERO_STRUCT(rr, rr_question);
	rr->name = name;
	rr->type = (rr_type)RR_PTR;
	rr->rr_class = 1;
	rr->qu = 1;
	return rr;
}

rr_question *rr_question_find(rr_qlist *rr_list, uint8_t *name) {
	rr_qlist *rr = rr_list;
	for (; rr; rr = rr->next) {
		if (cmp_nlabel(rr->qn->name, name) == 0) 
			return rr->qn;
	}
	return NULL;
}


uint8_t *mdns_write_u16(uint8_t *ptr, const uint16_t v) {
	*ptr++ = (uint8_t) (v >> 8) & 0xFF;
	*ptr++ = (uint8_t) (v >> 0) & 0xFF;
	return ptr;
}

uint8_t *mdns_write_u32(uint8_t *ptr, const uint32_t v) {
	*ptr++ = (uint8_t) (v >> 24) & 0xFF;
	*ptr++ = (uint8_t) (v >> 16) & 0xFF;
	*ptr++ = (uint8_t) (v >>  8) & 0xFF;
	*ptr++ = (uint8_t) (v >>  0) & 0xFF;
	return ptr;
}

uint16_t mdns_read_u16(const uint8_t *ptr) {
	return  ((ptr[0] & 0xFF) << 8) | 
			((ptr[1] & 0xFF) << 0);
}

uint32_t mdns_read_u32(const uint8_t *ptr) {
	return  ((ptr[0] & 0xFF) << 24) | 
			((ptr[1] & 0xFF) << 16) | 
			((ptr[2] & 0xFF) <<  8) | 
			((ptr[3] & 0xFF) <<  0);
}

// initialize the packet for reply
// clears the packet of list structures but not its list items
void mdns_init_reply(mdns_pkt *pkt, uint16_t id) {
	// copy transaction ID
	pkt->id = id;

	// response flags
	pkt->flags = MDNS_FLAG_RESP | MDNS_FLAG_AA;

	rr_qlist_destroy(&pkt->rr_qn,   0);
	rr_list_destroy(&pkt->rr_ans,  0);
	rr_list_destroy(&pkt->rr_auth, 0);
	rr_list_destroy(&pkt->rr_add,  0);

	pkt->rr_qn    = NULL;
	pkt->rr_ans   = NULL;
	pkt->rr_auth  = NULL;
	pkt->rr_add   = NULL;

	pkt->num_qn = 0;
	pkt->num_ans_rr = 0;
	pkt->num_auth_rr = 0;
	pkt->num_add_rr = 0;
}

// destroys an mdns_pkt struct, including its contents
void mdns_pkt_destroy(mdns_pkt **p) {
	rr_qlist_destroy(&(*p)->rr_qn, 1);
	rr_list_destroy(&(*p)->rr_ans, 1);
	rr_list_destroy(&(*p)->rr_auth, 1);
	rr_list_destroy(&(*p)->rr_add, 1);
	FREE_POINTER(*p);
}

int rr_qlist_append(rr_qlist **rr_head, rr_question *rr)
{
	rr_qlist *node = (rr_qlist *)malloc(sizeof(rr_qlist));
	MALLOC_ERROR_RETURN_WTTH_RET(node, -1)

	node->qn = rr;
	node->next = NULL;

	if (*rr_head == NULL) {
		*rr_head = node;
	} else {
		rr_qlist *e = *rr_head, *tail_e = NULL;
		for (; e; e = e->next) {
			// already in list - don't add
			if (e->qn == rr) {
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

// parse the MDNS questions section
// stores the parsed data in the given mdns_pkt struct
static size_t mdns_parse_qn(uint8_t *pkt_buf, size_t pkt_len, size_t off, 
		mdns_pkt *pkt) {
	const uint8_t *p = pkt_buf + off;
	rr_question *rr;
	uint8_t *name;
	uint16_t calss_qu;
   
	if (NULL == pkt)
		return 0;

	rr = (rr_question *)malloc(sizeof(rr_question)); 
	MALLOC_ERROR_RETURN_WTTH_RET(rr, 0)

	memset(rr, 0, sizeof(rr_question));

	name = uncompress_nlabel(pkt_buf, pkt_len, off);
	p += label_len(pkt_buf, pkt_len, off);
	rr->name = name;

	rr->type = (rr_type)mdns_read_u16(p);
	p += sizeof(uint16_t);

	calss_qu = mdns_read_u16(p);
	rr->rr_class = calss_qu & 0x7fff;
	rr->qu = calss_qu & 0x1;
	p += sizeof(uint16_t);

	rr_qlist_append(&pkt->rr_qn, rr);
	
	return p - (pkt_buf + off);
}

// parse the MDNS RR section
// stores the parsed data in the given mdns_pkt struct
static size_t mdns_parse_rr(uint8_t *pkt_buf, size_t pkt_len, size_t off, 
		mdns_pkt *pkt) {
	const uint8_t *p = pkt_buf + off;
	const uint8_t *e = pkt_buf + pkt_len;
	rr_entry *rr;
	uint8_t *name;
	size_t rr_data_len = 0;
	rr_data_txt *txt_rec;
	int parse_error = 0;

	if (NULL == pkt)
		return 0;

	if (off > pkt_len)
		return 0;

	rr = (rr_entry *)malloc(sizeof(rr_entry)); 
	MALLOC_ERROR_RETURN_WTTH_RET(rr, 0)

	memset(rr, 0, sizeof(rr_entry));

	name = uncompress_nlabel(pkt_buf, pkt_len, off);
	p += label_len(pkt_buf, pkt_len, off);
	rr->name = name;

	rr->type = (rr_type)mdns_read_u16(p);
	p += sizeof(uint16_t);

	rr->cache_flush = (*p & 0x80) == 0x80;
	rr->rr_class = mdns_read_u16(p) & ~0x80;
	p += sizeof(uint16_t);

	rr->ttl = mdns_read_u32(p);
	p += sizeof(uint32_t);

	// RR data
	rr_data_len = mdns_read_u16(p);
	p += sizeof(uint16_t);

	if (rr_data_len > (size_t)(e - p)) {
		//ipp_LogV_Prefix("rr_data_len goes beyond packet buffer: %lu > %lu\r\n", rr_data_len, e - p);
		rr_entry_destroy(&rr);
		return 0;
	}

	e = p + rr_data_len;

	// see if we can parse the RR data
	switch (rr->type) {
		case RR_A: {
				if (rr_data_len < sizeof(uint32_t)) {
					print_local_error(mdns_parse_rr_invalid_rr_data_len);
					parse_error = 1;
					break;
				}
				rr->data.A.addr = ntohl(mdns_read_u32(p)); /* addr already in net order */
				p += sizeof(uint32_t);
			}
			break;

		case RR_AAAA: 
			break;
		case RR_SRV:{
				rr->data.SRV.priority = mdns_read_u16(p);
				p += sizeof(uint16_t);

				rr->data.SRV.weight = mdns_read_u16(p);
				p += sizeof(uint16_t);

				rr->data.SRV.port = mdns_read_u16(p);
				p += sizeof(uint16_t);

				rr->data.SRV.target = (uint8_t *)malloc(rr_data_len - sizeof(uint16_t) * 3);
				if (NULL == rr->data.SRV.target) {
					print_local_error(mdns_parse_rr_data_SRV_target);
					parse_error = 1;
					break;
				}

				strncpy((char *)rr->data.SRV.target, (char *)p, rr_data_len - sizeof(uint16_t) * 3);
				p += sizeof(uint16_t);
			}
			break;

		case RR_PTR: {
				rr->data.PTR.name = uncompress_nlabel(pkt_buf, pkt_len, p - pkt_buf);
				if (rr->data.PTR.name == NULL) {
					print_local_error(mdns_parse_rr_unable_to_parse);
					parse_error = 1;
					break;
				}
				p += rr_data_len;
			}
			break;

		case RR_TXT: {
				txt_rec = &rr->data.TXT;

				// not supposed to happen, but we should handle it
				if (rr_data_len == 0) {
					print_local_error(mdns_parse_rr_rr_data_len_is0);
					txt_rec->txt = create_label("");
					break;
				}

				while (1) {
					txt_rec->txt = copy_label(pkt_buf, pkt_len, p - pkt_buf);
					if (txt_rec->txt == NULL) {
						print_local_error(mdns_parse_rr_unable_to_copy_label);
						parse_error = 1;
						break;
					}
					p += txt_rec->txt[0] + 1;

					if (p >= e) 
						break;

					// allocate another record
					txt_rec->next = (rr_data_txt *)malloc(sizeof(rr_data_txt));
					if (NULL == txt_rec->next) {
						print_local_error(mdns_parse_rr_txt_re_next);
						parse_error = 1;
						break;
					}

					txt_rec = txt_rec->next;
					txt_rec->next = NULL;
				}
			}
			break;

		default:
			// skip to end of RR data
			p = e;
	}

	// if there was a parse error, destroy partial rr_entry
	if (parse_error) {
		rr_entry_destroy(&rr);
		return 0;
	}

	rr_list_append(&pkt->rr_ans, rr);
	
	return p - (pkt_buf + off);
}

// parse a MDNS packet into an mdns_pkt struct
mdns_pkt *mdns_parse_pkt(uint8_t *pkt_buf, size_t pkt_len) {
	uint8_t *p = pkt_buf;
	size_t off;
	mdns_pkt *pkt;
	int i;

	if (pkt_len < 12) 
		return NULL;

	pkt = (mdns_pkt *)malloc(sizeof(mdns_pkt));
	MALLOC_ERROR_RETURN_WTTH_RET(pkt, NULL)
	memset(pkt, 0, sizeof(mdns_pkt));

	// parse header
	pkt->id 			= mdns_read_u16(p); p += sizeof(uint16_t);
	pkt->flags 			= mdns_read_u16(p); p += sizeof(uint16_t);
	pkt->num_qn 		= mdns_read_u16(p); p += sizeof(uint16_t);
	pkt->num_ans_rr 	= mdns_read_u16(p); p += sizeof(uint16_t);
	pkt->num_auth_rr 	= mdns_read_u16(p); p += sizeof(uint16_t);
	pkt->num_add_rr 	= mdns_read_u16(p); p += sizeof(uint16_t);

	if (TRASANCTION_ID != pkt->id) {
		mdns_pkt_destroy(&pkt);
		return NULL;
	}
	off = p - pkt_buf;

	// parse questions
	for (i = 0; i < pkt->num_qn; i++) {
		size_t l = mdns_parse_qn(pkt_buf, pkt_len, off, pkt);
		if (0 == l) {
			//ipp_LogV_Prefix("error parsing question #%d\r\n", i);
			mdns_pkt_destroy(&pkt);
			return NULL;
		}

		off += l;
	}

	// parse answer RRs
	for (i = 0; i < pkt->num_ans_rr; i++) {
		size_t l = mdns_parse_rr(pkt_buf, pkt_len, off, pkt);
		if (0 == l) {
			//ipp_LogV_Prefix("error parsing answer #%d\r\n", i);
			mdns_pkt_destroy(&pkt);
			return NULL;
		}

		off += l;
	}

	// TODO: parse the authority and additional RR sections

	return pkt;
}

// encodes a name (label) into a packet using the name compression scheme
// encoded names will be added to the compression list for subsequent use
static size_t mdns_encode_name(uint8_t *pkt_buf, size_t pkt_len, size_t off,
		const uint8_t *name, name_comp *comp) {
	name_comp *c, *c_tail = NULL;
	uint8_t *p = pkt_buf + off;
	size_t len = 0;

	if (name) {
		while (*name) {
			int segment_len;
			// cache the name for subsequent compression
			DECL_STRUCT(new_c, name_comp);

			// find match for compression
			for (c = comp; c; c = c->next) {
				if (cmp_nlabel(name, c->label) == 0) {
					mdns_write_u16(p, 0xC000 | (uint16_t)(c->pos & ~0xC000));
					return len + sizeof(uint16_t);
				}

				if (c->next == NULL)
					c_tail = c;
			}

			// copy this segment
			segment_len = *name + 1;
			strncpy((char *) p, (char *) name, segment_len);

			new_c = (name_comp *)malloc(sizeof(name_comp));
			MALLOC_ERROR_RETURN_WTTH_RET(new_c, 0)
			memset(new_c, 0, sizeof(name_comp));

			new_c->label = (uint8_t *) name;
			new_c->pos = p - pkt_buf;

			if (NULL != c_tail)
				c_tail->next = new_c;

			// advance to next name segment
			p += segment_len;
			len += segment_len;
			name += segment_len;
		}
	}

	*p = '\0';	// root "label"
	len += 1;

	return len;
}

// encodes an question entry at the given offset
// returns the size of the entire RR entry
static size_t mdns_encode_qn(uint8_t *pkt_buf, size_t pkt_len, size_t off, 
	rr_question *rr, name_comp *comp) 
{
	size_t l;
	uint8_t *p = pkt_buf + off;
	uint16_t classVal = (rr->rr_class & 0x7fff);
	uint16_t qu = (rr->qu & 0x1);

	if (off >= pkt_len)
		return 0;

	// name
	l = strlen((char *)rr->name);
	if (l <= 0)
		return 0;

	strncpy((char *)p, (char *)rr->name, l);
	p += l;
	*p = '\0';
	p++;

	// type
	p = mdns_write_u16(p, rr->type);

	// class & qu
	classVal = (rr->rr_class & 0x7fff);
	qu = (rr->qu & 0x1);
	p = mdns_write_u16(p, (qu << 15) | classVal);

	return p - pkt_buf - off;
}

// encodes an RR entry at the given offset
// returns the size of the entire RR entry
static size_t mdns_encode_rr(uint8_t *pkt_buf, size_t pkt_len, size_t off, 
		rr_entry *rr, name_comp *comp) {
	uint8_t *p = pkt_buf + off, *p_data;
	size_t l;
	rr_data_txt *txt_rec;
	uint8_t *label;
	int i;

	if (off >= pkt_len)
		return 0;

	// name
	l = mdns_encode_name(pkt_buf, pkt_len, off, rr->name, comp);
	if (l <= 0)
		return 0;

	p += l;

	// type
	p = mdns_write_u16(p, rr->type);

	// class & cache flush
	p = mdns_write_u16(p, (rr->rr_class & ~0x8000) | (rr->cache_flush << 15));

	// TTL
	p = mdns_write_u32(p, rr->ttl);
	
	// data length (filled in later)
	p += sizeof(uint16_t);

	// start of data marker
	p_data = p;

	switch (rr->type) {
		case RR_A:
			/* htonl() needed coz addr already in net order */
			p = mdns_write_u32(p, htonl(rr->data.A.addr));
			break;

		case RR_AAAA:
			break;

		case RR_PTR:
			label = MDNS_RR_GET_PTR_NAME(rr);
			p += mdns_encode_name(pkt_buf, pkt_len, p - pkt_buf, label, comp);
			break;

		case RR_TXT:
			txt_rec = &rr->data.TXT;
			for (; txt_rec; txt_rec = txt_rec->next) {
				int len = txt_rec->txt[0] + 1;
				strncpy((char *) p, (char *) txt_rec->txt, len);
				p += len;
			}
			break;

		case RR_SRV:
			p = mdns_write_u16(p, rr->data.SRV.priority);
			
			p = mdns_write_u16(p, rr->data.SRV.weight);

			p = mdns_write_u16(p, rr->data.SRV.port);

			p += mdns_encode_name(pkt_buf, pkt_len, p - pkt_buf, 
					rr->data.SRV.target, comp);
			break;

		case RR_NSEC:
			p += mdns_encode_name(pkt_buf, pkt_len, p - pkt_buf, 
					rr->name, comp);

			*p++ = 0;	// bitmap window/block number

			*p++ = sizeof(rr->data.NSEC.bitmap);	// bitmap length

			for (i = 0; i < sizeof(rr->data.NSEC.bitmap); i++) 
				*p++ = rr->data.NSEC.bitmap[i];

			break;

		default:
			ipp_LogV_Prefix("%d:type 0x%02x\r\n", mdns_encode_rr_unhandled_rr_type, rr->type);
	}

	// calculate data length based on p
	l = p - p_data;

	// fill in the length
	mdns_write_u16(p - l - sizeof(uint16_t), (uint16_t)l);

	return p - pkt_buf - off;
}

// encodes a MDNS packet from the given mdns_pkt struct into a buffer
// returns the size of the entire MDNS packet
size_t mdns_encode_pkt(mdns_pkt *answer, uint8_t *pkt_buf, size_t pkt_len) {
	name_comp *comp;
	uint8_t *p = pkt_buf;
	//uint8_t *e = pkt_buf + pkt_len;
	size_t off;
	int i;
	rr_list *rr_set[3];

	if (NULL == answer)
		return 0;

	if (pkt_len < 12)
		return 0;

	if (p == NULL)
		return 0;

	p = mdns_write_u16(p, answer->id);
	p = mdns_write_u16(p, answer->flags);
	p = mdns_write_u16(p, answer->num_qn);
	p = mdns_write_u16(p, answer->num_ans_rr);
	p = mdns_write_u16(p, answer->num_auth_rr);
	p = mdns_write_u16(p, answer->num_add_rr);

	off = p - pkt_buf;

	// allocate list for name compression
	comp = (name_comp *)malloc(sizeof(name_comp));
	MALLOC_ERROR_RETURN_WTTH_RET(comp, 0)

	memset(comp, 0, sizeof(name_comp));

	// dummy entry
	comp->label = (uint8_t *) "";
	comp->pos = 0;

	//encoding of qn
	off += answer->rr_qn ? mdns_encode_qn(pkt_buf, pkt_len, off, answer->rr_qn->qn, comp) : 0;

	//encoding of others
	rr_set[0] =	answer->rr_ans;
	rr_set[1] = answer->rr_auth;
	rr_set[2] =	answer->rr_add;

	// encode answer, authority and additional RRs
	for (i = 0; i < sizeof(rr_set) / sizeof(rr_set[0]); i++) {
		rr_list *rr = rr_set[i];
		for (; rr; rr = rr->next) {
			size_t l = mdns_encode_rr(pkt_buf, pkt_len, off, rr->e, comp);
			off += l;

			if (off >= pkt_len) {
				print_local_error(mdns_encode_pkt_packet_buffer_too_small);
				return 0;
			}
		}

	}

	// free name compression list
	while (comp) {
		name_comp *c = comp->next;
		FREE_POINTER(comp);
		comp = c;
	}

	return off;
}

#endif