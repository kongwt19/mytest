#include "biz_plf.h"
#include "cJSON.h"
#include "dev_mng.h"
#include "platform.h"
#include "error_code.h"
#include "ipp_crypto.h"
#include "defines.h"
#include "msg_mng.h"
#include "httpc.h"
#include "iot_mid_ware.h"

static char *g_gettoken 		= "/cdc/token/gettoken";//  

static char *g_alarmonoff 		= "/ippinte/api/alarm/setswitch";
static char *g_alarmswitch 		= "/ippinte/api/alarm/getswitch";
//static char *g_scenepanel 		= "/ippinte/api/scenepanel/report";
static char *g_scenepanel_get_point 		= "/ippinte/api/scenepanel/getByPoint";

static HTTP_FUNC_URL_S g_http_uri[] = 
{
	{MSG_REPORT_STATUS, 		"/cdc/device/upstatus"},
	{MSG_MULTI_REPORT_STATUS,	"/cdc/device/multiupstatus"},
	{MSG_CHILD_ONLINE, 			"/cdc/device/slaveonline"},
	{MSG_CHILD_OFFLINE, 		"/cdc/device/slaveoffline"},
	{MSG_UPDATE_INFO, 			"/cdc/device/update"},
	{MSG_ALARM, 				"/ippinte/api/alarm/newreport"},
	{MSG_REGIST, 				"/cdc/device/register"},
};

//加密
char * ipp_encrypt(uint8_t * in){
    //uint8_t *in = "hello"; //a4d4a2ed587540c429efdd80c4a3e22e
	int format = 2;     
	int inlen, outlen;	
	
	inlen = strlen((char *)in);

	uint8_t *out = NULL;
	out = (uint8_t *)malloc((inlen+1) * sizeof(uint8_t));
	if(NULL == out)
	{
		return NULL;
	}
	//outlen = strlen(out);
	ipp_aes_encrypt( in, inlen, &out, &outlen, format );
	return (char *)out;
}
//解密
char * ipp_decrypt(uint8_t * out){
	//uint8_t *in = "hello"; //a4d4a2ed587540c429efdd80c4a3e22e
	int format = 2;     
	int inlen, outlen;	
	
	outlen = strlen((char *)out);

    uint8_t *in = NULL;
	in = (uint8_t *)malloc( (outlen+1) * sizeof(uint8_t) );
	if(NULL == in)
	{
		return NULL;
	}
	inlen = strlen((char *)in);
	ipp_aes_decrypt(out,  outlen, &in, &inlen, &format);
	LogD_Prefix( "Response json:%s\n", in);
	return (char *)in;
}

//解析响应的json,正常返回GW_OK，错误返回GW_HTTP_FAIL
int parse_response_json(char * response_json){
	//解密
	/* 获取第一个子字符串 */
	char * response_str = NULL;
	response_str = strtok(response_json, "\n");
	if(response_str == NULL)//response_json不包含"\n"
	{
		response_str = response_json;
	}
	LogD_Prefix( "Response encrypt:%s\n", response_str);

	char * decrypt_str = ipp_decrypt((uint8_t *)response_str);
	if(decrypt_str == NULL) 
	{
		LogE_Prefix("%s\n","decrypt err");
		return GW_HTTP_FAIL;
	}
	
    cJSON * json = cJSON_Parse(decrypt_str);
	FREE_POINTER(decrypt_str);
	if(json == NULL) 
    {
		LogE_Prefix("%s\n","cJSON_Parset err");
        return GW_HTTP_FAIL;
    }
    cJSON * code_node = cJSON_GetObjectItem(json, "code");
	if(code_node == NULL)
	{
         cJSON_Delete(json);
	     LogE_Prefix("%s\n","data format error");
		 return GW_HTTP_FAIL;
	    }

	char * code = code_node->valuestring;
	if(strcmp(code, HTTP_REQ_SUCCESS)!=0) 
	{
		cJSON_Delete(json);
		LogE_Prefix("%s\n","response err");
		return GW_HTTP_FAIL;
	}

    cJSON_Delete(json);
    return GW_OK;
}

//获取token，正常返回token，错误返回NULL
char * parse_token(char * response_json){
	char * response_str = NULL;
	response_str = strtok(response_json, "\n");
	if(response_str == NULL)//response_json不包含"\n"
	{
		response_str = response_json;
	}
	char * decrypt_str = ipp_decrypt((uint8_t *)response_str);
	if(decrypt_str == NULL) 
	{
		LogE_Prefix("%s\n","decrypt err");
		return NULL;
	}
    cJSON * json = cJSON_Parse(decrypt_str);
	FREE_POINTER(decrypt_str);
	if(json == NULL) 
    {
		LogE_Prefix("%s\n","cJSON_Parset err");
        return NULL;
    }

	//code
	cJSON * code_node = cJSON_GetObjectItem(json, "code");
	if(code_node == NULL)
	{
         cJSON_Delete(json);
	     LogE_Prefix("%s\n","data format error");
		 return NULL;
	}
	char * code = code_node->valuestring;
	if(strcmp(code, HTTP_REQ_SUCCESS)!=0) 
	{
		cJSON_Delete(json);
		LogE_Prefix("%s\n","response err");
		return NULL;
	}

	//token
    cJSON * token_node = cJSON_GetObjectItem(json, "token");
	if(token_node == NULL)
	{
         cJSON_Delete(json);
	     LogE_Prefix("%s\n","data format error");
		 return NULL;
	 }
	char * token_str = token_node->valuestring;
	if(token_str == NULL)
	{
		cJSON_Delete(json);
		LogE_Prefix("%s\n","token is NULL");
		return NULL;
	}
	int token_len = strlen(token_node->valuestring)+1;
	char * token = (char *)malloc(token_len);
	if(token == NULL)
	{
		cJSON_Delete(json);
		LogE_Prefix("%s\n","token malloc is NULL");
		return NULL;
	}
	memset(token, '\0', token_len);
	strcpy(token, token_str);
    cJSON_Delete(json);
    return token;
}

//get scenepanel list, when deviceList is NULL,* scene_panel_list is NULL
int  parse_scene_panel_list(char * response_json, char ** scene_panel_list){
	RETURN_IF_NULL(response_json, GW_NULL_PARAM);
	
	* scene_panel_list = NULL;
	char * response_str = NULL;
	response_str = strtok(response_json, "\n");
	if(response_str == NULL)//response_json不包含"\n"
	{
		response_str = response_json;
	}
	char * decrypt_str = ipp_decrypt((uint8_t *)response_str);
	if(decrypt_str == NULL) 
	{
		LogE_Prefix("%s\n","decrypt err");
		return GW_ERR;
	}

    cJSON * json = cJSON_Parse(decrypt_str);
	FREE_POINTER(decrypt_str);
	if(json == NULL) 
    {
		LogE_Prefix("%s\n","cJSON_Parset err");
        return GW_ERR;
    }

	//code
	cJSON * code_node = cJSON_GetObjectItem(json, "code");
	if(code_node == NULL)
	{
         cJSON_Delete(json);
	     LogE_Prefix("%s\n","data format error");
		 return GW_ERR;
	}
	char * code = code_node->valuestring;
	if(strcmp(code, HTTP_REQ_SUCCESS)!=0) 
	{
		cJSON_Delete(json);
		LogE_Prefix("%s\n","response err");
		return GW_ERR;
	}

	//scene dev list
	cJSON * scenelist_node = cJSON_GetObjectItem(json, "scenePointInfo");
	if(scenelist_node == NULL)
	{
         cJSON_Delete(json);
	     LogE_Prefix("scenePointInfo is NULL\r\n");
		 return GW_ERR;
	}

	cJSON * devicelist_node = cJSON_GetObjectItem(scenelist_node, "deviceList");
	if(devicelist_node == NULL)
	{
		 cJSON_Delete(json);
	     LogE_Prefix("deviceList is NULL\r\n");
		 return GW_OK;
	}
    char * scene_panel_list_str = cJSON_PrintUnformatted(devicelist_node);
	if(scene_panel_list_str == NULL)
	{
         cJSON_Delete(json);
	     LogE_Prefix("scene panel list str is NULL\r\n");
		 return GW_ERR;
	}
    
	* scene_panel_list = scene_panel_list_str;
	
    cJSON_Delete(json);
    return GW_OK;
}
//得到请求结果后的回调函数
size_t write_data(char * buffer,size_t size,size_t nmemb,char *stream)
{
    stream = (char *)realloc(stream, strlen(buffer));
	if(stream != NULL)
	{
        memset(stream, '\0', strlen(buffer));
        strcpy(stream, buffer);
	}
	else
	{
		LogE_Prefix("%s\n", "realloc NULL error");
	}
    return size*nmemb;
}

/*
发送请求
@param    char * url：url地址
@param    char * is_post: "1" post ; "0" get
@param    char * json_data: post 发送的json
@return   char * 正常返回响应字符串首地址，错误返回NULL
*/
char * curl_send(URL_INFO_S *url, char * json_data)
{
	ac_tcpclient client;
	char * encrypt_str = NULL;
	char *resp = NULL;
	LogD_Prefix("Request json:%s\n", json_data);

	int ret = ac_tcpclient_create(&client, url->ip, url->port);
	if(0 != ret)
	{
		return NULL;
	}
	//加密
	encrypt_str = ipp_encrypt((uint8_t *)json_data);
	if(encrypt_str == NULL) 
	{
		LogE_Prefix("Encrypt request failed\n");
		ac_tcpclient_close(&client);
		return NULL;
	}
    LogD_Prefix("Request encrypt:%s\n", encrypt_str);
	ac_http_post( &client, url->uri, encrypt_str, &resp);
	FREE_POINTER(encrypt_str);

	return resp;
}

//统一设置ipp和key
void set_ipp_key(cJSON * json){
    cJSON * system_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "system", system_obj);
    cJSON_AddStringToObject(system_obj, "ipp", IPP);
    cJSON_AddStringToObject(system_obj, "key", KEY);
}

/*******************Device gettoken json content*********************
{
	"system":
			{
				"ipp":"3.0",
				"key":"xx"
			},
	
		"request":
			{
					"kid":"xxxxxxxxxxxxxxxxxxx"
			}
}
******************************************************************/
char * cdc_get_token(char *devID, char **token)
{
	char * json_data = NULL;
    char * response_data = NULL;
	cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		LogE_Prefix("%s\n", "root NULL error");
		return NULL;
	}
    cJSON *system = cJSON_CreateObject();
	if(system == NULL)
	{
		cJSON_Delete(root);
		LogE_Prefix("%s\n", "system NULL error");
		return NULL;
	}
    cJSON *request = cJSON_CreateObject();
	if(request == NULL)
	{
		cJSON_Delete(root);
		cJSON_Delete(system);
		LogE_Prefix("%s\n", "request NULL error");
		return NULL;
	}

    cJSON_AddItemToObject(root,"system",system);
    cJSON_AddStringToObject(system,"ipp",IPP);
    cJSON_AddStringToObject(system,"key",KEY);
    cJSON_AddItemToObject(root,"request",request);
    cJSON_AddStringToObject(request,"kid",devID);
	
    json_data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if(json_data == NULL){
		LogE_Prefix("%s\n", " cJSON_Print return NULL");
		return NULL;
	}

	URL_INFO_S url;
	memset(&url, 0, sizeof(url));
	get_cdc_server(url.ip, &url.port);
	url.uri = g_gettoken;

    response_data = curl_send(&url, json_data);
	FREE_POINTER(json_data);
	if(response_data == NULL){
		LogE_Prefix("%s\n", "curl_send return NULL");
		return NULL;
	}

	*token = parse_token(response_data);
	FREE_POINTER(response_data);

    return *token;
}

/*******************Device send alarm******************************
{
	"linkerid":"gh2c13c4bc41bc439010",
	"devid":"0D6F000BD72F7C00",
	"devtype":1,
	"content":"10022",
	"alarmNumber":"10001"
}
******************************************************************/
int ippinte_alarm(char *deviceID, char* alarmInfo, int64_t alarmNumber)
{
	int ret = GW_ERR;
	char * json_data = NULL;
	//***************************************
	DEV_INFO_S *dev = NULL;
	cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		LogE_Prefix("%s\n", "root NULL error");
		return GW_HTTP_FAIL;
	}

	if(!(dev = get_dev_info((char *)deviceID)))
	{
		LogE_Prefix("%s\n", "Do not find the device!");
		return GW_HTTP_FAIL;
	}
	cJSON_AddStringToObject(root, "linkerid", dev->gw_sn); 
	FREE_POINTER(dev);
	cJSON_AddStringToObject(root, "devid", deviceID);
	cJSON_AddNumberToObject(root, "devtype", DEV_TYPE);
	cJSON_AddStringToObject(root, "content", alarmInfo);
	cJSON_AddNumberToObject(root, "alarmNumber", alarmNumber);
	
    json_data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if(NULL != json_data)
	{
		ret = http_send_msg(deviceID, MSG_ALARM, json_data, strlen(json_data), TRUE);
		FREE_POINTER(json_data);
	}

    return ret;
}

/*******************Device get alarm switch***********************
{
	"devid":"0D6F000BD72F7C00",
	"devtype":1
}
******************************************************************/
int ippinte_get_alarm_switch(char *devID)
{
	char * json_data = NULL;
    char * response_data = NULL;
	//
	cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		LogE_Prefix("%s\n", "root NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON_AddStringToObject(root, "devid", devID);
	cJSON_AddNumberToObject(root, "devtype", DEV_TYPE);
	
    json_data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if(json_data == NULL){
		LogE_Prefix("%s\n", " cJSON_Print return NULL");
		return GW_HTTP_FAIL;
	}

	URL_INFO_S url;
	memset(&url, 0, sizeof(url));
	get_cdc_server(url.ip, &url.port);
	url.uri = g_alarmswitch;
    response_data = curl_send(&url, json_data);
	FREE_POINTER(json_data);
	if(response_data == NULL){
		LogE_Prefix("%s\n", "curl_send return NULL");
		return GW_HTTP_FAIL;
	}

	int result = parse_response_json(response_data);
	FREE_POINTER(response_data);

    return result;
}

/*******************Device set alarm switch***********************
{
	"devid":"0D6F000BD72F7C00",
	"devtype":1
	"onoff":1
}
******************************************************************/
int ippinte_set_alarm_switch(char *devID, int onoff)
{
	//
	char * json_data = NULL;
    char * response_data = NULL;
	//
	cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		LogE_Prefix("%s\n", "root NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON_AddStringToObject(root, "devid", devID);
	cJSON_AddNumberToObject(root, "devtype", DEV_TYPE);
	cJSON_AddNumberToObject(root, "onoff", onoff);
	
    json_data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if(json_data == NULL){
		LogE_Prefix("%s\n", " cJSON_Print return NULL");
		return GW_HTTP_FAIL;
	}

	URL_INFO_S url;
	memset(&url, 0, sizeof(url));
	get_cdc_server(url.ip, &url.port);
	url.uri = g_alarmonoff;
    response_data = curl_send(&url, json_data);
	FREE_POINTER(json_data);
	if(response_data == NULL){
		LogE_Prefix("%s\n", "curl_send return NULL");
		return GW_HTTP_FAIL;
	}

	int result = parse_response_json(response_data);
	FREE_POINTER(response_data);

    return result;
}

/*******************get scene panel info***********************
{"deviceId":"0D6F000D57B76500","point":1}
******************************************************************/
int ippinte_scene_panel_get_by_point(char *devid, int point, char ** scene_panel_list)
{
	RETURN_IF_NULL(devid, GW_NULL_PARAM);
	RETURN_IF_NULL(scene_panel_list, GW_NULL_PARAM);
	char * json_data = NULL;
	char * response_data = NULL;

	cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		LogE_Prefix("%s\n", "root NULL error");
		return GW_HTTP_FAIL;
	}

	cJSON_AddStringToObject(root, "deviceId", devid);
	cJSON_AddNumberToObject(root, "point", point);
	
    json_data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if(json_data == NULL){
		LogE_Prefix("%s\n", " cJSON_Print return NULL");
		return GW_HTTP_FAIL;
	}

	URL_INFO_S url;
	memset(&url, 0, sizeof(url));
	get_ippinte_server(url.ip, &url.port);
	url.uri = g_scenepanel_get_point;
    response_data = curl_send(&url, json_data);
	FREE_POINTER(json_data);
	if(response_data == NULL){
		LogE_Prefix("%s\n", "curl_send return NULL");
		return GW_HTTP_FAIL;
	}

	int result = parse_scene_panel_list(response_data, scene_panel_list);
	FREE_POINTER(response_data);

    return result;
}
#if 0
//场景
int ippinte_scene_panel_report(char *info, char *sn, int len)
{
	 char * response_data = NULL;
	char url[URL_SIZE] = {0};
    strcpy(url, g_ser_host);
    strcat(url, g_scenepanel);
    response_data = curl_send(url, POST, info);
    if(response_data == NULL){
		LogE_Prefix("%s\n", "curl_send return NULL");
		return GW_HTTP_FAIL;
	}

	int result = parse_response_json(response_data);
	
	FREE_POINTER(response_data);

    return result;
}
#endif
/******************************************************
 {
	"system":	{
		"ipp":	"3.0",
		"key":	"key3.0"
	},
	"request":	{
		"linker":	"sn",
		"devlist":	[{
				"devid":	      "asgfk231cw",
				"ip":	      "xxx.xxx.x.xx",
				"mac":		  "00-00-00-00-00-00",
				"product_id": "zdf34c45",
				"soft_ver":	  "abc"
			}]
	}
}
*******************************************************/
int cdc_slave_device_online(DEV_INFO_S *dev)
{
	int ret = GW_ERR;
	char * json_data = NULL;
    
    cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		LogE_Prefix("%s\n", "root NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON *system = cJSON_CreateObject();
	if(system == NULL)
	{
		cJSON_Delete(root);
		LogE_Prefix("%s\n", "system NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON *request = cJSON_CreateObject();
	if(request == NULL)
	{
		cJSON_Delete(root);
		cJSON_Delete(system);
		LogE_Prefix("%s\n", "request NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON_AddItemToObject(root,"system",system);
	cJSON_AddStringToObject(system,"ipp",IPP);
    cJSON_AddStringToObject(system,"key",KEY);

	cJSON_AddItemToObject(root,"request",request);
	cJSON_AddStringToObject(request,"linker",dev->gw_sn);  
    //cJSON Arraylist   
	cJSON *JsonArray = cJSON_CreateArray();     
	cJSON *ArrayItem = cJSON_CreateObject();
 	cJSON_AddStringToObject(ArrayItem, "devid", dev->sn);
	cJSON_AddStringToObject(ArrayItem, "ip", dev->ip);
	cJSON_AddStringToObject(ArrayItem, "mac", dev->mac);
	cJSON_AddStringToObject(ArrayItem, "productid", dev->product_id);
	cJSON_AddStringToObject(ArrayItem, "swver", dev->soft_ver);
	
	cJSON_AddItemToArray(JsonArray, ArrayItem);
    cJSON_AddItemToObject(request, "devlist", JsonArray);
   
    json_data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if(NULL != json_data)
	{
		ret = http_send_msg(dev->sn, MSG_CHILD_ONLINE, json_data, strlen(json_data), TRUE);
		FREE_POINTER(json_data);
	}

    return ret;
}

/*******************slave_device_offline*********************
{
	"system":
		{
			"ipp":"3.0",
			"key":"key3.0"
		},
	"request":
		{
			"linker":"MYW20160412-test123420",
			"slave" : ["0D6F000BD72F7C00"]
		}
}
************************************************************/
int cdc_slave_device_offline(char *slaveId)
{
	int ret = GW_ERR;
	DEV_INFO_S *dev = NULL;
//********************************
    char * json_data = NULL;
    cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		LogE_Prefix("%s\n", "root NULL error");
		return GW_HTTP_FAIL;
	}
    cJSON *system = cJSON_CreateObject();
	if(system == NULL)
	{
		cJSON_Delete(root);
		LogE_Prefix("%s\n", "system NULL error");
		return GW_HTTP_FAIL;
	}
    cJSON *request = cJSON_CreateObject();
	if(request == NULL)
	{
		cJSON_Delete(root);
		cJSON_Delete(system);
		LogE_Prefix("%s\n", "request NULL error");
		return GW_HTTP_FAIL;
	}

   //JsonArray
	cJSON *JsonArray = cJSON_CreateArray();
	if(JsonArray == NULL)
	{
		cJSON_Delete(root);
		cJSON_Delete(system);
		cJSON_Delete(request);
		LogE_Prefix("%s\n", "Json Array NULL error");
		return GW_HTTP_FAIL;
	}
    
    cJSON_AddItemToObject(root,"system",system);
    cJSON_AddStringToObject(system,"ipp",IPP);
    cJSON_AddStringToObject(system,"key",KEY);
    
    cJSON_AddItemToObject(root,"request",request);
	if(!(dev = get_dev_info((char *)slaveId)))
	{
		LogE_Prefix("%s\n", "Do not find the device!");
		return GW_HTTP_FAIL;
	}
    cJSON_AddStringToObject(request,"linker",dev->gw_sn);    

 	cJSON_AddStringToObject(JsonArray, "slave", dev->sn);
	FREE_POINTER(dev);
    cJSON_AddItemToObject(request, "slave", JsonArray);

    json_data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if(NULL != json_data)
	{
		ret = http_send_msg(slaveId, MSG_CHILD_OFFLINE, json_data, strlen(json_data), TRUE);
		FREE_POINTER(json_data);
	}

    return ret;
}
/*******************dev_report_status***********************
{
   "system":{
            "ipp":"3.0",
            "key":"1k8VKDqpOMndSxtDhrExYtTAsV"
    },
   "request":{
	   		"devid": "0D6F000D578D4F00",
             "status": "state:0"
             
    } 	
}
***********************************************************/
int cdc_dev_report_status(char *sn, char *data_json, BOOL multi)
{
	int ret = GW_ERR;
    char * json_data = NULL;
	cJSON *state = NULL;

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(data_json, GW_NULL_PARAM);
	
	state = cJSON_Parse(data_json);
	RETURN_IF_NULL(state, GW_NULL_PARAM);
	
	cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		LogE_Prefix("%s\n", "root NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON *system = cJSON_CreateObject();
	if(system == NULL)
	{
		cJSON_Delete(root);
		LogE_Prefix("%s\n", "system NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON *request = cJSON_CreateObject();
	if(request == NULL)
	{
		cJSON_Delete(root);
		cJSON_Delete(system);
		LogE_Prefix("%s\n", "request NULL error");
		return GW_HTTP_FAIL;
	}

	cJSON_AddItemToObject(root,"system",system);
	cJSON_AddStringToObject(system,"ipp",IPP);
    cJSON_AddStringToObject(system,"key",KEY);

	cJSON_AddItemToObject(root,"request",request);
	cJSON_AddStringToObject(request,"devid",sn);
	cJSON_AddItemToObject(request, "status", state);

    json_data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if(NULL != json_data)
	{
		MSG_TYPE_E msg_type = (TRUE == multi)?MSG_MULTI_REPORT_STATUS:MSG_REPORT_STATUS;
		ret = http_send_msg(sn, msg_type, json_data, strlen(json_data), TRUE);		
		FREE_POINTER(json_data);
	}

    return ret;
}
/*******************Device register json content*******************
{
	"system": 
	{
		"ipp":"3.0",
		"key":"key3.0"
	},
	"request": 
	{
		"devlist": 
		[
			{
				"devid":"xxxxx",
				"sn":"xxxxx",
				"productid":"CHHOTWTEST1",
				"swver":"abc",
				"hwver":"test",
				"mac":"00-00-00-00-00-00"
			}
		]
	}

******************************************************************/
int cdc_regist(DEV_INFO_S *dev)
{
	int ret = GW_ERR;
    char * json_data = NULL;

	cJSON *root = cJSON_CreateObject();
	if(root == NULL){
		LogE_Prefix("%s\n", "root NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON *system = cJSON_CreateObject();
	if(system == NULL)
	{
		cJSON_Delete(root);
		LogE_Prefix("%s\n", "system NULL error");
		return GW_HTTP_FAIL;
	}
    //request
	cJSON *request = cJSON_CreateObject();
	if(request == NULL)
	{
		cJSON_Delete(root);
		cJSON_Delete(system);
		LogE_Prefix("%s\n", "request NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON *device_arr = cJSON_CreateArray();
	if(device_arr == NULL)
	{
		cJSON_Delete(root);
		cJSON_Delete(system);
		cJSON_Delete(request);
		LogE_Prefix("%s\n", "device_arr NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON *device = cJSON_CreateObject();
	if(device == NULL)
	{
		cJSON_Delete(root);
		cJSON_Delete(system);
		cJSON_Delete(request);
		cJSON_Delete(device_arr);
		LogE_Prefix("%s\n", "device NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON_AddItemToObject(root,"system", system);
	cJSON_AddStringToObject(system,"ipp", IPP);
    cJSON_AddStringToObject(system,"key", KEY);
	cJSON_AddItemToObject(root,"request", request);
    //devlist
	cJSON_AddItemToObject(request,"devlist", device_arr);
	cJSON_AddItemToArray(device_arr, device);
	cJSON_AddStringToObject(device, "devid", dev->sn);
    cJSON_AddStringToObject(device, "sn", dev->sn);
    cJSON_AddStringToObject(device, "productid", dev->product_id);
    cJSON_AddStringToObject(device, "swver", dev->soft_ver);
    cJSON_AddStringToObject(device, "hwver", dev->soft_ver);
    cJSON_AddStringToObject(device, "mac", dev->mac);

    json_data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if(NULL != json_data)
	{
		BOOL async_msg = (TRUE == dev->is_master)?FALSE:TRUE;
		ret = http_send_msg(dev->sn, MSG_REGIST, json_data, strlen(json_data), async_msg);
		FREE_POINTER(json_data);
	}

	return ret;
}
/*******************Device update json content*********************
{
	"system":
		{
			"ipp":"3.0",
			"key":"xx"
		},
	"request":
		{
			"device": 
			{
				"devid":"ff80808128598r2e0128598a43b40009",
				"swver":"swver 1.1.0",
				"mac":"00-00-00-00-00-00"
			}
		}
}
******************************************************************/
int cdc_update(DEV_INFO_S *dev)
{
	int ret = GW_ERR;
    char * json_data = NULL;

	cJSON *root = cJSON_CreateObject();
	if(root == NULL){
		LogE_Prefix("%s\n", "root NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON *system = cJSON_CreateObject();
	if(system == NULL)
	{
		cJSON_Delete(root);
		LogE_Prefix("%s\n", "system NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON *request = cJSON_CreateObject();
	if(request == NULL)
	{
		cJSON_Delete(root);
		cJSON_Delete(system);
		LogE_Prefix("%s\n", "request NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON *device = cJSON_CreateObject();
	if(device == NULL)
	{
		cJSON_Delete(root);
		cJSON_Delete(system);
		cJSON_Delete(request);
		LogE_Prefix("%s\n", "device NULL error");
		return GW_HTTP_FAIL;
	}
	cJSON_AddItemToObject(root,"system", system);
	cJSON_AddStringToObject(system,"ipp", IPP);
    cJSON_AddStringToObject(system,"key", KEY);
	cJSON_AddItemToObject(root,"request", request);
	cJSON_AddItemToObject(request,"device", device);
	cJSON_AddStringToObject(device,"devid", dev->sn);
	cJSON_AddStringToObject(device,"swver", dev->soft_ver);
	cJSON_AddStringToObject(device,"mac", dev->mac);

    json_data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if(NULL != json_data)
	{
		ret = http_send_msg(dev->sn, MSG_UPDATE_INFO, json_data, strlen(json_data), TRUE);
		FREE_POINTER(json_data);
	}

    return ret;
}

int http_send(char *sn, MSG_TYPE_E msg_type, char *content, int content_len)
{
	int ret = GW_ERR;
	char *resp_data = NULL;
	unsigned int i = 0;
	URL_INFO_S url;

	for(i = 0;i < sizeof(g_http_uri)/sizeof(g_http_uri[0]); i++)
	{
		if(msg_type == g_http_uri[i].type)
		{
			memset(&url, 0, sizeof(url));
			get_cdc_server(url.ip, &url.port);
			url.uri = g_http_uri[i].uri;
			LogI_Prefix("===============http://%s:%d%s===============\n", url.ip, url.port, url.uri);
			resp_data = curl_send(&url, content);
			if(NULL == resp_data)
			{
				LogE_Prefix("%s send failed\n", url.uri);
				return GW_HTTP_FAIL;
			}
			ret = parse_response_json(resp_data);
			FREE_POINTER(resp_data);
		}
	}

	return ret;
}
int http_send_msg(char *sn, MSG_TYPE_E msg_type, char *content, int content_len, BOOL async_send)
{
	int ret = GW_ERR;

	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(content, GW_NULL_PARAM);

	//send http msg syncly
	if(FALSE == async_send)
	{
		ret = http_send(sn, msg_type, content, content_len);
	}
	//send http msg asyncly
	else
	{
		ret = post_to_queue((uint8_t*)content, content_len, sn, msg_type);
	}
	
	return ret;
}

