#include "error_code.h"
#include "defines.h"
#include "dev_mng.h"
#include "dev_cfg.h"
#include "rex_common.h"
#include "stdlib.h"

char cfg_file[256];
#define MAX_DEV_MEM 1300
/**
* Set device configuration file
* @param file, the path of config file
* @return 
*/
void set_cfg_file(char *file)
{
	LogI_Prefix("------------Set configure file path is %s------------\n",file);
    memset(cfg_file, 0, sizeof(cfg_file));
    strncpy(cfg_file, file,sizeof(cfg_file));
}

int creat_dev_cfg(void)
{  
    int fd = -1;   
	LogD_Prefix("Creat device file configuration.........\n");  
	fd = open(cfg_file,O_CREAT,S_IRWXU);
    if(0 > fd)
    {  
        LogE_Prefix("Fail to open or create file! %s\n", cfg_file);
        return GW_FILE_OPEN_FAIL;
    }  
    close(fd);  
	LogD_Prefix("Creat device file configuration ok.........\n");  
    return GW_OK;  
}  

/**
* Save device configuration into file
* @param buf, device information
* @param size
* @return 
*/
int save_dev_cfg(uint8_t *buf, uint32_t size)
{  
    int fd = -1;

	RETURN_IF_NULL(buf, GW_NULL_PARAM);
    
	LogD_Prefix("Save device configuration.........\n");  
	
	//fd = open(cfg_file,O_CREAT|O_RDWR);
	fd = open(cfg_file,O_CREAT|O_RDWR,S_IRWXU); 
    if(0 > fd)
    {  
        LogE_Prefix("Fail to open or create file! %s\n", cfg_file);
        return GW_FILE_OPEN_FAIL;
    }  
	lseek(fd, 0, SEEK_SET);
	if(0 > write(fd, buf, size))
    {  
        LogE_Prefix("Save device configuration error!\n");
		close(fd);
		return GW_FILE_WRITE_FAIL;
    }
	
    close(fd);  
	LogD_Prefix("Save device configuration ok.........\n");  

    return GW_OK;  
}  

/**
* Read device configuration from file
* @param buf, store device information
* @param size
* @return 
*/
int read_dev_cfg(uint8_t *buf, uint32_t size)
{
	int fd = -1;
	
	RETURN_IF_NULL(buf, GW_NULL_PARAM);
	
	LogI_Prefix("Read device configuration.........\n");  
	
    fd = open(cfg_file, O_RDWR);
    if(0 > fd)
    {  
        LogE_Prefix("Fail to open or create file! %s\n", cfg_file);
        return GW_FILE_OPEN_FAIL;
    }  
	lseek(fd, 0, SEEK_SET);
    if(0 > read(fd, buf, size))
    {
        LogE_Prefix("Read device configuration error!\n");
		close(fd);
		return GW_FILE_READ_FAIL;
    }
	
	LogD_Prefix("the data0 is %x\n",buf[0]);  
    LogD_Prefix("the data1 is %x\n",buf[1]); 
    LogD_Prefix("the data2 is %x\n",buf[2]); 
 	LogD_Prefix("the data3 is %x\n",buf[3]);
	LogD_Prefix("the data4 is %x\n",buf[4]);
	LogD_Prefix("the data5 is %x\n",buf[5]);
	LogD_Prefix("the data6 is %x\n",buf[6]);
	LogD_Prefix("the data7 is %x\n",buf[7]);
	LogD_Prefix("the data8 is %x\n",buf[8]);
	LogD_Prefix("the data9 is %x\n",buf[9]);
	LogD_Prefix("the dataMAX_DEV_MEM-1 is %x\n",buf[MAX_DEV_MEM-1]);
	
    close(fd);
	LogI_Prefix("Read device configuration ok.........\n");
	
    return GW_OK;  
}

/** 
* Erase device configuration from file
* @param
* @return 
*/
int erase_dev_cfg(void)
{
	int fd = -1;
	
	LogI_Prefix("Erase device configuration.........\n");  
	
    fd = open(cfg_file, O_RDWR);
    if(0 > fd)
    {
        LogE_Prefix("Fail to open or create file! %s\n", cfg_file);
        return GW_FILE_OPEN_FAIL;
    }

    ftruncate(fd,0);
    lseek(fd,0,SEEK_SET);
	close(fd);
	
	LogI_Prefix("Erase device configuration ok.........\n");  
	
	return GW_OK; 
}
/** 
* Check file message
* @param
* @return 
*/
int is_file_null(void)
{  
    int ret = -1;  
    FILE *fp;  
    char ch;  
      
    ret = (cfg_file != NULL);  
    LogI_Prefix(" Open  file! \n");  
    if(ret)  
    {  
          
        if((fp=fopen(cfg_file,"r+"))==NULL)  
        {  
            ret = -1;  
            LogE_Prefix("Fail to open or create file! \n");
            return ret;  
        }  
          
        ch=fgetc(fp);  
          
        if(ch == 0xff) //EOF = -1, Ò²¾ÍÊÇ0xff;
            ret = 0; 
  
        else  
            ret = 1; 
    }  
      
    fclose(fp);  
  
    return ret;   
}

/**
* Save new device message into file
* @param dev, device information
* @return 
*/
int save_dev_info(DEV_INFO_S *dev)
{
	uint8_t p[MAX_DEV_MEM];
	uint16_t number;
	int ret;
	RETURN_IF_NULL(dev, GW_NULL_PARAM);
	if(CONN_TYPE_ZIGBEE != dev->conn_type)
	{
		return GW_OK;
	}
	dev->offline_flag = 0;
	ret= is_file_null();
	memset(p, 0, MAX_DEV_MEM);
	if(ret == 0 || ret == 1)
	{
		if(ret == 0)
		{
			LogE_Prefix("The file is NULL.......... \r\n");
			//erase_dev_cfg();
			p[MAX_DEV_MEM-1]=0;
			number=0;
			
		}
		if(ret == 1)
		{
			read_dev_cfg(p,MAX_DEV_MEM);
			LogD_Prefix("The dev number is %02x............. \r\n",p[MAX_DEV_MEM-1]);
			number=p[MAX_DEV_MEM-1]*10;
		}
		
		//dev->child_info.zigbee.long_addr = mac_data_to_i(dev->sn);
		LogD_Prefix("New device dev->sn is %s \r\n", dev->sn);
		dev->child_info.zigbee.long_addr = strtoul(dev->sn, NULL, 16);
		dev->child_info.zigbee.node_number=p[MAX_DEV_MEM-1];
		p[MAX_DEV_MEM-1]+=1;
		LogD_Prefix("New device node_number is 0x%02X ............. \r\n",dev->child_info.zigbee.node_number);
		LogD_Prefix("New device mac_address is 0x%016lX ............. \r\n",dev->child_info.zigbee.long_addr);
		LogD_Prefix("New device type is 0x%02X ............. \r\n",dev->child_info.zigbee.type);
		p[number+0] =  (dev->child_info.zigbee.long_addr)&0xFF;
		p[number+1] = ((dev->child_info.zigbee.long_addr)>>8)&0xFF;
		p[number+2] = ((dev->child_info.zigbee.long_addr)>>16)&0xFF;
		p[number+3] = ((dev->child_info.zigbee.long_addr)>>24)&0xFF;
		p[number+4] = ((dev->child_info.zigbee.long_addr)>>32)&0xFF;
		p[number+5] = ((dev->child_info.zigbee.long_addr)>>40)&0xFF;
		p[number+6] = ((dev->child_info.zigbee.long_addr)>>48)&0xFF;
		p[number+7] = ((dev->child_info.zigbee.long_addr)>>56)&0xFF;
		//p[number+8] = dev->child_info.zigbee.type;
		p[number+8] = (dev->child_info.zigbee.type)&0xFF;
		p[number+9] = (dev->child_info.zigbee.type>>8)&0xFF;
		if( GW_OK == erase_dev_cfg())
		{
			save_dev_cfg(p, MAX_DEV_MEM);
		}
	
	}
	else
	{
		return GW_ERR;
	}	
	return GW_OK;
}

/**
* Save  device message into file after delete device
* @param sn, device serial number
* @return 
*/
int del_dev_info(char* sn, DEV_NODE_S *node, LIST_HEAD_T *dev_list_head)
{
	DEV_INFO_S *debug = NULL;
	uint8_t p[MAX_DEV_MEM];
	uint8_t number;
	int k,i;
	int ret;
	RETURN_IF_NULL(sn, GW_NULL_PARAM);
	RETURN_IF_NULL(node, GW_NULL_PARAM);
	RETURN_IF_NULL(dev_list_head, GW_NULL_PARAM);
	if(CONN_TYPE_ZIGBEE != node->dev_info.conn_type)
	{
		return GW_OK;
	}
	debug = &node->dev_info;
	if(NULL == debug)
	{
		LogE_Prefix("Delelte dev is not existed \r\n");
		return GW_ERR;
	}
	memset(p, 0, MAX_DEV_MEM);//181218
	ret= is_file_null();
	if(ret == 0)
	{
		LogE_Prefix("The file is NULL.......... \r\n");
		return GW_ERR;
	}
	if(ret == 1)
	{
		read_dev_cfg(p,MAX_DEV_MEM);
		number = debug->child_info.zigbee.node_number;
		LogD_Prefix("Delelte device node_number is 0x%02X ............. \r\n",debug->child_info.zigbee.node_number);
		LogD_Prefix("Delelte device type is 0x%02X ............. \r\n",debug->child_info.zigbee.type);
		k = number*10;
		LogD_Prefix("Delelte device type is 0x%02X ............. \r\n",p[k+8]);
		
		if( p[k+8] == ((debug->child_info.zigbee.type)&0xFF))
		{
			for(i=k;i<(MAX_DEV_MEM-1-10);i++)
			{	
				p[i] = p[i+10];
			}
			for(i=MAX_DEV_MEM-1-10;i<MAX_DEV_MEM-1;i++)
			{
				p[i]=0x00;
			}
			p[MAX_DEV_MEM-1] = p[MAX_DEV_MEM-1]-1;
			if(GW_OK == erase_dev_cfg())
			{
				if(GW_OK == save_dev_cfg(p, MAX_DEV_MEM))
				{	
					list_for_each_entry(node, dev_list_head, dev_head)
					{
						if ((node->dev_info.child_info.zigbee.node_number) > number)
						{
							 	node->dev_info.child_info.zigbee.node_number = (node->dev_info.child_info.zigbee.node_number)-1;
						}
					}
				}
			}
			
		}
		else
		{
			LogE_Prefix("Delelte device is not be found in file \r\n");
			return GW_ERR;
		}
	}
	return GW_OK;
}

/**
* Adapt product id
* @param dev
* @return 
*/
int product_id_adapt(DEV_INFO_S* dev)
{
	char *productID=NULL;
	RETURN_IF_NULL(dev, GW_NULL_PARAM);
	LogD_Prefix("dev->child_info.zigbee.type is 0x%04x\r\n",dev->child_info.zigbee.type);
	switch(dev->child_info.zigbee.type)
	{
		case 0:
			{
				productID = DEV_PRODUCT_GATEWAY;
				break;
			}
		case DEV_ONEWAY_PLUG:
			{
				productID = DEV_PRODUCT_ONEWAY_PLUG;
				break;
			}
		case DEV_TWOWAY_PLUG:
		case DEV_THREEWAY_PLUG:
		case DEV_FOURWAY_PLUG:
			{
				break;
			}
		case DEV_ONEWAY_LIGHT:
			{
				productID = DEV_PRODUCT_ONEWAY_LIGHT;
				break;
			}
		case DEV_TWOWAY_LIGHT:
			{
				productID = DEV_PRODUCT_TWOWAY_LIGHT;
				break;
			}
		case DEV_THREEWAY_LIGHT:
			{
				productID = DEV_PRODUCT_THREEWAY_LIGHT;
				break;
			}
		case DEV_FOURWAY_LIGHT:
			{
				break;
			}
		case DEV_ONEWAY_CURTAIN:
			{
				productID = DEV_PRODUCT_ONEWAY_CURTAIN;
				break;
			}
		case DEV_TWOWAY_CURTAIN:
		case DEV_THREEWAY_CURTAIN:
		case DEV_FOURWAY_CURTAIN:
		case DEV_FIVEWAY_CURTAIN:
		case DEV_SIXWAY_CURTAIN:
			{
				break;
			}
		case DEV_SCENE_PANEL:
			{
				productID = DEV_PRODUCT_SCENE;
				break;
			}
		default:
			LogI_Prefix("The productid is not be found!\r\n");
			break;
	}
	RETURN_IF_NULL(productID, GW_NULL_PARAM);
	AGENT_STRNCPY(dev->product_id, productID, strlen(productID));
	//agent_strncpy(dev->mac, "AA:BB:CC:DD:EE:FF", strlen("AA:BB:CC:DD:EE:FF"));
	AGENT_STRNCPY(dev->soft_ver, "1.1.01", strlen("1.1.01"));
	AGENT_STRNCPY(dev->gw_sn, get_gw_info()->sn, strlen(get_gw_info()->sn));
	AGENT_STRNCPY(dev->ip,get_gw_info()->ip , strlen(get_gw_info()->ip));
	LogD_Prefix("The sn is %s gw_sn is %s\r\n",dev->sn,dev->gw_sn);
	return GW_OK;
}

/**
* Read  device message from file and creat device list
* @param 
* @return 
*/
int read_dev_info(void)//
{
	uint8_t pp[MAX_DEV_MEM];
	uint8_t data[10];
	uint8_t number=0;
	int i;
	int j;
	int k;
	int n;
	int ret;
	int sum=0;
	LogI_Prefix("Reboot first read file device data start.......... \r\n");
    ret = is_file_null();
	memset(pp, 0, MAX_DEV_MEM);
	memset(data, 0, 10);
	if (ret == 0)
  	{
		LogI_Prefix("The file is NULL.......... \r\n");
		erase_dev_cfg();
  	}	
    else
    {
		if (ret == 1)
	 	{
			read_dev_cfg(pp,MAX_DEV_MEM);
			number=pp[MAX_DEV_MEM-1];
			LogI_Prefix("Read file reboot number  is %02X ............. \r\n",pp[MAX_DEV_MEM-1]);
			for(n=0;n<8;n++)
			{
				sum=sum+pp[n];
			}
			if(sum == 0)//fileå†…æ²¡æœ‰æ•°æ®
			{
				if(number!=0)
				{
					erase_dev_cfg();
				}	
				LogD_Prefix("File have no data.......... \r\n");
				return GW_OK;	
			}
			else
			{
				for(j=0;j<number;j++)
				{
				
			    	k=j*10;
					for(i=k;i<k+10;i++)
					{
					
					data[i-k] = pp[i];
					printf("data%d is %02X ............. \r\n",i,data[i-k]);
				
					}
					struct receive_data
					{
			    		uint64_t 	long_addr;
			    		uint16_t  	type;
					} __attribute__((__packed__)) *psMessage = (struct receive_data *)data;
			
					DEV_INFO_S* dev = NULL;
					dev = (DEV_INFO_S*)malloc(sizeof(DEV_INFO_S));	
					RETURN_IF_NULL(dev, GW_NULL_PARAM);
			   		memset(dev, 0, sizeof(DEV_INFO_S));
					dev->offline_flag = 0;
					dev->maxtime  = VC_MAX_TIME;
					dev->overtime = dev->maxtime;
					dev->child_info.zigbee.long_addr = psMessage->long_addr;
					dev->child_info.zigbee.type = psMessage->type;
					snprintf(dev->sn, SN_LEN, "%016lX", dev->child_info.zigbee.long_addr);
					uint8_t mac[6]={0};
					mac[0] = ((dev->child_info.zigbee.long_addr)>>16)&0xFF;
					mac[1] = ((dev->child_info.zigbee.long_addr)>>24)&0xFF;
					mac[2] = ((dev->child_info.zigbee.long_addr)>>32)&0xFF;
					mac[3] = ((dev->child_info.zigbee.long_addr)>>40)&0xFF;
					mac[4] = ((dev->child_info.zigbee.long_addr)>>48)&0xFF;
					mac[5] = ((dev->child_info.zigbee.long_addr)>>56)&0xFF;
					snprintf(dev->mac, sizeof(dev->mac), "%02x:%02x:%02x:%02x:%02x:%02x", \
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
					LogD_Prefix("Creat list sn= is 0x%016lX ............. \r\n",dev->child_info.zigbee.long_addr);
					LogD_Prefix("Creat list type= is 0x%02X ............. \r\n",dev->child_info.zigbee.type);
					dev->child_info.zigbee.node_number = j ;//FROM 0
					LogD_Prefix("Creat list node_number= is 0x%02X ............. \r\n",dev->child_info.zigbee.node_number);
					if(product_id_adapt(dev) == GW_OK)
					{
						add_dev(dev,0);
					}
					FREE_POINTER(dev);
					sleep_ms(20);
			    }	
		    }
		} 
		else
		{
			LogI_Prefix("Creat devie file......... \r\n");
			creat_dev_cfg();
			erase_dev_cfg();
		}
	}	
  	LogI_Prefix("Read file data ok,next start dongle sdk.......... \r\n");
  	return GW_OK;
}

