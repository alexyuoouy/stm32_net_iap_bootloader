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
static char recv_data_buffer[HTTPCLIENT_RECV_BUFFER_SIZE];
static char content_type_buffer[HTTPCLIENT_CONTENT_TYPE_BUFSZ];

/**
 * get http client struct pointer
 *
 * @param none
 *
 * @return   http client struct pointer
 */
struct HTTPClient * get_httpclient(void)
{
    return &httpclient;
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

	strcpy(httpclient.url, url);

	if (http_addr_parse(&res, url) == -1 || res == NULL)
	{
		DBG_LOG("http_addr_parse failed!\n");
		goto __exit;
	}
	http_add_reqheader(NULL);
	sock = socket(res->ai_family, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
    {
        DBG_LOG("socket(%d) create failed!\n", sock);
        goto __exit;
    }

    /* set receive and send timeout option */
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeout, sizeof(timeout));

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0)
    {
        /* connect failed, close socket */
        DBG_LOG("connect failed, connect socket(%d) error.", sock);
        closesocket(sock);

        goto __exit;
    }

    httpclient.socket = sock;

	DBG_LOG("httpclient connected!\n");
	return 0;

__exit:
	{
		if (res) freeaddrinfo(res);
		if ( sock > 0) closesocket(sock);

		DBG_LOG("httpclient connect failed!\n");
		return -1;
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
int http_read(char *buffer, int length)
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
int http_write(const char *buffer, int length)
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
        length = http_read(buf_ptr, 1);
        if (length <= 0)
		{
            DBG_LOG("Recive error, waiting...\n");
            rt_thread_delay( RT_TICK_PER_SECOND );
            count += 1;
            if (count < 6)
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
	if (httpclient.req_header.length <= 0)
	{
		if (http_add_reqheader(NULL) < 0)
		{
			DBG_LOG("add header failed!\n");
			return -1;
		}
	}

	int ret = http_write(httpclient.req_header.buffer, httpclient.req_header.length);
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

	/*�鿴�Ƿ���ǰ׺*/

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

	/*����������ַ�˿ں�·��*/
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

	/*detach host address �� port*/
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
	if (httpclient.resp_header.length <= 0)
	{
		DBG_LOG("response buffer is empty!\n");
		return -1;
	}
	else
	{
		char *ptr = strstr(httpclient.resp_header.buffer, "HTTP/");
        if (ptr) { sscanf(ptr, "%*s %d", &(httpclient.resp_header.status));}
        else { DBG_LOG("response header error!\n");
            return -1;}
        ptr = strstr(httpclient.resp_header.buffer, "Content-Type:");
        if (ptr) { sscanf(ptr, "%*s %s", httpclient.resp_header.content_type); }
        else { DBG_LOG("response header error!\n");
            return -1;}
        ptr = strstr(httpclient.resp_header.buffer, "Content-Length:");
        if (ptr) { sscanf(ptr, "%*s %d", &(httpclient.resp_header.content_length));}
        else { DBG_LOG("response header error!\n");
            return -1;}

        return 0;
	}
}


/**
 * add request header
 *
 * @param header The header will be added
 *
 * @return  >0: header length
*			<0: add failed or other error
 */
int http_add_reqheader(char *header)
{
	if (httpclient.req_header.length == 0)
	{
		int ret;

		ret = sprintf(httpclient.req_header.buffer, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
            "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.88 Safari/537.36\r\n"\
            "Host: %s\r\n"\
            "Connection: keep-alive\r\n"\
            "\r\n"\
        ,httpclient.path, httpclient.host);
        DBG_LOG("request header length is: %d\n", strlen(httpclient.req_header.buffer));
        DBG_LOG("request header is: %s\n", httpclient.req_header.buffer);
		if (ret < 0)
		{
			DBG_LOG("add header failed!\n");
			return -1;
		}
		else
		{
			httpclient.req_header.length = ret;
		}
	}
	if (header != NULL)
	{
		strcpy(httpclient.req_header.buffer + httpclient.req_header.length, header);
		httpclient.req_header.length = strlen(httpclient.req_header.buffer) - 1;
	}

	return httpclient.req_header.length;
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

	return 0;
}
