/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：ModBus_TCP.c
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/	
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/tcp.h>
#include <errno.h>
#include <termios.h>
#include <string.h>


#include "ennAPI.h"
#include "ennSocket.h"
#include "ModBus_TCP.h"
#include "ModBus_Slave_Table.h"
#include "IEC102_Main.h"
#include "IEC102protocol.h"



#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */


#define MB_TCP_TID          0
#define MB_TCP_PID          2
#define MB_TCP_LEN          4
#define MB_TCP_UID          6
#define MB_TCP_FUNC         7

#define MB_TCP_PROTOCOL_ID  0   /* 0 = Modbus Protocol */

#define MB_TCP_ADU_EVENT_LEN 	2
#define MB_TCP_ADU_PROTOCOL_LEN 2
#define MB_TCP_ADU_DATALEN_LEN 	2
#define MB_TCP_ADU_UNIT_LEN 	1



#define MB_TCP_ADU_HEAD_LEN     7
#define MB_TCP_ADU_SIZE_MAX     (MB_TCP_ADU_HEAD_LEN + 249)

ENN_S32 fd[MAXCLINE]; //连接的fd

ENN_S32 conn_amount; //当前的连接数

//TCP_DATA_LIST *gTCP_DATA_LIST_RECV_HEAD = NULL;
//TCP_DATA_LIST *gTCP_DATA_LIST_SEND_HEAD = NULL;

//ModbusMsg *gModbusMsg_Head = NULL;

ENN_U16 gSTAR_ADDR = 0;
static ENNOS_MSG_t gModBusTaskQueueID = 0;
/*************add by hyptek***********/
extern DPA_Mode_t CurrentDPAMode ;
extern DPA_SLAVE_PARAM *slave_table_Head;
extern DEVICE_IP_PARAM *	pDEVICE_IP_PARAM;

int fd_index_to_master[MAXCLINE];
UINT8 currentIndex;
DPA_CONN_CONTEXT *Pcontext[MAXCLINE];
#define	INFOOBJ_MAX_NUM 	128


/*************add by hyptek***********/

/*************add by wanglongchang***********/
extern ENN_CHAR    Sendbuf[128];
extern FunCode_List *gFunCode_List_head;


/*************add by wanglongchang***********/



#undef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))


//0X01,0X02,0X03,0X05,0X10	
/*static xMBFunctionHandler xFuncHandlers[] = 
{
    {MB_FUNC_READ_COILS, 				eMBFuncReadCoils},
    {MB_FUNC_READ_DISCRETE_INPUTS, 		eMBFuncReadDiscreteInputs},
	{MB_FUNC_READ_HOLDING_REGISTER, 	eMBFuncReadHoldingRegister},
	{MB_FUNC_WRITE_SINGLE_COIL, 		eMBFuncWriteCoil},
	{MB_FUNC_WRITE_MULTIPLE_REGISTERS, 	eMBFuncWriteMultipleHoldingRegister},
};

#define NumFuncHandler    (sizeof(xFuncHandlers)/sizeof(xMBFunctionHandler))

static xMBFunctionHandler* GetFuncHandler(ModbusMsg *mbMsg)
{
	ENN_U8 mbFunctionCode;
	ENN_U32 i;
	xMBFunctionHandler *functionHandler = NULL;

	mbFunctionCode = mbMsg->mbFunctionCode;
	
	for(i=0;i<NumFuncHandler;i++)
	{
		if(xFuncHandlers[i].ucFunctionCode == mbFunctionCode)
		{
			functionHandler = &xFuncHandlers[i];
		}
	}

	return functionHandler;
}*/

ENN_VOID TCP_DEBUG(ENN_CHAR *fmt, ...)
{
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	vfprintf(stdout, fmt, arg_ptr);
	va_end(arg_ptr);
}


void showclient()
{
	int i;
	ENNTRACE("client amount:%d\n", conn_amount);
	for(i=0; i<MAXCLINE; i++)
	{
		ENNTRACE("[%d]:%d ",i,fd[i]);
	}
	ENNTRACE("\n\n");
}

const ENN_U16 wCRCTalbeAbs[] =
{
0x0000, 0xCC01, 0xD801, 0x1400, 
0xF001, 0x3C00, 0x2800, 0xE401, 
0xA001, 0x6C00, 0x7800, 0xB401, 
0x5000, 0x9C01, 0x8801, 0x4400, 
};


/*  低位字节的 CRC  值  */
static const ENN_U8 auchCRCHi[] = {
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* Table of CRC values for low-order byte */
static const ENN_U8 auchCRCLo[] =
{
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

/*  函数以  unsignedshort  类型返回  CRC  */
ENN_U16 CRC16_Ex(ENN_U8 *pData, ENN_U16 DataLen)//(puchMsg, usDataLen)
{
	ENN_U8 uchCRCHi = 0xFF;  	/*CRC  的高字节初始化 */
	ENN_U8 uchCRCLo = 0xFF;  	/*CRC  的低字节初始化 */
	ENN_U32 uIndex = 0;  		/*CRC  查询表索引*/
	ENN_U8 *pTemp = NULL;

	ENNAPI_ASSERT((NULL != pData) && (0 != DataLen));
	
	pTemp = pData;
	while (DataLen--)  					/*  完成整个报文缓冲区*/
	{
		uIndex = uchCRCLo^ *pTemp++ ;  	/*  计算  CRC */
		uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex];
		uchCRCHi = auchCRCLo[uIndex];
	}

	return (uchCRCHi << 8 |uchCRCLo);
} 


ENN_U16 CRC16(ENN_U8 *pData, ENN_U16 DataLen)
{
    ENN_U16 wCRC = 0xFFFF;
    ENN_U16 i;
    ENN_U8 u8Data;

    for(i = 0; i < DataLen; i++)
    {
        u8Data = *pData++;
        wCRC = wCRCTalbeAbs[(u8Data ^ wCRC) & 15] ^ (wCRC >> 4);
        wCRC = wCRCTalbeAbs[((u8Data >> 4) ^ wCRC) & 15] ^ (wCRC >> 4);
    }

    return wCRC;
}


ENN_U8 aMBTCP_HEAD[7];
ENN_BOOL bflag = ENN_TRUE;
ENN_BOOL bReadflag = ENN_FALSE;

ENN_S32	g32Socket = 0;
ENN_ErrorCode_t ENNModBus_TCP_Send(ENN_S32 socket, ENN_U8 *pbuf, ENN_S32 Len)
{
	ENN_U8 *pSendBuf = NULL;
	ENN_U16 u16Len = 0;
	ENN_U16 u16TCPLen = 0;
	int num;
	int  i = 0;

	ENNAPI_ASSERT((NULL != pbuf) && (0 !=  Len));

	/*u16Len = Len - 2 + 7;
	u16TCPLen = Len - 2 + 1;
	ENNTRACE("Len = %d\n", Len);
	ENNTRACE("u16Len = %d\n", u16Len);
	ENNTRACE("u16TCPLen = %d\n", u16TCPLen);
	pSendBuf = (ENN_U8 *)malloc(u16Len);
	if(NULL == pSendBuf)
	{
		bflag = ENN_TRUE;
		return ENN_FAIL;
	}
	memset(pSendBuf, 0, u16Len);
	memcpy(pSendBuf, aMBTCP_HEAD, 4);
	pSendBuf[4] = (ENN_U8)(u16TCPLen >> 8);
	pSendBuf[5] = (ENN_U8)(u16TCPLen & 0x00FF);
	memcpy(pSendBuf+6, pbuf, Len - 2);

	for(i = 0; i < u16Len; i++)
		ENNTRACE("%2.2x ", pSendBuf[i]);
	ENNTRACE("\n");
	num = send(g32Socket, pSendBuf, u16Len, 0);
	ENNTRACE("num = %d\n", num);
	bflag = ENN_TRUE;*/
	num = send(socket, pbuf, Len, 0);
	ENNTRACE("num = %d\n", num);
	if(num <= 0)
	{
		perror("send:");
		return ENN_FAIL;
	}

	return ENN_SUCCESS;
}

#define READ_DATA_LEN 256
#if 0
void Test_Rx()
{
	ENN_U8 tmp[READ_DATA_LEN];
	int i = 0;
	ENN_S32 len;
	ENN_U16 u16CRC;

	ENNTRACE("\n%s, %d\n",__FUNCTION__,__LINE__);
	while(1)
	{
		ENNTRACE("\n%s, %d\n",__FUNCTION__,__LINE__);
		if(bReadflag)
		{
			ENNTRACE("\n%s, %d\n",__FUNCTION__,__LINE__);
			memset(tmp, 0, READ_DATA_LEN);
			len = UART_Read(UART4, tmp, READ_DATA_LEN);
			if(len > 0)
			{
			ENNTRACE("len = %d\n",len);
			ENNTRACE("\n%s, %d\n",__FUNCTION__,__LINE__);
			for(i = 0; i < len; i++)
				ENNTRACE(" %2.2x", tmp[i]);
			ENNTRACE("\n");

			ENNModBus_TCP_Send(tmp, len);
			bReadflag = ENN_FALSE;
			//ENNOS_DelayTask(100);
			}
			else
			{
				ENNTRACE("\n%s, %d\n",__FUNCTION__,__LINE__);
			}
		}
		else
		{
			ENNTRACE("\n%s, %d\n",__FUNCTION__,__LINE__);
			ENNOS_DelayTask(100);
		}
	}
}
#else
extern ENN_S32 gChannel3_Handle;
void Test_Rx()
{
	ENN_U8 tmp[READ_DATA_LEN];
	int i = 0;
	ENN_S32 len;
	ENN_U16 u16CRC;
	fd_set fs_read;
	struct timeval tv_timeout;
	ENN_S32 iRet;

	ENNTRACE("\n%s, %d\n",__FUNCTION__,__LINE__);
	while(1)
	{
		ENNTRACE("\n%s, %d\n",__FUNCTION__,__LINE__);
		FD_ZERO(&fs_read);
        FD_SET (gChannel3_Handle, &fs_read);

        tv_timeout.tv_sec = 0;
        tv_timeout.tv_usec = 1000000;
        iRet = ENNSock_Select(gChannel3_Handle + 1, (ENNSock_fd_set *)&fs_read, NULL, NULL, (ENNSock_Timeval *)&tv_timeout);
		if(iRet < 0)
		{
			ENNTRACE("	[%s %d select error!!]\r\n",__FUNCTION__, __LINE__);
			break;
		}
		else if(0 == iRet)	/* 指定的时间到*/
		{
			ENNTRACE("[%s %d timeout!]\r\n",__FUNCTION__, __LINE__);
			continue;
		}
		else
		{
			ENNTRACE("waiting for read data from UART............\n");
			if(FD_ISSET(gChannel3_Handle, &fs_read))
			{
				len = UART_Read(CHANNEL3, tmp, READ_DATA_LEN);
				ENNTRACE("len = %d\n",len);
				ENNTRACE("the read data is:\n");
				for(i = 0; i < len; i++)
					ENNTRACE("%2.2x ", tmp[i]);
				ENNTRACE("\n");

				//ENNModBus_TCP_Send(tmp, len);
				bReadflag = ENN_FALSE;
			}
			else
			{
				ENNTRACE("\n%s, %d\n",__FUNCTION__,__LINE__);
			}
		}
	}
}
#endif

UINT8 byte_checksum(void *buff, UINT16 count)
{
	UINT8 cc;

	cc=0;
	while(count--)
	{
		cc += *((UINT8 *)buff);
		buff = ((UINT8 *)buff) + 1;
	}
	return (cc);
}


ENN_U8 FT12_ll_con_ind(DPA_CONN_CONTEXT *context, void **udta)
{	
	UINT8            lcf;        /* Link Control Function                  */
	UINT8            ldo;        /* Link Data ofset                        */
	UINT8            stat;       /* Station                                */
	UINT8            ret = FT12_LL_ERROR;        /* Signal to upper layer  */
	U_WORD              laddr;      /* Link Address  of secondary station.*/
	UINT8            *pbuf;
	UINT8            *pdata;
	T_IEC102_SEQUENCE   *seq;

	if (NULL == context)
	{
	    TCP_DEBUG( "NULL pointer!,%s,%d\n",__FUNCTION__,__LINE__);
	    return FT12_LL_ERROR_FRAME;
	}

	seq = &context->seq;
	pbuf  = context->rxframe.databuf;
	*udta = NULL;

	if (pbuf[0] == FT12_FLF_START)
	{
		ldo = FT12_VLF_ASDU_START_POS;
	}
	else if (pbuf[0] == FT12_VLF_START)
	{
		ldo = FT12_FLF_ASDU_START_POS;
	}
	else
	{
		return FT12_LL_ERROR_FRAME;
	}

	/* Find out this message sent by primary or secondary station? */
	stat = (pbuf[ldo] & DLC_PRM) ? FT12_LL_STATION_PRI : FT12_LL_STATION_SEC;

	/* Check Link address. The link address 0 is general address. It has byte order LOW u_int8_t, HIGH u_int8_t
	   In any case adress of secondary station has to be stored in
	   channels[ch].seq.di.attr.da.laddr .                                    */
	laddr.wb.lb = pbuf[ldo+1];
	laddr.wb.hb = pbuf[ldo+2];

	if (laddr.w != seq->attr.iec870.laddr)
	{
		TCP_DEBUG("ERROR : Incorrect link address! configed link address=%d, asked link address=%d\n",
	    		seq->attr.iec870.laddr, laddr.w);
		return FT12_LL_ERROR_ADDRESS;
	}

	/* Get link function code from the control field */
	lcf = pbuf[ldo] & DLC_FUNC;

	switch (stat)
	{
	    	case FT12_LL_STATION_SEC:{   /* Message is sent by secondary station. */
			switch(seq->attr.iec870.ldid)
			{
				/* SEND/COFIRM functions. CONFIRM is expected. */
				case DLCF_PSC_RESLINK:
				case DLCF_PSC_RESPROC:
				case DLCF_PSC_DATA:
				switch(lcf)
				{
				case DLCF_SC_ACK:
				    ret = FT12_LL_CONFIRM_ACK;
				    break;

				case DLCF_SC_NACK:
				    ret = FT12_LL_CONFIRM_NAK;
				    break;

				default:
				    ret = FT12_LL_ERROR;
				    break;
				}
				break;

				/* SEND/NO_REPLAY. Nothing is expected. */
				case DLCF_PSN_DATA:
				ret = FT12_LL_ERROR;
				break;

				/* REQUEST/RESPOND functions. RESPOND is expected. */
				case DLCF_PRR_ACCESS:
				case DLCF_PRR_LINKSTA:
				switch(lcf)
				{
				case DLCF_SR_STAT:
				    *udta = &pbuf[ldo + 3];
				    ret = FT12_LL_CONFIRM_ACK;
				    break;

				case DLCF_SR_NACK:
				    ret = FT12_LL_CONFIRM_NAK;
				    break;

				default:
				    ret = FT12_LL_ERROR;
				    break;
				}
				break;

				case DLCF_PRR_DATA1:
				case DLCF_PRR_DATA2:
				switch(lcf)
				{
				case DLCF_SR_DATA:
				    *udta = &pbuf[ldo + 3];

				    /* Test for Access Demand for Class 1 Data */
				    if (pbuf[ldo] & DLC_FCB_ACD)
				    {
				        ret = FT12_LL_INDICATE;
				    }
				    else
				    {
				        ret = FT12_LL_CONFIRM_ACK;
				    }
				    break;

				case DLCF_SR_NACK:
				    ret = FT12_LL_CONFIRM_NAK;
				    break;

				default:
				    ret = FT12_LL_ERROR;
				    break;
				}
				break;
				default:
				break;
			}
			break;                              /* End of Primary Station*/
		}
		case FT12_LL_STATION_PRI:{
			//add for huazhong
			if (pbuf[ldo] & DLC_FCV_DFC)
			{
				if ((lcf != DLCF_PSC_DATA) && (lcf != DLCF_PRR_DATA1) && (lcf != DLCF_PRR_DATA2))
				{
					ret = FT12_LL_ERROR;
					break;
				}
			}
			else
			{
				if ((lcf != DLCF_PSC_RESLINK) && (lcf != DLCF_PRR_LINKSTA))
				{
					ret = FT12_LL_ERROR;
					break;
				}
			}//add end

			if ((pbuf[ldo] & DLC_FCV_DFC) &&
			    ((pbuf[ldo] & DLC_FCB_ACD) == (seq->lctrl & DLC_FCB_ACD)))
			{
			 	//if(lcf != DLCF_PSC_DATA)//对于请求帧控制域不是传送数据请求帧时需判断FCB位是否反转
			 	//{
					ret = FT12_LL_ERROR;
					TCP_DEBUG("[%s],%d,Error : pbuf[%d]=%d, seq->lctrl=%d\n", 
						__FUNCTION__,__LINE__,ldo, pbuf[ldo], seq->lctrl);

			    	if (0 == context->lasttxframe.datalen)
			    	{
					TCP_DEBUG("pbuf[%d]=%d, seq->lctrl=%d, datalen=0\n", ldo, pbuf[ldo], seq->lctrl);
			        	break;
			    	}

			    	pdata = context->lasttxframe.databuf;
			    	memcpy(context->txframe.databuf,
			           		pdata, context->lasttxframe.datalen);

			    	context->txframe.datalen = context->lasttxframe.datalen;
				if(lcf != DLCF_PSC_DATA )
				{
					break;
				}
				
				if(context->seq.attr.iec870_102attr.iec870seq.cseq == 1) 
				{
					break;
				}
				//}
			}

			if (pbuf[ldo] & DLC_FCV_DFC) /* If Frame Counter Valid */
			{
			    seq->lctrl = pbuf[ldo];
			}

			if ((pbuf[ldo] & DLC_FUNC) == DLCF_PSC_RESLINK)
			{
			    seq->lctrl = 0;
			}

			*udta  = &pbuf[ldo];
			ret    = FT12_LL_INDICATE;
			break;                       /* End of Secondary Station*/
		}
		default:
	        	break;
    }

    /*_FCB ^= DLC_FCB_ACD;*/
    return ret;
}


ENN_S32 iec102_search_startchar(UINT16 rxlen, UINT8 *pbuf, UINT16 *pdatalen, UINT16 *pdataflags)
{
	int i = 0;
	UINT16 tmplen = 0;

	if (5 == rxlen)
	{
		pbuf[0] = pbuf[3];
		pbuf[1] = pbuf[4];
		*pdatalen = 2;
		return 0;
	}
	else if (6 == rxlen)
	{
		pbuf[0] = pbuf[3];
		pbuf[1] = pbuf[4];
		pbuf[2] = pbuf[5];
		*pdatalen = 3;
		return 0;
	}
	else if (rxlen > 6)
	{
		for (i = 1; i < rxlen; i++)
		{
			if (FT12_VLF_START == pbuf[i] && pbuf[i+1] == pbuf[i+2] &&
				pbuf[i] == pbuf[i+3])
			{
				/*do not need to check the checksum*/
				if (rxlen <= pbuf[i+1] + i + 4)
				{
					memcpy(&pbuf[0], &pbuf[i], rxlen-i);
					*pdatalen = rxlen - i;
					return 0;
				}
				/*need to check the checksum*/
				else if (rxlen >= pbuf[i+1] + i + 5)
				{
					tmplen = pbuf[i+1] + i + 4;

					/*checksum error*/
					if (pbuf[tmplen] != byte_checksum(&pbuf[i+4], pbuf[i+1]))
					{
						continue;
					}
					/* Check frame end character */
					else if (rxlen > pbuf[i+1] + i + 5)
					{
						if (pbuf[tmplen+1] != FT12_VLF_END)
						{
							continue;
						}
						else
						{
							memcpy(&pbuf[0], &pbuf[i], rxlen-i);
							*pdatalen = rxlen - i;
							*pdataflags |= FBF_EOR;
							return 0;
						}
					}
					else
					{
						memcpy(&pbuf[0], &pbuf[i], rxlen-i);
						*pdatalen = rxlen - i;
						return 0;
					}
				}
			}
		}
	}

	return -1;
}

void iec102_rxframe_discard(DPA_CONN_CONTEXT *context)
{
    if (NULL == context)
    {
        TCP_DEBUG( "NULL pointer!,%s,%d\n",__FUNCTION__,__LINE__);
        return;
    }

    memset(&context->rxframe, 0, sizeof(context->rxframe));
}


ENN_S32 iec102_rx_available(T_IEC102_FRAME_BUF *pRX)
{
    if ((pRX->dataflags & FBF_EOR))
    {
        return TRUE;
    }
    return FALSE;
}


ENN_S32 dpa_port_select( ENN_S32 sockfd, ENN_S32 timeout_ms )
{
    struct  timeval recv_timeout;
	fd_set	readset;
	int		fdval;
	int		select_result;

	FD_ZERO(&readset);
	FD_SET(sockfd, &readset);
	fdval = sockfd+1;
	recv_timeout.tv_sec = timeout_ms / 1000;
	recv_timeout.tv_usec = (timeout_ms % 1000) * 1000;
	
	select_result = select(fdval, &readset, NULL, NULL, &recv_timeout);
	return select_result;
}

int iec102_rx_data(T_IEC102_FRAME_BUF *pRX, UINT8 rcvdata)
{
	UINT8 *pbuf;
	UINT8	i;
	UINT16 rxlen;
	UINT16	*pdatalen = NULL;
	UINT16	databufsize = sizeof(pRX->databuf);
	UINT16	*pdataflags = NULL;

	if (NULL == pRX)
	{
		return 1;
	}

	 /* For Single Frame Rx Process return if Frame Available
     * Frame is Completely stored in first Frame Buffer
     */
	if (iec102_rx_available(pRX))
	{
	    return 1;
	}

	pbuf = pRX->databuf;
	rxlen = pRX->datalen;
	pdatalen = &pRX->datalen;
	pdataflags = &pRX->dataflags;

	if (0 == rxlen)
	{/*the frame buffer is empty*/
	    if (rcvdata == FT12_VLF_START ||
	        rcvdata == FT12_FLF_START)
	    {
	        pbuf[rxlen++] = rcvdata;
	    }
	    else if (rcvdata==FT12_SCC_1 ||
	             rcvdata==FT12_SCC_2)
	    {
	        /* Set frame accepted status */
	        pbuf[rxlen++] = rcvdata;
	        pRX->dataflags |= FBF_EOR;
	    }
	    *pdatalen = rxlen;
	    return 0;
	}
	else if (rxlen > FT12_FRAMESIZE)
	{
	    *pdatalen = 0;
	    memset(pbuf, 0, databufsize);
	    return 1;
	}

	pbuf[rxlen++] = rcvdata;
	//ERR( "in iec102_rx_data rxdata=%x -------\n", rcvdata);
	/* Fixed length frame */
	if (pbuf[0] == FT12_FLF_START)
	{
	    /* Check checksum */
	    if (rxlen == FT12_FLF_SIZE+2)
	    {
	        if (pbuf[rxlen-1] != byte_checksum(&pbuf[1],FT12_FLF_SIZE))
	        {
	        	TCP_DEBUG("ERROR : checksum wrong , %s,%d\n",__FUNCTION__,__LINE__);
			//if checksum wrong ,check if there is start char after this wrong start char
			for (i = 1; i < rxlen; i++)
			{
			    if (pbuf[i] == FT12_FLF_START)
			    {
			        memcpy(&pbuf[0], &pbuf[i], rxlen-i);
			        *pdatalen = rxlen - i;
			        return 0;
			    }
			}
			*pdatalen = 0;
			memset(pbuf, 0, databufsize);
			return(1);
		}
	    }
	    /* Check frame end character */
	    else if (rxlen == FT12_FLF_SIZE+3)
	    {
		if (pbuf[rxlen-1] != FT12_FLF_END)
		{
		    for (i=1; i<rxlen; i++)
		    {
		        if (pbuf[i] == FT12_FLF_START)
		        {
		            memcpy(&pbuf[0], &pbuf[i], rxlen-i);
		            *pdatalen = rxlen-i;
		            return(0);
		        }
		    }

			TCP_DEBUG("ERROR : frame end character wrong , %s,%d\n",__FUNCTION__,__LINE__);
		    *pdatalen = 0;
		    memset(pbuf, 0, databufsize);
		    return 1;
	        }

	        /* Set frame accepted status */
	        pRX->dataflags |= FBF_EOR;
	    }
	    else if (rxlen > FT12_FLF_SIZE+3)
	    {
	    	TCP_DEBUG("ERROR : rxlen > FT12_FLF_SIZE+3 , %s,%d\n",__FUNCTION__,__LINE__);
		*pdatalen = 0;
		memset(pbuf, 0, databufsize);
		return 1;
	    }
	}

	/* Variable length frame */
	else if (pbuf[0] == FT12_VLF_START)
	{
		/* Check identity of both length specifications L */
		if (rxlen == 3)
		{
		    //INFO("rxlen=%d\n",rxlen);
		    if (pbuf[1] != pbuf[2])
		    {
		        TCP_DEBUG("WARNING : rxlen=3, pbuf[1] != pbuf[2], pbuf[1]=%d, pbuf[2]=%d\n",pbuf[1],pbuf[2]);
		        /*just like 68 68 09 09 68 53 B1 7F 67 00 05 B1 7F 00 1F 16, the first 68 is interferance*/
		        if (pbuf[1] == FT12_VLF_START)
		        {
		            pbuf[0] = pbuf[1];
		            pbuf[1] = pbuf[2];
		            *pdatalen = 2;

		            //INFO("rxlen=3,pbuf[0]=%d, pbuf[1]=%d\n",pbuf[0],pbuf[1]);
		            return(0);
		        }

		        /*just like 68 77 68 09 09 68 53 B1 7F 67 00 05 B1 7F 00 1F 16,the first 68 77 is interferance*/
		        if (pbuf[2] == FT12_VLF_START)
		        {
		            pbuf[0] = pbuf[2];
		            *pdatalen = 1;
		            TCP_DEBUG("WARNING : rxlen=3,pbuf[0]=%d\n",pbuf[0]);
		            return(0);
		        }
		        /*zxxx just for test*/
		        *pdatalen = 0;
		        memset(pbuf, 0, databufsize);
		        return 1;
		    }
		}
		/* Check identity of both start specifications */
		else if (rxlen == 4)
		{
		    //INFO("rxlex=%d\n",rxlen);
		    if (pbuf[0] != pbuf[3])
		    {
		        /*just like 68 68 68 09 09 68 53 B1 7F 67 00 05 B1 7F 00 1F 16,the first 68 68 is interferance*/
		        if (pbuf[2] == FT12_VLF_START)
		        {
		            pbuf[0] = pbuf[2];
		            pbuf[1] = pbuf[3];
		            *pdatalen = 2;

		            TCP_DEBUG("WARNING : rxlen=4, pbuf[0]=%d, pbuf[1]=%d\n",pbuf[0],pbuf[1]);
		            return(0);
		        }
		        /*zxxx just for test*/
		        *pdatalen = 0;
		        memset(pbuf, 0, databufsize);
		        return 1;
		    }
		}
		else if (rxlen > 4)
		{
		    //INFO("rxlex=%d\n",rxlen);
		    /* Check checksum */
		    if (rxlen == (pbuf[1]+5))
		    {
		        if (pbuf[rxlen-1] != byte_checksum(&pbuf[4], pbuf[1]))
		        {
		            TCP_DEBUG("WARNING : byte_checksum wrong!\n");
		            /* if checksum wrong ,check if there is start char after this wrong start char.
		                           * just like 68 09 09 68 09 09 68 53 B1 7F 67 00 05 B1 7F 00 1F 16, the first 68 09 09 is interferance
		                           */
		            if (0 == iec102_search_startchar(rxlen, pbuf, pdatalen, pdataflags))
		            {
		            	TCP_DEBUG("WARNING : iec102_search_startchar()==0 , %s,%d\n",__FUNCTION__,__LINE__);
		                return 0;
		            }

		            *pdatalen = 0;
		            memset(pbuf, 0, databufsize);
		            return 1;
		        }
		    }
		    /* Check frame end character */
		    else if (rxlen == (pbuf[1]+6) )
		    {
		        //INFO("rxlex=%d\n",rxlen);
		        if (pbuf[rxlen-1] != FT12_VLF_END)
		        {
		            TCP_DEBUG("WARNING : end character wrong!\n");
		            if (0 == iec102_search_startchar(rxlen, pbuf, pdatalen, pdataflags))
		            {
		                return 0;
		            }
		            /*zxxx just for test*/

		            *pdatalen = 0;
		            memset(pbuf, 0, databufsize);
		            return 1;
		        }
		        /* Set frame accepted status */
		        pRX->dataflags |= FBF_EOR;
		    }
		    else if (rxlen > (pbuf[1]+6) )
		    {
		        TCP_DEBUG("WARNING : rxlen > (pbuf[1]+6)!\n");
		        if (0 == iec102_search_startchar(rxlen, pbuf, pdatalen, pdataflags))
		        {
		            return 0;
		        }

		        *pdatalen = 0;
		        memset(pbuf, 0, databufsize);
		        return 1;
		    }
		} /* else */

	} /* variable length frame */

	/* Not FT12 Frame */
	else
	{
	    TCP_DEBUG("WARNING : Not FT12 Frame\n");
	    *pdatalen = 0;
	    memset(pbuf, 0, databufsize);
	    return 1;
	}

	*pdatalen = rxlen;
	return 0;

}



ENN_ErrorCode_t dpa_judge_master_connect(ENNSock_In_Addr master_ip, int *master_no)
{
	char            connect_ip[24] = {0};
	int             len = 0;
	char            find = 0;
	DPA_MASTER_PARAM *current_master_table = NULL;

	len = strlen(inet_ntoa(master_ip));
	memcpy(connect_ip, inet_ntoa(master_ip), len);
	//TCP_DEBUG("%s, %d, masterIP = %s\n",__FUNCTION__,__LINE__,connect_ip);

	current_master_table = slave_table_Head->master_table;
	/*judge connect ip address is right*/
	while(current_master_table != NULL)
	{
		if (strcmp(connect_ip, current_master_table->master_ip_addr) == 0)
        	{
			find = 1;
			TCP_DEBUG("%s, %d, Matched !masterIP = %s\n",__FUNCTION__,__LINE__,connect_ip);
			break;
        	}
		current_master_table = current_master_table->next;
	}

	//TCP_DEBUG("%s, %d\n",__FUNCTION__,__LINE__);
	if (!find)
	{
		current_master_table = slave_table_Head->master_table;
		/*judge connect ip address is right*/
		while(current_master_table != NULL)
		{
			if (strcmp("0.0.0.0", current_master_table->master_ip_addr) == 0)
			{
				find = 1;
				TCP_DEBUG("%s, %d, Any Matched !masterIP = %s\n",__FUNCTION__,__LINE__,
					current_master_table->master_ip_addr);
				break;
			}
			current_master_table = current_master_table->next;
		}
	}

	//TCP_DEBUG("%s, %d\n",__FUNCTION__,__LINE__);
	/*connect ip address is wrong*/
	if (!find)
	{
		*master_no = -1;
		TCP_DEBUG("connect ip address is illegal!connect ip=%s!\n", connect_ip);
		return ENN_FAIL;
	}
	else
	{
		if(current_master_table != NULL)
		{	
			*master_no = current_master_table->master_no;
	    		TCP_DEBUG("connect ip address is right! connect ip=%s, remote master ip=%s\n",
	        	connect_ip, current_master_table->master_ip_addr);
		}
		return ENN_SUCCESS;
	}
}

int dpa_net_receive(ENN_S32 sockfd ,T_IEC102_FRAME_BUF *pRX)
{

	/* check input param */
	if ((sockfd == 0) || (pRX == NULL))
	{
		TCP_DEBUG("Error********* :%s, %d,input param is NULL!\n",__FUNCTION__,__LINE__);
		return DPA_FAIL;
	}

	/* init */
	ENN_S32 readlen;
	UINT8 rxbuf[MAX_DATA_LEN_102];
	UINT16 	maxBytes = MAX_DATA_LEN_102;
	UINT32 frame_timeout;
	UINT32	char_timeout = 200; //字节超时ms
	/*Select*/
	int	select_result = 0;
	int	i;
	int master_no;
	DPA_MASTER_PARAM *current_master_table;

	master_no = fd_index_to_master[currentIndex];
	if(master_no >0)
	{
		current_master_table = slave_table_Head->master_table;
		while(current_master_table != NULL)
		{
			if(master_no == current_master_table->master_no){
				/*TCP_DEBUG("%s, %d, found master_no = %d, IP = %s\n",__FUNCTION__,__LINE__, 
					current_master_table->master_no, current_master_table->master_ip_addr);*/
				break;
			}
			current_master_table = current_master_table->next;
		}
	}
	pRX->datalen = 0;
	memset(pRX->databuf, 0, MAX_DATA_LEN_102);
	memset(rxbuf, 0, MAX_DATA_LEN_102);
	while (pRX->datalen < maxBytes)
	{
		readlen = recv(sockfd, (char *)rxbuf, maxBytes -pRX->datalen, 0);
		
		if (readlen > 0)
		{	
			TCP_DEBUG("Receive master [%s] data:\n",current_master_table->master_ip_addr);
			TCP_DEBUG("%s, %d, len = %d, recv_data = ",__FUNCTION__,__LINE__, readlen);
			for (i = 0; i<readlen ; i++)
				TCP_DEBUG("%x ",rxbuf[i]);
			TCP_DEBUG("\n");
			for (i=0; i<readlen; i++)
				iec102_rx_data(pRX, rxbuf[i]);
			
			if ( iec102_rx_available(pRX))
			{
				return DPA_SUCCESS;
			}
			
		}
		else if (readlen <= 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			
			/*TCP_DEBUG("master(%d) read return readlen=%d, strerror(%d)=%s", 
						   pPort->pCfgMaster->master_no, readlen, errno, strerror(errno));*/
			TCP_DEBUG("read error: readlen=%d, strerror(%d)=%s", 
						   readlen, errno, strerror(errno));
			return DPA_RECEIVE_FAIL;
		}


		select_result = dpa_port_select(sockfd, char_timeout);	
		if (select_result == 0)		/* select timeout */
		{
            		TCP_DEBUG("call select timeout2, return=%s\n", strerror(errno));
			break;
		}
		else if (select_result < 0)	/* select error */
		{
			/*TCP_DEBUG("master(%d) select failed, strerror(%d)=%s", 
						   pPort->pCfgMaster->master_no, errno, strerror(errno));*/
			TCP_DEBUG("select failed, strerror(%d)=%s", 
						   errno, strerror(errno));
			return DPA_SELECT_FAIL;
		}
	}

	return DPA_SUCCESS;
}

int dpa_net_transmit(DPA_CONN_CONTEXT *context, ENN_S32 sockfd)
{
	/* check input param */
	if ((context == NULL) )
	{
		TCP_DEBUG("Error********* :%s, %d,input param is NULL!\n",__FUNCTION__,__LINE__);
		return DPA_FAIL;
	}

	int 	status = DPA_SUCCESS;
	UINT16 	data_len = context->txframe.datalen;
	UINT16  	sent_total_len = 0;
	UINT16  	sent_len       = 0;


	/* flush port's input/output buffer */
	tcflush(sockfd, TCIOFLUSH);
    
	while(sent_total_len < data_len)
	{
		sent_len = send(sockfd, &context->txframe.databuf[sent_total_len], data_len - sent_total_len, MSG_NOSIGNAL);
			
		if(sent_len <= 0)
		{
		    TCP_DEBUG("Error : send to master(%s) fail, return %d, strerror(%d)= %s!", 
		    	context->masterIP, sent_len, errno, strerror(errno));
		    status = DPA_SEND_FAIL;
		    break;
		}
		else
		{
		    sent_total_len += sent_len;
		}
	}

	TCP_DEBUG("send to master(%s),end, send %d bytes\n", 
		context->masterIP, sent_total_len);
	return status;
}

/*************************************************************************
*  名字:  iec102_rx_process
*  说明:  IEC102 协议TCP 连接的数据接收
*  输入参数：sockfd，与主站的TCP连接句柄；
*					Rxbuf，从主站接收命令数据缓存buf         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
int iec102_rx_process(ENN_S32 sockfd, T_IEC102_FRAME_BUF *Rxbuf)
{
	int status = 0;
	T_IEC102_FRAME_BUF *pRX =NULL;

/*	pRX = (T_IEC102_FRAME_BUF *)malloc(sizeof(T_IEC102_FRAME_BUF));
	if(NULL == pRX)
	{
		TCP_DEBUG("Error********* :%s, %d, malloc faile !\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}*/
	pRX = Rxbuf;

	status = dpa_net_receive(sockfd,pRX);
	if (status != DPA_SUCCESS) 
	{	if (status != DPA_SELECT_TIME_OUT)
		{
			TCP_DEBUG("ERR, dpa_port_receive return %d, exit\n", status);
		}		
	}

	return status;
}

/*************************************************************************
*  名字:  iec102_rx_process
*  说明:  IEC102 协议，响应主站所要求的数据
*  输入参数：context，该连接相应的数据结构及存储空间  
*					sockfd，与主站的TCP连接句柄；
*					
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
int iec102_tx_process(DPA_CONN_CONTEXT *context, ENN_S32 sockfd)
{
	int status = DPA_SUCCESS;

#if 1	
	if (context->txframe.datalen)
	{
		status = dpa_net_transmit(context,sockfd);
		memset(&context->txframe, 0, sizeof(context->txframe));
	}
#else
	if (pPort->context.rxframe.datalen)
	{
		memcpy(&pPort->context.txframe.databuf, &pPort->context.rxframe.databuf, pPort->context.rxframe.datalen);
		pPort->context.txframe.datalen = pPort->context.rxframe.datalen;
		status = dpa_port_transmit(pPort);
		memset(&pPort->context.txframe, 0, sizeof(pPort->context.txframe));
	}
#endif
	return status;
}

/*************************************************************************
*  名字:  iec102_frame_process
*  说明:  IEC102 协议TCP 连接的接收数据，解析命令
*  输入参数：
*					context，该连接相应的数据结构及存储空间         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/

void iec102_frame_process(DPA_CONN_CONTEXT *context)
{
	UINT8		service;
	void*			udata;
	T_IEC102_ATTR	*p102attr = NULL;
	
	int master_no;
	DPA_MASTER_PARAM *current_master_table;

	/* Check if Rx Frame Available */
	if (!iec102_rx_available(&context->rxframe))
	{
	    return;
	}

	p102attr = &context->seq.attr.iec870_102attr;
	//context->initflag = 1;

	master_no = fd_index_to_master[currentIndex];
	if(master_no >0)
	{
		current_master_table = slave_table_Head->master_table;
		while(current_master_table != NULL)
		{
			if(master_no == current_master_table->master_no){
				/*TCP_DEBUG("%s, %d, found master_no = %d, IP = %s\n",__FUNCTION__,__LINE__, 
					current_master_table->master_no, current_master_table->master_ip_addr);*/
				break;
			}
			current_master_table = current_master_table->next;
		}
	}
	if(current_master_table != NULL){
		p102attr->laddr = current_master_table->link_addr;
		context->sync_time_flag = current_master_table->sync_time_flag;
		context->data_period=  current_master_table->data_period;
		memcpy(context->masterIP, current_master_table->master_ip_addr, sizeof(current_master_table->master_ip_addr));
		TCP_DEBUG("%s, %d, link_addr = %d, IP = %s\n",__FUNCTION__,__LINE__, 
					current_master_table->link_addr, current_master_table->master_ip_addr);
	}else{
		TCP_DEBUG("WARNING : %s, %d, not found link_addr\n",__FUNCTION__,__LINE__);
	}

	//IEC102_DEBUG("[%s],%d,laddr=%d\n", __FUNCTION__,__LINE__,context->seq.attr.iec870_102attr.laddr);
	service = FT12_ll_con_ind(context, &udata);
	//IEC102_DEBUG("[%s],%d,laddr=%d\n", __FUNCTION__,__LINE__,context->seq.attr.iec870_102attr.laddr);
	TCP_DEBUG("Test**********:service = %d\n",service);
	if ((service == FT12_LL_ERROR_FRAME) ||
        (service == FT12_LL_ERROR_ADDRESS) ||
        (service == FT12_LL_ERROR))
	{
		iec102_rxframe_discard(context);
		return;
	}

	 /* Check for event to handle */
	if (p102attr->iec870seq.commrun == 1)
	{
/*		if (1 == pPort->initflag)
		{
		    p102attr->iec870seq.eventind = 1;
		}
		else
		{
		    p102attr->iec870seq.eventind = 0;
		}*/
	}

	p102attr->iec870seq.extcom = 0;
	 switch(service)
	{
		case FT12_LL_CONFIRM_ACK:
		    break;

		case FT12_LL_CONFIRM_NAK:
		    break;

		case FT12_LL_INDICATE:
		    iec102_slave(context, (UINT8*)udata);
		    break;

		default:
		    break;
	}

	iec102_rxframe_discard(context);
}

/*************************************************************************
*  名字:  iec102_rx_frame_seq_tx_process
*  说明: IEC102 协议，解析命令，组织主站所要求的数据
*  输入参数：
*					context，该连接相应的数据结构及存储空间         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
void iec102_seq_process(DPA_CONN_CONTEXT *context)
{
	if (NULL == context)
	{
		TCP_DEBUG( "NULL pointer!,%s,%d\n",__FUNCTION__,__LINE__);
		return;
	}

	iec102_slave_seq(context);

}

/*************************************************************************
*  名字:  iec102_rx_frame_seq_tx_process
*  说明:  IEC102 协议TCP 连接的数据接收与响应的入口函数
*  输入参数：sockfd，与主站的TCP连接句柄；
*					fcontext，该连接相应的数据结构及存储空间         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
int iec102_rx_frame_seq_tx_process(ENN_S32 sockfd,DPA_CONN_CONTEXT *fcontext)
{
	DPA_CONN_CONTEXT *context;
	T_IEC102_FRAME_BUF *pRX =NULL;
	T_IEC102_SEQUENCE *seq = NULL;

/*	pRX = (T_IEC102_FRAME_BUF *)malloc(sizeof(T_IEC102_FRAME_BUF));
	if(NULL == pRX)
	{
		TCP_DEBUG("Error********* :%s, %d, malloc faile !\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	seq = (T_IEC102_SEQUENCE *)malloc(sizeof(T_IEC102_SEQUENCE));
	if(NULL == seq)
	{
		TCP_DEBUG("Error********* :%s, %d, malloc faile !\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}*/
	context = fcontext;
	pRX = &context->rxframe;
	
	/*if rx error, close old connect and setup a new connect*/
	if (iec102_rx_process(sockfd, pRX) < 0)
	{
	   TCP_DEBUG("ERROR : receive data failed!,%s,%d\n",__FUNCTION__,__LINE__);
	   return DPA_FAIL;
	}

	 /*process received data and orgnized sent data*/
	iec102_frame_process(context);
	iec102_seq_process(context);

	if (iec102_tx_process(context, sockfd) < 0)
	{
		TCP_DEBUG("ERROR : send data failed!,%s,%d\n",__FUNCTION__,__LINE__);
		return DPA_FAIL;
	}
   
	return DPA_SUCCESS;

}



ENN_ErrorCode_t ENNModBus_TCP_Receive(ENN_S32 socket, ENN_U8 *pbuf, ENN_S32 RecvLen)
{
	ENN_U8 *pTemp = NULL;
	ENN_U8 *pSendBuf = NULL;
	ENN_U16 u16Len = 0;
	ENN_S32 s32RetLen = 0;
	ENN_U16 u16CRC = 0;
	int i = 0;
	
	ENNAPI_ASSERT((NULL != pbuf) && (7 < RecvLen));
	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

	/*while(!bflag)
	{
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
		ENNOS_DelayTask(100);
	}*/

	bflag = ENN_FALSE;
	pTemp = pbuf;
	memset(aMBTCP_HEAD, 0, 7);
	g32Socket = socket;
	memcpy(aMBTCP_HEAD, pbuf, 7);
	u16Len = ((ENN_U16)(pTemp[4])) << 8;
	u16Len |= (ENN_U16)(pTemp[5]);

	pSendBuf = (ENN_U8 *)malloc(u16Len + 2);
	if(NULL == pSendBuf)
	{
		bflag = ENN_TRUE;
		return ENN_FAIL;
	}
	memset(pSendBuf, 0, (u16Len + 2));
	memcpy(pSendBuf, (pTemp + 6), u16Len);
	u16CRC = CRC16(pSendBuf, u16Len);
	ENNTRACE("u16CRC = 0x%x\n",u16CRC);
	ENNTRACE("u16Len = %d\n",u16Len);
	pSendBuf[u16Len] = (ENN_U8)(u16CRC & 0x00FF);
	pSendBuf[u16Len+1] = (ENN_U8)(u16CRC >> 8);
	for(i = 0; i < (u16Len + 2); i++)
			ENNTRACE(" %2.2x", pSendBuf[i]);
	ENNTRACE("\n");
	s32RetLen = UART_Write(CHANNEL3, pSendBuf, (u16Len + 2));
	ENNTRACE("s32RetLen = %d\n",s32RetLen);

	bReadflag = ENN_TRUE;

	return ENN_SUCCESS;
}

//static ENN_U8 SendBuf[17];
//ENN_U32	count = 0;

ENN_ErrorCode_t ENNModBus_Msg_Rx_Ex(void)
{
	ENN_U8 data[512];
	ENN_U16 len = 0;	
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	DATAQ_FRAME stDATAQ_FRAME;
	ENN_U8 *pData = NULL;
	ENN_U8 u8DataLen = 0;
	ENN_U32 u32Len = 0;
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U16 u16RegAddr = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U8 *pResData = NULL;
	ENN_U8 u8SlaveAddr = 0;
	ENN_U8 *pSendBuf = NULL;
	ENN_U8 i = 0;
	ENN_U8 u8Num = 0;
	ENN_U8 u8ModBusTCPLen = 0;
	ENN_S32 err = 0;
	ENN_U8 DataLen = 0;
	ENN_U8 TcpLen = 0;

	while(1)
	{
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
		returnCode = ENNOS_ReceiveMessage(gModBusTaskQueueID, ENNOS_TIMEOUT_INFINITY, (ENN_VOID *)&stDATAQ_FRAME);
		if(ENN_SUCCESS != returnCode)
		{
			ENNTRACE("\nReceive MSG Queue fail!\n");
			continue;
		}
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

		pData = stDATAQ_FRAME.p_data;
		u32Len = stDATAQ_FRAME.length;
		if((NULL == pData) || (0 == u32Len))
		{
			continue;
		}

		for(i = 0; i < u32Len; i++)
			ENNTRACE("%2.2x ", pData[i]);
		ENNTRACE("\n");

		if(u32Len > 7)
		{
			Len = 7;
			u8SlaveAddr = pData[6];
			u8FunCode = pData[Len++];

			ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__, u8FunCode);
			switch(u8FunCode)
			{
				case MB_FUNC_READ_COILS:
				case MB_FUNC_READ_DISCRETE_INPUTS:
					break;
				case MB_FUNC_READ_HOLDING_REGISTER:
					u16RegAddr = ((ENN_U16)(pData[Len++])) << 8;
					u16RegAddr |= (ENN_U16)(pData[Len++]);
					
					u16RegNum = ((ENN_U16)(pData[Len++])) << 8;
					u16RegNum |= (ENN_U16)(pData[Len++]);
					
					pResData = ENNModBus_Serch_FunCode_Data(u8FunCode, u16RegAddr, u16RegNum, &u8DataLen);
					if((NULL != pResData) && (0 != u8DataLen))
					{
						u8Num = u8DataLen + 1 + 1 + 1;
						u8ModBusTCPLen = u8DataLen;
						pSendBuf = (ENN_U8 *)malloc(7 + 1 + 1 + u8DataLen);
						if(NULL != pSendBuf)
						{
							memcpy(pSendBuf, pData, 7);
							pSendBuf[4] = (ENN_U8)(u8Num>>8);
							pSendBuf[5] = (ENN_U8)(u8Num & 0x00FF);
							*(pSendBuf+7) = u8FunCode;
							pSendBuf[8] = u8ModBusTCPLen;
							memcpy(pSendBuf+9, pResData, u8ModBusTCPLen);
							
							ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);

							for(i = 0; i < (u8DataLen + 1 + 7 + 1); i++)
								ENNTRACE("%2.2x ", pSendBuf[i]);
							ENNTRACE("\n");

							err = ENNSock_SocketSend(stDATAQ_FRAME.frame_net_info.u32Socket, pSendBuf, u8DataLen + 1 + 7 + 1, 1);
							if(err <= 0)
							{
								free(pSendBuf);
								pSendBuf = NULL;
							}
							//ENNModBus_TCP_Send(stDATAQ_FRAME.frame_net_info.u32Socket, pSendBuf, u8DataLen + 1 + 7 + 1);
						}
					}
					break;
				case MB_FUNC_READ_HISTORY_DATA:
					u16RegNum = ((ENN_U16)(pData[Len++])) << 8;
					u16RegNum |= (ENN_U16)(pData[Len++]);

					ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16RegNum);
					
					pResData = (ENN_U8 *)malloc(u16RegNum);
					if(NULL != pResData)
					{
						memset(pResData, 0, u16RegNum);
						returnCode = ENNModBus_Get_History_Data(pResData, u16RegNum, &DataLen);
						ENNTRACE("%s, %d, %d, %d\n",__FUNCTION__,__LINE__,u16RegNum, returnCode);
						if((ENN_SUCCESS != returnCode) || (0 == DataLen))
						{
							free(pResData);
							pResData = NULL;
							u16RegNum = 0;

							return ENN_FAIL;
						}
						ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,u16RegNum);
						
						u8Num = DataLen + 2 + 1 + 1;
						TcpLen = 7 + 1 + 2 + DataLen;
						pSendBuf = (ENN_U8 *)malloc(TcpLen);
						if(NULL != pSendBuf)
						{
							memcpy(pSendBuf, pData, 7);
							pSendBuf[4] = (ENN_U8)(u8Num >> 8);
							pSendBuf[5] = (ENN_U8)(u8Num & 0x00FF);
							*(pSendBuf+7) = u8FunCode;
							pSendBuf[8] = (ENN_U8)(DataLen >> 8);
							pSendBuf[9] = (ENN_U8)(DataLen & 0x00FF);
							for(i = 0; i < DataLen; i++)
								ENNTRACE("%2.2x ", pResData[i]);
							ENNTRACE("\n");
							if(NULL != pResData)
							{
								memcpy(pSendBuf+10, pResData, DataLen);
							}
							
							//ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);

							for(i = 0; i < TcpLen; i++)
								ENNTRACE("%2.2x ", pSendBuf[i]);
							ENNTRACE("\n");

							err = ENNSock_SocketSend(stDATAQ_FRAME.frame_net_info.u32Socket, pSendBuf, TcpLen, 1);
							ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,err);
							perror("send history data: ");
							if(err <= 0)
							{
								free(pSendBuf);
								pSendBuf = NULL;
							}
							
							//exit(0);
							//ENNModBus_TCP_Send(stDATAQ_FRAME.frame_net_info.u32Socket, pSendBuf, u8DataLen + 1 + 7 + 1);
						}
					}
				default:
					break;
			}

#if 0
			returnCode = ENNModBus_Request(CHANNEL1, u8SlaveAddr, u8FunCode, u16RegAddr, u16RegNum);
			if(ENN_SUCCESS != returnCode)
			{
				continue;
			}
			returnCode = ENNModBus_Select(0, 100);
			ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
			if(ENN_SUCCESS == returnCode)
			{
				len = ENNModBus_Response(CHANNEL1, &u8SlaveAddr, &u8FunCode, data);
				ENNTRACE("%s, %d, len = %d, u8FunCode = %d\n",__FUNCTION__,__LINE__,len, u8FunCode);
				ENNTRACE("u8FunCode = %d\n", u8FunCode);
				if(len > 0)
				{
					u16Num = len + 1 + 1 + 1;
					pSendBuf = (ENN_U8 *)malloc(len + 1 + 7 + 1);
					if(NULL != pSendBuf)
					{
						memcpy(pSendBuf, pData, 7);
						pSendBuf[4] = (ENN_U8)(u16Num>>8);
						pSendBuf[5] = (ENN_U8)(u16Num & 0x00FF);
						*(pSendBuf+7) = u8FunCode;
						pSendBuf[8] = len;
						memcpy(pSendBuf+9, data, len);
						
						ENNTRACE("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);

						for(i = 0; i < (len + 1 + 7 + 1); i++)
							ENNTRACE("%2.2x ", pSendBuf[i]);
						ENNTRACE("\n");

						ENNModBus_TCP_Send(stDATAQ_FRAME.frame_net_info.u32Socket, pSendBuf, len + 1 + 7 + 1);
					}
				}
			}
#endif
		}
	}
	
	return ENN_SUCCESS;
}


ENN_ErrorCode_t ENNModBus_Msg_Rx(void)
{
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	DATAQ_FRAME stDATAQ_FRAME;
	ENN_U8 *pData = NULL;
	ENN_U32 u32Len = 0;
	ENN_U32 Len = 0;
	ENN_U8 u8FunCode = 0;
	ENN_U8 u8SlaveAddr = 0;
	/*ENN_U8 data[512];
	ENN_U16 len = 0;	
	ENN_U16 u16DataLen = 0;
	ENN_U16 u16RegAddr = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U8 *pResData = NULL;
	ENN_U8 *pSendBuf = NULL;
	ENN_U8 i = 0;
	ENN_U16 u16Num = 0;
	ENN_U16 u16ModBusTCPLen = 0;
	ENN_S32 err = 0;
	ENN_U16 u16CoilLen = 0;
	ENN_U8 u8Bytes = 0;
	ENN_U8 u8ExceptionCode = 0;
	ENN_U16 u16InputValue = 0;*/
	int fd;

	while(1)
	{
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
		returnCode = ENNOS_ReceiveMessage(gModBusTaskQueueID, ENNOS_TIMEOUT_INFINITY, (ENN_VOID *)&stDATAQ_FRAME);
		if(ENN_SUCCESS != returnCode)
		{
			ENNTRACE("\nReceive MSG Queue fail!\n");
			continue;
		}
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
		
		fd = stDATAQ_FRAME.frame_net_info.u32Socket;
		ENNTRACE("%s, %d,fd = %d\n",__FUNCTION__,__LINE__,fd);
		pData = stDATAQ_FRAME.p_data;
		u32Len = stDATAQ_FRAME.length;
		if((NULL == pData) || (0 == u32Len))
		{
			continue;
		}

		if(u32Len > 7)
		{
			Len = 7;
			u8SlaveAddr = pData[6];
			u8FunCode = pData[Len++];
			fd = stDATAQ_FRAME.frame_net_info.u32Socket;
			ENNTRACE("%s, %d,fd = %d\n",__FUNCTION__,__LINE__,fd);
			_ENNModBus_Func_Parser(stDATAQ_FRAME.frame_net_info.u32Socket, pData);

			free(pData);
			pData = NULL;
		}
		ENNOS_DelayTask(100);
	}
	
	//return ENN_SUCCESS;
}

ENN_ErrorCode_t ENNModBus_TCP_Server(void)
{
	fd_set  fdsr;
	ENN_S32 ret = 0;
	ENN_S32 MaxSock = 0;
	ENN_S32 ListeningSocket = 0;
	ENN_S32 NewConnection = 0;
	ENN_S32	RecvLen = 0;
	ENN_S32 yes = 1;
	ENN_S32 sin_size = 0;
	struct 	timeval tv;
	struct 	ENNSock_SocketAddr_In s_add;
	struct 	ENNSock_SocketAddr_In c_add;
	ENN_U8 buf[TCP_BUF_SIZE];
	int i;
	int num;
	DATAQ_FRAME stDATAQ_FRAME;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;	

	ENN_U16 server_port;
	int status = ENN_SUCCESS;
	int master_no = -1;
	DPA_CONN_CONTEXT * context;

	memset(fd_index_to_master, 0, sizeof(fd_index_to_master));
	
	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	ListeningSocket = ENNSock_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);       //建立socket  连接
	if(-1 == ListeningSocket)
	{
	    ENNTRACE("socket fail ! \r\n");
	    return ENN_FAIL;
	}

	ENNTRACE("socket ok !\r\n");
	//允许重复使用本地地址与套接字绑定
	if(-1 == ENNSock_SetSockOpt(ListeningSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)))
	{
		ENNTRACE("socket fail ! \r\n");
	        return ENN_FAIL;
	}

	bzero(&s_add, sizeof(struct ENNSock_SocketAddr_In));
	s_add.sin_family = AF_INET;
	/*if(CurrentDPAMode == ENN_IEC102_MODE)
	{
		server_port = slave_table_Head->slave_port;
	}else
	{
		server_port = SERVER_PORT;
	}*/
	if(pDEVICE_IP_PARAM->serverport > 0)
		server_port = pDEVICE_IP_PARAM->serverport;
	else
	{
		server_port = SERVER_PORT;
	}
	TCP_DEBUG("%s, %d, server_port = %d\n",__FUNCTION__,__LINE__, server_port);
	s_add.sin_port = (unsigned short)ENNSock_htons(server_port);
	s_add.sin_addr.s_addr = (unsigned int)ENNSock_htonl(INADDR_ANY);
	memset((void *)(s_add.sin_zero), 0, sizeof(s_add.sin_zero));

	if(-1 == ENNSock_Bind(ListeningSocket, (struct ENNSock_SocketAddr *)(&s_add), sizeof(struct ENNSock_SocketAddr)))
	{
		ENNTRACE("bind fail !\r\n");
		return ENN_FAIL;
	}
	ENNTRACE("bind ok !\r\n");

	if(-1 == ENNSock_Listen(ListeningSocket, MAXCLINE))
	{
	    ENNTRACE("listen fail !\r\n");
	    return ENN_FAIL;
	}
	ENNTRACE("listen ok\r\n");

	conn_amount =0;
	sin_size = (ENN_S32)sizeof(c_add);
	MaxSock = ListeningSocket;
	/*maxfd = MAX(maxfd, ListeningSocket+1);*/

	while(1)
	{
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
		/*初始化文件描述符集合*/
	    	FD_ZERO(&fdsr); 					/*清除描述符集*/
	    	FD_SET(ListeningSocket, &fdsr); 	/*把sock_fd加入描述符集*/

			/*超时的设定*/
	    	tv.tv_sec = 3;
	        tv.tv_usec = 0;

		/*添加活动的连接*/
	   	 for(i=0; i<MAXCLINE; i++) 
	    	{
				if(0 != fd[i])
				{
					FD_SET(fd[i], &fdsr);
				}
	   	 }
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	    
		/*如果文件描述符中有连接请求 会做相应的处理，实现I/O的复用 多用户的连接通讯*/
		ret = ENNSock_Select((ENN_S32)(MaxSock+1), (ENNSock_fd_set *)&fdsr, NULL, NULL, (ENNSock_Timeval *)&tv);
		if(ret < 0) 		/*没有找到有效的连接 失败*/
		{
			printf("	[%s %d select error!!]\r\n",__FUNCTION__, __LINE__);
			break;
		}
		else if(0 == ret)	/* 指定的时间到*/
		{
			ENNTRACE("	[%s %d timeout!]\r\n",__FUNCTION__, __LINE__);
			continue;
		}
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

		/*循环判断有效的连接是否有数据到达*/
		for(i=0; i<conn_amount; i++)
		{
			if(FD_ISSET(fd[i], &fdsr))
			{
				if(CurrentDPAMode == ENN_IEC102_MODE)
				{
					currentIndex = i;
					if(Pcontext[i] != NULL)
						status = iec102_rx_frame_seq_tx_process(fd[i],Pcontext[i]);
					else
						TCP_DEBUG("Error*****:Pcontext[i] == NULL\n",i);
					if(status < 0)
					{
						TCP_DEBUG("WARNING*****:client[%d] close\n",i);
						ENNSock_SocketClose(fd[i]);
						FD_CLR(fd[i], &fdsr);
						fd[i]=0;
						fd_index_to_master[i] = -1;
						free(Pcontext[i]->prof_data.results);
						Pcontext[i]->prof_data.results = NULL;
						free(Pcontext[i]);
						Pcontext[i] = NULL;
						conn_amount--;
					}
				}
				else
				{
					memset((void *)buf, 0, sizeof(buf));
					RecvLen = ENNSock_SocketRecv(fd[i], (void *)buf, sizeof(buf), 0);
					if(RecvLen <= 0) /*客户端连接关闭，清除文件描述符集中的相应的位*/
					{
						/*ENNModBus_Tcp_Del_List(fd[i]);*/
					
						TCP_DEBUG("WARNING*****:client[%d] close\n",i);
						ENNSock_SocketClose(fd[i]);
						FD_CLR(fd[i], &fdsr);
						fd[i]=0;
						conn_amount--;
					}
					else 	/*否则有相应的数据发送过来 ，进行相应的处理*/
					{
						/*RecvLen = ENNSock_SocketSend(fd[i], buf, RecvLen, 1);
						if(RecvLen <= 0)
						{
							ENNTRACE("%s, %d, RecvLen = %d\n",__FUNCTION__,__LINE__,RecvLen);
						}*/
						ENNTRACE("%s, %d, RecvLen = %d\n",__FUNCTION__,__LINE__,RecvLen);
						printf("%s, %d, RecvLen = %d\n",__FUNCTION__,__LINE__,RecvLen);
						printf("%s, %d, buf  = %s\n",__FUNCTION__,__LINE__,buf);
						int j;
						for(j=0;j<RecvLen;j++)
						{
							printf("buf[%d] = %x\n",j,buf[j]);
						}
						printf("\n");
						if(RecvLen > 7)  
						{
							memset((void *)&stDATAQ_FRAME, 0, sizeof(DATAQ_FRAME));
							stDATAQ_FRAME.p_data = (ENN_U8 *)malloc(RecvLen);
							if(NULL != stDATAQ_FRAME.p_data)
							{
								memset((void *)stDATAQ_FRAME.p_data, 0, (size_t)RecvLen);
								memcpy((void *)stDATAQ_FRAME.p_data, buf, (size_t)RecvLen);
								stDATAQ_FRAME.p_buf = stDATAQ_FRAME.p_data;
								stDATAQ_FRAME.length = RecvLen;
								stDATAQ_FRAME.frame_net_info.u32Socket = fd[i];
								ENNTRACE("%s, %d, stDATAQ_FRAME.frame_net_info.u32Socket= %d\n",__FUNCTION__,__LINE__,stDATAQ_FRAME.frame_net_info.u32Socket);
								ENNTRACE("%s, %d, fd[%d]= %d\n",__FUNCTION__,__LINE__,i,fd[i]);
								returnCode = ENNOS_SendMessage(gModBusTaskQueueID, (ENN_VOID *)&stDATAQ_FRAME);
								ENNTRACE("%s, %d, returnCode = %d\n",__FUNCTION__,__LINE__,returnCode);
							}
					         }

				          }
				}
			}
		}

		if(FD_ISSET(ListeningSocket, &fdsr))
		{
			NewConnection = ENNSock_Accept(ListeningSocket,(struct ENNSock_SocketAddr *)(&c_add), &sin_size);
			if(-1 == NewConnection)
			{
				TCP_DEBUG("	[%s %d accept error!]\r\n",__FUNCTION__, __LINE__);
				continue;
			}
			TCP_DEBUG("%s, %d\n",__FUNCTION__,__LINE__);
//102
			if(CurrentDPAMode == ENN_IEC102_MODE)
			{
				if(dpa_judge_master_connect(c_add.sin_addr, &master_no) != ENN_SUCCESS)
				{
					TCP_DEBUG("%s, %d,connect IP not match !\n",__FUNCTION__,__LINE__);
					close(NewConnection);
					continue;
				}
			}
			/*添加新的fd 到数组中 判断有效的连接数是否小于最大的连接数，如果小于的话，就把新的连接套接字加入集合*/
			if(conn_amount < MAXCLINE)
			{
				for(i=0; i< MAXCLINE; i++)
				{
					if(0 == fd[i])
					{
						fd[i] = NewConnection;
						ENNTRACE("%s, %d, NewConnection = %d\n",__FUNCTION__,__LINE__,NewConnection);
						ENNTRACE("%s, %d, fd[%d] = %d\n",__FUNCTION__,__LINE__,i,fd[i]);
						if(CurrentDPAMode == ENN_IEC102_MODE)
						{
							if(master_no > 0){
								fd_index_to_master[i] = master_no;
								context = (DPA_CONN_CONTEXT *)malloc(sizeof(DPA_CONN_CONTEXT));
								if(NULL == context)
								{
									TCP_DEBUG("Error********* :%s, %d, malloc faile !\n",__FUNCTION__,__LINE__);
									return ENN_FAIL;
								}
								memset(context , 0 , sizeof(DPA_CONN_CONTEXT));
								Pcontext[i] = context;
								
								context->prof_data.results = 
									malloc(sizeof(DATA_PROFILE_REC) * INFOOBJ_MAX_NUM);
								if (context->prof_data.results == NULL)
								{
						            		TCP_DEBUG("Error********:%s, %d,malloc profile result error!",__FUNCTION__,__LINE__);
									return DPA_FAIL;
								}
								memset(context->prof_data.results, 0, 
									sizeof(DATA_PROFILE_REC) * INFOOBJ_MAX_NUM);
							}
							else
								TCP_DEBUG("Error********:%s, %d,master_no <0 !\n",__FUNCTION__,__LINE__);
						}
						break;
					}
				}
				conn_amount++;
				
				TCP_DEBUG("new connection client[%d]%s:%d\n",conn_amount,ENNSock_Inet_ntoa(c_add.sin_addr),ENNSock_ntohs(c_add.sin_port));
				if(NewConnection > MaxSock)
				{
					MaxSock = NewConnection;
				}
			}
			else
			{
				TCP_DEBUG("max connections arrive ,exit\n");
				send(NewConnection,"bye",4,0);
				close(NewConnection);
				continue;
			}
		}

		//showclient();
	}

	for(i=0;i<MAXCLINE; i++)
	{
		if(0 != fd[i])
		{
			ENNSock_SocketClose(fd[i]);
		}
	}

	return ENN_FAIL;

	/*while(1)
	{
		sin_size = sizeof(struct ENNSock_SocketAddr_In);

		NewConnection = ENNSock_Accept(ListeningSocket, (struct ENNSock_SocketAddr *)(&c_add), &sin_size);
		if(-1 == NewConnection)
		{
		    ENNTRACE("accept fail !\r\n");
		    return -1;
		}
		ENNTRACE("accept ok!\r\nServer start get connect from %#x : %#x\r\n",ENNSock_ntohl(c_add.sin_addr.s_addr), ENNSock_ntohs(c_add.sin_port));

		recvfrom()
		if(-1 == write(nfp,"hello,welcome to my server \r\n",32))
		{
		    ENNTRACE("write fail!\r\n");
		    return -1;
		}
		ENNTRACE("write ok!\r\n");
		close(NewConnection);
	}*/
}


ENN_ErrorCode_t InitENNModBus_Task()
{
	ENNOS_TASK_t taskID = 0;
	ENNOS_TASK_t taskMsgID = 0;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	
	returnCode = ENNOS_CreateMessage(sizeof(DATAQ_FRAME), 100, ENNOS_MSG_CREATE_FIFO, &gModBusTaskQueueID);
	if(ENN_SUCCESS != returnCode)
	{
		ENNTRACE("\nCreate MSG Queue fail!\n");
		return returnCode;
	}
                                                                                                                                                                  //4KB
	returnCode = ENNOS_CreateTask("TCP_SERVER", ENNOS_TASK_PRIORITY_MIDDLE, 4*1024, &taskID, ENNOS_TASK_START, (void*)ENNModBus_TCP_Server, NULL);
	if(ENN_SUCCESS != returnCode)
	{
		ENNTRACE("\nCreate TCP SERVER task fail!\n");
		return returnCode;
	}
	printf("%s, %d\n",__FUNCTION__,__LINE__);

	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	returnCode = ENNOS_CreateTask("MSG_RX", ENNOS_TASK_PRIORITY_MIDDLE, 4*1024, &taskMsgID, ENNOS_TASK_START, (void*)ENNModBus_Msg_Rx, NULL);
	if(ENN_SUCCESS != returnCode)
	{
		ENNTRACE("\nCreate TCP SERVER task fail!\n");
		return returnCode;
	}

	return ENN_SUCCESS;
}

char * ModBus_Copy_Data(ENN_U8 FunCode)
{
	FunCode_List *pFunCode_List_Temp = NULL;
	int i = 0;

	char *p = (char *)malloc(128);
	memset(p,0,128);
	
	pFunCode_List_Temp = gFunCode_List_head;
	while(NULL != pFunCode_List_Temp)
	{
		if(FunCode == pFunCode_List_Temp->u8MBFunCode)
		{
			printf("DataLen = %d \n", pFunCode_List_Temp->DataLen);
			if(0x03 == FunCode)
			{

				memcpy(p,pFunCode_List_Temp->pData,pFunCode_List_Temp->DataLen);
				/*
				for(i=0;i<pFunCode_List_Temp->DataLen;i++)
				
				{
					
					printf("%x",(pFunCode_List_Temp->pData[i]));
	
				}
				*/
			}

			break;
		}

		pFunCode_List_Temp = pFunCode_List_Temp->next;
	}
	return p;
}


ENN_ErrorCode_t   ENNAmmeter_Server_Tcp(void)
{
	struct 	ENNSock_SocketAddr_In s_add;
	struct 	ENNSock_SocketAddr_In c_add;
	ENN_S32	ListeningSocket;
	ENN_S32        NewConnection;
	ENN_S32	RecvLen ;
	ENN_S32        sendret;
	ENN_S32 yes = 1;

	unsigned char *p = NULL;
	//ENN_CHAR     Sendbuf[128]=;
	ENN_S32 sin_size = 0;
	
	ENN_U8 FunCode=0x03;
	FunCode_List *pFunCode_List_Temp = NULL;
	int i = 0;
	int j;
	int flag =1;

	
	ListeningSocket= ENNSock_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);       //建立socket  连接
	if(-1 == ListeningSocket)
	{
	    ENNTRACE("socket fail ! \r\n");
	    return ENN_FAIL;
	}

	if(-1 == ENNSock_SetSockOpt(ListeningSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)))
	{
		ENNTRACE("socket fail ! \r\n");
	        return ENN_FAIL;
	}

	bzero(&s_add, sizeof(struct ENNSock_SocketAddr_In));
	s_add.sin_family = AF_INET;
	s_add.sin_port = (unsigned short)ENNSock_htons(8000);
	s_add.sin_addr.s_addr = (unsigned int)ENNSock_htonl(INADDR_ANY);
	//s_add.sin_addr.s_addr = INADDR_ANY;
	memset((void *)(s_add.sin_zero), 0, sizeof(s_add.sin_zero));

	if(-1 == ENNSock_Bind(ListeningSocket, (struct ENNSock_SocketAddr *)(&s_add), sizeof(struct ENNSock_SocketAddr)))
	{
		ENNTRACE("bind fail !\r\n");
		return ENN_FAIL;
	}
	ENNTRACE("bind ok !\r\n");

	if(-1 == ENNSock_Listen(ListeningSocket, MAXCLINE))
	{
		ENNTRACE("listen fail !\r\n");
		return ENN_FAIL;
	}
	ENNTRACE("listen ok\r\n");

	sin_size = (ENN_S32)sizeof(c_add);
	NewConnection = ENNSock_Accept(ListeningSocket,(struct ENNSock_SocketAddr *)(&c_add), &sin_size);
	if(-1 == NewConnection)
	{
		printf("[%s %d accept error!]\r\n",__FUNCTION__, __LINE__);
	}
	printf("%s, %d    accept sucess!\n",__FUNCTION__,__LINE__);
	

	while(1)
	{
		//memset((void *)Sendbuf,0,sizeof(Sendbuf));
		p = ModBus_Copy_Data(0x03);
		printf("wanglongchang  Sendbuf is %s\n",p);
		sendret = ENNSock_SocketSend(NewConnection,p,strlen(p),0);	
		if(sendret < 0)
		{
			ENNTRACE("send fail !\r\n");
			break;;
		}
		else
		{
			for(i = 0;i<sendret;i++)
			{
				printf("wanglongchang %x",p[i]);
			}
			printf("p_len is %d\n",strlen(p));
			ENNTRACE("send sucess !\r\n");
		}
		sleep(2);
		
		
		
	}
	return  ENN_SUCCESS;

}
	
#if 0
ENN_S32 ENNModBus_TCP_Server(void)
{
	fd_set  fdsr;
	struct timeval tv;
	int MaxSock = 0;
	ENN_S32 ListeningSocket = 0;
	ENN_S32 NewConnection = 0;
	ENN_S32	RecvLen = 0;
	int 	sin_size;
	struct 	sockaddr_in s_add;
	struct 	sockaddr_in c_add;
	ENN_U8 buf[TCP_BUF_SIZE];
	int yes = 1;
	int ret;
	int i;
	
	ListeningSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == ListeningSocket)
	{
	    ENNTRACE("socket fail ! \r\n");
	    return -1;
	}

	ENNTRACE("socket ok !\r\n");

	if(-1 == setsockopt(ListeningSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)))
	{
		ENNTRACE("socket fail ! \r\n");
	    return -1;
	}

	s_add.sin_family = AF_INET;
	s_add.sin_addr.s_addr = htonl(INADDR_ANY);
	s_add.sin_port = htons(SERVER_PORT);
	memset(s_add.sin_zero, 0, sizeof(s_add.sin_zero));

	if(-1 == bind(ListeningSocket, (struct sockaddr *)(&s_add), sizeof(struct sockaddr)))
	{
		ENNTRACE("bind fail !\r\n");
		return -1;
	}
	ENNTRACE("bind ok !\r\n");

	if(-1 == listen(ListeningSocket, MAXCLINE))
	{
	    ENNTRACE("listen fail !\r\n");
	    return -1;
	}
	ENNTRACE("listen ok\r\n");

	conn_amount =0;
	sin_size = sizeof(c_add);
	MaxSock = ListeningSocket;
	//maxfd = MAX(maxfd, ListeningSocket+1);

	while(1)
	{
		//初始化文件描述符集合
	    FD_ZERO(&fdsr); //清除描述符集
	    FD_SET(ListeningSocket, &fdsr); //把sock_fd加入描述符集

		//超时的设定
	    tv.tv_sec = 0;
	    tv.tv_usec = 10000;

		//添加活动的连接
	    for(i=0; i<MAXCLINE; i++) 
	    {
			if(0 != fd[i])
			{
				FD_SET(fd[i], &fdsr);
			}
	    }
	    
		//如果文件描述符中有连接请求 会做相应的处理，实现I/O的复用 多用户的连接通讯
		ret = select((MaxSock+1), &fdsr, NULL, NULL, &tv);
		if(ret < 0) //没有找到有效的连接 失败
		{
			ENNTRACE("	[%s %d select error!!]\r\n",__FUNCTION__, __LINE__);
			break;
		}
		else if(0 == ret)// 指定的时间到，
		{
			//ENNTRACE("	[%s %d timeout!]\r\n",__FUNCTION__, __LINE__);
			continue;
		}
		ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

		//循环判断有效的连接是否有数据到达
		for(i=0; i<conn_amount; i++)
		{
			if(FD_ISSET(fd[i], &fdsr))
			{
				memset((void *)buf, 0, sizeof(buf));
				RecvLen = recv(fd[i], buf, sizeof(buf), 0);
				if(RecvLen <= 0) //客户端连接关闭，清除文件描述符集中的相应的位
				{
					//ENNModBus_Tcp_Del_List(fd[i]);
					
					ENNTRACE("client[%d] close\n",i);
					ENNSock_SocketClose(fd[i]);
					FD_CLR(fd[i],&fdsr);
					fd[i]=0;
					conn_amount--;
				}
				else 	//否则有相应的数据发送过来 ，进行相应的处理
				{
					//缓存接收到的数据
					if(RecvLen < TCP_BUF_SIZE)
					memset(&buf[RecvLen],'\0',1);
					ENNTRACE("client[%d] send:%s\n",i,buf);

					//ENNModBus_TCP_Update_Data(fd[i], buf, RecvLen);
				}
			}
		}

		if(FD_ISSET(ListeningSocket, &fdsr))
		{
			NewConnection = accept(ListeningSocket,(struct sockaddr *)(&c_add), &sin_size);
			if(-1 == NewConnection)
			{
				ENNTRACE("	[%s %d accept error!]\r\n",__FUNCTION__, __LINE__);
				continue;
			}
			ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);

			//ret = ENNModBus_Tcp_Add_List(NewConnection);

			//添加新的fd 到数组中 判断有效的连接数是否小于最大的连接数，如果小于的话，就把新的连接套接字加入集合
			if(conn_amount < MAXCLINE)
			{
				for(i=0; i< MAXCLINE; i++)
				{
					if(0 == fd[i])
					{
						fd[i] = NewConnection;
						break;
					}
				}
				conn_amount++;
				
				//ENNTRACE("new connection client[%d]%s:%d\n",conn_amount,ntoa(c_add.sin_addr),ntohs(c_add.sin_port));
				if(NewConnection > MaxSock)
				{
					MaxSock = NewConnection;
				}
			}
			else
			{
				ENNTRACE("max connections arrive ,exit\n");
				send(NewConnection,"bye",4,0);
				close(NewConnection);
				continue;
			}
		}

		//showclient();
	}

	for(i=0;i<MAXCLINE; i++)
	{
		if(0 != fd[i])
		{
			ENNSock_SocketClose(fd[i]);
		}
	}

	return -1;

	/*while(1)
	{
		sin_size = sizeof(struct ENNSock_SocketAddr_In);

		NewConnection = ENNSock_Accept(ListeningSocket, (struct ENNSock_SocketAddr *)(&c_add), &sin_size);
		if(-1 == NewConnection)
		{
		    ENNTRACE("accept fail !\r\n");
		    return -1;
		}
		ENNTRACE("accept ok!\r\nServer start get connect from %#x : %#x\r\n",ENNSock_ntohl(c_add.sin_addr.s_addr), ENNSock_ntohs(c_add.sin_port));

		recvfrom()
		if(-1 == write(nfp,"hello,welcome to my server \r\n",32))
		{
		    ENNTRACE("write fail!\r\n");
		    return -1;
		}
		ENNTRACE("write ok!\r\n");
		close(NewConnection);
	}*/
}
#endif

#ifdef __cplusplus
#if __cplusplus
    }
#endif /* __cpluscplus */
#endif /* __cpluscplus */


