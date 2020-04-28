/*******************************************************************************************************
**                                           File description
** File Name:           call_back.h
** Description:         定义回调函数
** Creator:             xieshaobing
** Creation Date:       2017年4月20日
** Modify Log:          none
** Last Modified Date:  2017年4月20日
*******************************************************************************************************/

#ifndef	__REX_CALL_BACK_H
#define	__REX_CALL_BACK_H

/* If this is a C++ compiler, use C linkage */
#if defined(__cplusplus) 
extern "C"
{
#endif

/**
 * call_back_device_join_cb
 *     设备入网回调函数
 * address[in]: 设备地址
 * reporting[in]: 上报周期
 * type[in]: 设备类型
 * model_id[in]: Model Id
 * return: 0成功,否则-1
 * note: none
 */
extern int call_back_device_join_cb(char *address, unsigned short reporting, char *type, char *model_id);

/**
 * call_back_device_leave_cb
 *     设备离网回调函数
 * address[in]: 设备地址
 * return: 0成功,否则-1
 * note: none
 */
extern int call_back_device_leave_cb(char *address);

/**
 * call_back_report_state_data_cb
 *     上报状态数据回调函数
 * address[in]: 设置地址
 * endpoint_id[in]: 设备端ID号,默认为1,多路设备从1依次递增
 * state_type[in]: 状态种类
 * state[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
extern int call_back_report_state_data_cb(char *address, unsigned char endpoint_id, unsigned short state_type, char *
state);

/**
 * call_back_report_event_data_cb
 *     上报事件数据回调函数
 * address[in]: 设置地址
 * endpoint_id[in]: 设备端ID号,默认为1,多路设备从1依次递增
 * event_type[in]: 事件种类
 * event[in]: 事件数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
extern int call_back_report_event_data_cb(char *address, unsigned char endpoint_id, unsigned short event_type, char *
event);

/**
 * call_back_report_self_defining_data_cb
 *     上报自定义数据回调函数
 * address[in]: 设置地址
 * endpoint_id[in]: 设备端ID号,默认为1,多路设备从1依次递增
 * data[in]: 自定义数据
 * length[in]: 自定义数据长度
 * return: 0成功,否则-1
 * note: none
 */
extern int call_back_report_self_defining_data_cb(char *address, unsigned char endpoint_id, char *data, unsigned 
short length);

/* If this is a C++ compiler, use C linkage */
#if defined(__cplusplus) 
}
#endif

#endif
/*******************************************************************************************************
**                                           End of file
*******************************************************************************************************/

