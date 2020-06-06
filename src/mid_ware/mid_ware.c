#include "defines.h"
#include "error_code.h"
#include "dev_mng.h"
#include "msg_mng.h"
#include "gw_service.h"

int init_mid_ware(void)
{
	init_dev_list(128, 64);

	dev_offline_check();

	init_msg_queue(256, process_http_msg);

	return GW_OK;
}

int deinit_mid_ware(void)
{
	return GW_OK;
}


