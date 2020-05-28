LOCAL_PATH:=$(call my-dir)

#Prebuildthe3rdlibraries
ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
	include $(CLEAR_VARS)
	LOCAL_MODULE:= paho-mqtt3a
	LOCAL_SRC_FILES:= ./lib/android/arm_64/libpaho-mqtt3a.so
	include $(PREBUILT_SHARED_LIBRARY)
	include $(CLEAR_VARS)
	LOCAL_MODULE:= voiceRecog
	LOCAL_SRC_FILES:= ./lib/android/arm_64/libvoiceRecog.so
	include $(PREBUILT_SHARED_LIBRARY)
endif

include $(CLEAR_VARS)
LOCAL_MODULE:=JniMethod

LOCAL_SRC_FILES:= $(wildcard $(LOCAL_PATH)/src/jni/*.cpp) \
				$(wildcard $(LOCAL_PATH)/*.c) \
				$(wildcard $(LOCAL_PATH)/src/*.c) \
				$(wildcard $(LOCAL_PATH)/src/service/*.c) \
				$(wildcard $(LOCAL_PATH)/src/mid_ware/*.c) \
				$(wildcard $(LOCAL_PATH)/src/iot_mid_ware/*.c) \
				$(wildcard $(LOCAL_PATH)/src/iot_mid_ware/wifi/*.c) \
				$(wildcard $(LOCAL_PATH)/src/iot_mid_ware/wifi/light_local/*.c) \
				$(wildcard $(LOCAL_PATH)/src/iot_mid_ware/wifi/light_local/agent/*.c) \
				$(wildcard $(LOCAL_PATH)/src/iot_mid_ware/wifi/light_local/common/*.c) \
				$(wildcard $(LOCAL_PATH)/src/iot_mid_ware/wifi/light_local/search/*.c) \
				$(wildcard $(LOCAL_PATH)/src/iot_mid_ware/wifi/light_local/service/*.c) \
				$(wildcard $(LOCAL_PATH)/src/iot_mid_ware/cloud/*.c) \
				$(wildcard $(LOCAL_PATH)/src/iot_mid_ware/cloud/mqtt/*.c) \
				$(wildcard $(LOCAL_PATH)/src/common/*.c) \
				$(wildcard $(LOCAL_PATH)/src/common/linux/*.c) \
				$(wildcard $(LOCAL_PATH)/src/iot_mid_ware/zigbee/*.c)
				
LOCAL_C_INCLUDES:= -I $(LOCAL_PATH)/inc \
				  -I $(LOCAL_PATH)/inc/jni \
				  -I $(LOCAL_PATH)/inc/common \
				  -I $(LOCAL_PATH)/inc/common/android \
				  -I $(LOCAL_PATH)/inc/service \
				  -I $(LOCAL_PATH)/inc/mid_ware \
				  -I $(LOCAL_PATH)/inc/iot_mid_ware \
				  -I $(LOCAL_PATH)/inc/iot_mid_ware/zigbee \
				  -I $(LOCAL_PATH)/inc/iot_mid_ware/wifi \
				  -I $(LOCAL_PATH)/inc/iot_mid_ware/wifi/light_local \
				  -I $(LOCAL_PATH)/inc/iot_mid_ware/wifi/light_local/agent \
				  -I $(LOCAL_PATH)/inc/iot_mid_ware/wifi/light_local/common \
				  -I $(LOCAL_PATH)/inc/iot_mid_ware/wifi/light_local/search \
				  -I $(LOCAL_PATH)/inc/iot_mid_ware/wifi/light_local/service \
				  -I $(LOCAL_PATH)/inc/iot_mid_ware/cloud \
				  -I $(LOCAL_PATH)/inc/iot_mid_ware/cloud/mqtt \
				  -I $(LOCAL_PATH)/inc/iot_mid_ware/cloud/mqtt/paho-mqtt3c
				  
$(warning $(LOCAL_C_INCLUDES))
LOCAL_LDLIBS := -lc -L$(SYSROOT)/usr/lib -llog
LOCAL_LDFLAGS := $(LOCAL_C_INCLUDES) -Wall -fPIC -fuse-ld=bfd -D
LOCAL_CFLAGS := -DZIGBEE_ENABLE

ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
	LOCAL_SHARED_LIBRARIES:= paho-mqtt3a voiceRecog
endif
include $(BUILD_SHARED_LIBRARY)