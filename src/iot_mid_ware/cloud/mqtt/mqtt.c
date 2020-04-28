#include <string.h>
#include <stdarg.h>

#include "platform.h"
#include "mqtt.h"
#include "pthread.h"
#include "iot_mid_ware.h"
#include "dev_mng.h"
#include "biz_plf.h"

// only for while cycle;
#define MALLOC(ptr, size, type) \
ptr = ( (type *) malloc( (size)* sizeof(type) ) ); \
if( NULL == ptr) 		\
{                        \
   LogE_Prefix("Error:(%d)-%s:%s malloc failed\r\n", __LINE__, __FUNCTION__, #ptr);\
}                        \
memset(ptr , 0 , size )

#define SOCKET_READ_TIMEOUT_MS  200
#define MSG_JSON_LEN         512

#define QUEUE_IS_FULL(q)     ((((q)->head+1)%PUBLISH_QUEUE_SIZE) == (q)->tail)
#define QUEUE_IS_EMPTY(q)    ((q)->head == (q)->tail)

static BOOL quit(MqttQuitNow * f)
{
    if(f)
    {
        return (*f)();
    }
    else
    {
        return FALSE;
    }
}

/* flash is not enough, NO CERT */
#define HEART_BEAT_INTERVAL  30      
#define PUBLISH_QUEUE_SIZE      8

static char *linker_sn = NULL;
static int   linker_sn_len =  0;
static int  isLinkerClosed = 0;

static const int QOS_reportStatus = 1;
static const int QOS_msgReceipt   = 1;
static const int QOS_msgMissing   = 1;

int_t g_msgid = 0;

static char *msgReceiptTopic = NULL;
static char *devStatusTopic  = NULL;
static char *msgMissingTopic  = "d/miss/m";
static MSG_PROC_FUNC msg_proc_func = NULL;
static pthread_t mqtt_pid;

//need get token
char *g_mqttToken = NULL;

typedef struct
{
	BOOL    retain;
    char    qos;
    short   payload_size;
    char  * payload;
    char  * topic;
}PublishMsg;


typedef struct
{
    PublishMsg queue[PUBLISH_QUEUE_SIZE];
    unsigned char       head;
    unsigned char       tail;
}PublishQueue;


typedef struct SdkContextTag
{
	MQTTAsync    client;
    //Network       network;
    PublishQueue  queue;
    MqttSdkInit  *pInitParam;
}SdkContext;

static SdkContext sdk_context = {0};
static pthread_mutex_t      mutex;


static  int startTime = 0;
static  int endTime = 0;

uint64_t mqtt_msg_id(void)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (uint64_t)((double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0);
}

void setReconnectTime(  int startime,  int endtime )
{
    startTime = startime;
	endTime   = endtime;

	return;
}

int reconnectTime(void   )
{
	int rconnectTime = 2;
	rconnectTime = rand()%( endTime - startTime + 1 ) + startTime;

	return rconnectTime;
}


//close msg cloud connect own
int shut_down(void)
{
    isLinkerClosed = 1;
    sleep_ms(200);
	return GW_OK;
}

BOOL mqtt_quit(void)
{
    if( 1 == isLinkerClosed )
        return TRUE;
    else
        return FALSE;
}

int sendout(char * topic, char * payload, int payloadsize, int qos, BOOL retain)
{
    return MQTTAsync_send(sdk_context.client, topic, payloadsize, payload, qos, retain, NULL);
}

static void enqueue(PublishQueue * queue, char * topic, char * payload, int payloadsize, int qos, BOOL retain)
{
    int h = queue->head;

    pthread_mutex_lock(&mutex);
    queue->queue[h].topic = topic;
    queue->queue[h].payload = payload;
    queue->queue[h].payload_size = payloadsize;
    queue->queue[h].qos = qos;
    queue->queue[h].retain = retain;
    queue->head = (h+1)%PUBLISH_QUEUE_SIZE;
    pthread_mutex_unlock(&mutex);
}

static PublishMsg * peekqueue(PublishQueue * queue)
{
    return &queue->queue[queue->tail];
}

static void dequeue(PublishQueue * queue)
{
	pthread_mutex_lock(&mutex);
    queue->tail = (queue->tail+1)%PUBLISH_QUEUE_SIZE;
    pthread_mutex_unlock(&mutex);
}

static BOOL process_pub_queue()
{
    BOOL rc = TRUE;
    PublishQueue * que = &sdk_context.queue;
    PublishMsg * p;
    while(QUEUE_IS_EMPTY(que) == FALSE)
    {
        p = peekqueue(que);
        if( sendout(p->topic, p->payload, p->payload_size, p->qos, p->retain) == 0 )
        {
        	msg_published(p->topic, p->payload);
            dequeue(que);
        }
        else
        {
            rc = FALSE;
            break;
        }
    }
    return rc;
}

BOOL emqtt_send(char * topic, char * payload, int payloadsize, int qos, BOOL retain)
{
    PublishQueue * queue = &sdk_context.queue;

    if( QUEUE_IS_FULL(queue) )
    {
        return FALSE;
    }
    else
    {
        enqueue(queue, topic, payload, payloadsize, qos, retain);
        return TRUE;
    }
}

int ipp_mqtt_send(IPP_MSG *ippMqtt, char *publishTopic, int qos)
{
	uint8_t *payload =  NULL;
	int   payload_size = 0;
	uint8_t *encrypt = NULL;
	int   encrypt_len = 0;
    int   aes_format = 2;
	mqtt_buffer_t mqtt_buffer = { 0 };

    if(NULL == ippMqtt )
    {
        LogE_Prefix("ippMqtt is NULL pointer\r\n");
        return GW_ERR;
    }

	LogI_Prefix("MQTT send msg\r\n");

	memset( &mqtt_buffer, 0, sizeof(mqtt_buffer) );
    put_ipp_msg_buffer(ippMqtt, &mqtt_buffer);

	payload_size = mqtt_buffer.current;
	payload = malloc(payload_size );
	if ( NULL == payload )
	{
		LogE_Prefix("ERROR: payload malloc failed\r\n");
		mqtt_cleanup_buffer(&mqtt_buffer);
		return GW_ERR;
	}

	memcpy(payload, mqtt_buffer.data, payload_size );

	mqtt_cleanup_buffer(&mqtt_buffer);

	encrypt=malloc( (payload_size+1) * sizeof(uint8_t));

	if ( NULL == encrypt )
	{
		LogE_Prefix("ERROR: encrypt malloc failed\r\n");
		FREE_POINTER(payload);
		return GW_ERR;
	}
	memset(encrypt, 0,  encrypt_len);

    if ( -1 == ipp_aes_encrypt(payload, payload_size, &encrypt, &encrypt_len, aes_format) )
    {
		LogE_Prefix("ERROR:aes_encrypt  failed\r\n");
		FREE_POINTER(payload);
		return GW_ERR;
	}
	FREE_POINTER(payload);
    LogI_Prefix("encrypt: %s,encrypt_len: %d\r\n",encrypt,encrypt_len);

	if( emqtt_send(publishTopic, (char *)encrypt, encrypt_len , qos, FALSE) < 0 )
    {
        LogI_Prefix("send message fail, queue is full\r\n");
        free(encrypt);
		return GW_ERR;
    }

	return GW_OK;
}

int msg_arrived(void* context, char* topicName, int topicLen, MQTTAsync_message* m)
{

	char *response = NULL;
	int resp_len = 0;
	uint8_t *decrypt = NULL;
	int   decrypt_len = 0;
	uint8_t *result = NULL;
	int   ressult_len = 0;
	IPP_MSG ipp_msg_get = {0};
	IPP_MSG ipp_msg_put = {0};
	int   aes_format = 2;
	static int64_t g_app_legal_msgid 	   = 0;

	LogI_Prefix("===============MQTT Recieved MSG===============\r\n");

	decrypt_len = m->payloadlen;
	decrypt = malloc(decrypt_len );

	if ( NULL == decrypt )
	{
		LogE_Prefix("ERROR: payload malloc failed\r\n");
		return GW_ERR;
	}
	strncpy((char *)decrypt, m->payload, m->payloadlen);
	LogI_Prefix("recv origi msg is: %s\r\n", decrypt);

	result = malloc( (decrypt_len+1) * sizeof(uint8_t) );
	memset(result, 0,  ressult_len);

	if( -1 == ipp_aes_decrypt(decrypt, decrypt_len, &result, &ressult_len, &aes_format)  )
	{
	   LogE_Prefix("ERROR: aes_decrypt failed\r\n");
	   FREE_POINTER(decrypt);
	   FREE_POINTER(result);
	   return GW_ERR;
	}
	FREE_POINTER(decrypt);
	/* get message buffer  */
	if ( -1 == get_ipp_msg_buffer(&ipp_msg_get, (char *)result))
	{
	   FREE_POINTER(result);
	   FREE_POINTER(ipp_msg_get.From);
	   FREE_POINTER(ipp_msg_get.To);
	   FREE_POINTER(ipp_msg_get.Content);
	   LogE_Prefix("recv msg is illegal!\r\n");
	   return GW_ERR;
	}
	FREE_POINTER(result);
	LogI_Prefix("Cloud Legal ID: %ld,Recved ID: %ld\r\n", g_app_legal_msgid, ipp_msg_get.Msgid);
	if(ipp_msg_get.Msgid<=g_app_legal_msgid)
	{
		LogE_Prefix("Replay Cloud Attack!!!\r\n");
		FREE_POINTER(ipp_msg_get.From);
		FREE_POINTER(ipp_msg_get.To);
		FREE_POINTER(ipp_msg_get.Content);
		return GW_ERR;
	}
	else
	{
		g_app_legal_msgid = ipp_msg_get.Msgid;
	}
	if( NULL == ipp_msg_get.From )
	{
		LogE_Prefix("ipp_msg_get.From is NULL!\r\n");
		FREE_POINTER(ipp_msg_get.To);
		FREE_POINTER(ipp_msg_get.Content);
		return GW_ERR;
	}
	else
	{
		if(NULL != msg_proc_func)
		{
			msg_proc_func((char*)ipp_msg_get.Content, ipp_msg_get.Length, &response, &resp_len);
		}
		ipp_msg_put.Version  = ipp_msg_get.Version;
		ipp_msg_put.FromLen  = ipp_msg_get.ToLen;
		ipp_msg_put.From	   = ipp_msg_get.To;
		ipp_msg_put.ToLen    = ipp_msg_get.FromLen;
		ipp_msg_put.To	   = ipp_msg_get.From;
		ipp_msg_put.Msgid    = ipp_msg_get.Msgid;
		ipp_msg_put.MsgType  = 0;// RES 0
		//ipp_msg_put.Length   = 5;
		//ipp_msg_put.Content  ="hello";
		ipp_msg_put.Length   = resp_len;
		ipp_msg_put.Content  = response;

		LogI_Prefix("User CMD Msgid: %ld\r\n", ipp_msg_get.Msgid);

		if (0 > ipp_mqtt_send(&ipp_msg_put, msgReceiptTopic, QOS_msgReceipt))
		{
			LogI_Prefix("ipp_mqtt_send Receipt failed!\r\n");
		}
	}

	FREE_POINTER(ipp_msg_get.From);
	FREE_POINTER(ipp_msg_get.To);
	FREE_POINTER(ipp_msg_get.Content);

	return 1;//can't return 0
}

int msg_published(char * topic, char * payload)
{
    LogI_Prefix("msg of %s has been sent, free its payload memory 0x%x\r\n", payload, (unsigned int)(long)payload);
    FREE_POINTER(payload);
    return GW_OK;
}


void mqtt_init(BOOL firstCall)
{
	DEV_INFO_S *dev = NULL;

	dev = get_gw_info();
	RETURN_VOID_IF_NULL(dev);
	
	while(TRUE)
	{
		if(NULL !=cdc_get_token(dev->sn, &g_mqttToken))
		{
			break;
		}
		sleep_ms(200);
	}
	
	//BOOL mqttConnectflag = FALSE;
	MQTTAsync *client = &sdk_context.client;
	MqttSdkInit init = {0};
	MQTTAsync_connectOptions connectData = MQTTAsync_connectOptions_initializer;
	MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
	int rc = 0, i = 0, reconnecttime = 2;
	
	linker_sn_len  = strlen(dev->sn);//need to get SN
    MALLOC( linker_sn, linker_sn_len + 1, char);
    strncpy(linker_sn, dev->sn, linker_sn_len + 1);
	LogI_Prefix("linker_sn: %s\r\n", linker_sn);
	LogI_Prefix("linker_sn_len: %d\r\n", linker_sn_len);

	MALLOC(  init.clientId, 2 + linker_sn_len + 1, char);
	snprintf(init.clientId, 2 + linker_sn_len + 1 , "d:%s", linker_sn);
	LogI_Prefix("init.clientId: %s\r\n",init.clientId);

	MALLOC(  init.username, 6 + linker_sn_len + 1, char);
	snprintf(init.username, 6 + linker_sn_len + 1, "IPP:d:%s", linker_sn);
	LogI_Prefix("init.username: %s\r\n",init.username);

	MALLOC(  init.password, strlen(g_mqttToken) + 1, char);
	snprintf(init.password, strlen(g_mqttToken) + 1 , "%s", g_mqttToken);
	LogI_Prefix("init.password:%s\r\n",init.password);

	MALLOC(  init.subscribedTopic[0], 4 + linker_sn_len + 1, char);
	snprintf(init.subscribedTopic[0], 4 + linker_sn_len + 1, "d/%s/i", linker_sn);
	LogI_Prefix("init.subscribedTopic[0]: %s\r\n", init.subscribedTopic[0]);

	MALLOC(  msgReceiptTopic, 4 + linker_sn_len + 1, char);
	snprintf(msgReceiptTopic, 4 + linker_sn_len + 1, "d/%s/m", linker_sn);
	LogI_Prefix("msgReceiptTopic: %s\r\n", msgReceiptTopic);

	MALLOC(  devStatusTopic, 4 + linker_sn_len + 1, char);
	snprintf(devStatusTopic, 4 + linker_sn_len + 1, "d/%s/m",linker_sn);
	LogI_Prefix("devStatusTopic: %s\r\n", devStatusTopic);

	MALLOC( init.host, IPV4_LEN, char);
	char ADDRESS[56] = {0};
	get_mqtt_server(init.host, &init.port);
	sprintf(ADDRESS, "tcp://%s:%d", init.host, init.port);
	init.callbackMsgArrived   = msg_arrived;
	init.subscribedTopicNum = 1;
	init.subscribeQos = 1;
	init.keepAliveInterval = HEART_BEAT_INTERVAL;
	init.cleanSession = TRUE;

	sdk_context.pInitParam = &init;
try_again:

	if( firstCall )
	{
		pthread_mutex_init(&mutex, NULL);
		firstCall = FALSE;
	}
	else
	{
		reconnecttime = reconnectTime();
		LogI_Prefix("reconnectTime: %d\r\n", reconnecttime );
		sleep_s( reconnecttime );    /* sleep reconnectTime seconds prior to reconnect */
	}

	rc = MQTTAsync_create(client, ADDRESS, init.clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTASYNC_SUCCESS)
	{
		LogE_Prefix("MQTT Client create error, try again\n");
		goto try_again;
	}

	connectData.keepAliveInterval = init.keepAliveInterval;
	connectData.cleansession = init.cleanSession;
	connectData.password = init.password;
	connectData.username = init.username;
	if(init.will.topicName )
	{
		connectData.will = &wopts;
		connectData.will->message = init.will.message;
		connectData.will->topicName = init.will.topicName;
		connectData.will->qos = init.will.qos;
		connectData.will->retained = init.will.retained;
	}

	rc = MQTTAsync_setCallbacks(sdk_context.client, NULL, NULL, sdk_context.pInitParam->callbackMsgArrived, NULL);

	if ((rc = MQTTAsync_connect(sdk_context.client, &connectData)) != 0)
	{
		LogI_Prefix("MQTT connect fail, try again\r\n");
		goto try_again;
	}
	else
	{
		LogI_Prefix("MQTT connected successfully\r\n");
	}

	for(i=0;i<init.subscribedTopicNum;i++)
	{
		if ((rc = MQTTAsync_subscribe(&sdk_context.client, init.subscribedTopic[i], init.subscribeQos, NULL)) != 0)
			LogI_Prefix("MQTT subscribe topic %s fail\r\n", init.subscribedTopic[i]);
		else
			LogI_Prefix("MQTT subscribe topic %s success\r\n", init.subscribedTopic[i]);
	}

	LogI_Prefix("connect mqtt broker success\r\n");

		
    return; 
}

void *mqtt_func(void *arg)
{
	BOOL firstCall = TRUE;

	setReconnectTime(2, 10);
reconnect:
	mqtt_init(firstCall);
	firstCall = FALSE;
	MqttQuitNow *quitnow = mqtt_quit;
	while(quit(quitnow) == FALSE)
	{
		if( process_pub_queue() == FALSE )
		{
			LogE_Prefix("publish message has error\r\n");
			goto reconnect;
		}
	}
	//MQTTAsync_disconnect(&sdk_context.client, 1000);
	MQTTAsync_destroy(&sdk_context.client);
	FREE_POINTER(linker_sn);
	FREE_POINTER(sdk_context.pInitParam->clientId);
	FREE_POINTER(sdk_context.pInitParam->username);
	FREE_POINTER(sdk_context.pInitParam->password);
	FREE_POINTER(sdk_context.pInitParam->subscribedTopic[0] );
	FREE_POINTER(msgReceiptTopic );
	FREE_POINTER(devStatusTopic );
	pthread_mutex_destroy(&mutex);

	return NULL;
}
void start_mqtt(MSG_PROC_FUNC cb)
{
	msg_proc_func = cb;
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&mqtt_pid, &attr, mqtt_func, NULL);
}


int mqtt_slave_online(const char *slaveId)
{

	const int online_flag = 1;
	char temp[MSG_JSON_LEN];
	char *json = "{\"serviceid\":%d,\"cmd\":0,\"devtype\":0,\"linker\":\"%s\",\"slave\":[\"%s\"]}";

    IPP_MSG  dev_msg = {0} ;

    LogI_Prefix("===============Report Msg slave upline===============\r\n");

	snprintf(temp, sizeof(temp), json, online_flag, linker_sn, slaveId);
    LogI_Prefix("whole:%s\r \n", temp);

	dev_msg.Version = 0;
	dev_msg.FromLen = strlen(linker_sn);
	dev_msg.From = linker_sn;
	dev_msg.ToLen = strlen("user");
	dev_msg.To = "user";
	dev_msg.Msgid = mqtt_msg_id();
	dev_msg.MsgType = 1; // REQ(2bit) 01
	dev_msg.Length = strlen(temp);
	dev_msg.Content = temp;

	if ( 0 > ipp_mqtt_send(&dev_msg, devStatusTopic, QOS_reportStatus) )
	{
	   LogE_Prefix("MQTT Slave Dev Upline Failed\r\n");
       return GW_ERR;
	}
	return GW_OK;
}

int mqtt_slave_offline(const char *slaveId)
{
    IPP_MSG  dev_msg = {0} ;
	char temp[MSG_JSON_LEN];
	const int offline_flag = 0;

	char *json = "{\"serviceid\":%d,\"cmd\":0,\"devtype\":0,\"linker\":\"%s\",\"slave\":[\"%s\"]}";

	LogI_Prefix("===============Report Msg slave offline===============\r\n");

	memset(temp, 0, sizeof(temp));

	snprintf(temp, sizeof(temp), json, offline_flag, linker_sn, slaveId);
    LogI_Prefix("whole:%s\r\n", temp);

	dev_msg.Version = 0;
	dev_msg.FromLen = linker_sn_len;
	dev_msg.From = linker_sn;
	dev_msg.ToLen = strlen("user");
	dev_msg.To = "user";
	dev_msg.Msgid = mqtt_msg_id();
	dev_msg.MsgType = 1; // REQ(2bit) 01
	dev_msg.Length = strlen(temp);
	dev_msg.Content = temp;

	if ( 0 > ipp_mqtt_send(&dev_msg, devStatusTopic, QOS_reportStatus) )
	{
	   LogI_Prefix("MQTT Slave Dev Offline Failed\r\n");
	   return GW_ERR;
	}

    return GW_OK;
}

int msg_reportMissing(char *info, int len, int64_t msg_id)
{
    IPP_MSG  dev_msg = {0} ;

	LogI_Prefix("===============Report Missing MSG===============\r\n");

	dev_msg.Version = 0;
	dev_msg.FromLen = linker_sn_len;
	dev_msg.From = linker_sn;
	dev_msg.ToLen = strlen("ippbizcloud");
	dev_msg.To = "ippbizcloud";
	dev_msg.Msgid = msg_id;
	dev_msg.MsgType = 1;
	dev_msg.Length = len;
	dev_msg.Content = info;

    LogI_Prefix("g_msgid: %ld\r\n",  msg_id);

	LogI_Prefix("MQTT send missing MSG\r\n");
	if ( 0 > ipp_mqtt_send(&dev_msg, msgMissingTopic, QOS_msgMissing) )
	{
	   LogI_Prefix("MQTT send missing MSG Failed\r\n");
       return GW_ERR;
	}

	return GW_OK;
}

int mqtt_report_status(char *info, int len, int64_t msg_id)
{
    IPP_MSG  dev_msg = {0} ;


	LogI_Prefix("===============Report msg device status===============\r\n");

	dev_msg.Version = 0;
	dev_msg.FromLen = linker_sn_len;
	dev_msg.From = linker_sn;
	dev_msg.ToLen = strlen("user");
	dev_msg.To = "user";
	dev_msg.Msgid = msg_id;
	dev_msg.MsgType = 1;
	dev_msg.Length = len;
	dev_msg.Content = info;

    LogI_Prefix("g_msgid: %ld\r\n", msg_id);

	LogI_Prefix("MQTT send dev status\r\n");
	if ( 0 > ipp_mqtt_send(&dev_msg, devStatusTopic, QOS_reportStatus) )
	{
	   LogI_Prefix("MQTT Send DevStatus Failed\r\n");
       return GW_ERR;
	}

	return GW_OK;
}

int msg_reporet_miss(char *info, int len, char *topic, int64_t msg_id)
{
    IPP_MSG  dev_msg = {0} ;

	LogI_Prefix("===============Report Miss:%s MSG===============\r\n", topic);

	dev_msg.Version = 0;
	dev_msg.FromLen = linker_sn_len;
	dev_msg.From = linker_sn;
	dev_msg.ToLen = strlen("ippbizcloud");
	dev_msg.To = "ippbizcloud";
	dev_msg.Msgid = msg_id;
	dev_msg.MsgType = 1;
	dev_msg.Length = len;
	dev_msg.Content = info;

    LogI_Prefix("g_msgid: %ld\r\n",  msg_id);

	LogI_Prefix("MQTT send missing MSG\r\n");
	if ( 0 > ipp_mqtt_send(&dev_msg, topic, 1) )
	{
	   LogI_Prefix("MQTT send missing MSG Failed\r\n");
       return GW_ERR;
	}

	return GW_OK;
}
