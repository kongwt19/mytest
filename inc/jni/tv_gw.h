/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_smart_changhong_smartipptv_JniMethod */

#ifndef _Included_com_smart_changhong_smartipptv_JniMethod
#define _Included_com_smart_changhong_smartipptv_JniMethod
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_setCfgFile(JNIEnv *, jobject, jstring);

JNIEXPORT jint JNICALL Java_com_smart_changhong_smartipptv_JniMethod_initMidWare(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_setGwInfo(JNIEnv *, jobject, jobject);
  
JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_regOnlineCallback(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_regOfflineCallback(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_regAlarmCallback(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_regZigbeeCallback(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_setMqttServer(JNIEnv *, jobject, jstring, jshort);

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_setCdcServer(JNIEnv *, jobject,  jstring, jshort);

JNIEXPORT void JNICALL Java_com_smart_changhong_smartipptv_JniMethod_setIppinteServer(JNIEnv *, jobject,  jstring, jshort);

JNIEXPORT jint JNICALL Java_com_smart_changhong_smartipptv_JniMethod_initIotMidWare(JNIEnv *, jobject);
#ifdef __cplusplus
}
#endif
#endif