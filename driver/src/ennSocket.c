/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：ennSocket.c
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/	


#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/tcp.h>
#include <unistd.h>
#include <netdb.h>
#include <linux/if_arp.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "ennAPI.h"
#include "ennDebug.h"

//head to c
#include <linux/if_arp.h>


#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */


#define ENNAPI_ETH_DEV_NAME "eth0"


/********************************************************************
* 函 数 名：		ENNSock_Init
* 函数介绍:		SOCKET初始化
* 输入参数:		无
* 输出参数:		无
* 返 回 值:		成功返回ENNO_SUCCESS
*					失败返回ENNO_FAILURE
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNSock_Init(ENN_VOID)
{
    return ENN_SUCCESS;
}


/********************************************************************
* 函 数 名：		ENNSock_Socket
* 函数介绍:		创建一个socket接口
* 输入参数:		domain:  协议域(AF_INET、AF_INET6)
*                          	type:	指定socket类型(SOCK_STREAM、SOCK_DGRAM、SOCK_RAW、SOCK_PACKET、SOCK_SEQPACKET)
*					protocol: 指定协议(IPPROTO_TCP、IPPTOTO_UDP、IPPROTO_SCTP、IPPROTO_TIPC)
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_Socket(ENN_S32 domain, ENN_S32 type, ENN_S32 protocol)
{
   	 int 	sock_domain = AF_INET;
    	 int 	sock_type = SOCK_STREAM;
   	 int 	sock_protocol = IPPROTO_TCP;
   	 ENN_S32 err = 0;

	enn_api_trace(ENNAPI_TRACE_DEBUG, "domain = %x, type = %d, protocol = %d\n", domain, type, protocol);
   
	sock_domain = (int)domain;
	sock_type = (int)type;
	sock_protocol = (int)protocol;

	//printf("FUNCTION:%s, the sock_domain is %d, the sock_type is %d,the sock_protocol is %d\n", __FUNCTION__,sock_domain,sock_type, sock_protocol);
	err = (ENN_S32)socket(sock_domain, sock_type, sock_protocol);
	enn_api_trace(ENNAPI_TRACE_DEBUG, "socket return err = %d\n", err);

	return err;
}


/********************************************************************
* 函 数 名：		ENNSock_Bind
* 函数介绍:		调用该接口将一个本地地址（包含有协议
*					族，端口和IP）指定到相应的socket上，socket
*					建立任何连接之前需要调用本接口实现协
*					议族、端口和IP的指定，参数需求同POSIX bind。
* 输入参数:		sockfd:  		socket描述符
*                          	my_addr:		指向一个cySock_SocketAddr的数据结构，此数据结构包含了要被绑定的socket的地址。这个地址的长度和初始化依赖于这个socket的地址族
*					addrlen: 		my_addr的长度
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_Bind(ENN_S32 sockfd,  const ENNSock_SocketAddr *my_addr, ENN_S32 addrlen)
{
	ENN_S32 err = 0;
    ENNAPI_ASSERT(NULL != my_addr);

    err = (ENN_S32)bind((int)sockfd, (struct sockaddr *)my_addr, (socklen_t)addrlen);

	return err;
}



/********************************************************************
* 函 数 名：		ENNSock_Connect
* 函数介绍:		连接指定IP的服务器，参数需求同POSIX connect。
* 输入参数:		sockfd:  		socket描述符
*                          	addr:		指向一个cySock_SocketAddr的数据结构，此数据结构包含了要被绑定的socket的地址。这个地址的长度和初始化依赖于这个socket的地址族
*					addrlen: 		addr的长度
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_Connect(ENN_S32 sockfd, const ENNSock_SocketAddr *addr, ENN_S32 addrlen)
{
	ENN_S32 err = 0;
    ENNAPI_ASSERT(NULL != addr);
	
    err = (ENN_S32)connect(sockfd, (struct sockaddr *)addr, (socklen_t)addrlen);

	return err;
}


/********************************************************************
* 函 数 名：		ENNSock_Listen
* 函数介绍:		通过socket描述符指定一个socket处于监
*					听状态，处于监听状态的socket将维护
*					一个连接请求队列，连接的最大数
*					量由参数backlog决定，参数需求同POSIX listen。
* 输入参数:		sockfd:  		socket描述符
*                          	backlog:		请求队列最大数量
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_Listen(ENN_S32 sockfd, ENN_S32 backlog)
{
	ENN_S32 err = 0;
	
    err = (ENN_S32)listen((int)sockfd, (int)backlog);

	return err;
}


/********************************************************************
* 函 数 名：		ENNSock_Accept
* 函数介绍:		处于监听状态的socket从连接请求队列
*					中取出排在最前的一个客户请求，
*					并且创建一个新的socket来与客户socket创
*					建连接通道，如果连接成功，就返回
*					新创建的socket的描述符。该接口只用
*					于基于链接的socket应用，参数需求同POSIX accept。
* 输入参数:		sockfd:  		socket描述符
*                          	addr:		用于存放客户端socket请求的socket描述地址
*					addrlen:		客户端socket地址长度
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_Accept(ENN_S32 sockfd, ENNSock_SocketAddr *addr, ENN_S32 *addrlen)
{
	ENN_S32 err = 0;
    ENNAPI_ASSERT(NULL != addr);
	
    err = (ENN_S32)accept((int)sockfd, (struct sockaddr *)addr, (socklen_t *)addrlen);

	return err;
}


/********************************************************************
* 函 数 名：		ENNSock_SocketSend
* 函数介绍:		向指定的已链接的socket发送数据，参数需求同POSIX send。
* 输入参数:		sockfd:  		socket描述符
*                          	msg:		待传输的消息地址
*					len:			消息长度，以字节为单位
*					flags:		消息传输类型标识
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_SocketSend(ENN_S32 sockfd, 
								  const ENN_VOID *msg, 
								  ENN_S32 len, 
								  ENN_S32 flags)
{
    ENN_S32 value = 0;
	ENN_S32 temp = 0;
    ENN_S32 err = 0;
    ENN_U32 i = 0;

	enn_api_trace(ENNAPI_TRACE_DEBUG, "fd = %x, flags = %x\n", sockfd, flags);

    ENNAPI_ASSERT(NULL != msg);

    value = 0;
    for(i=0; i<16; i++)
    {
        temp = flags & (0x1<<i);
        value |= temp;
    }
    flags = value;

	enn_api_trace(ENNAPI_TRACE_DEBUG, "fd = %x, flags = %x\n", sockfd, flags);
    err = (ENN_S32)send((int)sockfd, (void *)msg, (size_t)len, (int)flags);
	if(err <= 0) 
	{
		perror("cySock_SocketSend ");
		enn_api_trace(ENNAPI_TRACE_ERROR, "fd = %d, return err = %x\n", sockfd, err);
	}
	enn_api_trace(ENNAPI_TRACE_ERROR, "fd = %d, return err = %x\n", sockfd, err);

	return err;
}


/********************************************************************
* 函 数 名：		ENNSock_SocketSendTo
* 函数介绍:		向一个连接或非连接状态的socket发送
*					数据。如果socket是非连接模式的，那
*					么数据将通过指定的目的（to）socket来
*					发送，如果socket是连接模式的，那么将
*					忽略to。参数需求同POSIX sendto。
* 输入参数:		sockfd:  		socket描述符
*                          	msg:		待传输的消息地址
*					len:			消息长度，以字节为单位
*					flags:		消息传输类型标识
*					to:			消息发送的目的socket地址
*					tplen:		目的socket地址长度
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_SocketSendTo(ENN_S32 sockfd, 
									 const ENN_VOID *msg, 
									 ENN_S32 len,
									 ENN_S32 flags,
									 const ENNSock_SocketAddr *to, 
									 ENN_S32 tplen)
{
    ENN_S32 value = 0;
	ENN_S32 temp = 0;
    ENN_S32 err = 0;
    ENN_U32 i = 0;

    ENNAPI_ASSERT(NULL != msg);
    if(to == NULL)
    {
        return ENN_FAIL;
    }

    value = 0;
    for( i=0; i<16; i++)
    {
        temp = flags & (0x1<<i);
        value |= temp;
    }
    flags = value;

    err = (ENN_S32)sendto((int)sockfd, (void *)msg, (size_t)len, (int)flags, (struct sockaddr *)to, (socklen_t )tplen);
	if(err <= 0)
	{
		perror("ENNSock_SocketSendTo ");
		enn_api_trace(ENNAPI_TRACE_ERROR, "fd = %d, return err = %x\n", sockfd, err);
	}

	return err;
}


/********************************************************************
* 函 数 名：		ENNSock_SocketRecv
* 函数介绍:		从指定的已链接的socket接收数据，参数需求同POSIX recv。
* 输入参数:		sockfd:  		socket描述符
*                          	buf:			存放接收数据的缓冲区
*					len:			接收数据长度
*					flags:		消息传输类型标识
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_SocketRecv(ENN_S32 sockfd, ENN_VOID *buf, ENN_S32 len, ENN_S32 flags)
{
    ENN_S32 value, temp;
    ENN_U32 i;
    ENN_S32 err;

	//enn_api_trace(ENNAPI_TRACE_DEBUG, "fd = %x, len = %d, flags = %x\n", sockfd, len, flags);


    ENNAPI_ASSERT(NULL != buf);
#if 0
    value = 0;
    for( i=0; i<16; i++)
    {
        temp = flags & (0x1<<i);
        #ifdef SOCKET_MAP
        value |= cySock_Map(CYSOCK_MAP_MSG,temp);    
        #else
        value |= temp;
        #endif
    }
    flags = value;
#endif

        err = (ENN_S32)recv((int)sockfd, (void *)buf, (size_t)len, (int)flags);
        if(err <= 0) 
	{
		perror("ENNSock_SocketRecv ");
		enn_api_trace(ENNAPI_TRACE_ERROR, "ERROR: fd = %d, return err = %x\n", sockfd, err);
	}

	return err;
}


/********************************************************************
* 函 数 名：		ENNSock_SocketRecvFrom
* 函数介绍:		从一个链接或者非链接状态的socket接收数据。参数需求同POSIX recvfrom。
* 输入参数:		sockfd:  		socket描述符
*                          	buf:			存放接收数据的缓冲区
*					len:			接收数据长度
*					flags:		消息传输类型标识
*					from:		源数据socket地址
*					fromlen:		源数据socket地址长度
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_SocketRecvFrom(ENN_S32 sockfd,
										ENN_VOID *buf,
										ENN_S32 len,
                              			ENN_S32 flags,
                              			ENNSock_SocketAddr *from, 
                              			ENN_S32 *fromlen)
{  
    ENN_S32 value = 0;
	ENN_S32 temp = 0;
    ENN_S32 err = 0;
    ENN_U32 i = 0;

    ENNAPI_ASSERT(NULL != buf);

    value = 0;
    for( i=0; i<16; i++)
    {
        temp = flags & (0x1<<i);
        value |= temp;
    }
    flags = value;

    err = (ENN_S32)recvfrom((int)sockfd, (void *)buf, (size_t)len, (int)flags,(struct sockaddr *)from, (socklen_t *)fromlen);
	if(err <= 0) 
	{
		perror("ENNSock_SocketRecvFrom");
		enn_api_trace(ENNAPI_TRACE_ERROR, "ERROR: fd = %d, err = %x\n", sockfd, err);
	}

	return err;
}


/********************************************************************
* 函 数 名：		ENNSock_GetSockOpt
* 函数介绍:		获取指定socket的option参数，参数需求同POSIX getsockopt。
* 输入参数:		sockfd:  		socket描述符
*                          	level:		指定option相关协议
*					optname:		指定相关协议下的option选项
*					optval:		用于存放获得的选项内容
*					optlen:		成功时返回optval中内容的长度，输入时可以时optval的buffer长度
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_GetSockOpt(ENN_S32 sockfd, 
								ENN_S32 level,
								ENN_S32 optname,
								ENN_VOID *optval,
								ENN_S32  *optlen)
{
    ENN_S32 tmpLevel = 0;
	ENN_S32 tmpOptname = 0;
	ENN_S32 err = 0;
    
    ENNAPI_ASSERT(NULL != optval);
    ENNAPI_ASSERT(NULL != optlen);
    
    tmpLevel = level;    
    tmpOptname = optname;

    err = (ENN_S32)getsockopt((int)sockfd, (int)tmpLevel, (int)tmpOptname, (void *)optval, (socklen_t *)optlen);

	return err;
}


/********************************************************************
* 函 数 名：		ENNSock_SetSockOpt
* 函数介绍:		设置指定socket的option参数，参数需求同POSIX setsockopt。
* 输入参数:		sockfd:  		socket描述符
*                          	level:		指定option相关协议
*					optname:		指定相关协议下的option选项
*					optval:		需要设置的选项内容
*					optlen:		选项内容的长度
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_SetSockOpt(ENN_S32 sockfd, 
								ENN_S32 level, 
								ENN_S32 optname,
                                const ENN_VOID *optval,
                                ENN_S32  optlen)
{
    ENN_S32 tmpLevel = 0; 
    ENN_S32 tmpOptname = 0;
    int flags = 0; 
    
    tmpLevel = level;    
    tmpOptname = optname;    

    if(SO_NONBLOCK == optname)
    {
		/* Set socket to non-blocking */ 
		if((flags = fcntl(sockfd, F_GETFL, 0)) < 0) 
		{ 
			return -1;              
		} 
		else
		{
			return 0;
		}

		if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) 
		{ 
			return -1;               
		} 
		else
		{
			return 0;
		}
    }
    else
    {
		return setsockopt(sockfd, tmpLevel, tmpOptname, optval, optlen);
    }
}


/********************************************************************
* 函 数 名：		ENNSock_SocketClose
* 函数介绍:		调用该接口关闭socket，以socket描述符作为ID。
* 输入参数:		sockfd:  		socket描述符
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_SocketClose(ENN_S32 sockfd)
{
    return close((int)sockfd);
}


/********************************************************************
* 函 数 名：		ENNSock_Select
* 函数介绍:		调用该接口是用来程序监视多个文件
*					句柄(file descriptor)的状态是否发生指定的
*					变化。程序会停在该接口内部等待，
*					直到被监视的文件句柄有某一个或多
*					个发生了状态改变后返回。参数需求
*					同POSIX select。
* 输入参数:		n:  		socket描述符
*                          	readfds:		被监视的可用于读的文件描述符集
*					writefds:		被监视的可用于写的文件描述符集
*					exceptfds:	被监视的可能出现异常的文件描述符集
*					timeout:		超时时限，定义同struct timeval
* 输出参数:		无
* 返 回 值:		成功返回fd_set中的描述符个数，超时时间到时返回0，否则返回-1。
* 修          改:
********************************************************************/
ENN_S32 ENNSock_Select(ENN_S32 n, 
						 ENNSock_fd_set *readfds, 
						 ENNSock_fd_set *writefds, 
						 ENNSock_fd_set *exceptfds,
       					 ENNSock_Timeval *timeout)
{
    ENN_S32 err;
#if 0
#if 0
    CYAPI_ASSERT(NULL != readfds);
    CYAPI_ASSERT(NULL != writefds);
    CYAPI_ASSERT(NULL != exceptfds);
#else
    ENNSock_fd_set TmpFds;
    memset((void *)&TmpFds, 0, sizeof(ENNSock_fd_set));
    if(NULL == readfds) readfds = &TmpFds;
    if(NULL == writefds) writefds = &TmpFds;
    if(NULL == exceptfds) exceptfds = &TmpFds;
#endif

    enn_api_trace(ENNAPI_TRACE_DEBUG, "n = %d\n", n);
    err = select(n, (fd_set *)readfds->fd_array, (fd_set *)writefds->fd_array, (fd_set *)exceptfds->fd_array, (struct timeval *)timeout);
#endif
        err = select(n, (fd_set *)readfds, (fd_set *)writefds, (fd_set *)exceptfds, (struct timeval *)timeout);
	if(err < 0)
	{
		perror("ENNSock_Select ");
		enn_api_trace(ENNAPI_TRACE_ERROR, "n = %d, return err = %d, timeout = %ld - %ld\n", n, err, timeout->tv_sec, timeout->tv_usec);
	}

	return err;
}



/********************************************************************
* 函 数 名：		ENNSock_Ioctl
* 函数介绍:		调用该接口获得socket相关的选项。
* 输入参数:		d:  		socket文件描述符
*                          	request:		指定一个单一选项被重新获得    ？？
*            						... － 如果不是空指针，指向存储数据缓冲区
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_Ioctl(ENN_S32 d, ENN_S32 request, ...)
{
    void *arg;
    va_list list;

    va_start(list, request);
    arg = va_arg(list, void *);

    va_end(list);

    return ioctl(d,request,arg); 

}


/********************************************************************
* 函 数 名：		ENNSock_Shutdown
* 函数介绍:		调用该接口关闭与一个socket链接的部
*					分或者全部通讯。参数需求同POSIX shutdown。
* 输入参数:		s:  		待关闭的socket描述符
*                          	how:	指定的关闭类型
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32  ENNSock_Shutdown(ENN_S32 s, ENN_S32 how)
{
    return shutdown(s,how);
}


/********************************************************************
* 函 数 名：		ENNSock_FD_ZERO
* 函数介绍:		初始化一个文件描述符集。参数需求同POSIX FD_ZERO。
* 输入参数:		set:  		指向待清除的文件描述符集指针
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_VOID ENNSock_FD_ZERO(ENNSock_fd_set *set)
{
    ENNAPI_ASSERT(NULL != set);
    FD_ZERO((fd_set *)set->fd_array);
}


/********************************************************************
* 函 数 名：		ENNSock_FD_SET
* 函数介绍:		添加一个文件句柄到文件描述符集。参数需求同POSIX FD_SET。
* 输入参数:		fd:		socket描述符
*					set:  	fd需要加入的文件描述符集
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_VOID ENNSock_FD_SET(ENN_S32 fd, ENNSock_fd_set *set)
{
    ENNAPI_ASSERT(NULL != set);
    FD_SET(fd, (fd_set *)set->fd_array);
}


/********************************************************************
* 函 数 名：		ENNSock_FD_SET
* 函数介绍:		检查指定文件句柄是否发生了变化。参数需求同POSIX FD_ISSET。
* 输入参数:		fd:		socket描述符
*					set:  	确认fd是否是该描述符集的一个描述符
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_S32 ENNSock_FD_ISSET(ENN_S32 fd, ENNSock_fd_set *set)
{
    ENNAPI_ASSERT(NULL != set);
    return FD_ISSET(fd, (fd_set *)set->fd_array);
}


/********************************************************************
* 函 数 名：		ENNSock_Inet_ntoa
* 函数介绍:		把指定的Internet主机地址转换成Internet标准字符的字符串。
* 输入参数:		in:		待转换的主机地址
* 输出参数:		无
* 返 回 值:		成功返回转换后的字符串指针（字符串存储在静态内存区），否则返回NULL。
* 修          改:
********************************************************************/
ENN_CHAR *ENNSock_Inet_ntoa(ENNSock_In_Addr in)
{
    struct in_addr addr;
    addr.s_addr = (in_addr_t)in.s_addr;
    return inet_ntoa(addr);
}


/********************************************************************
* 函 数 名：		ENNSock_Inet_aton
* 函数介绍:		把字符串转化为Internet主机地址。
* 输入参数:		cp:		XX.XX.XX.XX形式的网络地址字符串
*					pin:  	用于存放转换后的主机地址
* 输出参数:		无
* 返 回 值:		成功返回转换后的主机地址，否则返回－1。
* 修          改:
********************************************************************/
ENN_S32 ENNSock_Inet_aton(ENN_CHAR *cp, ENNSock_In_Addr * pin)
{
    ENNAPI_ASSERT(NULL != cp);
    ENNAPI_ASSERT(NULL != pin);
    
    return inet_aton(cp,(struct in_addr *)pin);
}


/********************************************************************
* 函 数 名：		ENNSoct_inet_addr
* 函数介绍:		把字符串转化为Internet主机地址，功能同cySock_Inet_aton。
* 输入参数:		cp:		XX.XX.XX.XX形式的网络地址字符串
* 输出参数:		无
* 返 回 值:		成功返回主机地址，否则返回0xFFFFFFFF。
* 修          改:
********************************************************************/
ENN_S32 ENNSoct_inet_addr(ENN_CHAR *cp)
{
    ENNAPI_ASSERT(NULL != cp);
    return inet_addr(cp);
}


/********************************************************************
* 函 数 名：		ENNSock_htonl
* 函数介绍:		把整型数从主机字节序转换成网络字节序。
* 输入参数:		hostlong:		主机字节序的U32数据
* 输出参数:		无
* 返 回 值:		返回网络字节序对应的数据。
* 修          改:
********************************************************************/
ENN_U32 ENNSock_htonl(ENN_U32 hostlong)
{
    return htonl(hostlong);
}


/********************************************************************
* 函 数 名：		ENNSock_htons
* 函数介绍:		把短整型数从主机字节序转换成网络字节序。
* 输入参数:		hostshort:		主机字节序的U16数据
* 输出参数:		无
* 返 回 值:		返回网络字节序对应的数据。
* 修          改:
********************************************************************/
ENN_U16 ENNSock_htons(ENN_U16 hostshort)
{
    return htons(hostshort);
}


/********************************************************************
* 函 数 名：		ENNSock_ntohl
* 函数介绍:		把整型数从网络字节序转换成主机字节序。
* 输入参数:		netlong:		网络字节序的U32整型数据
* 输出参数:		无
* 返 回 值:		返回主机字节序的U32数据。
* 修          改:
********************************************************************/
ENN_U32 ENNSock_ntohl(ENN_U32 netlong)
{
    return ntohl(netlong);
}


/********************************************************************
* 函 数 名：		ENNSock_ntohs
* 函数介绍:		把短整型数从网络字节序转换成主机字节序。
* 输入参数:		netlong:		网络字节序的U16整型数据
* 输出参数:		无
* 返 回 值:		返回主机字节序的U16数据。
* 修          改:
********************************************************************/
ENN_U16 ENNSock_ntohs(ENN_U16 netlong)
{
    return ntohs(netlong);
}


/********************************************************************
* 函 数 名：		ENNSock_GetHostbyName
* 函数介绍:		通过名称获取主机地址。参数需求同POSIX gethostbyname。
* 输入参数:		domainName:		主机域名称（字符或者XX.XX.XX.XX格式）
* 输出参数:		无
* 返 回 值:		成功返回cySock_GetHostbyName结构的指针，否则返回NULL。
* 修          改:
********************************************************************/
ENNSock_Hostent *ENNSock_GetHostbyName(const ENN_CHAR *domainName)
{
	ENNSock_Hostent *pHostent = NULL;

    ENNAPI_ASSERT(NULL != domainName);
	pHostent = (ENNSock_Hostent *)gethostbyname(domainName);
	if(!pHostent)
	{
		perror("ENNSock_GetHostbyName");
		enn_api_trace(ENNAPI_TRACE_ERROR, "FAILED: err = %x, domainName = %s\n", errno, domainName);
	}
	return pHostent;
}

int is_a_ip(char *ipaddr)
{
  char *pnum,*pdot=ipaddr;
  int i=0;

  for(;*ipaddr;ipaddr=pdot++,i++)
  {
    int t=0,e=1;
    if(*(pnum=pdot)=='.') return 1;

    while(*pdot!='.'&&*pdot)
    {
        ++pdot;
    }

    ipaddr = pdot-1;
    while(ipaddr>=pnum)
    {
        t+=e*(*ipaddr---'0');
        e*=10;
    }
    //if(t<0||t>255||(pdot-pnum==3&&t<100)||(pdot-pnum==2&&t<10))
    if(t<0||t>255)
        return 1;
  }

  if (i !=4 ) return 1;

  return 0;
}

static char* skip_space(char* line)
{
    char* p = line;
    while (*p == ' ' || *p == '\t')
    {
        p++;
    }
    return p;
}

static char* get_word(char* line, char* value)
{
    char* p = line;

    p = skip_space(p);
    while (*p != '\t' && *p != ' ' && *p != '\n' && *p != 0)
    {
        *value++ = *p++; 
    }
    *value = 0;
    return p;
}

ENN_ErrorCode_t ENNSock_EthernetIPAddressGet(ENN_CHAR *ipAdd)
{
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in *s_in;

    ENNAPI_ASSERT(NULL != ipAdd);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        enn_api_trace(ENNAPI_TRACE_ERROR, "socket create fail!\n");
        return ENN_FAIL;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ENNAPI_ETH_DEV_NAME);
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)
    {
        enn_api_trace(ENNAPI_TRACE_ERROR, "ioctl fail!\n");
        return ENN_FAIL;
    }
    
    s_in = (struct sockaddr_in *)(&ifr.ifr_addr);
    memcpy((void *)ipAdd, inet_ntoa(s_in->sin_addr), 15);

	enn_api_trace(ENNAPI_TRACE_ERROR, "ip = %s\n", ipAdd);
    close(sockfd);
    return ENN_SUCCESS;
}

ENN_ErrorCode_t ENNSock_EthernetIPAddressSet(ENN_CHAR* ipAdd)
{
    int sockfd;
    struct ifreq    ifr;
    struct sockaddr_in *sin;

    ENNAPI_ASSERT(NULL != ipAdd);

    if(is_a_ip(ipAdd) != 0)    
    {
		enn_api_trace(ENNAPI_TRACE_ERROR, "set eth ip failed!\n");
		return ENN_FAIL;
    }
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        enn_api_trace(ENNAPI_TRACE_ERROR, "socket create fail!\n");
        return ENN_FAIL;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ENNAPI_ETH_DEV_NAME, sizeof(ifr.ifr_name)-1);
    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    sin->sin_family = AF_INET;

    if (inet_pton(AF_INET, ipAdd, &sin->sin_addr) <= 0)  // fix: change from < to <= for invalid ipAddr input
    {
        enn_api_trace(ENNAPI_TRACE_ERROR, "inet_pton  fail!\n");
        return ENN_FAIL;
    }

    if(ioctl(sockfd, SIOCSIFADDR, &ifr) < 0)
    {
        enn_api_trace(ENNAPI_TRACE_ERROR, "set ipaddr fail!\n");
        return ENN_FAIL;
    }

    close(sockfd);
    return ENN_SUCCESS;    
}


ENN_ErrorCode_t ENNSock_EthernetSubNetmaskGet(ENN_CHAR* subNetmask)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *s_in;

    ENNAPI_ASSERT(NULL != subNetmask);

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        enn_api_trace(ENNAPI_TRACE_ERROR, "socket create fail!\n");
        return ENN_FAIL;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ENNAPI_ETH_DEV_NAME);
    if (ioctl(skfd, SIOCGIFNETMASK, &ifr) < 0)
    {
        enn_api_trace(ENNAPI_TRACE_ERROR, "ioctl fail!\n");
        return ENN_FAIL;
    }

    s_in = (struct sockaddr_in *)(&ifr.ifr_netmask);
    
    memcpy((void *)subNetmask, inet_ntoa(s_in->sin_addr), 15);

    close(skfd);
    return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNSock_EthernetSubNetmaskSet(ENN_CHAR* subNetmask)
{
    int sockfd;
    struct ifreq    ifr;
    struct sockaddr_in *sin;

    ENNAPI_ASSERT(NULL != subNetmask);

    if( is_a_ip(subNetmask) != 0)   
	{
		enn_api_trace(ENNAPI_TRACE_ERROR, "set netmask failed!\n");
		return ENN_FAIL;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        enn_api_trace(ENNAPI_TRACE_ERROR, "socket create fail!\n");
        return ENN_FAIL;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ENNAPI_ETH_DEV_NAME, sizeof(ifr.ifr_name)-1);
    sin = (struct sockaddr_in *)&ifr.ifr_netmask;
    sin->sin_family = AF_INET;

    if (inet_pton(AF_INET, subNetmask, &sin->sin_addr) < 0)
    {
        enn_api_trace(ENNAPI_TRACE_ERROR, "inet_pton  fail!\n");
        return ENN_FAIL;
    }

    if(ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0)
    {
        enn_api_trace(ENNAPI_TRACE_ERROR, "set netmask fail!\n");
        return ENN_FAIL;
    }
    
    close(sockfd);
    return ENN_SUCCESS;
}


ENN_ErrorCode_t cySock_EthernetGatewayGet(ENN_CHAR* gateway)
{
    char line[512];
    char str[16];
    FILE* fp;

    if(gateway == NULL)
    {
    	enn_api_trace(ENNAPI_TRACE_ERROR, "gateway is NULL!\n");
        return ENN_FAIL;
    }

    fp = fopen("/proc/net/route", "r");
    if (fp)
    {
        memset(str, 0, sizeof(str));
        while (fgets(line, sizeof(line), fp) != 0)
        {
            char* p = line;
            p = strstr(p, ENNAPI_ETH_DEV_NAME);
            if (p)
            {
                p = get_word(p, str);
                p = get_word(p, str);
                if (strcmp(str, "00000000") == 0)
                {
                    p = get_word(p, str);
                    if (strcmp(str, "00000000") != 0)
                    {
                        int a, b, c, d;			
                        sscanf(str, "%02X%02X%02X%02X", &a, &b, &c, &d);
                        sprintf(gateway, "%d.%d.%d.%d", d, c, b, a);
//			hi_cyapi_trace(CYAPI_TRACE_ERROR, "gw: %02x %02x %02x %02x\n", (PL_U8)d, (PL_U8)c, (PL_U8)b, (PL_U8)a);
                        fclose(fp);
                        return ENN_SUCCESS;
                    }
                }
            }
        }
        fclose(fp);        
    }

    strcpy(gateway, "0.0.0.0");    
    return ENN_FAIL;
}


ENN_ErrorCode_t ENNSock_EthernetGatewaySet(ENN_CHAR *gateway)
{
    char str[512];
    int rt=0;

    if(gateway == NULL)
    {
        return ENN_FAIL;
    }

//	hi_cyapi_trace(CYAPI_TRACE_ERROR, "gateway = %s\n", gateway);
	
    if( is_a_ip(gateway) != 0)  
	{
		enn_api_trace(ENNAPI_TRACE_ERROR, "set gateway failed!\n");
		return ENN_FAIL;
    }

    rt=sprintf(str, "route add default gw %s", gateway);
    if( rt < 0)
        return ENN_FAIL;

    rt = system(str);
    if( rt < 0) 
	{
		perror("ENNSock_EthernetGatewaySet: ");
        return ENN_FAIL;
    }
    else 
    {
        return ENN_SUCCESS;
    }
}


#ifdef __cplusplus
#if __cplusplus
    }
#endif /* __cpluscplus */
#endif /* __cpluscplus */


