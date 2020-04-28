#ifndef __ERROR_CODE_H__
#define __ERROR_CODE_H__

//Common error code
#define GW_OK			0
#define GW_ERR			-1
#define GW_NULL_PARAM	-2
#define GW_MALLOR_FAIL	-3

//Cloud error code
#define	GW_HTTP_FAIL	-1000

//File option error code
#define GW_FILE_OPEN_FAIL   -2000
#define GW_FILE_WRITE_FAIL  -2001
#define GW_FILE_READ_FAIL   -2002
#define GW_FILE_NOT_EXISTS  -2003
#define GW_FILE_MSG_NULL	-2004

#define GW_CJSON_NULL	-2005

#endif/*__ERROR_CODE_H__*/
