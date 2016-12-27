#ifndef _MODBU_IEC102_MAIN_H_
#define _MODBU_IEC102_MAIN_H_




#ifdef __cplusplus
extern "C"
{
#endif



#include <stdlib.h>
#include <stdio.h>

#include "ennAPI.h"
#include "ModBus_Slave_Table.h"
//#include "IEC102protocol.h"
#include "ModBus_TCP.h"



#define 	DPA_IP_ADDRESS_LEN           16
#ifndef MAX_DATA_LEN_102
#define MAX_DATA_LEN_102 280
#endif
#define IEC102_READ_FILE_BUF_LEN 	1024



typedef struct _INFO_UNIT_MAP{
	UINT16	infoNum;	// 信息体 num
	UINT16	u16RegID;	//数据号
	struct _INFO_UNIT_MAP *next;
}INFO_UNIT_MAP;


/* master parameters table */
typedef struct _DPA_MASTER_PARAM
{
	char	    master_name[32];        /* master name */
	char	    master_ip_addr[DPA_IP_ADDRESS_LEN];     /* master IP Address */

	UINT8	master_no;              /* master no. */
	UINT8	sync_time_flag;         /* whether sync systemtime, 1='true';0='false' */
	UINT16	link_addr;              /* link address */

	UINT32   master_timeout;         /* master timeout(ms) of receive master data */
	//u_int32_t   frame_timeout;          /* frame timeout(ms) of receive master data */
	//u_int32_t   sync_time_limit;        /* no sync time less than the value(ms) */
	UINT32 data_period;            /* data real time period(s) */

	INFO_UNIT_MAP *p_info_unit_map;
	
	struct _DPA_MASTER_PARAM *next;
}DPA_MASTER_PARAM;

/*slave parameters table*/
typedef struct _DPA_SLAVE_PARAM
{
	UINT8	master_no;              /* master no. */
	
	//char		slave_ip_addr[DPA_IP_ADDRESS_LEN];	   /* slave IP Address */
	//UINT16	slave_port;
	
	DPA_MASTER_PARAM *master_table;
}DPA_SLAVE_PARAM;

typedef struct _DEVICE_REG_DATA{
	UINT16	u16RegID;   //寄存器ID
	//UINT32  	data;
	UINT8	*pdata;          //寄存器数据
	UINT8	datalen;        //寄存器数据长度
	UINT8 	type; // 0 -> int		1 -> float   // 寄存器数据类型
	UINT8 	valid;	//0 -> invalid 	1->valid   //数据是否有效
	UINT16 	u16SvInterval;     //数据采集周期     
}DEVICE_REG_DATA;

typedef struct _DEVICE_IP_PARAM{
	char		ip[16];
	char		mask[16]; 
	char		gateway[16];
	UINT16	serverport;
}DEVICE_IP_PARAM;



ENN_ErrorCode_t ENNIEC102_Get_DPAMode();
ENN_ErrorCode_t ENNIEC102_Read_MasterInfo();



#ifdef __cplusplus
}

#endif /* __cplusplus */


#endif     /* _CYAPI_H_ */

