#ifndef __MQTT_H__
#define __MQTT_H__
#include "MQTTAsync.h"
#include "ipp_crypto.h"
#include "ipp_msg_handle.h"
#include "ipp_protocol.h"
#include "error_code.h"
#include "defines.h"


typedef long long int_t;
typedef int (*MSG_PROC_FUNC)(char *req, int req_len, char **resp, int *resp_len);

void start_mqtt(MSG_PROC_FUNC cb);
int shut_down(void);
int ipp_mqtt_send(IPP_MSG *ippMqtt, char *publishTopic, int qos);
int mqtt_report_status(char *info, int len, int64_t msg_id);
int msg_reportMissing(char *info, int len, int64_t msg_id);


int mqtt_slave_offline(const char *slaveId);
int mqtt_slave_online(const char *slaveId);
int msg_reporet_miss(char *info, int len, char *topic, int64_t msg_id);

int msg_published(char * topic, char * payload);

int mqtt_send_contrl(char *dst_sn, char *info, int len, char *topic);

uint64_t msg_time_stamp(void);

#endif
