#include "platform.h"
#include "httpc.h"
#include "defines.h"

int ac_tcpclient_create(ac_tcpclient *pclient,const char *host, int port)
{
	if(pclient == NULL)
	{
		return -1;
	}
	
	memset(pclient,0,sizeof(ac_tcpclient));
	pclient->remote_port = port;
	strncpy(pclient->remote_ip, host,sizeof(pclient->remote_ip) - 1);

	pclient->_addr.sin_family = AF_INET;
	pclient->_addr.sin_port = htons(port);
	pclient->_addr.sin_addr.s_addr = inet_addr(host);
   
 	pclient->socket = socket(AF_INET,SOCK_STREAM,0);
	if(pclient->socket < 0)
	{
		LogE_Prefix("CREATE SOCKET ERROR! RETSET NOW!\r\n");
		return -1;
	}

	return 0;
}


int ac_tcpclient_conn(ac_tcpclient *pclient)
{
	int ret = -1;
    fd_set fdr, fdw;
    int fd_flag;
    struct timeval timeout;
	
	if(pclient->connected)
	{
		return 0;
	}

#if defined(_WIN32)
	u_long non_blk = 1;
	if(0 > ioctlsocket(pclient->socket, FIONBIO, &non_blk))
	{
		LogE_Prefix("Set fd_flag error:%d\r\n", errno);
        return -1;
	}
#else
    fd_flag = fcntl(pclient->socket, F_GETFL, 0);
    if (fd_flag < 0) {
        LogE_Prefix("Get fd_flag error:%d\r\n", errno);
        return -1;
    }
    fd_flag |= O_NONBLOCK;
    if (fcntl(pclient->socket, F_SETFL, fd_flag) < 0) {
        LogE_Prefix("Set fd_flag error:%d\r\n", errno);
        return -1;
    }
#endif

	int connect_flag = connect(pclient->socket, (struct sockaddr *)&(pclient->_addr),sizeof(struct sockaddr_in));	 
   
    if (connect_flag != 0) {
        if (errno == EINPROGRESS) {
            //���ڴ�������
            FD_ZERO(&fdr);
            FD_ZERO(&fdw);
            FD_SET(pclient->socket, &fdr);
            FD_SET(pclient->socket, &fdw);
            timeout.tv_sec  = 2;  // 2s timeout
            timeout.tv_usec = 0;
            connect_flag = select(pclient->socket + 1, &fdr, &fdw, NULL, &timeout);
            //select����ʧ��
            if (connect_flag < 0) {
            	ret = -1;
                LogE_Prefix("connect error:%d\r\n", errno);     
            }
            //���ӳ�ʱ
            else if (connect_flag == 0) {
				ret = -1;
                LogE_Prefix("Connect timeout.\r\n");
            }
			else
			{
				if (connect_flag == 1 && FD_ISSET(pclient->socket, &fdw))
				{
					   ret = 0;
					   pclient->connected = 1;
				}
				else
				{
 					   ret = -1;
				}
			}
        } 
		else
		{
			ret = -1;
			LogE_Prefix("connect failed, error:%d\r\n", errno);
		}
    } 
    else
    {
		 ret = 0;
		 pclient->connected = 1;
	}

#if defined(_WIN32)
	u_long blk = 0;
	if(0 > ioctlsocket(pclient->socket, FIONBIO, &blk))
	{
		LogE_Prefix("Set fd_flag error:%d\r\n", errno);
        return -1;
	}
#else	
    fd_flag = fcntl(pclient->socket, F_GETFL, 0);
    if (fd_flag < 0) {
	   	 LogE_Prefix("Get fd_flag error:%d\r\n", errno);
	   	 return -1;
    }
    fd_flag &= -O_NONBLOCK;
    if (fcntl(pclient->socket, F_SETFL, fd_flag) < 0) {
	   	 LogE_Prefix("Set fd_flag error:%d\r\n", errno);
	   	 return -1;
    }
#endif

	return ret;
}

int ac_tcpclient_recv(ac_tcpclient *pclient,char **lpbuff,int size)
{
	int 				recvnum = 0,tmpres = 0;
	char 				buff[BUFFER_SIZE];
	int 				ret;
	fd_set 			fd_read;
	struct timeval 		tv;
	int nTryTime = 1;
    char *temp = NULL;
	
	FREE_POINTER(*lpbuff);
	*lpbuff = NULL;	
	memset(buff, 0, BUFFER_SIZE);
	   
	while(recvnum < size || size == 0)
	{
		tv.tv_sec = 2;
    	tv.tv_usec = 0;
	
		FD_ZERO(&fd_read);
		FD_SET(pclient->socket,&fd_read);
		
		ret = select(pclient->socket + 1, &fd_read, NULL, NULL, &tv);
		if (ret == -1)
      	{
      		nTryTime++;
			LogE_Prefix("err: select() fail\r\n");
			if(nTryTime>=3)
			{
				break;
			}
			continue;
       	}
  		else if (ret > 0)    /*something can be read*/
      	{
			if (FD_ISSET(pclient->socket, &fd_read))
	  		 { 
				tmpres = recv(pclient->socket, buff,BUFFER_SIZE,0);
						
				if(tmpres <= 0)
			    {
					LogI_Prefix("recv errno:%d\r\n", errno);
					break;
				}
					
				recvnum += tmpres; 
				if(*lpbuff == NULL)
				{
					*lpbuff = (char*)calloc(recvnum+1,1);
					if(*lpbuff == NULL)
					{
						LogE_Prefix("Malloc:%s:%d:size %d\r\n", __FUNCTION__, __LINE__, recvnum);
						return -2;
					}
				}
				else
				{
					temp = (char*)realloc(*lpbuff,recvnum+1);  
					if(temp == NULL)
					{
					    FREE_POINTER(*lpbuff);
						LogE_Prefix("Malloc:%s:%d:size %d\r\n", __FUNCTION__, __LINE__, recvnum);
						return -2;
					}
					*lpbuff = temp;
	
				}
				memcpy(*lpbuff+recvnum-tmpres,buff,tmpres);		
	   		}
    	}
   		else    /*select() time out*/
       	{
       	    LogE_Prefix("select() time out\r\n");
 			if(nTryTime>2)
			{
				break;
			}
       	}	
   		nTryTime++;
	}
	
 	return recvnum;
}

int ac_tcpclient_send(ac_tcpclient *pclient,char *buff,int size)
{
	int sent=0,tmpres=0;
	int 				ret;
	fd_set 			fd_write;
	struct timeval 		tv;
	int nTryTime = 1;
	
	while(sent < size)
	{
		tv.tv_sec = 2;
		tv.tv_usec = 0;
	
		FD_ZERO(&fd_write);
		FD_SET(pclient->socket,&fd_write);
	
		ret = select(pclient->socket + 1, NULL, &fd_write, NULL, &tv);

		if (ret == -1)
      	{
      		nTryTime++;
       		LogE_Prefix("err: select() fail\r\n");
			if(nTryTime>=3)
			{
				break;
			}
			continue;
       	}
		else if (ret > 0)	/*something can be read*/
		{    
 			tmpres = send(pclient->socket,buff+sent,size-sent,0);//sock_back
			if (tmpres < 0) //0806 test
			   LogD_Prefix("send errno:%d\r\n", errno);

			if(tmpres == -1)
			{
				return -1;
			}
			sent += tmpres;
 		}
		else
		{
 			if(nTryTime > 2)
			{
				break;
			}
		}
		nTryTime++;
	}
	return sent;
}

int ac_tcpclient_close(ac_tcpclient *pclient)
{
 	close(pclient->socket);
 	pclient->connected = 0;
	return 0;
}

int http_post_buf(char **lpbuf, ac_tcpclient *pclient,char *page,char *request)
{
	int len = 0;
	char post[300] = {0};
	char host[100] = {0};
	char content_len[100] = {0};
	const char *accept = "Accept: */*\r\n";
	int num = 0;
	char header2[100] = {0};
 
	strcpy(header2,"Content-Type: application/json\r\n");
	
	num = sprintf(post,"POST %s HTTP/1.1\r\n",page);
	post[num] = '\0';

	num = sprintf(host,"HOST: %s:%d\r\n",pclient->remote_ip,pclient->remote_port);
	host[num] = '\0';

	num = sprintf(content_len,"Content-Length: %d\r\n\r\n",strlen(request));
	content_len[num] = '\0';

	len = strlen(post) + strlen(host) + strlen(header2) + strlen(content_len) + strlen(request) + strlen(accept) + 1;
	*lpbuf = (char*)malloc(len);
	if(*lpbuf == NULL)
	{
		LogE_Prefix("Malloc:%s:%d:size %d\r\n", __FUNCTION__, __LINE__, len);
		return -2;
	}
	memset(*lpbuf,0,len);
	
  	strcpy(*lpbuf,post);
	strcat(*lpbuf,host);
	strcat(*lpbuf,header2);
	strcat(*lpbuf,accept); 
	strcat(*lpbuf,content_len);
	strcat(*lpbuf,request);
	
	return len;
}

int http_resp_body( char *recvbuf, char **http_body)
{
	char *body = NULL;
	char *line = NULL;
	int i = 0;
	
	body = strstr(recvbuf, "\r\n\r\n") + strlen("\r\n\r\n");

	body = strtok(body, "\r\n");
	
	for (i = 0; i < 2; i++)
	{
		line = strtok(NULL, "\r\n");
		if(line)
		{
			if (strlen(line) > strlen(body))
			{
				body = line;
			}
		}
	}

	if (strstr(body, "HTTP"))
	{
		body = strtok(body, "HTTP");
	}

	*http_body = (char *)malloc(strlen(body) + 1);
	if(*http_body == NULL)
	{
		LogE_Prefix("Malloc:%s:%d\r\n", __FUNCTION__, __LINE__);
		return -1;
	}
	memset(*http_body, 0, strlen(body) + 1);
	
	memcpy(*http_body, body, strlen(body));
	
	return 0;
}


int ac_http_request(ac_tcpclient *pclient,char *lpbuf, int len, char **response) 
{
	char *recvbuf = NULL;
    /*
    char *body = NULL;
	char *line = NULL;
    int line_num = 0;
    int i = 0;
	*/
	
	if( 0 == pclient->connected)
	{
		if(ac_tcpclient_conn(pclient) < 0)
		{	
 			ac_tcpclient_close(pclient);
			return -1;
		}
	}

	if(ac_tcpclient_send(pclient,lpbuf,len) < 0)
	{
		LogE_Prefix("SEND ERROR!\r\n");
		ac_tcpclient_close(pclient);
		return -1;
	}

 	if(ac_tcpclient_recv(pclient,&recvbuf,0) <= 0)
	{
		LogE_Prefix("RECV ERROR!\r\n");
		FREE_POINTER(recvbuf);
		ac_tcpclient_close(pclient);
		return -1;
	}
		
 	if(!recvbuf)
	{
		LogE_Prefix("recvbuff is NULL!\r\n");
		return -1;
	}
 	ac_tcpclient_close(pclient);
    http_resp_body(recvbuf, response); 
	
	FREE_POINTER(recvbuf);
	return 0;
}

int ac_http_post(ac_tcpclient *pclient,char *page,char *request,char **response)
{
	char *lpbuf = NULL;
	int ret = http_post_buf(&lpbuf, pclient, page, request);
 	ret = (ret > 0) ? ac_http_request(pclient, lpbuf, ret, response) : ret;
	FREE_POINTER(lpbuf);
	
	return ret;
}

int http_request_get_buf(char **lpbuf, ac_tcpclient *pclient,char *page)
{
	char get[300];
	char host[100];
	char connet[100];
	int len = 0;

	sprintf(get,"GET %s HTTP/1.1\r\n",page);
	sprintf(host,"Host: %s:%d\r\n",pclient->remote_ip,pclient->remote_port);
	sprintf(connet,"Connection: Close\r\n\r\n");

	len = strlen(get) + strlen(host) + strlen(connet) + 1;
	*lpbuf = (char*)malloc(len);
	if(*lpbuf == NULL)
	{
	    LogE_Prefix("Malloc:%s:%d:size %d\r\n", __FUNCTION__, __LINE__, len);
		return -1;
	}
	memset(*lpbuf,0,len);

	strcpy(*lpbuf,get);
	strcat(*lpbuf,host);
	strcat(*lpbuf,connet);

	return len;
}

int ac_http_get(ac_tcpclient *pclient,char *page,char *request,char **response)
{
	
	char *lpbuf = NULL;
	int ret = http_request_get_buf(&lpbuf, pclient, page);
	(void)request;
	ret = (ret > 0) ? ac_http_request(pclient, lpbuf, ret, response) : ret;
	free(lpbuf);
	return ret;
}

