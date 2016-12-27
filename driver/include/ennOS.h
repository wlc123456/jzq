/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：ennOS.h
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/
#ifndef _ENNOS_H_
#define _ENNOS_H_


#ifdef __cplusplus
extern "C"
{
#endif

#include "ennType.h"

#ifndef ENNOS_TIMEOUT_IMMEDIATE
#define   ENNOS_TIMEOUT_IMMEDIATE  0x0
#endif

#define   ENNOS_TIMEOUT_INFINITY  0xFFFFFFFF


typedef  ENN_U32  ENNOS_TASK_t;
typedef enum
{
	ENNOS_TASK_PRIORITY_LOW=7,  /*!< low task priority*/
	ENNOS_TASK_PRIORITY_MIDDLE, /*!< middle task priority*/
	ENNOS_TASK_PRIORITY_HIGH    /*!< high task priority*/
}ENNOS_TASK_Priority_t;

typedef enum
{
	ENNOS_TASK_START,             /*!<Create a task and start it.*/
	ENNOS_TASK_SUSPEND                /*!<Create the task already suspended.*/
}ENNOS_TASK_CreateMode_t;

typedef   ENN_VOID(*ENNOS_TASKENTRY_f)(ENN_VOID *pParam);

/* Sem Func */
typedef enum
{
	ENNOS_SEMA_CREATE_FIFO,           /*!< create semaphore mode  */
	ENNOS_SEMA_CREATE_PRIORITY        /*!< create priority semaphore mode */
}ENNOS_SEMA_CreateMode_t;
typedef ENN_U32  ENNOS_SEMA_t;

typedef enum
{
    ENNOS_MSG_CREATE_FIFO,                /*!< create message mode with timeout capability */
    ENNOS_MSG_CREATE_PRIORITY             /*!< create priority message mode without timeout capabilithy*/
}ENNOS_MSG_CreateMode_t;
typedef ENN_U32  ENNOS_MSG_t;


ENN_VOID ENNTRACE(ENN_CHAR *fmt, ...);
ENN_U32 ENNStr_To_Hex(ENN_CHAR *str);
ENN_ErrorCode_t ENNOS_DelayTask(ENN_U32 delay);

extern ENN_ErrorCode_t ENNOS_Init(ENN_VOID);
ENN_ErrorCode_t ENNOS_CreateMessage (ENN_U32 elementSize,ENN_U32 noElements, 
												    ENNOS_MSG_CreateMode_t msgMode, ENNOS_MSG_t *message);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

