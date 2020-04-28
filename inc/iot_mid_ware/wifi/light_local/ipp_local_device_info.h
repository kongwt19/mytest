/******************************************************************************

                  版权所有 (C), 1958-2018, 长虹软服中心-IPP智慧家庭业务线

 ******************************************************************************
  版 本 号   : 初稿
  作    者   : 陈梁 20167633
  生成日期   : 2017年7月5日
  最近修改   :
  功能描述   : 本地设备列表管理
  修改历史   :
  1.日    期   : 2017年7月5日
    作    者   : 陈梁 20167633
    修改内容   : 创建文件
 ******************************************************************************/
#ifndef _IPP_LOCAL_DEVICE_INFO_
#define _IPP_LOCAL_DEVICE_INFO_

#include <stdint.h>
#include "ipp_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

//设备属性结构体
typedef struct _ipp_local_device
{
	char *guid;				//设备唯一标识
	char *product_id;		//设备产品标识
	char *software_ver;		//设备软件版本号
	int32_t device_type;	//设备类型
	char *mac;				//设备MAC地址
	int8_t connet_type;		//获取连接类型:蓝牙，zigbee
	char *name;				//设备名称(昵称)
	int8_t is_master;		//是否是主设备
	char *gateway_guid;		//所属网关标识，本身是网关则为空或等于自身guid
	int8_t *reserved;		//预留字段，可自定义
	int32_t reserved_len;	//预留字段长度
}ipp_local_device;

//设备列表结构体
typedef struct ipp_device_list
{
	ipp_local_device *device;
	struct ipp_device_list *next;
}ipp_device_list;

//SDK启动时，设备角色
typedef enum
{
	ipp_device_client  = 0x1,	//仅作为客户端
	ipp_device_server  = 0x2,	//仅作为服务端
	ipp_device_both = ipp_device_client | ipp_device_server,//同时作为客户端和服务端
	ipp_device_invalid,
}ipp_device_type;

/*
  函数名：	create_local_device
  功能：	创建一个本地设备信息结构体（内部已经初始化）
  入参：	无
  出参：	无
  返回值：	ipp_device_profile *  设备信息结构体，需要使用者使用free_local_device释放
*/
ipp_local_device *create_local_device();

/*
  函数名：	init_local_device
  功能：	将一个本地设备信息初始化，各个字段设置为默认值
  入参：	ipp_local_device *profile  本地设备信息结构体
  出参：	无
  返回值：	无
*/
void init_local_device(ipp_local_device *profile);

/*
  函数名：	init_local_device_from_protocol
  功能：	将一个本地设备信息初始化从一个消息缓存中解析出来
  入参：	ipp_local_device *profile  本地设备信息结构体，存放解析出来的数据
			ipp_protocol *protocol     消息缓存
  出参：	无
  返回值：	1：成功；-1：失败
*/
int init_local_device_from_protocol(ipp_local_device *profile, ipp_protocol *protocol);

/*
  函数名：	free_device_info
  功能：	释放设备信息结构体
  入参：	ipp_device_profile *profile
  出参：	无
  返回值：	无
*/
void free_local_device(ipp_local_device **profile);

/*
  函数名：	free_local_device_list
  功能：	释放设备列表
  入参：	ipp_device_list *profile_list
  出参：	无
  返回值：	无
*/
void free_local_device_list(ipp_device_list **profile_list);

/*
  函数名：	get_device_clone_from_other
  功能：	获得一个设备信息拷贝，返回值需要使用者释放
  入参：	ipp_local_device *other
  出参：	无
  返回值：	ipp_local_device	设备信息拷贝
*/
ipp_local_device *get_device_clone_from_other(ipp_local_device *other);

/*
  函数名：	ipp_device_list_append
  功能：	将一个设备添加到设备列表中
  入参：	ipp_device_list **head		设备列表头指针
			ipp_local_device *element	要添加的设备信息
  出参：	无
  返回值：	int							添加成功返回1，否则返回<=0
*/
int ipp_device_list_append(ipp_device_list **head, ipp_local_device *element);

/*
  函数名：	ipp_device_list_remove
  功能：	将一个设备添加从设备列表中移除
  入参：	ipp_device_list **head		设备列表头指针
			char *guidt					要移除的设备信息的sn
  出参：	无
  返回值：	ipp_device_list*			下一个设备的指针
*/
ipp_device_list *ipp_device_list_remove(ipp_device_list **head, char *guid);

/*
  函数名：	ipp_device_list_find
  功能：	根据SN从设备列表中查找设备
  入参：	ipp_device_list **head		设备列表头指针
			char *guid					要查找的设备信息的sn
  出参：	无
  返回值：	ipp_local_device *			找到的设备指针
*/
ipp_local_device *ipp_device_list_find(ipp_device_list **head, char *guid);

/*
  函数名：	get_device_info_protocol_len
  功能：	根据设备信息结构体获取发送此设备信息需要的缓存长度
  入参：	ipp_local_device *device	设备信息结构体
  出参：	无
  返回值：	int							需要的缓存长度
*/
int get_device_info_protocol_len(ipp_local_device *device);

/*
  函数名：	fill_to_protocol
  功能：	将设备信息填充到缓存中
  入参：	ipp_protocol *p				缓存
			ipp_local_device *device	设备信息
  出参：	无
  返回值：	int32_t						操作返回码
*/
int32_t fill_to_protocol(ipp_protocol *p, ipp_local_device *device);

#ifdef __cplusplus
}
#endif

#endif
