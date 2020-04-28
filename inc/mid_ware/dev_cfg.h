#ifndef __DEV_CFG_H__
#define __DEV_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
* Set device configuration file
* @param file, the path of config file
* @return 
*/
void set_cfg_file(char *file);
/**
* Save device configuration into file
* @param buf, device information
* @param size
* @return 
*/
int save_dev_cfg(uint8_t *buf, uint32_t size);
/**
* Read device configuration from file
* @param buf, store device information
* @param size
* @return 
*/
int read_dev_cfg(uint8_t *buf, uint32_t size);
/** 
* Erase device configuration from file
* @param
* @return 
*/
int erase_dev_cfg(void);
/** 
* Check file message
* @param
* @return 
*/
int is_file_null(void);
/**
* Save new device message into file
* @param dev, device information
* @return 
*/
int save_dev_info(DEV_INFO_S *dev);
/**
* Save  device message into file after delete device
* @param sn, device serial number
* @return 
*/
int del_dev_info(char* sn, DEV_NODE_S *node, LIST_HEAD_T *dev_list_head);
/**
* Read  device message from file and creat device list
* @param 
* @return 
*/
int read_dev_info(void);
int creat_dev_cfg(void);
#ifdef __cplusplus
}
#endif

#endif/*__DEV_CFG_H__*/
