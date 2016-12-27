/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：ennSocket.h
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/
#ifndef _ENNSOCKET_H_
#define _ENNSOCKET_H_


#ifdef __cplusplus
extern "C"
{
#endif

#define     SO_NONBLOCK    0x1016      /* set/get blocking mode via optval param */

#define SERVER_PORT 502
#define bzero(d,l) memset(d,0,l)

typedef struct ENNSock_SocketAddr
{
    unsigned short     sa_family;
    char   sa_data[14];
} ENNSock_SocketAddr;

typedef struct ENNSock_In_Addr
{
  unsigned int  s_addr;
}ENNSock_In_Addr;

typedef struct ENNSock_SocketAddr_In
{
   unsigned short    sin_family;
   unsigned short  sin_port;
   struct   ENNSock_In_Addr  sin_addr;
   unsigned char     sin_zero[8];
} ENNSock_SocketAddr_In;

typedef struct  ENNSock_fd_set  /* the select socket array manager */
{
   //unsigned int fd_count;               /* how many are SET? */
   unsigned long     fd_array[FD_SETSIZE];   /* an array of SOCKETs */
} ENNSock_fd_set;

typedef struct ENNSock_Timeval
{
   long tv_sec;     /* seconds */
   long tv_usec;    /* and microseconds */
}ENNSock_Timeval;

typedef struct ENNSock_Hostent
{
   char *   h_name;        /* Official name of host. */
   char **h_aliases;       /* Alias list.  */
   int   h_addrtype;       /* Host address type.  */
   int   h_length;         /* Length of address.  */
   char **h_addr_list;     /* List of addresses from name server.  */
#define  h_addr   h_addr_list[0] /* Address, for backward compatibility.  */
}ENNSock_Hostent;

ENN_ErrorCode_t ENNSock_Init(ENN_VOID);
ENN_S32 ENNSock_Socket(ENN_S32 domain, ENN_S32 type, ENN_S32 protocol);
ENN_S32 ENNSock_Bind(ENN_S32 sockfd,  const ENNSock_SocketAddr *my_addr, ENN_S32 addrlen);
ENN_S32 ENNSock_Connect(ENN_S32 sockfd, const ENNSock_SocketAddr *addr, ENN_S32 addrlen);
ENN_S32 ENNSock_Listen(ENN_S32 sockfd, ENN_S32 backlog);
ENN_S32 ENNSock_Accept(ENN_S32 sockfd, ENNSock_SocketAddr *addr, ENN_S32 *addrlen);
ENN_S32 ENNSock_SocketSend(ENN_S32 sockfd, const ENN_VOID *msg, ENN_S32 len, ENN_S32 flags);
ENN_S32 ENNSock_SocketSendTo(ENN_S32 sockfd, const ENN_VOID *msg, ENN_S32 len, ENN_S32 flags, const ENNSock_SocketAddr *to, ENN_S32 tplen);
ENN_S32 ENNSock_SocketRecv(ENN_S32 sockfd, ENN_VOID *buf, ENN_S32 len, ENN_S32 flags);
ENN_S32 ENNSock_SocketRecvFrom(ENN_S32 sockfd, ENN_VOID *buf, ENN_S32 len, ENN_S32 flags, ENNSock_SocketAddr *from, ENN_S32 *fromlen);
ENN_S32 ENNSock_GetSockOpt(ENN_S32 sockfd, ENN_S32 level, ENN_S32 optname, ENN_VOID *optval, ENN_S32  *optlen);
ENN_S32 ENNSock_SetSockOpt(ENN_S32 sockfd, ENN_S32 level, ENN_S32 optname, const ENN_VOID *optval, ENN_S32  optlen);
ENN_S32 ENNSock_SocketClose(ENN_S32 sockfd);
ENN_S32 ENNSock_Select(ENN_S32 n, ENNSock_fd_set *readfds, ENNSock_fd_set *writefds, ENNSock_fd_set *exceptfds,ENNSock_Timeval *timeout);
ENN_S32 ENNSock_Ioctl(ENN_S32 d, ENN_S32 request, ...);
ENN_S32  ENNSock_Shutdown(ENN_S32 s, ENN_S32 how);
ENN_VOID ENNSock_FD_ZERO(ENNSock_fd_set *set);
ENN_VOID ENNSock_FD_SET(ENN_S32 fd, ENNSock_fd_set *set);
ENN_S32 ENNSock_FD_ISSET(ENN_S32 fd, ENNSock_fd_set *set);
ENN_CHAR *ENNSock_Inet_ntoa(ENNSock_In_Addr in);
ENN_S32 ENNSock_Inet_aton(ENN_CHAR *cp, ENNSock_In_Addr * pin);
ENN_S32 ENNSoct_inet_addr(ENN_CHAR *cp);
ENN_U32 ENNSock_htonl(ENN_U32 hostlong);
ENN_U16 ENNSock_htons(ENN_U16 hostshort);
ENN_U32 ENNSock_ntohl(ENN_U32 netlong);
ENN_U16 ENNSock_ntohs(ENN_U16 netlong);
ENNSock_Hostent *ENNSock_GetHostbyName(const ENN_CHAR *domainName);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

