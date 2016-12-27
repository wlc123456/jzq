
#ifndef _MODBUSTCP_H_
#define _MODBUSTCP_H_


#include "ennType.h"
#include "IEC102_Main.h"



#ifdef __cplusplus
extern "C"
{
#endif

#define TCP_BUF_SIZE 260
#define MAXCLINE 8

#define MB_FUNC_NONE							( 0x00 )
#define MB_FUNC_READ_COILS						( 0x01 )
#define MB_FUNC_READ_DISCRETE_INPUTS			( 0x02 )
#define MB_FUNC_READ_HOLDING_REGISTER			( 0x03 )
#define MB_FUNC_READ_INPUT_REGISTER				( 0x04 )
#define MB_FUNC_WRITE_SINGLE_COIL				( 0x05 )
#define MB_FUNC_WRITE_SINGLE_REGISTER			( 0x06 )
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS        ( 0x10 )
#define MB_FUNC_READ_HISTORY_DATA              	( 0x18 )
#define MB_FUNC_ERROR              				( 0x80 )
//#define MB_FUNC_READ_INPUT_REGISTER				( 4 )
//#define MB_FUNC_WRITE_REGISTER					( 6 )
//#define MB_FUNC_DIAG_READ_EXCEPTION				( 7 )
//#define MB_FUNC_DIAG_DIAGNOSTIC					( 8 )
//#define MB_FUNC_DIAG_GET_COM_EVENT_CNT          ( 11 )
//#define MB_FUNC_DIAG_GET_COM_EVENT_LOG          ( 12 )
//#define MB_FUNC_WRITE_MULTIPLE_COILS			( 15 )
//#define MB_FUNC_OTHER_REPORT_SLAVEID			( 17 )
//#define MB_FUNC_READWRITE_MULTIPLE_REGISTERS  	( 23 )


#if 0

typedef struct tag_TCP_DATA_LIST
{
	ENN_U32	u32Socket;
	//ENN_U32	u32IP;
	//ENN_U16	u16Port;
	ENN_U16	u16Len;
	ENN_U16	u16Pos;
	ENN_U8	aData[TCP_BUF_SIZE];
	struct tag_TCP_DATA_LIST *next;
}TCP_DATA_LIST;


typedef enum tag_UART_STATUS
{
	UART_IDEL,
	UART_BUSY,
}UART_STATUS;

typedef enum tag_UART_CHANNEL
{
	UART_CHANNEL_0 = 0,
	UART_CHANNEL_1,
	UART_CHANNEL_2,
	UART_CHANNEL_3,
	UART_CHANNEL_4,
	UART_CHANNEL_5,
	UART_CHANNEL_6,
	UART_CHANNEL_7,
}UART_CHANNEL;


typedef struct dataQ_frame
{
	ENN_U8 	*p_buf;            //buf start pointer
    ENN_U8 	*p_data;
    ENN_U32 length;           //len of data
    ENN_U32 	status;
}DATAQ_FRAME;

typedef  struct  modbusMsg
{
	ENN_U32	u32Socket;
    ENN_U16 u16MBTID; //only tcp mode used;
    ENN_U8 	u8MBAddr;
    ENN_U8 	u8MBFunCode;
	ENN_U16 u16RegAddr;
	ENN_U16 u16RegNum;
    //DATAQ_FRAME *mbPDU;
    //TimeoutTimer timeoutTimer;
    struct modbusMsg *next;
}ModbusMsg;

typedef  struct  MBMsg_TCP_HEAD
{
	ENN_U32 u32IdelTime;
	ENN_U32	u32Socket;
    ENN_U16 u16MBTID; //only tcp mode used;
    ENN_U8 	u8MBAddr;
    ENN_U8 	u8MBFunCode;
	ENN_U8 *pData;
	ENN_U16 u16Len;
	ENN_U16 u16RegNum;
	ENN_U8 	u8WaitFlag;
	ENN_U8 	u8UARTChannel;
}MBMsg_TCP_HEAD;

typedef  struct  tag_MODBUS_INFO
{
	//ENN_U8 	u8UARTChannel;
	ENN_U32	u32Socket;
	ENN_U16 u16Offset;
	ENN_U32 u32IdelTime;
}MODBUS_INFO;


typedef struct tag_Slave_List
{
	ENN_U8	u8UART_Channel;
	ENN_U8	u8Slave_Addr;
	ENN_U16	u16Reg_Addr;
	ENN_U16	u16Reg_Num;
	ENN_U16 u16Offset;
	struct tag_Slave_List *next;
}Slave_List;

typedef struct tag_Master_List
{
	ENN_U32	u32Socket;
    ENN_U16 u16MBTID; //only tcp mode used;
    ENN_U8 	u8MBAddr;
    ENN_U8 	u8MBFunCode;
	ENN_U16 u16RegNum;
	ENN_U16 u16Len;
	ENN_U8 *pData;
	Slave_List *pSlave_List;
	struct tag_Master_List *next;
}Master_List;
#endif

typedef struct frame_net_info_
{
	ENN_U32 u32Socket;
	ENN_U32 u32IP;
	ENN_U16 u16Port;
	//INT8U frame_ch; //indicate channel
	//INT8U reserved;
}FRAME_NET_INFO;


typedef struct dataQ_frame
{
	ENN_U8 	 	*p_buf;            //buf start pointer
    	ENN_U8 	 	*p_data;         //数据指针
    	ENN_U32  	length;           //len of data
    	ENN_U32  	status;
	FRAME_NET_INFO frame_net_info;
}DATAQ_FRAME;


#if 0

/***********************************************************/
typedef struct tag_Register_List
{
	ENN_U16 	u16RegAddr;
	ENN_U16 	u16RegNum;
	ENN_U8		aRegType[20];
	struct tag_Register_List *next;
}Register_List;

typedef struct tag_FunCode_List
{
	ENN_U8 	u8MBFunCode;
	Register_List *pRegister_List;
	struct tag_FunCode_List *next;
}FunCode_List;

typedef struct tag_Slave_List
{
	ENN_U8 	u8SlaveAddr;
	ENN_U8 	aFunCode[4];
	ENN_U8 	u8FunCodeNum;
	FunCode_List *pFunCode_List;
	struct tag_Slave_List *next;
}Slave_List;

typedef struct tag_Device_List
{
	ENN_U8	u8UART_Channel;
	ENN_U8	u8Slave_Num;
	Slave_List 	*pSlave_List;
}Device_List;

typedef struct tag_Addr_Map_List
{
	ENN_U16 	u16DevRegAddr;
	//ENN_U16 	u16RegNum;
	ENN_U8		u8UARTChannel;
	ENN_U8 		u8SlaveAddr;
	ENN_U16 	u16SlaveRegAddr;
	ENN_U16 	u16SlaveRegNum;
	struct tag_Addr_Map_List *next;
}Addr_Map_List;

typedef struct tag_Matser_List
{
	ENN_U8 	u8MBFunCode;
	Addr_Map_List *pAddr_Map_List;
	struct tag_Matser_List *next;
}Matser_List;

#endif


#if 0
REGISTER_ADDR=0000
REGISTER_NUM=4
REGISTER_TYPE=工况类型1
REGISTER_ADDR=0004
REGISTER_NUM=4
REGISTER_TYPE=工况类型2
REGISTER_ADDR=0008
REGISTER_NUM=2
REGISTER_TYPE=工况类型3
REGISTER_ADDR=000A
REGISTER_NUM=2
REGISTER_TYPE=工况类型4

FUNCODE_1=2
REGISTER_ADDR		REGISTER_NUM		REGISTER_TYPE
0000			1			0:  1:  2:  3:  4:  5:  6:  7:  
0001			1			0:  1:  2:  3:  4:  5:  6:  7:
0002			1			0:  1:  2:  3:  4:  5:  6:  7:
0003			1			0:  1:  2:  3:  4:  5:  6:  7:

HOST_IP=192.168.1.155
BAUDRATE= 9600
DATABITS= 8
STOPBITS= 1
PARITY= NONE

FUNCODE=1 
REGISTER_START_ADDR=0000

#endif
/***********************************************************/

void Test_Rx();
ENN_ErrorCode_t ENNModBus_TCP_Receive(ENN_S32 socket, ENN_U8 *pbuf, ENN_S32 RecvLen);

ENN_ErrorCode_t ENNModBus_TCP_Send(ENN_S32 socket, ENN_U8 *pbuf, ENN_S32 Len);

ENN_ErrorCode_t ENNModBus_TCP_Server(void);

ENN_ErrorCode_t  ENNAmmeter_Server_Tcp(void);



#ifdef __cplusplus
}

#endif /* __cplusplus */


#endif     /* _CYAPI_H_ */


