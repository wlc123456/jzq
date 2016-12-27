/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：uart.c
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/	


#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <termios.h>
#include <errno.h>   
#include <limits.h> 
#include <asm/ioctls.h>
#include <time.h>
#include <pthread.h>

#include "ennDebug.h"
#include "ennAPI.h"



#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */



#define DATA_LEN                0xFF                                    /* test data's len              */


ENN_S32	gChannel1_Handle = 0;
ENN_S32	gChannel2_Handle = 0;
ENN_S32	gChannel3_Handle = 0;
ENN_S32	gChannel4_Handle = 0;
ENN_S32	gChannel5_Handle = 0;
ENN_S32	gChannel6_Handle = 0;
ENN_S32	gChannel7_Handle = 0;
ENN_S32	gChannel8_Handle = 0;



ENN_S32 UART_Open(ENN_CHAR *cSerialName, ENN_U32 BaudRate, ENN_U8 DataBit, ENN_U8 StopBit, ENN_U8 Prity)
{
    int iFd;
    int ret = 0;
    struct termios opt; 
	//printf("%s, %d\n",__FUNCTION__,__LINE__);

    DEBUG_UART("%s, %d, %s\n",__FUNCTION__,__LINE__, cSerialName);
    iFd = open(cSerialName, O_RDWR | O_NOCTTY | O_NONBLOCK);                        
    if(iFd < 0)
	{
		DEBUG_UART("%s, %d, %d\n",__FUNCTION__,__LINE__,iFd);
        	perror(cSerialName);
        	return -1;
    }
    DEBUG_UART("%s, %d, %d\n",__FUNCTION__,__LINE__,iFd);
	
    ret = tcgetattr(iFd, &opt); 
    if(0 != ret)
    {
	DEBUG_UART("%s, %d, ret = %d\n",__FUNCTION__,__LINE__,ret);
        perror(cSerialName);
        return -1;
    }
    DEBUG_UART("%s, %d, opt.c_cflag = 0x%X\n",__FUNCTION__,__LINE__, opt.c_cflag);

#if 0
	//cfsetispeed(&opt, B57600);
	//cfsetospeed(&opt, B57600);
	cfsetispeed(&opt, B115200);
	cfsetospeed(&opt, B115200);
#endif

	switch(BaudRate)
	{
		case BAUDRATE_1200:
			cfsetispeed(&opt, B1200);
			cfsetospeed(&opt, B1200);
			break;
		case BAUDRATE_2400:
			cfsetispeed(&opt, B2400);
			cfsetospeed(&opt, B2400);
			break;
		case BAUDRATE_4800:
			cfsetispeed(&opt, B4800);
			cfsetospeed(&opt, B4800);
			break;
		case BAUDRATE_9600:
			cfsetispeed(&opt, B9600);
			cfsetospeed(&opt, B9600);
			break;
		case BAUDRATE_19200:
			cfsetispeed(&opt, B19200);
			cfsetospeed(&opt, B19200);
			break;
		case BAUDRATE_38400:
			cfsetispeed(&opt, B38400);
			cfsetospeed(&opt, B38400);
			break;
		case BAUDRATE_57600:
			cfsetispeed(&opt, B57600);
			cfsetospeed(&opt, B57600);
			break;
		case BAUDRATE_115200:
			cfsetispeed(&opt, B115200);
			cfsetospeed(&opt, B115200);
			break;
		default:
			cfsetispeed(&opt, B115200);
			cfsetospeed(&opt, B115200);
			break;
	}

	opt.c_cflag   |=   CLOCAL | CREAD;
	DEBUG_UART("%s, %d, opt.c_cflag = 0x%X\n",__FUNCTION__,__LINE__, opt.c_cflag);
	if(7 == DataBit)
	{
		opt.c_cflag |= CS7;
	}
	else
	{
		opt.c_cflag |= CS8;
	}
	DEBUG_UART("%s, %d, opt.c_cflag = 0x%X, CS8 = 0x%X\n",__FUNCTION__,__LINE__, opt.c_cflag, CS8);
	DEBUG_UART("%s, %d, opt.c_cflag = 0x%X\n",__FUNCTION__,__LINE__, opt.c_cflag);

	if(2 == StopBit)
	{
		opt.c_cflag |= CSTOPB;
	}
	else
	{
		opt.c_cflag &= ~CSTOPB;
	}
	DEBUG_UART("%s, %d, opt.c_cflag = 0x%X\n",__FUNCTION__,__LINE__, opt.c_cflag);

	if(PARITY_ODD == Prity)
	{
		opt.c_cflag |= (PARENB | PARODD);
		DEBUG_UART("%s, %d, opt.c_cflag = 0x%X\n",__FUNCTION__,__LINE__, opt.c_cflag);
	}
	else if(PARITY_EVEN == Prity)
	{
		opt.c_cflag |= PARENB;
		DEBUG_UART("%s, %d, opt.c_cflag = 0x%X\n",__FUNCTION__,__LINE__, opt.c_cflag);
	}
	else
	{
		opt.c_cflag &= (~PARENB);
		DEBUG_UART("%s, %d, opt.c_cflag = 0x%X\n",__FUNCTION__,__LINE__, opt.c_cflag);
	}
	DEBUG_UART("%s, %d, opt.c_cflag = 0x%X\n",__FUNCTION__,__LINE__, opt.c_cflag);
	//opt.c_cflag   &=   ~(CSIZE);
        opt.c_lflag   &=   ~(ECHO   |   ICANON   |   IEXTEN   |   ISIG);
        opt.c_iflag   &=   ~(BRKINT   |   ICRNL   |   INPCK   |   ISTRIP   |   IXON);
        opt.c_oflag   &=   ~(OPOST);
	DEBUG_UART("%s, %d, opt.c_cflag = 0x%X\n",__FUNCTION__,__LINE__, opt.c_cflag);

	opt.c_cc[VMIN]  = 1;
  	opt.c_cc[VTIME] = 0;

	tcflush(iFd, TCIFLUSH);
        if(tcsetattr(iFd, TCSAFLUSH, &opt) < 0) 
	{
		DEBUG_UART("%s, %d\n",__FUNCTION__,__LINE__);
        	return -1;
        }

	tcflush(iFd, TCOFLUSH);

#if 0   
    /*raw mode*/
    opt.c_lflag   &=   ~(ECHO   |   ICANON   |   IEXTEN   |   ISIG);
    opt.c_iflag   &=   ~(BRKINT   |   ICRNL   |   INPCK   |   ISTRIP   |   IXON);
    opt.c_oflag   &=   ~(OPOST);
    opt.c_cflag   &=   ~(CSIZE   |   PARENB);
    opt.c_cflag   |=   CS8;

    /*'DATA_LEN' bytes can be read by serial*/
    opt.c_cc[VMIN]   =   DATA_LEN;                                      
    opt.c_cc[VTIME]  =   150;

    if(tcsetattr(iFd, TCSANOW, &opt)<0) 
	{
        return -1;
    }
#endif
	DEBUG_UART("%s, %d, %d\n",__FUNCTION__,__LINE__,iFd);

    return (ENN_S32)iFd;
}


/********************************************************************
* 函 数 名：		ENNSock_Init
* 函数介绍:		SOCKET初始化
* 输入参数:		无
* 输出参数:		无
* 返 回 值:		成功返回ENNO_SUCCESS
*					失败返回ENNO_FAILURE
* 修          改:
********************************************************************/
ENN_ErrorCode_t UART_Init(CHANNEL_t CHANNELX, ENN_U32 BaudRate, ENN_U8 DataBit, ENN_U8 StopBit, ENN_U8 Prity)
{
	switch(CHANNELX) 
	{
		case CHANNEL1:
			gChannel1_Handle = UART_Open("/dev/ttymxc1", BaudRate, DataBit, StopBit, Prity);
			if(gChannel1_Handle < 0)
			{
				return ENN_FAIL;
			}
			printf("%s, %d, Channel =%d\n",__FUNCTION__,__LINE__,CHANNEL1);
			break;
		case CHANNEL2:
			gChannel2_Handle = UART_Open("/dev/ttymxc2", BaudRate, DataBit, StopBit, Prity);
			if(gChannel2_Handle < 0)
			{
				return ENN_FAIL;
			}
			printf("%s, %d, Channel =%d\n",__FUNCTION__,__LINE__,CHANNEL2);
			break;
		case CHANNEL3:
			gChannel3_Handle = UART_Open("/dev/ttymxc3", BaudRate, DataBit, StopBit, Prity);
			if(gChannel3_Handle < 0)
			{
				return ENN_FAIL;
			}
			printf("%s, %d, Channel =%d\n",__FUNCTION__,__LINE__,CHANNEL3);
			break;
		case CHANNEL4:
			gChannel4_Handle = UART_Open("/dev/ttymxc4", BaudRate, DataBit, StopBit, Prity);
			if(gChannel4_Handle < 0)
			{
				return ENN_FAIL;
			}
			printf("%s, %d, Channel =%d\n",__FUNCTION__,__LINE__,CHANNEL4);
			break;
		case CHANNEL5:
			gChannel5_Handle = UART_Open("/dev/ttyS0", BaudRate, DataBit, StopBit, Prity);
			if(gChannel5_Handle < 0)
			{
				return ENN_FAIL;
			}
			printf("%s, %d, Channel =%d\n",__FUNCTION__,__LINE__,CHANNEL5);
			break;
		case CHANNEL6:
			gChannel6_Handle = UART_Open("/dev/ttyS1", BaudRate, DataBit, StopBit, Prity);
			if(gChannel6_Handle < 0)
			{
				return ENN_FAIL;
			}
			//printf("%s, %d, Channel =%d\n",__FUNCTION__,__LINE__,CHANNEL6);			
			printf("%s, %d, Channel =%d, gChannel6_Handle =%d\n",__FUNCTION__,__LINE__,CHANNEL6,gChannel6_Handle);
			break;
		case CHANNEL7:
			gChannel7_Handle = UART_Open("/dev/ttyS2", BaudRate, DataBit, StopBit, Prity);
			if(gChannel7_Handle < 0)
			{
				return ENN_FAIL;
			}
			//printf("%s, %d, Channel =%d\n",__FUNCTION__,__LINE__,CHANNEL7);		
			printf("%s, %d, Channel =%d, gChannel7_Handle =%d\n",__FUNCTION__,__LINE__,CHANNEL7,gChannel7_Handle);
			break;
		case CHANNEL8:
			gChannel8_Handle = UART_Open("/dev/ttyS3", BaudRate, DataBit, StopBit, Prity);
			if(gChannel8_Handle < 0)
			{
				return ENN_FAIL;
			}
			printf("%s, %d, Channel =%d, gChannel8_Handle =%d\n",__FUNCTION__,__LINE__,CHANNEL8,gChannel8_Handle);
			break;
		default:
			return ENN_FAIL;
	}
	
    return ENN_SUCCESS;
}


ENN_S32 UART_Write(CHANNEL_t CHANNELX, ENN_U8 *pBuf, ENN_U16 Len)
{
	int ret = 0;
	
	switch(CHANNELX) 
	{
		case CHANNEL1:
			ret = write((int)gChannel1_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL2:
			ret = write((int)gChannel2_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL3:
			ret = write((int)gChannel3_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL4:
			ret = write((int)gChannel4_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL5:
			ret = write((int)gChannel5_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL6:
			ret = write((int)gChannel6_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL7:
			ret = write((int)gChannel7_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL8:
			ret = write((int)gChannel8_Handle, pBuf, (size_t)Len);
			break;
		default:
			break;
	}
	
    return ret;
}



ENN_S32 UART_Read(CHANNEL_t CHANNELX, ENN_U8 *pBuf, ENN_U32 Len)
{
	int s32ReadLen = 0;
	ENNAPI_ASSERT(NULL != pBuf);
	
	switch(CHANNELX) 
	{
		case CHANNEL1:
			s32ReadLen = read((int)gChannel1_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL2:
			s32ReadLen = read((int)gChannel2_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL3:
			s32ReadLen = read((int)gChannel3_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL4:
			s32ReadLen = read((int)gChannel4_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL5:
			s32ReadLen = read((int)gChannel5_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL6:
			s32ReadLen = read((int)gChannel6_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL7:
			s32ReadLen = read((int)gChannel7_Handle, pBuf, (size_t)Len);
			break;
		case CHANNEL8:
			s32ReadLen = read((int)gChannel8_Handle, pBuf, (size_t)Len);
			break;
		default:
			break;
	}
    return s32ReadLen;
}

ENN_S32 UART_Get_Des(CHANNEL_t CHANNELX)
{
	ENN_S32 s32ChannelHandle = 0;
	
	switch(CHANNELX) 
	{
		case CHANNEL1:
			s32ChannelHandle = gChannel1_Handle;
			break;
		case CHANNEL2:
			s32ChannelHandle = gChannel2_Handle;
			break;
		case CHANNEL3:
			s32ChannelHandle = gChannel3_Handle;
			break;
		case CHANNEL4:
			s32ChannelHandle = gChannel4_Handle;
			break;
		case CHANNEL5:
			s32ChannelHandle = gChannel5_Handle;
			break;
		case CHANNEL6:
			s32ChannelHandle = gChannel6_Handle;
			break;
		case CHANNEL7:
			s32ChannelHandle = gChannel7_Handle;
			break;
		case CHANNEL8:
			s32ChannelHandle = gChannel8_Handle;
			break;
		default:
			s32ChannelHandle = -1;
			break;
	}

	return s32ChannelHandle;
}


#ifdef __cplusplus
#if __cplusplus
    }
#endif /* __cpluscplus */
#endif /* __cpluscplus */


