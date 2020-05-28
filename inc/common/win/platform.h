#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>  
#include <errno.h>  
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif


#define sleep_ms(x)                     usleep(x * 1000)
#define sleep_s(s)						sleep(s)
#define sleep_us(us)					usleep(us)

/*log function define*/
#define LogD_Prefix(__fmt__, ...) printf("[DEBUG]:" __fmt__, ##__VA_ARGS__)
#define LogI_Prefix(__fmt__, ...) printf("[INFO]:" __fmt__, ##__VA_ARGS__)
#define LogW_Prefix(__fmt__, ...) printf("[WARNING]:" __fmt__, ##__VA_ARGS__)
#define LogE_Prefix(__fmt__, ...) printf("[ERR]:" __fmt__, ##__VA_ARGS__)

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

//typedef long long  int64_t;

#ifdef __cplusplus
}
#endif

#endif/*#ifndef _PLATFORM_H_*/
