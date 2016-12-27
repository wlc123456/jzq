/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：ModBus_Module.c
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/	
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/tcp.h>


#include "ennAPI.h"
#include "ennSocket.h"
#include "ModBus_TCP.h"
#include "ModBus_Slave_Table.h"


#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */

typedef struct _MODBUS_DPA_REG_MAP
{
	ENN_U8		u8Funcode;
	ENN_U16 	u16RegAddr;
	ENN_U16		u16RegID;
	struct _MODBUS_DPA_REG_MAP *next;
}MODBUS_DPA_REG_MAP;

#define VirtualRegAddr 256

extern FunCode_List *gFunCode_List_head;
extern Channel_List *gChannel_List_Head;
extern UINT16 DevRegNum_Total;
extern DEVICE_REG_DATA *p_Reg_Data;
extern pthread_mutex_t MBS_mutexs[8];
extern ENN_U32 Modbus_Idletime;
extern ENN_U8 DevCount ;

MODBUS_DPA_REG_MAP *pModbus_Reg_Map = NULL;


ENN_ErrorCode_t ENNModBus_get_RegInfo(ENN_U16 RegID, ENN_U8 *u8Channel, 
													ENN_U8 *u8SlaveAddr, ENN_U8 *FunCode, ENN_U32 *RegAddr)
{
	Register_List *pRegister_List_Temp = NULL;
	Channel_List *pChannel_Temp = NULL;
	Slave_List	*pSlave_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;
	
	if(RegID == 0 ||NULL == gChannel_List_Head ||RegAddr ==NULL )
	{
		printf("ERROR : [%s], %d, Param == NULL\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}

	pChannel_Temp = gChannel_List_Head;
	while(NULL != pChannel_Temp)
	{
		if(PROTOCOL_MODBUS == pChannel_Temp->u8Protocol)
		{
			pSlave_Temp = pChannel_Temp->unDeviceType.pModBus_List;
			while(NULL != pSlave_Temp)
			{
				ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,pSlave_Temp->u8SlaveAddr);
				pSlave_FunCode_Temp = pSlave_Temp->pFunCode_List;
				while(NULL != pSlave_FunCode_Temp)
				{

					if(NULL != pSlave_FunCode_Temp->pRegister_List)
					{
						pRegister_List_Temp = pSlave_FunCode_Temp->pRegister_List;
					}
					else
					{
						pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
						continue;
					}

					ENNTRACE("test:%s, %d, pSlave_FunCode_Temp.fun =%d \n",__FUNCTION__,__LINE__, pSlave_FunCode_Temp->u8MBFunCode);
					while(NULL != pRegister_List_Temp)
					{
						if(pRegister_List_Temp->u16RegID == RegID)
						{
							*u8Channel = pChannel_Temp->u8Channel;
							*u8SlaveAddr = pSlave_Temp->u8SlaveAddr;
							*FunCode = pSlave_FunCode_Temp->u8MBFunCode;
							*RegAddr = pRegister_List_Temp->u16RegAddr;
							//*d = pRegister_List_Temp->fRegD;
							return ENN_SUCCESS;
						}
						pRegister_List_Temp = pRegister_List_Temp->next;
					}
					pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
				}
				pSlave_Temp = pSlave_Temp->next;
			}
		}
		pChannel_Temp = pChannel_Temp->next;
	}
	return ENN_FAIL;
}


ENN_ErrorCode_t ENNModBus_Reg_Map_init(ENN_CHAR * aParam)
{
	if(aParam == NULL ||(strlen(aParam) <= 0))
	{
		printf("ERROR : [%s], %d, Param == 0\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}

	ENN_CHAR *pTmp = NULL;
	ENN_CHAR *pTmp1 = NULL;
	ENN_CHAR *pBuf = NULL;
	ENN_CHAR *pb;
	ENN_CHAR *pc;
	ENN_U8 u8Num = 0;
	ENN_U8 u8Len = 0;
	int i;
	MODBUS_DPA_REG_MAP *pModbus_Reg_Map_add;
	MODBUS_DPA_REG_MAP *pLast_Reg_Map_add;

	pTmp = aParam;
	//IEC102_DEBUG("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pTmp),pTmp);
	while('\0' != *pTmp)   //遍历查看有多少组数据
	{
		if('{' == *pTmp)
		{
			u8Num++;
		}
		pTmp++;
	}
	if(u8Num <= 1)
		return ENN_SUCCESS;
	
	pTmp = aParam;
	u8Num--;            //获取数据组数

	while('{' != *pTmp)  //MAP={{3,0,1},{3,1,2},{3,2,3},{3,3,4},{3,4,5},{
	{
		pTmp++;
	}
	pTmp++;     //第二个{

	for(i=0; i<u8Num; i++)
	{
		u8Len = 0;
		pTmp = strchr(pTmp, '{');
		if(NULL == pTmp)
		{
			ERR("ERROR :%s, %d, pTmp = NULL !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}
		pTmp++;      //第一个表格第一个数字3,0,1},{3,1,2},{3,2,3},{3,3,4},{3,4,5},{

		pTmp1 = pTmp;
		while('}' != *pTmp1)
		{
			u8Len++;      //u8Len = 5
			pTmp1++;     // 指 向 }
		}

		pBuf = (ENN_CHAR *)malloc(u8Len+1);
		if(NULL == pBuf)
		{
			ERR("Error********* :%s, %d, malloc faile !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}
		memset((void *)pBuf, 0, u8Len+1);
		memcpy((void *)pBuf, (void *)pTmp, u8Len);
		pTmp = pTmp1;      //循环的关键处理的一步，重新定位到下一个{

		pModbus_Reg_Map_add = (MODBUS_DPA_REG_MAP*)malloc(sizeof(MODBUS_DPA_REG_MAP));
		if(NULL == pModbus_Reg_Map_add)
		{
			ERR("Error********* :%s, %d, malloc faile !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}
		memset(pModbus_Reg_Map_add, 0, sizeof(MODBUS_DPA_REG_MAP));
		pModbus_Reg_Map_add->next = NULL;

		pb = strchr(pBuf, ',');
		*pb = '\0';
		pb++;
		pModbus_Reg_Map_add->u8Funcode= atoi(pBuf);

		pc = strchr(pb, ',');
		*pc = '\0';
		pc++;
		pModbus_Reg_Map_add->u16RegAddr= atoi(pb);

		pModbus_Reg_Map_add->u16RegID = atoi(pc);
		//INFO("pc =%s\n",pc);
		
		if(pModbus_Reg_Map == NULL)
		{
			pModbus_Reg_Map = pModbus_Reg_Map_add;
		}else{
			pLast_Reg_Map_add->next = pModbus_Reg_Map_add;
		}
		pLast_Reg_Map_add = pModbus_Reg_Map_add;

		free(pBuf);
		pBuf = NULL;
	}
	return ENN_SUCCESS;
}

/*************************************************************************
*  名字:  ENNModBus_DPA_readConfig
*  说明:  从配置文件中读取Modbus 虚拟映射关系
*  输入参数：void
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
ENN_ErrorCode_t ENNModBus_DPA_readConfig()
{
	ENN_CHAR	aBuffer[IEC102_READ_FILE_BUF_LEN * 2];
	ENN_CHAR	aSection[IEC102_READ_FILE_BUF_LEN];
	ENN_CHAR	aName[IEC102_READ_FILE_BUF_LEN];
	ENN_CHAR	aParam[IEC102_READ_FILE_BUF_LEN * 2];
	ENN_CHAR	*pStr1 = NULL;
	ENN_CHAR	*pStr2 = NULL;
	ENN_CHAR	*cptr = NULL;

	FILE	*fpConfig;
	ENN_ErrorCode_t ret = ENN_SUCCESS;
	int modnum = -1;
	int r = -1;

	fpConfig = fopen("/home/DPA_MODBUS.ini","r");
	if(NULL == fpConfig)
	{
		perror("open configure file fail ");
		return ENN_FAIL;
	}
	while(0 == feof(fpConfig))
	{
		memset(aBuffer, 0, IEC102_READ_FILE_BUF_LEN * 2);
		if(NULL == fgets(aBuffer, IEC102_READ_FILE_BUF_LEN * 2, fpConfig)) 
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
				r = fclose(fpConfig);
				fpConfig = NULL;
				printf("%s, %d, fclose(/home/DPA_MODBUS.ini) ret = %d\n",__FUNCTION__,__LINE__,r);
				return ENN_FAIL;
			}

			while(' ' == *(pStr1-1))
			{
				pStr1--;
			}
			*pStr1 = '\0';	

			memset(aSection, 0, IEC102_READ_FILE_BUF_LEN);
			strcpy(aSection, pStr2);
			//INFO("%s, %d, aSection = %s\n",__FUNCTION__,__LINE__, aSection);
			//ENNModBus_Set_Text(aSection, NULL, NULL);
					
		}
		else if(('#' != *pStr1) && (*pStr1 > ' ')) // name identifier
		{
			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pStr1),pStr1);
			memset(aName, 0, IEC102_READ_FILE_BUF_LEN);
			ret = ENNCopy_Name(aName, pStr1);
			if(ENN_SUCCESS != ret)
			{
				perror("get name fail ");
				r = fclose(fpConfig);
				fpConfig = NULL;
				printf("%s, %d, fclose(/home/DPA_MODBUS.ini) ret = %d\n",__FUNCTION__,__LINE__,r);
				return ENN_FAIL;
			}
			
			INFO("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aName),aName);
			memset(aParam, 0, IEC102_READ_FILE_BUF_LEN * 2);
			ret = ENNCopy_Param(aParam, pStr1);
			if(ENN_SUCCESS != ret)
			{
				perror("get param fail ");
				r = fclose(fpConfig);
				fpConfig = NULL;
				printf("%s, %d, fclose(/home/DPA_MODBUS.ini) ret = %d\n",__FUNCTION__,__LINE__,r);
				return ENN_FAIL;
			}
			INFO("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aParam),aParam);
			if((strcmp(aName, "MAP") == 0)&&(strlen(aParam) >  0))
			{	
				//memcpy(pDEVICE_IP_PARAM->ip, aParam, strlen(aParam));
				ENNModBus_Reg_Map_init(aParam);
			}
		}
	}
	r = fclose(fpConfig);
	fpConfig = NULL;
	printf("%s, %d, fclose(/home/DPA_MODBUS.ini) ret = %d\n",__FUNCTION__,__LINE__,r); 
	
	//test_int();

	return ENN_SUCCESS;
}

ENN_ErrorCode_t _ENNModBus_Read_CoilStatus(ENN_U8 MBFunCode, ENN_U16 u16RegID, ENN_U16 RegNum, ENN_U8 u8Channel,
													ENN_U16 *CoilLen, ENN_U8 *pResData)
{
	ENN_U8 u8FunCode = 0;
	ENN_U8 u8Channelx = 0;
	ENN_U16 u16Offset = 0;
	//ENN_U16 u16RegNum = 0;
	// ENN_U16 u16ByteNum = 0;
	ENN_U16 RegID;
	ENN_CHAR *pData = NULL;

	Register_List *pRegister_List_Temp = NULL;
	Channel_List *pChannel_Temp = NULL;
	FunCode_List *pFunCode_Temp = NULL;
	Slave_List	*pSlave_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;

	ENN_CHAR aTemp[9];
	ENN_CHAR ucTemp = 0;
	ENN_U8 *pTemp = NULL;
	ENN_U16 u16CoilLen = 0;
	ENN_U16 u16Pos = 0;
	ENN_U8 i = 0;
	int u8Len = 0;
	int u16RegNum = 0;

	ENNAPI_ASSERT(NULL != pResData);

	pTemp = pResData;
	aTemp[8] = '\0';
	u16CoilLen = *CoilLen;
	u16RegNum = RegNum;
	ENN_U8 u8ByteValue = 0;
	bool channeloffsetflag = FALSE;

	if(NULL == gFunCode_List_head ||NULL == gChannel_List_Head)
	{
		printf("ERROR : [%s], %d, NULL == gFunCode_List_head ||NULL == gChannel_List_Head\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	pChannel_Temp = gChannel_List_Head;
	while(NULL != pChannel_Temp)
	{
		u8Channelx = pChannel_Temp->u8Channel;
		channeloffsetflag = TRUE;
		if(u8Channel == pChannel_Temp->u8Channel)
		{
			pSlave_Temp = pChannel_Temp->unDeviceType.pModBus_List;
			while(NULL != pSlave_Temp)
			{
				ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,pSlave_Temp->u8SlaveAddr);
				pSlave_FunCode_Temp = pSlave_Temp->pFunCode_List;
				while(NULL != pSlave_FunCode_Temp)
				{
					if((NULL != pSlave_FunCode_Temp->pRegister_List) && 
						(pSlave_FunCode_Temp->u8MBFunCode ==MBFunCode))
					{
						pRegister_List_Temp = pSlave_FunCode_Temp->pRegister_List;
					}
					else{
						pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
						continue;
					}
					//u8FunCode = pSlave_FunCode_Temp->u8MBFunCode;
					pFunCode_Temp = gFunCode_List_head;
					while((pFunCode_Temp != NULL)&&(pFunCode_Temp->u8MBFunCode != MBFunCode))
						pFunCode_Temp = pFunCode_Temp->next;
					if(pFunCode_Temp == NULL)
					{
						IEC102_DEBUG("WARNING : %s, %d, not found pFunCode_Temp \n",__FUNCTION__,__LINE__);
						//pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
						//continue;
						return ENN_FAIL;
					}
					if(channeloffsetflag == TRUE){
						u16Offset = pFunCode_Temp->Offset[u8Channelx];
						channeloffsetflag = FALSE;
					}
					while(NULL != pRegister_List_Temp)
					{
						//u16RegNum = pRegister_List_Temp->u16RegNum;
						RegID = pRegister_List_Temp->u16RegID;
						if(NULL != pFunCode_Temp->pData)
						{
							if(RegID == u16RegID)
							{
								pData = pFunCode_Temp->pData + u16Offset;
								u16Pos = 0;
								while(0 != u16CoilLen)
								{
									pData += u16Pos;
									u8Len = strlen(pData);
									if(u16RegNum < 8)
									{
										u8Len = u8Len < u16RegNum ? u8Len : u16RegNum;
									}
									if(u8Len <= 0)
									{
										return ENN_SUCCESS;
									}
									else	if(u8Len < 8)
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
									u16RegNum -=8;
									u16CoilLen--;
								}
								return ENN_SUCCESS;
							}
							u16Offset++;
						}
						pRegister_List_Temp = pRegister_List_Temp->next;
					}
					pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
				}
				pSlave_Temp = pSlave_Temp->next;
			}
		}
/*		else if((PROTOCOL_645_1997 == pChannel_Temp->u8Protocol) || (PROTOCOL_645_2007 == pChannel_Temp->u8Protocol))
		{
			pFunCode_Temp = gFunCode_List_head;
			while((NULL != pFunCode_Temp)
				&& (MB_FUNC_READ_HOLDING_REGISTER != pFunCode_Temp->u8MBFunCode))
			{
				//printf("%s, %d, u8MBFunCode = %d\n",__FUNCTION__,__LINE__, pFunCode_Temp->u8MBFunCode);
				pFunCode_Temp = pFunCode_Temp->next;
			}

			u16Offset = pFunCode_Temp->Offset[u8Channelx];
			u16Offset = u16Offset * 2;

			pDev645_List_Temp = pChannel_Temp->unDeviceType.pDev645_List;
			while(NULL != pDev645_List_Temp)
			{
				pCode_645_List_Temp = pDev645_List_Temp->pCode_645_List;
				while(NULL != pCode_645_List_Temp)
				{
					RegID = pCode_645_List_Temp->u16RegID;
					
					for(i = 0; i < DevRegNum_Total; i++)
					{
						if(p_Reg_Data[i].u16RegID == RegID)
						{
							memcpy(&p_Reg_Data[i].data,(void *)(pFunCode_Temp->pData + u16Offset), 4);
							p_Reg_Data[i].valid = 1;
							break;
						}
					}
					if(i == DevRegNum_Total)
						IEC102_DEBUG("WARNING : %s, %d, not found p_Reg_Data[i].u16RegID \n",__FUNCTION__,__LINE__);	
					u16Offset = u16Offset + 4;
					pCode_645_List_Temp = pCode_645_List_Temp->next;
				}
				pDev645_List_Temp = pDev645_List_Temp->next;
			}
		}*/
		pChannel_Temp = pChannel_Temp->next;
	}
	return ENN_FAIL;

}


ENN_U8 *_ENNModBus_Serch_FunCode_Data(ENN_U16 u16RegID, ENN_U8 u8Channel, 
															ENN_U8 u8Funcode,ENN_U8 *pDatalen)
{
	ENN_U8 u8FunCode = 0;
	ENN_U8 u8Channelx = 0;
	ENN_U16 u16Offset = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U16 u16ByteNum = 0;
	ENN_U16 RegID;

	Register_List *pRegister_List_Temp = NULL;
	Channel_List *pChannel_Temp = NULL;
	FunCode_List *pFunCode_Temp = NULL;
	Slave_List 	*pSlave_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;

	Dev645_List *pDev645_List_Temp = NULL;
	Code_645_List *pCode_645_List_Temp = NULL;
	bool channeloffsetflag = FALSE;

	if(NULL == gFunCode_List_head ||NULL == gChannel_List_Head)
	{
		printf("ERROR : [%s], %d, NULL == gFunCode_List_head ||NULL == gChannel_List_Head\n",__FUNCTION__,__LINE__);
		return NULL;
	}
	pChannel_Temp = gChannel_List_Head;
	while(NULL != pChannel_Temp)
	{
		u8Channelx = pChannel_Temp->u8Channel;
		channeloffsetflag = TRUE;
		if(u8Channel != pChannel_Temp->u8Channel)
		{
			pChannel_Temp = pChannel_Temp->next;
			continue;
		}
		if(PROTOCOL_MODBUS== pChannel_Temp->u8Protocol)
		{
			pSlave_Temp = pChannel_Temp->unDeviceType.pModBus_List;
			while(NULL != pSlave_Temp)
			{
				ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,pSlave_Temp->u8SlaveAddr);
				pSlave_FunCode_Temp = pSlave_Temp->pFunCode_List;
				while(NULL != pSlave_FunCode_Temp)
				{
					/*if(Existflag & pSlave_FunCode_Temp->u8MBFunCode)
					{
						IEC102_DEBUG("WARNING : %s, %d, not found pFunCode_Temp \n",__FUNCTION__,__LINE__);
					}*/
					pFunCode_Temp = gFunCode_List_head;
					if((NULL != pSlave_FunCode_Temp->pRegister_List) && 
						(pSlave_FunCode_Temp->u8MBFunCode ==u8Funcode))
					{
						pRegister_List_Temp = pSlave_FunCode_Temp->pRegister_List;
					}
					else{
						pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
						continue;
					}
					//u8FunCode = pSlave_FunCode_Temp->u8MBFunCode;
					
					while((pFunCode_Temp != NULL)&&(pFunCode_Temp->u8MBFunCode != u8Funcode))
						pFunCode_Temp = pFunCode_Temp->next;
					if(pFunCode_Temp == NULL)
					{
						IEC102_DEBUG("WARNING : %s, %d, not found pFunCode_Temp \n",__FUNCTION__,__LINE__);
						//pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
						//continue;
						return NULL;
					}
					if(channeloffsetflag == TRUE)
					{
						u16Offset = pFunCode_Temp->Offset[u8Channelx];
						if((MB_FUNC_READ_HOLDING_REGISTER == u8Funcode)  
						|| (MB_FUNC_READ_INPUT_REGISTER == u8Funcode))
						{
							u16Offset = u16Offset * 2;
						}
						else if((MB_FUNC_WRITE_SINGLE_COIL == u8Funcode)  
							 || (MB_FUNC_WRITE_MULTIPLE_REGISTERS == u8Funcode))
						{
							//pFunCode_Temp = pFunCode_Temp->next;
							pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
							continue;
						}
						channeloffsetflag = FALSE;
					}
					if((0x03 == u8Funcode) || (0x04 == u8Funcode)){
						while(NULL != pRegister_List_Temp)
						{
							//IEC102_DEBUG("[%s], %d, Channel = %d, u16Offset = %d\n",__FUNCTION__,__LINE__,u8Channelx,u16Offset);
							RegID = pRegister_List_Temp->u16RegID;

							u16RegNum = pRegister_List_Temp->u16RegNum;
							u16ByteNum = u16RegNum * 2;
							if(NULL != pFunCode_Temp->pData)
							{
								if(RegID == u16RegID){
									*pDatalen = u16ByteNum;
									/*printf("%s, %d, RegID =%d, u16Offset =%d, (pFunCode_Temp->pData + u16Offset)=%2x\n",
										__FUNCTION__,__LINE__,RegID,u16Offset,*(pFunCode_Temp->pData + u16Offset));*/
									return (pFunCode_Temp->pData + u16Offset);
								}
							}
							u16Offset += u16ByteNum;
							//IEC102_DEBUG("[%s], %d, Channel = %d, u16Offset = %d\n",__FUNCTION__,__LINE__,u8Channelx,u16Offset);
							pRegister_List_Temp = pRegister_List_Temp->next;
						}
					}else if((0x01 == u8Funcode) || (0x02 == u8Funcode))
					{
						while(NULL != pRegister_List_Temp)
						{
							//u16RegNum = pRegister_List_Temp->u16RegNum;
							RegID = pRegister_List_Temp->u16RegID;
							if(NULL != pFunCode_Temp->pData)
							{
								if(RegID == u16RegID)
									return (pFunCode_Temp->pData + u16Offset);
								u16Offset++;
							}
							pRegister_List_Temp = pRegister_List_Temp->next;
						}
					}
					//Existflag |= u8FunCode;
					pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
				}
				pSlave_Temp = pSlave_Temp->next;
			}
		}
		else if((PROTOCOL_645_1997 == pChannel_Temp->u8Protocol) || (PROTOCOL_645_2007 == pChannel_Temp->u8Protocol))
		{
			pFunCode_Temp = gFunCode_List_head;
			while((NULL != pFunCode_Temp)
				&& (MB_FUNC_READ_HOLDING_REGISTER != pFunCode_Temp->u8MBFunCode))
			{
				//printf("%s, %d, u8MBFunCode = %d\n",__FUNCTION__,__LINE__, pFunCode_Temp->u8MBFunCode);
				pFunCode_Temp = pFunCode_Temp->next;
			}

			u16Offset = pFunCode_Temp->Offset[u8Channelx];
			u16Offset = u16Offset * 2;

			pDev645_List_Temp = pChannel_Temp->unDeviceType.pDev645_List;
			while(NULL != pDev645_List_Temp)
			{
				pCode_645_List_Temp = pDev645_List_Temp->pCode_645_List;
				while(NULL != pCode_645_List_Temp)
				{
					RegID = pCode_645_List_Temp->u16RegID;

					//u16RegNum = pCode_645_List_Temp->u16RegNum;
					//u16ByteNum = u16RegNum * 2;
					if(NULL != pFunCode_Temp->pData)
					{
						if(RegID == u16RegID){
							*pDatalen = 4;
							return (pFunCode_Temp->pData + u16Offset);
						}
					}
					
					u16Offset = u16Offset + 4;
					pCode_645_List_Temp = pCode_645_List_Temp->next;
				}
				pDev645_List_Temp = pDev645_List_Temp->next;
			}
		}
		pChannel_Temp = pChannel_Temp->next;
	}
	return NULL;

}


ENN_U8 *ENNModBus_Serch_FunCode_Data(ENN_U8 u8FunCode, ENN_U16 u16RegAddr, ENN_U16 u16RegNum, ENN_U16 *DataLen)
{
	FunCode_List *pFunCode_List_Temp = NULL;
	ENN_U8 *pData = NULL;
	ENN_U16 u16Pos = 0;

	pFunCode_List_Temp = gFunCode_List_head;
	while(NULL != pFunCode_List_Temp)
	{
		if(u8FunCode == pFunCode_List_Temp->u8MBFunCode)
		{
			if((u16RegAddr < pFunCode_List_Temp->u16StartAddr)
			|| (u16RegAddr + u16RegNum > pFunCode_List_Temp->u16EndAddr))
			{
				*DataLen = 0x02;
				return NULL;
			}
			
			ENNTRACE("%s, %d, %d, %d, %d\n",__FUNCTION__,__LINE__,u16RegAddr, u16RegNum, pFunCode_List_Temp->u16StartAddr);
			u16Pos = (u16RegAddr - pFunCode_List_Temp->u16StartAddr) * 2;
			ENNTRACE("%s, %d, u16Pos = %d\n",__FUNCTION__,__LINE__,u16Pos);
			*DataLen = u16RegNum * 2;
			ENNTRACE("%s, %d, *DataLen = %d\n",__FUNCTION__,__LINE__,*DataLen);
			return (pFunCode_List_Temp->pData + u16Pos);
		}

		pFunCode_List_Temp = pFunCode_List_Temp->next;
	}

	*DataLen = 0x01;
	return NULL;
}


ENN_ErrorCode_t ENNModBus_Func_Exception(ENN_S32 socket, ENN_U8 u8Exception, ENN_U8 *pBuf)
{
	ENN_U16 u16Num = 0;
	ENN_U16 u16ModBusTCPLen = 0;
	ENN_U8 *pSendBuf = NULL;
	ENN_U8 i = 0;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;

	ENNAPI_ASSERT(NULL != pBuf);
	
	u16Num = 2 + 1;
	u16ModBusTCPLen = 7 + 1 + 1;
	pSendBuf = (ENN_U8 *)malloc(u16ModBusTCPLen);
	if(NULL == pSendBuf)
	{
		return ENN_MEM_MALLOC_FAIL;
	}
	memset(pSendBuf, 0, u16ModBusTCPLen);
	memcpy(pSendBuf, pBuf, 7);
	pSendBuf[4] = (ENN_U8)(u16Num >> 8);
	pSendBuf[5] = (ENN_U8)(u16Num & 0x00FF);
	*(pSendBuf+7) = *(pBuf+7) | 0x80;
	*(pSendBuf+8) = u8Exception;
	
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16ModBusTCPLen);

	for(i = 0; i < u16ModBusTCPLen; i++)
		ENNTRACE("%2.2x ", pSendBuf[i]);
	ENNTRACE("\n");

	returnCode = ENNModBus_TCP_Send(socket, pSendBuf, u16ModBusTCPLen);
	if(ENN_SUCCESS != returnCode)
	{
		free(pSendBuf);
		pSendBuf = NULL;
	}

	return returnCode;
}


ENN_ErrorCode_t ENNModBus_Func_Read_Coils_Or_DiscreteInputs(ENN_S32 socket, ENN_U8 *pBuf,
																	ENN_U16 RegID, ENN_U8 Channel, ENN_U8 Funcode)
{
	ENN_U16 u16RegAddr = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U16 u16CoilLen = 0;
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 i = 0;
	ENN_U8 u8ExceptionCode = 0;
	ENN_U16 u16Num = 0;
	ENN_U16 u16ModBusTCPLen = 0;
	ENN_U8 *pData = NULL;
	ENN_U8 *pResData = NULL;
	ENN_U8 *pSendBuf = NULL;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;

	ENNAPI_ASSERT(NULL != pBuf);
	pData = pBuf;

	Len = 7;
	u8FunCode = pData[Len++];
	u16RegAddr = ((ENN_U16)(pData[Len++])) << 8;
	u16RegAddr |= (ENN_U16)(pData[Len++]);
	
	u16RegNum = ((ENN_U16)(pData[Len++])) << 8;
	u16RegNum |= (ENN_U16)(pData[Len++]);
	u16CoilLen = u16RegNum/8;
	if(0 != u16RegNum%8)
	{
		u16CoilLen++;
	}
	
	if((u16RegNum < 0x0001) || (u16RegNum > 0x07D0))
	{
		//0x03
		returnCode = ENNModBus_Func_Exception(socket, 0x03, pBuf);
		return returnCode;
	}

	pResData = (ENN_U8 *)malloc(u16CoilLen);
	if(NULL == pResData)
	{
		return ENN_MEM_MALLOC_FAIL;
	}
	memset(pResData, 0, u16CoilLen);
	
	//returnCode = ENNModBus_Read_CoilStatus(u8FunCode, u16RegAddr, u16RegNum, &u16CoilLen, pResData);
	returnCode = _ENNModBus_Read_CoilStatus(Funcode, RegID, u16RegNum, Channel, 
											&u16CoilLen, pResData);
	for(i = 0; i < u16CoilLen; i++)
			ENNTRACE("%2.2X ",pResData[i]);
	ENNTRACE("\n");
	
	if(ENN_SUCCESS!= returnCode)
	{
		free(pResData);
		pResData = NULL;
		u8ExceptionCode = 0x01;
		returnCode = ENNModBus_Func_Exception(socket, u8ExceptionCode, pBuf);
	}
	else
	{
		u16Num = u16CoilLen + 1 + 1 + 1;
		u16ModBusTCPLen = 7 + 1 + 1 + u16CoilLen;
		pSendBuf = (ENN_U8 *)malloc(u16ModBusTCPLen);
		if(NULL == pSendBuf)
		{
			free(pResData);
			pResData = NULL;
			return ENN_MEM_MALLOC_FAIL;
		}
		memset(pSendBuf, 0, u16ModBusTCPLen);
		memcpy(pSendBuf, pData, 7);
		pSendBuf[4] = (ENN_U8)(u16Num>>8);
		pSendBuf[5] = (ENN_U8)(u16Num & 0x00FF);
		*(pSendBuf+7) = u8FunCode;
		pSendBuf[8] = (ENN_U8)u16CoilLen;
		memcpy(pSendBuf+9, pResData, u16CoilLen);

		free(pResData);
		pResData = NULL;
		
		ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);

		for(i = 0; i < (u16CoilLen + 1 + 7 + 1); i++)
			ENNTRACE("%2.2x ", pSendBuf[i]);
		ENNTRACE("\n");

		/*err = ENNSock_SocketSend(stDATAQ_FRAME.frame_net_info.u32Socket, pSendBuf, u16ModBusTCPLen, 1);
		if(err <= 0)
		{
			free(pSendBuf);
			pSendBuf = NULL;
		}*/
		returnCode = ENNModBus_TCP_Send(socket, pSendBuf, u16ModBusTCPLen);
		if(ENN_SUCCESS != returnCode)
		{
			free(pSendBuf);
			pSendBuf = NULL;
		}
	}

	return returnCode;
}

ENN_ErrorCode_t _ENNModBus_Func_Read_Holding_Or_Input_Register(ENN_S32 socket, 
												ENN_U8 *pBuf, ENN_U16 RegID, ENN_U8 Channel, ENN_U8 Funcode,
												MODBUS_DPA_REG_MAP *pCur_Reg_Map)
{
	ENN_U16 u16RegAddr = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U16 u16CoilLen = 0;
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 i = 0;
	ENN_U8 u8ExceptionCode = 0;
	ENN_U16 u16Num = 0;
	ENN_U16 u16ModBusTCPLen = 0;
	ENN_U16 u16DataLen = 0;
	ENN_U8 *pData = NULL;
	ENN_U8 *pResData = NULL;
	ENN_U8 *pSendBuf = NULL;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;

	MODBUS_DPA_REG_MAP *Cur_Reg_Map;
	//ENN_U16 RegID;
	ENN_U8 RegDatalen = 0;
	ENN_U8 *data = NULL;
	int len = 0;
	ENN_U16 regID;
	ENN_U8 u8Channel;
	ENN_U8 funCode;
	ENN_U32 RegAddr;
	ENN_U8 u8SlaveAddr;
	

	ENNAPI_ASSERT(NULL != pBuf);
	pData = pBuf;
	Cur_Reg_Map = pCur_Reg_Map;

	Len = 7;
	u8FunCode = pData[Len++];
	u16RegAddr = ((ENN_U16)(pData[Len++])) << 8;
	u16RegAddr |= (ENN_U16)(pData[Len++]);
	//u8FunCode = FunCode;
	//u16RegAddr = RegAddr;
	
	u16RegNum = ((ENN_U16)(pData[Len++])) << 8;
	u16RegNum |= (ENN_U16)(pData[Len++]);
	printf("%s, %d, %d\n",__FUNCTION__,__LINE__, u16RegNum);

	u16DataLen = u16RegNum * 2;//长度200
	data = (ENN_U8 *)malloc(u16DataLen);
	if(NULL == data)
	{
		printf("ERROR : [%s], %d, malloc fail \n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	memset(data, 0, u16DataLen);
	

	if((u16RegNum < 0x0001) || (u16RegNum > 0x007D))
	{
		returnCode = ENNModBus_Func_Exception(socket, 0x03, pBuf);
		return returnCode;
	}

	pResData = _ENNModBus_Serch_FunCode_Data(RegID, Channel, Funcode, &RegDatalen);
	if(NULL == pResData)
	{
		printf("ERROR: %s, %d\n",__FUNCTION__,__LINE__);
		u8ExceptionCode = 0x02;
		returnCode = ENNModBus_Func_Exception(socket, u8ExceptionCode, pBuf);
		return returnCode;
	}
	memcpy(data, pResData, RegDatalen);
#if 0
	printf("%s, %d, RegDatalen =%d, data =",__FUNCTION__,__LINE__,RegDatalen);
	for(i =0; i<RegDatalen; i++)
		printf("%2x ",*(pResData + i));
	printf("\r\n");
#endif	
	len += RegDatalen;
	if(len >= u16DataLen)
		goto TCP_SEND;

	u16RegAddr = Cur_Reg_Map->u16RegAddr;
	for(i = 1; i < u16RegNum; i++)
	{
		//u16RegAddr = Cur_Reg_Map->u16RegAddr;
		if(Cur_Reg_Map->next == NULL)
			goto TCP_SEND;
		if(++u16RegAddr != Cur_Reg_Map->next->u16RegAddr)
		{
			len += 2;
			ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__);
			if(len >= u16DataLen)
				goto TCP_SEND;
			continue;
		}
		Cur_Reg_Map = Cur_Reg_Map->next;
		
		regID = Cur_Reg_Map->u16RegID;
		returnCode = ENNModBus_get_RegInfo(regID, &u8Channel, &u8SlaveAddr, &funCode, &RegAddr);
		if(ENN_SUCCESS != returnCode)
		{
			ENNModBus_Func_Exception(socket, 0x02, pBuf);
			printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
			return ENN_FAIL;
		}		
		ENNTRACE("[%s], %d, u8Channel =%d, RegID =%d\n",__FUNCTION__,__LINE__,u8Channel,regID);
		pResData = _ENNModBus_Serch_FunCode_Data(regID, u8Channel, funCode, &RegDatalen);
		if(NULL == pResData)
		{
			u8ExceptionCode = 0x02;
			returnCode = ENNModBus_Func_Exception(socket, u8ExceptionCode, pBuf);
			return returnCode;
		}
#if 0	
		int j= 0;
		for(j; j<RegDatalen; j ++)
			printf("%2x ",*(pResData + j));
		printf("\r\n");
#endif		
		memcpy((data + len), pResData, RegDatalen);
		len += RegDatalen;
		if(len >= u16DataLen)
			goto TCP_SEND;
	}
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__);
	for(i = 0; i < u16DataLen; i++)
		ENNTRACE("%2.2x ", pResData[i]);
	ENNTRACE("\n");

TCP_SEND:
	u16Num = u16DataLen + 1 + 1 + 1;
	u16ModBusTCPLen = 7 + 1 + 1 + u16DataLen;
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16ModBusTCPLen);
	pSendBuf = (ENN_U8 *)malloc(u16ModBusTCPLen);
	if(NULL == pSendBuf)
	{
		return ENN_MEM_MALLOC_FAIL;
	}
	memset(pSendBuf, 0, u16ModBusTCPLen);
	memcpy(pSendBuf, pData, 7);
	pSendBuf[4] = (ENN_U8)(u16Num >> 8);
	pSendBuf[5] = (ENN_U8)(u16Num & 0x00FF);
	*(pSendBuf+7) = u8FunCode;
	pSendBuf[8] = (ENN_U8)u16DataLen;
	memcpy(pSendBuf+9, data, u16DataLen);
	
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16ModBusTCPLen);

	for(i = 0; i < u16ModBusTCPLen; i++)
		ENNTRACE("%2.2x ", pSendBuf[i]);
	ENNTRACE("\n");

	returnCode = ENNModBus_TCP_Send(socket, pSendBuf, u16ModBusTCPLen);
	if(ENN_SUCCESS != returnCode)
	{
		printf("ERROR : [%s], %d, ENNModBus_TCP_Send() fail \n",__FUNCTION__,__LINE__);
	}
	free(pSendBuf);
	pSendBuf = NULL;
	free(data);
	data =NULL;

	return returnCode;
}



ENN_ErrorCode_t ENNModBus_Func_Read_Holding_Or_Input_Register(ENN_S32 socket, ENN_U8 *pBuf, 																		
																		ENN_U16 RegID, ENN_U8 Channel, ENN_U8 Funcode)
{
	ENN_U16 u16RegAddr = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U16 u16CoilLen = 0;
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 i = 0;
	ENN_U8 u8ExceptionCode = 0;
	ENN_U16 u16Num = 0;
	ENN_U16 u16ModBusTCPLen = 0;
	ENN_U16 u16DataLen = 0;
	ENN_U8 *pData = NULL;
	ENN_U8 *pResData = NULL;
	ENN_U8 *pSendBuf = NULL;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;

	MODBUS_DPA_REG_MAP *pCur_Reg_Map;
	//ENN_U16 RegID;
	ENN_U8 datalen;

	ENNAPI_ASSERT(NULL != pBuf);
	pData = pBuf;

	Len = 7;
	u8FunCode = pData[Len++];
	u16RegAddr = ((ENN_U16)(pData[Len++])) << 8;
	u16RegAddr |= (ENN_U16)(pData[Len++]);
	//u8FunCode = FunCode;
	//u16RegAddr = RegAddr;
	
	u16RegNum = ((ENN_U16)(pData[Len++])) << 8;
	u16RegNum |= (ENN_U16)(pData[Len++]);
	ENNTRACE("%s, %d, %d, %d\n",__FUNCTION__,__LINE__,u16RegAddr, u16RegNum);

	if((u16RegNum < 0x0001) || (u16RegNum > 0x007D))
	{
		returnCode = ENNModBus_Func_Exception(socket, 0x03, pBuf);
		return returnCode;
	}

/*	for(i = 0; i < DevRegNum_Total; i++)
	{
		if(p_Reg_Data[i].u16RegID == RegID)
		{
			memcpy(&p_Reg_Data[i].data, (void *)(pFunCode_Temp->pData + u16Offset), u16ByteNum);
			p_Reg_Data[i].valid = 1;
			break;
		}
	}
	if(i == DevRegNum_Total)
		IEC102_DEBUG("WARNING : %s, %d, not found p_Reg_Data[i].u16RegID \n",__FUNCTION__,__LINE__);	
*/
	pResData = _ENNModBus_Serch_FunCode_Data(RegID, Channel, Funcode, &datalen);

	//pResData = ENNModBus_Serch_FunCode_Data(u8FunCode, u16RegAddr, u16RegNum, &u16DataLen);
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16DataLen);
	if(NULL == pResData)
	{
		u8ExceptionCode = 0x02;
		returnCode = ENNModBus_Func_Exception(socket, u8ExceptionCode, pBuf);
		return returnCode;
	}

	u16DataLen = u16RegNum * 2;

	for(i = 0; i < u16DataLen; i++)
		ENNTRACE("%2.2x ", pResData[i]);
	ENNTRACE("\n");

	u16Num = u16DataLen + 1 + 1 + 1;
	u16ModBusTCPLen = 7 + 1 + 1 + u16DataLen;
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16ModBusTCPLen);
	pSendBuf = (ENN_U8 *)malloc(u16ModBusTCPLen);
	if(NULL == pSendBuf)
	{
		return ENN_MEM_MALLOC_FAIL;
	}
	memset(pSendBuf, 0, u16ModBusTCPLen);
	memcpy(pSendBuf, pData, 7);
	pSendBuf[4] = (ENN_U8)(u16Num >> 8);
	pSendBuf[5] = (ENN_U8)(u16Num & 0x00FF);
	*(pSendBuf+7) = u8FunCode;
	pSendBuf[8] = (ENN_U8)u16DataLen;
	memcpy(pSendBuf+9, pResData, u16DataLen);
	
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16ModBusTCPLen);

	for(i = 0; i < u16ModBusTCPLen; i++)
		ENNTRACE("%2.2x ", pSendBuf[i]);
	ENNTRACE("\n");

	returnCode = ENNModBus_TCP_Send(socket, pSendBuf, u16ModBusTCPLen);
	if(ENN_SUCCESS != returnCode)
	{
		free(pSendBuf);
		pSendBuf = NULL;
	}

	return returnCode;
}


ENN_ErrorCode_t ENNModBus_Func_Write_Single_Coil(ENN_S32 socket, ENN_U8 *pBuf,ENN_U16 RegID, ENN_U8 Channel)
{
	ENN_U16 u16RegAddr = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U16 u16CoilLen = 0;
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 i = 0;
	ENN_U8 u8ExceptionCode = 0;
	ENN_U16 u16Num = 0;
	ENN_U16 u16ModBusTCPLen = 0;
	ENN_U16 u16DataLen = 0;
	ENN_U16 u16InputValue = 0;
	ENN_U8 *pData = NULL;
	ENN_U8 *pResData = NULL;
	ENN_U8 *pSendBuf = NULL;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;

	ENNAPI_ASSERT(NULL != pBuf);
	pData = pBuf;

	Len = 7;
	u8FunCode = pData[Len++];

	u16RegAddr = ((ENN_U16)(pData[Len++])) << 8;
	u16RegAddr |= (ENN_U16)(pData[Len++]);
	u16InputValue = ((ENN_U16)(pData[Len++])) << 8;
	u16InputValue |= (ENN_U16)(pData[Len++]);

	if((0x0000 != u16InputValue) && (0xFF00 != u16InputValue))
	{
		returnCode = ENNModBus_Func_Exception(socket, 0x03, pBuf);
		return returnCode;
	}
	printf("%s, %d\n",__FUNCTION__,__LINE__);

	returnCode = ENNModBus_Write_Single_Coil(RegID, Channel, u16InputValue);
	ENNWRITETRACE("%s, %d, returnCode = %d\n",__FUNCTION__,__LINE__, returnCode);
	if(ENN_ERR_INVALID_FUNCODE == returnCode)
	{
		u8ExceptionCode = 0x01;
		returnCode = ENNModBus_Func_Exception(socket, u8ExceptionCode, pBuf);
	}
	else if(ENN_MODBUS_ERROR== returnCode)
	{
		u8ExceptionCode = 0x04;
		returnCode = ENNModBus_Func_Exception(socket, u8ExceptionCode, pBuf);
	}
	else if(ENN_SUCCESS == returnCode)
	{
		u16Num = 5 + 1;
		u16ModBusTCPLen = 7 + 5;
		pSendBuf = (ENN_U8 *)malloc(u16ModBusTCPLen);
		if(NULL == pSendBuf)
		{
			return ENN_MEM_MALLOC_FAIL;
		}
		memset(pSendBuf, 0, u16ModBusTCPLen);
		memcpy(pSendBuf, pData, 7);
		pSendBuf[4] = (ENN_U8)(u16Num >> 8);
		pSendBuf[5] = (ENN_U8)(u16Num & 0x00FF);
		memcpy(pSendBuf+7, pData+7, 5);
		
		ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16ModBusTCPLen);

		for(i = 0; i < u16ModBusTCPLen; i++)
			ENNTRACE("%2.2x ", pSendBuf[i]);
		ENNTRACE("\n");

		returnCode = ENNModBus_TCP_Send(socket, pSendBuf, u16ModBusTCPLen);
		if(ENN_SUCCESS != returnCode)
			printf("ERROR :%s, %d\n",__FUNCTION__,__LINE__);

		free(pSendBuf);
		pSendBuf = NULL;
		
	}
	else
	{
		perror("mem malloc fail or modbus error");
	}

	return returnCode;
}

ENN_ErrorCode_t ENNModBus_Func_Write_Single_Register(ENN_S32 socket, ENN_U8 *pBuf,
																ENN_U16 RegID, ENN_U8 Channel, ENN_U8 u8SlaveAddr, ENN_U16 u16RegAddr)
{
	Channel_List *pChannel_Temp = NULL;
	//ENN_U16 u16RegAddr = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U16 u16CoilLen = 0;
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 i = 0;
	ENN_U8 u8ExceptionCode = 0;
	ENN_U16 u16Num = 0;
	ENN_U16 u16ModBusTCPLen = 0;
	ENN_U16 u16DataLen = 0;
	ENN_U16 u16InputValue = 0;
	ENN_U8 *pData = NULL;
	ENN_U8 pReqData[4];
	ENN_U8 *pSendBuf = NULL;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	ENN_U8 u8SlaveAddr1 = 0;
	ENN_U16 len = 0;
	ENN_U8 data[512];
	ENN_U8 u8FunCode1 = 0;

	ENNAPI_ASSERT(NULL != pBuf);
	pData = pBuf;

	pChannel_Temp = gChannel_List_Head;
	while(NULL != pChannel_Temp)
	{
		//u8Channel = pChannel_Temp->u8Channel;
		if(Channel == pChannel_Temp->u8Channel){
			break;
		}
		pChannel_Temp = pChannel_Temp->next;
	}

	Len = 7;
	u8FunCode = pData[Len++];
	//u16RegAddr = ((ENN_U16)(pData[Len++])) << 8;
	//u16RegAddr |= (ENN_U16)(pData[Len++]);
	Len += 2;
	//u16InputValue = ((ENN_U16)(pData[Len++])) << 8;
	//u16InputValue |= (ENN_U16)(pData[Len++]);

	memset(pReqData, 0, 4);
	pReqData[0] = (ENN_U8)(u16RegAddr/256); 
	pReqData[1] = (ENN_U8)(u16RegAddr & 0x00FF);
	pReqData[2] = pData[Len++];
	pReqData[3] = pData[Len++];

	printf("[%s], %d, pReqData[0]=%2x, pReqData[1]=%2x\n",__FUNCTION__,__LINE__,pReqData[0], pReqData[1]);

	//returnCode = ENNModBus_Write_Single_Coil(RegID, Channel, u16InputValue);
	
	//function MB_FUNC_WRITE_SINGLE_REGISTER	6
	pthread_mutex_lock(&MBS_mutexs[Channel]);
	//写操作
	ENNModBus_Set_IdelTime(pChannel_Temp->u32BaudRate);
	ENNModBus_Sleep(Modbus_Idletime);
	returnCode = ENNModBus_Write(Channel, u8SlaveAddr, MB_FUNC_WRITE_SINGLE_REGISTER, pReqData, 4);
	ENNWRITETRACE("%s, %d, returnCode = %d\n",__FUNCTION__,__LINE__, returnCode);
	if(ENN_SUCCESS != returnCode)
	{
		u8ExceptionCode = returnCode;
		returnCode = ENNModBus_Func_Exception(socket, u8ExceptionCode, pBuf);
	}
	ENNModBus_Sleep(Modbus_Idletime);
	len = ENNModBus_Response(Channel, &u8SlaveAddr1, &u8FunCode1, data);
	printf("[%s], %d, Channel = %d, len = %d\n",__FUNCTION__,__LINE__,Channel, len);
	pthread_mutex_unlock(&MBS_mutexs[Channel]);

	if(len >= 7)
	{
		u16Num = 5 + 1;
		u16ModBusTCPLen = 7 + 5;
		pSendBuf = (ENN_U8 *)malloc(u16ModBusTCPLen);
		if(NULL == pSendBuf)
		{
			return ENN_MEM_MALLOC_FAIL;
		}
		memset(pSendBuf, 0, u16ModBusTCPLen);
		memcpy(pSendBuf, pData, 7);;
		pSendBuf[4] = (ENN_U8)(u16Num >> 8);
		pSendBuf[5] = (ENN_U8)(u16Num & 0x00FF);
		memcpy(pSendBuf+7, pData+7, 5);
		//pSendBuf[u16Num++] = u8SlaveAddr1;
		//pSendBuf[u16Num++] = u8FunCode1;
		//memcpy(&pSendBuf[u16Num], data+2, 4);
		
		ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16ModBusTCPLen);

		for(i = 0; i < u16ModBusTCPLen; i++)
			ENNTRACE("%2.2x ", pSendBuf[i]);
		ENNTRACE("\n");

		returnCode = ENNModBus_TCP_Send(socket, pSendBuf, u16ModBusTCPLen);
		if(ENN_SUCCESS != returnCode)
			printf("ERROR :%s, %d\n",__FUNCTION__,__LINE__);

		free(pSendBuf);
		pSendBuf = NULL;
	}
	else
	{
		perror("modbus response error");
		return ENN_FAIL;
	}

	return returnCode;
}


ENN_ErrorCode_t ENNModBus_Func_Write_Multiple_Register(ENN_S32 socket, ENN_U8 *pBuf, ENN_U16 RegID, ENN_U8 Channel)
{
	ENN_U16 u16RegAddr = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U16 u16CoilLen = 0;
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 i = 0;
	ENN_U8 u8ExceptionCode = 0;
	ENN_U16 u16Num = 0;
	ENN_U16 u16ModBusTCPLen = 0;
	ENN_U16 u16DataLen = 0;
	ENN_U16 u16InputValue = 0;
	ENN_U8 u8Bytes = 0;
	ENN_U8 *pData = NULL;
	ENN_U8 *pResData = NULL;
	ENN_U8 *pSendBuf = NULL;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;

	ENNAPI_ASSERT(NULL != pBuf);
	pData = pBuf;

	Len = 7;
	u8FunCode = pData[Len++];

	u16RegAddr = ((ENN_U16)(pData[Len++])) << 8;
	u16RegAddr |= (ENN_U16)(pData[Len++]);
	
	u16RegNum = ((ENN_U16)(pData[Len++])) << 8;
	u16RegNum |= (ENN_U16)(pData[Len++]);
	u8Bytes = pData[Len++];	
	
	if((u16RegNum < 0x0001) || (u16RegNum > 0x007B) ||(u8Bytes != (u16RegNum * 2)))
	{
		returnCode = ENNModBus_Func_Exception(socket, 0x03, pBuf);
		return returnCode;
	}

	returnCode = ENNModBus_Write_Multiple_Register(RegID, Channel, u16RegNum, u8Bytes, &pData[Len]);
	ENNWRITETRACE("%s, %d, returnCode = %d\n",__FUNCTION__,__LINE__, returnCode);
	if(ENN_ERR_INVALID_FUNCODE == returnCode)
	{
		u8ExceptionCode = 0x01;
		returnCode = ENNModBus_Func_Exception(socket, u8ExceptionCode, pBuf);
	}
	else if(ENN_ERR_INVALID_REG_ADDR == returnCode)
	{
		u8ExceptionCode = 0x02;
		returnCode = ENNModBus_Func_Exception(socket, u8ExceptionCode, pBuf);
	}
	else if(ENN_SUCCESS == returnCode)
	{
		u16Num = 5 + 1;
		u16ModBusTCPLen = 7 + 5;
		pSendBuf = (ENN_U8 *)malloc(u16ModBusTCPLen);
		if(NULL == pSendBuf)
		{
			return ENN_MEM_MALLOC_FAIL;
		}
		memset(pSendBuf, 0, u16ModBusTCPLen);
		memcpy(pSendBuf, pData, 7);
		pSendBuf[4] = (ENN_U8)(u16Num >> 8);
		pSendBuf[5] = (ENN_U8)(u16Num & 0x00FF);
		memcpy(pSendBuf+7, pData+7, 5);
		
		ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16ModBusTCPLen);

		for(i = 0; i < u16ModBusTCPLen; i++)
			ENNTRACE("%2.2x ", pSendBuf[i]);
		ENNTRACE("\n");

		returnCode = ENNModBus_TCP_Send(socket, pSendBuf, u16ModBusTCPLen);
		if(ENN_SUCCESS != returnCode)
			printf("ERROR :%s, %d\n",__FUNCTION__,__LINE__);

		free(pSendBuf);
		pSendBuf = NULL;
	}

	return returnCode;
}


ENN_ErrorCode_t ENNModBus_Func_Read_History_Data(ENN_S32 socket, ENN_U8 *pBuf)
{
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U16 u16DataLen = 0;
	ENN_U16 u16Num = 0;
	ENN_U16 u16ModBusTCPLen = 0;
	ENN_U8 i = 0;
	ENN_U8 *pData = NULL;
	ENN_U8 *pResData = NULL;
	ENN_U8 *pSendBuf = NULL;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;

	ENNAPI_ASSERT(NULL != pBuf);
	pData = pBuf;
	
	Len = 7;
	u8FunCode = pBuf[Len++];

	u16RegNum = ((ENN_U16)(pData[Len++])) << 8;
	u16RegNum |= (ENN_U16)(pData[Len++]);

	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16RegNum);
	
	pResData = (ENN_U8 *)malloc(u16RegNum);
	if(NULL == pResData)
	{
		return ENN_MEM_MALLOC_FAIL;
	}
	memset(pResData, 0, u16RegNum);
	
	returnCode = ENNModBus_Get_History_Data(pResData, u16RegNum, &u16DataLen);
	ENNTRACE("%s, %d, %d, %d\n",__FUNCTION__,__LINE__,u16DataLen, returnCode);
	if((ENN_SUCCESS != returnCode) || (0 == u16DataLen) || (NULL == pResData))
	{
		free(pResData);
		pResData = NULL;
		return ENN_MEM_MALLOC_FAIL;
	}
	
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16DataLen);
	
	u16Num = u16DataLen + 2 + 1 + 1;
	u16ModBusTCPLen = 7 + 1 + 2 + u16DataLen;
	pSendBuf = (ENN_U8 *)malloc(u16ModBusTCPLen);
	if(NULL == pSendBuf)
	{
		free(pResData);
		pResData = NULL;
		return ENN_MEM_MALLOC_FAIL;
	}
	memcpy(pSendBuf, pData, 7);
	pSendBuf[4] = (ENN_U8)(u16Num >> 8);
	pSendBuf[5] = (ENN_U8)(u16Num & 0x00FF);
	*(pSendBuf+7) = u8FunCode;
	pSendBuf[8] = (ENN_U8)(u16DataLen >> 8);
	pSendBuf[9] = (ENN_U8)(u16DataLen & 0x00FF);
	for(i = 0; i < u16DataLen; i++)
		ENNTRACE("%2.2x ", pResData[i]);
	ENNTRACE("\n");

	memcpy(pSendBuf+10, pResData, u16DataLen);
	free(pResData);
	pResData = NULL;
	
	//ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);

	for(i = 0; i < u16ModBusTCPLen; i++)
		ENNTRACE("%2.2x ", pSendBuf[i]);
	ENNTRACE("\n");

	/*err = ENNSock_SocketSend(stDATAQ_FRAME.frame_net_info.u32Socket, pSendBuf, u16ModBusTCPLen, 1);
	ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,err);
	perror("send history data: ");
	if(err <= 0)
	{
		free(pSendBuf);
		pSendBuf = NULL;
	}*/
	returnCode = ENNModBus_TCP_Send(socket, pSendBuf, u16ModBusTCPLen);
	if(ENN_SUCCESS != returnCode)
	{
		free(pSendBuf);
		pSendBuf = NULL;
	}
	
	return returnCode;
}

ENN_ErrorCode_t ENNModBus_Read_VirtReg_to_DevStatus(ENN_S32 socket, ENN_U16 u16RegNum, ENN_U8 *pBuf)
{
	ENN_U16 u16RegAddr = 0;
	//ENN_U16 u16RegNum = 0;
	ENN_U16 u16CoilLen = 0;
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 i = 0;
	ENN_U8 u8ExceptionCode = 0;
	ENN_U16 u16Num = 0;
	ENN_U16 u16ModBusTCPLen = 0;
	ENN_U8 *pData = NULL;
	ENN_U8 *pResData = NULL;
	ENN_U8 *pSendBuf = NULL;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;

	ENN_U8 DevsByteLen = 0;
	int counter = 0;
	Channel_List *pChannel_Temp = NULL;
	Slave_List	*pSlave_Temp = NULL;

	Dev645_List *pDev645_List_Temp = NULL;

	if((DevCount < 1) ||(pBuf == NULL) ||(u16RegNum < 0x0001) || (u16RegNum > 0x07D0))
	{
		//0x03
		returnCode = ENNModBus_Func_Exception(socket, 0x03, pBuf);
		return returnCode;
	}
	pData = pBuf;

	Len = 7;
	u8FunCode = pData[Len++];

	DevsByteLen = DevCount/8;
	if(0 != DevCount%8)
	{
		DevsByteLen++;
	}

	//u16CoilLen = u16RegNum * 2;
	u16CoilLen = u16RegNum;

	pResData = (ENN_U8 *)malloc(u16CoilLen);
	if(NULL == pResData)
	{
		return ENN_MEM_MALLOC_FAIL;
	}
	memset(pResData, 0, u16CoilLen);

	//if(DevsByteLen <= u16CoilLen)
	{
		pChannel_Temp = gChannel_List_Head;
		while(NULL != pChannel_Temp)
		{
			if(PROTOCOL_MODBUS == pChannel_Temp->u8Protocol)
			{
				pSlave_Temp = pChannel_Temp->unDeviceType.pModBus_List;
				while(NULL != pSlave_Temp)
				{
					//printf("%s, %d, counter =%d\n",__FUNCTION__,__LINE__,counter);
					pResData[counter / 8] |= (pSlave_Temp->u16Status) << (counter % 8);
					counter ++;
					if((counter == DevCount) ||(counter == u16CoilLen *8))
						goto Send;
					pSlave_Temp = pSlave_Temp->next;
				}
			}
			else if((PROTOCOL_645_1997 == pChannel_Temp->u8Protocol) || (PROTOCOL_645_2007 == pChannel_Temp->u8Protocol))
			{
				pDev645_List_Temp = pChannel_Temp->unDeviceType.pDev645_List;
				while(NULL != pDev645_List_Temp)
				{
					//printf("%s, %d, counter =%d\n",__FUNCTION__,__LINE__,counter);
					pResData[counter / 8] |= (pDev645_List_Temp->u16Status) << (counter % 8);
					counter ++;
					if((counter == DevCount) ||(counter == u16CoilLen *8))
						goto Send;
					pDev645_List_Temp = pDev645_List_Temp->next;
				}
			}
			pChannel_Temp = pChannel_Temp->next;
		}
	}
	
Send:	
	{
		for(i = 0; i < u16CoilLen; i++)
			printf("%2.2X ",pResData[i]);
		printf("\n");
		u16Num = u16CoilLen + 1 + 1 + 1;
		u16ModBusTCPLen = 7 + 1 + 1 + u16CoilLen;
		pSendBuf = (ENN_U8 *)malloc(u16ModBusTCPLen);
		if(NULL == pSendBuf)
		{
			free(pResData);
			pResData = NULL;
			return ENN_MEM_MALLOC_FAIL;
		}
		memset(pSendBuf, 0, u16ModBusTCPLen);
		memcpy(pSendBuf, pData, 7);
		pSendBuf[4] = (ENN_U8)(u16Num>>8);
		pSendBuf[5] = (ENN_U8)(u16Num & 0x00FF);
		*(pSendBuf+7) = u8FunCode;
		pSendBuf[8] = (ENN_U8)u16CoilLen;
		memcpy(pSendBuf+9, pResData, u16CoilLen);

		free(pResData);
		pResData = NULL;
		
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);

		for(i = 0; i < (u16CoilLen + 1 + 7 + 1); i++)
			printf("%2.2x ", pSendBuf[i]);
		printf("\n");
		
		returnCode = ENNModBus_TCP_Send(socket, pSendBuf, u16ModBusTCPLen);
		if(ENN_SUCCESS != returnCode)
		{
			printf("ERROR :%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		}
		free(pSendBuf);
		pSendBuf = NULL;
	}

	return returnCode;
}



ENN_ErrorCode_t _ENNModBus_Func_Parser(ENN_S32 socket, ENN_U8 *pBuf)
{
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	ENN_U16 u16RegAddr;
	ENN_U16 u16RegNum;

	MODBUS_DPA_REG_MAP *pCur_Reg_Map;
	ENN_U16 RegID;
	ENN_U32 RegAddr;
	ENN_U8 FunCode = 0;
	ENN_U8 u8Channel;
	ENN_U8 u8SlaveAddr = 0;
	//ENN_U8 datalen;

	ENNAPI_ASSERT(NULL != pBuf);
	
	Len = 7;
	u8FunCode = pBuf[Len++];
	printf("%s, %d, u8FunCode = 0x%2.2X\n",__FUNCTION__,__LINE__,u8FunCode);

	u16RegAddr = ((ENN_U16)(pBuf[Len++])) << 8;
	u16RegAddr |= (ENN_U16)(pBuf[Len++]);

	u16RegNum = ((ENN_U16)(pBuf[Len++])) << 8;
	u16RegNum |= (ENN_U16)(pBuf[Len++]);
	//printf("%s, %d, u16RegNum =%d\n",__FUNCTION__,__LINE__,u16RegNum);
	//datalen = u16RegNum * 2;

	if(u16RegAddr == VirtualRegAddr)
	{
		printf("%s, %d, u16RegAddr =%d\n",__FUNCTION__,__LINE__,u16RegAddr);
		returnCode = ENNModBus_Read_VirtReg_to_DevStatus(socket, u16RegNum, pBuf);
		return returnCode;
	}

	if(pModbus_Reg_Map != NULL)
		pCur_Reg_Map = pModbus_Reg_Map;
	while(pCur_Reg_Map != NULL)
	{
		if((pCur_Reg_Map->u8Funcode == u8FunCode) && (pCur_Reg_Map->u16RegAddr == u16RegAddr))
			break;
		pCur_Reg_Map = pCur_Reg_Map->next;
	}
	if(pCur_Reg_Map == NULL)
	{
		ENNModBus_Func_Exception(socket, 0x02, pBuf);
		ERR("Not found Reg in pModbus_Reg_Map\n");
		return ENN_FAIL;
	}
	RegID = pCur_Reg_Map->u16RegID;
	returnCode = ENNModBus_get_RegInfo(RegID, &u8Channel, &u8SlaveAddr, &FunCode, &RegAddr);
	if(ENN_SUCCESS != returnCode)
	{
		ENNModBus_Func_Exception(socket, 0x02, pBuf);
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return ENN_FAIL;
	}
	//printf("[%s], %d, u8Channel =%d,u8FunCode = %d, RegID =%d\n",__FUNCTION__,__LINE__,u8Channel,FunCode,RegID);
	//ENNIEC102_Data_Map();
	//test_102_print1();
	switch(FunCode)
	{
		case MB_FUNC_READ_COILS:
		case MB_FUNC_READ_DISCRETE_INPUTS:
			returnCode = ENNModBus_Func_Read_Coils_Or_DiscreteInputs(socket, pBuf, RegID, u8Channel, FunCode);
			break;
		case MB_FUNC_READ_HOLDING_REGISTER:
		case MB_FUNC_READ_INPUT_REGISTER:
			returnCode = _ENNModBus_Func_Read_Holding_Or_Input_Register(socket, pBuf, 
																		RegID, u8Channel, FunCode,
																		pCur_Reg_Map);
			break;
		case MB_FUNC_WRITE_SINGLE_COIL:
			returnCode = ENNModBus_Func_Write_Single_Coil(socket, pBuf,RegID, u8Channel);
			break;
		case MB_FUNC_WRITE_SINGLE_REGISTER:
			returnCode = ENNModBus_Func_Write_Single_Register(socket, pBuf,
															RegID, u8Channel, u8SlaveAddr, RegAddr);
			break;
		case MB_FUNC_WRITE_MULTIPLE_REGISTERS:
			returnCode = ENNModBus_Func_Write_Multiple_Register(socket, pBuf, RegID, u8Channel);
			break;
		case MB_FUNC_READ_HISTORY_DATA:
			returnCode = ENNModBus_Func_Read_History_Data(socket, pBuf);
			break;
		default:
			returnCode = ENNModBus_Func_Exception(socket, 0x01, pBuf);
			break;
	}
	
	return returnCode;
}


ENN_ErrorCode_t ENNModBus_Func_Parser(ENN_S32 socket, ENN_U8 *pBuf)
{
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	ENN_U16 u16RegAddr;

	MODBUS_DPA_REG_MAP *pCur_Reg_Map;
	ENN_U16 RegID;
	ENN_U32 RegAddr;
	ENN_U8 FunCode = 0;
	ENN_U8 u8Channel;
	ENN_U8 u8SlaveAddr;

	ENNAPI_ASSERT(NULL != pBuf);
	
	Len = 7;
	u8FunCode = pBuf[Len++];
	ENNWRITETRACE("%s, %d, u8FunCode = 0x%2.2X\n",__FUNCTION__,__LINE__,u8FunCode);

	u16RegAddr = ((ENN_U16)(pBuf[Len++])) << 8;
	u16RegAddr |= (ENN_U16)(pBuf[Len++]);

	if(pModbus_Reg_Map != NULL)
		pCur_Reg_Map = pModbus_Reg_Map;
	while(pCur_Reg_Map != NULL)
	{
		if((pCur_Reg_Map->u8Funcode == u8FunCode) && (pCur_Reg_Map->u16RegAddr == u16RegAddr))
			break;
		pCur_Reg_Map = pCur_Reg_Map->next;
	}
	if(pCur_Reg_Map == NULL)
	{
		ERR("Not found Reg in pModbus_Reg_Map\n");
		return ENN_FAIL;
	}
	RegID = pCur_Reg_Map->u16RegID;
	returnCode = ENNModBus_get_RegInfo(RegID, &u8Channel, &u8SlaveAddr, &FunCode, &RegAddr);
	if(ENN_SUCCESS != returnCode)
	{
		ENNModBus_Func_Exception(socket, 0x02, pBuf);
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return ENN_FAIL;
	}
	//printf("[%s], %d, u8Channel =%d,u8FunCode = %d\n",__FUNCTION__,__LINE__,u8Channel,FunCode);
	//ENNIEC102_Data_Map();
	//test_102_print1();
	switch(FunCode)
	{
		case MB_FUNC_READ_COILS:
		case MB_FUNC_READ_DISCRETE_INPUTS:
			returnCode = ENNModBus_Func_Read_Coils_Or_DiscreteInputs(socket, pBuf, RegID, u8Channel, FunCode);
			break;
		case MB_FUNC_READ_HOLDING_REGISTER:
		case MB_FUNC_READ_INPUT_REGISTER:
			returnCode = ENNModBus_Func_Read_Holding_Or_Input_Register(socket, pBuf, RegID, u8Channel, FunCode);
			break;
		case MB_FUNC_WRITE_SINGLE_COIL:
			returnCode = ENNModBus_Func_Write_Single_Coil(socket, pBuf, RegID, u8Channel);
			break;
		case MB_FUNC_WRITE_MULTIPLE_REGISTERS:
			returnCode = ENNModBus_Func_Write_Multiple_Register(socket, pBuf, RegID, u8Channel);
			break;
		case MB_FUNC_READ_HISTORY_DATA:
			returnCode = ENNModBus_Func_Read_History_Data(socket, pBuf);
			break;
		default:
			returnCode = ENNModBus_Func_Exception(socket, 0x01, pBuf);
			break;
	}
	
	return returnCode;
}


#ifdef __cplusplus
#if __cplusplus
    }
#endif /* __cpluscplus */
#endif /* __cpluscplus */


