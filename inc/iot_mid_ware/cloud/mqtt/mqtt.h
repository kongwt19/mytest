#ifndef __MQTT_H__
#define __MQTT_H__
#include "MQTTAsync.h"
#include "ipp_crypto.h"
#include "ipp_msg_handle.h"
#include "ipp_protocol.h"
#include "error_code.h"
#include "defines.h"

#define Mqtt_TOPIC_ARRAY_SIZE  4

typedef long long int_t;


typedef struct _MqttRequest{
	char* devId;
	char type;
	int  serviceId;
	unsigned char *request;
	int requestLen;
}MQTT_REQUEST_S;


typedef struct _stMqttDataList{

	int listSize;
	unsigned char* dataList;
}MQTT_CONTENT_S;

typedef struct
{
    char     *topicName;
    char     *message;
    short     message_size;
    BOOL  retained;
    char      qos;
} MqttWill;

typedef struct MessageData
{
	MQTTAsync_message* message;
    char* topicName;
} MessageData;

typedef int MqttMsgArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* m);
typedef int MqttMsgPublished(char *topic, char * payload);
typedef BOOL MqttQuitNow(void);
typedef void MqttConnectionLost(void);
typedef void MqttConnectSuccess(void);
typedef int (*MSG_PROC_FUNC)(char *req, int req_len, char **resp, int *resp_len);

//typedef MqttQuitNow 		mqtt_exit;
//typedef MqttMsgArrived 		mqtt_received;
//typedef MqttMsgPublished 	mqtt_published;

typedef struct MqttSdkInitTag
{
	BOOL          cleanSession;
    short             port;
    short             keepAliveInterval;
    char             *host;
    char             *clientId;
    char             *username;
    char             *password;
    char              subscribeQos;
    char              subscribedTopicNum;
    char             *subscribedTopic[Mqtt_TOPIC_ARRAY_SIZE];
    MqttMsgArrived   *callbackMsgArrived;
    MqttWill         will;
    const char       *cacert;
    unsigned int      cacert_len;
}MqttSdkInit;

void start_mqtt(MSG_PROC_FUNC cb);
int shut_down(void);
int ipp_mqtt_send(IPP_MSG *ippMqtt, char *publishTopic, int qos);
int mqtt_report_status(char *info, int len, int64_t msg_id);
int msg_reportMissing(char *info, int len, int64_t msg_id);


int mqtt_slave_offline(const char *slaveId);
int mqtt_slave_online(const char *slaveId);
int msg_reporet_miss(char *info, int len, char *topic, int64_t msg_id);

int msg_published(char * topic, char * payload);

#endif
