#include "defines.h"
#include "error_code.h"
#include "msg_mng.h"

static MQ_MGR_S msg_queue;

void *msg_process_func(void *arg);

void mq_lock(void)
{
	pthread_mutex_lock(&msg_queue.mq_mutex);
}

void mq_unlock(void)
{
	pthread_mutex_unlock(&msg_queue.mq_mutex);
}

/**
* Initialize a message queue
* @param size, max size of the queue
* @param msg_func, process messages
* @return 
*/
int init_msg_queue(int size, MSG_PROCESS_UNIT msg_func)
{
	RETURN_IF_NULL(msg_func, GW_NULL_PARAM);
	
	memset(&msg_queue, 0, sizeof(msg_queue));
	msg_queue.mq_item_num = 0;
	msg_queue.max_item_num = (size > MAX_MQ_SIZE)?MAX_MQ_SIZE:size;
	msg_queue.mq_func = msg_func;
	INIT_LIST_HEAD(&msg_queue.mq_head);
	if(-1 == pthread_mutex_init(&msg_queue.mq_mutex, 0))
	{
		return GW_ERR;
	}
	if(-1 == pthread_cond_init(&msg_queue.mq_cond, 0))
	{
		pthread_mutex_destroy(&msg_queue.mq_mutex);
		return GW_ERR;
	}

	pthread_t pid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&pid, &attr, msg_process_func,(void*)&msg_queue);
	//pthread_create(&pid, &attr, msg_process_func,(void*)&msg_queue);
	
	LogI_Prefix("Message queue initialized to max item number %d\n", msg_queue.max_item_num);
	return GW_OK;
}

int add_msg(MQ_ITEM_S *item)
{
	int ret = GW_ERR;

	RETURN_IF_NULL(item, GW_NULL_PARAM);
	
	mq_lock();
	if(msg_queue.mq_item_num < msg_queue.max_item_num)
	{
		list_add(&item->msg_head, &msg_queue.mq_head);
		msg_queue.mq_item_num ++;
		ret = GW_OK;
		LogI_Prefix("Add item to queue, %d in queue\n", msg_queue.mq_item_num);
	}
	mq_unlock();

	return ret;
}


/**
* Add a message to the queue
* @param buf, message content
* @param size, message size
* @param sn, device serial of the message
* @param type, message type
* @return 
*/
int post_to_queue(uint8_t *buf, int size, char *sn, MSG_TYPE_E type)
{
	int ret = GW_ERR;
	
	if(NULL == buf || NULL == sn || msg_queue.mq_item_num >= msg_queue.max_item_num)
	{
		LogE_Prefix("Content is NULL or queue is full\n");
		return GW_ERR;
	}

	MQ_ITEM_S *item = NULL;

	item = (MQ_ITEM_S*)malloc(sizeof(MQ_ITEM_S));
	RETURN_IF_NULL(item, GW_NULL_PARAM);
	memset(item, 0, sizeof(MQ_ITEM_S));

	item->msg_buf = malloc(size + 1);
	if(NULL == item->msg_buf)
	{
		FREE_POINTER(item);
		return GW_ERR;
	}
	memset(item->msg_buf, 0, size + 1);
	memcpy(item->msg_buf, buf, size);
	strncpy(item->sn, sn, SN_LEN);
	item->type = type;
	item->msg_size = size;
	item->msg_time = time(NULL);//Record timestamp
	INIT_LIST_HEAD(&item->msg_head);

	ret = add_msg(item);

	return ret;
}

/**
* Get a message from the queue
* @param 
* @return 
*/
MQ_ITEM_S *dequeue_msg(void)
{
	MQ_ITEM_S *item = NULL;

	mq_lock();
	if(!list_empty(&msg_queue.mq_head))
	{
		//Get the first item from queue
		item = list_entry(msg_queue.mq_head.prev, MQ_ITEM_S, msg_head);
		list_del(&item->msg_head);
		msg_queue.mq_item_num --;
		LogI_Prefix("Del item from queue, %d in queue\n", msg_queue.mq_item_num);
	}
	mq_unlock();

	return item;
}

void free_mq_item(MQ_ITEM_S *item)
{
	if(NULL != item)
	{
		if(NULL != item->msg_buf)
		{
			free(item->msg_buf);
		}
		free(item);
	}
}
void *msg_process_func(void *arg)
{
	if(NULL == arg)
	{
		return NULL;
	}

	LogI_Prefix("Message queue process thread started\n");

	MQ_MGR_S *mq = (MQ_MGR_S*)arg;
	MQ_ITEM_S *item = NULL;
	while(1)
	{
		item = dequeue_msg();
		if(NULL != item)
		{
			if(GW_OK == mq->mq_func(item->msg_buf, item->msg_size, item->sn, item->type))
			{
				free_mq_item(item);
				item = NULL;
			}
			else
			{
				add_msg(item);
			}
		}
		usleep(500 * 1000);
	}
}

/*int main(int argc, char *argv[])
{
	int i = 0;
	uint8_t buf[10];
	init_msg_queue(10, msg_handle);
	start_msg_queue();

	while(1)
	{
		memset(buf, 0x41 + i, sizeof(buf) - 1);
		if(-1 == post_to_queue(buf, sizeof(buf), "1234", MSG_CHILD_ONLINE))
		{
			;
		}
		else
		{
			i ++;
		}
		usleep(100 * 1000);
	}
	return 0;
}*/

