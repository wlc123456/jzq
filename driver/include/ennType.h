/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：ennType.h
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/
#ifndef _ENNTYPES_H_
#define _ENNTYPES_H_


#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef signed char         SINT8;
typedef signed short        SINT16;
typedef signed int          SINT32;

typedef unsigned char       ENN_U8;
typedef unsigned short      ENN_U16;
typedef unsigned int        ENN_U32;
typedef signed char         ENN_S8;
typedef signed short        ENN_S16;
typedef signed int          ENN_S32;
typedef unsigned char       ENN_BOOL;
typedef void				ENN_VOID;
typedef unsigned long       ENN_ULONG;
typedef signed long         ENN_SLONG;
typedef long long		    ENN_LLONG;
typedef char				ENN_CHAR;
typedef float			ENN_FLOAT;

/*#ifndef ENN_SUCCESS
#define ENN_SUCCESS      	0x00000000
#endif

#ifndef ENN_FAIL
#define ENN_FAIL         	0xFFFFFFFF
#endif*/

#ifndef ENN_TRUE
#define ENN_TRUE         	0x01
#endif

#ifndef ENN_FALSE
#define ENN_FALSE        	0x00
#endif

#define bool int

#define TRUE         		0x01
#define FALSE         		0x00

#ifndef NULL
#define NULL 				0
#endif

/*typedef UINT32  ENN_ErrorCode_t;


typedef enum PL_RetCode
{
    PL_SUCCESS = 0,
    PL_ERR_NONE = 0,
    PL_MALLOC_FAIL,


	PL_MODBUS_ERROR,


    PL_ERR_UNKNOWN = -1,
    PL_FAILURE = -1,
}PL_RetCode_t;*/


typedef enum ENN_ErrorCode
{
    ENN_SUCCESS = 0,
    ENN_ERR_NONE = 0,
    
    ENN_ERR_INVALID_FUNCODE = 1,
	ENN_ERR_INVALID_REG_ADDR,
    ENN_ERR_INVALID_REG_NUM,
	ENN_MEM_MALLOC_FAIL,
	


	ENN_MODBUS_ERROR = 4,


    ENN_ERR_UNKNOWN = -1,
    ENN_FAIL = -1,
}ENN_ErrorCode_t;

//add by hyptek,2015
typedef enum DPA_Mode	//upload mode:modbus or 102
{
	ENN_IEC102_MODE = 1,
	ENN_MODBUS_MODE,
}DPA_Mode_t;



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif













