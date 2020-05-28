#include "tv_gw.h"
#include "mid_ware.h"
#include "iot_mid_ware.h"
#include "gw_service.h"
#include "dev_mng.h"
#include "dev_cfg.h"
#include <string.h>
#include <wchar.h>


static JavaVM* vm;
static jobject j_obj;
jmethodID onlineCallback = NULL;
jmethodID offlineCallback = NULL;
jmethodID alarmCallback = NULL;
jmethodID zigbeeCallback = NULL;
bool needDetach = false;

static JNIEnv* getJNIEnv(JavaVM* pJavaVM)
{
   	JNIEnv* lEnv;
   	if((pJavaVM)->AttachCurrentThread((void **)&lEnv, NULL) != JNI_OK){
   		lEnv = NULL;
   	}
   	return lEnv;
}

jstring cTojs(JNIEnv* env, char* str)
{
	const size_t cSize = strlen(str)+1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs (wc, str, cSize);
	size_t len = wcslen(wc);
	jchar* str2 = (jchar*)malloc(sizeof(jchar)*(len+1));
	int i;
	for (i = 0; i < len; i++)
	{
		str2[i] = wc[i];
	}
	str2[len] = 0;
	jstring js = env->NewString(str2, len);
	free(str2);
	return js;
}

class GW{
	
public:
    GW();
	void gw_set_cfg_file(char *file);
	int gw_init_mid_ware();
	void gw_set_gw_info(DEV_INFO_S *gw);
	void gw_reg_online_callback(ONLINE_CALLBACK cb);
	void gw_reg_offline_callback(OFFLINE_CALLBACK cb);
	void gw_reg_alarm_callback(ALARM_CALLBACK cb);
	void gw_reg_zigbee_callback(ZIGBEE_CALLBACK cb);
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

void GW::gw_reg_zigbee_callback(ZIGBEE_CALLBACK cb)
{
	reg_zigbee_callback(cb);
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

void online_callback(char *sn, char *product_id)
{
	JNIEnv* jniEnv = __null;
	jstring jsn;
	jstring jproduct_id;
	if (vm->GetEnv(reinterpret_cast<void **> (&jniEnv), JNI_VERSION_1_6) != JNI_OK)
	{
		jniEnv = getJNIEnv(vm);  //锟斤拷锟斤拷锟斤拷一锟轿达拷锟斤拷
		needDetach = true;
	}
	if(sn != NULL)
	{
		jsn = cTojs(jniEnv, sn);
		jproduct_id = cTojs(jniEnv, product_id);
	}
	else
	{
		if(needDetach)
		{
			vm->DetachCurrentThread();
		}
		return;
	}
	(*jniEnv).CallIntMethod(j_obj, onlineCallback, jsn, jproduct_id);
	if(needDetach)
	{
		vm->DetachCurrentThread();
	}
}

void offline_callback(char *sn, char *product_id)
{
	JNIEnv* jniEnv = __null;
	jstring jsn;
	jstring jproduct_id;
	if (vm->GetEnv(reinterpret_cast<void **> (&jniEnv), JNI_VERSION_1_6) != JNI_OK)
	{
		jniEnv = getJNIEnv(vm);  //锟斤拷锟斤拷锟斤拷一锟轿达拷锟斤拷
		needDetach = true;
	}
	if(sn != NULL)
	{
		jsn = cTojs(jniEnv, sn);
		jproduct_id = cTojs(jniEnv, product_id);
	}
	else
	{
		if(needDetach)
		{
			vm->DetachCurrentThread();
		}
		return;
	}
	(*jniEnv).CallIntMethod(j_obj, offlineCallback, jsn, jproduct_id);
	if(needDetach)
	{
		vm->DetachCurrentThread();
	}
}

void alarm_callback(char *sn, char *alarm)
{
	JNIEnv* jniEnv = __null;
	jstring jsn;
	jstring jalarm;
	if (vm->GetEnv(reinterpret_cast<void **> (&jniEnv), JNI_VERSION_1_6) != JNI_OK)
	{
		jniEnv = getJNIEnv(vm);  //锟斤拷锟斤拷锟斤拷一锟轿达拷锟斤拷
		needDetach = true;
	}
	if(sn != NULL && alarm != NULL)
	{
		jsn = cTojs(jniEnv, sn);
		jalarm = cTojs(jniEnv, alarm);
	}
	else
	{
		if(needDetach)
		{
			vm->DetachCurrentThread();
		}
		return;
	}
	(*jniEnv).CallIntMethod(j_obj, alarmCallback, jsn, jalarm);
	if(needDetach)
	{
		vm->DetachCurrentThread();
	}
}

void zigbee_callback(int flag)
{
	JNIEnv* jniEnv = __null;
	if (vm->GetEnv(reinterpret_cast<void **> (&jniEnv), JNI_VERSION_1_6) != JNI_OK)
	{
		jniEnv = getJNIEnv(vm);  //锟斤拷锟斤拷锟斤拷一锟轿达拷锟斤拷
		needDetach = true;
	}
	(*jniEnv).CallIntMethod(j_obj, zigbeeCallback, (jint)flag);
	if(needDetach)
	{
		vm->DetachCurrentThread();
	}
}



JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_setCfgFile(JNIEnv *env, jobject, jstring file)
{
	GW t;      //????????
	char* str_file=(char *)(*env).GetStringUTFChars(file, NULL);
    t.gw_set_cfg_file(str_file);
	(*env).ReleaseStringUTFChars(file, str_file);
}

JNIEXPORT jint JNICALL Java_com_smart_changhong_smartipptv_JniMethod_initMidWare(JNIEnv *env, jobject)
{
	GW t;
    return t.gw_init_mid_ware();
}

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_setGwInfo(JNIEnv *env, jobject, jobject g_w)
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


JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_regOnlineCallback(JNIEnv *env, jobject obj)
{
	env->GetJavaVM(&vm);
	if(vm == NULL)
	{
		LogD_Prefix("GetJavaVM: failed to get reference");
		return;
	}
	j_obj = env->NewGlobalRef(obj);
	jclass clazz = env->GetObjectClass(obj);
	if (!clazz) {
		LogD_Prefix("OnlineCallback: failed to get class reference\n");
		return;
	}
	jmethodID methodID = (*env).GetMethodID(clazz, "onlinecallback", "(Ljava/lang/String;Ljava/lang/String;)I");
	if (methodID == NULL)
	{
		LogD_Prefix("OnlineCallback methodID: failed to get class reference\n");
		return;
	}
	onlineCallback = methodID;
	GW t;
	t.gw_reg_online_callback(online_callback);
}

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_regOfflineCallback(JNIEnv *env, jobject obj)
{
	jclass clazz = env->GetObjectClass(obj);
	if (!clazz) {
		LogD_Prefix("OfflineCallback: failed to get class reference\n");
		return;
	}
	jmethodID methodID = (*env).GetMethodID(clazz, "offlinecallback", "(Ljava/lang/String;Ljava/lang/String;)I");
	if (methodID == NULL)
	{
		LogD_Prefix("OfflineCallback methodID: failed to get class reference\n");
		return;
	}
	offlineCallback = methodID;
	GW t;
	t.gw_reg_offline_callback(offline_callback);
}

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_regAlarmCallback(JNIEnv *env, jobject obj)
{
	jclass clazz = env->GetObjectClass(obj);
	if (!clazz) {
		LogD_Prefix("AlarmCallback: failed to get class reference\n");
		return;
	}
	jmethodID methodID = (*env).GetMethodID(clazz, "alarmcallback", "(Ljava/lang/String;Ljava/lang/String;)I");
	if (methodID == NULL)
	{
		LogD_Prefix("AlarmCallback methodID: failed to get class reference\n");
		return;
	}
	alarmCallback = methodID;
	GW t;
	t.gw_reg_alarm_callback(alarm_callback);
}

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_regZigbeeCallback(JNIEnv *env, jobject obj)
{
	jclass clazz = env->GetObjectClass(obj);
	if (!clazz) {
		LogD_Prefix("ZigbeeCallback: failed to get class reference\n");
		return;
	}
	jmethodID methodID = (*env).GetMethodID(clazz, "zigbeecallback", "(I)I");
	if (methodID == NULL)
	{
		LogD_Prefix("ZigbeeCallback methodID: failed to get class reference\n");
		return;
	}
	zigbeeCallback = methodID;
	GW t;
	t.gw_reg_zigbee_callback(zigbee_callback);
}


JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_setMqttServer(JNIEnv *env, jobject, jstring ip, jshort port)
{
	GW t;
	const char* mqtt_ip   = (*env).GetStringUTFChars(ip, NULL);
	const short mqtt_port = (const short)port;
    t.gw_set_mqtt_server(mqtt_ip, mqtt_port);
	(*env).ReleaseStringUTFChars(ip, mqtt_ip);
}

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_setCdcServer(JNIEnv *env, jobject, jstring ip, jshort port)
{
	GW t;
	const char* cdc_ip   = (*env).GetStringUTFChars(ip, NULL);
	const short cdc_port = (const short)port;
    t.gw_set_cdc_server(cdc_ip, cdc_port);
	(*env).ReleaseStringUTFChars(ip, cdc_ip);
}

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_setIppinteServer(JNIEnv *env, jobject, jstring ip, jshort port)
{
	GW t;
	const char* ippinte_ip   = (*env).GetStringUTFChars(ip, NULL);
	const short ippinte_port = (const short)port;
    t.gw_set_ippinte_server(ippinte_ip, ippinte_port);
	(*env).ReleaseStringUTFChars(ip, ippinte_ip);
}

JNIEXPORT jint JNICALL Java_com_smart_changhong_smartipptv_JniMethod_initIotMidWare(JNIEnv *, jobject)
{
	GW t;
    return t.gw_init_iot_mid_ware();
}


