/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：ModBus_TCP.c
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/	

#include <iconv.h>

#include "ennAPI.h"
#include "ModBus_Slave_Table.h"


#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */

#define DATA_MAC_LENGTH (1024*1024)

#define FIFIO_HTML_MSG 		"/opt/ipnc/msgfifo"
#define FIFIO_HTML_OUT_MSG 	"/home/msgoutfifo"


#define PIPE_PRINT 
//#define PIPE_PRINT


/*#define ENN_SLAVES_SAVE			0x00
#define ENN_SLAVES_FREE			0x01

#define ENN_UART_SET			0x06

#define ENN_FUNCODE_ADD			0x10
#define ENN_FUNCODE_DEL			0x11
#define ENN_FUNCODE_MOD			0x12
#define ENN_SLAVE_DEL			0x13

#define ENN_FUNCODE_SHOW		0x20
#define ENN_FUNCODE_DOWNLOAD	0x21*/
#define ENN_CMD_SET_IP_REQ    			17
#define ENN_CMD_SET_IP_RSP    			18

#define ENN_CMD_CHANNEL_STATUS_REQ    	13
#define ENN_CMD_CHANNEL_STATUS_RSP    	14

#define ENN_CMD_GET_UART_CONF_REQ    	19
#define ENN_CMD_GET_UART_CONF_RSP    	20

#define ENN_CMD_SET_UART_CONF_REQ    	21
#define ENN_CMD_SET_UART_CONF_RSP    	22

#define ENN_CMD_SLAVE_GET_REQ   		31
#define ENN_CMD_SLAVE_GET_RSP    		32

#define ENN_CMD_SLAVE_SET_REQ   		33
#define ENN_CMD_SLAVE_SET_RSP    		34

#define ENN_CMD_SLAVE_REG_GET_REQ   		35
#define ENN_CMD_SLAVE_REG_GET_RSP    		36

#define ENN_CMD_SLAVE_REG_SET_REQ   		37
#define ENN_CMD_SLAVE_REG_SET_RSP    		38

#define ENN_CMD_SLAVE_SAVE_REQ   		39
#define ENN_CMD_SLAVE_SAVE_RSP    		40
#define ENN_CMD_REBOOT_REQ   		50

#define ENN_CMD_DEV645_CODE_GET_REQ   	60
#define ENN_CMD_DEV645_CODE_GET_RSP   	61
#define ENN_CMD_DEV645_CODE_SET_REQ   	62
#define ENN_CMD_DEV645_CODE_SET_RSP   	63

#define 	ENNMODBUS_CHANNEL_SIZE		(UART_CHANNEL_MAX_NUM * sizeof(Channel_List))
ENN_U8 channel_memory[ENNMODBUS_CHANNEL_SIZE];
ENN_U8 *pchannel_memory = NULL;
Channel_List *gSlave_Set_List_Head = NULL;
extern Channel_List *gChannel_List_Head;

void debug_int()
{
	Channel_List *pChannel_List_Temp = NULL;
	Slave_List *pSlave_List_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;
	Register_List *pRegister_List_Temp = NULL;
	Dev645_List *pDev645_List_Temp = NULL;
	Code_645_List *pCode_645_List_Temp = NULL;

	pChannel_List_Temp = gSlave_Set_List_Head;
	while(NULL != pChannel_List_Temp)
	{
		printf("Channel = %d, SlaveNum = %d, u32BaudRate = %d, Parity = %d\n", pChannel_List_Temp->u8Channel, pChannel_List_Temp->u8SlaveNum, pChannel_List_Temp->u32BaudRate, pChannel_List_Temp->u8Parity);
		printf("Protocol = %d\n", pChannel_List_Temp->u8Protocol);
		if(PROTOCOL_MODBUS == pChannel_List_Temp->u8Protocol)
		{
			//pSlave_List_Temp = pChannel_List_Temp->pSlaveList;
			pSlave_List_Temp = pChannel_List_Temp->unDeviceType.pModBus_List;
			while(NULL != pSlave_List_Temp)
			{
				printf("		pSlave_List_Temp = %x\n", pSlave_List_Temp);
				//printf("	SlaveAddr = %d, format = %d\n", pSlave_List_Temp->u8SlaveAddr,pSlave_List_Temp->u8DataFormat);
				printf("	SlaveAddr = %d, SlaveName = %s[%d], format = %d\n", pSlave_List_Temp->u8SlaveAddr, pSlave_List_Temp->pSlaveName, strlen(pSlave_List_Temp->pSlaveName), pSlave_List_Temp->u8DataFormat);
				pSlave_FunCode_Temp = pSlave_List_Temp->pFunCode_List;
				while(NULL != pSlave_FunCode_Temp)
				{
					printf("		MBFunCode = %d, Count = %d, TotalRegNum = %d\n", pSlave_FunCode_Temp->u8MBFunCode,pSlave_FunCode_Temp->u16Count,pSlave_FunCode_Temp->u16TotalRegNum);
					pRegister_List_Temp = pSlave_FunCode_Temp->pRegister_List;
					while(NULL != pRegister_List_Temp)
					{
						printf("			%d, %d, %s, %s\n", pRegister_List_Temp->u16RegAddr, pRegister_List_Temp->u16RegNum, pRegister_List_Temp->pName, pRegister_List_Temp->pAttr);
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
			//printf("	pDev645_List_Temp = %p\n", pDev645_List_Temp);
			while(NULL != pDev645_List_Temp)
			{
				printf("	ucDevAddr = %s, SlaveName = %s[%d]\n", pDev645_List_Temp->ucDevAddr, pDev645_List_Temp->pSlaveName, strlen(pDev645_List_Temp->pSlaveName));
				pCode_645_List_Temp = pDev645_List_Temp->pCode_645_List;
				while(NULL != pCode_645_List_Temp)
				{
					printf("			u32ChannelCode = 0x%X, u16RegNum = %d, pName = %s, pAttr = %s\n", pCode_645_List_Temp->u32ChannelCode, pCode_645_List_Temp->u16RegNum, pCode_645_List_Temp->pName, pCode_645_List_Temp->pAttr);
					pCode_645_List_Temp = pCode_645_List_Temp->next;
				}

				pDev645_List_Temp = pDev645_List_Temp->next;
			}
		}

		pChannel_List_Temp = pChannel_List_Temp->next;
	}
}

#define ENN_645_MAX_CHANNEL_LEN	14
static const Dev645_Map gDev645_Map[ENN_645_MAX_CHANNEL_LEN] = {
													{0x0101,"正向有功总电量"},
													{0x0102,"反向有功总电量"},
													{0x0103,"正向无功总电量"},
													{0x0104,"反向无功总电量"},
													{0x0201,"A相电压"},
													{0x0202,"B相电压"},
													{0x0203,"C相电压"},
													{0x0204,"A相电流"},
													{0x0205,"B相电流"},
													{0x0206,"C相电流"},
													{0x0207,"有功功率总"},
													{0x0208,"无功功率总"},
													{0x0209,"功率因数"},
													{0x020A,"频率"}};


#if 1
Channel_List *ENNModBus_Channel_Search(ENN_U8 u8Channel)
{
	Channel_List *pCurrent_Channel_List = NULL;
	Channel_List *pLast_Channel_List = NULL;
	ENN_U8 Channel = 0;
	
	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
	if(NULL == gSlave_Set_List_Head)
	{
		return NULL;
	}

	PIPE_PRINT("%s, %d, u8Channel = %d\n",__FUNCTION__,__LINE__, u8Channel);
	Channel = u8Channel - 1;
	pCurrent_Channel_List = gSlave_Set_List_Head;
	while((Channel != pCurrent_Channel_List->u8Channel) && (NULL != pCurrent_Channel_List->next))
	{
		pCurrent_Channel_List = pCurrent_Channel_List->next;
	}

	if(Channel != pCurrent_Channel_List->u8Channel)
	{
		PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
		return NULL;
	}
	else
	{
		PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
		return pCurrent_Channel_List;
	}
}


Slave_FunCode_List *ENNModBus_Slave_Search_Ex(ENN_U8 u8Slave, ENN_U8 u8FunCode, ENN_U8 u8DataFormat, Channel_List *pChannel_List)
{
	Slave_List *pSlave_List_Add = NULL;
	Slave_List *pCurrent_Slave_List = NULL;
	Slave_List *pLast_Slave_List = NULL;
	Slave_FunCode_List *pFunCode_List_Add = NULL;
	Slave_FunCode_List *pCurrent_FunCode_List = NULL;
	Slave_FunCode_List *pLast_FunCode_List = NULL;
	Register_List *pCurrent_Register_List = NULL;
	Register_List *pLast_Register_List = NULL;
	
	ENNAPI_ASSERT(NULL != pChannel_List);

#if 0
	pCurrent_Slave_List = pChannel_List->pSlaveList;
	if(NULL != pCurrent_Slave_List)
	{
		while((u8Slave != pCurrent_Slave_List->u8SlaveAddr) && (NULL != pCurrent_Slave_List->next))
		{
			pCurrent_Slave_List = pCurrent_Slave_List->next;
		}

		if(u8Slave == pCurrent_Slave_List->u8SlaveAddr)
		{
			//return NULL;
			//查询功能码
			pCurrent_Slave_List->u8DataFormat = u8DataFormat;
			pCurrent_FunCode_List = pCurrent_Slave_List->pFunCode_List;

			while((u8FunCode != pCurrent_FunCode_List->u8MBFunCode) && (NULL != pCurrent_FunCode_List->next))
			{
				pCurrent_FunCode_List = pCurrent_FunCode_List->next;
			}

			if(u8FunCode == pCurrent_FunCode_List->u8MBFunCode)
			{
				PIPE_PRINT("%s, %d, %d\n",__FUNCTION__,__LINE__,u8FunCode);
				pCurrent_Register_List = pCurrent_FunCode_List->pRegister_List;
				PIPE_PRINT("%s, %d, pCurrent_Register_List = %p\n",__FUNCTION__,__LINE__,pCurrent_Register_List);
				while(NULL != pCurrent_Register_List)
				{
					pLast_Register_List = pCurrent_Register_List;
					PIPE_PRINT("%s, %d, pLast_Register_List = %p\n",__FUNCTION__,__LINE__,pLast_Register_List);
					pCurrent_Register_List = pCurrent_Register_List->next;
					PIPE_PRINT("%s, %d, pCurrent_Register_List = %p\n",__FUNCTION__,__LINE__,pCurrent_Register_List);

					if(NULL != pLast_Register_List->pName)
					{
						free(pLast_Register_List->pName);
						pLast_Register_List->pName = NULL;
					}
					if(NULL != pLast_Register_List->pAttr)
					{
						free(pLast_Register_List->pAttr);
						pLast_Register_List->pAttr = NULL;
					}

					free(pLast_Register_List);
					pLast_Register_List = NULL;
				}
				
				pCurrent_FunCode_List->pRegister_List = NULL;

				return pCurrent_FunCode_List;
			}
			else
			{
				//增加功能码
				pFunCode_List_Add = (Slave_FunCode_List *)malloc(sizeof(Slave_FunCode_List));
				if(NULL == pFunCode_List_Add)
				{
					return NULL;
				}
				memset(pFunCode_List_Add, 0, sizeof(Slave_FunCode_List));
				pFunCode_List_Add->u8MBFunCode = u8FunCode;
				pFunCode_List_Add->next = NULL;

				pCurrent_FunCode_List->next = pFunCode_List_Add;

				return pFunCode_List_Add;
			}
		}
		else
		{
			goto AddSlave; //增加从站
		}
	}
	else
	{
		goto AddSlave; //增加从站
	}

AddSlave:
	pSlave_List_Add = (Slave_List *)malloc(sizeof(Slave_List));
	if(NULL == pSlave_List_Add)
	{
		return NULL;
	}
	memset(pSlave_List_Add, 0, sizeof(Slave_List));
	pSlave_List_Add->u8SlaveAddr = u8Slave;
	pSlave_List_Add->u8DataFormat = u8DataFormat;
	pSlave_List_Add->next = NULL;

	if(NULL == pCurrent_Slave_List)
	{
		pChannel_List->pSlaveList = pSlave_List_Add;
	}
	else
	{
		pCurrent_Slave_List->next = pSlave_List_Add;
	}

	//增加功能码
	pFunCode_List_Add = (Slave_FunCode_List *)malloc(sizeof(Slave_FunCode_List));
	if(NULL == pFunCode_List_Add)
	{
		free(pSlave_List_Add);
		pSlave_List_Add = NULL;
		return NULL;
	}
	memset(pFunCode_List_Add, 0, sizeof(Slave_FunCode_List));
	pFunCode_List_Add->u8MBFunCode = u8FunCode;
	pFunCode_List_Add->next = NULL;

	pSlave_List_Add->pFunCode_List = pFunCode_List_Add;

	pChannel_List->u8SlaveNum++;

#endif
	return pFunCode_List_Add;
}


/*ENN_ErrorCode_t ENNModBus_Reg_Add(ENN_U16 u16RegAddr, ENN_U16 u16RegNum, ENN_CHAR *RegName, ENN_CHAR *RegAttr, Slave_FunCode_List *pFunCode_List)
{
	Register_List *pRegister_List_Add = NULL;
	Register_List *pCurrent_Register_List = NULL;
	Register_List *pLast_Register_List = NULL;
	Slave_FunCode_List *pCurrent_FunCode_List = NULL;
	ENN_U8 *pTemp = NULL;
	ENN_U32 length = 0;
	
	ENNAPI_ASSERT(NULL != RegName);
	ENNAPI_ASSERT(NULL != RegAttr);
	ENNAPI_ASSERT(NULL != pFunCode_List);

	pCurrent_FunCode_List = pFunCode_List;

	pRegister_List_Add = (Register_List *)malloc(sizeof(Register_List));
	if(NULL == pRegister_List_Add)
	{
		return ENN_FAIL;
	}
	memset(pRegister_List_Add, 0, sizeof(Register_List));
	pRegister_List_Add->u16RegAddr = u16RegAddr;
	pRegister_List_Add->u16RegNum = u16RegNum;
	pRegister_List_Add->pName = RegName;
	pRegister_List_Add->pAttr = RegAttr;
	pRegister_List_Add->next = NULL;

	return ENN_SUCCESS;
}*/


ENN_ErrorCode_t ENNModBus_Slave_Save(ENN_VOID)
{
	ENN_U8 u8channelNum = 0;
	ENN_CHAR vlaue[30];
	ENN_U8 i = 0;
	ENN_U8 j = 0;
	ENN_U8 u8SlaveAddr = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 u8DataFormat = 0;
	ENN_U16 u16RegAddr = 0;
	ENN_U16 u16RegNum = 0;
	ENN_S32 s32Ret = 0;
	Channel_List *pCurrent_Channel_List = NULL;
	Slave_List *pCurrent_Slave_List = NULL;
	Slave_FunCode_List *pCurrent_FunCode_List = NULL;
	Register_List *pRegister_List_Add = NULL;
	Register_List *pCurrent_Register_List = NULL;
	Register_List *pLast_Register_List = NULL;
	FILE 	*fpConfig = NULL;
	Protocol_Type eProtocol_Type = PROTOCOL_MODBUS;
	Dev645_List *pCurrent_Dev645_List = NULL;
	Code_645_List *pCurrentCode_645_List = NULL;
	ENN_U16 u16Channel_Code = 0;
	
	if(NULL == gSlave_Set_List_Head)
	{
		return ENN_FAIL;
	}

#if 1
	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
	fpConfig = fopen("/home/modbus_20150306.ini", "w+");
	if(NULL == fpConfig)
	{
		perror("save configure file fail ");
		return ENN_FAIL;
	}
	
	s32Ret = (ENN_S32)fputs("[MAIN]\r\n", fpConfig);
	if(s32Ret < 0)
	{
		fclose(fpConfig);
		/*if(0 != fclose(fpConfig))
		{
			perror("save configure file fail ");
			ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
		}*/
		return ENN_FAIL;
	}

	s32Ret = (ENN_S32)fputs("CHANNEL_NUM=", fpConfig);
	if(s32Ret < 0)
	{
		fclose(fpConfig);
		return ENN_FAIL;
	}
	memset(vlaue, 0, 30);
	u8channelNum = UART_CHANNEL_MAX_NUM;
	sprintf(vlaue, "%d\r\n", u8channelNum);
	s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
	if(s32Ret < 0)
	{
		fclose(fpConfig);
		return ENN_FAIL;
	}

	s32Ret = (ENN_S32)fputs("FUNCODE={{1,0000},{2,0000},{3,0000},{4,0000}}\r\n", fpConfig);
	if(s32Ret < 0)
	{
		fclose(fpConfig);
		return ENN_FAIL;
	}

	pCurrent_Channel_List = gSlave_Set_List_Head;
	while(NULL != pCurrent_Channel_List)
	{
		s32Ret = (ENN_S32)fputs("\r\n", fpConfig);
		if(s32Ret < 0)
		{
			fclose(fpConfig);
			return ENN_FAIL;
		}
		i++;
		memset(vlaue, 0, 30);
		sprintf(vlaue, "[CHANNEL_%d]\r\n", i);
		s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
		if(s32Ret < 0)
		{
			fclose(fpConfig);
			return ENN_FAIL;
		}

		memset(vlaue, 0, 30);
		sprintf(vlaue, "BAUDRATE=%d\r\n", pCurrent_Channel_List->u32BaudRate);
		s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
		if(s32Ret < 0)
		{
			fclose(fpConfig);
			return ENN_FAIL;
		}

		s32Ret = (ENN_S32)fputs("PARITY=", fpConfig);
		if(s32Ret < 0)
		{
			fclose(fpConfig);
			return ENN_FAIL;
		}
		
		switch(pCurrent_Channel_List->u8Parity)
		{
			case PARITY_NONE:
				s32Ret = (ENN_S32)fputs(MODBUS_PRITY_NONE, fpConfig);
				break;
			case PARITY_ODD:
				s32Ret = (ENN_S32)fputs(MODBUS_PRITY_ODD, fpConfig);
				break;
			case PARITY_EVEN:
				s32Ret = (ENN_S32)fputs(MODBUS_PRITY_EVEN, fpConfig);
				break;
			case PARITY_MARK:
				s32Ret = (ENN_S32)fputs(MODBUS_PRITY_MARK, fpConfig);
				break;
			case PARITY_SPACE:
				s32Ret = (ENN_S32)fputs(MODBUS_PRITY_SPACE, fpConfig);
				break;
			default:
				fclose(fpConfig);
				return ENN_FAIL;
		}

		if(s32Ret < 0)
		{
			fclose(fpConfig);
			return ENN_FAIL;
		}

		s32Ret = (ENN_S32)fputs("\r\n", fpConfig);
		if(s32Ret < 0)
		{
			fclose(fpConfig);
			return ENN_FAIL;
		}

		memset(vlaue, 0, 30);
		sprintf(vlaue, "DATABITS=%d\r\n", pCurrent_Channel_List->u8DataBit);
		s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
		if(s32Ret < 0)
		{
			fclose(fpConfig);
			return ENN_FAIL;
		}

		memset(vlaue, 0, 30);
		sprintf(vlaue, "STOPBITS=%d\r\n", pCurrent_Channel_List->u8StoptBit);
		s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
		if(s32Ret < 0)
		{
			fclose(fpConfig);
			return ENN_FAIL;
		}

		memset(vlaue, 0, 30);
		sprintf(vlaue, "SLAVE_NUM=%d\r\n", pCurrent_Channel_List->u8SlaveNum);
		s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
		if(s32Ret < 0)
		{
			fclose(fpConfig);
			return ENN_FAIL;
		}

		j = 0;
		eProtocol_Type = (Protocol_Type)pCurrent_Channel_List->u8Protocol;

		if(PROTOCOL_MODBUS == eProtocol_Type)
		{
			pCurrent_Slave_List = pCurrent_Channel_List->unDeviceType.pModBus_List;
			//for(j=0; j<pCurrent_Channel_List->u8SlaveNum; j++)
			while(NULL != pCurrent_Slave_List)
			{
				j++;
				//PIPE_PRINT("%s, %d, j = %d\n",__FUNCTION__,__LINE__, j);
				memset(vlaue, 0, 30);
				sprintf(vlaue, "SLAVE_%d={", j);
				PIPE_PRINT("%s, %d, vlaue = %s\n",__FUNCTION__,__LINE__, vlaue);
				s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
				if(s32Ret < 0)
				{
					fclose(fpConfig);
					return ENN_FAIL;
				}

				u8SlaveAddr = pCurrent_Slave_List->u8SlaveAddr;
				u8DataFormat = pCurrent_Slave_List->u8DataFormat;
				pCurrent_FunCode_List = pCurrent_Slave_List->pFunCode_List;

				memset(vlaue, 0, 30);
				sprintf(vlaue, "%d,", u8SlaveAddr);
				s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
				if(s32Ret < 0)
				{
					fclose(fpConfig);
					return ENN_FAIL;
				}

				fputs(pCurrent_Slave_List->pSlaveName, fpConfig);
				fputs(",{", fpConfig);

				while(NULL != pCurrent_FunCode_List)
				{
					u8FunCode = pCurrent_FunCode_List->u8MBFunCode;

					memset(vlaue, 0, 30);
					sprintf(vlaue, "%d,", u8FunCode);
					s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
					if(s32Ret < 0)
					{
						fclose(fpConfig);
						return ENN_FAIL;
					}
					
					if((0x03 == u8FunCode) || (0x04 == u8FunCode))
					{
						memset(vlaue, 0, 30);
						sprintf(vlaue, "%d,{", u8DataFormat);
						s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
						if(s32Ret < 0)
						{
							fclose(fpConfig);
							return ENN_FAIL;
						}
					}
					else
					{
						s32Ret = (ENN_S32)fputs("{", fpConfig);
						if(s32Ret < 0)
						{
							fclose(fpConfig);
							return ENN_FAIL;
						}
					}
					pCurrent_Register_List = pCurrent_FunCode_List->pRegister_List;

					while(NULL != pCurrent_Register_List)
					{
						u16RegAddr = pCurrent_Register_List->u16RegAddr;
						u16RegNum = pCurrent_Register_List->u16RegNum;

						memset(vlaue, 0, 30);
						if((0x03 == u8FunCode) || (0x04 == u8FunCode) || (0x10 == u8FunCode))
						{
							sprintf(vlaue, "%X,%d,", u16RegAddr,u16RegNum);
						}
						else
						{
							//sprintf(vlaue, "%d,%d,", u16RegAddr,u16RegNum);
							sprintf(vlaue, "%d,", u16RegAddr);
						}
						s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
						if(s32Ret < 0)
						{
							fclose(fpConfig);
							return ENN_FAIL;
						}

						s32Ret = (ENN_S32)fputs(pCurrent_Register_List->pName, fpConfig);
						if(s32Ret < 0)
						{
							fclose(fpConfig);
							return ENN_FAIL;
						}

						s32Ret = (ENN_S32)fputs(",", fpConfig);
						if(s32Ret < 0)
						{
							fclose(fpConfig);
							return ENN_FAIL;
						}

						s32Ret = (ENN_S32)fputs(pCurrent_Register_List->pAttr, fpConfig);
						if(s32Ret < 0)
						{
							fclose(fpConfig);
							return ENN_FAIL;
						}
						pCurrent_Register_List = pCurrent_Register_List->next;
						
						if(NULL == pCurrent_Register_List)
						{
							s32Ret = (ENN_S32)fputs("}", fpConfig);
						}
						else
						{
							s32Ret = (ENN_S32)fputs(",", fpConfig);
						}
						if(s32Ret < 0)
						{
							fclose(fpConfig);
							return ENN_FAIL;
						}
					}
					s32Ret = (ENN_S32)fputs("}", fpConfig);
					if(s32Ret < 0)
					{
						fclose(fpConfig);
						return ENN_FAIL;
					}
					
					pCurrent_FunCode_List = pCurrent_FunCode_List->next;
					if(NULL == pCurrent_FunCode_List)
					{
						s32Ret = (ENN_S32)fputs("}", fpConfig);
					}
					else
					{
						s32Ret = (ENN_S32)fputs(",{", fpConfig);
					}
					if(s32Ret < 0)
					{
						fclose(fpConfig);
						return ENN_FAIL;
					}
				}

				s32Ret = (ENN_S32)fputs("\r\n", fpConfig);
				if(s32Ret < 0)
				{
					fclose(fpConfig);
					return ENN_FAIL;
				}
				pCurrent_Slave_List = pCurrent_Slave_List->next;
			}
		}
		else if((PROTOCOL_645_1997 == eProtocol_Type) || (PROTOCOL_645_2007 == eProtocol_Type))
		{
			pCurrent_Dev645_List = pCurrent_Channel_List->unDeviceType.pDev645_List;
			//for(j=0; j<pCurrent_Channel_List->u8SlaveNum; j++)
			while(NULL != pCurrent_Dev645_List)
			{
				j++;
				//PIPE_PRINT("%s, %d, j = %d\n",__FUNCTION__,__LINE__, j);
				memset(vlaue, 0, 30);
				sprintf(vlaue, "SLAVE_%d={", j);
				PIPE_PRINT("%s, %d, vlaue = %s\n",__FUNCTION__,__LINE__, vlaue);
				s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
				if(s32Ret < 0)
				{
					fclose(fpConfig);
					return ENN_FAIL;
				}

				s32Ret = (ENN_S32)fputs(pCurrent_Dev645_List->ucDevAddr, fpConfig);
				if(s32Ret < 0)
				{
					fclose(fpConfig);
					return ENN_FAIL;
				}
				fputs(",", fpConfig);

				fputs(pCurrent_Slave_List->pSlaveName, fpConfig);
				fputs(",{", fpConfig);

				pCurrentCode_645_List = pCurrent_Slave_List->pFunCode_List;
				while(NULL != pCurrentCode_645_List)
				{
					u16Channel_Code = pCurrentCode_645_List->u32ChannelCode;

					memset(vlaue, 0, 30);
					sprintf(vlaue, "%X,", u16Channel_Code);
					s32Ret = (ENN_S32)fputs(vlaue, fpConfig);
					if(s32Ret < 0)
					{
						fclose(fpConfig);
						return ENN_FAIL;
					}

					for(i = 0; i < ENN_645_MAX_CHANNEL_LEN; i++)
					{
						if(gDev645_Map[i].u16code == u16Channel_Code)
						{
							fputs(gDev645_Map[i].strName, fpConfig);
							break;
						}
					}
					fputs(",", fpConfig);

					if(Data_DECIMAL == pCurrentCode_645_List->eData_Type)
					{
						fputs("浮点数", fpConfig);
					}
					else
					{
						fputs("整数", fpConfig);
					}

					pCurrentCode_645_List = pCurrentCode_645_List->next;
					if(NULL == pCurrentCode_645_List)
					{
						fputs("}", fpConfig);
					}
					else
					{
						fputs(",", fpConfig);
					}
				}

				s32Ret = (ENN_S32)fputs("\r\n", fpConfig);
				if(s32Ret < 0)
				{
					fclose(fpConfig);
					return ENN_FAIL;
				}
				pCurrent_Dev645_List = pCurrent_Dev645_List->next;
			}
		}

		pCurrent_Channel_List = pCurrent_Channel_List->next;
	}
	
	fclose(fpConfig);
#endif
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_FunCode_Add(ENN_U8 *pdata, ENN_U32 len)
{
	ENN_U8 *pTemp = NULL;
	ENN_U32 length = 0;
	ENN_U8 		channel = 0; 
	ENN_U8 		slave = 0;
	ENN_U8 		funCode = 0;
	ENN_U8 		dataFormat = 0;
	ENN_U8 		regCount = 0;
	ENN_U16 	regAddr = 0;
	ENN_U16 	regNum = 0;
	ENN_U16 	regNameLen = 0;
	ENN_CHAR 	*regName = NULL;
	ENN_U16 	regDesLen = 0;
	ENN_CHAR 	*regDes = NULL;
	ENN_U8 i = 0;
	Channel_List *pCurrent_Channel_List = NULL;
	Slave_FunCode_List *pCurrent_FunCode_List = NULL;
	Register_List *pRegister_List_Add = NULL;
	Register_List *pCurrent_Register_List = NULL;
	Register_List *pLast_Register_List = NULL;
	ENN_CHAR *ptest = NULL;
	ENN_U8 outlen = 0;
	FILE *fp = NULL;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

#if 0
	if(NULL == gSlave_Set_List_Head)
	{
		return ENN_FAIL;
	}

	pTemp = pdata;
	length = len;

	channel = *pTemp;
	length--;
	pTemp++;

	slave = *pTemp;
	length--;
	pTemp++;
	
	funCode = *pTemp;
	length--;
	pTemp++;

	if(0x03 == funCode)
	{
		dataFormat = *pTemp;
	}
	length--;
	pTemp++;

	/*查询通道*/
	pCurrent_Channel_List = ENNModBus_Channel_Search(channel);
	if(NULL == pCurrent_Channel_List)
	{
		return ENN_FAIL;
	}
	
	pCurrent_FunCode_List = ENNModBus_Slave_Search_Ex(slave, funCode, dataFormat, pCurrent_Channel_List);
	if(NULL == pCurrent_Channel_List)
	{
		return ENN_FAIL;
	}

	regCount = *pTemp;
	length--;
	pTemp++;

	for(i=0; i<regCount; i++)
	{
		memcpy(&regAddr, pTemp, 2);
		pTemp += 2;
		length -= 2;
		
		memcpy(&regNum, pTemp, 2);
		pTemp += 2;
		length -= 2;
		
		memcpy(&regNameLen, pTemp, 2);
		pTemp += 2;
		length -= 2;

		regName = (ENN_CHAR *)malloc(regNameLen + 1);
		if(NULL == regName)
		{
			return ENN_FAIL;
		}
		memset(regName, 0, regNameLen + 1);
		memcpy(regName, pTemp, regNameLen);
		pTemp += regNameLen;
		length -= regNameLen;
		PIPE_PRINT("%d, regName = %s\n",regNameLen, regName);
		/*fp = fopen("shanjianchao.txt", "ab+");
		fwrite(regName, strlen(regName), 1, fp);
		fclose(fp);*/

		memcpy(&regDesLen, pTemp, 2);
		pTemp += 2;
		length -= 2;

		regDes = (ENN_CHAR *)malloc(regDesLen + 1);
		if(NULL == regDes)
		{
			return ENN_FAIL;
		}
		memset(regDes, 0, regDesLen + 1);
		memcpy(regDes, pTemp, regDesLen);
		pTemp += regDesLen;
		length -= regDesLen;
		PIPE_PRINT("%d, regDes = %s\n",regDesLen, regDes);
		/*fp = fopen("shanjianchao.txt", "ab+");
		fwrite(regDes, strlen(regDes), 1, fp);
		fclose(fp);*/

		/*test
		iconv_t cd = iconv_open("GBK", "UTF-8");
		PIPE_PRINT("cd = %d\n",cd);
		ptest = (ENN_CHAR *)malloc(regNameLen + 1);
		size_t re = iconv(cd, &regName,(size_t *)&regNameLen, &ptest, (size_t *)&outlen);
		PIPE_PRINT("cd = %d\n",re);
		outlen = strlen(ptest);
		PIPE_PRINT("%d, ptest: %s\n",outlen, ptest);
		free(ptest);
		iconv_close(cd);
		*******************/
		PIPE_PRINT("pRegister_List = %p\n",pCurrent_FunCode_List->pRegister_List);

		pRegister_List_Add = (Register_List *)malloc(sizeof(Register_List));
		if(NULL == pRegister_List_Add)
		{
			free(regName);
			regName = NULL;
			free(regDes);
			regDes = NULL;
			return ENN_FAIL;
		}
		memset(pRegister_List_Add, 0, sizeof(Register_List));
		pRegister_List_Add->u16RegAddr = regAddr;
		pRegister_List_Add->u16RegNum = regNum;
		pRegister_List_Add->pName = regName;
		pRegister_List_Add->pAttr = regDes;
		pRegister_List_Add->next = NULL;

		if(NULL == pCurrent_FunCode_List->pRegister_List)
		{
			pCurrent_FunCode_List->pRegister_List = pRegister_List_Add;
			pLast_Register_List = pRegister_List_Add;
		}
		else
		{
			pLast_Register_List->next = pRegister_List_Add;
			pLast_Register_List = pRegister_List_Add;
		}
	}
	debug_int();
#endif
	return ENN_SUCCESS;

	/*iconv_t cd = iconv_open("GBK", "UTF-8");
	char *outbuf=(char*)malloc(inlen*4);
	bzero(outbuf,inlen*4);
	char *in=inbuf;
	char *out=outbuf;
	size_t outlen = inlen*4;
	iconv(cd,&in,(size_t*)&inlen,&out,&outlen);
	outlen = strlen(outbuf);
	PIPE_PRINT("%s\n",outbuf);
	free(outbuf);
	iconv_close(cd);*/
}

Slave_List *ENNModBus_Slave_Search(ENN_U8 u8Slave, Channel_List *pChannel_List, Slave_List **pCurrent_Slave_List, Slave_List **pPrevious_Slave_List)
{
	Slave_List *pSearch_Slave_List = NULL;
	
	ENNAPI_ASSERT(NULL != pChannel_List);

	pSearch_Slave_List = pChannel_List->unDeviceType.pModBus_List;
	if(NULL != pSearch_Slave_List)
	{
		while((u8Slave != pSearch_Slave_List->u8SlaveAddr) && (NULL != pSearch_Slave_List->next))
		{
			*pPrevious_Slave_List = pSearch_Slave_List;
			pSearch_Slave_List = pSearch_Slave_List->next;
		}

		if(u8Slave == pSearch_Slave_List->u8SlaveAddr)
		{
			return pSearch_Slave_List;
		}
		else
		{
			*pCurrent_Slave_List = pSearch_Slave_List;
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

Dev645_List *ENNDev_645_Search(ENN_CHAR *pDevAddr, Channel_List *pChannel_List, Dev645_List **pCurrent_Dev645_List, Dev645_List **pPrevious_Dev645_List)
{
	Dev645_List *pSearch_Dev645_List = NULL;
	
	PIPE_PRINT("%s, %d, pDevAddr = %s[%d]\n",__FUNCTION__,__LINE__, pDevAddr, strlen(pDevAddr));
	ENNAPI_ASSERT(NULL != pChannel_List);
	ENNAPI_ASSERT(NULL != pDevAddr);

	PIPE_PRINT("%s, %d, pDevAddr = %s[%d]\n",__FUNCTION__,__LINE__, pDevAddr, strlen(pDevAddr));
	pSearch_Dev645_List = pChannel_List->unDeviceType.pDev645_List;
	PIPE_PRINT("%s, %d, ucDevAddr = %s\n",__FUNCTION__,__LINE__, pSearch_Dev645_List->ucDevAddr);	
	if(NULL != pSearch_Dev645_List)
	{
		while((0 != strcmp(pDevAddr, pSearch_Dev645_List->ucDevAddr)) 
			&& (NULL != pSearch_Dev645_List->next))
		{
			*pPrevious_Dev645_List = pSearch_Dev645_List;
			pSearch_Dev645_List = pSearch_Dev645_List->next;
		}

		if(0 == strcmp(pDevAddr, pSearch_Dev645_List->ucDevAddr))
		{
			PIPE_PRINT("%s, %d, pCode_645_List = %p\n",__FUNCTION__,__LINE__, pSearch_Dev645_List->pCode_645_List);	
			return pSearch_Dev645_List;
		}
		else
		{
			*pCurrent_Dev645_List = pSearch_Dev645_List;
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}


/*Slave_FunCode_List *ENNModBus_Funcode_Search(ENN_U8 u8Funcode, Channel_List *pChannel_List, Slave_List **pPrevious_Slave_List)
{
	Slave_FunCode_List *pCurrent_FunCode_List = NULL;
	
	ENNAPI_ASSERT(NULL != pChannel_List);

	pCurrent_FunCode_List = pChannel_List->pSlaveList;
	if(NULL != pSearch_Slave_List)
	{
		while((u8Slave != pSearch_Slave_List->u8SlaveAddr) && (NULL != pSearch_Slave_List->next))
		{
			*pPrevious_Slave_List = pSearch_Slave_List;
			pSearch_Slave_List = pSearch_Slave_List->next;
		}

		if(u8Slave == pSearch_Slave_List->u8SlaveAddr)
		{
			return pSearch_Slave_List;
		}
		else
		{
			*pCurrent_Slave_List = pSearch_Slave_List;
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}*/


ENN_ErrorCode_t ENNModBus_Get_Channel_Status(ENN_U8 *pdata, ENN_U16 len)
{
	ENN_U8 data[1024];
	ENN_U8 *pBuf = NULL;
	ENN_U16 u16Len = 0;
	
	ENN_U8 *pTemp = NULL;
	ENN_U16 length = 0;
	ENN_U8 channel = 0;
	ENN_U8 Slave_Num = 0;
	ENN_U8 i = 0;
	ENN_U32 CMD_RSP = 0;
	ENN_CHAR name[20];
	ENN_CHAR slave_name[20];
	ENN_U8  Name_Len = 0;
	ENN_U8 u8Slave_Status = 0;
	ENN_S32 s32ret = 0;
	ENN_S32 s32fd = 0;
	Channel_List *pCurrent_Channel_List = NULL;
	Slave_List	 *pCurrent_Slave_List = NULL;
	int ret = 0;
	Protocol_Type eProtocol_Type = PROTOCOL_MODBUS;
	Dev645_List *pCurrent_Dev645_List = NULL;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

	if(NULL == gChannel_List_Head)
	{
		return ENN_FAIL;
	}

	pTemp = pdata;
	length = len;

	channel = *pTemp;
	length--;

	pCurrent_Channel_List = gChannel_List_Head;
	while((channel != (pCurrent_Channel_List->u8Channel + 1))
	    &&(NULL != pCurrent_Channel_List->next))
	{
		pCurrent_Channel_List = pCurrent_Channel_List->next;
	}

	if(channel != (pCurrent_Channel_List->u8Channel + 1))
	{
		return ENN_FAIL;
	}
	//PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
	PIPE_PRINT("%s, %d, Slave_Num = %d\n",__FUNCTION__,__LINE__, pCurrent_Channel_List->u8SlaveNum);	

	Slave_Num = pCurrent_Channel_List->u8SlaveNum;
	eProtocol_Type = (Protocol_Type)pCurrent_Channel_List->u8Protocol;
	//pCurrent_Slave_List = pCurrent_Channel_List->pSlaveList;
	memset(data, 0, 1024);
	pBuf = &data;
	pBuf += 2;
	
	CMD_RSP = ENN_CMD_CHANNEL_STATUS_RSP;
	memcpy(pBuf, &CMD_RSP, 2);
	u16Len += 2;
	pBuf += 2;

	*pBuf = channel;
	u16Len++;
	pBuf++;

	*pBuf = (ENN_U8)eProtocol_Type;
	u16Len++;
	pBuf++;

	*pBuf = Slave_Num;
	u16Len++;
	pBuf++;
	
	PIPE_PRINT("%s, %d, Slave_Num = %d\n",__FUNCTION__,__LINE__, Slave_Num);	
	/*for(i=0; i<Slave_Num; i++)
	{
	}*/
	if(PROTOCOL_MODBUS == eProtocol_Type)
	{
		pCurrent_Slave_List = pCurrent_Channel_List->unDeviceType.pModBus_List;
		while(NULL != pCurrent_Slave_List)
		{
			i++;
			
			*pBuf = 1;
			u16Len++;
			pBuf++;

			*pBuf = pCurrent_Slave_List->u8SlaveAddr;
			u16Len++;
			pBuf++;

			/*memset(name, 0, 20);
			sprintf(name, "设备%d", i);
			PIPE_PRINT("%s, %d, 设备名 = %s,   %d\n",__FUNCTION__,__LINE__, name, strlen(name));
			for(i=0; i<strlen(name); i++)
			{
				PIPE_PRINT("%2.2X\n",name[i]);
			}*/		

			/*memset(slave_name, 0, 20);
			ret = GB2312_To_UTF8(pCurrent_Slave_List->pSlaveName, strlen(pCurrent_Slave_List->pSlaveName), slave_name, 20);
			PIPE_PRINT("%s, %d, ret = %d\n",__FUNCTION__,__LINE__, ret);
			PIPE_PRINT("%s, %d, 设备名1 = %s,   %d\n",__FUNCTION__,__LINE__, slave_name, strlen(slave_name));
			for(i=0; i<strlen(slave_name); i++)
			{
				PIPE_PRINT("%2.2X\n",slave_name[i]);
			}	*/	
			Name_Len = strlen(pCurrent_Slave_List->pSlaveName);
			*pBuf = Name_Len;
			u16Len++;
			pBuf++;

			memcpy(pBuf, pCurrent_Slave_List->pSlaveName, Name_Len);
			u16Len += Name_Len;
			pBuf += Name_Len;

			if(0 != pCurrent_Slave_List->u16Status)
			{
				u8Slave_Status = 1;
			}
			else
			{
				u8Slave_Status = 0;
			}
			*pBuf = u8Slave_Status;
			u16Len++;
			pBuf++;
			
			pCurrent_Slave_List = pCurrent_Slave_List->next;
		}
	}
	else if((PROTOCOL_645_1997 == eProtocol_Type) || (PROTOCOL_645_2007 == eProtocol_Type))
	{
		pCurrent_Dev645_List = pCurrent_Channel_List->unDeviceType.pDev645_List;
		while(NULL != pCurrent_Dev645_List)
		{
			i++;
			
			*pBuf = (ENN_U8)strlen(pCurrent_Dev645_List->ucDevAddr);
			u16Len++;
			pBuf++;

			memcpy(pBuf, pCurrent_Dev645_List->ucDevAddr, strlen(pCurrent_Dev645_List->ucDevAddr));
			u16Len += strlen(pCurrent_Dev645_List->ucDevAddr);
			pBuf += strlen(pCurrent_Dev645_List->ucDevAddr);

			Name_Len = strlen(pCurrent_Dev645_List->pSlaveName);
			*pBuf = Name_Len;
			u16Len++;
			pBuf++;

			memcpy(pBuf, pCurrent_Dev645_List->pSlaveName, Name_Len);
			u16Len += Name_Len;
			pBuf += Name_Len;

			if(0 != pCurrent_Dev645_List->u16Status)
			{
				u8Slave_Status = 1;
			}
			else
			{
				u8Slave_Status = 0;
			}
			*pBuf = u8Slave_Status;
			u16Len++;
			pBuf++;
			
			pCurrent_Dev645_List = pCurrent_Dev645_List->next;
		}	
	}
	else
	{
		return ENN_FAIL;
	}

	pBuf = &data;
	memcpy(pBuf, &u16Len, 2);
	u16Len += 2;

	for(i=0; i<u16Len; i++)
		PIPE_PRINT("%x ", pBuf[i]);
	PIPE_PRINT("\n");

	s32ret = ENNFIFO_create(FIFIO_HTML_OUT_MSG);
	PIPE_PRINT("%s, %d, s32ret = %d\n",__FUNCTION__,__LINE__, s32ret);	
	if(s32ret < 0)
	{
        perror("create failed:\n");
		return ENN_FAIL;
	}

	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);	
	//s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_WRONLY | O_NONBLOCK);
	//s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_WRONLY | O_NONBLOCK);
	s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_RDWR | O_NONBLOCK);
	PIPE_PRINT("%s, %d, s32fd = %d\n",__FUNCTION__,__LINE__, s32fd);	
	if(s32fd < 0)
	{
        perror("open failed:\n");
		return ENN_FAIL;
	}

	s32ret = ENNFIFO_write(s32fd, pBuf, u16Len);
	PIPE_PRINT("%s, %d, s32ret = %d, u16Len = %d\n",__FUNCTION__,__LINE__, s32ret, u16Len);	
	if((s32ret < 0) || (s32ret != u16Len))
	{
        perror("write failed:\n");
		return ENN_FAIL;
	}
	
	//close(s32fd);

	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_UART_Get(ENN_U8 *pdata, ENN_U16 len)
{
	ENN_U8 data[50];
	ENN_U8 *pBuf = NULL;
	ENN_U16 u16Len = 0;
	ENN_U8 *pTemp = NULL;
	ENN_U16 length = 0;
	ENN_U8 channel = 0;
	ENN_U8 i = 0;
	ENN_U8 u8ParityLen = 0;
	ENN_U32 CMD_RSP = 0;
	ENN_S32 s32ret = 0;
	ENN_S32 s32fd = 0;
	ENN_U32 u32BaudRate = 0;
	ENN_U8 	u8DataBit = 0;
	ENN_U8 	u8StopBit = 0;
	ENN_U8 	u8Parity = 0;
	ENN_CHAR *pParity = NULL;
	ENN_CHAR name[20];
	Channel_List *pCurrent_Channel_List = NULL;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

	if(NULL == gSlave_Set_List_Head)
	{
		return ENN_FAIL;
	}

	pTemp = pdata;
	length = len;

	channel = *pTemp;
	length--;

	pCurrent_Channel_List = gSlave_Set_List_Head;
	while((channel != (pCurrent_Channel_List->u8Channel + 1))
	    &&(NULL != pCurrent_Channel_List->next))
	{
		pCurrent_Channel_List = pCurrent_Channel_List->next;
	}

	if(channel != (pCurrent_Channel_List->u8Channel + 1))
	{
		return ENN_FAIL;
	}

	memset(data, 0, 50);
	pBuf = &data;
	pBuf += 2;

	CMD_RSP = ENN_CMD_GET_UART_CONF_RSP;
	memcpy(pBuf, &CMD_RSP, 2);
	u16Len += 2;
	pBuf += 2;

	*pBuf = pCurrent_Channel_List->u8Protocol;
	u16Len++;
	pBuf++;
	
	//PIPE_PRINT("%s, %d, channel = %d\n",__FUNCTION__,__LINE__, channel);	
	*pBuf = channel;
	u16Len++;
	pBuf++;

	u32BaudRate = pCurrent_Channel_List->u32BaudRate;
	u8DataBit = pCurrent_Channel_List->u8DataBit;
	u8StopBit = pCurrent_Channel_List->u8StoptBit;
	u8Parity = pCurrent_Channel_List->u8Parity;
	memcpy(pBuf, &u32BaudRate, 4);
	u16Len += 4;
	pBuf += 4;

	memcpy(pBuf, &u8DataBit, 1);
	u16Len++;
	pBuf++;

	memcpy(pBuf, &u8StopBit, 1);
	u16Len++;
	pBuf++;

	memcpy(pBuf, &u8Parity, 1);
	u16Len++;

	pBuf = &data;
	memcpy(pBuf, &u16Len, 2);
	u16Len += 2;

	for(i=0; i<u16Len; i++)
		PIPE_PRINT("%x ", pBuf[i]);
	PIPE_PRINT("\n");

	s32ret = ENNFIFO_create(FIFIO_HTML_OUT_MSG);
	PIPE_PRINT("%s, %d, s32ret = %d\n",__FUNCTION__,__LINE__, s32ret);	
	if(s32ret < 0)
	{
        perror("create failed:\n");
		return ENN_FAIL;
	}

	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);	
	//s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_WRONLY | O_NONBLOCK);
	s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_RDWR | O_NONBLOCK);
	PIPE_PRINT("%s, %d, s32fd = %d\n",__FUNCTION__,__LINE__, s32fd);	
	if(s32fd < 0)
	{
        perror("open failed:\n");
		return ENN_FAIL;
	}

	s32ret = ENNFIFO_write(s32fd, pBuf, u16Len);
	PIPE_PRINT("%s, %d, s32ret = %d, u16Len = %d\n",__FUNCTION__,__LINE__, s32ret, u16Len);	
	if((s32ret < 0) || (s32ret != u16Len))
	{
        perror("write failed:\n");
		return ENN_FAIL;
	}
	
	//close(s32fd);
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_UART_Set(ENN_U8 *pdata, ENN_U32 len)
{
	ENN_U8 *pTemp = NULL;
	ENN_U32 length = 0;
	ENN_U8		channel = 0; 
	ENN_U32 	baudrate = 0;
	ENN_U8 		databit = 0;
	ENN_U8 		stopbit = 0;
	ENN_U8 		parity = 0;
	Channel_List *pCurrent_Channel_List = NULL;
	ENN_U8 		u8Protocol = 0;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

	if(NULL == gSlave_Set_List_Head)
	{
		return ENN_FAIL;
	}

	pTemp = pdata;
	length = len;

	u8Protocol = *pTemp;
	length--;
	pTemp++;
	
	channel = *pTemp;
	length--;
	pTemp++;

	/*查询通道*/
	pCurrent_Channel_List = ENNModBus_Channel_Search(channel);
	if(NULL == pCurrent_Channel_List)
	{
		return ENN_FAIL;
	}

	pCurrent_Channel_List->u8Protocol = u8Protocol;
	
	memcpy(&baudrate, pTemp, 4);
	length -= 4;
	pTemp += 4;
	pCurrent_Channel_List->u32BaudRate = baudrate;

	databit = *pTemp;
	length--;
	pTemp++;
	pCurrent_Channel_List->u8DataBit = databit;

	stopbit = *pTemp;
	length--;
	pTemp++;
	pCurrent_Channel_List->u8StoptBit = stopbit;

	parity = *pTemp;
	length--;
	pCurrent_Channel_List->u8Parity = parity;

	debug_int();

	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Slave_Get(ENN_U8 *pdata, ENN_U32 len)
{
	ENN_U8 data[1024];
	ENN_U8 *pBuf = NULL;
	ENN_U16 u16Len = 0;
	ENN_U8 *pTemp = NULL;
	ENN_U32 length = 0;
	ENN_U8		channel = 0; 
	ENN_U8 		u8SlaveNum = 0;
	ENN_U32 CMD_RSP = 0;
	ENN_U8 i = 0;
	ENN_CHAR name[20];
	ENN_CHAR slave_name[20];
	ENN_U8  Name_Len = 0;
	ENN_S32 s32ret = 0;
	ENN_S32 s32fd = 0;
	Channel_List *pCurrent_Channel_List = NULL;
	Slave_List	 *pCurrent_Slave_List = NULL;
	int ret = 0;
	Protocol_Type eProtocol_Type = PROTOCOL_MODBUS;
	Dev645_List *pCurrent_Dev645_List = NULL;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

#if 1
	if(NULL == gSlave_Set_List_Head)
	{
		return ENN_FAIL;
	}

	pTemp = pdata;
	length = len;

	channel = *pTemp;
	length--;
	pTemp++;

	/*查询通道*/
	pCurrent_Channel_List = ENNModBus_Channel_Search(channel);
	if(NULL == pCurrent_Channel_List)
	{
		return ENN_FAIL;
	}
	
	u8SlaveNum = pCurrent_Channel_List->u8SlaveNum;
	eProtocol_Type = (Protocol_Type)pCurrent_Channel_List->u8Protocol;
	
	memset(data, 0, 1024);
	pBuf = &data;
	pBuf += 2;
	
	CMD_RSP = ENN_CMD_SLAVE_GET_RSP;
	memcpy(pBuf, &CMD_RSP, 2);
	u16Len += 2;
	pBuf += 2;

	*pBuf = channel;
	u16Len++;
	pBuf++;

	*pBuf = (ENN_U8)eProtocol_Type;
	u16Len++;
	pBuf++;

	*pBuf = u8SlaveNum;
	u16Len++;
	pBuf++;

	PIPE_PRINT("%s, %d, Slave_Num = %d\n",__FUNCTION__,__LINE__, u8SlaveNum);	
	if(PROTOCOL_MODBUS == eProtocol_Type)
	{
		pCurrent_Slave_List = pCurrent_Channel_List->unDeviceType.pModBus_List;;
		while(NULL != pCurrent_Slave_List)
		{
			i++;

			*pBuf = 1;
			u16Len++;
			pBuf++;
			
			*pBuf = pCurrent_Slave_List->u8SlaveAddr;
			u16Len++;
			pBuf++;

			/*memset(name, 0, 20);
			sprintf(name, "设备%d", i);
			PIPE_PRINT("%s, %d, 设备名 = %s,   %d\n",__FUNCTION__,__LINE__, name, strlen(name));*/

			/*memset(slave_name, 0, 20);
			ret = GB2312_To_UTF8(pCurrent_Slave_List->pSlaveName, strlen(pCurrent_Slave_List->pSlaveName), slave_name, 20);
			PIPE_PRINT("%s, %d, ret = %d\n",__FUNCTION__,__LINE__, ret);
			PIPE_PRINT("%s, %d, 设备名 = %s,   %d\n",__FUNCTION__,__LINE__, slave_name, strlen(slave_name));*/
			
			PIPE_PRINT("%s, %d, name = %s,   %d\n",__FUNCTION__,__LINE__, pCurrent_Slave_List->pSlaveName, strlen(pCurrent_Slave_List->pSlaveName));
			Name_Len = strlen(pCurrent_Slave_List->pSlaveName);
			*pBuf = Name_Len;
			u16Len++;
			pBuf++;

			memcpy(pBuf, pCurrent_Slave_List->pSlaveName, Name_Len);
			u16Len += Name_Len;
			pBuf += Name_Len;
			
			pCurrent_Slave_List = pCurrent_Slave_List->next;
		}
	}
	else if((PROTOCOL_645_1997 == eProtocol_Type) || (PROTOCOL_645_2007 == eProtocol_Type))
	{
		pCurrent_Dev645_List = pCurrent_Channel_List->unDeviceType.pDev645_List;
		while(NULL != pCurrent_Dev645_List)
		{
			i++;
			
			*pBuf = (ENN_U8)strlen(pCurrent_Dev645_List->ucDevAddr);
			u16Len++;
			pBuf++;

			memcpy(pBuf, pCurrent_Dev645_List->ucDevAddr, strlen(pCurrent_Dev645_List->ucDevAddr));
			u16Len += strlen(pCurrent_Dev645_List->ucDevAddr);
			pBuf += strlen(pCurrent_Dev645_List->ucDevAddr);

			Name_Len = strlen(pCurrent_Dev645_List->pSlaveName);
			*pBuf = Name_Len;
			u16Len++;
			pBuf++;

			memcpy(pBuf, pCurrent_Dev645_List->pSlaveName, Name_Len);
			u16Len += Name_Len;
			pBuf += Name_Len;
			
			pCurrent_Dev645_List = pCurrent_Dev645_List->next;
		}	
	}
	else
	{
		return ENN_FAIL;
	}

	pBuf = &data;
	memcpy(pBuf, &u16Len, 2);
	u16Len += 2;

	for(i=0; i<u16Len; i++)
		PIPE_PRINT("%x ", pBuf[i]);
	PIPE_PRINT("\n");

	s32ret = ENNFIFO_create(FIFIO_HTML_OUT_MSG);
	PIPE_PRINT("%s, %d, s32ret = %d\n",__FUNCTION__,__LINE__, s32ret);	
	if(s32ret < 0)
	{
        perror("create failed:\n");
		return ENN_FAIL;
	}

	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);	
	//s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_WRONLY | O_NONBLOCK);
	//s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_WRONLY | O_NONBLOCK);
	s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_RDWR | O_NONBLOCK);
	PIPE_PRINT("%s, %d, s32fd = %d\n",__FUNCTION__,__LINE__, s32fd);	
	if(s32fd < 0)
	{
        perror("open failed:\n");
		return ENN_FAIL;
	}

	s32ret = ENNFIFO_write(s32fd, pBuf, u16Len);
	PIPE_PRINT("%s, %d, s32ret = %d, u16Len = %d\n",__FUNCTION__,__LINE__, s32ret, u16Len);	
	if((s32ret < 0) || (s32ret != u16Len))
	{
        perror("write failed:\n");
		return ENN_FAIL;
	}
	
	//close(s32fd);
#endif
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Slave_Set(ENN_U8 *pdata, ENN_U32 len)
{
	ENN_U16 u16Len = 0;
	ENN_U8 *pTemp = NULL;
	ENN_U32 length = 0;
	ENN_U8		channel = 0; 
	ENN_U8 		u8SlaveNum = 0;
	ENN_U8 		u8OldSlaveAddr = 0;
	ENN_U8 		u8NewSlaveAddr = 0;
	ENN_U8		type = 0;
	ENN_U8 i = 0;
	ENN_CHAR name[20];
	ENN_CHAR slave_name[20];
	ENN_CHAR slave_name1[20];
	ENN_CHAR *pslave_name = NULL;;
	ENN_U8  Name_Len = 0;
	ENN_S32 s32ret = 0;
	ENN_S32 s32fd = 0;
	Channel_List *pCurrent_Channel_List = NULL;
	Slave_List *pSearch_Slave_List = NULL;
	Slave_List	 *pCurrent_Slave_List = NULL;
	Slave_List *pPrevious_Slave_List = NULL;
	Slave_List *pSlave_List_Add = NULL;
	Slave_List *pSlave_List_Temp = NULL;
	Slave_FunCode_List *pCurrent_FunCode_List = NULL;
	Slave_FunCode_List *pCurrent_FunCode_Temp = NULL;
	Register_List *pCurrent_Register_List = NULL;
	Register_List *pCurrent_Register_Temp = NULL;
	int ret = 0;
	ENN_CHAR old645_addr[16];
	ENN_CHAR new645_addr[16];
	ENN_U8  Addr_Len = 0;
	Protocol_Type eProtocol_Type = PROTOCOL_MODBUS;
	Dev645_List *pSearch_Dev645_List = NULL;
	Dev645_List *pCurrent_Dev645_List = NULL;
	Dev645_List *pPrevious_Dev645_List = NULL;
	Dev645_List *pDev645_List_Add = NULL;
	Dev645_List *pDev645_List_Temp = NULL;
	Code_645_List *pCode_645_List_Temp = NULL;
	Code_645_List *pCode_645_List = NULL;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

	if(NULL == gSlave_Set_List_Head)
	{
		return ENN_FAIL;
	}

	pTemp = pdata;
	length = len;

	channel = *pTemp;
	length--;
	pTemp++;

	eProtocol_Type = (Protocol_Type)*pTemp;
	length--;
	pTemp++;

	type = *pTemp;
	length--;
	pTemp++;

	/*查询通道*/
	pCurrent_Channel_List = ENNModBus_Channel_Search(channel);
	if(NULL == pCurrent_Channel_List)
	{
		free(pslave_name);
		pslave_name = NULL;
		return ENN_FAIL;
	}
	u8SlaveNum = pCurrent_Channel_List->u8SlaveNum;

	if(PROTOCOL_MODBUS == eProtocol_Type)
	{
		length--;
		pTemp++;
		
		u8OldSlaveAddr = *pTemp;
		length--;
		pTemp++;

		length--;
		pTemp++;
		
		u8NewSlaveAddr = *pTemp;
		length--;
		pTemp++;

		Name_Len = *pTemp;
		length--;
		pTemp++;

		memset(slave_name, 0, 20);
		memcpy(slave_name, pTemp, Name_Len);

		memset(slave_name1, 0, 20);
		ret = UTF8_To_GB2312(slave_name, strlen(slave_name), slave_name1, 20);
		pslave_name = (ENN_CHAR *)malloc(strlen(slave_name1) + 1);
		if(NULL == pslave_name)
		{
			return ENN_FAIL;
		}
		
		memset((void *)pslave_name, 0, (strlen(slave_name1) + 1));
		strcpy((void *)pslave_name, slave_name1);
		
		PIPE_PRINT("%s, %d, ret = %d\n",__FUNCTION__,__LINE__, ret);
		PIPE_PRINT("%s, %d, name = %s,   %d\n",__FUNCTION__,__LINE__, pslave_name, strlen(pslave_name));
		
		//pCurrent_Slave_List = pCurrent_Channel_List->pSlaveList;
		PIPE_PRINT("%s, %d, u8SlaveNum = %d\n",__FUNCTION__,__LINE__, u8SlaveNum);

		pSearch_Slave_List = ENNModBus_Slave_Search(u8OldSlaveAddr, 
													pCurrent_Channel_List, 
													&pCurrent_Slave_List, 
													&pPrevious_Slave_List);
		PIPE_PRINT("%s, %d, pSearch_Slave_List = %x\n",__FUNCTION__,__LINE__, pSearch_Slave_List);

		switch(type)
		{
			case 1:
				if(NULL != pSearch_Slave_List)
				{
					free(pslave_name);
					pslave_name = NULL;
					return ENN_FAIL;
				}
				
				pSlave_List_Add = (Slave_List *)malloc(sizeof(Slave_List));
				if(NULL == pSlave_List_Add)
				{
					free(pslave_name);
					pslave_name = NULL;
					return ENN_FAIL;
				}
				memset(pSlave_List_Add, 0, sizeof(Slave_List));
				pSlave_List_Add->u8SlaveAddr = u8OldSlaveAddr;
				pSlave_List_Add->pSlaveName = pslave_name;
				pSlave_List_Add->next = NULL;
				pSlave_List_Add->pFunCode_List = NULL;
				pCurrent_Channel_List->u8SlaveNum++;

				if(NULL == pCurrent_Channel_List->unDeviceType.pModBus_List)
				{
					pCurrent_Channel_List->unDeviceType.pModBus_List = pSlave_List_Add;
				}
				else
				{
					pCurrent_Slave_List->next = pSlave_List_Add;
				}
				break;
			case 2:
				free(pslave_name);
				pslave_name = NULL;
				if(NULL == pSearch_Slave_List)
				{
					return ENN_FAIL;
				}
				
				PIPE_PRINT("%s, %d, pSlave_List_Temp = %x\n",__FUNCTION__,__LINE__,pSlave_List_Temp);
				PIPE_PRINT("%s, %d, pPrevious_Slave_List = %x\n",__FUNCTION__,__LINE__,pPrevious_Slave_List);
				PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
				if(pSearch_Slave_List == pCurrent_Channel_List->unDeviceType.pModBus_List)
				{
					pSlave_List_Temp = pSearch_Slave_List;
					pCurrent_Channel_List->unDeviceType.pModBus_List = pSearch_Slave_List->next;
				}
				else
				{
					pSlave_List_Temp = pSearch_Slave_List;
					pPrevious_Slave_List->next = pSlave_List_Temp->next;
				}
				pCurrent_Channel_List->u8SlaveNum--;
				PIPE_PRINT("%s, %d, u8SlaveNum = %d\n",__FUNCTION__,__LINE__, pCurrent_Channel_List->u8SlaveNum);

				//删除设备
				pCurrent_FunCode_List = pSlave_List_Temp->pFunCode_List;
				PIPE_PRINT("%s, %d, pCurrent_FunCode_List = %x\n",__FUNCTION__,__LINE__, pCurrent_FunCode_List);
				while(NULL != pCurrent_FunCode_List)
				{
					PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
					pCurrent_Register_List = pCurrent_FunCode_List->pRegister_List;
					while(NULL != pCurrent_Register_List)
					{
						pCurrent_Register_Temp = pCurrent_Register_List;
						pCurrent_Register_List = pCurrent_Register_List->next;

						if(NULL != pCurrent_Register_Temp->pName)
						{
							free(pCurrent_Register_Temp->pName);
							pCurrent_Register_Temp->pName = NULL;
						}

						if(NULL != pCurrent_Register_Temp->pAttr)
						{
							free(pCurrent_Register_Temp->pAttr);
							pCurrent_Register_Temp->pAttr = NULL;
						}

						free(pCurrent_Register_Temp);
						pCurrent_Register_Temp = NULL;
					}

					pCurrent_FunCode_Temp = pCurrent_FunCode_List;
					pCurrent_FunCode_List = pCurrent_FunCode_List->next;

					free(pCurrent_FunCode_Temp);
					pCurrent_FunCode_Temp = NULL;
				}
				PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
				
				free(pSlave_List_Temp->pSlaveName);
				PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
				pSlave_List_Temp->pSlaveName = NULL;
				free(pSlave_List_Temp);
				PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
				pSlave_List_Temp = NULL;
				break;
			case 3:
				if(NULL == pSearch_Slave_List)
				{
					free(pslave_name);
					pslave_name = NULL;
					return ENN_FAIL;
				}

				pSearch_Slave_List->u8SlaveAddr = u8NewSlaveAddr;
				free(pSearch_Slave_List->pSlaveName);
				pSearch_Slave_List->pSlaveName = NULL;
				pSearch_Slave_List->pSlaveName = pslave_name;
				
				break;
			default:
				return ENN_FAIL;
		}
	}
	else if((PROTOCOL_645_1997 == eProtocol_Type) || (PROTOCOL_645_2007 == eProtocol_Type))
	{
		Addr_Len = *pTemp;
		length--;
		pTemp++;

		memset(old645_addr, 0, 16);
		memcpy(old645_addr, pTemp, Addr_Len);
		length -= Addr_Len;
		pTemp += Addr_Len;
		PIPE_PRINT("%s, %d, old645_addr = %s[%d]\n",__FUNCTION__,__LINE__, old645_addr, strlen(old645_addr));
		
		Addr_Len = *pTemp;
		length--;
		pTemp++;

		memset(new645_addr, 0, 16);
		memcpy(new645_addr, pTemp, Addr_Len);
		length -= Addr_Len;
		pTemp += Addr_Len;
		PIPE_PRINT("%s, %d, new645_addr = %s[%d]\n",__FUNCTION__,__LINE__, new645_addr, strlen(new645_addr));

		Name_Len = *pTemp;
		length--;
		pTemp++;

		memset(slave_name, 0, 20);
		memcpy(slave_name, pTemp, Name_Len);

		memset(slave_name1, 0, 20);
		ret = UTF8_To_GB2312(slave_name, strlen(slave_name), slave_name1, 20);
		pslave_name = (ENN_CHAR *)malloc(strlen(slave_name1) + 1);
		if(NULL == pslave_name)
		{
			return ENN_FAIL;
		}
		memset((void *)pslave_name, 0, (strlen(slave_name1) + 1));
		strcpy((void *)pslave_name, slave_name1);
		PIPE_PRINT("%s, %d, pslave_name = %s[%d]\n",__FUNCTION__,__LINE__, pslave_name, strlen(pslave_name));

		PIPE_PRINT("%s, %d, pCurrent_Channel_List = %p\n",__FUNCTION__,__LINE__, pCurrent_Channel_List);
		pSearch_Dev645_List = ENNDev_645_Search(old645_addr, 
													pCurrent_Channel_List, 
													&pCurrent_Dev645_List, 
													&pPrevious_Dev645_List);
		PIPE_PRINT("%s, %d, pSearch_Dev645_List = %x\n",__FUNCTION__,__LINE__, pSearch_Dev645_List);

		switch(type)
		{
			case 1:
				if(NULL != pSearch_Dev645_List)
				{
					free(pslave_name);
					pslave_name = NULL;
					return ENN_FAIL;
				}
				
				pDev645_List_Add = (Dev645_List *)malloc(sizeof(Dev645_List));
				if(NULL == pDev645_List_Add)
				{
					free(pslave_name);
					pslave_name = NULL;
					return ENN_FAIL;
				}
				memset(pDev645_List_Add, 0, sizeof(Dev645_List));
				memset(pDev645_List_Add->ucDevAddr, 0, 16);
				memcpy(pDev645_List_Add->ucDevAddr, new645_addr, Addr_Len);
				pDev645_List_Add->pSlaveName = pslave_name;
				pDev645_List_Add->next = NULL;
				pDev645_List_Add->pCode_645_List = NULL;
				pCurrent_Channel_List->u8SlaveNum++;

				if(NULL == pCurrent_Channel_List->unDeviceType.pDev645_List)
				{
					pCurrent_Channel_List->unDeviceType.pDev645_List = pDev645_List_Add;
				}
				else
				{
					pCurrent_Dev645_List->next = pDev645_List_Add;
				}
				break;
			case 2:
				free(pslave_name);
				pslave_name = NULL;
				if(NULL == pSearch_Dev645_List)
				{
					return ENN_FAIL;
				}
				
				PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
				if(pSearch_Dev645_List == pCurrent_Channel_List->unDeviceType.pDev645_List)
				{
					pDev645_List_Temp = pSearch_Dev645_List;
					pCurrent_Channel_List->unDeviceType.pDev645_List = pSearch_Dev645_List->next;
				}
				else
				{
					pDev645_List_Temp = pSearch_Dev645_List;
					pPrevious_Dev645_List->next = pDev645_List_Temp->next;
				}
				pCurrent_Channel_List->u8SlaveNum--;
				PIPE_PRINT("%s, %d, u8SlaveNum = %d\n",__FUNCTION__,__LINE__, pCurrent_Channel_List->u8SlaveNum);

				//删除设备
				pCode_645_List = pDev645_List_Temp->pCode_645_List;
				PIPE_PRINT("%s, %d, pCode_645_List = %x\n",__FUNCTION__,__LINE__, pCode_645_List);
				while(NULL != pCode_645_List)
				{
					pCode_645_List_Temp = pCode_645_List;
					pCode_645_List = pCode_645_List->next;

					free(pCode_645_List_Temp->pName);
					pCode_645_List_Temp->pName = NULL;

					free(pCode_645_List_Temp->pAttr);
					pCode_645_List_Temp->pAttr = NULL;

					free(pCode_645_List_Temp);
					pCode_645_List_Temp = NULL;
				}
				PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
				
				free(pDev645_List_Temp->pSlaveName);
				PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
				pDev645_List_Temp->pSlaveName = NULL;
				free(pDev645_List_Temp);
				PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
				pDev645_List_Temp = NULL;
				break;
			case 3:
				if(NULL == pSearch_Dev645_List)
				{
					free(pslave_name);
					pslave_name = NULL;
					return ENN_FAIL;
				}

				memset(pSearch_Dev645_List->ucDevAddr, 0, 16);
				memcpy(pSearch_Dev645_List->ucDevAddr, new645_addr, Addr_Len);
				free(pSearch_Dev645_List->pSlaveName);
				pSearch_Dev645_List->pSlaveName = NULL;
				pSearch_Dev645_List->pSlaveName = pslave_name;
				
				break;
			default:
				return ENN_FAIL;
		}
	}
	debug_int();

	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Slave_Reg_Get(ENN_U8 *pdata, ENN_U32 len)
{
	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
	//ENN_U8 data[1024*5+512];
	ENN_U8 *data = NULL;
	ENN_U8 *pBuf = NULL;
	ENN_U8 *pTemp1 = NULL;
	ENN_U8 *pTemp2 = NULL;
	ENN_U16 u16Len = 0;
	ENN_U8 *pTemp = NULL;
	ENN_U32 length = 0;
	ENN_U8		channel = 0; 
	ENN_U8 		u8SlaveNum = 0;
	ENN_U32 CMD_RSP = 0;
	ENN_U32 i = 0;
	//ENN_CHAR name[20];
	ENN_CHAR slave_name[20];
	ENN_U8  Name_Len = 0;
	ENN_S32 s32ret = 0;
	ENN_S32 s32fd = 0;
	Channel_List *pCurrent_Channel_List = NULL;
	Slave_List *pSearch_Slave_List = NULL;
	Slave_List	 *pCurrent_Slave_List = NULL;
	Slave_List *pPrevious_Slave_List = NULL;
	ENN_U8 		u8SlaveAddr = 0;
	Slave_FunCode_List *pCurrent_FunCode_List = NULL;
	Slave_FunCode_List *pCurrent_FunCode_Temp = NULL;
	Register_List *pCurrent_Register_List = NULL;
	Register_List *pCurrent_Register_Temp = NULL;
	ENN_U8 		u8FuncodeCount = 0;
	ENN_U8 		u8NameLen = 0;
	ENN_U16 		u16RegAddr = 0;
	ENN_U16 		u16RegNum = 0;
	ENN_CHAR 		*pDataFormat = "AB";
	float			fK = 3.16;
	ENN_U8 			u8D = 5;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

#if 1
	pTemp = pdata;
	length = len;

	channel = *pTemp;
	length--;
	pTemp++;

	u8SlaveAddr = *pTemp;
	length--;
	pTemp++;
	
	/*查询通道*/
	pCurrent_Channel_List = ENNModBus_Channel_Search(channel);
	if(NULL == pCurrent_Channel_List)
	{
		goto FIFOWRITE;
	}
	
	u8SlaveNum = pCurrent_Channel_List->u8SlaveNum;
	//pCurrent_Slave_List = pCurrent_Channel_List->pSlaveList;

	pSearch_Slave_List = ENNModBus_Slave_Search(u8SlaveAddr, 
												pCurrent_Channel_List, 
												&pCurrent_Slave_List, 
												&pPrevious_Slave_List);

	if(NULL == pSearch_Slave_List)
	{
		goto FIFOWRITE;
	}

	pCurrent_FunCode_List = pSearch_Slave_List->pFunCode_List;

FIFOWRITE:	
	data = (ENN_U8 *)malloc(1024*1024);
	if(NULL == data)
	{
		return ENN_FAIL;
	}
	memset(data, 0, 1024*1024);
	//memset(data, 0, 1024*5+512);
	//pBuf = &data;
	pBuf = data;
	pBuf += 2;
	
	CMD_RSP = ENN_CMD_SLAVE_REG_GET_RSP;
	memcpy(pBuf, &CMD_RSP, 2);
	u16Len += 2;
	pBuf += 2;

	*pBuf = channel;
	u16Len++;
	pBuf++;

	*pBuf = u8SlaveAddr;
	u16Len++;
	pBuf++;
	pTemp1 = pBuf;
	u16Len++;
	pBuf++;

	while(NULL != pCurrent_FunCode_List)
	{
		u8FuncodeCount++;
		*pBuf = pCurrent_FunCode_List->u8MBFunCode;
		u16Len++;
		pBuf++;

		memcpy(pBuf, &(pCurrent_FunCode_List->u16TotalRegNum), 2);
		u16Len += 2;
		pBuf += 2;

		pCurrent_Register_List = pCurrent_FunCode_List->pRegister_List;
		while(NULL != pCurrent_Register_List)
		{			
			u16RegAddr = pCurrent_Register_List->u16RegAddr;
			u16RegNum = pCurrent_Register_List->u16RegNum;

			memcpy(pBuf, &u16RegAddr, 2);
			u16Len += 2;
			pBuf += 2;
			memcpy(pBuf, &u16RegNum, 2);
			u16Len += 2;
			pBuf += 2;

			u8NameLen = strlen(pDataFormat);
			*pBuf = u8NameLen;
			u16Len++;
			pBuf++;
			//PIPE_PRINT("%s, %d, u8NameLen = %d\n",__FUNCTION__,__LINE__,u8NameLen);
			
			memcpy(pBuf, pDataFormat, u8NameLen);
			u16Len += u8NameLen;
			pBuf += u8NameLen;
			
			u8NameLen = strlen(pCurrent_Register_List->pName);
			*pBuf = u8NameLen;
			u16Len++;
			pBuf++;
			//PIPE_PRINT("%s, %d, u8NameLen = %d\n",__FUNCTION__,__LINE__,u8NameLen);
			
			memcpy(pBuf, pCurrent_Register_List->pName, u8NameLen);
			u16Len += u8NameLen;
			pBuf += u8NameLen;
			
			u8NameLen = strlen(pCurrent_Register_List->pAttr);
			*pBuf = u8NameLen;
			u16Len++;
			pBuf++;
			//PIPE_PRINT("%s, %d, u8NameLen = %d\n",__FUNCTION__,__LINE__,u8NameLen);
			
			memcpy(pBuf, pCurrent_Register_List->pAttr, u8NameLen);
			u16Len += u8NameLen;
			pBuf += u8NameLen;
			
			memcpy(pBuf, &fK, 4);
			u16Len += 4;
			pBuf += 4;
			PIPE_PRINT("%s, %d, u16Len = %d\n",__FUNCTION__,__LINE__,u16Len);
			
			memcpy(pBuf, &u8D, 1);
			u16Len++;
			pBuf++;
			PIPE_PRINT("%s, %d, u16Len = %d\n",__FUNCTION__,__LINE__,u16Len);
			
			pCurrent_Register_List = pCurrent_Register_List->next;
		}
		
		pCurrent_FunCode_List = pCurrent_FunCode_List->next;
	}

	*pTemp1 = u8FuncodeCount;	

	pBuf = data;
	memcpy(pBuf, &u16Len, 2);
	u16Len += 2;

	for(i=0; i<u16Len; i++)
	{
		if(i%10 == 0)
		{
			PIPE_PRINT("\n");
		}
		PIPE_PRINT("%2.2x ", pBuf[i]);
	}
	PIPE_PRINT("\n");

	s32ret = ENNFIFO_create(FIFIO_HTML_OUT_MSG);
	PIPE_PRINT("%s, %d, s32ret = %d\n",__FUNCTION__,__LINE__, s32ret);	
	if(s32ret < 0)
	{
        perror("create failed:\n");
		free(data);
		data = NULL;
		return ENN_FAIL;
	}

	//s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_WRONLY | O_NONBLOCK);
	//s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_WRONLY | O_NONBLOCK);
	s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_RDWR | O_NONBLOCK);
	PIPE_PRINT("%s, %d, s32fd = %d\n",__FUNCTION__,__LINE__, s32fd);	
	if(s32fd < 0)
	{
        perror("open failed:\n");
		free(data);
		data = NULL;
		return ENN_FAIL;
	}

	s32ret = ENNFIFO_write(s32fd, pBuf, u16Len);
	PIPE_PRINT("%s, %d, s32ret = %d, u16Len = %d\n",__FUNCTION__,__LINE__, s32ret, u16Len);	
	if((s32ret < 0) || (s32ret != u16Len))
	{
        perror("write failed:\n");
		free(data);
		data = NULL;
		return ENN_FAIL;
	}
	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);	
	
	//close(s32fd);
	free(data);
	data = NULL;
	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);	

#endif
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Slave_Reg_Set(ENN_U8 *pdata, ENN_U32 len)
{
	ENN_U8 *pTemp = NULL;
	ENN_U32 length = 0;
	ENN_U8		channel = 0; 
	ENN_U32 CMD_RSP = 0;
	ENN_U8 i = 0;
	ENN_S32 s32ret = 0;
	ENN_S32 s32fd = 0;
	Channel_List *pCurrent_Channel_List = NULL;
	Slave_List *pSearch_Slave_List = NULL;
	Slave_List	 *pCurrent_Slave_List = NULL;
	Slave_List *pPrevious_Slave_List = NULL;
	ENN_U8 		u8SlaveAddr = 0;
	Slave_FunCode_List *pCurrent_FunCode_List = NULL;
	Slave_FunCode_List *pCurrent_FunCode_Temp = NULL;
	Slave_FunCode_List *pFunCode_List_Add = NULL;
	Register_List *pPrevious_Register_List = NULL;
	Register_List *pCurrent_Register_List = NULL;
	Register_List *pRegister_List_Temp = NULL;
	Register_List *pRegister_List_Add = NULL;
	ENN_U8 		u8Funcode = 0;
	ENN_U8 		u8Type = 0;
	ENN_U16 		u16OldRegAddr = 0;
	ENN_U16 		u16RegAddr = 0;
	ENN_U16 		u16RegNum = 0;
	float			fK = 3.16;
	ENN_U8 			u8D = 5;
	ENN_U8 			u8NameLen = 0;
	ENN_CHAR 		*dataformat = NULL;
	ENN_CHAR 		adataformat[10];
	ENN_CHAR 		*reg_name = NULL;
	ENN_CHAR 		*reg_des = NULL;
	ENN_CHAR 		*reg_name1 = NULL;
	ENN_CHAR 		*reg_des1 = NULL;
	ENN_CHAR 		areg_name[100];
	ENN_CHAR 		areg_des[100];
	int ret = 0;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

#if 1
	if(NULL == gSlave_Set_List_Head)
	{
		return ENN_FAIL;
	}

	pTemp = pdata;
	length = len;

	channel = *pTemp;
	length--;
	pTemp++;

	u8SlaveAddr = *pTemp;
	length--;
	pTemp++;
	
	u8Funcode = *pTemp;
	length--;
	pTemp++;
	
	u8Type = *pTemp;
	length--;
	pTemp++;

	memcpy(&u16OldRegAddr, pTemp, 2);
	length -= 2;
	pTemp += 2;
	
	memcpy(&u16RegAddr, pTemp, 2);
	length -= 2;
	pTemp += 2;

	memcpy(&u16RegNum, pTemp, 2);
	length -= 2;
	pTemp += 2;

	u8NameLen= *pTemp;
	length--;
	pTemp++;

	dataformat = (ENN_CHAR *)malloc(u8NameLen + 1);
	if(NULL == dataformat)
	{
		return ENN_FAIL;
	}
	memset(dataformat, 0, u8NameLen + 1);
	memcpy(dataformat, pTemp, u8NameLen);
	length -= u8NameLen;
	pTemp += u8NameLen;

	memset(adataformat, 0, 10);
	ret = UTF8_To_GB2312(dataformat, strlen(dataformat), adataformat, 10);
	free(dataformat);
	dataformat = NULL;
	if(0 != ret)
	{
		return ENN_FAIL;
	}

	u8NameLen= *pTemp;
	length--;
	pTemp++;
	
	reg_name = (ENN_CHAR *)malloc(u8NameLen + 1);
	if(NULL == reg_name)
	{
		return ENN_FAIL;
	}
	memset(reg_name, 0, u8NameLen + 1);
	memcpy(reg_name, pTemp, u8NameLen);
	length -= u8NameLen;
	pTemp += u8NameLen;
	PIPE_PRINT("%s, %d, des = %s,   %d\n",__FUNCTION__,__LINE__, reg_name, strlen(reg_name));

	memset(areg_name, 0, 100);
	ret = UTF8_To_GB2312(reg_name, strlen(reg_name), areg_name, 100);
	free(reg_name);
	reg_name = NULL;
	PIPE_PRINT("%s, %d, name = %s,   %d\n",__FUNCTION__,__LINE__, areg_name, strlen(areg_name));
	
	reg_name1 = (ENN_CHAR *)malloc(strlen(areg_name) + 1);
	if(NULL == reg_name1)
	{
		return ENN_FAIL;
	}
	memset(reg_name1, 0, strlen(areg_name) + 1);
	memcpy(reg_name1, areg_name, strlen(areg_name));

	u8NameLen= *pTemp;
	length--;
	pTemp++;
	
	reg_des = (ENN_CHAR *)malloc(u8NameLen + 1);
	if(NULL == reg_des)
	{
		free(reg_name1);
		reg_name1 = NULL;
		return ENN_FAIL;
	}
	memset(reg_des, 0, u8NameLen + 1);
	memcpy(reg_des, pTemp, u8NameLen);
	length -= u8NameLen;
	pTemp += u8NameLen;
	PIPE_PRINT("%s, %d, des = %s,   %d\n",__FUNCTION__,__LINE__, reg_des, strlen(reg_des));

	memset(areg_des, 0, 100);
	ret = UTF8_To_GB2312(reg_des, strlen(reg_des), areg_des, 100);
	free(reg_des);
	reg_des = NULL;
	PIPE_PRINT("%s, %d, des = %s,   %d\n",__FUNCTION__,__LINE__, areg_des, strlen(areg_des));

	reg_des1 = (ENN_CHAR *)malloc(strlen(areg_des) + 1);
	if(NULL == reg_des1)
	{
		free(reg_name1);
		reg_name1 = NULL;
		return ENN_FAIL;
	}
	memset(reg_des1, 0, strlen(areg_des) + 1);
	memcpy(reg_des1, areg_des, strlen(areg_des));
	
	/*查询通道*/
	pCurrent_Channel_List = ENNModBus_Channel_Search(channel);
	if(NULL == pCurrent_Channel_List)
	{
		free(reg_des1);
		reg_des1 = NULL;
		free(reg_name1);
		reg_name1 = NULL;
		return ENN_FAIL;
	}
	
	pSearch_Slave_List = ENNModBus_Slave_Search(u8SlaveAddr, 
												pCurrent_Channel_List, 
												&pCurrent_Slave_List, 
												&pPrevious_Slave_List);
	if(NULL == pSearch_Slave_List)
	{
		free(reg_des1);
		reg_des1 = NULL;
		free(reg_name1);
		reg_name1 = NULL;
		return ENN_FAIL;
	}

	if(0 == strcmp(adataformat, "AB"))
	{
		pSearch_Slave_List->u8DataFormat = 0;
	}
	else
	{
		pSearch_Slave_List->u8DataFormat = 1;
	}
	
	if(NULL != pSearch_Slave_List->pFunCode_List)
	{
		pCurrent_FunCode_List = pSearch_Slave_List->pFunCode_List;
		while((u8Funcode != pCurrent_FunCode_List->u8MBFunCode)
			&&(NULL != pCurrent_FunCode_List->next))
		{
			pCurrent_FunCode_List = pCurrent_FunCode_List->next;
		}
	}
	
	switch(u8Type)
	{
		case 1:
			if((NULL == pSearch_Slave_List->pFunCode_List)
				||(u8Funcode != pCurrent_FunCode_List->u8MBFunCode))
			{
				pFunCode_List_Add = (Slave_FunCode_List *)malloc(sizeof(Slave_FunCode_List));
				if(NULL == pFunCode_List_Add)
				{
					free(reg_des1);
					reg_des1 = NULL;
					free(reg_name1);
					reg_name1 = NULL;
					return ENN_FAIL;
				}
				memset(pFunCode_List_Add, 0, sizeof(Slave_FunCode_List));
				pFunCode_List_Add->u8MBFunCode = u8Funcode;
				pFunCode_List_Add->u16TotalRegNum = 1;
				pFunCode_List_Add->pRegister_List = NULL;
				pFunCode_List_Add->next = NULL;

				pFunCode_List_Add->pRegister_List = (Register_List *)malloc(sizeof(Register_List));
				if(NULL == pFunCode_List_Add->pRegister_List)
				{
					free(pFunCode_List_Add);
					pFunCode_List_Add = NULL;
					free(reg_des1);
					reg_des1 = NULL;
					free(reg_name1);
					reg_name1 = NULL;
					return ENN_FAIL;
				}
				memset(pFunCode_List_Add->pRegister_List, 0, sizeof(Register_List));
				pFunCode_List_Add->pRegister_List->u16RegAddr = u16RegAddr;
				pFunCode_List_Add->pRegister_List->u16RegNum = u16RegNum;
				pFunCode_List_Add->pRegister_List->pName = reg_name1;
				pFunCode_List_Add->pRegister_List->pAttr = reg_des1;
				pFunCode_List_Add->pRegister_List->next = NULL;

				if(NULL == pSearch_Slave_List->pFunCode_List)
				{
					pSearch_Slave_List->pFunCode_List = pFunCode_List_Add;
				}
				else
				{
					pCurrent_FunCode_List->next = pFunCode_List_Add;
				}
			}
			else
			{
				pRegister_List_Add = (Register_List *)malloc(sizeof(Register_List));
				if(NULL == pRegister_List_Add)
				{
					free(reg_des1);
					reg_des1 = NULL;
					free(reg_name1);
					reg_name1 = NULL;
					return ENN_FAIL;
				}
				memset(pRegister_List_Add, 0, sizeof(Register_List));
				pRegister_List_Add->u16RegAddr = u16RegAddr;
				pRegister_List_Add->u16RegNum = u16RegNum;
				pRegister_List_Add->pName = reg_name1;
				pRegister_List_Add->pAttr = reg_des1;
				pRegister_List_Add->next = NULL;
				if(NULL == pCurrent_FunCode_List->pRegister_List)
				{
					pCurrent_FunCode_List->pRegister_List = pRegister_List_Add;
				}
				else
				{
					pCurrent_Register_List = pCurrent_FunCode_List->pRegister_List;
					while(((pCurrent_Register_List->u16RegAddr) < u16RegAddr)
						&&(NULL != pCurrent_Register_List->next))
					{
						pPrevious_Register_List = pCurrent_Register_List;
						pCurrent_Register_List = pCurrent_Register_List->next;
					}

					if(pCurrent_Register_List == pCurrent_FunCode_List->pRegister_List)
					{
						if((pCurrent_Register_List->u16RegAddr) < u16RegAddr)
						{
							pRegister_List_Add->next = pCurrent_FunCode_List->pRegister_List->next;
							pCurrent_FunCode_List->pRegister_List->next = pRegister_List_Add;
							pCurrent_FunCode_List->u16TotalRegNum++;
						}
						else if((pCurrent_Register_List->u16RegAddr) > u16RegAddr)
						{
							pRegister_List_Add->next = pCurrent_FunCode_List->pRegister_List;
							pCurrent_FunCode_List->pRegister_List = pRegister_List_Add;
							pCurrent_FunCode_List->u16TotalRegNum++;
						}
						else
						{
							free(reg_name1);
							reg_name1 = NULL;
							free(reg_des1);
							reg_des1 = NULL;
							free(pRegister_List_Add);
							pRegister_List_Add = NULL;
						}
					}
					else
					{
						if((pCurrent_Register_List->u16RegAddr) < u16RegAddr)
						{
							pRegister_List_Add->next = pCurrent_Register_List->next;
							pCurrent_Register_List->next = pRegister_List_Add;
							pCurrent_FunCode_List->u16TotalRegNum++;
						}
						else if((pCurrent_Register_List->u16RegAddr) > u16RegAddr)
						{
							pPrevious_Register_List->next = pRegister_List_Add;
							pRegister_List_Add->next = pCurrent_Register_List;
							pCurrent_FunCode_List->u16TotalRegNum++;
						}
						else
						{
							free(reg_name1);
							reg_name1 = NULL;
							free(reg_des1);
							reg_des1 = NULL;
							free(pRegister_List_Add);
							pRegister_List_Add = NULL;
						}
					}
				}
			}
			break;
		case 2:
			if((NULL == pSearch_Slave_List->pFunCode_List)
				||(u8Funcode != pCurrent_FunCode_List->u8MBFunCode))
			{
				free(reg_des);
				reg_des = NULL;
				free(reg_name);
				reg_name = NULL;
				free(dataformat);
				dataformat = NULL;
				return ENN_FAIL;
			}
			else
			{
				pCurrent_Register_List = pCurrent_FunCode_List->pRegister_List;
				if(NULL == pCurrent_Register_List)
				{
					free(reg_des1);
					reg_des1 = NULL;
					free(reg_name1);
					reg_name1 = NULL;
					return ENN_FAIL;
				}
				while((u16RegAddr != (pCurrent_Register_List->u16RegAddr))
					&&(NULL != pCurrent_Register_List->next))
				{
					pPrevious_Register_List = pCurrent_Register_List;
					pCurrent_Register_List = pCurrent_Register_List->next;
				}

				if(u16RegAddr != (pCurrent_Register_List->u16RegAddr))
				{
					free(reg_des1);
					reg_des1 = NULL;
					free(reg_name1);
					reg_name1 = NULL;
					return ENN_FAIL;
				}
				else
				{
					if(pCurrent_Register_List == pCurrent_FunCode_List->pRegister_List)
					{
						pRegister_List_Temp = pCurrent_FunCode_List->pRegister_List;
						pCurrent_FunCode_List->pRegister_List = pCurrent_FunCode_List->pRegister_List->next;
					}
					else
					{
						pRegister_List_Temp = pCurrent_Register_List;
						pPrevious_Register_List->next = pCurrent_Register_List->next;
					}

					pCurrent_FunCode_List->u16TotalRegNum--;

					free(pRegister_List_Temp->pName);
					pRegister_List_Temp->pName = NULL;
					free(pRegister_List_Temp->pAttr);
					pRegister_List_Temp->pAttr = NULL;
					free(pRegister_List_Temp);
					pRegister_List_Temp = NULL;
				}
			}
			break;
		case 3:
			if((NULL == pSearch_Slave_List->pFunCode_List)
				||(u8Funcode != pCurrent_FunCode_List->u8MBFunCode))
			{
				free(reg_des1);
				reg_des1 = NULL;
				free(reg_name1);
				reg_name1 = NULL;
				return ENN_FAIL;
			}
			else
			{
				pCurrent_Register_List = pCurrent_FunCode_List->pRegister_List;
				if(NULL == pCurrent_Register_List)
				{
					free(reg_des1);
					reg_des1 = NULL;
					free(reg_name1);
					reg_name1 = NULL;
					return ENN_FAIL;
				}
				while((u16OldRegAddr != (pCurrent_Register_List->u16RegAddr))
					&&(NULL != pCurrent_Register_List->next))
				{
					pPrevious_Register_List = pCurrent_Register_List;
					pCurrent_Register_List = pCurrent_Register_List->next;
				}

				if(u16OldRegAddr != (pCurrent_Register_List->u16RegAddr))
				{
					free(reg_des1);
					reg_des1 = NULL;
					free(reg_name1);
					reg_name1 = NULL;
					return ENN_FAIL;
				}
				else
				{
					if(pCurrent_Register_List == pCurrent_FunCode_List->pRegister_List)
					{
						pRegister_List_Temp = pCurrent_FunCode_List->pRegister_List;
						pCurrent_FunCode_List->pRegister_List = pCurrent_FunCode_List->pRegister_List->next;
					}
					else
					{
						pRegister_List_Temp = pCurrent_Register_List;
						pPrevious_Register_List->next = pCurrent_Register_List->next;
					}

					pCurrent_FunCode_List->u16TotalRegNum--;

					free(pRegister_List_Temp->pName);
					pRegister_List_Temp->pName = NULL;
					free(pRegister_List_Temp->pAttr);
					pRegister_List_Temp->pAttr = NULL;
					free(pRegister_List_Temp);
					pRegister_List_Temp = NULL;

					pRegister_List_Add = (Register_List *)malloc(sizeof(Register_List));
					if(NULL == pRegister_List_Add)
					{
						free(reg_des1);
						reg_des1 = NULL;
						free(reg_name1);
						reg_name1 = NULL;
						return ENN_FAIL;
					}
					memset(pRegister_List_Add, 0, sizeof(Register_List));
					pRegister_List_Add->u16RegAddr = u16RegAddr;
					pRegister_List_Add->u16RegNum = u16RegNum;
					pRegister_List_Add->pName = reg_name1;
					pRegister_List_Add->pAttr = reg_des1;
					pRegister_List_Add->next = NULL;
					if(NULL == pCurrent_FunCode_List->pRegister_List)
					{
						pCurrent_FunCode_List->pRegister_List = pRegister_List_Add;
					}
					else
					{
						pCurrent_Register_List = pCurrent_FunCode_List->pRegister_List;
						while(((pCurrent_Register_List->u16RegAddr) < u16RegAddr)
							&&(NULL != pCurrent_Register_List->next))
						{
							pCurrent_Register_List = pCurrent_Register_List->next;
						}

						if(pCurrent_Register_List == pCurrent_FunCode_List->pRegister_List)
						{
							if((pCurrent_Register_List->u16RegAddr) < u16RegAddr)
							{
								pRegister_List_Add->next = pCurrent_FunCode_List->pRegister_List->next;
								pCurrent_FunCode_List->pRegister_List->next = pRegister_List_Add;
								pCurrent_FunCode_List->u16TotalRegNum++;
							}
							else if((pCurrent_Register_List->u16RegAddr) > u16RegAddr)
							{
								pRegister_List_Add->next = pCurrent_FunCode_List->pRegister_List;
								pCurrent_FunCode_List->pRegister_List = pRegister_List_Add;
								pCurrent_FunCode_List->u16TotalRegNum++;
							}
							else
							{
								free(reg_name1);
								reg_name1 = NULL;
								free(reg_des1);
								reg_des1 = NULL;
								free(pRegister_List_Add);
								pRegister_List_Add = NULL;
							}
							//pRegister_List_Add->next = pCurrent_Register_List;
							//pCurrent_Register_List = pRegister_List_Add;
						}
						else
						{
							if((pCurrent_Register_List->u16RegAddr) < u16RegAddr)
							{
								pRegister_List_Add->next = pCurrent_Register_List->next;
								pCurrent_Register_List->next = pRegister_List_Add;
								pCurrent_FunCode_List->u16TotalRegNum++;
							}
							else if((pCurrent_Register_List->u16RegAddr) > u16RegAddr)
							{
								pPrevious_Register_List->next = pRegister_List_Add;
								pRegister_List_Add->next = pCurrent_Register_List;
								pCurrent_FunCode_List->u16TotalRegNum++;
							}
							else
							{
								free(reg_name1);
								reg_name1 = NULL;
								free(reg_des1);
								reg_des1 = NULL;
								free(pRegister_List_Add);
								pRegister_List_Add = NULL;
							}
						}
					}
				}
			}
			break;
		default:
			break;
	}
	debug_int();

#endif
	return ENN_SUCCESS;
}

ENN_ErrorCode_t ENNModBus_Set_IP(ENN_U8 *pdata, ENN_U16 len)
{
	ENN_CHAR cValue[16];
	ENN_U8 *pTemp = NULL;
	ENN_U32 length = 0;
	ENN_U8 IPLen = 0;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

	if(NULL == gSlave_Set_List_Head)
	{
		return ENN_FAIL;
	}

	pTemp = pdata;
	length = len;

	IPLen = *pTemp;
	length--;
	pTemp++;

	memset(cValue, 0, 16);
	memcpy(cValue, pTemp, IPLen);
	length -= IPLen;
	pTemp += IPLen;
	printf("%s, %d, IPADDR = %s (%d)\n",__FUNCTION__,__LINE__, cValue, strlen(cValue));
	ENNSock_EthernetIPAddressSet(cValue);

	IPLen = *pTemp;
	length--;
	pTemp++;

	memset(cValue, 0, 16);
	memcpy(cValue, pTemp, IPLen);
	length -= IPLen;
	pTemp += IPLen;
	printf("%s, %d, Netmask = %s (%d)\n",__FUNCTION__,__LINE__, cValue, strlen(cValue));
	ENNSock_EthernetSubNetmaskSet(cValue);
	
	IPLen = *pTemp;
	length--;
	pTemp++;

	memset(cValue, 0, 16);
	memcpy(cValue, pTemp, IPLen);
	length -= IPLen;
	printf("%s, %d, Gateway = %s (%d)\n",__FUNCTION__,__LINE__, cValue, strlen(cValue));
	ENNSock_EthernetGatewaySet(cValue);

	return ENN_SUCCESS;
}
#endif

ENN_ErrorCode_t ENNDev645_Channel_Code_Get(ENN_U8 *pdata, ENN_U32 len)
{
	ENN_U8 *data = NULL;
	ENN_U8 *pBuf = NULL;
	ENN_U8 *pTemp = NULL;
	ENN_U8 *pTemp1 = NULL;
	ENN_U16 CMD_RSP = 0;
	ENN_U16 u16Len = 0;
	ENN_U32 length = 0;
	ENN_U8 		u8ChannelNum = 0;
	ENN_U8		channel = 0; 
	ENN_U8  	Addr_Len = 0;
	ENN_U8 		u8ChannelCodeCount = 0;
	ENN_CHAR slave_addr[16];
	Channel_List *pCurrent_Channel_List = NULL;
	Dev645_List *pCurrent_Dev645_List = NULL;
	Dev645_List *pSearch_Dev645_List = NULL;
	Dev645_List *pPrevious_Dev645_List = NULL;
	Code_645_List *pCurrentCode_645_List = NULL;
	float			fK = 10.0;
	ENN_U8 			u8D = 10;
	ENN_U32 i = 0;
	ENN_S32 s32ret = 0;
	ENN_S32 s32fd = 0;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

	pTemp = pdata;
	length = len;

	channel = *pTemp;
	length--;
	pTemp++;

	Addr_Len = *pTemp;
	length--;
	pTemp++;

	memset(slave_addr, 0, 16);
	memcpy(slave_addr, pTemp, Addr_Len);
		
	/*查询通道*/
	pCurrent_Channel_List = ENNModBus_Channel_Search(channel);
	if(NULL == pCurrent_Channel_List)
	{
		goto FIFOWRITE;
	}
	PIPE_PRINT("%s, %d, slave_addr = %s\n",__FUNCTION__,__LINE__, slave_addr);	
	
	//u8SlaveNum = pCurrent_Channel_List->u8SlaveNum;
	pSearch_Dev645_List = ENNDev_645_Search(slave_addr, 
										   pCurrent_Channel_List, 
										   &pCurrent_Dev645_List, 
										   &pPrevious_Dev645_List);
	if(NULL == pSearch_Dev645_List)
	{
		goto FIFOWRITE;
	}

	u8ChannelNum = pSearch_Dev645_List->u8ChannelNum;
	pCurrentCode_645_List = pSearch_Dev645_List->pCode_645_List;

FIFOWRITE:	
	data = (ENN_U8 *)malloc(1024*1024);
	if(NULL == data)
	{
		return ENN_FAIL;
	}
	memset(data, 0, 1024*1024);
	//memset(data, 0, 1024*5+512);
	//pBuf = &data;
	pBuf = data;
	pBuf += 2;
	
	CMD_RSP = ENN_CMD_DEV645_CODE_GET_RSP;
	memcpy(pBuf, &CMD_RSP, 2);
	u16Len += 2;
	pBuf += 2;

	*pBuf = channel;
	u16Len++;
	pBuf++;

	*pBuf = Addr_Len;
	u16Len++;
	pBuf++;
	
	memcpy(pBuf, &slave_addr, Addr_Len);
	u16Len += Addr_Len;
	pBuf += Addr_Len;

	pTemp1 = pBuf;
	u16Len++;
	pBuf++;

	PIPE_PRINT("%s, %d, pCurrentCode_645_List = %p\n",__FUNCTION__,__LINE__, pCurrentCode_645_List);	
	while(NULL != pCurrentCode_645_List)
	{
		u8ChannelCodeCount++;
		memcpy(pBuf, &(pCurrentCode_645_List->u32ChannelCode), 2);
		u16Len += 2;
		pBuf += 2;

		*pBuf = (ENN_U8)(pCurrentCode_645_List->eData_Type);
		u16Len++;
		pBuf++;

		memcpy(pBuf, &fK, 4);
		u16Len += 4;
		pBuf += 4;
		
		memcpy(pBuf, &u8D, 1);
		u16Len++;
		pBuf++;
		
		pCurrentCode_645_List = pCurrentCode_645_List->next;
	}

	*pTemp1 = u8ChannelCodeCount;	

	pBuf = data;
	memcpy(pBuf, &u16Len, 2);
	u16Len += 2;

	for(i=0; i<u16Len; i++)
	{
		if(i%10 == 0)
		{
			PIPE_PRINT("\n");
		}
		PIPE_PRINT("%2.2x ", pBuf[i]);
	}
	PIPE_PRINT("\n");

	s32ret = ENNFIFO_create(FIFIO_HTML_OUT_MSG);
	PIPE_PRINT("%s, %d, s32ret = %d\n",__FUNCTION__,__LINE__, s32ret);	
	if(s32ret < 0)
	{
        perror("create failed:\n");
		free(data);
		data = NULL;
		return ENN_FAIL;
	}

	//s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_WRONLY | O_NONBLOCK);
	//s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_WRONLY | O_NONBLOCK);
	s32fd = ENNFIFO_Open(FIFIO_HTML_OUT_MSG, O_RDWR | O_NONBLOCK);
	PIPE_PRINT("%s, %d, s32fd = %d\n",__FUNCTION__,__LINE__, s32fd);	
	if(s32fd < 0)
	{
        perror("open failed:\n");
		free(data);
		data = NULL;
		return ENN_FAIL;
	}

	s32ret = ENNFIFO_write(s32fd, pBuf, u16Len);
	PIPE_PRINT("%s, %d, s32ret = %d, u16Len = %d\n",__FUNCTION__,__LINE__, s32ret, u16Len);	
	if((s32ret < 0) || (s32ret != u16Len))
	{
        perror("write failed:\n");
		free(data);
		data = NULL;
		return ENN_FAIL;
	}
	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);	
	
	//close(s32fd);
	free(data);
	data = NULL;
	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);	

	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNDev645_Channel_Code_Set(ENN_U8 *pdata, ENN_U32 len)
{
	ENN_U8 	*pTemp = NULL;
	ENN_U32 length = 0;
	ENN_U8		channel = 0; 
	ENN_U32 CMD_RSP = 0;
	ENN_U8 i = 0;
	ENN_U8 j = 0;
	ENN_S32 s32ret = 0;
	ENN_S32 s32fd = 0;
	ENN_U8  	Addr_Len = 0;
	ENN_U32 		u32ChannelCode = 0;
	ENN_U8  	u8ChannelNum = 0;
	ENN_U8  	u8ChannelType = 0;
	ENN_CHAR slave_addr[16];
	float fK = 0.0;
	ENN_U8 iD = 0;
	Channel_List *pCurrent_Channel_List = NULL;
	Dev645_List *pCurrent_Dev645_List = NULL;
	Dev645_List *pSearch_Dev645_List = NULL;
	Dev645_List *pPrevious_Dev645_List = NULL;
	Dev645_List *pDev645_List_Add = NULL;
	Code_645_List *pCode_645_List_Temp = NULL;
	Code_645_List *pCode_645_List_Temp1 = NULL;
	Code_645_List *pCode_645_List_Add = NULL;
	Code_645_List *pLastCode_645_List = NULL;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

	if(NULL == gSlave_Set_List_Head)
	{
		return ENN_FAIL;
	}

	pTemp = pdata;
	length = len;

	channel = *pTemp;
	length--;
	pTemp++;

	Addr_Len = *pTemp;
	length--;
	pTemp++;

	memset(slave_addr, 0, 16);
	memcpy(slave_addr, pTemp, Addr_Len);
	length -= Addr_Len;
	pTemp += Addr_Len;

	u8ChannelNum = *pTemp;
	length--;
	pTemp++;
	PIPE_PRINT("%s, %d, u8ChannelNum = %d\n",__FUNCTION__,__LINE__, u8ChannelNum);
	
	/*查询通道*/
	pCurrent_Channel_List = ENNModBus_Channel_Search(channel);
	if(NULL == pCurrent_Channel_List)
	{
		return ENN_FAIL;
	}
	
	pSearch_Dev645_List = ENNDev_645_Search(slave_addr, 
										   pCurrent_Channel_List, 
										   &pCurrent_Dev645_List, 
										   &pPrevious_Dev645_List);
	PIPE_PRINT("%s, %d, pSearch_Dev645_List = %p\n",__FUNCTION__,__LINE__, pSearch_Dev645_List);
	if(NULL == pSearch_Dev645_List)
	{
		/*pDev645_List_Add = (Dev645_List *)malloc(sizeof(Dev645_List));
		if(NULL == pDev645_List_Add)
		{
			return ENN_FAIL;
		}
		memset(pDev645_List_Add, 0, sizeof(Dev645_List));
		memcpy(pDev645_List_Add->ucDevAddr, slave_addr, Addr_Len);
		pDev645_List_Add->
		
		if(NULL == pCurrent_Channel_List->unDeviceType.pDev645_List)
		{
		
		}
		else
		{
			if(NULL == pCurrent_Dev645_List)
			{
				return ENN_FAIL;
			}
			
		}*/
		return ENN_FAIL;
	}
	else
	{
		pCode_645_List_Temp = pSearch_Dev645_List->pCode_645_List;
		while(NULL != pCode_645_List_Temp)
		{
			pCode_645_List_Temp1 = pCode_645_List_Temp;
			pCode_645_List_Temp = pCode_645_List_Temp->next;

			free(pCode_645_List_Temp1->pName);
			pCode_645_List_Temp1->pName = NULL;
			
			free(pCode_645_List_Temp1->pAttr);
			pCode_645_List_Temp1->pAttr= NULL;
			
			free(pCode_645_List_Temp1);
			pCode_645_List_Temp1 = NULL;
		}

		pSearch_Dev645_List->pCode_645_List = NULL;

		for(i = 0; i < u8ChannelNum; i++)
		{
			pCode_645_List_Add = (Code_645_List *)malloc(sizeof(Code_645_List));
			if(NULL == pCode_645_List_Add)
			{
				return ENN_FAIL;
			}
			memset(pCode_645_List_Add, 0, sizeof(Code_645_List));
			
			memcpy(&u32ChannelCode, pTemp, 2);
			length -= 2;
			pTemp += 2;
			
			u8ChannelType = *pTemp;
			length--;
			pTemp++;
			
			memcpy(&fK, pTemp, 4);
			length -= 4;
			pTemp += 4;

			iD = *pTemp;
			length--;
			pTemp++;

			pCode_645_List_Add->u32ChannelCode = u32ChannelCode;
			pCode_645_List_Add->eData_Type = (Data_Type)u8ChannelType;
			pCode_645_List_Add->u16RegNum = 4;
			pCode_645_List_Add->pName = NULL;
			pCode_645_List_Add->pAttr = NULL;
			pCode_645_List_Add->next = NULL;

			/*for(j = 0; j < ENN_645_MAX_CHANNEL_LEN; j++)
			{
				if(gDev645_Map[j].u16code == u32ChannelCode)
				{
					pCode_645_List_Add->pName = (ENN_CHAR *)malloc(strlen(gDev645_Map[j].strName) + 1);
					break;
				}
			}*/
			PIPE_PRINT("%s, %d, pCode_645_List = %p\n",__FUNCTION__,__LINE__, pSearch_Dev645_List->pCode_645_List);
			if(NULL == pSearch_Dev645_List->pCode_645_List)
			{
				pSearch_Dev645_List->pCode_645_List = pCode_645_List_Add;
			}
			else
			{
				pLastCode_645_List->next = pCode_645_List_Add;
			}
			pLastCode_645_List = pCode_645_List_Add;
		}
	}
	
	debug_int();

	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Parser_Data(const ENN_U8 *pdata, ENN_U32 len)
{
	ENN_U16 req_cmd = 0;
	ENN_U16 rsp_cmd = 0;
	ENN_U8 *pTemp = NULL;
	ENN_U32 length = 0;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	
	ENNAPI_ASSERT(NULL != pdata);
	ENNAPI_ASSERT(0 != len);

	pTemp = pdata;
	length = len;

	memcpy(&req_cmd, pTemp, 2);
	length -= 2;
	pTemp += 2;
	PIPE_PRINT("%s, %d, req_cmd = %d\n",__FUNCTION__,__LINE__, req_cmd);

	switch(req_cmd)
	{
		case ENN_CMD_SET_IP_REQ:
			returnCode = ENNModBus_Set_IP(pTemp, length);
			break;
		case ENN_CMD_CHANNEL_STATUS_REQ:
			returnCode = ENNModBus_Get_Channel_Status(pTemp, length);
			break;
		case ENN_CMD_GET_UART_CONF_REQ:
			returnCode = ENNModBus_UART_Get(pTemp, length);
			break;
		case ENN_CMD_SET_UART_CONF_REQ:
			returnCode = ENNModBus_UART_Set(pTemp, length);
			break;
		case ENN_CMD_SLAVE_GET_REQ:
			returnCode = ENNModBus_Slave_Get(pTemp, length);
			break;
		case ENN_CMD_SLAVE_SET_REQ:
			returnCode = ENNModBus_Slave_Set(pTemp, length);
			break;
		case ENN_CMD_SLAVE_REG_GET_REQ:
			PIPE_PRINT("%s, %d, length = %d\n",__FUNCTION__,__LINE__, length);
			returnCode = ENNModBus_Slave_Reg_Get(pTemp, length);
			break;
		case ENN_CMD_SLAVE_REG_SET_REQ:
			returnCode = ENNModBus_Slave_Reg_Set(pTemp, length);
			break;
		case ENN_CMD_SLAVE_SAVE_REQ:
			returnCode = ENNModBus_Slave_Save();
			break;
		case ENN_CMD_REBOOT_REQ:
			returnCode = ENNOS_SwReset();
			break;
		case ENN_CMD_DEV645_CODE_GET_REQ:
			ENNDev645_Channel_Code_Get(pTemp, length);
			break;
		case ENN_CMD_DEV645_CODE_SET_REQ:
			ENNDev645_Channel_Code_Set(pTemp, length);
			break;
		default:
			return ENN_FAIL;
	}
	
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_PIPE_Task(void)
{
	//int fd;
	ENN_S32 fd = 0;
	ENN_S32 n = 0;
	ENN_U32 dataLen = 0;
	ENN_U32 i = 0;
	ENN_U8 *pbuf = NULL;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	int ret;
	char cmd[30];
	ENN_CHAR tmp[5];

	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);

	ret = ENNFIFO_create(FIFIO_HTML_MSG);
	if(ret < 0)
	{
		perror("create failed:\n");
		return ENN_FAIL;
	}
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "chmod 777 %s", FIFIO_HTML_MSG);
	system(cmd);

	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
	if((fd = ENNFIFO_Open(FIFIO_HTML_MSG, O_RDONLY)) < 0)
	{
		//ENNOS_DelayTask(100);
		PIPE_PRINT("WARNNING :%s, %d\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	while(1)
	{
		//n = ENNFIFO_read(fd, (ENN_CHAR *)&dataLen, 2);
		n = ENNFIFO_read(fd, tmp, 2);
		dataLen = tmp[0];	//cmd Length

		if(n > 0)
		{
			PIPE_PRINT("%s, %d, n = %d, dataLen = %d\n",__FUNCTION__,__LINE__, n, dataLen);

			pbuf = (ENN_U8 *)malloc(dataLen);
			if(NULL == pbuf)
			{
				ENNOS_DelayTask(100);
				continue;
			}
			memset(pbuf, 0, dataLen);
			n = ENNFIFO_read(fd, (ENN_CHAR *)pbuf, dataLen);
			if((n > 0) && (dataLen == n))
			{
				for(i=0; i<dataLen; i++)
					PIPE_PRINT("%x ", pbuf[i]);
				PIPE_PRINT("\n");

				returnCode = ENNModBus_Parser_Data(pbuf, dataLen);
			}
			free(pbuf);
			pbuf = NULL;
		}

	//ENNOS_DelayTask(100);
	}
	close(fd);

	return ENN_SUCCESS;
}



ENN_ErrorCode_t ENNModBus_Create_PIPE_Task(void)
{
	ENNOS_TASK_t taskID;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	ENN_U8 i = 0;
	Channel_List *pChannel_List_Add = NULL;
	Channel_List *pLast_Channel_List = NULL;
	FILE	*fpConfig = NULL;

/*for test
{
		int ret;
		char aregname[100];
		char regname[10];

		memset(aregname, 0 ,sizeof(aregname));
		regname[0] = 0xE4;
		regname[1] = 0xBD;
		regname[2] = 0xA0;
		regname[3] = 0xE5;
		regname[4] = 0xA5;
		regname[5] = 0xBD;
		regname[6] = 0;
		ret = UTF8_To_GB2312(regname,strlen(regname), aregname,100);
		printf("[%s],%d,ret =%d, aregname =%s\n",__FUNCTION__,__LINE__,ret, aregname);

	}
*/
	fpConfig = fopen("/home/modbus.ini","r");
	if((NULL == fpConfig) || (NULL == gChannel_List_Head))
	{
		memset(channel_memory, 0, sizeof(channel_memory));
		pchannel_memory = channel_memory;
		
		for(i=0; i<UART_CHANNEL_MAX_NUM; i++)
		{
			pChannel_List_Add = (Channel_List *)(pchannel_memory + (i * (sizeof(Channel_List))));
			pChannel_List_Add->u8Channel = i;
			pChannel_List_Add->u8Protocol = 0;
			pChannel_List_Add->u8SlaveNum = 0;
			pChannel_List_Add->u32BaudRate = 9600;
			pChannel_List_Add->u8DataBit = 8;
			pChannel_List_Add->u8StoptBit = 1;
			pChannel_List_Add->u8Parity = 0;
			pChannel_List_Add->unDeviceType.pModBus_List = NULL;
			pChannel_List_Add->next = NULL;

			if(NULL == pLast_Channel_List)
			{
				gSlave_Set_List_Head = pChannel_List_Add;
				pLast_Channel_List = gSlave_Set_List_Head;
			}
			else
			{
				pLast_Channel_List->next = pChannel_List_Add;
				pLast_Channel_List = pChannel_List_Add;
			}
		}

		/******Debug******/
		pLast_Channel_List = gSlave_Set_List_Head;
		for(i=0; i<UART_CHANNEL_MAX_NUM; i++)
		{
			PIPE_PRINT("%s, %d, Channel = %d\n",__FUNCTION__,__LINE__, pLast_Channel_List->u8Channel);
			pLast_Channel_List = pLast_Channel_List->next;
		}
		
	}
	else
	{
		if(NULL != fpConfig)
		{
			close(fpConfig);
			fpConfig = NULL;
		}

		/*returnCode = ENNModBus_Channel_Init();
		if(ENN_SUCCESS != returnCode)
		{
			PIPE_PRINT("\nENNModBus_Channel_Init fail!\n");
			return returnCode;
		}*/
	}
	//debug_int();

	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
	returnCode = ENNOS_CreateTask("PIPE_TASK", ENNOS_TASK_PRIORITY_MIDDLE, 4*1024, &taskID, ENNOS_TASK_START, (void*)ENNModBus_PIPE_Task, NULL);
	if(ENN_SUCCESS != returnCode)
	{
		PIPE_PRINT("\nCreate PIPE task fail!\n");
		return returnCode;
	}
	
	PIPE_PRINT("%s, %d\n",__FUNCTION__,__LINE__);
	return ENN_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
    }
#endif /* __cpluscplus */
#endif /* __cpluscplus */


