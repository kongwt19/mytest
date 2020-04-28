#ifndef _IPP_DEFINES_H_
#define _IPP_DEFINES_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef char IPP_BOOL;

#ifndef UBOUND
#define UBOUND(arr) (int32_t)((sizeof(arr)) / (sizeof(arr[0])))
#endif

#ifndef ipp_max
#define ipp_max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef ipp_min
#define ipp_min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef BOOL
typedef char BOOL;
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
