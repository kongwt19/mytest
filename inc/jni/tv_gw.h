#include <jni.h>


#ifndef _Included_tv_gw
#define _Included_tv_gw
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_tv_gw_setCfgFile(JNIEnv *, jobject, jstring);

JNIEXPORT jint JNICALL Java_tv_gw_initMidWare(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_tv_gw_setGwInfo(JNIEnv *, jobject, jobject);
  
JNIEXPORT void JNICALL Java_tv_gw_regOnlineCallback(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_tv_gw_regOfflineCallback(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_tv_gw_regAlarmCallback(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_tv_gw_setMqttServer(JNIEnv *, jobject, jstring, jshort);

JNIEXPORT void JNICALL Java_tv_gw_setCdcServer(JNIEnv *, jobject,  jstring, jshort);

JNIEXPORT void JNICALL Java_tv_gw_setIppinteServer(JNIEnv *, jobject,  jstring, jshort);

JNIEXPORT jint JNICALL Java_tv_gw_initIotMidWare(JNIEnv *, jobject);
#ifdef __cplusplus
}
#endif
#endif