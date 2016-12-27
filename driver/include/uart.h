/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：uart.h
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/
#ifndef __UART_H__
#define __UART_H__


#ifdef __cplusplus
extern "C"
{
#endif

#include "ennType.h"


#define PARITY_NONE 0
#define PARITY_ODD 1
#define PARITY_EVEN 2
#define PARITY_MARK 3
#define PARITY_SPACE 4

typedef enum tag_CHANNEL_t	
{
	CHANNEL1 = 0,
	CHANNEL2,
	CHANNEL3,
	CHANNEL4,
	CHANNEL5,
	CHANNEL6,
	CHANNEL7,
	CHANNEL8,
}CHANNEL_t;

typedef enum tag_BAUDRATE_t	
{
	USART_BAUD_9600,
	USART_BAUD_115200,
}BAUDRATE_t;

typedef enum tag_CHANNEL_BAUDRATE
{
	BAUDRATE_1200 = 1200,
	BAUDRATE_2400 = 2400,
	BAUDRATE_4800 = 4800,
	BAUDRATE_9600 = 9600,
	BAUDRATE_19200 = 19200,
	BAUDRATE_38400 = 38400,
	BAUDRATE_57600 = 57600,
	BAUDRATE_115200 = 115200,
}CHANNEL_BAUDRATE;


/*typedef enum tag_CHANNEL_PARITY
{
	PARITY_NONE = 0,
	PARITY_ODD,
	PARITY_EVEN,
}CHANNEL_PARITY;*/


ENN_ErrorCode_t UART_Init(CHANNEL_t CHANNELX, ENN_U32 BaudRate, ENN_U8 DataBit, ENN_U8 StopBit, ENN_U8 Prity);
ENN_S32 UART_Write(CHANNEL_t CHANNELX, ENN_U8 *pBuf, ENN_U16 Len);
ENN_S32 UART_Read(CHANNEL_t CHANNELX, ENN_U8 *pBuf, ENN_U32 Len);
ENN_S32 UART_Open(ENN_CHAR *cSerialName, ENN_U32 BaudRate, ENN_U8 DataBit, ENN_U8 StopBit, ENN_U8 Prity);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

