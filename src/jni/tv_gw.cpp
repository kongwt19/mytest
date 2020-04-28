#include "tv_gw.h"
#include "mid_ware.h"
#include "iot_mid_ware.h"
#include "gw_service.h"
#include "dev_mng.h"
#include "dev_cfg.h"

static JNIEnv* Env;
static jobject j_obj = NULL;
static jmethodID online_callback_id = NULL;
static jmethodID offline_callback_id = NULL;
static jmethodID alarm_callback_id = NULL;


class GW{
	
public:
    GW();
	void gw_set_cfg_file(char *file);
	int gw_init_mid_ware();
	void gw_set_gw_info(DEV_INFO_S *gw);
	void gw_reg_online_callback(ONLINE_CALLBACK cb);
	void gw_reg_offline_callback(OFFLINE_CALLBACK cb);
	void gw_reg_alarm_callback(ALARM_CALLBACK cb);
	void gw_set_mqtt_server(const char *ip, const short port);
	void gw_set_cdc_server(const char *ip, const short port);
	void gw_set_ippinte_server(const char *ip, const short port);
	int gw_init_iot_mid_ware(void);
    ~GW();

};


GW::GW()
{
    
}
GW::~GW()
{
}

void GW::gw_set_cfg_file(char *file)
{
    set_cfg_file(file);
}

int GW::gw_init_mid_ware()
{
	return init_mid_ware();
}

void GW::gw_set_gw_info(DEV_INFO_S *gw)
{
    set_gw_info(gw);
}

void GW::gw_reg_online_callback(ONLINE_CALLBACK cb)
{
    reg_online_callback(cb);
}

void GW::gw_reg_offline_callback(OFFLINE_CALLBACK cb)
{
    reg_offline_callback(cb);
}

void GW::gw_reg_alarm_callback(ALARM_CALLBACK cb)
{
    reg_alarm_callback(cb);
}

void GW::gw_set_mqtt_server(const char *ip, const short port)
{
    set_mqtt_server(ip, port);
}

void GW::gw_set_cdc_server(const char *ip, const short port)
{
    set_cdc_server(ip, port);
}

void GW::gw_set_ippinte_server(const char *ip, const short port)
{
    set_ippinte_server(ip, port);
}

int GW::gw_init_iot_mid_ware()
{
    return init_iot_mid_ware();
}

jstring charTojstring(JNIEnv* env, const char* pat) {
    //????java String?? strClass
    jclass strClass = (env)->FindClass("Ljava/lang/String;");
    //???String(byte[],String)?L?????,?????????byte[]????????h????String
    jmethodID ctorID = (env)->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
    //????byte????
    jbyteArray bytes = (env)->NewByteArray(strlen(pat));
    //??char* ????byte????
    (env)->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte*) pat);
    // ????String, ????????????,????byte?????????String??????
    jstring encoding = (env)->NewStringUTF("GB2312");
    //??byte????????java String,?????
    return (jstring) (env)->NewObject(strClass, ctorID, bytes, encoding);
}


void online_callback(char *sn)
{
	jstring jsn;
	if(sn != NULL)
	{
		jsn = charTojstring(Env, sn);
	}
	else
	{
		return;
	}
	if(j_obj != NULL && online_callback_id != NULL)
	{
		(*Env).CallVoidMethod(j_obj, online_callback_id, jsn);
	}
	else
	{
		return;
	}
}

void offline_callback(char *sn)
{
	jstring jsn;
	if(sn != NULL)
	{
		jsn = charTojstring(Env, sn);
	}
	else
	{
		return;
	}
	if(j_obj != NULL && offline_callback_id != NULL)
	{
		(*Env).CallVoidMethod(j_obj, offline_callback_id, jsn);
	}
	else
	{
		return;
	}
}

void alarm_callback(char *sn, char *alarm)
{
	jstring jsn;
	jstring jalarm;
	if(sn != NULL && alarm != NULL)
	{
		jsn = charTojstring(Env, sn);
		jalarm = charTojstring(Env, alarm);
	}
	else
	{
		return;
	}
	if(j_obj != NULL && alarm_callback_id != NULL)
	{
		(*Env).CallVoidMethod(j_obj, alarm_callback_id, jsn, jalarm);
	}
	else
	{
		return;
	}
}



JNIEXPORT void JNICALL Java_tv_gw_setCfgFile(JNIEnv *env, jobject, jstring file)
{
	GW t;      //????????
	char* str_file=(char *)(*env).GetStringUTFChars(file, NULL);
    t.gw_set_cfg_file(str_file);
	(*env).ReleaseStringUTFChars(file, str_file);
}

JNIEXPORT jint JNICALL Java_tv_gw_initMidWare(JNIEnv *env, jobject)
{
	GW t;
    return t.gw_init_mid_ware();
}

JNIEXPORT void JNICALL Java_tv_gw_setGwInfo(JNIEnv *env, jobject, jobject g_w)
{
	GW t;
	DEV_INFO_S gw;
	memset(&gw, 0, sizeof(gw));
	jclass objectClass = (*env).GetObjectClass(g_w);
	if (!objectClass) {
		LogD_Prefix("EventHandler: failed to get class reference\n");
		return;
	}
	
	jmethodID getSn_method = (*env).GetMethodID(objectClass, "getsn", "()Ljava/lang/String;");
    if (getSn_method == 0) 
	{
		LogD_Prefix("getSn_method: failed to get class reference\n");
		return;
	}
	jstring sn_str = static_cast<jstring>((*env).CallObjectMethod(g_w, getSn_method));
	if (sn_str == NULL) 
	{
		LogD_Prefix("sn_str: failed to get class reference\n");
		return;
	}
    char *sn = (char *)(*env).GetStringUTFChars(sn_str, NULL);
	
	jmethodID getIp_method = (*env).GetMethodID(objectClass, "getip", "()Ljava/lang/String;");
    if (getIp_method == 0) 
	{
		LogD_Prefix("getIp_method: failed to get class reference\n");
		return;
	}
	jstring ip_str = static_cast<jstring>((*env).CallObjectMethod(g_w, getIp_method));
	if (ip_str == NULL) 
	{
		LogD_Prefix("ip_str: failed to get class reference\n");
		return;
	}
    char *ip = (char *)(*env).GetStringUTFChars(ip_str, NULL);
	
	jmethodID getMac_method = (*env).GetMethodID(objectClass, "getmac", "()Ljava/lang/String;");
    if (getMac_method == 0) 
	{
		LogD_Prefix("getMac_method: failed to get class reference\n");
		return;
	}
	jstring mac_str = static_cast<jstring>((*env).CallObjectMethod(g_w, getMac_method));  
	if (mac_str == NULL) 
	{
		LogD_Prefix("mac_str: failed to get class reference\n");
		return;
	}
    char *mac = (char *)(*env).GetStringUTFChars(mac_str, NULL);
	
	jmethodID getProduct_id_method = (*env).GetMethodID(objectClass, "getproduct_id", "()Ljava/lang/String;");
    if (getProduct_id_method == 0) 
	{
		LogD_Prefix("getProduct_id_method: failed to get class reference\n");
		return;
	}
	jstring product_id_str = static_cast<jstring>((*env).CallObjectMethod(g_w, getProduct_id_method));  
	if (product_id_str == NULL) 
	{
		LogD_Prefix("product_id_str: failed to get class reference\n");
		return;
	}
    char *product_id = (char *)(*env).GetStringUTFChars(product_id_str, NULL);
	
	jmethodID getSoft_ver_method = (*env).GetMethodID(objectClass, "getsoft_ver", "()Ljava/lang/String;");
    if (getSoft_ver_method == 0) 
	{
		LogD_Prefix("getSoft_ver_method: failed to get class reference\n");
		return;
	}
	jstring soft_ver_str = static_cast<jstring>((*env).CallObjectMethod(g_w, getSoft_ver_method)); 
	if (soft_ver_str == NULL) 
	{
		LogD_Prefix("soft_cer_str: failed to get class reference\n");
		return;
	}	
    char *soft_ver = (char *)(*env).GetStringUTFChars(soft_ver_str, NULL);
	
	jmethodID getIs_master_method = (*env).GetMethodID(objectClass, "getis_master", "()Z");
    if (getIs_master_method == 0) 
	{
		LogD_Prefix("getIs_master_method: failed to get class reference\n");
		return;
	}
	jboolean is_master_n = static_cast<jboolean>((*env).CallBooleanMethod(g_w, getIs_master_method)); 
    BOOL is_master = (BOOL)is_master_n;
	
	strncpy(gw.sn, sn, sizeof(gw.sn));
	strncpy(gw.product_id, product_id, sizeof(gw.product_id));
	strncpy(gw.soft_ver, soft_ver, sizeof(gw.soft_ver));
	strncpy(gw.ip, ip, sizeof(gw.ip));
	strncpy(gw.mac, mac, sizeof(gw.mac));
	gw.is_master = is_master;
	t.gw_set_gw_info(&gw); 
	(*env).ReleaseStringUTFChars(sn_str, sn);
	(*env).ReleaseStringUTFChars(ip_str, ip);
	(*env).ReleaseStringUTFChars(mac_str, mac);
	(*env).ReleaseStringUTFChars(product_id_str, product_id);
	(*env).ReleaseStringUTFChars(soft_ver_str, soft_ver);
/*	

	jmethodID getGw_sn_method = (*env).GetMethodID(objectClass, "getgw_sn", "()Ljava/lang/String;");
    if (getGw_sn_method == 0) 
	{
		LogD_Prefix("getGw_sn_method: failed to get class reference\n");
		return;
	}
	jstring gw_sn_str = static_cast<jstring>((*env).CallObjectMethod(g_w, getGw_sn_method));  
	if (gw_sn_str == NULL) 
	{
		LogD_Prefix("gw_sn_str: failed to get class reference\n");
		return;
	}
    gw.gw_sn_str = (char *)(*env).GetStringUTFChars(gw_sn_str, NULL);
	
	jmethodID getConnectType_method = (*env).GetMethodID(objectClass, "getconnectType", "()I");
    if (getConnectType_method == 0) 
	{
		LogD_Prefix("getConnectType_method: failed to get class reference\n");
		return;
	}
	jint connectType_n = static_cast<jint>((*env).CallIntMethod(g_w, getConnectType_method));  
    gw.connectType = (int)connectType_n;
	
	jmethodID getLong_addr_method = (*env).GetMethodID(objectClass, "getlong_addr", "()J");
    if (getLong_addr_method == 0) 
	{
		LogD_Prefix("getLong_addr_method: failed to get class reference\n");
		return;
	}
	jlong long_addr_n = static_cast<jlong>((*env).CallLongMethod(g_w, getLong_addr_method));  
    gw.child_info.zigbee.long_addr = (uint64_t)long_addr_n;
	
	jmethodID getType_method = (*env).GetMethodID(objectClass, "gettype", "()C");
    if (getType_method == 0) 
	{
		LogD_Prefix("getType_method: failed to get class reference\n");
		return;
	}
	jchar type_n = static_cast<jchar>((*env).CallCharMethod(g_w, getType_method));  
    gw.child_info.zigbee.type = (uint8_t)type_n;
	
	jmethodID getNode_number_method = (*env).GetMethodID(objectClass, "getnode_number", "()C");
    if (getNode_number_method == 0) 
	{
		LogD_Prefix("getNode_number_method: failed to get class reference\n");
		return;
	}
	jchar node_number_n = static_cast<jchar>((*env).CallCharMethod(g_w, getNode_number_method));  
    gw.child_info.zigbee.node_number = (uint8_t)node_number_n;
	
	jmethodID getReserve_method = (*env).GetMethodID(objectClass, "getreserve", "()J");
    if (getReserve_method == 0) 
	{
		LogD_Prefix("getReserve_method: failed to get class reference\n");
		return;
	}
	jlong reserve_n = static_cast<jlong>((*env).CallLongMethod(g_w, getReserve_method));  
    gw.child_info.bt.reserve = (uint64_t)reserve_n;
	
	jmethodID getReserver_method = (*env).GetMethodID(objectClass, "getreserver", "()J");
    if (getReserver_method == 0) 
	{
		LogD_Prefix("getReserver_method: failed to get class reference\n");
		return;
	}
	jlong reserver_n = static_cast<jlong>((*env).CallLongMethod(g_w, getReserver_method));  
    gw.child_info.bt.reserver = (uint64_t)reserver_n;*/

}


JNIEXPORT void JNICALL Java_tv_gw_regOnlineCallback(JNIEnv *env, jobject obj)
{
	Env = env;
    j_obj = env->NewGlobalRef(obj);//???????h????
	jclass clazz = env->GetObjectClass(obj);
	if (!clazz) {
		LogD_Prefix("OnlineCallback: failed to get class reference\n");
		return;
	}
	jmethodID methodID = (*env).GetMethodID(clazz, "onlinecallback", "(Ljava/lang/String;)I");
	if (methodID == 0) 
	{
		LogD_Prefix("OnlineCallback methodID: failed to get class reference\n");
		return;
	}
	online_callback_id = methodID;
	GW t;
	t.gw_reg_online_callback(online_callback);
}

JNIEXPORT void JNICALL Java_tv_gw_regOfflineCallback(JNIEnv *env, jobject obj)
{
	jclass clazz = env->GetObjectClass(obj);
	if (!clazz) {
		LogD_Prefix("OfflineCallback: failed to get class reference\n");
		return;
	}
	jmethodID methodID = (*env).GetMethodID(clazz, "offlinecallback", "(Ljava/lang/String;)I");
	if (methodID == 0) 
	{
		LogD_Prefix("OfflineCallback methodID: failed to get class reference\n");
		return;
	}
	offline_callback_id = methodID;
	GW t;
	t.gw_reg_offline_callback(offline_callback);
}

JNIEXPORT void JNICALL Java_tv_gw_regAlarmCallback(JNIEnv *env, jobject obj)
{
	jclass clazz = env->GetObjectClass(obj);
	if (!clazz) {
		LogD_Prefix("AlarmCallback: failed to get class reference\n");
		return;
	}
	jmethodID methodID = (*env).GetMethodID(clazz, "alarmcallback", "(Ljava/lang/String;Ljava/lang/String;)I");
	if (methodID == 0) 
	{
		LogD_Prefix("AlarmCallback methodID: failed to get class reference\n");
		return;
	}
	alarm_callback_id = methodID;
	GW t;
	t.gw_reg_alarm_callback(alarm_callback);
}


JNIEXPORT void JNICALL Java_tv_gw_setMqttServer(JNIEnv *env, jobject, jstring ip, jshort port)
{
	GW t;
	const char* mqtt_ip   = (*env).GetStringUTFChars(ip, NULL);
	const short mqtt_port = (const short)port;
    t.gw_set_mqtt_server(mqtt_ip, mqtt_port);
	(*env).ReleaseStringUTFChars(ip, mqtt_ip);
}

JNIEXPORT void JNICALL Java_tv_gw_setCdcServer(JNIEnv *env, jobject, jstring ip, jshort port)
{
	GW t;
	const char* cdc_ip   = (*env).GetStringUTFChars(ip, NULL);
	const short cdc_port = (const short)port;
    t.gw_set_cdc_server(cdc_ip, cdc_port);
	(*env).ReleaseStringUTFChars(ip, cdc_ip);
}

JNIEXPORT void JNICALL Java_tv_gw_setIppinteServer(JNIEnv *env, jobject, jstring ip, jshort port)
{
	GW t;
	const char* ippinte_ip   = (*env).GetStringUTFChars(ip, NULL);
	const short ippinte_port = (const short)port;
    t.gw_set_ippinte_server(ippinte_ip, ippinte_port);
	(*env).ReleaseStringUTFChars(ip, ippinte_ip);
}

JNIEXPORT jint JNICALL Java_tv_gw_initIotMidWare(JNIEnv *, jobject)
{
	GW t;
    return t.gw_init_iot_mid_ware();
}


