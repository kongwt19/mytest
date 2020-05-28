#include <string.h>
#include <stdarg.h>

#include "platform.h"
#include "mqtt.h"
#include "iot_mid_ware.h"
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
#define RETAIN     FALSE

/* flash is not enough, NO CERT */
#define HEART_BEAT_INTERVAL  30      
#define PUBLISH_QUEUE_SIZE      8

static char *linker_sn = NULL;
static int   linker_sn_len =  0;
static char *clientId = NULL;
static char *username = NULL;
static char *password = NULL;
static char *subscribedTopic = NULL;
static char *host     = NULL;
static short port     = 0;
static int  isLinkerClosed = 0;

static const int QOS_reportStatus = 1;
static const int QOS_msgReceipt   = 1;
static const int QOS_msgMissing   = 1;

volatile MQTTAsync_token deliveredtoken;

static char *msgReceiptTopic = NULL;
static char *devStatusTopic  = NULL;
static char *msgMissingTopic  = "d/miss/m";
static MSG_PROC_FUNC msg_proc_func = NULL;
static pthread_t mqtt_pid;

//need get token
char *g_mqttToken = NULL;

static MQTTAsync client;

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

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	LogI_Prefix("mqtt successful disconnection\n");
}


void onSubscribe(void* context, MQTTAsync_successData* response)
{
	LogI_Prefix("mqtt subscribe succeeded\n");
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	LogI_Prefix("mqtt subscribe failed, rc %d\n", response ? response->code : 0);
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	LogI_Prefix("mqtt connect failed, rc %d\n", response ? response->code : 0);
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client_rec = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;

	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client_rec;
	deliveredtoken = 0;

	if ((rc = MQTTAsync_subscribe(client_rec, subscribedTopic, QOS_msgReceipt, &opts)) != MQTTASYNC_SUCCESS)
	{
		LogE_Prefix("mqtt failed to start subscribe, return code %d\n", rc);
	}
	LogI_Prefix("mqtt successful subscribed to topic %s\n", subscribedTopic);
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

	memset(&mqtt_buffer, 0, sizeof(mqtt_buffer));
    put_ipp_msg_buffer(ippMqtt, &mqtt_buffer);

	payload_size = mqtt_buffer.current;
	payload = (uint8_t *)malloc(payload_size );
	if ( NULL == payload )
	{
		LogE_Prefix("ERROR: payload malloc failed\r\n");
		mqtt_cleanup_buffer(&mqtt_buffer);
		return GW_ERR;
	}

	memcpy(payload, mqtt_buffer.data, payload_size );

	mqtt_cleanup_buffer(&mqtt_buffer);

	encrypt=(uint8_t *)malloc( (payload_size+1) * sizeof(uint8_t));

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

	if ( MQTTAsync_send(client, publishTopic, encrypt_len, (char *)encrypt, qos, RETAIN, NULL) < 0 )
    {
        LogI_Prefix("send message fail, queue is full\r\n");
        FREE_POINTER(encrypt);
		return GW_ERR;
    }
	LogI_Prefix("msg of %s has been sent, free its payload memory 0x%x\r\n", encrypt, (unsigned int)(long)encrypt);
	FREE_POINTER(encrypt);
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
	decrypt = (uint8_t *)malloc(decrypt_len );

	if ( NULL == decrypt )
	{
		LogE_Prefix("ERROR: payload malloc failed\r\n");
		return GW_ERR;
	}
	strncpy((char *)decrypt, (const char *)m->payload, m->payloadlen);
	LogI_Prefix("recv msg is: %s\r\n", decrypt);

	result = (uint8_t *)malloc( (decrypt_len+1) * sizeof(uint8_t) );
	if(NULL == result)
	{
		FREE_POINTER(decrypt);
		LogE_Prefix("%s-%d:NULL pointer, return!\r\n",    __func__, __LINE__);
		return GW_ERR;
	}
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
	/*if(ipp_msg_get.Msgid<=g_app_legal_msgid)
	{
		LogE_Prefix("Replay Cloud Attack!!!\r\n");
		FREE_POINTER(ipp_msg_get.From);
		FREE_POINTER(ipp_msg_get.To);
		FREE_POINTER(ipp_msg_get.Content);
		return GW_ERR;
	}
	else
	{*/
		g_app_legal_msgid = ipp_msg_get.Msgid;
	//}
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
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc = 0,reconnecttime = 2;
	
	linker_sn_len  = strlen(dev->sn);//need to get SN
    MALLOC( linker_sn, linker_sn_len + 1, char);
    strncpy(linker_sn, dev->sn, linker_sn_len + 1);
	LogI_Prefix("linker_sn: %s\r\n", linker_sn);

	MALLOC(  clientId, 2 + linker_sn_len + 1, char);
	snprintf(clientId, 2 + linker_sn_len + 1 , "d:%s", linker_sn);
	LogI_Prefix("clientId: %s\r\n",clientId);

	MALLOC(  username, 6 + linker_sn_len + 1, char);
	snprintf(username, 6 + linker_sn_len + 1, "IPP:d:%s", linker_sn);
	LogI_Prefix("username: %s\r\n",username);

	MALLOC(  password, strlen(g_mqttToken) + 1, char);
	snprintf(password, strlen(g_mqttToken) + 1 , "%s", g_mqttToken);
	LogI_Prefix("password:%s\r\n",password);

	MALLOC(  subscribedTopic, 4 + linker_sn_len + 1, char);
	snprintf(subscribedTopic, 4 + linker_sn_len + 1, "d/%s/i", linker_sn);
	LogI_Prefix("subscribedTopic: %s\r\n", subscribedTopic);

	MALLOC(  msgReceiptTopic, 4 + linker_sn_len + 1, char);
	snprintf(msgReceiptTopic, 4 + linker_sn_len + 1, "d/%s/m", linker_sn);
	LogI_Prefix("msgReceiptTopic: %s\r\n", msgReceiptTopic);

	MALLOC(  devStatusTopic, 4 + linker_sn_len + 1, char);
	snprintf(devStatusTopic, 4 + linker_sn_len + 1, "d/%s/m",linker_sn);
	LogI_Prefix("devStatusTopic: %s\r\n", devStatusTopic);

	MALLOC( host, IPV4_LEN, char);
	char ADDRESS[56] = {0};
	get_mqtt_server(host, &port);
	sprintf(ADDRESS, "tcp://%s:%d", host, port);


try_again:

	if( firstCall )
	{
		firstCall = FALSE;
	}
	else
	{
		reconnecttime = reconnectTime();
		LogI_Prefix("reconnectTime: %d\r\n", reconnecttime );
		sleep_s( reconnecttime );    /* sleep reconnectTime seconds prior to reconnect */
	}

	MQTTAsync_create(&client, ADDRESS, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	MQTTAsync_setCallbacks(client, client, NULL, msg_arrived, NULL);

	conn_opts.keepAliveInterval = 30;
	conn_opts.cleansession = 1;
	conn_opts.username = username;
	conn_opts.password = password;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.automaticReconnect = 1;
	conn_opts.minRetryInterval = 2; //seconds
	conn_opts.maxRetryInterval = 365*24*60*60;
	conn_opts.context = client;
	if ( (rc = MQTTAsync_connect(client, &conn_opts) ) != MQTTASYNC_SUCCESS)
	{
		LogE_Prefix("Failed to start connect, return code %d\n", rc);
		goto try_again;
	}
	while(1)
    {
		rc = MQTTAsync_setConnected(client, (void *)client, onConnect);
        if (rc == MQTTASYNC_SUCCESS)
        {
            LogI_Prefix("Successfully setConnected.\n");
            break;
        }
        sleep_s(1);
        LogE_Prefix("Failed to setConnected, return error code %d.\n", rc);
    }
	LogI_Prefix("connect mqtt broker success\r\n");

		
    return; 
}

void *mqtt_func(void *arg)
{
	BOOL firstCall = TRUE;

	setReconnectTime(2, 10);
	mqtt_init(firstCall);

	while(isLinkerClosed == 0)
	{
		sleep_s(10);
	}
	int rc = 0;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	disc_opts.onSuccess = onDisconnect;
	if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		LogE_Prefix("Failed to start disconnect, return code %d\n", rc);
	}
	MQTTAsync_destroy(&client);
	FREE_POINTER(linker_sn);
	FREE_POINTER(clientId);
	FREE_POINTER(username);
	FREE_POINTER(password);
	FREE_POINTER(subscribedTopic);
	FREE_POINTER(msgReceiptTopic );
	FREE_POINTER(devStatusTopic );

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
	dev_msg.Msgid = mqtt_msg_id();
	dev_msg.MsgType = 1;
	dev_msg.Length = len;
	dev_msg.Content = info;

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
	dev_msg.Msgid = mqtt_msg_id();
	dev_msg.MsgType = 1;
	dev_msg.Length = len;
	dev_msg.Content = info;

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
	dev_msg.Msgid = mqtt_msg_id();
	dev_msg.MsgType = 1;
	dev_msg.Length = len;
	dev_msg.Content = info;

	if ( 0 > ipp_mqtt_send(&dev_msg, topic, 1) )
	{
	   LogI_Prefix("MQTT send missing MSG Failed\r\n");
       return GW_ERR;
	}

	return GW_OK;
}

int mqtt_send_contrl(char *dst_sn, char *info, int len, char *topic)
{
    IPP_MSG  dev_msg = {0} ;

	LogI_Prefix("===============Report control:%s MSG===============\r\n", topic);

	dev_msg.Version = 0;
	dev_msg.FromLen = linker_sn_len;
	dev_msg.From = linker_sn;
	dev_msg.ToLen = strlen(dst_sn);
	dev_msg.To = dst_sn;
	dev_msg.Msgid = mqtt_msg_id();
	dev_msg.MsgType = 1;
	dev_msg.Length = len;
	dev_msg.Content = info;

	if ( 0 > ipp_mqtt_send(&dev_msg, topic, QOS_reportStatus) )
	{
	   LogI_Prefix("MQTT send control MSG Failed\r\n");
       return GW_ERR;
	}

	return GW_OK;
}
