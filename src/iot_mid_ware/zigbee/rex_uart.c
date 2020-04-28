#include "rex_uart.h"
#include "rex_common.h"
#include "rex_export_gateway.h"
#include "platform.h"
#include "error_code.h"
#include "defines.h"
#include "iot_mid_ware.h"
#include "gw_service.h"

#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h> 
#include <fcntl.h>


#define UART_REV_LEN	512

static USB_PORT_CHECK usb_port_cb = NULL;


void reg_usb_port_callback(USB_PORT_CHECK cb)
{
	usb_port_cb = cb;
}


/**
 * uart_set
 * 串口设置函数
 * fd: 文件描述符
 * iBaudRate: 波特率
 * iDataSize: 字节大小
 * cParity: 校验位
 * iStopBit: 停止位
 * return: 0成功,否则-1
 * note: none
 */
static int uart_set(int fd, int iBaudRate, int iDataSize, char cParity, int iStopBit)
{
    int iResult = 0;
    struct termios oldtio, newtio;

    iResult = tcgetattr(fd, &oldtio);/*保存原先串口配置*/
    if(iResult)
    {
        LogE_Prefix("Can't get old terminal description !");
        return GW_ERR;
    }
      
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD;/*设置本地连接和接收使用*/  
      
    /*设置输入输出波特率*/  
    switch(iBaudRate)
    {
        case 2400:
            cfsetispeed(&newtio,B2400);
            cfsetospeed(&newtio,B2400);
            break;
        case 4800:
            cfsetispeed(&newtio,B4800);
            cfsetospeed(&newtio,B4800);
            break;
        case 9600:
            cfsetispeed(&newtio,B9600);
            cfsetospeed(&newtio,B9600);
            break;
        case 19200:
            cfsetispeed(&newtio,B19200);
            cfsetospeed(&newtio,B19200);
            break;
        case 38400:  
            cfsetispeed(&newtio,B38400);
            cfsetospeed(&newtio,B38400);
            break;
        case 57600:
            cfsetispeed(&newtio,B57600);
            cfsetospeed(&newtio,B57600);
            break;
        case 115200:
            cfsetispeed(&newtio,B115200);
            cfsetospeed(&newtio,B115200);
            break;
        case 460800:
            cfsetispeed(&newtio,B460800);
            cfsetospeed(&newtio,B460800);
            break;
        default:  
    		LogE_Prefix("Don't exist iBaudRate %d !\n", iBaudRate);  
    	return GW_ERR;
    }
    
    /*设置数据位*/  
    newtio.c_cflag &= (~CSIZE);
    switch( iDataSize )  
    {
        case 7:
            newtio.c_cflag |= CS7;
            break;  
        case 8:
            newtio.c_cflag |= CS8;
            break;
        default:
            LogE_Prefix("Don't exist iDataSize %d !\n", iDataSize);  
            return GW_ERR;
    }
      
    /*设置校验位*/  
    switch(cParity)
    {
        case 'N':                    /*无校验*/  
            newtio.c_cflag &= (~PARENB);  
            break;  
        case 'O':                    /*奇校验*/  
            newtio.c_cflag |= PARENB;  
            newtio.c_cflag |= PARODD;  
            newtio.c_iflag |= (INPCK | ISTRIP);  
            break;  
        case 'E':                    /*偶校验*/  
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= (~PARODD);
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;                
        default:  
            LogE_Prefix("Don't exist cParity %c !\n", cParity);
            return GW_ERR;                                  
    }
      
    /*设置停止位*/  
    switch(iStopBit)
    {
        case 1:
            newtio.c_cflag &= (~CSTOPB);  
            break;  
        case 2:
            newtio.c_cflag |= CSTOPB;  
            break;  
        default:
            LogE_Prefix("Don't exist iStopBit %d !\n", iStopBit);  
            return GW_ERR;                                  
    }
    newtio.c_cc[VTIME] = 0; /*设置等待时间*/  
    newtio.c_cc[VMIN] = 0;  /*设置最小字符*/  
    tcflush(fd, TCIFLUSH);       /*刷新输入队列(TCIOFLUSH为刷新输入输出队列)*/  
    iResult = tcsetattr(fd, TCSANOW, &newtio);    /*激活新的设置使之生效,参数TCSANOW表示更改立即发生*/  
  
    if(iResult)
    {  
        LogE_Prefix("Set new terminal description error !\n");
        close(fd);
        return GW_ERR;
    }
      
    return GW_OK;
}

/**
 * init_uart_sock
 * sock设置函数
 * return: 0成功,否则-1
 * note: none
 */
static int init_uart_sock(void)
{
	int sock = -1;
    int ret = -1;
	struct sockaddr_nl stNl;
	const int buffersize = 1024;
	
	memset(&stNl, 0x00, sizeof(struct sockaddr_nl));
	stNl.nl_family = AF_NETLINK;
	stNl.nl_pid = getpid();
	stNl.nl_groups = 1;
	
	sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (-1 == sock)
	{
		LogE_Prefix("Create socket error: %s\n", strerror(errno));
		return -1;
	}
	
	ret = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize));
	if(0 > ret)
	{
		LogE_Prefix("setsockopt error: %s\n", strerror(errno));
		return -1;
	}
	
	ret = bind(sock, (struct sockaddr *) &stNl, sizeof(struct sockaddr_nl));
	if (0 > ret)
	{
		LogE_Prefix("Bind error: %s\n", strerror(errno));
		close(sock);
		return -1;
	}
	
	return sock;
}

/**
 * uart_open
 * c串口打开函数
 * name[in]: 文件名
 * return: 0成功,否则-1
 * note: none
 */
static int uart_open(char *name)
{
	RETURN_IF_NULL(name, GW_NULL_PARAM);

	int iBaudRate, iDataSize, iStopBit;
	char cParity;
	int fd = -1;
    char path_name[20];

    if(NULL == strstr(name, "ttyUSB"))
    {
		LogE_Prefix("Open serial port3 %s failed\r\n", name);
        return -1;
    }

    memset(path_name, 0, 20);
    snprintf(path_name, sizeof(path_name), "/dev/%s", name);
    LogD_Prefix("uart_open:path_name is %s\n",path_name);
	fd = open(path_name, O_RDWR|O_NDELAY);
	if(fd == -1)
	{
		LogE_Prefix("uart_open error\n");
		return -1;
	}

	if(fcntl(fd, F_SETFL, 0) < 0)
    {
        LogE_Prefix("fcntl failed!\n");
        return -1;
    }

	iBaudRate = 115200;
	iDataSize = 8;
	cParity = 'N';
	iStopBit = 1;
	if(uart_set(fd, iBaudRate, iDataSize, cParity, iStopBit) != 0)
	{
		LogE_Prefix("uart set failed!\n");
		return -1;
	}
	
	return fd;
}

/**
 * check_uart_type
 * 检测串口函数
 * name[in]: 文件名
 * return: 0成功,否则-1
 * note: none
 */
int check_uart_type(char *name)
{
	RETURN_IF_NULL(name, GW_NULL_PARAM);

	int fd = -1;
    int ret = 0;
    uint8_t cmd[6] = {0x41, 0x54, 0x2B, 0x56, 0x45, 0x52};
    uint8_t buf[20];
	char *p = name;
    
    sleep_ms(1 * 100);
    
    fd = uart_open(p);
    if(0 > fd)
    {
		sleep_ms(1 * 100);
		fd = uart_open(p);
		if(0 > fd)
		{
			LogE_Prefix("Open serial port %s is failed\r\n", p);
	        return GW_ERR;
		}
	}
    LogD_Prefix("check_uart_type:Open serial port  %s is succeed， fd = %d\r\n", p, fd);

    ret = write(fd, cmd, sizeof(cmd));
    if(0 < ret)
    {
		sleep_ms(1 * 100);
		LogD_Prefix("write is ok\r\n");
		ret = read(fd, buf, sizeof(buf));
		LogD_Prefix("rev_ret %d\r\n",ret);
		if(0 < ret)
		{
            LogD_Prefix("rev_buf is %s\r\n", buf);
			if(0x52 == buf[2] && 0x45 == buf[3])
			{
				LogD_Prefix("%s is a dongle\r\n", name);
				close(fd);
				return GW_OK;
			}
		}
    }

    LogE_Prefix("%s is not a dongle\r\n", name);
    close(fd);
    return GW_ERR;
}

/**
 * read_uart_dev
 * 读串口设备文件函数
 * return: "USB"读取成功,否则NULL
 * note: none
 */
char *read_uart_dev(void)
{
	DIR *fd = NULL;
	const char *dir = "/dev/";
    char path[101] = {0};
	struct dirent *dp = NULL;
	char *ptr = NULL;

	memcpy(path, dir, strlen(dir));

	fd = opendir(path);
	if(NULL == fd)
    {
		LogE_Prefix("open dir failed! dir: %s\n", path);
        return NULL;
    }

    for(dp = readdir(fd); NULL != dp; dp = readdir(fd))
    {
    	if(NULL != strstr(dp->d_name, "ttyUSB"))
    	{
    		LogD_Prefix("Find com: %s\n", dp->d_name);
    		if(0 <= check_uart_type(dp->d_name))
            {
            	int dp_len = strlen(dp->d_name) + 10;
            	ptr = (char *)malloc(dp_len);
                if(NULL == ptr)
                {
                	FREE_POINTER(ptr);
                    return NULL;
                }
                memset(ptr, 0, sizeof(dp_len));
                snprintf(ptr, dp_len, "/dev/%s", dp->d_name);
                LogD_Prefix("check uart is ok: %s\n", ptr);
                break;
            }
    	}
    }
	
	return ptr;
}

/**
 * usb_dev_query
 * 查询usb是否存在函数
 * return: 0创建网络,否则-1
 * note: none
 */
int usb_dev_query(void)
{
	char *names = NULL;
	
	rex_end();
	names = read_uart_dev();
	if(names)
	{
		if(init_rex_sdk(names))
		{
			FREE_POINTER(names);
			LogD_Prefix("start rex sdk is success \r\n");
			return GW_OK;
		}
		else
		{
			LogD_Prefix("checking the usb is fail \r\n");
			return GW_ERR;
		}
	}
	else
	{
		LogD_Prefix("checking the usb is not exist \r\n");
	}
	
	return GW_NULL_PARAM;
}

/**
 * read_uart_dev
 * usb插入检测函数
 * return: "USB"插入成功,否则NULL
 * note: none
 */
int add_uart_check(char *buf)
{
	RETURN_IF_NULL(buf, GW_NULL_PARAM);
	
	char *start = NULL;
	char *end = NULL;
	char path_name[20];

    memset(path_name, 0, sizeof(path_name));

	start = strstr(buf, "ttyUSB");
	if(NULL != start)
	{
		LogD_Prefix("add_uart_check:add dev_str msg start: %s\n", start);
		end = strstr(start, "/tty/");
		if(NULL == end)
		{
			snprintf(path_name, 20, "/dev/%s", start);
			LogD_Prefix("add_uart_check:usb path_name: %s\n", path_name);
			if(GW_OK == check_uart_type(start))
			{
				if(NULL != usb_port_cb)
				{
					LogD_Prefix("=============usb add =============\n");
					usb_port_cb(USB_ADD);
				}
				return GW_OK;
			}
			else
			{
				LogE_Prefix("udev path name is not exist \n");
			}
		}
	}
	
	return GW_ERR;
}

/**
 * read_uart_dev
 * usb拔出检测函数
 * return: "USB"拔出成功,否则NULL
 * note: none
 */
int remove_uart_check(char *buf)
{
	RETURN_IF_NULL(buf, GW_NULL_PARAM);

	char *start = NULL;
	char *end = NULL;
	
	start = strstr(buf, "ttyUSB");
	if(NULL != start)
	{
		LogD_Prefix("remove_uart_check:remove dev_str msg start: %s\n", start);
		end = strstr(start, "/tty/");
		if(NULL == end)
		{
			LogD_Prefix("add dev_str msg end: %s\n", end);
			if(NULL != usb_port_cb)
			{
				LogD_Prefix("============= usb remove =============\n");
				usb_port_cb(USB_REMOVE);
			}
		}
	}
	
	return GW_OK;
}

/**
 * uart_dev_check_fnc
 * 监测串口线程函数
 * return: 0成功,否则-1
 * note: none
 */
void *uart_dev_check_fnc(void *arg)
{
	int com_fd  = -1;
	int rev_len = 0;
	char buf[UART_REV_LEN];

	char *uart_name = NULL;

	com_fd = init_uart_sock();
	if(0 > com_fd)
	{
		LogE_Prefix("udev fd is error!!!\n");
		return NULL;
	}
	
restart:	
	uart_name = read_uart_dev();
	if(NULL != uart_name)
	{
		LogD_Prefix("rex sdk is start uart path name is: %s\n", uart_name);
		if(GW_OK == init_rex_sdk(uart_name))
		{
			LogD_Prefix("========rex sdk start is ok======== \n");
			FREE_POINTER(uart_name);
		}
		else
		{
			LogE_Prefix("rex sdk start is fail\n");
			goto restart;
		}
	}
	else
	{
		LogE_Prefix("device name is not exist \n");
	}

	while(1)
	{
		memset(buf, 0, sizeof(buf));
		rev_len = recv(com_fd, buf, sizeof(buf), 0);
        if(0 < rev_len)
        {
        	LogD_Prefix("recv usb msg: %s\n", buf);
			if(strstr(buf, "add"))
			{
				if(GW_OK == add_uart_check(buf))
				{
					goto restart;
				}
			}
			else if(strstr(buf, "remove"))
			{
				remove_uart_check(buf);
			}
        }
	}

	close(com_fd);
	
	return NULL;
}

/**
 * init_rex_uart
 * 串口初始函数
 * return: 0成功,否则-1
 * note: none
 */
int init_rex(void)
{
	pthread_attr_t rex_attr;
	pthread_t rex_threadId;

	pthread_attr_init(&rex_attr);
	pthread_attr_setdetachstate(&rex_attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&rex_threadId, &rex_attr, uart_dev_check_fnc, NULL);
	
	return GW_OK;
}




