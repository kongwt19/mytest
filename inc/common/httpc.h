#ifndef __HTTPC_H__
#define __HTTPC_H__

#define BUFFER_SIZE             1024

typedef struct _url_info
{
    char ip[16];
    short port;
    char *uri;
}URL_INFO_S;

typedef struct _ac_tcpclient{
	int socket;
	int remote_port;     
	char remote_ip[16];  
	struct sockaddr_in _addr; 
	int connected;       
} ac_tcpclient;

int ac_tcpclient_create( ac_tcpclient *pclient, const char *host, int port );
int ac_tcpclient_conn(ac_tcpclient *pclient);
int ac_tcpclient_recv(ac_tcpclient *pclient,char **lpbuff,int size);
int ac_tcpclient_send(ac_tcpclient *pclient,char *buff,int size);
int ac_tcpclient_close(ac_tcpclient *pclient);
int ac_http_post(ac_tcpclient *pclient,char *page,char *request,char **response);
int ac_http_request(ac_tcpclient *pclient,char *lpbuf, int len, char **response); 
int ac_http_get(ac_tcpclient *pclient,char *page,char *request,char **response);
#endif
