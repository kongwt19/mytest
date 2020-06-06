#ifndef _MSG_MNG_H_
#define _MSG_MNG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"
#include <time.h>
#include <pthread.h>

#define MAX_MQ_SIZE 1024

typedef enum _msg_type
{
	MSG_CHILD_ONLINE = 0,
	MSG_CHILD_OFFLINE,
	MSG_UPDATE_INFO,
	MSG_ALARM,
	MSG_REPORT_STATUS,
	MSG_MULTI_REPORT_STATUS,
	MSG_REGIST,
	
}MSG_TYPE_E;

typedef struct mq_item {
	LIST_HEAD_T msg_head;
	uint8_t *msg_buf;
	int msg_size;
	char sn[SN_LEN];
	MSG_TYPE_E type;
	uint64_t msg_time;
	int retry_flag;
}MQ_ITEM_S;

typedef int (*MSG_PROCESS_UNIT)(uint8_t *buf, int size, char *sn, MSG_TYPE_E type);

typedef struct mq_mgr {
	LIST_HEAD_T mq_head;
	int mq_item_num;
	int max_item_num;
	MSG_PROCESS_UNIT mq_func;
	pthread_mutex_t mq_mutex;
	pthread_cond_t mq_cond;
	uint64_t tail_msg_timestamp;
}MQ_MGR_S;

/**
* Initialize a message queue
* @param size, max size of the queue
* @param msg_func, process messages
* @return 
*/
int init_msg_queue(int size, MSG_PROCESS_UNIT msg_func);
/**
* Add a message to the queue
* @param buf, message content
* @param size, message size
* @param sn, device serial of the message
* @param type, message type
* @return 
*/
int post_to_queue(uint8_t *buf, int size, char *sn, MSG_TYPE_E type);

#ifdef __cplusplus
}
#endif

#endif //_MSG_MNG_H_

