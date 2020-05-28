TARGET := soft_gw

ZIGBEE		= false
PLATFORM	= x86_64

ifeq ($(PLATFORM), arm64)
	CXX		:= aarch64-linux-gnu-g++
	CC		:= aarch64-linux-gnu-gcc
else
	CXX			:= g++
	CC			:= gcc
endif

CPP_SOURCE := 	$(wildcard ./*.cpp)
				
CPP_OBJS := $(patsubst %.cpp,%.o,$(CPP_SOURCE))

C_SOURCE := 	$(wildcard ./*.c) \
				$(wildcard ./src/*.c) \
				$(wildcard ./src/service/*.c) \
				$(wildcard ./src/mid_ware/*.c) \
				$(wildcard ./src/iot_mid_ware/*.c) \
				$(wildcard ./src/iot_mid_ware/wifi/*.c) \
				$(wildcard ./src/iot_mid_ware/wifi/light_local/*.c) \
				$(wildcard ./src/iot_mid_ware/wifi/light_local/agent/*.c) \
				$(wildcard ./src/iot_mid_ware/wifi/light_local/common/*.c) \
				$(wildcard ./src/iot_mid_ware/wifi/light_local/search/*.c) \
				$(wildcard ./src/iot_mid_ware/wifi/light_local/service/*.c) \
				$(wildcard ./src/iot_mid_ware/cloud/*.c) \
				$(wildcard ./src/iot_mid_ware/cloud/mqtt/*.c) \
				$(wildcard ./src/common/*.c) \
				$(wildcard ./src/iot_mid_ware/zigbee/*.c)

INCLUDE :=	-I ./ \
			-I ./inc/common \
			-I ./inc/common/curl \
			-I ./inc/service \
			-I ./inc/mid_ware \
			-I ./inc/iot_mid_ware \
			-I ./inc/iot_mid_ware/zigbee \
			-I ./inc/iot_mid_ware/wifi \
			-I ./inc/iot_mid_ware/wifi/light_local \
			-I ./inc/iot_mid_ware/wifi/light_local/agent \
			-I ./inc/iot_mid_ware/wifi/light_local/common \
			-I ./inc/iot_mid_ware/wifi/light_local/search \
			-I ./inc/iot_mid_ware/wifi/light_local/service \
			-I ./inc/iot_mid_ware/cloud \
			-I ./inc/iot_mid_ware/cloud/mqtt \
			-I ./inc/iot_mid_ware/cloud/mqtt/paho-mqtt3c \

DEFINES := -DHAVE_CONFIG_H -DLINUX -DREX_ENABLE
LIBS := -lpaho-mqtt3a -lpthread
LDFLAGS :=

ifeq ($(PLATFORM), arm64)
	C_SOURCE += $(wildcard ./src/common/linux/*.c)
	INCLUDE += -I ./inc/common/linux
	LDFLAGS += -L ./lib/linux/arm_64
endif

ifeq ($(PLATFORM), x86_64)
	C_SOURCE += $(wildcard ./src/common/linux/*.c)
	INCLUDE += -I ./inc/common/linux
	LDFLAGS += -L ./lib/linux/x86_64
endif

ifeq ($(PLATFORM), win)
	C_SOURCE += $(wildcard ./src/common/win/*.c)
	INCLUDE += -I ./inc/common/win
	LDFLAGS += -L ./lib/win -lws2_32
endif

ifeq ($(ZIGBEE), true)
	DEFINES := -DHAVE_CONFIG_H -DLINUX -DZIGBEE_ENABLE
	LIBS	+= -lrexgatewaysdk -lpthread
endif
				
C_OBJS := $(patsubst %.c,%.o,$(C_SOURCE))
CFLAGS := -g -Wall -static -fno-stack-protector

all:$(TARGET)

$(TARGET):$(CPP_OBJS) $(C_OBJS)
	$(CXX) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
$(CPP_OBJS):%o:%cpp
	$(CXX) -o $@ -c $<  $(DEFINES) $(INCLUDE) $(CFLAGS) $(CXX_VER)
$(C_OBJS):%o:%c
	$(CC) -o $@ -c $< $(DEFINES) $(INCLUDE) $(CFLAGS)

clean:
	rm -rf $(CPP_OBJS) $(C_OBJS) $(TARGET)
