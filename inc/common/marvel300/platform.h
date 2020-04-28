#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wmstdio.h>
#include <wm_os.h>
#include <netdb.h>
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <lwip/netdb.h>
#include <FreeRTOS.h>
#include <mw300_uart.h>
#include <mdev_uart.h>
#include <mdev_pinmux.h>
#include <mdev_gpio.h>
#include <rfget.h>
#include <partition.h>
#include <firmware_structure.h>

#define LOCAL_SERVICE		        1
#define SLAVE_DEVICE_ENABLE			1
//#define MCU_UART_ENABLE				1

#define MAX_DEV_NUM  			17
#define IPP_MSG_QUEUE_MAX_LEN	10

#ifdef __cplusplus
extern "C" {
#endif

#define printf							wmprintf

#if LOG_OPEN_ALL

#define ipp_LogV_Prefix(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogD_Prefix(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogI_Prefix(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogW_Prefix(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogE_Prefix(__fmt__, ...) printf("[IPP ERR]:"__fmt__, ##__VA_ARGS__)

#define ipp_LogV(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogD(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogI(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogW(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogE(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)

#else

#define ipp_LogV_Prefix(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogD_Prefix(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogI_Prefix(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogW_Prefix(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogE_Prefix(__fmt__, ...) printf("[IPP ERR]:"__fmt__, ##__VA_ARGS__)

#define ipp_LogV(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogD(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogI(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogW(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogE(__fmt__, ...) printf("[IPP]:"__fmt__, ##__VA_ARGS__)

#endif//end #if LOG_OPEN_ALL

#define sleep_ms(x)                     os_thread_sleep(os_msec_to_ticks(x))
#define sleep_s(s)						sleep_ms(s * 1000)
#define sleep_us(us)					sleep_ms(us / 1000)
#define ipp_ntoa(in)                    inet_ntoa(in.s_addr)
const char *ipp_ntop(int af, struct in_addr *src, char *dst, int cnt);

/*memory define*/
#define malloc(size)                    os_mem_alloc(size)
#define alloc(old_ptr,new_size)         os_mem_realloc(old_ptr,new_size)
#define free(ptr)                       os_mem_free(ptr)
#define realloc(oldpt,newsize)          os_mem_realloc(oldpt,newsize)
#define calloc(nmemb, size)             os_mem_calloc(nmemb * size)
/*memory define end*/

/*mutex define*/
typedef xSemaphoreHandle                mutex_t;
int mutex_init(mutex_t *mutex, void *attr);
int mutex_free(mutex_t *mutex);
int mutex_lock(mutex_t *mutex);
int mutex_unlock(mutex_t *mutex);
/*mutex define end*/

/*thread define*/
typedef void                            THREAD_VOID;
#define thread_return                   return
#define THREAD_OK                       pdPASS
#define MUTEX_OK			 			pdPASS
typedef xTaskHandle                     thread_t;

int create_thread(THREAD_VOID(*func)(void *arg), const char *name, int stack, void *param, int prio, thread_t *handle, void *attr);
int kill_thread(thread_t handle);
int delete_thread(thread_t handle);
int detach_thread(void);
/*thread define end*/

uint64_t get_process_msec();
#ifdef __cplusplus
}
#endif

#endif/*#ifndef _PLATFORM_H_*/

