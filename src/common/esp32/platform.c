#include "platform.h"
#include "ipp_util.h"
#include "ipp_defines.h"

int mutex_init(mutex_t *mutex, void *attr)
{
	pthread_mutexattr_t attr_temp; 
	int ret;
	if((ret = pthread_mutexattr_init(&attr_temp)) != 0){
		ipp_LogE_Prefix("pthread_mutexattr_init failed!\n");
		return -1;
	}

	if((ret = pthread_mutexattr_settype(&attr_temp, PTHREAD_MUTEX_RECURSIVE_NP)) != 0){
		ipp_LogE_Prefix("pthread_mutexattr_settype failed!\n");
		return -1;
	}
	
	return pthread_mutex_init(mutex, &attr_temp);
}

int mutex_free(mutex_t *mutex)
{
    return pthread_mutex_destroy(mutex);
}

int mutex_lock(mutex_t *mutex)
{
    return pthread_mutex_lock(mutex);
}

int mutex_unlock(mutex_t *mutex)
{
    return pthread_mutex_unlock(mutex);
}
/*mutex define end*/

int create_thread(THREAD_VOID(*func)(void *arg), const char *name, int stack, void *param, int prio, thread_t *handle, void *attr)
{
    return pthread_create(handle, (pthread_attr_t*)attr, func, param);
}

int kill_thread(thread_t handle)
{
    return pthread_kill(handle, SIGUSR2);
}

int detach_thread(void)
{
    return pthread_detach(pthread_self());
}
/*thread define end*/

uint32_t get_process_msec()
{
	return (uint32_t)((double)system_get_time() / 1000.0);
}
