#ifndef __REX_COMMON_H__
#define __REX_COMMON_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

/** defined constant */
#define MAIN_LOG_LEVEL 							0
#define MAIN_LOG_MAX_SIZE 						1048576
#define MAIN_TRACK_LEVEL 						3
#define MAIN_SERIAL_NAME 						"/dev/ttyUSB0"
#define MAIN_SERIAL_BAUD_RATE 					115200
#define MAIN_CLOUD_SERVER_ADDRESS 				"192.168.1.91"
#define MAIN_CLOUD_SERVER_PORT 					60000
#define MAIN_OTA_IMAGE_FILE_PATH 				"/data/local/tmp"
#define MAIN_RUN_MODE 							0

/** define constant */
#define PROFILE_HELPER_ITEM_VALUE_LEN 			100
/** defined constant */
#define JSON_MAX_LEN 				            1024

#define VC_MAX_TIME                             360


#define JOIN_NETWORK_OPEN						0
#define JOIN_NETWORK_CLOSE						-1



/* 设备生产ID */
#define DEV_PRODUCT_GATEWAY						"SLIFE_GW0001_hyzn002"
#define DEV_PRODUCT_SCENE						"SLIFE_GW0001_hyzn001"

#define DEV_PRODUCT_ONEWAY_PLUG					"SMH01_SOCK01_hyzn001"
#define DEV_PRODUCT_TWOWAY_PLUG					"SMH01_SOCK01_hyzn002"
#define DEV_PRODUCT_THREEWAY_PLUG				"SMH01_SOCK01_hyzn003"
#define DEV_PRODUCT_FOURWAY_PLUG				"SMH01_SOCK01_hyzn004"

#define DEV_PRODUCT_ONEWAY_LIGHT				"SMH01_SWTH01_hyzn001"
#define DEV_PRODUCT_TWOWAY_LIGHT				"SMH01_SWTH01_hyzn002"
#define DEV_PRODUCT_THREEWAY_LIGHT				"SMH01_SWTH01_hyzn003"
#define DEV_PRODUCT_FOURWAY_LIGHT				"SMH01_SWTH01_hyzn004"

#define DEV_PRODUCT_ONEWAY_CURTAIN				"SMH01_BLIND1_hyzn001"


#define agent_strncpy(dest,src,n)\
do \
{	\
	strncpy(dest, src, n);\
	dest[n] = '\0';\
} while (0);


typedef struct data_item{
	uint8_t state;
	uint8_t level;
	uint32_t temperature;
	uint32_t times;
	uint8_t groud_id;
	uint8_t scene_id;
	uint8_t *scene_name;
} DataItem, *DItem;


//应用层下发命令类型
typedef enum
{
	user_get_net = 0,				//获取网络信息
	user_join_net,					//准许入网
	user_stop_net,					//拒绝入网
	user_set_net,					//设置网络信息
	user_get_gateway_ver,			//获取网关版本
	user_get_gateway_info,			//获取网关认证信息
	user_creat_net,					//建立网络
	user_set_factor,				//恢复出厂设置
	
	user_curtain_on,				//开窗帘
	user_curtain_close,				//关窗帘
	user_curtain_stop,				//停止窗帘
	user_curtain_goto_percent,		//设置窗帘位置

	user_light_turn_on,				//开灯
	user_light_turn_off,			//关灯
	user_light_change_level,		//灯亮度

	user_plug_turn_on,				//开插座
	user_plug_turn_off,				//关插座
	user_plug_config_cprrect,       //配置校准
	user_plug_auto_cprrect,			//自动校准
	user_plug_set_waittime,			//开设置延迟

	user_sound_light_alarm,			//产生声光报警

	user_air_temp,					//空调温度
	user_air_hum,					//空调湿度
	user_air_wind_model,       		//空调送风模式
	user_air_work_model,			//空调工作模式
	user_air_get_info,				//获取空调信息

	user_device_status,				//检测设备状态
	user_clear_device,				//设备注销
	user_clear_all_devices,			//设备所有注销
	user_device_info,				//设备信息
	user_creat_groupid,				//创建分组
	user_delete_grouid,				//删除分组
	user_delete_all_grouid,			//删除所有分组
	user_query_grouid,				//查询分组
	user_creat_scene,				//创建场景
	user_delete_scene,				//删除场景
	user_delete_all_scene,			//删除所有场景
	user_run_scene,					//执行场景
	user_query_scene,				//查询场景
	user_get_bat_info,				//获取电池信息
	user_device_bind,				//设备绑定
	user_call_device,				//设备点名
	user_sync_clock					//同步时钟
	
}USER_CONTROL_OPTION_E;


//下发SDK设备控制命令类型
typedef enum
{
	//网关系统命令
	ZIGBEE_GET_NET					= 0x0100,   //获取网络信息
	ZIGBEE_JOIN_NET					= 0x0101,   //入网
	ZIGBEE_SET_NET					= 0x0102,   //设置网络信息
	ZIGBEE_CREAT_NET				= 0x01F0,   //创建网络
	ZIGBEE_SET_FACTOR				= 0x0124,   //恢复出厂设置

	//插座命令
	ZIGBEE_PLUG_SET_STATE			= 0x0105,   //插座状态
	ZIGBEE_PLUG_SET_CPRRECT			= 0x01F4,   //插座校准
	ZIGBEE_PLUG_AUTO_CPRRECT		= 0x01F5,   //自动校准
	
	//灯开关命令
	ZIGBEE_LIGHT_SET_STATE			= 0x0106,   //灯状态
	ZIGBEE_LIGHT_SET_LEVEL			= 0x0107,   //灯亮度
	ZIGBEE_LIGHT_SET_TEMPER			= 0x0108,   //灯色温

	//窗帘命令
	ZIGBEE_CURTAIN_SET_LEVEL		= 0x010C,   //窗帘所在位置
	ZIGBEE_CURTAIN_SET_OPERATE		= 0x010D,   //窗帘操作

	//公共命令
	ZIGBEE_DEVICE_STATUS			= 0x0103,	//检测设备状态
	ZIGBEE_CLEAR_DEVICE				= 0x010E, 	//注销
	ZIGBEE_DEVICE_INFO				= 0x0116,	//设备信息
	ZIGBEE_MANAGE_GROUID			= 0x0120,	//管理分组
	ZIGBEE_MANAGE_SCENE				= 0x0121,	//管理场景
	ZIGBEE_GET_BAT_INFO				= 0x0123,	//获取电池信息
	ZIGBEE_DEVICE_BIND				= 0x0125,	//设备绑定
	ZIGBEE_CALL_DEVICE				= 0x01F8,	//设备点名
	ZIGBEE_SYNC_CLOCK				= 0x0133	//同步时钟
	
}ZIGBEE_CLUSTER_TYPE_E;


//入网设备SDK命令类型
typedef enum
{
	DEV_GATEWAY				= 0x0000,
	//插座设备
	DEV_ONEWAY_PLUG 		= 0x1001,
	DEV_TWOWAY_PLUG 		= 0x1002,
	DEV_THREEWAY_PLUG 		= 0x1003,
	DEV_FOURWAY_PLUG 		= 0x1004,

	//灯开关
	DEV_ONEWAY_LIGHT 		= 0x1101,
	DEV_TWOWAY_LIGHT 		= 0x1102,
	DEV_THREEWAY_LIGHT 		= 0x1103,
	DEV_FOURWAY_LIGHT 		= 0x1104,

	//电动窗帘
	DEV_ONEWAY_CURTAIN 		= 0x1304,
	DEV_TWOWAY_CURTAIN 		= 0x1341,
	DEV_THREEWAY_CURTAIN 	= 0x1342,
	DEV_FOURWAY_CURTAIN 	= 0x1343,
	DEV_FIVEWAY_CURTAIN 	= 0x1344,
	DEV_SIXWAY_CURTAIN 		= 0x1345,

	//场景面板
	DEV_SCENE_PANEL 		= 0x1312
}DEVICE_JION_NET_TYPE_E;

//设备SDK上报类型
typedef enum
{
	//状态
	NET_INFO					= 0x0200,  //网关网络
	BAT_INFO					= 0x0201,  //电池
	PLUG_INFO					= 0x0202, //插座
	LIGHT_INFO					= 0x0207,  //灯具
	CURTAIN_INFO				= 0x0209,  //窗帘
	VER_INFO					= 0x020A,  //版本
	DEV_INFO					= 0x020E,  //设备
	AIR_INFO                    = 0x0218,  //空调
	OTA_INFO					= 0x0214,  //OTA
	GOUID_INFO					= 0x0219,  //分组
	SCENE_INFO					= 0x021A,  //场景
	BIND_INFO					= 0x021E,  //绑定
	//事件
	SCENE_EVENT					= 0x0305,  //场景触发
	SCENE_KEY					= 0x03A1,  //场景按键
	SCENE_SWITCH                = 0x0218,  //场景开关
	ONLINE_EVENT				= 0x0308,  //上线
	LOW_POWER_EVENT				= 0x0309,  //低电量
	AUTH_EVENT					= 0x0370, //认证
	OTHER_EVENT					= 0x03A7   //其他
}DEVICE_REPORT_TYPE_E;

//设备SDK上报类型
typedef enum
{
	ONLINE_CHANG,
	ONLINE_LONG
}DEVICE_ONLINE_TYPE_E;

//添加文件保存类型
typedef enum
{
	FILE_NO_SAVE,
	FILE_SAVE
}FILE_ONLINE_TYPE_E;


int init_rex_sdk(char *name);
int device_data_report(char *sn, uint32_t ep_id, uint16_t data_type, char *data);
int controlZigbeeDevice(char *sn, uint32_t ep_id, USER_CONTROL_OPTION_E type, DItem pDes);

int device_join_net_report(char *sn, char *type);
int device_online_report(char *sn, DEVICE_ONLINE_TYPE_E type);
int device_offline_report(char *sn);
int set_network_sta(void);
int zigbee_network_query(void);
void ctl_dev_clear_net(char *sn);
void ctl_all_dev_clear_net(void);
int ctl_dev_adapt(uint16_t dev_type, char *sn, char *cmd);

#endif

