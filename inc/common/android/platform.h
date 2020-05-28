#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>  
#include <sys/ioctl.h>
#include <sys/wait.h>  
#include <arpa/inet.h>
#include <linux/types.h>  
#include <linux/netlink.h> 
#include <pthread.h>
#include <signal.h>
#include <ctype.h>  
#include <errno.h>  
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif


#define sleep_ms(x)                     usleep(x * 1000)
#define sleep_s(s)						sleep(s)
#define sleep_us(us)					usleep(us)

#define  TAG    "[TVGW]"

/*log function define*/
#define LogV_Prefix(__fmt__, ...)  __android_log_print(ANDROID_LOG_VERBOSE,TAG, __fmt__,##__VA_ARGS__)
#define LogD_Prefix(__fmt__, ...)  __android_log_print(ANDROID_LOG_DEBUG,TAG, __fmt__,##__VA_ARGS__)
#define LogI_Prefix(__fmt__, ...)  __android_log_print(ANDROID_LOG_INFO,TAG, __fmt__,##__VA_ARGS__)
#define LogW_Prefix(__fmt__, ...)  __android_log_print(ANDROID_LOG_WARN,TAG, __fmt__,##__VA_ARGS__)
#define LogE_Prefix(__fmt__, ...)  __android_log_print(ANDROID_LOG_ERROR,TAG, __fmt__,##__VA_ARGS__)

#define ipp_LogV_Prefix(__fmt__, ...) printf("[IPP]:" __fmt__, ##__VA_ARGS__)
#define ipp_LogD_Prefix(__fmt__, ...) printf("[IPP_DEBUG]:" __fmt__, ##__VA_ARGS__)
#define ipp_LogI_Prefix(__fmt__, ...) printf("[IPP_INFO]:" __fmt__, ##__VA_ARGS__)
#define ipp_LogW_Prefix(__fmt__, ...) printf("[IPP_WARNING]:" __fmt__, ##__VA_ARGS__)
#define ipp_LogE_Prefix(__fmt__, ...) printf("[IPP ERR]:" __fmt__, ##__VA_ARGS__)

#define ipp_LogV(__fmt__, ...) printf(__fmt__, ##__VA_ARGS__)
#define ipp_LogD(__fmt__, ...) printf(__fmt__, ##__VA_ARGS__)
#define ipp_LogI(__fmt__, ...) printf(__fmt__, ##__VA_ARGS__)
#define ipp_LogW(__fmt__, ...) printf(__fmt__, ##__VA_ARGS__)
#define ipp_LogE(__fmt__, ...) printf(__fmt__, ##__VA_ARGS__)
/*log function define end*/

/*mutex define*/
typedef pthread_mutex_t                 mutex_t;
int mutex_init(mutex_t *mutex, void *attr);
int mutex_free(mutex_t *mutex);
int mutex_lock(mutex_t *mutex);
int mutex_unlock(mutex_t *mutex);
/*mutex define end*/

/*thread define*/
typedef void*                           THREAD_VOID;
#define thread_return                   return NULL
#define THREAD_OK                       0
typedef pthread_t                       thread_t;
int create_thread(THREAD_VOID(*func)(void *arg), const char *name, int stack, void *param, int prio, thread_t *handle, void *attr);
int kill_thread(thread_t handle);
#define delete_thread(handle)
int detach_thread(void);
/*thread define end*/

const char *ipp_ntop(int af, struct in_addr *src, char *dst, int cnt);
uint64_t get_process_msec();
int8_t check_time_out(uint32_t after, uint32_t before, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif/*#ifndef _PLATFORM_H_*/
