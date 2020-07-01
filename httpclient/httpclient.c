/*
 * HTTP Client Via Socket API
 *
 * Change Logs:
 * Date			Author		Notes
 * 2020.5.24	yuoouy		the first version
 *
 *
 *
*/
#include <socket.h>
#include <string.h>
#include <httpclient.h>
#include <stdlib.h>
#include <stdio.h>
#include "iap_config.h"

static struct HTTPClient httpclient;
static struct Prefix_Struct prefix[] = {{"http://", 7} ,{"https://", 8}};
static char url_buffer[HTTPCLIENT_URL_MAX_SIZE];
static char host_buffer[HTTPCLIENT_HOST_SIZE];
static char ip_addr_buffer[HTTPCLIENT_IPADDR_SIZE];
static char path_buffer[HTTPCLIENT_PATH_SIZE];
static char req_addr_buffer[HTTPCLIENT_REQADDR_SIZE];
static char respheader_buffer[HTTPCLIENT_RESPONSE_BUFSZ];
static char reqheader_buffer[HTTPCLIENT_HEADER_SIZE];
static unsigned char recv_data_buffer[HTTPCLIENT_RECV_BUFFER_SIZE];
static char content_type_buffer[HTTPCLIENT_CONTENT_TYPE_BUFSZ];
static char reqheader_content_range_buf[HTTPCLIENT_CONTENT_RANGE_BUFSZ];

static struct Resp_Status_Operation resp_operation_table[] = 
{
    {100, 101, NoHandle},
    {200, 200, success200},
    {206, 206, success206},
    {201, 205, NoHandle},
    {300, 505, NoHandle}
};

int NoHandle(void)
{
    return -1;
}
int success200(void)
{
    char *ptr = strstr(httpclient.resp_header.buffer, "Content-Type:");
    if (ptr) { sscanf(ptr, "%*s %s", httpclient.resp_header.content_type); }
    else { DBG_LOG("response header not include content type!\n");
        return -1;}
    ptr = strstr(httpclient.resp_header.buffer, "Content-Length:");
    if (ptr) { sscanf(ptr, "%*s %d", &(httpclient.resp_header.content_length));}
    else { DBG_LOG("response header not include content length!\n");
        return -1;}
    return 0;
}
int success206(void)
{
    char *ptr = strstr(httpclient.resp_header.buffer, "Content-Type:");
    if (ptr) { sscanf(ptr, "%*s %s", httpclient.resp_header.content_type); }
    else { DBG_LOG("response header not include content type!\n");
        return -1;}
    ptr = strstr(httpclient.resp_header.buffer, "Content-Length:");
    if (ptr) { sscanf(ptr, "%*s %d", &(httpclient.resp_header.content_length));}
    else { DBG_LOG("response header not include content length!\n");
        return -1;}
    ptr = strstr(httpclient.resp_header.buffer, "Content-Range:");
    if (ptr) { sscanf(ptr, "%*s bytes %d-%d/%d", &httpclient.resp_header.content_range_start, &httpclient.resp_header.content_range_end, &httpclient.resp_header.content_overall_length);}
    else { DBG_LOG("response header not include content range!\n");
        return -1;}
    return 0;
}

/**
 * connect to http server
 *
 * @param url the input server URL address
 *
 * @return   0: connect successed
*			<0: connect failed or other error
 */
int http_connect(char *url)
{
	struct addrinfo *res = NULL;
	struct timeval timeout;
	int sock;
    int parse_failed_count = 0;
    int socket_failed_count = 0;
    int connect_failed_count = 0;
	
	strcpy(httpclient.url, url);
    
__parser:	
	if (http_addr_parse(&res, url) == -1 || res == NULL)
	{
		DBG_LOG("http_addr_parse failed%d!\n", parse_failed_count);
        rt_thread_delay(RT_TICK_PER_SECOND);
        parse_failed_count++;
        if (parse_failed_count > 10)
        {
            parse_failed_count = 0;
            goto __deinit_esp8266;
        }
        goto __parser;
	}
    
__socket:    
	sock = socket(res->ai_family, SOCK_STREAM, IPPROTO_TCP);
    DBG_LOG("socket socket socket\n\n");
	if (sock < 0)
    {
        DBG_LOG("socket(%d) create failed%d!\n", sock, socket_failed_count);
        rt_thread_delay(RT_TICK_PER_SECOND);
        socket_failed_count++;
        if (socket_failed_count > 10)
        {
            socket_failed_count = 0;
            goto __parser;
        }
        goto __socket;
    }

    /* set receive and send timeout option */
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeout, sizeof(timeout));
    
__connect:
    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0)
    {
        /* connect failed, close socket */
        DBG_LOG("connect failed%d, connect socket(%d) error.", connect_failed_count,  sock);
        rt_thread_delay(RT_TICK_PER_SECOND);
        connect_failed_count++;
        if (connect_failed_count > 10)
        {
            connect_failed_count = 0;
            closesocket(sock);

            goto __socket;
        }
        goto __deinit_esp8266;
    }

    httpclient.socket = sock;
	
	DBG_LOG("httpclient connected!\n");
	goto __exit;

__deinit_esp8266:
    while(esp8266_net_init() != 0);
    goto __parser;
    
__exit:
	{
		return 0;
								   

										  
			
	}
}


/**
 * read data to buffer from http server
 *
 * @param buffer: data buffer
 * @param length: data length
 *
 * @return  >0:		read data length
 *			<=0: 	read failed or http server disconnect
 */
int http_read(unsigned char *buffer, int length)
{
    int bytes_read = 0;

    if (length == 0)
    {
        return -1;
    }

    bytes_read = recv(httpclient.socket, buffer, length, 0);
    if (bytes_read <= 0)
    {
        closesocket(httpclient.socket);
        httpclient.socket = -1;
        return -1;
    }
	else
	{
		return bytes_read;
	}
}

/**
 *  write data to http server.
 *
 * @param buffer: data buffer
 * @param length: data length
 *
 * @return  >0:		write data length
 *			<=0: 	write failed or http server disconnect
 */
int http_write(const unsigned char *buffer, int length)
{
    int bytes_write = 0;

    if (length == 0)
    {
        return -1;
    }

    bytes_write = send(httpclient.socket, buffer, length, 0);
    if (bytes_write <= 0)
    {
        closesocket(httpclient.socket);
        httpclient.socket = -1;
		return -1;
    }
	else
	{
		return bytes_write;
	}
}

/**
 * close a http client session.
 *
 * @param none
 *
 * @return 0: close success
 */
int http_close(void)
{

    if (httpclient.socket >= 0)
    {
        closesocket(httpclient.socket);
        httpclient.socket = -1;
    }

    return 0;
}

/**
 * get httpclient request response data.
 *
 * @param buffer response buffer
 *
 * @return   >0: response data size
* 			<0: buffer is overflow or recevice error
 */
int http_recv_respheader(void)
{
    int length;
	char *buf_ptr;
	int flag = 0;
	buf_ptr = httpclient.resp_header.buffer;
    int count = 0;

    DBG_LOG("waiting response...\n");
__again:
    {
        /* waiting response header */
        length = http_read((unsigned char *)buf_ptr, 1);
        if (length <= 0)
		{
            DBG_LOG("Recive error, waiting...\n");
            rt_thread_delay( RT_TICK_PER_SECOND );
            count += 1;
            if (count < 3)
            {
                goto __again;
            }
            else
            {
                DBG_LOG("response timeout!\n");
                *buf_ptr = '\0';
                httpclient.resp_header.length = buf_ptr - httpclient.resp_header.buffer;
                DBG_LOG("Recive failed! Maybe the connect is break!\n");
                return -1;
            }

		}

		if (flag == 0)	{if (*buf_ptr == '\r') flag = 1;}
		else if (flag == 1)
		{
			if (*buf_ptr == '\n') flag = 2;
			else if (*buf_ptr == '\r')flag = 1;
			else flag = 0;
		}
		else if (flag == 2)
		{
			if (*buf_ptr == '\r') flag = 3;
			else flag = 0;
		}
		else if (flag == 3)
		{
			if (*buf_ptr == '\n')
			{
				*(buf_ptr + 1) = '\0';
				goto __exit;
			}
			else if (*buf_ptr == '\r')flag = 1;
			else flag = 0;
		}

		buf_ptr += length;
		if (buf_ptr - httpclient.resp_header.buffer >= HTTPCLIENT_RESPONSE_BUFSZ - 2)
		{
			httpclient.resp_header.length = buf_ptr - httpclient.resp_header.buffer + 1;
			DBG_LOG("response data is too large, pointer is close to the buffer end!\n");
			return -1;
		}
        goto __again;
    }

__exit:
	httpclient.resp_header.length = buf_ptr - httpclient.resp_header.buffer + 1;
    DBG_LOG("response header length is :%d\n", httpclient.resp_header.length);
    return httpclient.resp_header.length;
}

/**
 * httpclient send request data.
 *
 * @param none
 *
 * @return >0: send data length
 *			<0: send failed or header buffer error
 */
int http_send_reqheader(void)
{
	

	int ret = http_write((unsigned char *)httpclient.req_header.buffer, httpclient.req_header.length);
	if (ret <= 0)
	{
		DBG_LOG("header send failed!\n");
		return -1;
	}
	else
	{
		return ret;
	}
}



/**
 * parse url
 *
 * @param res address info struct pointer
 * @param url the input server URL address
 *
 * @return   0: parse successed
*			<0: parse failed or other error
 */
static int http_addr_parse(struct addrinfo **res, char *url)
{
	int url_length;
	url_length = strlen(url);
	if (url_length > HTTPCLIENT_URL_MAX_SIZE + 1)
	{
		DBG_LOG("url address is too long!\n");
		return -1;
	}

	/*查看是否有前缀*/

	if (strncmp(url, prefix[1].prefix, prefix[1].length) == 0)
	{
		DBG_LOG("not support https!");
		return -1;
	}
	httpclient.prefix_num = -1;
	if (strncmp(url, prefix[0].prefix, prefix[0].length) == 0)
	{
		httpclient.prefix_num = 0;
	}

	/*分离主机地址端口和路径*/
	int i;
	for (i = prefix[httpclient.prefix_num].length; url[i] != '/' && url[i] != '\0'; i++)
	{
		httpclient.host[i - prefix[httpclient.prefix_num].length] = url[i];
       // DBG_LOG("url[%d] : %c\n", i, url[i]);
	}

	if (url[i] == '\0')
	{
		DBG_LOG("url is not include path!\n");
		return -1;
	}
	else if (url[i] == '/')
	{
		/*get path*/
		strcpy(httpclient.path, &url[i]);
        DBG_LOG("path is :%s\n", httpclient.path);
	}
	else
	{
		DBG_LOG("never reach here!\n");
	}
	strcpy(httpclient.req_addr, httpclient.host);

	/*detach host address and port*/
	char *p = strchr(httpclient.host, ':');
	if (p != NULL)
	{
		httpclient.port = atoi(p + 1);
		*p = '\0';
	}
	else
	{
		httpclient.port = 80;
	}

	 /* resolve the host name. */
    struct addrinfo hint;
    int ret;

    memset(&hint, 0, sizeof(hint));
    DBG_LOG("host is :%s\n", httpclient.host );
    ret = getaddrinfo(httpclient.host, p + 1, &hint, res);
    if (ret != 0)
    {
		DBG_LOG("address resolve failed!\n");
        return -1;
    }
    return 0;
}

struct Resp_Status_Operation *get_resp_status_op(void)
{
    //DBG_LOG("resp_operation_table size: %d,httpclient.resp_header.status: %d\n", sizeof(resp_operation_table)/sizeof(struct Resp_Status_Operation), httpclient.resp_header.status);
    int i;
    int size = sizeof(resp_operation_table)/sizeof(struct Resp_Status_Operation);
    for(i = 0; i < size; i++)
    {
//        DBG_LOG("httpclient.resp_header.status : %d\n", httpclient.resp_header.status);
//        DBG_LOG("resp_operation_table[i].start_status : %d\n", resp_operation_table[i].start_status);
//        DBG_LOG("resp_operation_table[i].end_status : %d\n", resp_operation_table[i].end_status);
        if (httpclient.resp_header.status >= resp_operation_table[i].start_status && httpclient.resp_header.status <= resp_operation_table[i].end_status)
        {
            return &resp_operation_table[i];
        } 
    }
    if (i == size)
    {
        return NULL;
    }
}
/**
 * response header parse
 *
 * @param none
 *
 * @return   0: parse success
 *          -1: header error or empty
 */
int http_respheader_parse(void)
{
	char *ptr = strstr(httpclient.resp_header.buffer, "HTTP/");
    if (ptr) { sscanf(ptr, "%*s %d", &(httpclient.resp_header.status));}
    else { DBG_LOG("response header error!\n");
        return -1;}
    struct Resp_Status_Operation *resp_sta_op = get_resp_status_op();
    if (resp_sta_op == NULL)
    {
        DBG_LOG("http status code not support!\n");
        return -1;
    }
    else
    {
        return resp_sta_op->func();
	}
}


/**
 * make a request header
 *
 * @param header The header will be added
 *
 * @return  >0: header length
*			<0: add failed or other error
 */
int http_make_reqheader(struct HTTPClient * httpcli_t)
 
									   
{ 
    httpcli_t->req_header.length = 0;
    int ret;
	if (httpcli_t->req_header.break_point_resume == 0)
	{
		ret = sprintf(httpcli_t->req_header.buffer, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
            "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.88 Safari/537.36\r\n"\
            "Host: %s\r\n"\
            "Connection: keep-alive\r\n"\
            "\r\n"\
        ,httpcli_t->path, httpcli_t->req_addr);
        DBG_LOG("request header length is: %d\n", strlen(httpcli_t->req_header.buffer));
        DBG_LOG("request header is: %s\n", httpcli_t->req_header.buffer);
		if (ret < 0)
		{
			DBG_LOG("add header failed!\n");
			return -1;
		}
		else
		{
			httpcli_t->req_header.length = ret;
		}
	}
	else
    {
        ret = sprintf(httpcli_t->req_header.buffer, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
            "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.88 Safari/537.36\r\n"\
            "Host: %s\r\n"\
            "Range: bytes=%s\r\n"\
            "Connection: keep-alive\r\n"\
            "\r\n"\
        ,httpcli_t->path, httpcli_t->req_addr, httpcli_t->req_header.content_range);
        DBG_LOG("request header length is: %d\n", strlen(httpcli_t->req_header.buffer));
        DBG_LOG("request header is: %s\n", httpcli_t->req_header.buffer);
		if (ret < 0)
		{
			DBG_LOG("add header failed!\n");
			return -1;
		}
		else
		{
			httpcli_t->req_header.length = ret;
		}
    }
	return httpcli_t->req_header.length;
}

/**
 * http client print response header
 *
 * @param none
 *
 * @return   0: print successed
*          -1: print failed
 */
int http_print_resp_header(void)
{
    if (httpclient.resp_header.length <= 0)
    {
        DBG_LOG("response header is empty!\n");
        return -1;
    }
    DBG_LOG("recive response header is :\n%s\n", httpclient.resp_header.buffer);
    return 0;
}

/**
 * http client print response header
 *
 * @param none
 *
 * @return   0: print successed
*          -1: print failed
 */
int http_print_resp_header2(void)
{
    if (httpclient.resp_header.length <= 0)
    {
        DBG_LOG("response header is empty!\n");
        return -1;
    }
    DBG_LOG("HTTP/: %d\n", httpclient.resp_header.status);
    DBG_LOG("content type: %s\n", httpclient.resp_header.content_type);
    DBG_LOG("content length: %d\n\n", httpclient.resp_header.content_length);
    return 0;
}

/**
 * http client initialization
 *
 * @param none
 *
 * @return   0: init successed
 */
int httpclient_init(void)
{
    httpclient.req_header.length = 0;
	httpclient.req_header.buffer = reqheader_buffer;
    httpclient.resp_header.length = 0;
    httpclient.resp_header.buffer = respheader_buffer;
	httpclient.host = host_buffer;
	httpclient.ip_addr = ip_addr_buffer;
	httpclient.path = path_buffer;
	httpclient.req_addr = req_addr_buffer;
	httpclient.url = url_buffer;
	httpclient.data_buffer = recv_data_buffer;
    httpclient.resp_header.content_type = content_type_buffer;
	httpclient.req_header.break_point_resume = 0;
    httpclient.req_header.content_range = reqheader_content_range_buf;															  

	return 0;
}

/**
 * get httpclient pointer
 *
 * @param none
 *
 * @return   struct HTTPClient pointer
 */
struct HTTPClient *get_httpclient(void)
{
    return &httpclient;
}
