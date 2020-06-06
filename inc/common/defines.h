#ifndef _DEFINES_H_
#define _DEFINES_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include "platform.h"

/*To connect servers for test, open this macro define*/
//#define SERVER_FOR_TEST     1


#define SN_LEN          65
#define PRODUCT_ID_LEN  30
#define IPV4_LEN        16
#define MAC_LEN         18
#define VER_LEN         20
#define NAME_LEN		30

#ifndef UBOUND
#define UBOUND(arr) (int32_t)((sizeof(arr)) / (sizeof(arr[0])))
#endif

#ifndef ipp_max
#define ipp_max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef ipp_min
#define ipp_min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#if !defined(_WIN32)
#ifndef BOOL
typedef char BOOL;
#endif
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifndef FREE_POINTER
#define FREE_POINTER(p) \
do\
{\
	if (NULL != p)\
	{\
		free(p);\
		p = NULL;\
	}\
} while (0)
#endif

#ifndef AGENT_STRNCPY
#define AGENT_STRNCPY(dest,src,n)\
do \
{	\
	strncpy(dest, src, n);\
	dest[n] = '\0';\
} while (0);
#endif

#ifndef MALLOC_ERROR
#define MALLOC_ERROR(p) \
if (NULL == p) {\
	LogE_Prefix("malloc:%s(%d)\r\n",__FUNCTION__,__LINE__);\
}
#endif

#ifndef MALLOC_ERROR_RETURN
#define MALLOC_ERROR_RETURN(p) \
if (NULL == p) {\
	LogE_Prefix("malloc:%s(%d)\r\n",__FUNCTION__,__LINE__);\
	return;\
}
#endif

#ifndef MALLOC_ERROR_RETURN_WTTH_RET
#define MALLOC_ERROR_RETURN_WTTH_RET(p, ret) \
if (NULL == p) {\
	LogE_Prefix("malloc:%s(%d)\r\n",__FUNCTION__,__LINE__);\
	return (ret);\
}
#endif

#ifndef BREAK_IF_NULL
#define BREAK_IF_NULL(ptr)  if(NULL == ptr)         { LogE_Prefix("%s-%d:NULL pointer, break!\r\n",     __func__, __LINE__);break; }
#endif

#ifndef CONTINUE_IF_NULL
#define CONTINUE_IF_NULL(ptr)  if(NULL == ptr)      { LogE_Prefix("%s-%d:NULL pointer, continue!\r\n",  __func__, __LINE__);continue; }
#endif

#ifndef RETURN_IF_NULL
#define RETURN_IF_NULL(ptr, ret) if(NULL == ptr)    { LogE_Prefix("%s-%d:NULL pointer, return!\r\n",    __func__, __LINE__);return ret; }
#endif

#ifndef RETURN_VOID_IF_NULL
#define RETURN_VOID_IF_NULL(ptr) if(NULL == ptr)    { LogE_Prefix("%s-%d:NULL pointer, return!\r\n",    __func__, __LINE__);return; }
#endif

#ifndef RETURN_NULL_IF_NULL
#define RETURN_NULL_IF_NULL(ptr) if(NULL == ptr)    { LogE_Prefix("%s-%d:NULL pointer, return!\r\n",    __func__, __LINE__);return NULL; }
#endif

#endif/*#ifndef _IPP_DEFINES_H_*/
