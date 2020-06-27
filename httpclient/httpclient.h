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



#define HTTPCLIENT_URL_MAX_SIZE 						128
#define HTTPCLIENT_HOST_SIZE							32
#define HTTPCLIENT_IPADDR_SIZE							16
#define HTTPCLIENT_PATH_SIZE							64
#define HTTPCLIENT_REQADDR_SIZE							64
#define HTTPCLIENT_HEADER_SIZE							1024
#define HTTPCLIENT_RECV_BUFFER_SIZE						4096
#define HTTPCLIENT_RESPONSE_BUFSZ						1024
#define HTTPCLIENT_CONTENT_TYPE_BUFSZ				    64

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
	char *buffer;                            /* request header buffer */
    int length;                    		     /* length */
};

struct HTTPClient_Resp_Header
{
    char *buffer;						     /* response header buffer */

	int status;							     /* request status */
	int length;							     /* response content length */
	char *content_type;					     /* response content type */
	int content_length;				         /* response content length */
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
    char *data_buffer;                       /* receive data buffer */
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
int http_read(char *buffer, int length);
int http_write(const char *buffer, int length);
int http_close(void);
int http_send_reqheader(void);
int http_recv_respheader(void);
int http_add_reqheader(char *header);
int http_respheader_parse(void);
int http_print_resp_header2(void);
static int http_addr_parse(struct addrinfo **res, char *url);

#endif
