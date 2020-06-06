#ifndef __SCENE_PANEL_H__
#define __SCENE_PANEL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"
#include <pthread.h>
#include "defines.h"
#include "cJSON.h"

#define MAX_POINT 12
typedef struct __panelInfo
{
	char sn[SN_LEN];        //网关sn
	char devid[SN_LEN];     //场景面板id
	char addDevid[SN_LEN];  //场景中设备id
	int point;              //场景面板point
	char *cmd;              //场景控制命令
}PANEL_INFO_S;

typedef struct _panelNode
{
	LIST_HEAD_T panel_head;
	PANEL_INFO_S panel_info;
}PANEL_NODE_S;

typedef struct _panellistMng
{
	LIST_HEAD_T	panel_list_head;
	pthread_mutex_t panel_list_mutex;
}PANEL_LIST_MNG_S;

typedef struct method_transfer
{
	char * old_method;
	char * new_method;
}METHOD_TRANSFER;

int init_panel_list(void);

void init_scene_panel();

int update_scene_panel(char *deviceId, int point);

void execute_scene_panel(char *deviceId, int keyval);

int get_cmd_num(char *devid, int point);

int add_node(PANEL_INFO_S *p_info);

int cmd_transfer_point(char * deviceId, int point, cJSON * dev_json, PANEL_INFO_S ** dev_panel);

#ifdef __cplusplus
}
#endif

#endif