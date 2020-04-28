#include "platform.h"
#include "ipp_defines.h"

const char *ipp_ntop(int af, struct in_addr *src, char *dst, int cnt)
{
	return inet_ntop(af, (ip_addr_t*)&src->s_addr, dst, cnt);
}

int mutex_init(mutex_t *mutex, void *attr)
{
    RETURN_IF_NULL(mutex, -1);
    
	*mutex = xSemaphoreCreateMutex();
    RETURN_IF_NULL(*mutex, -1);
	return 0;
}

int mutex_free(mutex_t *mutex)
{
    RETURN_IF_NULL(mutex, -1);
    RETURN_IF_NULL(*mutex, -1);
    vSemaphoreDelete(*mutex);
    return 0;
}

int mutex_lock(mutex_t *mutex)
{
    RETURN_IF_NULL(mutex, -1);
    RETURN_IF_NULL(*mutex, -1);
    return xSemaphoreTake(*mutex, portMAX_DELAY);
}

int mutex_unlock(mutex_t *mutex)
{
    RETURN_IF_NULL(mutex, -1);
    RETURN_IF_NULL(*mutex, -1);
    return xSemaphoreGive(*mutex);
}
/*mutex define end*/

int create_thread(THREAD_VOID(*func)(void *arg), const char *name, int stack, void *param, int prio, thread_t *handle, void *attr)
{
    RETURN_IF_NULL(func, -1);
    RETURN_IF_NULL(handle, -1);

    return xTaskCreate(func, (const signed char *)name, stack / sizeof(portSTACK_TYPE), param, prio, handle);
}

int kill_thread(thread_t handle)
{
    vTaskDelete(handle);
    return 0;
}

int delete_thread(thread_t handle)
{
	os_thread_delete(handle);
	return 0;
}

int detach_thread(void)
{
    return 0;
}
/*thread define end*/

uint64_t get_process_msec()
{
	return (uint64_t)xTaskGetTickCount();
}
