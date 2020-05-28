#ifndef __LIGHT_LOCAL_H__
#define __LIGHT_LOCAL_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*LOCAL_MSG_PROC_FUNC)(char *req, int req_len, char **resp, int *resp_len);

int local_ctrl_dev(char *sn, char *msg, int len);

void start_light_local(LOCAL_MSG_PROC_FUNC cb);

#ifdef __cplusplus
}
#endif

#endif