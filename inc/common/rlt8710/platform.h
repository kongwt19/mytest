#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <lwip/netdb.h>
#include <semphr.h>
#include <task.h>
#include <projdefs.h>
#include <portmacro.h>
#include <queue.h>

#define LOCAL_SERVICE		        1
//#define SLAVE_DEVICE_ENABLE			1
//#define MCU_UART_ENABLE				1

#define MAX_DEV_NUM  			1
#define IPP_MSG_QUEUE_MAX_LEN	5

//#define BOOL							int
//#define TRUE							1
//#define FALSE							0

#ifdef __cplusplus
extern "C" {
#endif

#if LOG_OPEN_ALL

#define ipp_LogV_Prefix(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogD_Prefix(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogI_Prefix(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogW_Prefix(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogE_Prefix(__fmt__, ...) //printf("[IPP ERR]:"__fmt__, ##__VA_ARGS__)

#define ipp_LogV(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogD(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogI(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogW(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogE(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)

#else

#define ipp_LogV_Prefix(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogD_Prefix(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogI_Prefix(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogW_Prefix(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogE_Prefix(__fmt__, ...) //printf("[IPP ERR]:"__fmt__, ##__VA_ARGS__)

#define ipp_LogV(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogD(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogI(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogW(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)
#define ipp_LogE(__fmt__, ...) //printf("[IPP]:"__fmt__, ##__VA_ARGS__)

#endif//end #if LOG_OPEN_ALL

#define sleep_ms(x)                     vTaskDelay(x / portTICK_RATE_MS)
#define sleep_s(s)						sleep_ms(s * 1000)
#define sleep_us(us)					sleep_ms(us / 1000)
#define ipp_ntoa(in)                    inet_ntoa(in.s_addr)

static inline int ipp_ntop(int af, struct in_addr *src, char *dst, int cnt)
{
	char *ip = inet_ntoa(*src);
    strncpy(dst, ip, cnt);

    return 0;
}

#define realloc(ptr, size)              pvPortReAlloc(ptr, size)

/*mutex define*/
typedef xSemaphoreHandle                mutex_t;
static inline int mutex_init(mutex_t *mutex, void *attr)
{
    if(NULL == mutex) return -1;
    *mutex = xSemaphoreCreateMutex();
    if(NULL == *mutex) return -1;
    return 0;
}

static inline int mutex_free(mutex_t *mutex)
{
    if(NULL == mutex) return -1;
    if(NULL == *mutex) return -1;
    vSemaphoreDelete(*mutex);
    return 0;
}
static inline int mutex_lock(mutex_t *mutex)
{
    if(NULL == mutex) return -1;
    if(NULL == *mutex) return -1;

    return xSemaphoreTake(*mutex, portMAX_DELAY);
}
static inline int mutex_unlock(mutex_t *mutex)
{
    if(NULL == mutex) return -1;
    if(NULL == *mutex) return -1;

    return xSemaphoreGive(*mutex);
}
/*mutex define end*/

/*thread define*/
typedef void                            THREAD_VOID;
#define thread_return                   return
#define THREAD_OK                       pdPASS
#define MUTEX_OK			 			pdPASS
typedef xTaskHandle                     thread_t;

static inline int create_thread(THREAD_VOID(*func)(void *arg), const char *name, int stack, void *param, int prio, thread_t *handle, void *attr)
{
    if(NULL == func) return -1;
    if(NULL == handle) return -1;
    
    return xTaskCreate(func, (const signed char *)name, stack, param, prio, handle);
}
static inline int kill_thread(thread_t handle)
{

    vTaskDelete(handle);
    return 0;
}
static inline int delete_thread(thread_t handle)
{
    vTaskDelete(handle);
	return 0;
}
static inline int detach_thread(void)
{
    return 0;
}
/*thread define end*/

static inline uint64_t get_process_msec()
{
	return (uint64_t)xTaskGetTickCount();
}

#ifdef __cplusplus
}
#endif

#endif/*#ifndef _PLATFORM_H_*/

