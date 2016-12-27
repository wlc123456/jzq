
#ifndef _MODBU_SLAVE_TABLE_H_
#define _MODBU_SLAVE_TABLE_H_


#include "ennType.h"
#include "bacenum.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define MODBUS_STRING_MAX_LEN	20

#define UART_CHANNEL_MAX_NUM		4

#define MODBUS_REG_MAX_NAME		20
#define MODBUS_FUNCODE_MAX_NUM		4
#define MODBUS_READ_FILE_BUF_LEN 	4096
#define MODBUS_REG_ATTR_MAX_LEN 	512

#define MODBUS_PRITY_NONE "NONE"
#define MODBUS_PRITY_ODD "ODD"
#define MODBUS_PRITY_EVEN "EVEN"
#define MODBUS_PRITY_MARK "MARK"
#define MODBUS_PRITY_SPACE "SPACE"

#define PRO_645_MAX_CHANNEL_NUM 	14
/***********************************************************/

typedef enum tag_CHANNEL_STATUS
{
	CHANNEL_IDEL,
	CHANNEL_BUSY,
}CHANNEL_STATUS;

typedef enum tag_MODBUS_CHANNEL
{
	MODBUS_CHANNEL_0 = 0,
	MODBUS_CHANNEL_1,
	MODBUS_CHANNEL_2,
	MODBUS_CHANNEL_3,
	MODBUS_CHANNEL_4,
	MODBUS_CHANNEL_5,
	MODBUS_CHANNEL_6,
	MODBUS_CHANNEL_7,
}MODBUS_CHANNEL;


/*add by shanjianchao 2015-01-22*/
typedef enum tag_Protocol_Type
{
	PROTOCOL_MODBUS = 0,
	PROTOCOL_645_1997,
	PROTOCOL_645_2007,
}Protocol_Type;

typedef enum tag_Data_Type
{
	Data_DECIMAL = 0,
	Data_INT,
}Data_Type;

typedef struct tag_Bacnet_Register_List{
	BACNET_OBJECT_TYPE Object_Type;
	uint32_t Object_Instance;
	BACNET_PROPERTY_ID Object_Property;
	float Present_Value;
	BACNET_ENGINEERING_UNITS Units;
}Bacnet_Register_List;


typedef struct tag_Register_List
{
	ENN_U8		u8Interval;    // 当前寄存器与上一个寄存器的地址间隔
	ENN_U16 	u16RegAddr;   //寄存器 起始地址
	ENN_U16 	u16RegNum;   //寄存器长度
	ENN_CHAR		*pName;  //寄存器的名字
	ENN_CHAR		*pAttr;    //寄存器的属性
	ENN_FLOAT		fRegK;   //K参数值y=kx+d
	ENN_FLOAT		fRegD;    //D参数值
	ENN_U16		u16SvInterval;        //10s,20s   //数据采集周期
	//ENN_U8			u8ByteOrder;//0->'AB'  1->'BA'  2->''
	ENN_CHAR			*pBtOrder;//'AB'  'BA'  '1234'
	ENN_U16		u16RegID;      //寄存器(传感器)ID
	//added for bacnet
	BACNET_OBJECT_TYPE   object_type;  //bacnet对象类型
	ENN_U32	object_instance;                 //bacnet对象实例
	BACNET_ENGINEERING_UNITS   units;   //对应寄存器数据的单位
	
	struct tag_Register_List *next;
}Register_List;

typedef struct tag_Slave_FunCode_List
{
	ENN_U8 	u8MBFunCode;
	ENN_U16 u16Count;     //当前设备下所处功能码的 寄存器总长度(包括间隔距离)(寄存器数量)
	ENN_U16 u16TotalRegNum;   //当前设备下所处功能码寄存器的总长度
	Register_List *pRegister_List;
	struct tag_Slave_FunCode_List *next;
}Slave_FunCode_List;

typedef struct tag_Slave_List
{
	ENN_U8 	u8SlaveAddr;  //设备地址
	ENN_CHAR	*pSlaveName;  //设备名字
	ENN_U8	u8DataFormat;   //数据格式
	ENN_U16	u16Status;    //设备的工作状态 1  代表正常，0 代表不正常
	Slave_FunCode_List *pFunCode_List;  //功能码链表
	struct tag_Slave_List *next;     //指向下一个设备
}Slave_List;

typedef struct tag_Code_645_List
{
	ENN_U32			u32ChannelCode;
	ENN_U16			u16RegNum;
	Data_Type		eData_Type;
	//ENN_S16			s16DCoe;
	//float 			fKCoe;
	ENN_CHAR		*pName;
	ENN_CHAR		*pAttr;

	ENN_FLOAT	fRegK;
	ENN_FLOAT	fRegD;
	ENN_U16		u16SvInterval;
	ENN_U8			u8ByteOrder;//0->'AB'  1->'BA'  2->''
	ENN_U16		u16RegID;
	struct tag_Code_645_List *next;
}Code_645_List;


typedef struct tag_Dev645_List
{
	ENN_CHAR 	ucDevAddr[16];
	ENN_CHAR	*pSlaveName;
	ENN_U16		u16Status;
	ENN_U8		u8ChannelNum;
	Code_645_List *pCode_645_List;
	//ENN_U32		*pChannelCode;
	struct tag_Dev645_List *next;
}Dev645_List;

typedef union tag_Device_Type
{
	Slave_List *pModBus_List;
	Dev645_List *pDev645_List;
}Device_Type;

typedef struct tag_Channel_List
{
	ENN_U8	u8Channel;      //通道号从0 开始
	ENN_U8 	u8Protocol;     //主从设备的通信协议号
	ENN_U8 	u8Status;
	ENN_U8	u8SlaveNum;   //相应通道下的从设备数量
	ENN_U8	u8DataBit;        //串口参数的数据位
	ENN_U8	u8StoptBit;       //串口参数的停止位
	ENN_U8	u8Parity;            //串口参数的奇偶校验位
	ENN_U32	u32BaudRate;     //串口参数的波特率bit/s
	ENN_U32	u32Idletime;        //通道超时等待时间
	//Slave_List 	*pSlaveList;
	Device_Type unDeviceType;
	struct tag_Channel_List *next;
}Channel_List;

typedef struct tag_Addr_Map_List
{
	ENN_U16 	u16DevRegAddr;
	ENN_U16 	u16SlaveRegNum;
	ENN_U8		u8UARTChannel;
	ENN_U8 		u8SlaveAddr;
	ENN_U16 	u16SlaveRegAddr;
	struct tag_Addr_Map_List *next;
}Addr_Map_List;

typedef struct tag_FunCode_Write_List
{
	ENN_U8 		u8MBFunCode;
	ENN_U16 	u16StartAddr;
	ENN_U16 	u16EndAddr;
	Addr_Map_List 	*pAddr_Map_List;
	struct tag_FunCode_Write_List *next;
}FunCode_Write_List;


typedef struct tag_FunCode_List
{
	ENN_U8 	u8MBFunCode;
	ENN_U16 u16StartAddr;   //映射功能码的起始的寄存器地址
	ENN_U16 u16EndAddr;    //映射功能码的结束的寄存器地址实际上保存的是寄存器的总数量
	ENN_U16 Offset[8];           //记录每个一个通道内相同功能码相对于起始位置的偏移量
	ENN_U16 DataLen;           //数据总长度:注意数据长度是寄存器长度的两倍一个长度的寄存器返回两个字节
	ENN_U8 	*pData;                //数据存储空间在ENNModBus_Add_FunCode函数中已经分配空间和清零
	ENN_U8 	*pPos;                 //初始化的值为pData的首地址
	Addr_Map_List 	*pAddr_Map_List;
	struct tag_FunCode_List *next;
}FunCode_List;

typedef struct tag_rlSectionName
{
	ENN_CHAR            *name;
	ENN_CHAR            *param;
	struct tag_rlSectionName   *next;
}rlSectionName;

typedef struct tag_rlSection
{
	ENN_CHAR		*name;
	rlSectionName *firstName;
	struct tag_rlSection   *next;
}rlSection;

typedef struct tag_Dev645_Map
{
	ENN_U16		u16code;
	ENN_CHAR 	strName[50];
}Dev645_Map;

ENN_ErrorCode_t ENNModBus_Read_Configure(void);

#ifdef __cplusplus
}

#endif /* __cplusplus */


#endif     /* _CYAPI_H_ */


