/*
 * socket API
 * 
 * Change Logs:
 * Date			Author		Notes
 * 2020.6.21	yuoouy		the first version
 *
 *
 *
*/
#ifndef __SOCKET__
#define __SOCKET__



#define socket(domain, type, protocol)                                               /*need to be implemented*/
#define closesocket(socket)                                                                 /*need to be implemented*/
//#define shutdown(socket, how)                               at_shutdown(socket, how)
//#define bind(socket, name, namelen)                         at_bind(socket, name, namelen)
#define connect(socket, name, namelen)                                                  /*need to be implemented*/
//#define sendto(socket, data, size, flags, to, tolen)        at_sendto(socket, data, size, flags, to, tolen)
#define send(socket, data, size, flags)                                       /*need to be implemented*/
#define recv(socket, mem, len, flags)                                    /*need to be implemented*/
//#define recvfrom(socket, mem, len, flags, from, fromlen)    at_recvfrom(socket, mem, len, flags, from, fromlen)
#define getsockopt(socket, level, optname, optval, optlen)           /*need to be implemented*/
#define setsockopt(socket, level, optname, optval, optlen)  )        /*need to be implemented*/

//#define gethostbyname(name)                                 at_gethostbyname(name)
#define getaddrinfo(nodename, servname, hints, res)         )               /*need to be implemented*/
#define freeaddrinfo(ai)                                                                             /*need to be implemented*/

#endif
