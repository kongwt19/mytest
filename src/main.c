#include "mid_ware.h"
#include "iot_mid_ware.h"
#include "gw_service.h"
#include "dev_mng.h"
#include "dev_cfg.h"
#include "rex_uart.h"

const char *test_mqtt_ip			= "202.98.157.137";
const short test_mqtt_port 		= 1883;
const char *test_cdc_ip			= "202.98.157.138";
const short test_cdc_port 		= 80;
const char *test_ippinte_ip		= "202.98.157.138";
const short test_ippinte_port	= 80;

void online_callback(char *sn, char *product_id)
{
	return;
}

void offline_callback(char *sn, char *product_id)
{
	return;
}

void alarm_callback(char *sn, char *alarm)
{
	return;
}

void zigbee_callback(int flag)
{
	return;
}




int main(int argc, char **argv)
{
	DEV_INFO_S gw;

	memset(&gw, 0, sizeof(gw));
	strncpy(gw.sn, "TV_GW_5013956DE2E2", sizeof(gw.sn));
	strncpy(gw.product_id, "SLIFE_GW0001_TVGW001", sizeof(gw.product_id));
	strncpy(gw.soft_ver, "1.1.1", sizeof(gw.soft_ver));
	strncpy(gw.ip, "192.168.1.10", sizeof(gw.ip));
	strncpy(gw.mac, "AA:BB:CC:DD:EE:FF", sizeof(gw.mac));
	gw.is_master = TRUE;

	set_cfg_file("/tmp/zigbee");
	set_gw_info(&gw);
	init_mid_ware();

	reg_online_callback(online_callback);
	reg_offline_callback(offline_callback);
	reg_alarm_callback(alarm_callback);
	reg_zigbee_callback(zigbee_callback);

	set_mqtt_server(test_mqtt_ip, test_mqtt_port);
	set_cdc_server(test_cdc_ip, test_cdc_port);
	set_ippinte_server(test_ippinte_ip, test_ippinte_port);

	init_iot_mid_ware();

	while(TRUE)
	{
		sleep_s(60);
	}
	
	return 0;
}
