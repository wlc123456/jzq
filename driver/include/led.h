/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：uart.h
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/
#ifndef __LED_H__
#define __LED_H__


#ifdef __cplusplus
extern "C"
{
#endif

#include "ennType.h"


typedef enum tag_LED_Status
{
	LED_NORMAL = 0,    //绿灯
	LED_ERROR,             //红灯
	LED_OFF,                   //灭灯
}LED_Status;

ENN_S32 ENNLED_Init(ENN_VOID);
ENN_S32 ENNLED_On(CHANNEL_t CHANNELX, LED_Status led_status);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

