/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：ModBus_Slave_Table.c
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/	
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/tcp.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


#include "ennAPI.h"
#include "ennSocket.h"
#include "ModBus_TCP.h"
#include "ModBus_Slave_Table.h"
#include  "test_645.h"






  
#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */

#define DEBUG_TRACE   printf
//#define DEBUG_TRACE

#define DEBUG_PRINT printf
//#define DEBUG_PRINT

#define DEBUG_645 printf
//#define DEBUG_645


#define MODBUS_HISTORY_DATA_WRITE_FILE		"/home/historydata.log"
#define MODBUS_HISTORY_DATA_READ_FILE		"/home/data.log"

#define MODBUS_FILE_COILSTATUS 			"1.ini"
#define MODBUS_FILE_DISCRETE_INPUTS 	"2.ini"
#define MODBUS_FILE_HOLDING_REGISTER 	"3.ini"

#define HYPTEK_DEBUG_PRINT 

rlSection *grlSection_Head = NULL;
Channel_List *gChannel_List_Head = NULL;
extern ENN_S32 gChannel1_Handle;
extern ENN_S32 gChannel2_Handle;
extern ENN_S32 gChannel3_Handle;
extern ENN_S32 gChannel4_Handle;
extern ENN_S32 gChannel5_Handle;
extern ENN_S32 gChannel6_Handle;
extern ENN_S32 gChannel7_Handle;
extern ENN_S32 gChannel8_Handle;

/*声明全局锁*/
pthread_mutex_t MBS_mutexs[8];
ENN_U8 DevCount = 0;



FunCode_List *gFunCode_List_head = NULL;
FunCode_List *pLast_FunCode_List = NULL;

FunCode_Write_List *gFunCode_Write_List_Head = NULL;
FunCode_Write_List *pLast_FunCode_Write_List = NULL;

static ENNOS_SEMA_t g_memsema = 0;

ENN_U32 Modbus_Idletime = (4*1000)/96;
extern Channel_List *gSlave_Set_List_Head;

extern UINT16 DevRegNum_Total;

static ENN_U32 gFirst_History_Time = 0;
ENN_ErrorCode_t eMBFuncReadCoils(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List);
ENN_ErrorCode_t eMBFuncReadDiscreteInputs(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List);
ENN_ErrorCode_t eMBFuncReadHoldingRegister(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List);
ENN_ErrorCode_t eMBFuncReadInputRegister(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List);
ENN_ErrorCode_t eMBFuncWriteSingleCoils(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List);
ENN_ErrorCode_t eMBFuncWriteSingleRegister(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List);
ENN_ErrorCode_t eMBFuncWriteMulitpleRegister(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List);

typedef ENN_ErrorCode_t (*MBFunctionHandler)(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List);

typedef struct
{
    ENN_U8 u8FunCode;
    MBFunctionHandler functionHandler;
} xMBFunctionHandler;

static const xMBFunctionHandler xFuncHandlers[] = 
{
        {MB_FUNC_READ_COILS, 				eMBFuncReadCoils},
	{MB_FUNC_READ_DISCRETE_INPUTS, 		eMBFuncReadDiscreteInputs},
	{MB_FUNC_READ_HOLDING_REGISTER, 	eMBFuncReadHoldingRegister},
	{MB_FUNC_READ_INPUT_REGISTER, 		eMBFuncReadInputRegister},
	{MB_FUNC_WRITE_SINGLE_COIL, 		eMBFuncWriteSingleCoils},
   	{MB_FUNC_WRITE_SINGLE_REGISTER,	eMBFuncWriteSingleRegister},
	{MB_FUNC_WRITE_MULTIPLE_REGISTERS, 	eMBFuncWriteMulitpleRegister},
};

#define NumFuncHandler    (sizeof(xFuncHandlers)/sizeof(xMBFunctionHandler))

static xMBFunctionHandler *GetFuncHandler(ENN_U8 u8FunCode)
{
	ENN_U32 i;
	xMBFunctionHandler *functionHandler = NULL;
	
	for(i=0;i<NumFuncHandler;i++)
	{
		if(xFuncHandlers[i].u8FunCode == u8FunCode)
		{
			functionHandler = &xFuncHandlers[i];
		}
	}

	return functionHandler;
}

ENN_ErrorCode_t eMBFuncReadCoils(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List)
{
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;
	Register_List *pRegister_List_Add = NULL;
	Register_List *pCurrent_Register_List = NULL;
	Register_List *pLast_Register_List = NULL;
	ENN_U8 i = 0;
	ENN_U32 u32Len = 0;
	ENN_U8 u8Len = 0;
	ENN_CHAR *pTemp = NULL;
	ENN_CHAR aCTmp[MODBUS_REG_ATTR_MAX_LEN];
	ENN_CHAR *pTmp = NULL;
	ENN_CHAR *pName = NULL;
	ENN_CHAR *pAttr = NULL;
	ENN_CHAR *pBtOrder = NULL;
	ENN_U16	u16RegAddr = 0;
	ENN_U16	u16LastRegAddr = 0;
	ENN_U16	u16TotalCount = 0;
	ENN_U16 u16TotalRegNum = 0;

	ENN_FLOAT	fRegK;
	ENN_FLOAT	fRegD;
	ENN_U16	u16SvInterval;
	ENN_U16	u16RegID;
	//ENN_U8		u8ByteOrder;
	
	ENNAPI_ASSERT(NULL != pStr);
	ENNAPI_ASSERT(NULL != pFunCode_List);
	//return ENN_SUCCESS;
	//ENNTRACE("%s, %d, %X, %d, %s\n",__FUNCTION__,__LINE__,pFunCode_List, strlen(pStr), pStr);

	pSlave_FunCode_Temp = pFunCode_List;
	pTemp = pStr;
	u32Len = (ENN_U32)strlen(pStr);

	while('}' != *pTemp)
	//while(0 != u32Len)
	{
		pName = NULL;
		pAttr = NULL;
		for(i=0; i<8; i++)
		{
			u8Len = 0;
			while((',' != *pTemp) && ('}' != *pTemp))
			{
				u8Len++;
				pTemp++;
			}

			//printf("%s, %d, %d\n",__FUNCTION__,__LINE__, strlen(pTemp));
			pTemp -= u8Len;
			u32Len = u32Len - u8Len;

			if(0 == i%8)
			{
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				aCTmp[u8Len] = '\0';
				u16RegAddr = atoi(aCTmp);
				//printf("%s, %d,%d\n",__FUNCTION__,__LINE__, u16RegAddr);
			}
			else if(1 == i%8)
			{
				pName = (ENN_CHAR *)malloc(u8Len + 1);
				if(NULL == pName)
				{
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				memset((void *)pName, 0, u8Len + 1);
				memcpy((void *)pName, (void *)pTemp, (size_t)u8Len);
				//ENNTRACE("pName = %s\n",pName);
			}
			else if(2 == i%8)
			{
				pAttr = (ENN_CHAR *)malloc(u8Len + 1);
				if(NULL == pAttr)
				{
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					free(pName);
					pName = NULL;
					return ENN_FAIL;
				}
				memset((void *)pAttr, 0, u8Len + 1);
				memcpy((void *)pAttr, (void *)pTemp, (size_t)u8Len);
				//ENNTRACE("pAttr = %s\n",pAttr);
			}
			else if(3 == i%8)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				fRegK= atof(aCTmp);
			}
			else if(4 == i%8)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				fRegD= atof(aCTmp);
			}
			else if(5 == i%8)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				u16SvInterval= atoi(aCTmp);
			}
			else if(6 == i%8)
			{
				pBtOrder = (ENN_CHAR *)malloc(u8Len + 1);
				if(NULL == pBtOrder)
				{
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				memset((void *)pBtOrder, 0, u8Len + 1);
				//memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)pBtOrder, (void *)pTemp, u8Len);

				/*if(strcmp(aCTmp, "AB") == 0)
				{
					u8ByteOrder = 0;
				}else if(strcmp(aCTmp, "BA") == 0)
				{
					u8ByteOrder = 1;
				}else{
					u8ByteOrder = 0;
				}*/
			}
			else if(7 == i%8)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				u16RegID= atoi(aCTmp);
			}
			pTemp += u8Len;

			if('}' != *pTemp)
			//if(0 != u32Len)
			{
				pTemp++;
				u32Len--;
			}
			/*else
			{
				pTemp--;
				u32Len++;
			}*/

			//ENNTRACE("%s, %d, %d, %d, %s\n",__FUNCTION__,__LINE__, u32Len, strlen(pTemp), pTemp);
		}

		pRegister_List_Add = (Register_List *)malloc(sizeof(Register_List));
		if(NULL == pRegister_List_Add)
		{
			ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
			free(pName);
			pName = NULL;
			free(pAttr);
			pAttr = NULL;
			return ENN_FAIL;
		}
		memset((void *)pRegister_List_Add, 0, sizeof(Register_List));
		if(0 != (u16RegAddr - u16LastRegAddr))
		{
			pRegister_List_Add->u8Interval = (u16RegAddr - u16LastRegAddr);
		}
		u16LastRegAddr = u16RegAddr;
		pRegister_List_Add->u16RegAddr = u16RegAddr;
		pRegister_List_Add->u16RegNum = 1;
		pRegister_List_Add->pName = pName;
		pRegister_List_Add->pAttr = pAttr;
		pRegister_List_Add->fRegK = fRegK;
		pRegister_List_Add->fRegD = fRegD;
		pRegister_List_Add->u16SvInterval = u16SvInterval;
		pRegister_List_Add->pBtOrder = pBtOrder;
		pRegister_List_Add->u16RegID= u16RegID;
		pRegister_List_Add->next = NULL;

		DevRegNum_Total++;

		if(NULL == pSlave_FunCode_Temp->pRegister_List)
		{
			pSlave_FunCode_Temp->pRegister_List = pRegister_List_Add;
			pRegister_List_Add->u8Interval = 0;
			u16TotalCount++;
		}
		else
		{
			pLast_Register_List->next = pRegister_List_Add;
			u16TotalCount = u16TotalCount + pRegister_List_Add->u8Interval;
		}
		u16TotalRegNum += 1;
		pLast_Register_List = pRegister_List_Add;
		ENNTRACE("%s, %d, %d, %d, %s\n",__FUNCTION__,__LINE__, u32Len, strlen(pTemp), pTemp);
	}

	printf("[%s],%d\n",__FUNCTION__,__LINE__);
	pSlave_FunCode_Temp->u16Count = u16TotalCount;
	pSlave_FunCode_Temp->u16TotalRegNum = u16TotalRegNum;
	
	return ENN_SUCCESS;
}


ENN_ErrorCode_t eMBFuncReadDiscreteInputs(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List)
{
	ENNAPI_ASSERT(NULL != pStr);
	ENNAPI_ASSERT(NULL != pFunCode_List);

	return eMBFuncReadCoils(pStr, pFunCode_List);
}

ENN_ErrorCode_t eMBFuncReadHoldingRegister(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List)
{
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;
	Register_List *pRegister_List_Add = NULL;
	Register_List *pLast_Register_List = NULL;
	Register_List *pCurrent_Register_List = NULL;
	ENN_U8 i = 0;
	ENN_U8 u8RegNum = 0;
	ENN_U32 u32Len = 0;
	ENN_U8 u8Len = 0;
	ENN_CHAR *pTemp = NULL;
	ENN_CHAR aCTmp[MODBUS_REG_ATTR_MAX_LEN];
	ENN_CHAR *pTmp = NULL;
	ENN_CHAR *pName = NULL;
	ENN_CHAR *pAttr = NULL;
	ENN_CHAR *pBtOrder = NULL;
	ENN_U16	u16RegAddr = 0;
	ENN_U16	u16LastRegAddr = 0;
	ENN_U8 u8LastRegNum = 0;
	ENN_U16	u16TotalCount = 0;
	ENN_U16 u16TotalRegNum = 0;

	ENN_FLOAT	fRegK;
	ENN_FLOAT	fRegD;
	ENN_U16	u16SvInterval;
	ENN_U16	u16RegID;
	
	BACNET_OBJECT_TYPE   object_type;  //bacnet对象类型
	ENN_U32	object_instance;                 //bacnet对象类型的实例
	BACNET_ENGINEERING_UNITS   units;   //bacnet单位
	
	//ENN_U8		u8ByteOrder;
	
	ENNAPI_ASSERT(NULL != pStr);
	ENNAPI_ASSERT(NULL != pFunCode_List);
	
	ENNTRACE("%s, %d, %X, %d, %s\n",__FUNCTION__,__LINE__,pFunCode_List, strlen(pStr), pStr);
	//return ENN_SUCCESS;
	
	pSlave_FunCode_Temp = pFunCode_List;
	pTemp = pStr;
	u32Len = (ENN_U32)strlen(pStr);

	//while(0 != u32Len)
	while('}' != *pTemp)   //0000,1,电压,电压V,1.00,0.00,50,AB,2,8,1,0004,1,频率,频率HZ,1.00,0.00,50,AB,0,27,2}
	{				  
		pName = NULL;
		pAttr = NULL;
		
		for(i=0; i<12; i++)
		{
			u8Len = 0;
			while((',' != *pTemp) && ('}' != *pTemp))
			{
				u8Len++;
				pTemp++;
			}

			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
			pTemp -= u8Len;
			u32Len = u32Len - u8Len;

			if(0 == i%12)
			{
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				u16RegAddr = ENNStr_To_Hex(aCTmp);
			}
			else if(1 == i%12)
			{
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				u8RegNum = atoi(aCTmp);
			}
			else if(2 == i%12)  
			{   //0000,1,电压,电压V,1.00,0.00,50,AB,2,8,1,0004,1,频率,频率HZ,1.00,0.00,50,AB,0,27,2}
				pName = (ENN_CHAR *)malloc(u8Len + 1);
				if(NULL == pName)
				{
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				memset((void *)pName, 0, u8Len + 1);
				memcpy((void *)pName, (void *)pTemp, (size_t)u8Len);
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pName), pName);
				//ENNTRACE("pName = %s\n",pName);
			}
			else if(3 == i%12)  //0000,1,电压,电压V,1.00,0.00,50,AB,2,8,1,0004,1,频率,频率HZ,1.00,0.00,50,AB,0,27,2}
			{
				pAttr = (ENN_CHAR *)malloc(u8Len + 1);
				if(NULL == pAttr)
				{
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				memset((void *)pAttr, 0, u8Len + 1);
				memcpy((void *)pAttr, (void *)pTemp, (size_t)u8Len);
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pAttr), pAttr);
				//ENNTRACE("pAttr = %s\n",pAttr);
			}
			else if(4 == i%12)  //0000,1,电压,电压V,1.00,0.00,50,AB,2,8,1,0004,1,频率,频率HZ,1.00,0.00,50,AB,0,27,2}
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				fRegK= atof(aCTmp);
			}
			else if(5 == i%12)  //0000,1,电压,电压V,1.00,0.00,50,AB,2,8,1,0004,1,频率,频率HZ,1.00,0.00,50,AB,0,27,2}
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				fRegD= atof(aCTmp);
			}
			else if(6 == i%12)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				u16SvInterval= atoi(aCTmp);
			}
			else if(7 == i%12)
			{
				pBtOrder = (ENN_CHAR *)malloc(u8Len + 1);
				if(NULL == pBtOrder)
				{
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				memset((void *)pBtOrder, 0, u8Len + 1);
				memcpy((void *)pBtOrder, (void *)pTemp, u8Len);
			}
			else if(8 == i%12)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				u16RegID= atoi(aCTmp);
			}
			else if(9 == i%12)
			{
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp,0,MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp,(void *)pTemp,u8Len);
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				object_type = (BACNET_OBJECT_TYPE)atoi(aCTmp);
			}
			else if(10 == i%12)
			{
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp,0,MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp,(void *)pTemp,u8Len);
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				object_instance = (BACNET_ENGINEERING_UNITS)atoi(aCTmp);
			}
			else if(11 == i%12)
			{
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp,0,MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp,(void *)pTemp,u8Len);
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				units = (BACNET_ENGINEERING_UNITS)atoi(aCTmp);
			}
			pTemp += u8Len;
			//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);

			if('}' != *pTemp)    //跨过字符串中的,号
			{
				pTemp++;
				u32Len--;
			}
			
			//ENNTRACE("%s, %d, %d, %d, %s\n",__FUNCTION__,__LINE__, u32Len, strlen(pTemp), pTemp);
		}
		//printf("[%s],%d,test****\n",__FUNCTION__,__LINE__);

		pRegister_List_Add = (Register_List *)malloc(sizeof(Register_List));
		if(NULL == pRegister_List_Add)
		{
			ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
			free(pName);
			pName = NULL;
			free(pAttr);
			pAttr = NULL;
			return ENN_FAIL;
		}
		memset((void *)pRegister_List_Add, 0, sizeof(Register_List));
		if(u8LastRegNum != (u16RegAddr - u16LastRegAddr))  //没有间隙的判断方法
		{											     //用当前寄存器地址减去上一个寄存器的地址，判断是否等于上一个寄存器的长度，如果相等，那么就没有间隙
			pRegister_List_Add->u8Interval = (u16RegAddr - u16LastRegAddr - u8LastRegNum);
		}
		u16LastRegAddr = u16RegAddr;
		u8LastRegNum = u8RegNum;
		pRegister_List_Add->u16RegAddr = u16RegAddr;
		pRegister_List_Add->u16RegNum = u8RegNum;
		pRegister_List_Add->pName = pName;
		pRegister_List_Add->pAttr = pAttr;
		pRegister_List_Add->fRegK = fRegK;
		pRegister_List_Add->fRegD = fRegD;
		pRegister_List_Add->u16SvInterval = u16SvInterval;
		pRegister_List_Add->pBtOrder = pBtOrder;
		pRegister_List_Add->u16RegID= u16RegID;
		pRegister_List_Add->object_type = object_type;
		pRegister_List_Add->object_instance = object_instance;
		pRegister_List_Add->units = units;
		pRegister_List_Add->next = NULL;

		DevRegNum_Total++;

		if(NULL == pSlave_FunCode_Temp->pRegister_List)
		{
			pSlave_FunCode_Temp->pRegister_List = pRegister_List_Add;
			pRegister_List_Add->u8Interval = 0;
			u16TotalCount += u8RegNum;
		}
		else
		{
			pLast_Register_List->next = pRegister_List_Add;
			u16TotalCount = u16TotalCount + pRegister_List_Add->u8Interval + u8RegNum;  //所有涉及的寄存器数量(包括间隔)
		}
		u16TotalRegNum += u8RegNum;  //所有寄存器的数量(长度)
		pLast_Register_List = pRegister_List_Add;
		//ENNTRACE("pTemp:%s\n",pTemp);
		//ENNTRACE("%4.4XH, %d, %s, %s\n",u32RegAddr, u8RegNum, pName, pAttr);
	}

	pSlave_FunCode_Temp->u16Count = u16TotalCount;      //当前寄存器的总长度包括间隙
	pSlave_FunCode_Temp->u16TotalRegNum = u16TotalRegNum;     //当前功能码下的寄存器总长度(个数)
	ENNTRACE("%s, %d,pSlave_FunCode_Temp->u16Count = %d,pSlave_FunCode_Temp->u16TotalRegNum = %d\n",
			       __FUNCTION__,__LINE__,pSlave_FunCode_Temp->u16Count,pSlave_FunCode_Temp->u16TotalRegNum );
	return ENN_SUCCESS;
}


ENN_ErrorCode_t eMBFuncReadInputRegister(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List)
{
	ENNAPI_ASSERT(NULL != pStr);
	ENNAPI_ASSERT(NULL != pFunCode_List);
	
	ENNTRACE("%s, %d, %X, %d, %s\n",__FUNCTION__,__LINE__,pFunCode_List, strlen(pStr), pStr);	
	eMBFuncReadHoldingRegister(pStr, pFunCode_List);
	
	return ENN_SUCCESS;
}


ENN_ErrorCode_t eMBFuncWriteSingleCoils(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List)
{
	ENNAPI_ASSERT(NULL != pStr);
	ENNAPI_ASSERT(NULL != pFunCode_List);

	return eMBFuncReadCoils(pStr, pFunCode_List);
}

ENN_ErrorCode_t eMBFuncWriteSingleRegister(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List)
{
	ENNAPI_ASSERT(NULL != pStr);
	ENNAPI_ASSERT(NULL != pFunCode_List);

	return eMBFuncReadCoils(pStr, pFunCode_List);
}


ENN_ErrorCode_t eMBFuncWriteMulitpleRegister(ENN_CHAR *pStr, Slave_FunCode_List *pFunCode_List)
{
	ENNAPI_ASSERT(NULL != pStr);
	ENNAPI_ASSERT(NULL != pFunCode_List);

	return eMBFuncReadHoldingRegister(pStr, pFunCode_List);
}


ENN_ErrorCode_t eMBFunc_Read_645(ENN_CHAR *pStr, Dev645_List *pDev645_List)
{
	ENN_CHAR *pTemp = NULL;
	ENN_U8 i = 0;
	ENN_U8 u8Index = 0;
	ENN_U8 u8Len = 0;
	ENN_U8 u8ChannelNum = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U32 u32Len = 0;
	ENN_U32 u32ChannelCode = 0;//[645_MAX_CHANNEL_NUM];
	Dev645_List *pDev645_List_Temp = NULL;
	Code_645_List *pCode_645_Add = NULL;
	Code_645_List *pLast_Code_645 = NULL;
	ENN_CHAR *pName = NULL;
	ENN_CHAR *pAttr = NULL;
	ENN_CHAR aCTmp[MODBUS_REG_ATTR_MAX_LEN];
	Data_Type		eData_Type;

	ENN_FLOAT	fRegK;
	ENN_FLOAT	fRegD;
	ENN_U16		u16SvInterval;
	ENN_U16		u16RegID;
	ENN_U8		u8ByteOrder;
	
	ENNAPI_ASSERT(NULL != pStr);
	ENNAPI_ASSERT(NULL != pDev645_List);
	
	ENNTRACE("%s, %d, 0x%X, %d, %s\n",__FUNCTION__,__LINE__,pDev645_List, strlen(pStr), pStr);
	//return ENN_SUCCESS;
	pDev645_List_Temp = pDev645_List;	
	pTemp = pStr;
	
	//memset((void *)u32ChannelCode, 0, 645_MAX_CHANNEL_NUM * sizeof(ENN_U32));
	u8ChannelNum = 0;
	u8Index = 0;
	u32Len = (ENN_U32)strlen(pStr);

	while('}' != *pTemp)
	{
		pName = NULL;
		pAttr = NULL;
		
		for(i=0; i<9; i++)
		{
			u8Len = 0;
			while((',' != *pTemp) && ('}' != *pTemp))
			{
				u8Len++;
				pTemp++;
			}

			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
			pTemp -= u8Len;
			u32Len = u32Len - u8Len;

			if(0 == i%9)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				u32ChannelCode = ENNStr_To_Hex(aCTmp);
				u8Index++;
				ENNTRACE("%X\n", u32ChannelCode);
			}
			else if(1 == i%9)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				u16RegNum = atoi(aCTmp);
			}
			else if(2 == i%9)
			{
				pName = (ENN_CHAR *)malloc(u8Len + 1);
				if(NULL == pName)
				{
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				memset((void *)pName, 0, u8Len + 1);
				memcpy((void *)pName, (void *)pTemp, (size_t)u8Len);
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pName), pName);
				//ENNTRACE("pName = %s\n",pName);
			}
			else if(3 == i%9)
			{
				pAttr = (ENN_CHAR *)malloc(u8Len + 1);
				if(NULL == pAttr)
				{
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				memset((void *)pAttr, 0, u8Len + 1);
				memcpy((void *)pAttr, (void *)pTemp, (size_t)u8Len);
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pAttr), pAttr);
				//ENNTRACE("pAttr = %s\n",pAttr);
				
				const ENN_U8 floatStr[]={0xE6,0xB5,0xAE,0xE7,0x82,0xB9,0xE6,0x95,0xB0,0x00};

				if(0 == strcmp(pAttr,floatStr))
				{
					eData_Type = Data_DECIMAL;
				}
				else
				{
					eData_Type = Data_INT;
				}
			}
			else if(4 == i%9)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				fRegK= atof(aCTmp);
			}
			else if(5 == i%9)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				fRegD= atof(aCTmp);
			}
			else if(6 == i%9)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				u16SvInterval= atoi(aCTmp);
			}
			else if(7 == i%9)
			{
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				//u16RegID= atoi(aCTmp);
				if(strcmp(aCTmp, "AB") == 0)
				{
					u8ByteOrder = 0;
				}else if(strcmp(aCTmp, "BA") == 0)
				{
					u8ByteOrder = 1;
				}else{
					u8ByteOrder = 0;
				}
			}
			else if(8 == i%9)
			{
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				memcpy((void *)aCTmp, (void *)pTemp, u8Len);
				//aCTmp[u8Len] = '\0';
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(aCTmp), aCTmp);
				u16RegID= atoi(aCTmp);
			}
			pTemp += u8Len;
			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pTemp), pTemp);
			ENNTRACE("i = %d\n",i);

			if('}' != *pTemp)
			{
				pTemp++;
				u32Len--;
			}
			
			//ENNTRACE("%s, %d, %d, %d, %s\n",__FUNCTION__,__LINE__, u32Len, strlen(pTemp), pTemp);
		}

		pCode_645_Add = (Code_645_List *)malloc(sizeof(Code_645_List));
		if(NULL == pCode_645_Add)
		{
			ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
			free(pName);
			pName = NULL;
			free(pAttr);
			pAttr = NULL;
			return ENN_FAIL;
		}
		memset((void *)pCode_645_Add, 0, sizeof(Code_645_List));
		pCode_645_Add->u32ChannelCode = u32ChannelCode;
		pCode_645_Add->u16RegNum = u16RegNum;
		pCode_645_Add->eData_Type = eData_Type;
		pCode_645_Add->pName = pName;
		pCode_645_Add->pAttr = pAttr;
		pCode_645_Add->fRegK = fRegK;
		pCode_645_Add->fRegD = fRegD;
		pCode_645_Add->u16SvInterval = u16SvInterval;
		pCode_645_Add->u8ByteOrder = u8ByteOrder;
		pCode_645_Add->u16RegID= u16RegID;
		pCode_645_Add->next = NULL;

		DevRegNum_Total++;

		if(NULL == pDev645_List_Temp->pCode_645_List)
		{
			pDev645_List_Temp->pCode_645_List = pCode_645_Add;
		}
		else
		{
			pLast_Code_645->next = pCode_645_Add;
		}
		pLast_Code_645 = pCode_645_Add;
		ENNTRACE("%s, %d, %d, %d, %s\n",__FUNCTION__,__LINE__, u32Len, strlen(pTemp), pTemp);
	}
	
	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	pDev645_List_Temp->u8ChannelNum = u8Index;
	
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Set_IdelTime(ENN_U32 BaudRate)
{
	switch(BaudRate)
	{
		case BAUDRATE_4800:
			Modbus_Idletime = (4*1000)/48;
			break;
		case BAUDRATE_9600:
			Modbus_Idletime = (4*1000)/96;
			break;
		case BAUDRATE_19200:
			Modbus_Idletime = (4*1000)/192;
			break;
		case BAUDRATE_38400:
			Modbus_Idletime = (4*1000)/384;
			break;
		case BAUDRATE_57600:
			Modbus_Idletime = (4*1000)/576;
			break;
		case BAUDRATE_115200:
			Modbus_Idletime = (4*1000)/1152;
			break;
		default:
			Modbus_Idletime = (4*1000)/96;
			break;
	}

	return ENN_SUCCESS;
}

ENN_ErrorCode_t ENNCopy_Name(ENN_CHAR *pname, const ENN_CHAR *pbuf)
{
	ENN_U8 i = 0;
	ENN_CHAR *p = pname;

	ENNAPI_ASSERT((NULL != pname) && (NULL != pbuf));

	pname[0] = '\0';//这句代码没用
	
	while(('=' != pbuf[i]) && ('\0' != pbuf[i]))
	{
		*p++ = pbuf[i++];
	}
	*p = '\0';
	i--; // eventually delete spaces
	while((i>=0) && ((pname[i] == ' ') || (pname[i] == '\t')))
	{
		pname[i--] = '\0';
	} 

	return ENN_SUCCESS;
}

ENN_ErrorCode_t ENNCopy_Param(ENN_CHAR *pparam, const ENN_CHAR *pbuf)
{
	const ENN_CHAR *cptr = pbuf;

	ENNAPI_ASSERT((NULL != pparam) && (NULL != pbuf));
	pparam[0] = '\0';  //这句也没用
	ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(cptr),cptr);
	while(1)
	{
		cptr = strchr(cptr, '=');
		if(cptr == NULL) 
		{
			return ENN_FAIL;
		}
		cptr++;     //指向参数的第一个字符
		//if(cptr[-2] != '\\')
		{
			break;
		}
	} 
	while(((*cptr == ' ') || (*cptr == '\t')) && *cptr != '\0') //跳过空白
	{
		cptr++;
	}
	ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(cptr),cptr);
	if(*cptr == '\0') 
	{
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	while((*cptr != '\0') && (*cptr != '\n')) 
	{
		*pparam++ = *cptr++;
	}
	*pparam = '\0';
	pparam--;
	while ((*pparam == ' ') || (*pparam == '\t'))
	{
		*pparam-- = '\0';
	}
	
	return ENN_SUCCESS;
}

ENN_CHAR *ENNModBus_Get_Text(const ENN_CHAR *section, const ENN_CHAR *name)
{
	rlSection *current_section;
	rlSectionName *current_name;

	ENNAPI_ASSERT(NULL != section);
	ENNAPI_ASSERT(NULL != name);

	current_section = grlSection_Head;
	while(current_section != NULL)
	{
		if(strcmp(section, current_section->name) == 0)
		{
			current_name = current_section->firstName;
			while(current_name != NULL)
			{
				if((NULL != current_name->name) && (strcmp(name, current_name->name) == 0))
				{
					return current_name->param;
				}
				current_name = current_name->next;
			}
			return NULL;
		}
		current_section = current_section->next;
	}
	
	return NULL;;
}

/*************************************************************************
*  名字:  ENNModBus_Set_Text
*  说明:  解析配置文件,使key/value一一对应,系统启动时调用
*  输入参数：section: 配置文件中包含[]的字符串
*           name: Key的值的字符串
*           text: Value的字符串
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/

ENN_ErrorCode_t ENNModBus_Set_Text(const ENN_CHAR *section, const ENN_CHAR *name, const ENN_CHAR *text)
{
	rlSection 		*current_section = NULL; 
	rlSection 		*last_section = NULL;
	rlSection 		*add_section = NULL;
	rlSectionName 	*current_name = NULL;
	rlSectionName 	*last_name = NULL;
	rlSectionName 		*add_name = NULL;
	
	ENNAPI_ASSERT(NULL != section);

	current_section = grlSection_Head;
	while(NULL != current_section)
	{
		if(strcmp(section, current_section->name) == 0)
	        {
	    	        last_name = NULL;
			current_name = current_section->firstName;
			while(current_name != NULL)
			{
				if((NULL != name) 
				&& ('#' != name[0]) 
				&& ('\0' != name[0]) 
				&& (strcmp(name, current_name->name) == 0))
				{
					if(NULL != current_name->param)
					{
						free(current_name->param);
						current_name->param = NULL;
					}
					
					if(text == NULL)
					{
						current_name->param = (ENN_CHAR *)malloc(1);
						if(NULL == current_name->param)
						{
							return ENN_FAIL;
						}
						current_name->param[0] = '\0';
					}
					else
					{
						current_name->param = (ENN_CHAR *)malloc(strlen(text)+1);
						if(NULL == current_name->param)
						{
							return ENN_FAIL;
						}
						memset((void *)current_name->param, 0, strlen(text)+1);
						strcpy((char *)current_name->param, (char *)text);
					}
					
					return ENN_SUCCESS;
				}
				last_name = current_name;
				current_name = current_name->next;
			}

			add_name = (rlSectionName *)malloc(sizeof(rlSectionName));   //构造rlSectionName 的第一个空间
			if(NULL == add_name)
			{
				return ENN_FAIL;
			}
			memset((void *)add_name, 0, sizeof(rlSectionName));
			
			if(last_name == NULL)       //current_section->firstName 和current_name 指向add_name
			{
				current_section->firstName = add_name;
				current_name = current_section->firstName;
			}
			else   //last_name 的作用是个记录游标联系1044行一起看
			{
				last_name->next = add_name;
				current_name = last_name->next;
			}
			if(name == NULL)
			{
				current_name->name = (ENN_CHAR *)malloc(1);
				if(NULL == current_name->name)
				{
					free(add_name);
					add_name = NULL;
					return ENN_FAIL;
				}
				current_name->name[0] = '\0';
			}
			else
			{
				current_name->name = (ENN_CHAR *)malloc(strlen(name)+1);
				if(NULL == current_name->name)
				{
					free(add_name);
					add_name = NULL;
					return ENN_FAIL;
				}
				memset((void *)current_name->name, 0, strlen(name)+1);
				strcpy((char *)current_name->name, (char *)name);
			}
			if(text == NULL)
			{
				current_name->param = (ENN_CHAR *)malloc(1);
				if(NULL == current_name->param)
				{
					free(current_name->name);
					current_name->name = NULL;
					free(add_name);
					add_name = NULL;
					return ENN_FAIL;
				}
				current_name->param[0] = '\0';
			}
			else
			{
				current_name->param = (ENN_CHAR *)malloc(strlen(text)+1);
				if(NULL == current_name->param)
				{
					free(current_name->name);
					current_name->name = NULL;
					free(add_name);
					add_name = NULL;
					return ENN_FAIL;
				}
				memset((void *)current_name->param, 0, strlen(text)+1);
				strcpy((char *)current_name->param, (char *)text);
			}
			current_name->next = NULL;
			
			return ENN_SUCCESS;
		}
		
		last_section = current_section;
		current_section = current_section->next;
	}

	add_section = (rlSection *)malloc(sizeof(rlSection));      //rlSection为空的时候进行第一个结构体的构造
	if(NULL == add_section)
	{
		return ENN_FAIL;
	}
	memset((void *)add_section, 0, sizeof(rlSection));
	if(NULL == last_section)      //第一个add_section ,grlSection_Head 和last_section 都指向新构造的add_section
	{
		grlSection_Head = add_section;
		last_section = grlSection_Head;
	}
	else                                    //新构造的add_secion放到链表的尾部，同时last_section前移
	{
		last_section->next = add_section;
		last_section = add_section;
	}
	last_section->next = NULL;
	
	last_section->name = (ENN_CHAR *)malloc(strlen(section)+1);
	if(NULL == last_section->name)
	{
		free(add_section);
		add_section = NULL;
		
		return ENN_FAIL;
	}
	memset((void *)last_section->name, 0, strlen(section)+1);
	strcpy((char *)last_section->name, (char *)section);
	//last_section->next = NULL;
	
	if(name == NULL)
	{
		last_section->firstName = NULL;
	}
	else
	{
		last_section->firstName = (rlSectionName *)malloc(sizeof(rlSectionName));
		if(NULL == last_section->firstName)
		{
			free(last_section->name);
			last_section->name = NULL;
			free(add_section);
			add_section = NULL;
			
			return ENN_FAIL;
		}
		memset((void *)last_section->firstName, 0, sizeof(rlSectionName));
		
		current_name = last_section->firstName;
		current_name->name = (ENN_CHAR *)malloc(strlen(name)+1);
		if(NULL == current_name->name)
		{
			free(current_name);
			current_name = NULL;
			free(last_section->name);
			last_section->name = NULL;
			free(add_section);
			add_section = NULL;
			
			return ENN_FAIL;
		}
		memset((void *)current_name->name, 0, strlen(name)+1);
		strcpy((char *)current_name->name, (char *)name);
		if(text == NULL)
		{
			current_name->param = (ENN_CHAR *)malloc(1);
			if(NULL == current_name->param)
			{
				free(current_name->name);
				current_name->name = NULL;
				free(current_name);
				current_name = NULL;
				free(last_section->name);
				last_section->name = NULL;
				free(add_section);
				add_section = NULL;
				
				return ENN_FAIL;
			}
			current_name->param[0] = '\0';
		}
		else
		{
			current_name->param = (ENN_CHAR *)malloc(strlen(text)+1);
			if(NULL == current_name->param)
			{
				free(current_name->name);
				current_name->name = NULL;
				free(current_name);
				current_name = NULL;
				free(last_section->name);
				last_section->name = NULL;
				free(add_section);
				add_section = NULL;
				
				return ENN_FAIL;
			}
			memset((void *)current_name->param, 0, strlen(text)+1);
			strcpy((char *)current_name->param, (char *)text);
		}
		current_name->next = NULL;
	}
}

void test_int()
{
#if 0
	int i = 0;
	int k = 0;
	rlSection 		*tmp = NULL;
	rlSectionName 	*tmp1 = NULL;
	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	tmp = grlSection_Head;
  	while(NULL != tmp)
	{
	  	ENNTRACE("%s\n",tmp->name);
		tmp1 = tmp->firstName;
		i = 0;
		while(NULL != tmp1)
		{
			ENNTRACE("   %s=%s\n",tmp1->name,tmp1->param);
			tmp1 = tmp1->next;
			i++;
		}
		ENNTRACE("i = %d\n", i);
		ENNTRACE("\n");
		tmp = tmp->next;
		k++;
	}
	ENNTRACE("k = %d\n", k);
#else
	Channel_List *pChannel_List_Temp = NULL;
	Slave_List *pSlave_List_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;
	Register_List *pRegister_List_Temp = NULL;
	Dev645_List *pDev645_List_Temp = NULL;
	Code_645_List *pCode_645_List_Temp = NULL;

	DEBUG_PRINT("%s, %d\n", __FUNCTION__, __LINE__);
	pChannel_List_Temp = gChannel_List_Head;
	while(NULL != pChannel_List_Temp)
	{
		DEBUG_PRINT("	Channel = %d, SlaveNum = %d, u32BaudRate = %d\n", pChannel_List_Temp->u8Channel, 
					     pChannel_List_Temp->u8SlaveNum, pChannel_List_Temp->u32BaudRate);
		if(PROTOCOL_MODBUS == pChannel_List_Temp->u8Protocol)
		{
			//pSlave_List_Temp = pChannel_List_Temp->pSlaveList;
			pSlave_List_Temp = pChannel_List_Temp->unDeviceType.pModBus_List;
			while(NULL != pSlave_List_Temp)
			{
				DEBUG_PRINT("	SlaveAddr = %d, SlaveName = %s[%d], format = %d\n", pSlave_List_Temp->u8SlaveAddr, 
							    pSlave_List_Temp->pSlaveName, strlen(pSlave_List_Temp->pSlaveName), 
							    pSlave_List_Temp->u8DataFormat);
				pSlave_FunCode_Temp = pSlave_List_Temp->pFunCode_List;
				while(NULL != pSlave_FunCode_Temp)
				{
					DEBUG_PRINT("	MBFunCode = %d, Count = %d,  %d: \n", pSlave_FunCode_Temp->u8MBFunCode,
								     pSlave_FunCode_Temp->u16Count, pSlave_FunCode_Temp->u16TotalRegNum);
					pRegister_List_Temp = pSlave_FunCode_Temp->pRegister_List;
					while(NULL != pRegister_List_Temp)
					{
						DEBUG_PRINT("	%d, %d, %d\n", pRegister_List_Temp->u8Interval,
									   pRegister_List_Temp->u16RegAddr, pRegister_List_Temp->u16RegNum);
						pRegister_List_Temp = pRegister_List_Temp->next;
					}
					pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
				}

				pSlave_List_Temp = pSlave_List_Temp->next;
			}
		}
		else if((PROTOCOL_645_1997 == pChannel_List_Temp->u8Protocol) || (PROTOCOL_645_2007 == pChannel_List_Temp->u8Protocol))
		{
			pDev645_List_Temp = pChannel_List_Temp->unDeviceType.pDev645_List;
			while(NULL != pDev645_List_Temp)
			{
				DEBUG_PRINT("	pDev645_List_Temp = %s, SlaveName = %s[%d]\n", pDev645_List_Temp->ucDevAddr, 
					pDev645_List_Temp->pSlaveName, strlen(pDev645_List_Temp->pSlaveName));
				pCode_645_List_Temp = pDev645_List_Temp->pCode_645_List;
				while(NULL != pCode_645_List_Temp)
				{
					DEBUG_PRINT("u32ChannelCode = 0x%X, u16RegNum = %d, pName = %s, pAttr = %s\n", 
						pCode_645_List_Temp->u32ChannelCode, pCode_645_List_Temp->u16RegNum, 
						pCode_645_List_Temp->pName, pCode_645_List_Temp->pAttr);
						pCode_645_List_Temp = pCode_645_List_Temp->next;
				}

				pDev645_List_Temp = pDev645_List_Temp->next;
			}
		}

		pChannel_List_Temp = pChannel_List_Temp->next;
	}
#endif
}

/*************************************************************************
*  名字:  ENNModBus_Acqurie_FunCode
*  说明:  用来提取2,0000   3,0000  4,0000  5,0000  16,0000 这5个字符串中的功能码
*  输入参数：eg:3,0000 字符串
*  返回值: 功能码的整形数字
 *************************************************************************/

ENN_U8   ENNModBus_Acqurie_FunCode(ENN_CHAR *pStr)
{
	ENN_U8 u8StrLen = 0;
	ENN_CHAR *pTmp = NULL;
	ENN_U8 u8Len = 0;
	ENN_CHAR *pBuf = NULL;
	ENN_U8 u8FunCode = 0;
	
	u8StrLen = strlen(pStr);  //3,0000
	pTmp = strrchr(pStr, ',');  //3,0000   

	if(NULL == pTmp)
	{
		return ENN_FAIL;
	}
	u8Len = strlen(pTmp);
	
	u8Len = u8StrLen - u8Len;
	pBuf = (ENN_CHAR *)malloc(u8Len+1);
	if(NULL == pBuf)
	{
		return ENN_FAIL;
	}
	memset((void *)pBuf, 0, u8Len+1);
	memcpy((void *)pBuf, (void *)pStr, u8Len);
	ENNTRACE("%s, %d, %s\n",__FUNCTION__,__LINE__,pBuf);

	u8FunCode = atoi(pBuf);
	free(pBuf);

	return u8FunCode;
}


/*************************************************************************
*  名字:  ENNModBus_Add_FunCode
*  说明:  生成虚拟寄存器的说明性文件，并为每个channel分配offset
*  生成以功能码为主值的寄存器虚拟设备说明性文(这行是自己加
*  的，感觉原作者的解释不太好)
*  输入参数：pStr: 配置文件中
*					FUNCODE={{2,0000},{3,0000},{4,0000},{5,0000},{16,0000}}
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/

ENN_ErrorCode_t ENNModBus_Add_FunCode(ENN_CHAR *pStr)
{
	ENN_CHAR cFile[20];
	ENN_CHAR *pTmp = NULL;
	ENN_U16 u16RegAddr = 0;
	ENN_U16 u16Ret = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 u8Len = 0;
	ENN_U8 i = 0;
	ENN_U8 u8ChannelX = 0;
	ENN_CHAR aName[MODBUS_REG_ATTR_MAX_LEN];
	Channel_List *pChannel_List_Temp = NULL;
	Slave_List *pSlave_List_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;
	Register_List *pRegister_List_Temp = NULL;
	FILE 	*fpConfig;
	Addr_Map_List *pAddr_Map_List_Add = NULL;
	Addr_Map_List *pLast_Addr_Map = NULL;
	static ENN_U16 u16LastRegAddr = 0;
	ENN_U16 u16Offset = 0;
	FunCode_List *pFunCode_List_Add = NULL;
	ENN_U16 u16TotalNum = 0;
	ENN_U16 u16DataLen = 0;
	ENN_S32 s32Ret = 0;
	ENN_U8 u8LastRegNum = 0;
	ENN_BOOL bFlag = FALSE;
	Dev645_List *pDev645_List_Temp = NULL;
	Code_645_List *pCode_645_List_Temp = NULL;
	
	ENNAPI_ASSERT(NULL != pStr);

	u8FunCode = ENNModBus_Acqurie_FunCode(pStr);
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u8FunCode);
	
	pTmp = strrchr(pStr, ',');  //3,0000   
	if(NULL == pTmp)
	{
		return ENN_FAIL;
	}
	pTmp++;   //0000
	u16RegAddr = ENNStr_To_Hex(pTmp);
	u16LastRegAddr = u16RegAddr;
	ENNTRACE("%s, %d, %s\n",__FUNCTION__,__LINE__,pTmp);

	memset(cFile, 0, 20);
	sprintf(cFile, "/home/%d.ini", u8FunCode);
	
	printf("%s, %d, %d, cFile = %s\n",__FUNCTION__,__LINE__, strlen(cFile), cFile);
	fpConfig = fopen(cFile, "w+");
	perror("open configure file: ");
	if(NULL == fpConfig)
	{
		printf("ERROR : %s, %d, fpConfig == NULL\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	
	s32Ret = fputs("数据项地址\t寄存器长度\t通道\t设备地址\t寄存器地址\t字节序\t寄存器名字\t寄存器描述\tK系数\tD系数\t对象类型\t对象类型实例\t单位\r\n", fpConfig);
	
	pFunCode_List_Add = (FunCode_List *)malloc(sizeof(FunCode_List));
	if(NULL == pFunCode_List_Add)
	{
		return ENN_FAIL;
	}
	memset((void *)pFunCode_List_Add, 0, sizeof(FunCode_List));
	pFunCode_List_Add->u8MBFunCode = u8FunCode;
	pFunCode_List_Add->u16StartAddr = u16RegAddr;
	memset((void *)pFunCode_List_Add->Offset, 0, 8);

	pChannel_List_Temp = gChannel_List_Head;
	while(NULL != pChannel_List_Temp)
	{
		if(PROTOCOL_MODBUS == pChannel_List_Temp->u8Protocol)
		{
			//pSlave_List_Temp = pChannel_List_Temp->pSlaveList;
			pSlave_List_Temp = pChannel_List_Temp->unDeviceType.pModBus_List;
			while(NULL != pSlave_List_Temp)
			{
				pSlave_FunCode_Temp = pSlave_List_Temp->pFunCode_List;
				while(NULL != pSlave_FunCode_Temp)
				{
					if(u8FunCode == pSlave_FunCode_Temp->u8MBFunCode)
					{
						bFlag = TRUE;
						pRegister_List_Temp = pSlave_FunCode_Temp->pRegister_List;
						while(NULL != pRegister_List_Temp)
						{
							memset((void *)aName, 0, MODBUS_REG_ATTR_MAX_LEN);
							sprintf(aName, "%d", u16LastRegAddr);
							u16Ret = fputs(aName, fpConfig);
							//ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u8FunCode);
							
							if((MB_FUNC_READ_COILS == u8FunCode) 
							|| (MB_FUNC_READ_DISCRETE_INPUTS == u8FunCode)
							|| (MB_FUNC_WRITE_SINGLE_COIL == u8FunCode))
							{
								memset((void *)aName, 0, MODBUS_REG_ATTR_MAX_LEN);
								sprintf(aName, "%d", 1);
								//ENNTRACE("%s, %d, aName = %s\n",__FUNCTION__,__LINE__, aName);
							}
							else
							{
								//ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__, pRegister_List_Temp->u16RegNum);
								memset((void *)aName, 0, MODBUS_REG_ATTR_MAX_LEN);
								sprintf(aName, "%d", pRegister_List_Temp->u16RegNum);
								//ENNTRACE("%s, %d, aName = %s\n",__FUNCTION__,__LINE__, aName);
							}
// 数据项地址\t寄存器长度\t通道\t设备地址\t寄存器地址\t字节序\t寄存器名字\t寄存器描述\tK系数\tD系数\r\n
							u16Ret = fputs("\t\t\t", fpConfig);
							u16Ret = fputs(aName, fpConfig);
							
							/*
							memset((void *)aName, 0, MODBUS_REG_ATTR_MAX_LEN);
							u16Ret = fputs("\t", fpConfig);
							u16Ret = fputs(aName, fpConfig);
							*/
							
							memset((void *)aName, 0, MODBUS_REG_ATTR_MAX_LEN);
							sprintf(aName, "%d", (pChannel_List_Temp->u8Channel+1));
							u16Ret = fputs("\t\t\t", fpConfig);
							u16Ret = fputs(aName, fpConfig);//输入通道号
							
							memset((void *)aName, 0, MODBUS_REG_ATTR_MAX_LEN);
							sprintf(aName, "%d", pSlave_List_Temp->u8SlaveAddr);
							u16Ret = fputs("\t\t", fpConfig);
							u16Ret = fputs(aName, fpConfig); //输入设备地址
								
							sprintf(aName, "%d", pRegister_List_Temp->u16RegAddr);		
							u16Ret = fputs("\t\t\t", fpConfig);
							u16Ret = fputs(aName, fpConfig); //输入寄存器地址

							
							
//数据项地址\t寄存器长度\t通道\t设备地址\t寄存器地址\t字节序\t寄存器名字\t寄存器描述\tK系数\tD系数\t对象类型\t对象类型实例\t单位\r\n
							u16Ret = fputs("\t\t\t", fpConfig);
							u16Ret = fputs(pRegister_List_Temp->pBtOrder, fpConfig);//输入字节序
							
							u16Ret = fputs("\t\t", fpConfig);
							u16Ret = fputs(pRegister_List_Temp->pName, fpConfig);//输入寄存器名字

							u16Ret = fputs("\t\t", fpConfig);
							u16Ret = fputs(pRegister_List_Temp->pAttr, fpConfig);//输入寄存器属性

							memset((void *)aName, 0, MODBUS_REG_ATTR_MAX_LEN);
							u16Ret = fputs("\t\t", fpConfig);
							sprintf(aName, "%.2f", pRegister_List_Temp->fRegK);  //输入K值
							u16Ret = fputs(aName, fpConfig);

							memset((void *)aName, 0, MODBUS_REG_ATTR_MAX_LEN);
							u16Ret = fputs("\t", fpConfig);
							sprintf(aName, "%.2f", pRegister_List_Temp->fRegD);//输入D值
							u16Ret = fputs(aName, fpConfig);

							memset((void *)aName,0,MODBUS_REG_ATTR_MAX_LEN);
							u16Ret = fputs("\t\t",fpConfig);
							sprintf(aName,"%d",pRegister_List_Temp->object_type);//输入对象类型
							u16Ret = fputs(aName,fpConfig);

							memset((void *)aName,0,MODBUS_REG_ATTR_MAX_LEN);
							u16Ret = fputs("\t\t",fpConfig);
							sprintf(aName,"%d",pRegister_List_Temp->object_instance);//输入对象类型实例
							u16Ret = fputs(aName,fpConfig);

							memset((void *)aName,0,MODBUS_REG_ATTR_MAX_LEN);
							u16Ret = fputs("\t\t\t\t",fpConfig);
							sprintf(aName,"%d",pRegister_List_Temp->units);
							u16Ret = fputs(aName,fpConfig);//输入对象数据的单位

							memset((void *)aName,0,MODBUS_REG_ATTR_MAX_LEN);
							u16Ret = fputs("\r\n", fpConfig);

							ENNTRACE("%s, %d, aName = %s\n",__FUNCTION__,__LINE__, aName);
							//计算寄存器在虚拟缓存设备(buffer)中的起始地址位置
							u16LastRegAddr = u16LastRegAddr + pRegister_List_Temp->u16RegNum;  
							ENNTRACE("%d: 	u16LastRegAddr		= %XH\n",__LINE__,u16LastRegAddr);
							u8LastRegNum = pRegister_List_Temp->u16RegNum;

							ENNTRACE("%s, %d, u16Offset = %d, u16Count = %d\n",__FUNCTION__,__LINE__, u16Offset, pSlave_FunCode_Temp->u16Count);
							u16Offset = u16Offset + pRegister_List_Temp->u16RegNum;//计算偏移量
							
							pRegister_List_Temp = pRegister_List_Temp->next;
							
						}
						break;      //跳到下一个设备
					}
					pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
				}//break 
				pSlave_List_Temp = pSlave_List_Temp->next;
			}
		}
		
		u16TotalNum = u16Offset;        //从起始地址开始总的寄存器长度
		if(u8ChannelX < 7)
		{
			u8ChannelX++;
			pFunCode_List_Add->Offset[u8ChannelX] = u16Offset;     //每一个通道结束位置相对于其实位置的偏移量
			ENNTRACE("%s, %d,pFunCode_List_Add->Offset[%d]  = %d \n",__FUNCTION__,__LINE__,u8ChannelX,pFunCode_List_Add->Offset[u8ChannelX]);
		}
		ENNTRACE("MBFunCode = %d, Offset = %d\n",pFunCode_List_Add->u8MBFunCode, pFunCode_List_Add->Offset[u8ChannelX]);
#ifdef HYPTEK_DEBUG_PRINT
                printf("%s,%d,Test***** MBFunCode = %d, Offset[%d] = %d\n",__FUNCTION__,__LINE__,
                pFunCode_List_Add->u8MBFunCode, u8ChannelX, pFunCode_List_Add->Offset[u8ChannelX]);
#endif
		pChannel_List_Temp = pChannel_List_Temp->next;
	}

	if(0 != fclose(fpConfig))
	{
		free(pFunCode_List_Add);
		pFunCode_List_Add = NULL;
		perror("save configure file fail ");
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}

	if(!bFlag)
	{
		unlink(cFile); 
	}
	//ENNOS_DelayTask(1000);

	//pFunCode_List_Add->u16EndAddr = u16LastRegAddr - u8LastRegNum;
	pFunCode_List_Add->u16EndAddr = u16LastRegAddr;
	ENNTRACE("%s, %d, %d,%d,%d\n",__FUNCTION__,__LINE__,pFunCode_List_Add->u16EndAddr,pFunCode_List_Add->u16StartAddr,pFunCode_List_Add->u8MBFunCode);
	
	ENNTRACE("test: u16TotalNum = %d\n", u16TotalNum);
	if((MB_FUNC_READ_HOLDING_REGISTER == u8FunCode) 
             || (MB_FUNC_READ_INPUT_REGISTER == u8FunCode))
	//|| (MB_FUNC_WRITE_MULTIPLE_REGISTERS == u8FunCode)) 
	{
		u16DataLen = u16TotalNum * 2;	//二进制数据保存
	}
	else if((MB_FUNC_READ_COILS == u8FunCode) 
		 || (MB_FUNC_READ_DISCRETE_INPUTS == u8FunCode))
	{
		//u16DataLen = (u16TotalNum + (8 - u16TotalNum%8))/8;
		u16DataLen = u16TotalNum + 1; 	//字符串保存
	}
	else
	{
		goto FUNCODE_ADD;
	}
	
	pFunCode_List_Add->DataLen = u16DataLen;
	pFunCode_List_Add->pData = (ENN_U8 *)malloc(u16DataLen);
	if(NULL == pFunCode_List_Add->pData)
	{
		free(pFunCode_List_Add);
		pFunCode_List_Add = NULL;

		return ENN_FAIL;
	}
	if((MB_FUNC_READ_HOLDING_REGISTER == u8FunCode) || (MB_FUNC_READ_INPUT_REGISTER == u8FunCode))
	{
		memset((void *)pFunCode_List_Add->pData, 0, u16DataLen);
	}
	else if((MB_FUNC_READ_COILS == u8FunCode) || (MB_FUNC_READ_DISCRETE_INPUTS == u8FunCode))
	{
		for(i=0; i<u16DataLen-1; i++)
		{
			//pFunCode_List_Add->pData[i] = '1';
			pFunCode_List_Add->pData[i] = 0;
		}
		//memset((void *)pFunCode_List_Add->pData, '0', u16DataLen-1);
		pFunCode_List_Add->pData[i] = '\0';
		
		DEBUG_TRACE("%s, %d, %s\n",__FUNCTION__,__LINE__,pFunCode_List_Add->pData);
	}
	pFunCode_List_Add->pPos = pFunCode_List_Add->pData;
	ENNTRACE("test: u16DataLen = %d\n", u16DataLen);

FUNCODE_ADD:
	if(NULL == gFunCode_List_head)
	{
		gFunCode_List_head = pFunCode_List_Add;
	}
	else
	{
		pLast_FunCode_List->next = pFunCode_List_Add;
	}
	pLast_FunCode_List = pFunCode_List_Add;
	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	
	return ENN_SUCCESS;
}

ENN_ErrorCode_t ENNModBus_Build_File(void)
{
	ENN_CHAR *pBuf = NULL;
	ENN_CHAR *pStr = NULL;
	ENN_CHAR *pTmp = NULL;
	ENN_CHAR *pTmp1 = NULL;
	ENN_U8 u8FunCode = 0;
	ENN_U8 i = 0;
	ENN_U8 u8Num = 0;
	ENN_U8 u8Len = 0;
	ENN_ErrorCode_t ret = ENN_SUCCESS;

	pStr = ENNModBus_Get_Text("MAIN", "FUNCODE");   //FUNCODE={{2,0000},{3,0000},{4,0000},{5,0000},{16,0000}}
	if(NULL == pStr)
	{
		return ENN_FAIL;
	}
	pTmp = pStr;
	ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pTmp),pTmp);
	while('\0' != *pTmp)  //遍历
	{
		if('{' == *pTmp)
		{
			u8Num++;
		}
		pTmp++;
	}
	pTmp = pStr;
	u8Num--;        //获得有几个功能码
	while('{' != *pTmp)
	{
		pTmp++;
	}
	pTmp++;          // 指向{2,0000},{3,0000},{4,0000},{5,0000},{16,0000}}

	for(i=0; i<u8Num; i++)
	{
		u8Len = 0;
		pTmp = strchr(pTmp, '{');
		if(NULL == pTmp)
		{
			return ENN_FAIL;
		}
		pTmp++;       // 指向 2,0000},{3,0000},{4,0000},{5,0000},{16,0000}}

		pTmp1 = pTmp;
		while('}' != *pTmp1)
		{
			u8Len++;
			pTmp1++;
		}

		pBuf = (ENN_CHAR *)malloc(u8Len+1);
		if(NULL == pBuf)
		{
			return ENN_FAIL;
		}
		memset((void *)pBuf, 0, u8Len+1);
		memcpy((void *)pBuf, (void *)pTmp, u8Len);
		pTmp = pTmp1;
		
		ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,u8Num,pBuf);
		//2,0000	3,0000
		ret = ENNModBus_Add_FunCode(pBuf);
		free(pBuf);
		pBuf = NULL;
	}
	
	return ENN_SUCCESS;
}

ENN_ErrorCode_t ENNModBus_Read_Configure(void)
{
	ENN_CHAR	aBuffer[MODBUS_READ_FILE_BUF_LEN];
	ENN_CHAR	aSection[MODBUS_READ_FILE_BUF_LEN];
	ENN_CHAR	aName[MODBUS_READ_FILE_BUF_LEN];
	ENN_CHAR	aParam[MODBUS_READ_FILE_BUF_LEN];
	ENN_CHAR	*pStr1 = NULL;
	ENN_CHAR	*pStr2 = NULL;
	ENN_CHAR 	*cptr = NULL;
	FILE 	*fpConfig;
	ENN_ErrorCode_t ret = ENN_SUCCESS;

	fpConfig = fopen("/home/modbus.ini","r");
	//fpConfig = fopen("/home/ryan_modbus.ini","r");
	if(NULL == fpConfig)
	{
		perror("open configure file fail ");
		return ENN_FAIL;
	}

	while(0 == feof(fpConfig))  
	{
	    	memset(aBuffer, 0, MODBUS_READ_FILE_BUF_LEN);
	    	if(NULL == fgets(aBuffer, MODBUS_READ_FILE_BUF_LEN, fpConfig)) 
		{
            		break;
		}
		ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aBuffer),aBuffer);

		cptr = strchr(aBuffer,0x0d);
		if(cptr != NULL) 
		{
			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aBuffer),aBuffer);
			*cptr = '\0';
		}
		cptr = strchr(aBuffer,0x0a);
		if(cptr != NULL) 
		{
			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aBuffer),aBuffer);
			*cptr = '\0';
		}

		pStr1 = aBuffer; 
		ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pStr1),pStr1);
	        while((' ' == *pStr1) || ('\t' == *pStr1))
	    	{
	            pStr1++; 
	    	}

		if((0x0d == *pStr1) ||(0x0a == *pStr1))
		{
			continue;
		}
		
        	if('[' == *pStr1)  
	        {  
	            pStr1++;  
	            while((' ' == *pStr1) || ('\t' == *pStr1))
	            {
	                pStr1++;
	            }
				
	            pStr2 = pStr1;  
	            while((']' != *pStr1) && ('\0' != *pStr1))
	            {
	                pStr1++;
	            }
				
	            if('\0' == *pStr1)
	            {
	                //continue;
	                return ENN_FAIL;
	            }
				
	            while(' ' == *(pStr1-1))
	            {
	                pStr1--;
	            }
	            *pStr1 = '\0';  

			memset(aSection, 0, MODBUS_READ_FILE_BUF_LEN);
			strcpy(aSection, pStr2);
			ENNModBus_Set_Text(aSection, NULL, NULL);
		} 
		else if(('#' != *pStr1) && (*pStr1 > ' ')) // name identifier
		{
			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pStr1),pStr1);
			memset(aName, 0, MODBUS_READ_FILE_BUF_LEN);
			ret = ENNCopy_Name(aName, pStr1);
			if(ENN_SUCCESS != ret)
			{
				perror("get name fail ");
				return ENN_FAIL;
			}
			
			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aName),aName);
			memset(aParam, 0, MODBUS_READ_FILE_BUF_LEN);
			ret = ENNCopy_Param(aParam, pStr1);
			if(ENN_SUCCESS != ret)
			{
				printf("Test************%s, %d\n",__FUNCTION__,__LINE__);
				perror("get param fail ");
				return ENN_FAIL;
			}
			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aParam),aParam);
			ENNTRACE("%s, %d, %s, %d, %s, %d, %s\n",__FUNCTION__,__LINE__,aSection,strlen(aName),aName,strlen(aParam),aParam);
			ENNModBus_Set_Text(aSection, aName, aParam);
		}
		else
		{
			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pStr1),pStr1);
			ENNModBus_Set_Text(aSection, pStr1, NULL);
		}
    }      
	
    fclose(fpConfig);  
	
	//test_int();

	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Slave_Init(void)
{
	ENN_U8 u8FunCode = 0;
	ENN_U8 u8DataFormat = 0;
	ENN_U8 u8SlaveAddr = 0;
	ENN_U8 u8SlaveNum = 0;
	ENN_U8 u8ConFlag = 0;
	ENN_U8 u8ChannelNum = 0;
	ENN_U16 u16Len = 0;
	ENN_U8 i = 1;
	ENN_U8 j = 1;
	ENN_U8 u8Falg = 0;
	ENN_CHAR  *pFuncode = NULL;
	ENN_CHAR  *pSlave = NULL;
	ENN_CHAR  *pSlaveTmp = NULL;
	ENN_CHAR  *pCurrentPtr = NULL;
	ENN_CHAR  aSection[MODBUS_STRING_MAX_LEN]; 
	ENN_CHAR  aSlave[MODBUS_STRING_MAX_LEN];
	Channel_List *pChannel_List_Add = NULL;
	Channel_List *pChannel_List_Temp = NULL;
	Channel_List *pLast_Channel = NULL;
	Channel_List *pCurrent_Channel = NULL;
	Slave_List	 *pSlave_List_Add = NULL;
	Slave_List	 *pCurrent_Slave_List = NULL;
	Slave_List	 *pLast_Slave_List = NULL;
	Slave_FunCode_List *pSlave_FunCode_Add = NULL;
	Slave_FunCode_List *pLast_Slave_FunCode = NULL;
	xMBFunctionHandler *functionHandler = NULL;
	ENN_CHAR *pBuf = NULL;
	ENN_CHAR aCTmp[MODBUS_REG_MAX_NAME];
	ENN_CHAR *pTmp = NULL;
	ENN_ErrorCode_t ret = ENN_SUCCESS;
	ENN_U32 u32BaudRate = 0;
	ENN_U8 u8DataBit = 0;
	ENN_U8 u8StopBit = 0;
	ENN_U8 u8Prity = 0;
	ENN_CHAR *pParam = NULL;
	ENN_CHAR *pSlaveName = NULL;
	ENN_U8 u8SlaveNameLen = 0;
	Protocol_Type eProtocol_Type = PROTOCOL_MODBUS;
	Dev645_List *pDev645_List_Add = NULL;
	Dev645_List *pLast_Dev645_List = NULL;

	Register_List *p_Register_List;
	Code_645_List *p_645_List;

	pParam = ENNModBus_Get_Text("MAIN", "CHANNEL_NUM");   //获取通道数对应的参数
	if(NULL != pParam)
	{
		u8ChannelNum = atoi(pParam);
	}

	pFuncode = ENNModBus_Get_Text("MAIN", "FUNCODE");       //获取功能码对应的参数
	if(NULL == pFuncode)
	{
		return ENN_FAIL;
	};
	ENNTRACE("%s, %d, %d, %d, %s\n",__FUNCTION__,__LINE__,u8ConFlag, u8ChannelNum, pFuncode);

	for(i = 1; i <= u8ChannelNum; i++)
	{
		memset((void *)aSection, 0, MODBUS_STRING_MAX_LEN);
		sprintf(aSection, "CHANNEL_%d", i);

		eProtocol_Type = (Protocol_Type)atoi(ENNModBus_Get_Text(aSection, "PROTOCOL"));
		u8SlaveNum = atoi(ENNModBus_Get_Text(aSection, "SLAVE_NUM"));

		pParam = ENNModBus_Get_Text(aSection, "BAUDRATE");
		if(NULL != pParam)
		{
			u32BaudRate = atoi(pParam);
		}
		pParam = ENNModBus_Get_Text(aSection, "DATABITS");
		if(NULL != pParam)
		{
			u8DataBit = atoi(pParam);
		}
		pParam = ENNModBus_Get_Text(aSection, "STOPBITS");
		if(NULL != pParam)
		{
			u8StopBit = atoi(pParam);
		}

		pParam = ENNModBus_Get_Text(aSection, "PARITY");
		if(NULL != pParam)
		{
			if(0 == strcmp(pParam, MODBUS_PRITY_NONE))
			{
				u8Prity = PARITY_NONE;
			}
			else if(0 == strcmp(pParam, MODBUS_PRITY_ODD))
			{
				u8Prity = PARITY_ODD;
			}
			else if(0 == strcmp(pParam, MODBUS_PRITY_EVEN))
			{
				u8Prity = PARITY_EVEN;
			}
		}
		
		ENNTRACE("%s, %d, %s, %d\n",__FUNCTION__,__LINE__,aSection, u32BaudRate);
		
		pChannel_List_Add = (Channel_List *)malloc(sizeof(Channel_List));
		if(NULL == pChannel_List_Add)
		{
			return ENN_FAIL;
		}
		memset((void *)pChannel_List_Add, 0, sizeof(Channel_List));
		pChannel_List_Add->u8Channel = i - 1;
		pChannel_List_Add->u8Protocol = (ENN_U8)eProtocol_Type;
		pChannel_List_Add->u8Status = 0;
		pChannel_List_Add->u8SlaveNum = u8SlaveNum;
		pChannel_List_Add->u8DataBit = u8DataBit;
		pChannel_List_Add->u8StoptBit = u8StopBit;
		pChannel_List_Add->u8Parity = u8Prity;
		pChannel_List_Add->u32BaudRate = u32BaudRate;
		//pChannel_List_Add->pSlaveList = NULL;
		pChannel_List_Add->next = NULL;
		ENNTRACE("%s, %d, %d, %d\n",__FUNCTION__,__LINE__,pChannel_List_Add->u8Channel, pChannel_List_Add->u8SlaveNum);

		DevCount += u8SlaveNum;
		
		//pCurrent_Channel = gChannel_List_Head;
		if(NULL == gChannel_List_Head)
		{
			gChannel_List_Head = pChannel_List_Add;     //确定链表头指针
		}
		else
		{
			pLast_Channel->next = pChannel_List_Add;
		}
		pLast_Channel = pChannel_List_Add;   //链表位置针后移

		if(0 == u8SlaveNum)
		{
			continue;
		}

		//test_int();
		//return ENN_SUCCESS;

		if(PROTOCOL_MODBUS == eProtocol_Type)
		{
			pChannel_List_Add->unDeviceType.pModBus_List = NULL;
			for(j = 1; j <= u8SlaveNum; j++)
			{
				memset((void *)aSlave, 0, MODBUS_STRING_MAX_LEN);
				sprintf(aSlave, "SLAVE_%d", j);
				pSlave = ENNModBus_Get_Text(aSection, aSlave);
				printf("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pSlave), pSlave);

				//解析从站pSlave 指针指向字符串
				// SLAVE_1={20,电表,{3,0,{0000,1,电压,电压V,1.00,0.00,50,AB,1,2,2,8,0004,1,频率,频率HZ,1.00,0.00,50,AB,0,1,27}}}
				if(pSlave == NULL)
				{
					ENNTRACE("ERROR:%s, %d\n", __FUNCTION__, __LINE__);
					break;
				}
				if('{' != *pSlave)
				{
					//free memory
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					ENNTRACE("%s, %d\n", __FUNCTION__, __LINE__);
					return ENN_FAIL;
				}

				pSlaveTmp = pSlave;  //param的首地址进行临时转换
				pSlaveTmp++;    //指向设备地址 20,电表,{3,0,{0000,1,电压,电压V,1.00,0.00,50,AB,1,2,2,8,0004,1,频率,频率HZ,1.00,0.00,50,AB,0,1,27}}}
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				pTmp = &aCTmp;
				while(',' != *pSlaveTmp)
				{
					*(pTmp++) = *(pSlaveTmp++);
				}
				*pTmp = '\0';
				u8SlaveAddr = atoi(aCTmp);
				printf("%s, %d, %d\n",__FUNCTION__,__LINE__,u8SlaveAddr);
				pSlaveTmp++;    // 指向设备名称的第一个字符 电表,{3,0,{0000,1,电压,电压V,1.00,0.00,50
				/*add by sjc*/
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				pTmp = &aCTmp;
				u8SlaveNameLen = 0;
				while('\0' != *pSlaveTmp && ',' != *pSlaveTmp)
				{
					*(pTmp++) = *(pSlaveTmp++);
					u8SlaveNameLen++;
				}
				*pTmp = '\0';
				printf("%s, %d, %s\n",__FUNCTION__,__LINE__,aCTmp);

				pSlaveName = (ENN_CHAR *)malloc(strlen(aCTmp) + 1);
				if(NULL == pSlaveName)
				{
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

					return ENN_FAIL;
				}
				//printf("%s, %d, %s\n",__FUNCTION__,__LINE__,aCTmp);
				memset((void *)pSlaveName, 0, (strlen(aCTmp) + 1));
				strcpy((void *)pSlaveName, aCTmp);
				printf("%s, %d, name = %s, len = %d\n",__FUNCTION__,__LINE__, pSlaveName, strlen(pSlaveName));
				/*end*/
				pSlaveTmp = strchr(pSlaveTmp, '{');     //找到第二个{3,0,{0000,1,电压,电压V,1.00,0.00,50,AB,1,2,2,8,0004,1,频率,频率HZ,1.00,0.00,50,AB,0,1,27}}},
												
				if(NULL == pSlaveTmp)
				{
					//free memory
					free(pSlaveName);
					pSlaveName = NULL;
					//free(pChannel_List_Add);
					//pChannel_List_Add = NULL;
					printf("%s, %d\n",__FUNCTION__,__LINE__);

					//return ENN_FAIL;
					continue;
				}

				pSlave_List_Add = (Slave_List *)malloc(sizeof(Slave_List));
				if(NULL == pSlave_List_Add)
				{
					//free memory
					free(pSlaveName);
					pSlaveName = NULL;
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

					return ENN_FAIL;
				}
				memset((void *)pSlave_List_Add, 0, sizeof(Slave_List));
				//20,电表,{3,0,{0000,1,电压,电压V,1.00,0.00,50,AB,1,2,2,8,0004,1,频率,频率HZ,1.00,0.00,50,AB,0,1,27}}}
				pSlave_List_Add->u8SlaveAddr = u8SlaveAddr;
				pSlave_List_Add->pSlaveName = pSlaveName;
				pSlave_List_Add->u16Status = 0;
				pSlave_List_Add->pFunCode_List = NULL;
				pSlave_List_Add->next = NULL;

				if(NULL == pChannel_List_Add->unDeviceType.pModBus_List)
				{
					//pChannel_List_Add->pSlaveList = pSlave_List_Add;
					pChannel_List_Add->unDeviceType.pModBus_List = pSlave_List_Add;
				}
				else
				{
					pLast_Slave_List->next = pSlave_List_Add;
				}
				pLast_Slave_List = pSlave_List_Add;
				//{3,0,{0000,1,电压,电压V,1.00,0.00,50,AB,1,2,2,8,0004,1,频率,频率HZ,1.00,0.00,50,AB,0,1,27}}}
				while(NULL != (pSlaveTmp = strchr(pSlaveTmp, '{')))//功能码集合的循环中，因为一个设备不一定只有一个功能码
				{
					ENNTRACE("%s, %d, %d, %d, %s\n",__FUNCTION__,__LINE__,u16Len, strlen(pSlaveTmp), pSlaveTmp);
					u16Len = 0;
					pSlaveTmp++;        //指向功能码//{3,0,{0000,1,电压,电压V,1.00,0.00,50,AB
					memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
					pTmp = &aCTmp;
					while(',' != *pSlaveTmp)
					{
						*(pTmp++) = *(pSlaveTmp++);
					}
					*pTmp = '\0';
					u8FunCode = atoi(aCTmp);
					ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u8FunCode);

					if((MB_FUNC_READ_HOLDING_REGISTER == u8FunCode) ||(MB_FUNC_READ_INPUT_REGISTER == u8FunCode))
					{
						pSlaveTmp++; //指向功能码后面的数据格式
						memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
						pTmp = &aCTmp;
						while(',' != *pSlaveTmp)
						{
							*(pTmp++) = *(pSlaveTmp++);
						}
						*pTmp = '\0';
						u8DataFormat = atoi(aCTmp);
						pSlave_List_Add->u8DataFormat = u8DataFormat;
					}
					ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u8FunCode);

					functionHandler = GetFuncHandler(u8FunCode);
					if(NULL == functionHandler)
					{
						//free memory
						free(pSlaveName);
						pSlaveName = NULL;
						free(pChannel_List_Add);
						pChannel_List_Add = NULL;
						ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

						return ENN_FAIL;
					}

					pSlave_FunCode_Add = (Slave_FunCode_List *)malloc(sizeof(Slave_FunCode_List));
					if(NULL == pSlave_FunCode_Add)
					{
						//free memory
						free(pSlaveName);
						pSlaveName = NULL;
						free(pChannel_List_Add);
						pChannel_List_Add = NULL;
						ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

						return ENN_FAIL;
					}
					memset((void *)pSlave_FunCode_Add, 0, sizeof(Slave_FunCode_List));
					pSlave_FunCode_Add->u8MBFunCode = u8FunCode;
					pSlave_FunCode_Add->pRegister_List = NULL;
					pSlave_FunCode_Add->next = NULL;

					if(NULL == pSlave_List_Add->pFunCode_List)
					{
						pSlave_List_Add->pFunCode_List = pSlave_FunCode_Add;
						//pLast_Slave_FunCode = pSlave_FunCode_Add;
					}
					else
					{
						pLast_Slave_FunCode->next = pSlave_FunCode_Add;
						//pLast_Slave_FunCode = pSlave_FunCode_Add;
					}
					pLast_Slave_FunCode = pSlave_FunCode_Add;

					ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pSlaveTmp), pSlaveTmp);
					pSlaveTmp = strchr(pSlaveTmp, '{');  
					pSlaveTmp++;     	//point to register 0000,1,电压,电压V,1.00,0.00,50,AB,OBJECT_ANALOG_VALUE,UNITS_VOLT_AMPERES,1,
												//0004,1,频率,频率HZ,1.00,0.00,50,AB,OBJECT_ANALOG_INPUT,UNITS_HERTZ,2}}}
					while('}' != *pSlaveTmp)        //获取{}内数据的长度，pSlaveTmp最后指向}
					{
						u16Len++;
						pSlaveTmp++;             
					}
					ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pSlaveTmp), pSlaveTmp);
					pSlaveTmp -= u16Len; //pSlaveTmp最后指向0000,1,JWST1-TEMP,JWST1-TEMP,1.00
					u16Len++;                     //为什么要加1 呢?加上}
					ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pSlaveTmp), pSlaveTmp);

					pBuf = (ENN_CHAR *)malloc(u16Len + 1);
					if(NULL == pBuf)
					{
						//free memory  1234
						free(pSlaveName);
						pSlaveName = NULL;
						free(pChannel_List_Add);
						pChannel_List_Add = NULL;
						ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
						return ENN_FAIL;
					}
					memset((void *)pBuf, 0, u16Len + 1);
					memcpy((void *)pBuf, (void *)pSlaveTmp, u16Len); //从数据到}都复制进入pBuf
					
					ENNTRACE("%s, %d, u16Len = %d\n",__FUNCTION__,__LINE__,u16Len);
					//*(pBuf+u16Len+1) = '\0';
					//ENNTRACE("%d, %s\n",strlen(pBuf), pBuf);
					u16Len--;
					pSlaveTmp += u16Len;    			//指向右边第一个}
					ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pSlaveTmp), pSlaveTmp);

					ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pBuf), pBuf);
					if(0x04 == u8FunCode)
					{
						printf("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pBuf), pBuf);
					}
					//0000,1,电压有效数据,Val,0001,1,...}
					ret = functionHandler->functionHandler(pBuf, pSlave_FunCode_Add);
					//printf("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pBuf), pBuf);
					if(ENN_SUCCESS != ret)
					{
						ENNTRACE("[%s, %d]    error\n",__FUNCTION__,__LINE__);
						free(pBuf);
						pBuf = NULL;
						return ENN_FAIL;
					}

					free(pBuf);
					pBuf = NULL;
				}
			}
		}
		else
		{
			free(pChannel_List_Add);
			pChannel_List_Add = NULL;
			ENNTRACE("%s, %d\n", __FUNCTION__, __LINE__);
			return ENN_FAIL;
		}
	}

	printf("[%s], %d, DevRegNum_Total = %d\n",__FUNCTION__,__LINE__,DevRegNum_Total);

	//test_int();
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Channel_Init(void)
{
	ENN_U8 u8FunCode = 0;
	ENN_U8 u8DataFormat = 0;
	ENN_U8 u8SlaveAddr = 0;
	ENN_U8 u8SlaveNum = 0;
	ENN_U8 u8ConFlag = 0;
	ENN_U8 u8ChannelNum = 0;
	ENN_U16 u16Len = 0;
	ENN_U8 i = 1;
	ENN_U8 j = 1;
	ENN_U8 u8Falg = 0;
	ENN_CHAR  *pFuncode = NULL;
	ENN_CHAR  *pSlave = NULL;
	ENN_CHAR  *pSlaveTmp = NULL;
	ENN_CHAR  *pCurrentPtr = NULL;
	ENN_CHAR  aSection[MODBUS_STRING_MAX_LEN]; 
	ENN_CHAR  aSlave[MODBUS_STRING_MAX_LEN];
	Channel_List *pChannel_List_Add = NULL;
	Channel_List *pChannel_List_Temp = NULL;
	Channel_List *pLast_Channel = NULL;
	Channel_List *pCurrent_Channel = NULL;
	Slave_List	 *pSlave_List_Add = NULL;
	Slave_List	 *pCurrent_Slave_List = NULL;
	Slave_List	 *pLast_Slave_List = NULL;
	Slave_FunCode_List *pSlave_FunCode_Add = NULL;
	Slave_FunCode_List *pLast_Slave_FunCode = NULL;
	xMBFunctionHandler *functionHandler = NULL;
	ENN_CHAR *pBuf = NULL;
	ENN_CHAR aCTmp[MODBUS_REG_MAX_NAME];
	ENN_CHAR *pTmp = NULL;
	ENN_ErrorCode_t ret = ENN_SUCCESS;
	ENN_U32 u32BaudRate = 0;
	ENN_U8 u8DataBit = 0;
	ENN_U8 u8StopBit = 0;
	ENN_U8 u8Prity = 0;
	ENN_CHAR *pParam = NULL;
	ENN_CHAR *pSlaveName = NULL;
	ENN_U8 u8SlaveNameLen = 0;
	Protocol_Type eProtocol_Type = PROTOCOL_MODBUS;
	Dev645_List *pDev645_List_Add = NULL;
	Dev645_List *pLast_Dev645_List = NULL;
	
	pParam = ENNModBus_Get_Text("MAIN", "CHANNEL_NUM");
	if(NULL != pParam)
	{
		u8ChannelNum = atoi(pParam);
	}
	
	for(i = 1; i <= u8ChannelNum; i++)
	{
		memset((void *)aSection, 0, MODBUS_STRING_MAX_LEN);
		sprintf(aSection, "CHANNEL_%d", i);

		eProtocol_Type = (Protocol_Type)atoi(ENNModBus_Get_Text(aSection, "PROTOCOL"));
		u8SlaveNum = atoi(ENNModBus_Get_Text(aSection, "SLAVE_NUM"));

		pParam = ENNModBus_Get_Text(aSection, "BAUDRATE");
		if(NULL != pParam)
		{
			u32BaudRate = atoi(pParam);
		}
		pParam = ENNModBus_Get_Text(aSection, "DATABITS");
		if(NULL != pParam)
		{
			u8DataBit = atoi(pParam);
		}
		pParam = ENNModBus_Get_Text(aSection, "STOPBITS");
		if(NULL != pParam)
		{
			u8StopBit = atoi(pParam);
		}

		pParam = ENNModBus_Get_Text(aSection, "PARITY");
		if(NULL != pParam)
		{
			if(0 == strcmp(pParam, MODBUS_PRITY_NONE))
			{
				u8Prity = PARITY_NONE;
			}
			else if(0 == strcmp(pParam, MODBUS_PRITY_ODD))
			{
				u8Prity = PARITY_ODD;
			}
			else if(0 == strcmp(pParam, MODBUS_PRITY_EVEN))
			{
				u8Prity = PARITY_EVEN;
			}
		}
		
		ENNTRACE("%s, %d, %s, %d\n",__FUNCTION__,__LINE__,aSection, u32BaudRate);
		
		pChannel_List_Add = (Channel_List *)malloc(sizeof(Channel_List));
		if(NULL == pChannel_List_Add)
		{
			return ENN_FAIL;
		}
		memset((void *)pChannel_List_Add, 0, sizeof(Channel_List));
		pChannel_List_Add->u8Channel = i - 1;
		pChannel_List_Add->u8Protocol = (ENN_U8)eProtocol_Type;
		pChannel_List_Add->u8Status = 0;
		pChannel_List_Add->u8SlaveNum = u8SlaveNum;
		pChannel_List_Add->u32BaudRate = u32BaudRate;
		pChannel_List_Add->u8DataBit = u8DataBit;
		pChannel_List_Add->u8StoptBit = u8StopBit;
		pChannel_List_Add->u8Parity = u8Prity;
		//pChannel_List_Add->pSlaveList = NULL;
		pChannel_List_Add->next = NULL;
		ENNTRACE("%s, %d, %d, %d\n",__FUNCTION__,__LINE__,pChannel_List_Add->u8Channel, pChannel_List_Add->u8SlaveNum);
		
		if(NULL == gSlave_Set_List_Head)
		{
			gSlave_Set_List_Head = pChannel_List_Add;
		}
		else
		{
			pLast_Channel->next = pChannel_List_Add;
		}
		pLast_Channel = pChannel_List_Add;

		if(0 == u8SlaveNum)
		{
			continue;
		}

		if(PROTOCOL_MODBUS == eProtocol_Type)
		{
			pChannel_List_Add->unDeviceType.pModBus_List = NULL;
			for(j = 1; j <= u8SlaveNum; j++)
			{
				memset((void *)aSlave, 0, MODBUS_STRING_MAX_LEN);
				sprintf(aSlave, "SLAVE_%d", j);
				pSlave = ENNModBus_Get_Text(aSection, aSlave);
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pSlave), pSlave);

				//解析从站pSlave 指针指向字符串
				if('{' != *pSlave)
				{
					//free memory
					
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}

				pSlaveTmp = pSlave;
				pSlaveTmp++;
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				pTmp = &aCTmp;
				while(',' != *pSlaveTmp)
				{
					*(pTmp++) = *(pSlaveTmp++);
				}
				*pTmp = '\0';
				u8SlaveAddr = atoi(aCTmp);
				//ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u8SlaveAddr);
				pSlaveTmp++;

				/*add by sjc*/
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				pTmp = &aCTmp;
				u8SlaveNameLen = 0;
				while('\0' != *pSlaveTmp && ',' != *pSlaveTmp)
				{
					*(pTmp++) = *(pSlaveTmp++);
					u8SlaveNameLen++;
				}
				*pTmp = '\0';

				pSlaveName = (ENN_CHAR *)malloc(strlen(aCTmp) + 1);
				if(NULL == pSlaveName)
				{
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

					return ENN_FAIL;
				}
				memset((void *)pSlaveName, 0, (strlen(aCTmp) + 1));
				strcpy((void *)pSlaveName, aCTmp);
				printf("%s, %d, name = %s, len = %d\n",__FUNCTION__,__LINE__, pSlaveName, strlen(pSlaveName));
				/*end*/
				
				pSlaveTmp = strchr(pSlaveTmp, '{');
				if(NULL == pSlaveTmp)
				{
					//free memory
					free(pSlaveName);
					pSlaveName = NULL;
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

					//return ENN_FAIL;
					continue;
				}
				//pSlaveTmp[strlen(pSlaveTmp)+1] = '\0';

				pSlave_List_Add = (Slave_List *)malloc(sizeof(Slave_List));
				if(NULL == pSlave_List_Add)
				{
					//free memory
					
					free(pSlaveName);
					pSlaveName = NULL;
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

					return ENN_FAIL;
				}
				memset((void *)pSlave_List_Add, 0, sizeof(Slave_List));
				pSlave_List_Add->u8SlaveAddr = u8SlaveAddr;
				pSlave_List_Add->pSlaveName = pSlaveName;
				pSlave_List_Add->pFunCode_List = NULL;
				pSlave_List_Add->next = NULL;

				if(NULL == pChannel_List_Add->unDeviceType.pModBus_List)
				{
					pChannel_List_Add->unDeviceType.pModBus_List = pSlave_List_Add;
					//pLast_Slave_List = pSlave_List_Add;
				}
				else
				{
					pLast_Slave_List->next = pSlave_List_Add;
					//pLast_Slave_List = pSlave_List_Add;
				}
				pLast_Slave_List = pSlave_List_Add;

				while(NULL != (pSlaveTmp = strchr(pSlaveTmp, '{')))
				{
					ENNTRACE("%s, %d, %d, %d, %s\n",__FUNCTION__,__LINE__,u16Len, strlen(pSlaveTmp), pSlaveTmp);
					u16Len = 0;
					pSlaveTmp++;
					memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
					pTmp = &aCTmp;
					while(',' != *pSlaveTmp)
					{
						*(pTmp++) = *(pSlaveTmp++);
					}
					*pTmp = '\0';
					u8FunCode = atoi(aCTmp);
					ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u8FunCode);

					if((MB_FUNC_READ_HOLDING_REGISTER == u8FunCode) ||(MB_FUNC_READ_INPUT_REGISTER == u8FunCode))
					{
						pSlaveTmp++;
						memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
						pTmp = &aCTmp;
						while(',' != *pSlaveTmp)
						{
							*(pTmp++) = *(pSlaveTmp++);
						}
						*pTmp = '\0';
						u8DataFormat = atoi(aCTmp);
						pSlave_List_Add->u8DataFormat = u8DataFormat;
					}
					ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u8FunCode);

					functionHandler = GetFuncHandler(u8FunCode);
					if(NULL == functionHandler)
					{
						//free memory
						
						free(pSlaveName);
						pSlaveName = NULL;
						free(pChannel_List_Add);
						pChannel_List_Add = NULL;
						ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

						return ENN_FAIL;
					}

					pSlave_FunCode_Add = (Slave_FunCode_List *)malloc(sizeof(Slave_FunCode_List));
					if(NULL == pSlave_FunCode_Add)
					{
						//free memory
						
						free(pSlaveName);
						pSlaveName = NULL;
						free(pChannel_List_Add);
						pChannel_List_Add = NULL;
						ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

						return ENN_FAIL;
					}
					memset((void *)pSlave_FunCode_Add, 0, sizeof(Slave_FunCode_List));
					pSlave_FunCode_Add->u8MBFunCode = u8FunCode;
					pSlave_FunCode_Add->pRegister_List = NULL;
					pSlave_FunCode_Add->next = NULL;

					if(NULL == pSlave_List_Add->pFunCode_List)
					{
						pSlave_List_Add->pFunCode_List = pSlave_FunCode_Add;
						//pLast_Slave_FunCode = pSlave_FunCode_Add;
					}
					else
					{
						pLast_Slave_FunCode->next = pSlave_FunCode_Add;
						//pLast_Slave_FunCode = pSlave_FunCode_Add;
					}
					pLast_Slave_FunCode = pSlave_FunCode_Add;

					ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pSlaveTmp), pSlaveTmp);
					pSlaveTmp = strchr(pSlaveTmp, '{');
					pSlaveTmp++;
					while('}' != *pSlaveTmp)
					{
						u16Len++;
						pSlaveTmp++;
					}
					ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pSlaveTmp), pSlaveTmp);
					pSlaveTmp -= u16Len;
					u16Len++;
					ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pSlaveTmp), pSlaveTmp);

					pBuf = (ENN_CHAR *)malloc(u16Len + 1);
					if(NULL == pBuf)
					{
						//free memory  1234
						
						free(pSlaveName);
						pSlaveName = NULL;
						free(pChannel_List_Add);
						pChannel_List_Add = NULL;
						ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
						return ENN_FAIL;
					}
					memset((void *)pBuf, 0, u16Len + 1);
					memcpy((void *)pBuf, (void *)pSlaveTmp, u16Len);
					
					ENNTRACE("%s, %d, u16Len = %d\n",__FUNCTION__,__LINE__,u16Len);
					//*(pBuf+u16Len+1) = '\0';
					//ENNTRACE("%d, %s\n",strlen(pBuf), pBuf);
					u16Len--;
					pSlaveTmp += u16Len;
					ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pSlaveTmp), pSlaveTmp);

					ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pBuf), pBuf);
					//if(0x04 == u8FunCode)
					//{
						//printf("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pBuf), pBuf);
					//}
					ret = functionHandler->functionHandler(pBuf, pSlave_FunCode_Add);
					if(ENN_SUCCESS != ret)
					{
						ENNTRACE("[%s, %d]    error\n",__FUNCTION__,__LINE__);
						free(pBuf);
						pBuf = NULL;
						return ENN_FAIL;
					}

					free(pBuf);
					pBuf = NULL;
				}
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pSlaveTmp), pSlaveTmp);
			}
		}
		else if((PROTOCOL_645_1997 == eProtocol_Type) || (PROTOCOL_645_2007 == eProtocol_Type))
		{
			pChannel_List_Add->unDeviceType.pDev645_List = NULL;
			for(j = 1; j <= u8SlaveNum; j++)
			{
				memset((void *)aSlave, 0, MODBUS_STRING_MAX_LEN);
				sprintf(aSlave, "SLAVE_%d", j);
				pSlave = ENNModBus_Get_Text(aSection, aSlave);
				ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__, strlen(pSlave), pSlave);

				//解析从站pSlave 指针指向字符串
				if('{' != *pSlave)
				{
					//free memory
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					ENNTRACE("%s, %d\n", __FUNCTION__, __LINE__);
					return ENN_FAIL;
				}

				pSlaveTmp = pSlave;
				pSlaveTmp++;
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				pTmp = &aCTmp;
				while(',' != *pSlaveTmp)
				{
					*(pTmp++) = *(pSlaveTmp++);
				}
				*pTmp = '\0';
				//u8SlaveAddr = atoi(aCTmp);
				//ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u8SlaveAddr);
				pSlaveTmp++;

				pDev645_List_Add = (Dev645_List *)malloc(sizeof(Dev645_List));
				if((NULL == pDev645_List_Add) || (16 - 1) < (strlen(aCTmp)))
				{
					//free memory
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				memset((void *)pDev645_List_Add, 0, sizeof(Dev645_List));
				memset((void *)pDev645_List_Add->ucDevAddr, 0, 16);
				pDev645_List_Add->pSlaveName = NULL;
				pDev645_List_Add->u16Status = 0;
				pDev645_List_Add->u8ChannelNum = 0;
				pDev645_List_Add->pCode_645_List = NULL;
				pDev645_List_Add->next = NULL;
				
				memcpy((void *)pDev645_List_Add->ucDevAddr, (void *)aCTmp, strlen(aCTmp));
				
				/*add by sjc*/
				memset((void *)aCTmp, 0, MODBUS_REG_MAX_NAME);
				pTmp = &aCTmp;
				u8SlaveNameLen = 0;
				while(',' != *pSlaveTmp)
				{
					*(pTmp++) = *(pSlaveTmp++);
					u8SlaveNameLen++;
				}
				*pTmp = '\0';

				pSlaveName = (ENN_CHAR *)malloc(strlen(aCTmp) + 1);
				if(NULL == pSlaveName)
				{
					free(pDev645_List_Add);
					pDev645_List_Add = NULL;
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				memset((void *)pSlaveName, 0, (strlen(aCTmp) + 1));
				strcpy((void *)pSlaveName, aCTmp);
				printf("%s, %d, name = %s, len = %d\n",__FUNCTION__,__LINE__, pSlaveName, strlen(pSlaveName));
				/*end*/
				pDev645_List_Add->pSlaveName = pSlaveName;
				
				pSlaveTmp = strchr(pSlaveTmp, '{');
				if(NULL == pSlaveTmp)
				{
					//free memory
					free(pSlaveName);
					pSlaveName = NULL;
					free(pDev645_List_Add);
					pDev645_List_Add = NULL;
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				//pSlaveTmp[strlen(pSlaveTmp)+1] = '\0';

				if(NULL == pChannel_List_Add->unDeviceType.pDev645_List)
				{
					pChannel_List_Add->unDeviceType.pDev645_List = pDev645_List_Add;
				}
				else
				{
					pLast_Dev645_List->next = pDev645_List_Add;
				}
				pLast_Dev645_List = pDev645_List_Add;

				pSlaveTmp++;
				ret = eMBFunc_Read_645(pSlaveTmp, pDev645_List_Add);
				if(ENN_SUCCESS != ret)
				{
					free(pSlaveName);
					pSlaveName = NULL;
					free(pDev645_List_Add);
					pDev645_List_Add = NULL;
					free(pChannel_List_Add);
					pChannel_List_Add = NULL;
					return ret;
				}
				
				//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pSlaveTmp), pSlaveTmp);
			}
		}
		else
		{
			free(pChannel_List_Add);
			pChannel_List_Add = NULL;
			ENNTRACE("%s, %d\n", __FUNCTION__, __LINE__);
			return ENN_FAIL;
		}
	}

	return ENN_SUCCESS;
}

ENN_ErrorCode_t ENNModBus_Write(ENN_U8 Channel, ENN_U8 SlaveAddr, ENN_U8 FunCode, 
										     const ENN_CHAR *data, ENN_U8 len)
{
	ENN_S32 ret = 0;
	ENN_U32 u32len;
	ENN_U32 i;
	ENN_U16 u16CRC;
	//ENN_U8 tel[256];
	ENN_U8 *tel;
	
	u32len = 0;

	tel = (ENN_U8 *)malloc(len + 1 + 1 + 2);
	if(NULL == tel)
	{
		return ENN_FAIL;
	}
	memset((void *)tel, 0, (len + 1 + 1 + 2));
		
	tel[u32len++] = (ENN_U8)SlaveAddr;
	tel[u32len++] = (ENN_U8)FunCode;
	for(i=0; i<len; i++)
	{
		tel[u32len++] = data[i];
	}
	u16CRC = CRC16(tel, u32len);
	DEBUG_TRACE("u16CRC = %x \n", u16CRC);
	ENNWRITETRACE("u16CRC = %x \n", u16CRC);
	
	tel[u32len++] = (ENN_U8)(u16CRC & 0x00FF);
	tel[u32len++] = (ENN_U8)(u16CRC >> 8);
	DEBUG_TRACE("%s, %d, u32len = %d\n", __FUNCTION__, __LINE__, u32len);
	if(FunCode == 3 && (Channel ==5))
	{
		DEBUG_TRACE("[%s],%d,u8Channelx[%d] ,data:   ",__FUNCTION__,__LINE__,Channel);
		for(i=0; i<u32len; i++)
		{
			DEBUG_TRACE("%X ", tel[i]);
			ENNWRITETRACE("%X ", tel[i]);	
		}
		DEBUG_TRACE("\n");
	}
	ENNWRITETRACE("\n");	

	ret = UART_Write(Channel, tel, u32len);
	if(ret < 0)
	{
		free(tel);
		return ENN_FAIL;
	}
	free(tel);

	return ENN_SUCCESS;
}

ENN_ErrorCode_t ENNModBus_Request(ENN_U8 Channel, ENN_U8 SlaveAddr, ENN_U8 FunCode, ENN_U16 RegAddr, ENN_U16 Num)
{
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	//int rlModbus::request(int slave, int function, int start_adr, int num_register)
	ENN_CHAR data[4];
	
	data[0] = (unsigned char) ( RegAddr / 256 );
	data[1] = (unsigned char) ( RegAddr & 0x0ff );
	data[2] = (unsigned char) ( Num / 256 ); 
	data[3] = (unsigned char) ( Num & 0x0ff );
	returnCode = ENNModBus_Write(Channel, SlaveAddr, FunCode, data, 4);
	
	return returnCode;
}


ENN_ErrorCode_t ENNModBus_Sleep(ENN_U8 IdelTime)
{
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	fd_set fs_read;
	fd_set fs_write;
	fd_set fs_except;
	struct timeval tv_timeout;
	ENN_S32 iRet;
	
	FD_ZERO(&fs_read);
    FD_ZERO(&fs_write);
  	FD_ZERO(&fs_except);

    tv_timeout.tv_sec = IdelTime/1000;
    tv_timeout.tv_usec = (IdelTime%1000) * 1000;
    iRet = ENNSock_Select(1, &fs_read, &fs_write, &fs_except, &tv_timeout);

	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Select(ENN_U8 u8Channelx, ENN_U8 IdelTime)
{
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	fd_set fs_read;
	struct timeval tv_timeout;
	ENN_S32 iRet;
	ENN_S32 CHANNELX_Handle = 0;
	
	FD_ZERO(&fs_read);
	switch(u8Channelx)
	{
		case 0:
			CHANNELX_Handle = gChannel1_Handle;
			break;
		case 1:
			CHANNELX_Handle = gChannel2_Handle;
			break;
		case 2:
			CHANNELX_Handle = gChannel3_Handle;
			break;
		case 3:
			CHANNELX_Handle = gChannel4_Handle;
			break;
		case 4:
			CHANNELX_Handle = gChannel5_Handle;
			break;
		case 5:
			CHANNELX_Handle = gChannel6_Handle;
			break;
		case 6:
			CHANNELX_Handle = gChannel7_Handle;
			break;
		case 7:
			CHANNELX_Handle = gChannel8_Handle;
			break;

		default:
			break;
	}
    FD_SET (CHANNELX_Handle, &fs_read);

    tv_timeout.tv_sec = IdelTime/1000;
    tv_timeout.tv_usec = (IdelTime%1000) * 1000;
    iRet = ENNSock_Select(CHANNELX_Handle + 1, &fs_read, NULL, NULL, &tv_timeout);
    if(iRet > 0)
    {
		return ENN_SUCCESS;
    }
    else
    {
		return ENN_FAIL;
    }
}


ENN_S32 ENNModBus_ReadChar(ENN_U8 u8Channelx)
{
	ENN_S32  ret;
	ENN_U8 buf[2];

	ret = UART_Read(u8Channelx, buf, 1);

	if(ret == 1) return buf[0];
	if(ret == 0) return -2;
	return -1;
}

ENN_S32 ENNModBus_ReadBlock(ENN_U8 u8Channelx, ENN_U8 *buf, ENN_U32 len, ENN_U32 timeout)
{
	ENN_U32 c, retlen;
	ENN_U32 i = 0;

	retlen = 0;
	for(i=0; i<len; i++)
	{
		if(timeout >= 0)
		{
			if(ENN_SUCCESS != ENNModBus_Select(u8Channelx, timeout)) 
			{
				break; // timeout
			}
		}
		c = ENNModBus_ReadChar(u8Channelx);
		if(c < 0) return c;
		buf[i] = (ENN_U8) c;
		retlen = i+1;
	}
	if(retlen <= 0) return -1;
	return retlen;
}


ENN_U32 ENNModBus_Response(ENN_U8 u8Channelx, ENN_U8 *slave, ENN_U8 *function, ENN_U8 *data)
{
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	ENN_S32 s32Len = 0;
	ENN_U32 len = 0;
	ENN_U8 aReadData[512];
	ENN_U8 byte_count = 0;
	ENN_U16 u16CRC1 = 0;
	ENN_U16 u16CRC2 = 0;
	ENN_U8 i = 0;

	ENNAPI_ASSERT(NULL != data);

	returnCode = ENNModBus_Select(u8Channelx, 1000);
	ENNWRITETRACE("%s, %d, returnCode = %d\n",__FUNCTION__,__LINE__, returnCode);
	if(ENN_SUCCESS != returnCode)
	{
		return 0;
	}

	if(ENNModBus_ReadBlock(u8Channelx, aReadData, 2, 1000) <= 0)
	{
		return 0;
	}
	*slave     = aReadData[len++];
	*function  = aReadData[len++];
	ENNWRITETRACE("%s, %d, slave = %d, function = %d\n",__FUNCTION__,__LINE__,*slave, *function);
	switch(*function)
	{
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
			returnCode = ENNModBus_Select(u8Channelx, 1000);
			if(ENN_SUCCESS != returnCode)
			{
				return 0;
			}
			if(ENNModBus_ReadBlock(u8Channelx, &aReadData[len], 1, 1000) <= 0)
			{
				return 0;
			}
			byte_count = aReadData[len++];
			DEBUG_TRACE("%s, %d, byte_count = %d\n",__FUNCTION__,__LINE__,byte_count);
			ENNWRITETRACE("%s, %d, byte_count = %d\n",__FUNCTION__,__LINE__,byte_count);
			returnCode = ENNModBus_Select(u8Channelx, 1000);
			if(ENN_SUCCESS != returnCode)
			{
				return 0;
			}

			s32Len = ENNModBus_ReadBlock(u8Channelx, data, byte_count+2, 2000);
			ENNTRACE("%s, %d, s32Len = %d\n",__FUNCTION__,__LINE__,s32Len);
			//if(ENNModBus_ReadBlock(u8Channelx, data, byte_count+2, 1000) <= 0)
			if((s32Len <= 0) && (s32Len != (byte_count+2)))
			{
				return 0;
			}
			memcpy(&aReadData[len], data, byte_count+2);
			len += byte_count + 2;
			DEBUG_TRACE("len = %d\n", len);
			for(i=0; i<len; i++)
			{
				DEBUG_TRACE("%s, %d, %x \n",__FUNCTION__,__LINE__,aReadData[i]);
			}
			DEBUG_TRACE("\n");
			u16CRC1 = CRC16(aReadData, len-2);
			ENNTRACE("%s, %d, %x, %x\n",__FUNCTION__,__LINE__,aReadData[len-1], aReadData[len-2]);
			u16CRC2 = (ENN_U16)(aReadData[len-1]) << 8;
			ENNTRACE("%s, %d, u16CRC2 = %x\n",__FUNCTION__,__LINE__,u16CRC2);
			u16CRC2 |= (ENN_U16)(aReadData[len-2]);
			ENNTRACE("%s, %d, %x, %x\n",__FUNCTION__,__LINE__,u16CRC1, u16CRC2);
			if(u16CRC1 != u16CRC2)
			{
				return 0;
			}
			return byte_count;
		case 0x05:
		case 0x06:
		case 0x10:
			returnCode = ENNModBus_Select(u8Channelx, 1000);
			if(ENN_SUCCESS != returnCode)
			{
				return 0;
			}
			if(ENNModBus_ReadBlock(u8Channelx, data, 6, 1000) <= 0)
			{
				return 0;
			}
			memcpy(&aReadData[len], data, 6);
			len += 6;

			ENNWRITETRACE("len = %d\n", len);
			for(i=0; i<len; i++)
			{
				ENNWRITETRACE("%x ",aReadData[i]);
			}
			ENNWRITETRACE("\n");

			u16CRC1 = CRC16(aReadData, len-2);
			ENNWRITETRACE("%s, %d, %x, %x\n",__FUNCTION__,__LINE__,aReadData[len-1], aReadData[len-2]);
			u16CRC2 = (ENN_U16)(aReadData[len-1]) << 8;
			ENNWRITETRACE("%s, %d, u16CRC2 = %x\n",__FUNCTION__,__LINE__,u16CRC2);
			u16CRC2 |= (ENN_U16)(aReadData[len-2]);
			ENNWRITETRACE("%s, %d, %x, %x\n",__FUNCTION__,__LINE__,u16CRC1, u16CRC2);
			if(u16CRC1 != u16CRC2)
			{
				return 0;
			}
			return len;
		default:
			return 0;
	}

#if 0
	memset((void *)aReadData, 0, 512);
	s32Len = UART_Read(u8Channelx, aReadData, 512);
	if(s32Len <= 2)
	{
		return 0;
	}
	ENNTRACE("s32Len = %d\n", s32Len);
	for(i=0; i<s32Len; i++)
	{
		ENNTRACE("%x ",aReadData[i]);
	}
	ENNTRACE("\n");
	*slave 		= 	aReadData[len++];
	*function  	= 	aReadData[len++];
	if(0x80 == (*function & 0x80))
	{
		return 0;
	}
	byte_count  = 	aReadData[len++];
	memcpy(data, aReadData+3, s32Len-3-2);

	u16CRC1 = CRC16(aReadData, s32Len-2);
	//ENNTRACE("%s, %d, %x, %x\n",__FUNCTION__,__LINE__,aReadData[s32Len-1], aReadData[s32Len-2]);
	u16CRC2 = (ENN_U16)(aReadData[s32Len-1]) << 8;
	//ENNTRACE("%s, %d, u16CRC2 = %x\n",__FUNCTION__,__LINE__,u16CRC2);
	u16CRC2 |= (ENN_U16)(aReadData[s32Len-2]);
	ENNTRACE("%s, %d, %x, %x\n",__FUNCTION__,__LINE__,u16CRC1, u16CRC2);
	if(u16CRC1 != u16CRC2)
	{
		return 0;
	}
	//ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,byte_count);
	
    return byte_count;
#endif
}


ENN_ErrorCode_t ENNModBus_Print_Data(ENN_U8 FunCode)
{
	FunCode_List *pFunCode_List_Temp = NULL;
	int i = 0;

	pFunCode_List_Temp = gFunCode_List_head;
	while(NULL != pFunCode_List_Temp)
	{
		if(FunCode == pFunCode_List_Temp->u8MBFunCode)
		{
			DEBUG_TRACE("DataLen = %d \n", pFunCode_List_Temp->DataLen);
			if((0x03 == FunCode) || (0x04 == FunCode))
			{
				for(i=0;i<pFunCode_List_Temp->DataLen;i++)
				//for(i=0;i<pFunCode_List_Temp->DataLen;i=i+4)
				{
					//DEBUG_TRACE("%f",*((float *)&(pFunCode_List_Temp->pData[i])));
					DEBUG_TRACE("%s,%d,%x ,%x\n",__FUNCTION__,__LINE__,&(pFunCode_List_Temp->pData[i]),(pFunCode_List_Temp->pData[i]));
	
				}
			}
			else if((0x01 == FunCode) || (0x02 == FunCode))
			{
				DEBUG_TRACE("%s ",pFunCode_List_Temp->pData);
			}
			DEBUG_TRACE("\n\n");

			break;
		}

		pFunCode_List_Temp = pFunCode_List_Temp->next;
	}
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Packet_Data(ENN_U8 MBFunCode, ENN_U32 Ideltime, ENN_U8 *pData, ENN_U16 Len, ENN_U8 *pBuf)
{
	ENN_U8 u16len = 0;
	ENN_U16 u16BufLen = 0;

	ENNAPI_ASSERT(NULL != pData);
	ENNAPI_ASSERT(NULL != pBuf);
	ENNAPI_ASSERT(0 != Len);

	pBuf[0] = 0x68;
	u16len     = 2;
	
	memcpy(pBuf + u16len, &MBFunCode, sizeof(ENN_U8));    //FunCode,
	u16len = u16len + sizeof(ENN_U8);
	//ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16len ); 

	memcpy(pBuf + u16len, &Ideltime, sizeof(ENN_U32));              //time
	u16len = u16len + sizeof(ENN_U32);
	//ENNTRACE("%s, %d, %x, %d\n",__FUNCTION__,__LINE__,Ideltime, u16len ); 

	memcpy(pBuf + u16len, pData, Len);
	u16len = u16len + Len;

	pBuf[u16len++] = 0x16;

	u16len = u16len - 2;
	memcpy(pBuf + 1, &u16len, sizeof(ENN_U8));          //数据长度

	return ENN_SUCCESS;
}

ENN_ErrorCode_t ENNModBus_History_Init()
{
	ENNOS_CreateSemaphore(1, ENNOS_SEMA_CREATE_FIFO, &g_memsema);
	//system("rm /home/historydata.log"); 
	
	return ENN_SUCCESS;
}

FILE *fpHistory = NULL;
ENN_U32 FileWriteOffset = 0;
ENN_U32 FileReadOffset = 0;
//ENN_BOOL bRead = ENN_FALSE;
ENN_BOOL bRead = ENN_TRUE;
//ENN_U8 fReaded = 0;
ENN_ErrorCode_t ENNModBus_History_Set_Data(ENN_U8 MBFunCode, ENN_U8 *pData, ENN_U16 Len)
{
#if 1
	ENN_U32 u32Idel_History_Time = 0;
	ENN_U8 i = 0;
	ENN_U8 u8HourTime = 0;
	ENN_U8 u8MinuteTime = 0;
	ENN_U8 *pParam = NULL;
	FunCode_List *pFunCode_List_Temp = NULL;
	ENN_U16 u16BufLen = 0;
	ENN_U8 *pBuf = NULL;
	ENN_S32 ret = 0;

	//ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,MBFunCode);	

	ENNAPI_ASSERT(NULL != pData);
	ENNAPI_ASSERT(0 != Len);

	if(0x03 != MBFunCode)
	{
		return ENN_FAIL;
	}
	//写历史数据
	pParam = ENNModBus_Get_Text("MAIN", "CYCLE_TIME");
	if(NULL == pParam)
	{
		return ENN_FAIL;
	}

	u8MinuteTime = atoi(pParam);
	//ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u8MinuteTime);
	
	if(0 == gFirst_History_Time)
	{
		gFirst_History_Time = ENNOS_GetSysTime();
	}

	u32Idel_History_Time = ENNOS_GetSysTime();
	//ENNTRACE("%s, %d, %ld, %ld\n",__FUNCTION__,__LINE__,gFirst_History_Time,u32Idel_History_Time);
	//ENNTRACE("%s, %d, %ld\n",__FUNCTION__,__LINE__,u32Idel_History_Time - gFirst_History_Time);
	/*if((u32Idel_History_Time - gFirst_History_Time) > 30000)
	{
		exit(0);
	}*/
#if 1
	if((u32Idel_History_Time - gFirst_History_Time) > (u8MinuteTime * 3 * 1000))
	{
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
		ENNOS_WaitSemaphore(g_memsema, ENNOS_TIMEOUT_INFINITY);
		
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
		//gFirst_History_Time = ENNOS_GetSysTime();
		//重新定位文件头，并清除文件内容
		if(NULL != fpHistory)
		{
			fclose(fpHistory);
			fpHistory = NULL;
		}
		
		fpHistory = fopen(MODBUS_HISTORY_DATA_WRITE_FILE, "ab+");
		if(NULL == fpHistory)
		{
			perror("open history file: ");
			ENNOS_SignalSemaphore(g_memsema);
			return ENN_FAIL;
		}

		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
		u16BufLen = 2 + 1 + sizeof(ENN_U8) + sizeof(ENN_U32) + Len;
		pBuf = (ENN_U8 *)malloc(u16BufLen);
		if(NULL == pBuf)
		{
			ENNOS_SignalSemaphore(g_memsema);
			return ENN_FAIL;
		}
		memset(pBuf, 0, u16BufLen);
		
		ENNModBus_Packet_Data(MBFunCode, u32Idel_History_Time, pData, Len, pBuf);
		ENNTRACE("%s, %d, %x\n",__FUNCTION__,__LINE__,pBuf);
		if(NULL != pBuf)
		{
			ret = fwrite(pBuf, u16BufLen, 1, fpHistory);
			ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,ret);
			FileWriteOffset += u16BufLen;
			gFirst_History_Time = u32Idel_History_Time;
		}
		ENNOS_SignalSemaphore(g_memsema);
		//fclose(fpWriteHistory);
		//fpWriteHistory = NULL;
		//exit(0);
	}
#endif
	/*for(i=0; i<u16BufLen; i++)
	{
		ENNTRACE("%x ",pBuf[i]);
	}
	ENNTRACE("\n\n");*/
	//fclose(fpHistory);
#endif
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Get_History_Data(ENN_U8 *pData, ENN_U16 u16RegNum, ENN_U8 *Len)
{
	ENN_S32 ret = 0;
	
	ENNAPI_ASSERT(NULL != pData);
	ENNAPI_ASSERT(0 != u16RegNum);

	ENNTRACE("%s, %d, %d, %x\n",__FUNCTION__,__LINE__,u16RegNum, fpHistory);

	ENNOS_WaitSemaphore(g_memsema, ENNOS_TIMEOUT_INFINITY);

	if(NULL != fpHistory)
	{
		fclose(fpHistory);
		fpHistory = NULL;
	}
	//exit(0);

	ENNTRACE("%s, %d, %d, %x\n",__FUNCTION__,__LINE__,u16RegNum, fpHistory);
	fpHistory = fopen(MODBUS_HISTORY_DATA_WRITE_FILE, "ab+");
	if(NULL == fpHistory)
	{
		perror("open history file fail: ");
		ENNOS_SignalSemaphore(g_memsema);
		return ENN_FAIL;
	}
	bRead = ENN_TRUE;
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,FileReadOffset);
	fseek(fpHistory, FileReadOffset, SEEK_SET);
	//fseek(fpHistory, 0, SEEK_SET);
	ENNTRACE("%s, %d, %d, %x\n",__FUNCTION__,__LINE__,u16RegNum, fpHistory);
	ENNTRACE("%s, %d, %d, %x\n",__FUNCTION__,__LINE__,u16RegNum, pData);
		
	ret = fread(pData, 1, u16RegNum, fpHistory);
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,ret);
	if(ret <= 0)
	{
		perror("read configure file fail: ");
		FileReadOffset = 0;
		ENNOS_SignalSemaphore(g_memsema);
		return ENN_FAIL;
	}
	*Len = ret;
	
	fclose(fpHistory);
	fpHistory = NULL;
	FileReadOffset += ret;
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,FileReadOffset);
	ENNOS_SignalSemaphore(g_memsema);

	return ENN_SUCCESS;
}


#if 0
ENN_ErrorCode_t ENNModBus_Write_Register(ENN_U8 Channel, ENN_U8 SlaveAddr, ENN_U8 FunCode, const ENN_U8 *data, ENN_U8 len)
{
	ENN_S32 ret = 0;
	ENN_U32 u32len;
	ENN_U32 i;
	ENN_U16 u16CRC;
	//ENN_U8 tel[256];
	ENN_U8 *tel;

	if(SlaveAddr < 0 || SlaveAddr > 255) 
	{
		return ENN_FAIL;
	}
	u32len = 0;

	tel = (ENN_U8 *)malloc(len + 1 + 1 + 2);
	if(NULL == tel)
	{
		return ENN_FAIL;
	}
	memset((void *)tel, 0, (len + 1 + 1 + 2));
		
	tel[u32len++] = (ENN_U8)SlaveAddr;
	tel[u32len++] = (ENN_U8)FunCode;
	for(i=0; i<len; i++)
	{
		tel[u32len++] = data[i];
	}
	u16CRC = CRC16(tel, u32len);
	DEBUG_TRACE("u16CRC = %x \n", u16CRC);
	tel[u32len++] = (ENN_U8)(u16CRC & 0x00FF);
	tel[u32len++] = (ENN_U8)(u16CRC >> 8);
	for(i=0; i<u32len; i++)
	{
		DEBUG_TRACE("%X ", tel[i]);
	}
	DEBUG_TRACE("\n");

	ret = UART_Write(Channel, tel, u32len);
	if(ret < 0)
	{
		return ENN_FAIL;
	}

	return ENN_SUCCESS;
}
#endif


FunCode_List *ENNModBus_Exception_Check(ENN_U8 u8FunCode, ENN_U16 u16RegAddr, ENN_U16 u16RegNum)
{
	FunCode_List *pFunCode_Temp = NULL;
	
	ENNAPI_ASSERT(NULL != gFunCode_List_head);

	pFunCode_Temp = gFunCode_List_head;
	while((u8FunCode != pFunCode_Temp->u8MBFunCode) 
	   && (NULL != pFunCode_Temp->next))
	{
		pFunCode_Temp = pFunCode_Temp->next;
	}

	if(u8FunCode != pFunCode_Temp->u8MBFunCode)
	{
		return NULL;
	}

	if((u16RegAddr < pFunCode_Temp->u16StartAddr)
	||(u16RegAddr >= pFunCode_Temp->u16EndAddr)
	||((u16RegAddr + u16RegNum) > pFunCode_Temp->u16EndAddr))
	{
		return NULL;
	}

	return pFunCode_Temp;		
}


ENN_ErrorCode_t ENNModBus_Write_Single_Coil(ENN_U16 u16RegID,  ENN_U8 Channel,ENN_U16 u16InputValue)
{
	Channel_List *pChannel_Temp = NULL;
	Slave_List	 *pSlave_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;
	Register_List 		*pRegister_List_Temp = NULL;
	ENN_U8 u8Channel = 0;
	ENN_U8 u8Mask = 0;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	ENN_U16 u16TotalRegNum = 0;
	ENN_U8 u8SlaveAddr = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U16 u16Offset = 0;
	ENN_U16 u16WriteRegAddr = 0;
	ENN_U16 u16WriteRegNum = 0;
	ENN_U8 pReqData[4];
	ENN_U8 u8SlaveAddr1 = 0;
	ENN_U16 len = 0;
	ENN_U8 data[512];
	ENN_U8 u8FunCode1 = 0;
	int j=0;

	ENNAPI_ASSERT(NULL != gChannel_List_Head);

#if 1
	pChannel_Temp = gChannel_List_Head;
	while(NULL != pChannel_Temp)
	{
		u8Channel = pChannel_Temp->u8Channel;
		if(Channel != pChannel_Temp->u8Channel){
			pChannel_Temp = pChannel_Temp->next;
			continue;
		}
		//pSlave_Temp = pChannel_Temp->pSlaveList;
		pSlave_Temp = pChannel_Temp->unDeviceType.pModBus_List;
		while(NULL != pSlave_Temp)
		{
			u8SlaveAddr = pSlave_Temp->u8SlaveAddr;
			pSlave_FunCode_Temp = pSlave_Temp->pFunCode_List;
			ENNWRITETRACE("%s, %d, u8SlaveAddr = %d\n",__FUNCTION__,__LINE__, u8SlaveAddr);
			while(NULL != pSlave_FunCode_Temp) 
			{
				u8FunCode = pSlave_FunCode_Temp->u8MBFunCode;
				u8Mask = u8FunCode - 1;
				if(MB_FUNC_WRITE_SINGLE_COIL == u8FunCode)
				{
					pRegister_List_Temp = pSlave_FunCode_Temp->pRegister_List;
					u16TotalRegNum = pSlave_FunCode_Temp->u16TotalRegNum;
					ENNWRITETRACE("%s, %d, u16TotalRegNum = %d\n",__FUNCTION__,__LINE__, u16TotalRegNum);

					if((NULL == pRegister_List_Temp) || (0 == u16TotalRegNum))
					{
						//return ENN_FAIL;
						pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
						break;
					}

					while(NULL != pRegister_List_Temp)
					{
						if(pRegister_List_Temp->u16RegID == u16RegID)
						{
							u16WriteRegAddr = pRegister_List_Temp->u16RegAddr;
							break;
							
							//return ENN_SUCCESS;
						}
						pRegister_List_Temp = pRegister_List_Temp->next;
					}
					if(pRegister_List_Temp == NULL)
						break;
						//u16WriteRegAddr = pRegister_List_Temp->u16RegAddr;

						//写寄存器
						/*pReqData = (ENN_U8 *)malloc(4);
						if(NULL == pReqData)
						{
							return ENN_MEM_MALLOC_FAIL;
							
						}*/
						memset(pReqData, 0, 4);
						pReqData[0] = (ENN_U8)(u16WriteRegAddr/256); 
						pReqData[1] = (ENN_U8)(u16WriteRegAddr & 0x00FF);
						pReqData[2] = (ENN_U8)(u16InputValue >> 8);
						pReqData[3] = (ENN_U8)u16InputValue;
						//memcpy(pReqData + 2, &u16InputValue, 2);
						ENNWRITETRACE("%s, %d, u16InputValue = 0x%X\n",__FUNCTION__,__LINE__, u16InputValue);

						pthread_mutex_lock(&MBS_mutexs[u8Channel]);
						//写操作
						ENNModBus_Set_IdelTime(pChannel_Temp->u32BaudRate);
						ENNModBus_Sleep(Modbus_Idletime);
						returnCode = ENNModBus_Write(u8Channel, u8SlaveAddr, u8FunCode, pReqData, 4);
						
						if(ENN_SUCCESS != returnCode)
						{
							pthread_mutex_unlock(&MBS_mutexs[u8Channel]);
							perror("ENNModBus_Write ");
							pSlave_Temp->u16Status = 0;
							printf("ERROR :%s, %d\n",__FUNCTION__,__LINE__);
							return returnCode;
						}
						else
						{
							ENNModBus_Sleep(Modbus_Idletime);
							len = ENNModBus_Response(u8Channel, &u8SlaveAddr1, &u8FunCode1, data);
							ENNWRITETRACE("Channelx = %d, len = %d\n",u8Channel, len);
							pthread_mutex_unlock(&MBS_mutexs[u8Channel]);
							if(len > 0)
							{
								pSlave_Temp->u16Status = 1;								
								for(; j < 4; j++)
								{
									if(pReqData[j] != data[j])
									{
										printf("ERROR :%s, %d\n",__FUNCTION__,__LINE__);
										return ENN_MODBUS_ERROR;
									}
								}
								//if((pReqData[0] != data[0]) ||)
							}
							else
							{
								pSlave_Temp->u16Status = 0;
								return ENN_MODBUS_ERROR;
							}
							
							ENNWRITETRACE("SlaveAddr = %d, Status = %d\n",pSlave_Temp->u8SlaveAddr,pSlave_Temp->u16Status);
						}

						return ENN_SUCCESS;
					//}
				}
				
				pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
			}

			pSlave_Temp = pSlave_Temp->next;
		}

		pChannel_Temp = pChannel_Temp->next;
	}
#endif		
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Write_Multiple_Register(ENN_U16 u16RegID, ENN_U8 Channel,
															ENN_U16 u16RegNum, ENN_U8 u8Bytes, ENN_U8 *pData)
{
	ENN_U8 data[512];
	ENN_U16 len = 0;
	ENN_U8 u8SlaveAddr1 = 0;
	ENN_U8 u8FunCode1 = 0;
	ENN_S32 ret = 0;
	ENN_U8 *pTemp = NULL;
	ENN_U8  u8BytesEx = 0;
	ENN_U16 u16RegNumEx = 0;
	ENN_U16 u16RegAddrEx = 0;
	ENN_U16 u16Offset = 0;
	ENN_U16 u16TotalRegNum = 0;
	ENN_U16 u16WriteRegAddr = 0;
	ENN_U16 u16WriteRegNum = 0;
	//FunCode_List *pFunCode_Temp = NULL;
	Channel_List *pChannel_Temp = NULL;
	Slave_List	 *pSlave_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;
	Register_List 		*pRegister_List_Temp = NULL;
	ENN_U8 u8SlaveAddr = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 *pReqData = NULL;
	ENN_U8 u8Interval = 0;
	ENN_U8 u8Channel = 0;
	ENN_U8 u8Mask = 0;
	ENN_U8 i = 0;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	
	ENNAPI_ASSERT(NULL != pData);
	ENNAPI_ASSERT(0 != u16RegNum);
	ENNAPI_ASSERT(0 != u8Bytes);
	ENNAPI_ASSERT(NULL != gChannel_List_Head);

	//ENNWRITETRACE("%s, %d, %d, %d, %d\n",__FUNCTION__,__LINE__,u16RegAddr, u16RegNum, u8Bytes);
	
	pTemp = pData;

	for(i=0; i<u8Bytes; i++)
	{
		ENNWRITETRACE("%X ", pTemp[i]);	
	}
	ENNWRITETRACE("\n");

	u16RegNumEx = u16RegNum;
	u8BytesEx = u8Bytes;
	//u16Offset = u16RegAddr - pFunCode_Temp->u16StartAddr;

#if 1
	pChannel_Temp = gChannel_List_Head;
	while(NULL != pChannel_Temp)
	{
		u8Channel = pChannel_Temp->u8Channel;
		if(Channel != pChannel_Temp->u8Channel){
			pChannel_Temp = pChannel_Temp->next;
			continue;
		}
		pSlave_Temp = pChannel_Temp->unDeviceType.pModBus_List;
		while(NULL != pSlave_Temp)
		{
			u8SlaveAddr = pSlave_Temp->u8SlaveAddr;
			pSlave_FunCode_Temp = pSlave_Temp->pFunCode_List;
			while(NULL != pSlave_FunCode_Temp) 
			{
				u8FunCode = pSlave_FunCode_Temp->u8MBFunCode;
				u8Mask = u8FunCode - 1;
				if(MB_FUNC_WRITE_MULTIPLE_REGISTERS == u8FunCode)
				{
					pRegister_List_Temp = pSlave_FunCode_Temp->pRegister_List;
					u16TotalRegNum = pSlave_FunCode_Temp->u16TotalRegNum;

					if((NULL == pRegister_List_Temp) || (0 == u16TotalRegNum))
					{
						//return ENN_FAIL;
						//pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
						break;
					}
						
					while(NULL != pRegister_List_Temp)
					{
						if(pRegister_List_Temp->u16RegID == u16RegID)
						{
							u16WriteRegAddr = pRegister_List_Temp->u16RegAddr;
							break;
							
							//return ENN_SUCCESS;
						}
						pRegister_List_Temp = pRegister_List_Temp->next;
					}
					if(pRegister_List_Temp == NULL)
						break;
						/*u8Interval = pRegister_List_Temp->u8Interval;
						if(0 != pRegister_List_Temp->u8Interval)
						{
							pRegister_List_Temp->u8Interval = 0;
						}*/
						
						//u16WriteRegAddr = pRegister_List_Temp->u16RegAddr;
					while(NULL != pRegister_List_Temp)
					{
						u16WriteRegNum += pRegister_List_Temp->u16RegNum;
						if((0 == pRegister_List_Temp->u8Interval)
						|| (pRegister_List_Temp == pSlave_FunCode_Temp->pRegister_List))
						{
							//u16WriteRegNum += pRegister_List_Temp->u16RegNum;
							//pRegister_List_Temp->u8Interval = u8Interval;

							if(u16WriteRegNum >= u16RegNumEx)
							{
								//写寄存器
								pReqData = (ENN_U8 *)malloc(5 + u16RegNumEx * 2);
								if(NULL == pReqData)
								{
									return ENN_MEM_MALLOC_FAIL;
									
								}
								memset(pReqData, 0, (5 + u16RegNumEx * 2));
								pReqData[0] = u16WriteRegAddr/256; 
								pReqData[1] = u16WriteRegAddr & 0x00FF;
								pReqData[2] = u16RegNumEx/256; 
								pReqData[3] = u16RegNumEx & 0x00FF;
								pReqData[4] = (ENN_U8)(u16RegNumEx * 2);
								memcpy(pReqData + 5, pTemp, u16RegNumEx * 2);
								pthread_mutex_lock(&MBS_mutexs[u8Channel]);
								//写操作
								ENNModBus_Set_IdelTime(pChannel_Temp->u32BaudRate);
								ENNModBus_Sleep(Modbus_Idletime);
								returnCode = ENNModBus_Write(u8Channel, u8SlaveAddr, u8FunCode, pReqData, (5 + u16WriteRegNum * 2));
								free(pReqData);
								pReqData = NULL;
								if(ENN_SUCCESS != returnCode)
								{
									pthread_mutex_unlock(&MBS_mutexs[u8Channel]);
									perror("ENNModBus_Write ");
									ENNWRITETRACE("%s, %d\n",__FUNCTION__,__LINE__);
									pSlave_Temp->u16Status = 0;
								}
								else
								{
									ENNModBus_Sleep(Modbus_Idletime);
									len = ENNModBus_Response(u8Channel, &u8SlaveAddr1, &u8FunCode1, data);
									pthread_mutex_unlock(&MBS_mutexs[u8Channel]);
									ENNWRITETRACE("Channelx = %d, len = %d\n",u8Channel, len);
									ENNWRITETRACE("%s, %d, u16RegNumEx = %d\n",__FUNCTION__,__LINE__,u16RegNumEx);
									if(len > 0)
									{
										pSlave_Temp->u16Status = 1;
									}
									else
									{
										pSlave_Temp->u16Status = 0;
										return ENN_MODBUS_ERROR;
									}
								}

								return ENN_SUCCESS;
							}
							
							pRegister_List_Temp = pRegister_List_Temp->next;
						}
						else
						{
							//写寄存器
							pReqData = (ENN_U8 *)malloc(5 + u16WriteRegNum * 2);
							if(NULL == pReqData)
							{
								return ENN_MEM_MALLOC_FAIL;
								
							}
							memset(pReqData, 0, (5 + u16WriteRegNum * 2));
							pReqData[0] = u16WriteRegAddr/256; 
							pReqData[1] = u16WriteRegAddr & 0x00FF;
							pReqData[2] = u16WriteRegNum/256; 
							pReqData[3] = u16WriteRegNum & 0x00FF;
							pReqData[4] = (ENN_U8)(u16WriteRegNum * 2);
							memcpy(pReqData + 5, pTemp, u16WriteRegNum * 2);
							//写操作
							ENNModBus_Set_IdelTime(pChannel_Temp->u32BaudRate);
							ENNModBus_Sleep(Modbus_Idletime);
							returnCode = ENNModBus_Write(u8Channel, u8SlaveAddr, u8FunCode, pReqData, (5 + u16WriteRegNum * 2));
							free(pReqData);
							pReqData = NULL;
							if(ENN_SUCCESS != returnCode)
							{
								perror("ENNModBus_Write ");
								ENNWRITETRACE("%s, %d\n",__FUNCTION__,__LINE__);
								pSlave_Temp->u16Status = 0;
							}
							else
							{
								ENNModBus_Sleep(Modbus_Idletime);
								len = ENNModBus_Response(u8Channel, &u8SlaveAddr1, &u8FunCode1, data);
								ENNWRITETRACE("Channelx = %d, len = %d\n",u8Channel, len);
								ENNWRITETRACE("%s, %d, u16RegNumEx = %d\n",__FUNCTION__,__LINE__,u16RegNumEx);
								if(len > 0)
								{
									pSlave_Temp->u16Status = 0;
								}
								else
								{
									pSlave_Temp->u16Status = 1;
								}
							}
							ENNWRITETRACE("%s, %d, u16RegNumEx = %d\n",__FUNCTION__,__LINE__,u16RegNumEx);
							pTemp = pTemp + u16WriteRegNum * 2;

							u16RegNumEx -= u16WriteRegNum;
							ENNWRITETRACE("%s, %d, u16RegNumEx = %d\n",__FUNCTION__,__LINE__,u16RegNumEx);
							if(0 == u16RegNumEx)
							{
								ENNWRITETRACE("%s, %d\n",__FUNCTION__,__LINE__);
								return ENN_SUCCESS;
							}
							else
							{
								u16WriteRegNum = 0;
								u16WriteRegNum += pRegister_List_Temp->u16RegNum;
								u16WriteRegAddr = pRegister_List_Temp->u16RegAddr;
								pRegister_List_Temp = pRegister_List_Temp->next;
							}
						}
						
						if(NULL == pRegister_List_Temp)
						{
							//写寄存器
							pReqData = (ENN_U8 *)malloc(5 + u16WriteRegNum * 2);
							if(NULL == pReqData)
							{
								return ENN_MEM_MALLOC_FAIL;
								
							}
							memset(pReqData, 0, (5 + u16WriteRegNum * 2));
							pReqData[0] = u16WriteRegAddr/256; 
							pReqData[1] = u16WriteRegAddr & 0x00FF;
							pReqData[2] = u16WriteRegNum/256; 
							pReqData[3] = u16WriteRegNum & 0x00FF;
							pReqData[4] = (ENN_U8)(u16WriteRegNum * 2);
							memcpy(pReqData + 5, pTemp, u16WriteRegNum * 2);
							//写操作
							ENNModBus_Set_IdelTime(pChannel_Temp->u32BaudRate);
							ENNModBus_Sleep(Modbus_Idletime);
							returnCode = ENNModBus_Write(u8Channel, u8SlaveAddr, u8FunCode, pReqData, (5 + u16WriteRegNum * 2));
							free(pReqData);
							pReqData = NULL;
							if(ENN_SUCCESS != returnCode)
							{
								perror("ENNModBus_Write ");
								ENNWRITETRACE("%s, %d\n",__FUNCTION__,__LINE__);
								pSlave_Temp->u16Status = 0;
							}
							else
							{
								ENNModBus_Sleep(Modbus_Idletime);
								len = ENNModBus_Response(u8Channel, &u8SlaveAddr1, &u8FunCode1, data);
								ENNWRITETRACE("Channelx = %d, len = %d\n",u8Channel, len);
								if(len > 0)
								{
									pSlave_Temp->u16Status = 1;
								}
								else
								{
									pSlave_Temp->u16Status = 0;
								}
							}
							pTemp = pTemp + u16WriteRegNum * 2;

							u16RegNumEx -= u16WriteRegNum;
							ENNWRITETRACE("%s, %d, u16RegNumEx = %d\n",__FUNCTION__,__LINE__,u16RegNumEx);
							if(0 == u16RegNumEx)
							{
								return ENN_SUCCESS;
							}
						}
					}
					break;
				}

				pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
			}

			pSlave_Temp = pSlave_Temp->next;
		}

		pChannel_Temp = pChannel_Temp->next;
	}
#endif		
	return ENN_SUCCESS;
}

ENN_ErrorCode_t HandleByteOrder_Fun34(FunCode_List *pFunCode_Temp, Register_List *pRegister_List_Temp, 
											ENN_U8 **pBuf, ENN_U16 *pu16Offset)
{
	ENN_U16 u16RegNum = 0;
	ENN_U16 u16ByteNum = 0;

	u16RegNum = pRegister_List_Temp->u16RegNum;
	u16ByteNum = u16RegNum * 2;
	//DEBUG_TRACE("%s, %d, %d, %d\n",__FUNCTION__,__LINE__,u8Channelx, returnCode);
	if(0 != pRegister_List_Temp->u8Interval)
	{
		*pBuf += ((pRegister_List_Temp->u8Interval) * 2);
	}

	//DEBUG_TRACE("%s, %d, %d, %d\n",__FUNCTION__,__LINE__,u8Channelx, returnCode);
	//DEBUG_TRACE("%s, %d, %d, u16Offset = %d\n",__FUNCTION__,__LINE__,u8Channelx, u16Offset);
	//edit by hyptek
	//if(0 == u8DataFormat)
	printf("[%s],%d, pRegister_List_Temp->u8ByteOrder =%s, u16Offset = %d, pBuf = %x, RegID =%d\n",__FUNCTION__,__LINE__,
		pRegister_List_Temp->pBtOrder, *pu16Offset, **pBuf, pRegister_List_Temp->u16RegID);
	if(strncmp(pRegister_List_Temp->pBtOrder, "AB", 2) == 0)
	{
		if(NULL != pFunCode_Temp->pData)
		{
			memcpy((void *)(pFunCode_Temp->pData + *pu16Offset), (void *)*pBuf, u16ByteNum);
		}
		*pu16Offset += u16ByteNum;
	}
	else if(strncmp(pRegister_List_Temp->pBtOrder, "BA", 2) == 0)
	{
		while(0 != u16ByteNum)
		{
			u16ByteNum--;
			if(NULL != pFunCode_Temp->pData)
			{
				*(pFunCode_Temp->pData + *pu16Offset) = *(*pBuf += u16ByteNum);
			}
			(*pu16Offset)++;
		}
		//printf("%s, %d,  u16Offset = %d\n",__FUNCTION__,__LINE__,*pu16Offset);
		u16ByteNum = u16RegNum * 2;
	}else
	{
		
	}
	//DEBUG_TRACE("%s, %d, Channel = %d, u16ByteNum = %d\n",__FUNCTION__,__LINE__,u8Channelx,u16ByteNum);
	//printf("%s, %d,  u16Offset = %d\n",__FUNCTION__,__LINE__,*pu16Offset);
	*pBuf += u16ByteNum;

	return ENN_SUCCESS;
}

ENN_U8 Debug_Test[8];
#if 1
ENN_ErrorCode_t ENNModBus_Channel_Run(Channel_List *pChannel_List, FunCode_List *pFunCode_HOLDING_REG)
{
	ENN_U8 data[512];
	ENN_U16 len = 0;
	ENN_U8 u8SlaveAddr = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 u8SlaveAddr1 = 0;
	ENN_U8 u8FunCode1 = 0;
	ENN_U16 u16RegAddr = 0;
	ENN_U16 u16Num = 0;
	ENN_U8 u8Channelx = 0;
	Channel_List *pChannel_Temp = NULL;
	Slave_List 	*pSlave_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;
	FunCode_List *pFunCode_Temp = NULL;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	ENN_U16 u16Offset = 0;
	Register_List *pRegister_List_Temp = NULL;
	ENN_U8 *pBuf = NULL;
	ENN_CHAR *pStr = NULL;
	ENN_CHAR chStr[10];
	ENN_U8  u8ByTe = 0;
	ENN_U8  u8Interval = 0;
	
	ENN_U8	u8DataFormat = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U16 u16ByteNum = 0;
	ENN_U8 u8Data = 0;
	ENN_U8 u8BitCount = 0;
	ENN_U8 i = 0;
	ENN_U8 u8StrLen = 0;
	ENN_CHAR Tmp[9];
	ENN_U32 u32BaudRate = 0;
	ENN_U8 u8Mask = 0;
	Dev645_List *pDev645_List_Temp = NULL;
	Code_645_List *pCode_645_List_Temp = NULL;
	T_REG_DATA tdata;
	ENN_U8 u8ChannelIndex = 0;

	ENNAPI_ASSERT(NULL != pChannel_List);
	ENNAPI_ASSERT(NULL != pFunCode_HOLDING_REG);
	
	pChannel_Temp = pChannel_List;
	u8Channelx = pChannel_Temp->u8Channel;
	u32BaudRate = pChannel_Temp->u32BaudRate;
	ENNModBus_Set_IdelTime(u32BaudRate);

	if(PROTOCOL_MODBUS == pChannel_Temp->u8Protocol)
	{
		DEBUG_PRINT("%s, %d,   MODBUS\n",__FUNCTION__,__LINE__);
		pFunCode_Temp = gFunCode_List_head;
		while(NULL != pFunCode_Temp)
		{
			DEBUG_TRACE("%s, %d, Channelx = %d, MBFunCode = %d\n",__FUNCTION__,__LINE__,u8Channelx, pFunCode_Temp->u8MBFunCode);
			u16Offset = pFunCode_Temp->Offset[u8Channelx];
			if((MB_FUNC_READ_HOLDING_REGISTER == pFunCode_Temp->u8MBFunCode)  
			|| (MB_FUNC_READ_INPUT_REGISTER == pFunCode_Temp->u8MBFunCode))
			{
				u16Offset = u16Offset * 2;  //将每一个channel的寄存器的偏移量的长度转换为对应数据长度的偏移量
			}
			else if((MB_FUNC_WRITE_SINGLE_COIL == pFunCode_Temp->u8MBFunCode)  
			     || (MB_FUNC_WRITE_MULTIPLE_REGISTERS == pFunCode_Temp->u8MBFunCode))
			{
				pFunCode_Temp = pFunCode_Temp->next;
				continue;
			}
			
			//pSlave_Temp = pChannel_Temp->pSlaveList;
			pSlave_Temp = pChannel_Temp->unDeviceType.pModBus_List;
			DEBUG_TRACE("%s, %d, Offset = %d\n",__FUNCTION__,__LINE__,u16Offset);
			while(NULL != pSlave_Temp)
			{
				ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,pSlave_Temp->u8SlaveAddr);
				u8DataFormat = pSlave_Temp->u8DataFormat;
				u8SlaveAddr = pSlave_Temp->u8SlaveAddr;
				pSlave_FunCode_Temp = pSlave_Temp->pFunCode_List;
				while(NULL != pSlave_FunCode_Temp)
				{
					u8FunCode = pSlave_FunCode_Temp->u8MBFunCode;
					u8Mask = u8FunCode - 1; 
					if(u8FunCode == pFunCode_Temp->u8MBFunCode)
					{//break;
						u16Num = pSlave_FunCode_Temp->u16Count;
						if(NULL != pSlave_FunCode_Temp->pRegister_List)
						{
							pRegister_List_Temp = pSlave_FunCode_Temp->pRegister_List;
							if(NULL == pRegister_List_Temp)
							{
								continue;
							}
							u16RegAddr = pRegister_List_Temp->u16RegAddr;
							DEBUG_TRACE("%s, %d\n",__FUNCTION__,__LINE__);
							pthread_mutex_lock(&MBS_mutexs[u8Channelx]);
							returnCode = ENNModBus_Request(u8Channelx, u8SlaveAddr, u8FunCode, u16RegAddr, u16Num);
							//printf("%s, %d, u8Channelx[%d],u8SlaveAddr[%d]\n",__FUNCTION__,__LINE__,u8Channelx,u8SlaveAddr);
							if(ENN_SUCCESS != returnCode)
							{
								ENNLED_On(u8Channelx, LED_ERROR);
								pSlave_Temp->u16Status = 0;
								pthread_mutex_unlock(&MBS_mutexs[u8Channelx]);
								ENNOS_DelayTask(Modbus_Idletime + 200);
								printf("ERROR:%s, %d\n",__FUNCTION__,__LINE__);
								perror("ENNModBus_Request ");								
							}
							else
							{
								//returnCode = ENNModBus_Select(u8Channelx, 1000);
								//ENNModBus_Sleep(Modbus_Idletime + 300);
								ENNOS_DelayTask(Modbus_Idletime + 200);//当前线程挂起241毫秒
								DEBUG_TRACE("%s, %d, %d, %d, %d, %d, %d\n",__FUNCTION__,__LINE__,u8Channelx, u8SlaveAddr, u8FunCode, u16RegAddr, u16Num);
								DEBUG_TRACE("%s, %d, %d, %d\n",__FUNCTION__,__LINE__,u8Channelx, returnCode);
								if(ENN_SUCCESS == returnCode)  //发送请求成功，开始进行数据的读取
								{
									len = ENNModBus_Response(u8Channelx, &u8SlaveAddr1, &u8FunCode1, data);//len是读取的数据长度
									pthread_mutex_unlock(&MBS_mutexs[u8Channelx]);
									DEBUG_TRACE("%s,%d ,Channelx = %d, len = %d\n",__FUNCTION__,__LINE__,u8Channelx, len);
									if(len > 0)
									{
										ENNLED_On(u8Channelx, LED_NORMAL);
										pSlave_Temp->u16Status = 1;      //1表示设备工作正常
										ENNTRACE("data:   ");
										for(i=0; i<len; i++)
										{
											DEBUG_TRACE("%x ",data[i]);
										}
										DEBUG_TRACE("\n\n");
										
										ENNTRACE("addr:   ");
										for(i=0; i<len; i++)
										{
											DEBUG_TRACE("%d ",&data[i]);
										}
										DEBUG_TRACE("\n\n");

										pBuf = (ENN_U8 *)data;

										while(NULL != pRegister_List_Temp)
										{
											if((0x03 == u8FunCode) || (0x04 == u8FunCode))
											{
												DEBUG_TRACE("%s, %d, %d, %d\n",__FUNCTION__,__LINE__,u8Channelx,returnCode);
												//HandleByteOrder_Fun34(pFunCode_Temp, pRegister_List_Temp, &pBuf, &u16Offset);
												u16RegNum = pRegister_List_Temp->u16RegNum;
												u16ByteNum = u16RegNum * 2;
												DEBUG_TRACE("%s, %d, %d, %d\n",__FUNCTION__,__LINE__,u8Channelx, returnCode);
												if(0 != pRegister_List_Temp->u8Interval)
												{
													pBuf += ((pRegister_List_Temp->u8Interval) * 2);
												}

												DEBUG_TRACE("%s, %d, %d, %d\n",__FUNCTION__,__LINE__,u8Channelx, returnCode);
												DEBUG_TRACE("%s, %d, %d, u16Offset = %d\n",__FUNCTION__,__LINE__,u8Channelx, u16Offset);
												//edit by hyptek
												//if(0 == u8DataFormat)
												DEBUG_TRACE("[%s],%d, pRegister_List_Temp->u8ByteOrder =%s\n",__FUNCTION__,__LINE__,pRegister_List_Temp->pBtOrder);
												/*printf("[%s],%d, pRegister_List_Temp->u8ByteOrder =%s, u16Offset = %d, pBuf = %x\n",__FUNCTION__,__LINE__,
													pRegister_List_Temp->pBtOrder, u16Offset, *pBuf);*/
											
												if(strlen(pRegister_List_Temp->pBtOrder) == 2)
												{	
													if(strncmp(pRegister_List_Temp->pBtOrder, "AB", 2) == 0)  //大端模式
													{
														if(NULL != pFunCode_Temp->pData)
														{
															memcpy((void *)(pFunCode_Temp->pData + u16Offset), (void *)pBuf, u16ByteNum); //讲读取到的数据中的有效数据集中存放在pData中
														}
														u16Offset += u16ByteNum;
													}
													else if(strncmp(pRegister_List_Temp->pBtOrder, "BA", 2) == 0)  //小端模式
													{
														while(0 != u16ByteNum)
														{
															u16ByteNum--;
															if(NULL != pFunCode_Temp->pData)
															{
																*(pFunCode_Temp->pData + u16Offset) = pBuf[u16ByteNum];
															}
															u16Offset++;
														}
														u16ByteNum = u16RegNum * 2;
													}
												}else
												{//Variable bytes order													
													int blen = strlen(pRegister_List_Temp->pBtOrder);//获取当前寄存器字节序的长度
													if(blen != u16ByteNum)                                              //当前字节序的长度不等于所处的寄存器的长度
													{
														printf("WARNING : %s, %d, blen != u16ByteNum\n",__FUNCTION__,__LINE__);
														if(NULL != pFunCode_Temp->pData)
														{
															memcpy((void *)(pFunCode_Temp->pData + u16Offset), (void *)pBuf, u16ByteNum);
														}
														u16Offset += u16ByteNum;
													}else                                                                      //字节序长度等于寄存器的长度
													{
														int itmp;
														int j = 0;
														ENN_CHAR *p = pRegister_List_Temp->pBtOrder;
														for(; j < blen; j++)
														{
															if((*(p + j) < 'A') ||(*(p + j) > 'H'))
																break;
															itmp = *(p + j) -'A';
															if(itmp > blen)
																break;
															*(pFunCode_Temp->pData + u16Offset + j) = pBuf[itmp - 1];
														}
														/*for(j = 0; j < blen; j++)
														{														
															printf("%x,",*(pFunCode_Temp->pData + u16Offset + j));
														}
														printf("\n");*/
														u16Offset += u16ByteNum;
													}
												}
												DEBUG_TRACE("%s, %d, Channel = %d, u16ByteNum = %d\n",__FUNCTION__,__LINE__,u8Channelx,u16ByteNum);
												DEBUG_TRACE("%s, %d, Channel = %d, u16Offset = %d\n",__FUNCTION__,__LINE__,u8Channelx,u16Offset);
												pBuf += u16ByteNum;
												//ENNModBus_Print_Data(u8FunCode1);
											}
											else if((0x01 == u8FunCode) || (0x02 == u8FunCode))
											{
												while((0 != len) && (NULL != pRegister_List_Temp))
												{
													//DEBUG_TRACE("%s, %d, pRegister_List_Temp = %x\n",__FUNCTION__,__LINE__,pFunCode_Temp);
													u16RegNum = pRegister_List_Temp->u16RegNum;
													u8ByTe = *pBuf;
													memset((void *)&chStr, 0, 10);
													u8StrLen = 0;
													pStr = ENN_Int_To_Format(u8ByTe, chStr, 2);
													/*if(NULL == pStr)
													{
														return ENN_FAIL;
													}*/
													
													//printf("[%s], %d, pBuf = %s, pStr = %s, u8ByTe = %d\n",__FUNCTION__,__LINE__, pBuf, pStr, u8ByTe);

													while('\0' != *pStr)
													{
														u8StrLen++;
														pStr++;
													}
													
													memset((void *)&Tmp, 0, 9);
													Tmp[8] = '\0';
													i = 0;
													while(0 != u8StrLen)
													{
														u8StrLen--;
														pStr--;
														Tmp[i++] = *pStr;
													}
													while(i < 8)
													{
														Tmp[i++] = '0';
													}

													pStr = &Tmp;
													/*for(i=0; i<9; i++)
													{
														DEBUG_TRACE("%s, %d, pStr[%d] = %c, shanjianchao\n",__FUNCTION__,__LINE__,i,pStr[i]);
													}*/
													//DEBUG_TRACE("\n\n\n");
													//DEBUG_TRACE("%s, %d, strlen(pStr) = %d\n",__FUNCTION__,__LINE__,strlen(pStr));
													DEBUG_TRACE("%s, %d, Channel = %d, pStr = %s\n",__FUNCTION__,__LINE__,u8Channelx,pStr);
													u8Interval = 0;

													while(('\0' != *pStr) && (NULL != pRegister_List_Temp))
													{
														//DEBUG_TRACE("%s, %d, addr(pStr) = %x\n",__FUNCTION__,__LINE__,pStr);
														//DEBUG_TRACE("%s, %d, strlen(pStr) = %d\n",__FUNCTION__,__LINE__,strlen(pStr));
														//DEBUG_TRACE("%s, %d, *pStr[%d] = %c\n",__FUNCTION__,__LINE__,u16Offset,*pStr);
														//if(0 != pRegister_List_Temp->u8Interval)
														{
															//pStr += (pRegister_List_Temp->u8Interval);
															//pStr += u8Interval;
														}

														pFunCode_Temp->pData[u16Offset] = *pStr;
														//DEBUG_TRACE("%s, %d, pData[%d] = %c\n",__FUNCTION__,__LINE__,u16Offset,pFunCode_Temp->pData[u16Offset]);
														u16Offset++;
														/*pRegister_List_Temp = pRegister_List_Temp->next;

														if(NULL != pRegister_List_Temp)
														{
															if(0 == pRegister_List_Temp->u8Interval)
															{
																pStr++;
																DEBUG_TRACE("%s, %d, addr(pStr) = %x\n",__FUNCTION__,__LINE__,pStr);
															}
														}*/
														//DEBUG_TRACE("%s, %d, strlen(pStr+1) = %d\n",__FUNCTION__,__LINE__,strlen(pStr+1));
														//DEBUG_TRACE("%s, %d, *pStr[%d] = %c\n",__FUNCTION__,__LINE__,u16Offset,*(pStr+1));
														if('\0' == *(pStr+1))
														{
															//DEBUG_TRACE("%s, %d, *pStr[%d] = %c\n",__FUNCTION__,__LINE__,u16Offset,*(pStr+1));
															break;
														}
														else
														{	
															//DEBUG_TRACE("%s, %d, pRegister_List_Temp->next = %x\n",__FUNCTION__,__LINE__,pRegister_List_Temp->next);
															if(NULL != pRegister_List_Temp->next)
															{
																pRegister_List_Temp = pRegister_List_Temp->next;
																pStr += (pRegister_List_Temp->u8Interval);
															}
															else
															{
																break;
															}
														}
														
														//DEBUG_TRACE("%s, %d, addr(pStr) = %x\n",__FUNCTION__,__LINE__,pStr);
														//DEBUG_TRACE("%s, %d, strlen(pStr) = %d\n",__FUNCTION__,__LINE__,strlen(pStr));
														//DEBUG_TRACE("%s, %d, *pStr[%d] = %c\n",__FUNCTION__,__LINE__,u16Offset,*pStr);
														//DEBUG_TRACE("%s, %d, pData[%d] = %c\n",__FUNCTION__,__LINE__,u16Offset,pFunCode_Temp->pData[u16Offset]);

													}
													
													len--;
													//DEBUG_TRACE("%s, %d, %d, %x\n",__FUNCTION__,__LINE__,len,pFunCode_Temp);
													pBuf++;
													if(NULL != pRegister_List_Temp)
													{
														pRegister_List_Temp = pRegister_List_Temp->next;
													}
												}
												
												//ENNModBus_Print_Data(0x01);
											}

											if(NULL != pRegister_List_Temp)
											{
												pRegister_List_Temp = pRegister_List_Temp->next;
											}
										}
									}
									else
									{
										ENNLED_On(u8Channelx, LED_ERROR);
										ENNModBus_Sleep(200);
										//perror("ENNModBus_Response ");
										ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
										if((0x03 == u8FunCode) || (0x04 == u8FunCode))
										{
											//u16Offset += pSlave_FunCode_Temp->u16Count * 2;
											u16Offset += pRegister_List_Temp->u16RegNum * 2;
											while(NULL != pRegister_List_Temp->next)
											{
												pRegister_List_Temp = pRegister_List_Temp->next;
												u16Offset += pRegister_List_Temp->u16RegNum * 2;
											}
										}
										else if((0x01 == u8FunCode) || (0x02 == u8FunCode))
										{
											//u16Offset += (8 - (pSlave_FunCode_Temp->u16Count)%8);
											u16Offset++;
											while(NULL != pRegister_List_Temp->next)
											{
												pRegister_List_Temp = pRegister_List_Temp->next;
												u16Offset++;
											}
										}
										pSlave_Temp->u16Status = 0;
									}
								}
								else
								{
									if((0x03 == u8FunCode) || (0x04 == u8FunCode))
									{
										//u16Offset += pSlave_FunCode_Temp->u16Count * 2;
										u16Offset += pRegister_List_Temp->u16RegNum * 2;
										while(NULL != pRegister_List_Temp->next)
										{
											pRegister_List_Temp = pRegister_List_Temp->next;
											u16Offset += pRegister_List_Temp->u16RegNum * 2;
										}
									}
									else if((0x01 == u8FunCode) || (0x02 == u8FunCode))
									{
										//u16Offset += (8 - (pSlave_FunCode_Temp->u16Count)%8);
										u16Offset++;
										while(NULL != pRegister_List_Temp->next)
										{
											pRegister_List_Temp = pRegister_List_Temp->next;
											u16Offset++;
										}
									}
								}
							}
						}
						break;
					}

					ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
					pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
				}
				
				pSlave_Temp = pSlave_Temp->next;
			}		

			ENNModBus_Print_Data(u8FunCode1);
			ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u8FunCode1); 
			Debug_Test[u8Channelx] = 1;

			//写历史数据
			//ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__); 
			if(MB_FUNC_ERROR != (u8FunCode1 & MB_FUNC_ERROR))
			{
				if(!bRead)
				{
					ENNModBus_History_Set_Data(u8FunCode1, pFunCode_Temp->pData, pFunCode_Temp->DataLen);
				}
			}
				
			pFunCode_Temp = pFunCode_Temp->next;
		}
	}
	
	ENNModBus_Print_Data(0x03);
	return ENN_SUCCESS;
}
#endif

ENN_ErrorCode_t ENNModBus_Channel_Task(ENN_VOID *param)
{
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	Channel_List *pChannel_List = NULL;
	ENN_U8	u8Channel = 0;
	ENN_U32 u32BaudRate = 0;
	ENN_U8	u8DataBit = 0;
	ENN_U8	u8StopBit = 0;
	ENN_U8	u8Prity = 0;
	FunCode_List *pFunCode_Temp = NULL;
	int ret;/* initialize an attribute to default value */
	
	ENNAPI_ASSERT(NULL != param);
	pChannel_List = (Channel_List *)param;

	if(0 == pChannel_List->u8SlaveNum)
	{
		return ENN_SUCCESS;
	}

	/*init uart*/
	u8Channel = pChannel_List->u8Channel;
	u32BaudRate = pChannel_List->u32BaudRate;
	u8DataBit = pChannel_List->u8DataBit;
	u8StopBit = pChannel_List->u8StoptBit;
	u8Prity = pChannel_List->u8Parity;

/*	if(u8Channel ==0)
		return;*/

	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	ENNTRACE("Channel: %d\nBaudRate: %d\nDataBit: %d\nStopBit: %d\nPrity: %d\n",u8Channel, u32BaudRate,u8DataBit,
																		  u8StopBit,u8Prity);
	returnCode = UART_Init(u8Channel, u32BaudRate, u8DataBit, u8StopBit, u8Prity);    //串口初始化
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d, %d, %d, %d, %d\n",__FUNCTION__,__LINE__,u8Channel,u32BaudRate,u8DataBit, u8StopBit, u8Prity);
		return ENN_FAIL;
	}
	
	pFunCode_Temp = gFunCode_List_head;
	while((NULL != pFunCode_Temp)
		&& (MB_FUNC_READ_HOLDING_REGISTER != pFunCode_Temp->u8MBFunCode))
	{
		printf("%s, %d, u8MBFunCode = %d\n",__FUNCTION__,__LINE__, pFunCode_Temp->u8MBFunCode);
		pFunCode_Temp = pFunCode_Temp->next;
	}
	printf("%s, %d, Channel = %d, Protocol = %d\n",__FUNCTION__,__LINE__, pChannel_List->u8Channel, pChannel_List->u8Protocol);

	//MBS_mutexs[u8Channel] = PTHREAD_MUTEX_INITIALIZER;
	//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	ret = pthread_mutex_init(&MBS_mutexs[u8Channel], NULL);
	if(ret != 0)
	{
		printf("ERROR :%s, %d, pthread_mutex_init err !\n",__FUNCTION__,__LINE__);
	}
	while(1)
	{
		//if(0 == Debug_Test[u8Channel])
		{
			returnCode = ENNModBus_Channel_Run(pChannel_List, pFunCode_Temp);
			if(ENN_SUCCESS != returnCode)
			{
				return returnCode;
			}
		}
		//fReaded =fReaded | (1 << pChannel_List->u8Channel);
		//printf("Test*******:%s, %d,Channel = %d, fReaded = %d\n",__FUNCTION__,__LINE__,pChannel_List->u8Channel, fReaded);
		//ENNModBus_Print_Data(0x03);
		ENNOS_DelayTask(2000);
	}
	
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Create_Channel_Task(void)
{
	ENN_CHAR aName[10];
	ENNOS_TASK_t taskID;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	Channel_List *pChannel_Temp = NULL;

	if(NULL == gFunCode_List_head)
	{
		return ENN_FAIL;
	}
	
	memset(Debug_Test, 0, 8);
	pChannel_Temp = gChannel_List_Head;
	while(NULL != pChannel_Temp)
	{
		memset(aName, 0, 10);
		sprintf(aName, "%s_%d", "CHANNEL", pChannel_Temp->u8Channel);
		//printf("%s, %d, Protocol = %d\n",__FUNCTION__,__LINE__, pChannel_Temp->u8Protocol);
		returnCode = ENNOS_CreateTask(aName, 
			ENNOS_TASK_PRIORITY_MIDDLE, 
			4*1024,                                          
			&taskID, 
			ENNOS_TASK_START, 
			(void *)ENNModBus_Channel_Task, 
			(ENN_VOID *)pChannel_Temp);
		if(ENN_SUCCESS != returnCode)
		{
			ENNTRACE("\nCreate channel task fail!\n");
			return returnCode;
		}
		pChannel_Temp = pChannel_Temp->next;
	}
	
	return ENN_SUCCESS;
}


#if 1
ENN_ErrorCode_t ENNModBus_Read_CoilStatus(ENN_U8 MBFunCode, ENN_U16 RegAddr, ENN_U16 RegNum, ENN_U16 *CoilLen, ENN_U8 *pResData)
{
	FunCode_List *pFunCode_List_Temp = NULL;
	ENN_CHAR *pData = NULL;
	ENN_U16 u16StrLen = 0;
	ENN_U16 u16Pos = 0;
	ENN_U16 u16CoilLen = 0;
	ENN_U8 u8ByteValue = 0;
	ENN_CHAR aTemp[9];
	ENN_CHAR ucTemp = 0;
	ENN_U8 *pTemp = NULL;
	ENN_U8 i = 0;
	ENN_U8 u8Len = 0;
	
	ENNAPI_ASSERT(NULL != pResData);

	pTemp = pResData;
	aTemp[8] = '\0';
	u16CoilLen = *CoilLen;
	pFunCode_List_Temp = gFunCode_List_head;
	while(NULL != pFunCode_List_Temp)
	{
		if(MBFunCode == pFunCode_List_Temp->u8MBFunCode)
		{
			ENNTRACE("%s, %d, %d, %d, %d\n",__FUNCTION__,__LINE__,RegAddr, RegNum, pFunCode_List_Temp->u16StartAddr);
			
			if((RegAddr < pFunCode_List_Temp->u16StartAddr)
			|| (RegAddr + RegNum > pFunCode_List_Temp->u16EndAddr))
			{
				return ENN_ERR_INVALID_REG_ADDR;
			}
			
			u16Pos = (RegAddr - pFunCode_List_Temp->u16StartAddr); 
			pData = pFunCode_List_Temp->pData;
			pData = pData + u16Pos;
			ENNTRACE("%s, %d, u16Pos = %d\n",__FUNCTION__,__LINE__,u16Pos);
			ENNTRACE("%s, %d, pData = %s\n",__FUNCTION__,__LINE__,pData);

			u16StrLen = pFunCode_List_Temp->DataLen;
			u16Pos = 0;
			while(0 != u16CoilLen)
			{
				pData += u16Pos;
				u8Len = strlen(pData);
				if(u8Len < 8)
				{
					memcpy(aTemp, pData, u8Len);
					while(u8Len < 8)
					{
						aTemp[u8Len++] = '0';
					}
				}
				else
				{
					memcpy(aTemp, pData, 8);
				}
				ENNTRACE("%s, %d, aTemp = %s\n",__FUNCTION__,__LINE__,aTemp);
				for(i=0; i<4; i++)
				{
					ucTemp = aTemp[7-i];   //7-0  6-1  5-2 4-3
					aTemp[7-i] = aTemp[i];
					aTemp[i] = ucTemp;
				}
				ENNTRACE("%s, %d, aTemp = %s\n",__FUNCTION__,__LINE__,aTemp);
				u8ByteValue = ENN_strtol(&aTemp, NULL, 2);
				ENNTRACE("%s, %d, u8ByteValue = %d\n",__FUNCTION__,__LINE__,u8ByteValue);

				*pTemp++ = u8ByteValue;
				u16Pos += 8;
				u16CoilLen--;
			}
			//exit(0);
			return ENN_SUCCESS;
		}

		pFunCode_List_Temp = pFunCode_List_Temp->next;
	}

	return ENN_ERR_INVALID_FUNCODE;
}
#endif
#ifdef __cplusplus
#if __cplusplus
    }
#endif /* __cpluscplus */
#endif /* __cpluscplus */

  
