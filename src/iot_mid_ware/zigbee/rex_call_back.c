/*******************************************************************************************************
**                                           File description
** File Name:           call_back.c
** Description:         实现回调函数
** Creator:             xieshaobing
** Creation Date:       2017年4月20日
** Modify Log:          none
** Last Modified Date:  2017年4月20日
*******************************************************************************************************/

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "platform.h"
#include "rex_call_back.h"
#include "rex_common.h"
#include "string_processor.h"
#include "dev_mng.h"
#include "error_code.h"

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
int call_back_device_join_cb(char *address, unsigned short reporting, char *type, char *model_id)
{
	LogD_Prefix("Started reporting join data to the cloud server:\r\n    address = %s\r\n    reporting = %d\r\n    type = %s\r\n    model_id = %s\r\n",
		address, reporting, type, model_id);

	device_join_net_report(address, type);
		

	return GW_OK;
}

/**
 * call_back_device_leave_cb
 *     设备离网回调函数
 * address[in]: 设备地址
 * return: 0成功,否则-1
 * note: none
 */
int call_back_device_leave_cb(char *address)
{
	LogD_Prefix("Started reporting leave data to the cloud server:\r\n    address = %s\r\n", address);

	device_offline_report(address);
	
	return GW_OK;
}

/**
 * call_back_report_state_data_cb
 *     上报状态数据回调函数
 * address[in]: 设备地址
 * endpoint_id[in]: 设备端ID号,默认为1,多路设备从1依次递增
 * state_type[in]: 状态种类
 * state[in]: 状态数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
int call_back_report_state_data_cb(char *address, unsigned char endpoint_id, unsigned short state_type, char *state)
{
	LogD_Prefix("Started reporting state data to the cloud server:\r\n    address = %s\r\n    endpoint_id = %d\r\n    state_type = 0x%04X\r\n    state = %s\r\n",
		address, endpoint_id, state_type, state);

	device_data_report(address, endpoint_id, state_type, state);

	return GW_OK;
}

/**
 * call_back_report_event_data_cb
 *     上报事件数据回调函数
 * address[in]: 设备地址
 * endpoint_id[in]: 设备端ID号,默认为1,多路设备从1依次递增
 * event_type[in]: 事件种类
 * event[in]: 事件数据,为JSON字符串
 * return: 0成功,否则-1
 * note: none
 */
int call_back_report_event_data_cb(char *address, unsigned char endpoint_id, unsigned short event_type, char *event)
{
	LogD_Prefix("Started reporting event data to the cloud server:\r\n    address = %s\r\n    endpoint_id = %d\r\n    event_type = 0x%04X\r\n    event = %s\r\n",
		address, endpoint_id, event_type, event);

	device_data_report(address, endpoint_id, event_type, event);

	return GW_OK;
}

/**
 * call_back_report_self_defining_data_cb
 *     上报自定义数据回调函数
 * address[in]: 设备地址
 * endpoint_id[in]: 设备端ID号,默认为1,多路设备从1依次递增
 * data[in]: 自定义数据
 * length[in]: 自定义数据长度
 * return: 0成功,否则-1
 * note: none
 */
int call_back_report_self_defining_data_cb(char *address, unsigned char endpoint_id, char *data, unsigned short length)
{
	return GW_OK;
}

/*******************************************************************************************************
**                                           End of file
*******************************************************************************************************/

