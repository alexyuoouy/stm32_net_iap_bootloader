/*
 * HTTP Client  Via Socket API
 *
 * Change Logs:
 * Date			Author		Notes
 * 2020.5.24	yuoouy		the first version
 *
 *
 *
*/
#ifndef __HTTPCLIENT__
#define __HTTPCLIENT__

#include "iap_config.h"
#include "stm32f1xx.h"


#define HTTPCLIENT_URL_MAX_SIZE 						128
#define HTTPCLIENT_HOST_SIZE							32
#define HTTPCLIENT_IPADDR_SIZE							16
#define HTTPCLIENT_PATH_SIZE							64
#define HTTPCLIENT_REQADDR_SIZE							64
#define HTTPCLIENT_HEADER_SIZE							1024
#define HTTPCLIENT_RECV_BUFFER_SIZE						4096
#define HTTPCLIENT_RESPONSE_BUFSZ						1024
#define HTTPCLIENT_CONTENT_TYPE_BUFSZ				    64
#define HTTPCLIENT_CONTENT_RANGE_BUFSZ				    16

struct addrinfo {
    int               ai_flags;              /* Input flags. */
    int               ai_family;             /* Address family of socket. */
    int               ai_socktype;           /* Socket type. */
    int               ai_protocol;           /* Protocol of socket. */
    uint32_t         ai_addrlen;             /* Length of socket address. */
    struct sockaddr  *ai_addr;               /* Socket address of socket. */
    char             *ai_canonname;          /* Canonical name of service location. */
    struct addrinfo  *ai_next;               /* Pointer to next in list. */
};


struct HTTPClient_Req_Header
{
	char *buffer;
    int length;                    		/* 长度 */
                 
    char *content_range;                /* 下载范围 */
    int break_point_resume;
};

struct HTTPClient_Resp_Header
{
    char *buffer;						/* 缓存 */
    int length;							/* 长度 */
    int status;                         /* 状态码 */
    char *content_type;					/* 内容类型 */
    int content_length;		    		/* 内容长度 */
    int content_overall_length;         /* 文件总长 */
    int content_range_start;            /* 起始字节 */
    int content_range_end;              /* 终止字节 */
};

struct Resp_Status_Operation
{
    int start_status;
    int end_status;
    int (* func)(void);                /* 状态码在某个区间内的采取相同操作 */

};

struct HTTPClient
{
	struct HTTPClient_Req_Header req_header;
	struct HTTPClient_Resp_Header resp_header;

	int socket;

	char *url;
	int prefix_num;						     /* url prefix number */
	char *host;							     /* host address */
	int port;							     /* port */
	char *ip_addr;						     /* ip address */
	char *path;						         /* file path */
	char *req_addr;						     /* request address */
    unsigned char *data_buffer;              /* receive data buffer */
};

struct Prefix_Struct
{
	char *prefix;						     /* url prefix */
	int length;							     /* prefix length */
};

struct timeval {
    long    tv_sec;                          /* seconds */
    long    tv_usec;                         /* and microseconds */
};


struct HTTPClient * get_httpclient(void);
int http_print_resp_header(void);
int httpclient_init(void);
int http_connect(char *url);
int http_read(unsigned char *buffer, int length);
int http_write(const unsigned char *buffer, int length);
int http_close(void);
int http_send_reqheader(void);
int http_recv_respheader(void);
int http_make_reqheader(struct HTTPClient * httpcli_t);
int http_respheader_parse(void);
int http_print_resp_header2(void);
static int http_addr_parse(struct addrinfo **res, char *url);

int NoHandle(void);
int success200(void);
int success206(void);

struct Resp_Status_Operation *get_resp_status_op(void);
#endif
