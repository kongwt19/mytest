#include "local_common.h"
#include "platform.h"

//��Դ�ַ���������ָ�����ַ����ڴ�ָ����
int copy_string(char **dest, char *src)
{
	int len;
	FREE_POINTER(*dest);

	if (NULL == src)
		return 1;

	len = ipp_min((int)strlen(src), 128);
	*dest = (char *)malloc(len + 1);
	MALLOC_ERROR_RETURN_WTTH_RET(*dest, -1)

	strncpy(*dest, src, len);
	*(*dest + len) = '\0';
	return 1;
}
