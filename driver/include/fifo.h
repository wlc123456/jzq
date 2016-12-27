/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：FIFO.h
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/
#ifndef _FIFO_H_
#define _ENNOS_H_


#ifdef __cplusplus
extern "C"
{
#endif

#include "ennType.h"

ENN_S32 ENNFIFO_Open(const ENN_CHAR *fifoname, ENN_S32 flags);
ENN_S32 ENNFIFO_create(ENN_CHAR *name);
ENN_S32 ENNFIFO_write(ENN_S32 fd, ENN_CHAR *buff_w, ENN_S32 len_w);
ENN_S32 ENNFIFO_read(ENN_S32 fd, ENN_CHAR *buff_r, ENN_S32 len_r);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

