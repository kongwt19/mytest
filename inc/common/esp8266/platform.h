#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <lwip/netdb.h>
#include <espressif/esp_common.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/projdefs.h>
#include <freertos/portmacro.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <nopoll/nopoll.h>
#include <ssl_compat-1.0.h>
#include <esp_common.h>

//#define ALLJOYN_ENABLE		1
//#define SLAVE_DEVICE_ENABLE			1
#define MCU_UART_ENABLE				1
//#define IPC_ENABLE                  1

#define MAX_DEV_NUM  			1
#define IPP_MSG_QUEUE_MAX_LEN	2

#define sleep_ms(x)                     vTaskDelay(x / portTICK_RATE_MS)

#define ipp_ntoa(in)                    inet_ntoa(in.s_addr)

/*mutex define*/
typedef xSemaphoreHandle                mutex_t;
static inline int mutex_init(mutex_t *mutex, void *attr)
{
    RETURN_IF_NULL(mutex, -1);
    
	*mutex = xSemaphoreCreateMutex();
    RETURN_IF_NULL(*mutex, -1);
	return 0;
}
static inline int mutex_free(mutex_t *mutex)
{
    RETURN_IF_NULL(mutex, -1);
    RETURN_IF_NULL(*mutex, -1);
    vSemaphoreDelete(*mutex);
    return 0;
}
static inline int mutex_lock(mutex_t *mutex)
{
    RETURN_IF_NULL(mutex, -1);
    RETURN_IF_NULL(*mutex, -1);
    return xSemaphoreTake(*mutex, portMAX_DELAY);
}
static inline int mutex_unlock(mutex_t *mutex)
{
    RETURN_IF_NULL(mutex, -1);
    RETURN_IF_NULL(*mutex, -1);
    return xSemaphoreGive(*mutex);
}
/*mutex define end*/

/*thread define*/
typedef void                            THREAD_VOID;
#define thread_return                   return
#define THREAD_OK                       pdPASS
typedef xTaskHandle                     thread_t;
static inline int create_thread(THREAD_VOID(*func)(void *arg), const char *name, int stack, void *param, int prio, thread_t *handle, void *attr)
{
    RETURN_IF_NULL(func, -1);
    RETURN_IF_NULL(handle, -1);

    return xTaskCreate(func, (const signed char *)name, stack, param, prio, handle);
}
static inline int kill_thread(thread_t handle)
{
    vTaskDelete(handle);
    return 0;
}
static inline int detach_thread(void)
{
    return 0;
}
/*thread define end*/

#endif/*#ifndef _PLATFORM_H_*/

