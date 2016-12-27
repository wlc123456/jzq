#ifndef _IEC102_PROTOCOL_H
#define _IEC102_PROTOCOL_H


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>


#include "ennAPI.h"
#include "ennDatabase.h"

#include "IEC102_Main.h"


#ifndef MAX_DATA_LEN_102
#define MAX_DATA_LEN_102 280
#endif

#define 	FBF_PKTHDR    0x0001     /* start of record */
#define 	FBF_EOR       0x0002     /* end of record */



/*
 * define function's result 
 */
#define     DPA_SUCCESS          0
#define     DPA_FAIL             -1
#define     DPA_FILE_FAIL        -2
#define     DPA_MALLOC_FAIL      -3
#define 	DPA_PORT_FAIL		 -4
#define     DPA_PARAM_FAIL       -5
#define     DPA_THREAD_FAIL      -6

//DPA_PORT_FAIL
#define 	DPA_SELECT_TIME_OUT  -41
#define 	DPA_SELECT_FAIL      -42
#define 	DPA_RECEIVE_FAIL     -43
#define 	DPA_SEND_FAIL        -44
#define		DPA_CLOSE_FAIL		 -45
#define		DPA_SOCKET_FAIL		 -46
#define		DPA_ACCEPT_FAIL		 -47



/****************************************************************************************/

/* **** Frame Defines ***************************************************************** */ 
#define FT12_FRAMESIZE       261
#define FT12_LDATASIZE       255
#define FT12_UDATASIZE       246
#define FT12_METERSSIZE      239


/* *********** Format FT 1.2 defines ************************************* */
#define FT12_VLF_START      0x68   /* FT 1.2 variable length frame start */
#define FT12_VLF_END        0x16   /* FT 1.2 variable length frame end */
#define FT12_FLF_START      0x10   /* FT 1.2 fixed length frame start */
#define FT12_FLF_END        0x16   /* FT 1.2 fixed length frame end */
#define FT12_SCC_1          0xE5   /* FT 1.2 single control character 1 */
#define FT12_SCC_2          0xA2   /* FT 1.2 single control character 2 */

#define FT12_FLF_SIZE       3      /* FT 1.2 fixed length frame data size */

#define FT12_ADDR_SIZE      2      /* FT 1.2 address size */

#define FT12_VLF_ASDU_START_POS  1 /* FT 1.2 ASDU start position in a frame */
#define FT12_FLF_ASDU_START_POS  4 /* FT 1.2 ASDU start position in a frame */


/* **** Services Defines ************************************************************** */
/* IEC870-5-2 Link Control Functions */

/* Balanced and unbalanced transmission - Primary station */
#define DLCF_PSC_RESLINK  0 /* Reset of remote link        S/C exp  FCV=0 */
#define DLCF_PSC_RESPROC  1 /* Reset of user process       S/C exp  FCV=0 */
#define DLCF_PSC_TEST     2 /* Test function for link      S/C exp  FCV=1 */
#define DLCF_PSC_DATA     3 /* Send User data              S/C exp  FCV=1 */
#define DLCF_PSN_DATA     4 /* Send User data              S/N exp  FCV=0  */
#define DLCF_PRR_ACCESS   8 /* Request for access demand   R/R exp  FCV=0  */
#define DLCF_PRR_LINKSTA  9 /* Request for link status     R/R exp  FCV=0  */
#define DLCF_PRR_DATA1   10 /* Request user data class 1   R/R exp  FCV=1  */
#define DLCF_PRR_DATA2   11 /* Request user data class 2   R/R exp  FCV=1  */

/* Unbalanced transmission - Secondary station */
#define DLCF_SC_ACK       0 /* ACK-Positive acknowledge          CONFIRM  */
#define DLCF_SC_NACK      1 /* NACK-Negative acknowledge         CONFIRM */
#define DLCF_SR_DATA      8 /* User data                         RESPOND */
#define DLCF_SR_NACK      9 /* NACK-Data not available           RESPOND */
#define DLCF_SR_STAT     11 /* Status of link or access demand   RESPOND */



/* *********************************************************************** */
/* Control field of IEC870-5-2 link layer data */
/* IEC870-5-2 Link Control Bits Masks */
#define DLC_FUNC       0x0F /* Function code */
#define DLC_FCV_DFC    0x10 /* Frame Count Valid / Data Flow Control */
#define DLC_FCB_ACD    0x20 /* Frame Count Bit / Access Demand */
#define DLC_PRM        0x40 /* Primary station  */
#define DLC_RES_PRM    0x80 /* Reserved / Direction */

/* **** Link Layer's Primitives ******************************************************* */
/* CONFIRM nad INDICATE Primitve Signals */
#define FT12_LL_CONFIRM_ACK         1
#define FT12_LL_CONFIRM_NAK         2
#define FT12_LL_INDICATE            3


/* **** Link Layer's Internals ******************************************************** */
/* Station */
#define FT12_LL_STATION_SEC         0
#define FT12_LL_STATION_PRI         1

/* Frame Count Bit - FCB */
#define FT12_FCB_MAX_STATIONS       16
#define FT12_FCB_RESET_STATE        0xFFFF
#define FT12_FCB_INVALID_STATION    0xFFFF


/* Errors  */
#define FT12_LL_ERROR               251
#define FT12_LL_ERROR_CTRLFUNCTION  252
#define FT12_LL_ERROR_STATION       253
#define FT12_LL_ERROR_ADDRESS       254
#define FT12_LL_ERROR_FRAME         255

/* ***** Process Defines ************************************************************** */
/**  Defines for Type Identification  **/

#define IEC870_TYPE_NONE            0       /* Not defined */
#define IEC870_TYPE_M_SP_TA_2       1       /* Single-point information with time tag */
#define IEC870_TYPE_M_IT_TA_2       2       /* Accounting integrated totals, four octets each  */
#define IEC870_TYPE_M_IT_TB_2       3       /* Accounting integrated totals, three octets each */
#define IEC870_TYPE_M_IT_TC_2       4       /* Accounting integrated totals, two octets each */
#define IEC870_TYPE_M_IT_TD_2       5       /* Periodically reset accounting integrated totals, four octets each */
#define IEC870_TYPE_M_IT_TE_2       6       /* Periodically reset accounting integrated totals, three octets each */
#define IEC870_TYPE_M_IT_TF_2       7       /* Periodically reset accounting integrated totals, two octets each */
#define IEC870_TYPE_M_IT_TG_2       8       /* Operational integrated totals, four octets each  */
#define IEC870_TYPE_M_IT_TH_2       9       /* Operational integrated totals, three octets each */
#define IEC870_TYPE_M_IT_TI_2       10      /* Operational integrated totals, two octets each */
#define IEC870_TYPE_M_IT_TK_2       11      /* Periodically reset operational integrated totals, four octets each */
#define IEC870_TYPE_M_IT_TL_2       12      /* Periodically reset operational integrated totals, three octets each */
#define IEC870_TYPE_M_IT_TM_2       13      /* Periodically reset operational integrated totals, two octets each */

/* <14..69> :=  reserved for further compatible definitions */


#define IEC870_TYPE_M_EI_NA_2       70      /* End of initialization*/
#define IEC870_TYPE_P_MP_NA_2       71      /* Manafacturer and product specification of integrated total DTE */
#define IEC870_TYPE_M_TI_TA_2       72      /* Current system time of integrated total DTE */

/* <73..99> :=  reserved for further compatible definitions */

#define IEC870_TYPE_C_RD_NA_2       100     /* Read manafacturer an product specification */
#define IEC870_TYPE_C_SP_NA_2       101     /* Read record of single-point information with time tag */
#define IEC870_TYPE_C_SP_NB_2       102     /* Read record of single-point information with time tag of 
                                             * a selected time range */
#define IEC870_TYPE_C_TI_NA_2       103     /* Read current system time of integrated total DTE */
#define IEC870_TYPE_C_CI_NA_2       104     /* Read accounting integrated totals of the oldest integration period */
#define IEC870_TYPE_C_CI_NB_2       105     /* Read accounting integrated totals of the oldest 
                                             * integration period and of selected range of addresses */
#define IEC870_TYPE_C_CI_NC_2       106     /* Read accounting integrated totals of the specific 
                                             * past integration period */
#define IEC870_TYPE_C_CI_ND_2       107     /* Read accounting integrated totals of the specific 
                                             * integration period and of selected range of addresses */
#define IEC870_TYPE_C_CI_NE_2       108     /* Read periodically reset accounting integrated 
                                             * totals of the oldest integration period */
#define IEC870_TYPE_C_CI_NF_2       109     /* Read periodically reset accounting integrated totals of the oldest 
                                             * integration period and of selected range of addresses */
#define IEC870_TYPE_C_CI_NG_2       110     /* Read periodically reset accounting integrated 
                                             * totals of the specific past integration period */
#define IEC870_TYPE_C_CI_NH_2       111     /* Read periodically reset accounting integrated totals of 
                                             * the specific integration period and of selected range of addresses*/
#define IEC870_TYPE_C_CI_NJ_2       112     /* Read operational integrated totals of the oldest 
                                             * integration period */
#define IEC870_TYPE_C_CI_NK_2       113     /* Read operational integrated totals of the oldest 
                                             * integration period and of selected range of addresses */
#define IEC870_TYPE_C_CI_NL_2       114     /* Read operational integrated totals of the 
                                             * specific past integration period */
#define IEC870_TYPE_C_CI_NM_2       115     /* Read operational integrated totals of the 
                                             * specific integration period and of selected range of addresses*/
#define IEC870_TYPE_C_CI_NN_2       116     /* Read periodically reset operational integrated 
                                             * totals integrated totals of the oldest integration period */
#define IEC870_TYPE_C_CI_NO_2       117     /* Read periodically reset operational integrated 
                                             * totals integrated totals of the oldest 
                                             * integration period and of selected range of addresses*/
#define IEC870_TYPE_C_CI_NP_2       118     /* Read periodically reset operational integrated 
                                             * totals integrated totals of the specific past integration period */
#define IEC870_TYPE_C_CI_NQ_2       119     /* Read periodically reset operational integrated 
                                             * totals integrated totals of the specific 
                                             * integration period and of selected range of addresses */
#define IEC870_TYPE_C_CI_NR_2       120     /* Read accounting integrated totals of selected 
                                             * time range and of a selected range of addresses  */
#define IEC870_TYPE_C_CI_NS_2       121     /* Read periodically reset accounting integrated 
                                             * totals of selected time range and of a selected range of addresses */
#define IEC870_TYPE_C_CI_NT_2       122     /* Read operational integrated totals of selected 
                                             * time range and of a selected range of addresses  */
#define IEC870_TYPE_C_CI_NU_2       123     /* Read periodically reset operational integrated 
                                             * totals of selected time range and of a selected range of addresses */

/* <12..127> := reserved for further compatible definitions */
#define IEC870_TYPE_EXT             128     /* User specific ASDU-s in range 128-255 */


/**  Defines for Cause of Transmision    **/

/* <0..2> := not used */

#define IEC870_CAUSE_NONE               0   /* Not in use  */
#define IEC870_CAUSE_SPONT              3   /* Spontaneus */
#define IEC870_CAUSE_INIT               4   /* Initialized */
#define IEC870_CAUSE_REQ                5   /* Request or requested */
#define IEC870_CAUSE_ACTIV              6   /* Activation */
#define IEC870_CAUSE_ACTIV_CON          7   /* Activation confirmation */
#define IEC870_CAUSE_DEACTIV            8   /* Deactivation */
#define IEC870_CAUSE_DEACTIV_CON        9   /* Deactivation conformation */
#define IEC870_CAUSE_ACTIV_TERM         10  /* Activation termination */

/* <11..12> := not used */

#define IEC870_CAUSE_REC_NAVAILABLE     13  /* Requsted data record not available */
#define IEC870_CAUSE_TYPE_NAVAILABLE    14  /* Requsted ASDU-type not available */
#define IEC870_CAUSE_RECNO_NKNOWN       15  /* Record number in the ASDU sent by the controlling 
                                                                           * station is not known */
#define IEC870_CAUSE_ADDR_NKNOWEN       16  /* Address specification in the ASDU sent by the 
                                                                           * controlling station is not known */
#define IEC870_CAUSE_OBJ_NAVAILABLE     17  /* Requested information object not available */
#define IEC870_CAUSE_PER_NAVAILABLE     18  /* Requested integration period not available */

/* <19>     := reserved for further compatible definitions  */
/* <20..41> := not used                                     */
/* <42..47> := reserved for further compatible definitions  */
/* <48..63> := for special use (private range)              */


/***********************************/
/**  Defines for Record address   **/

#define IEC870_RECADDR_DEFAULT              0       /* Default */
#define IEC870_RECADDR_IT_SAC_PER           1       /* Record address o integrated totals from 
                                                     * the start of the accounting period*/
                                                                      
/* <2..10> := reserved for further compatible definitions */

#define IEC870_RECADDR_IT_PER_1             11      /* Record address o integrated totals integration period 1 */
#define IEC870_RECADDR_IT_PER_2             12      /* Record address o integrated totals integration period 2 */
#define IEC870_RECADDR_IT_PER_3             13      /* Record address o integrated totals integration period 3 */



/* ***** Constants ******************************************************************** */
#define	IEC870_5_102_UDATASIZE      246 /*max length of one frame*/

#define IEC870_5_102_TIMEA	        0
#define IEC870_5_102_TIMEB	        1

#define IEC870_5_102_SIGNATURE_ON   1   /* 1-signature on, 0-signature off */

#define IEC870_DEFAULT_ASSODOC_DATE     0x89
#define IEC870_DEFAULT_MANUFAC_CODE     0x12
#define IEC870_DEFAULT_PRODUCT_CODE     0xfffefffe

#define IEC870_PARAM_SPI_ENABLE         0x0001
#define IEC870_PARAM_TX_LOCAL_ACK       0x0002

/**************************/
/**  Defines for Time A  **/

#define IEC870_TIMEA_SIZE                   5

/**************************/
/**  Defines for Time B  **/

#define IEC870_TIMEB_SIZE                   7

/* *********** Application-Service-Data-Unit Sizes *********************** */

#define IEC870_5_102_DUI_SIZE   6

#define IEC870_SIZE_NONE        0       
#define IEC870_SIZE_M_SP_TA_2   0      
#define IEC870_SIZE_M_IT_TA_2   0       
#define IEC870_SIZE_M_IT_TB_2   0      
#define IEC870_SIZE_M_IT_TC_2   0       
#define IEC870_SIZE_M_IT_TD_2   0       
#define IEC870_SIZE_M_IT_TE_2   0       
#define IEC870_SIZE_M_IT_TF_2   0       
#define IEC870_SIZE_M_IT_TG_2   0       
#define IEC870_SIZE_M_IT_TH_2   0       
#define IEC870_SIZE_M_IT_TI_2   0      
#define IEC870_SIZE_M_IT_TK_2   0      
#define IEC870_SIZE_M_IT_TL_2   0      
#define IEC870_SIZE_M_IT_TM_2   0      

#define IEC870_SIZE_M_EI_NA_2   2  
#define IEC870_SIZE_P_MP_NA_2   6   
#define	IEC870_SIZE_M_TI_TA_2	7

#define IEC870_SIZE_C_RD_NA_2   0    
#define IEC870_SIZE_C_SP_NA_2   0    
#define IEC870_SIZE_C_SP_NB_2   10    
#define IEC870_SIZE_C_TI_NA_2   0     
#define IEC870_SIZE_C_CI_NA_2   0     
#define IEC870_SIZE_C_CI_NB_2   2    
#define IEC870_SIZE_C_CI_NC_2   5     
#define IEC870_SIZE_C_CI_ND_2   7     
#define IEC870_SIZE_C_CI_NE_2   0     
#define IEC870_SIZE_C_CI_NF_2   2     
#define IEC870_SIZE_C_CI_NG_2   5     
#define IEC870_SIZE_C_CI_NH_2   7    
#define IEC870_SIZE_C_CI_NJ_2   0     
#define IEC870_SIZE_C_CI_NK_2   2     
#define IEC870_SIZE_C_CI_NL_2   5    
#define IEC870_SIZE_C_CI_NM_2   7     
#define IEC870_SIZE_C_CI_NN_2   0     
#define IEC870_SIZE_C_CI_NO_2   2    
#define IEC870_SIZE_C_CI_NP_2   5    
#define IEC870_SIZE_C_CI_NQ_2   7    
#define IEC870_SIZE_C_CI_NR_2   12     
#define IEC870_SIZE_C_CI_NS_2   12    
#define IEC870_SIZE_C_CI_NT_2   12    
#define IEC870_SIZE_C_CI_NU_2   12   


/* ****************** Cause of initialization defines ********************************* */
#define	LOCAL_PWS_ON            0x00
#define LOCAL_MAN_RESET         0x01
#define REMOTE_RESET            0x02

#define NO_CHG_OF_PARAMETERS    0x00
#define CHG_OF_PARAMETERS       0x01

#define DLCF_SC_ACK_E5              0xE5    /* added by Betty Xu 2006-04-24 */

#define MASTER_TYPE_IEC_102 0
#define TIMEDIFF_SYNCH_LIMIT 40 /* Syncronization limit in ms	*/	


#define LANJIER_IEC870_C_SYN_TA_2 72        
#define IEC870_C_SYN_TA_2         128

/*
CHINA
*/
#define IEC870_CAUSE_CHINA_SYNCH        48
#define IEC870_LANGJIER_SYNC_UPTIME     30  /* for time synchronization,added by Betty Xu 2006-04-30 */

/* IT ASDU-s with same timing request information */
#define IEC870_OLDEST               0
#define IEC870_OLDEST_AND_RANGE     1
#define IEC870_PAST                 2
#define IEC870_PAST_AND_RANGE       3
#define IEC870_TIME_AND_RANGE       4


/* profile/channel status */
#define 	DATA_POWER_FAIL         0x0001  /* Power Fail                        */
#define 	DATA_POWER_ON           0x0002  /* Power On                          */
#define 	DATA_TIMESYNC_LONGPER   0x0004  /* Time Synchronize - Longer Period  */
#define 	DATA_TIMESYNC_SHORTPER  0x0008  /* Time Synchronize - Shorter Period */
#define 	DATA_DATA_ADJUST        0x0010  /* Data Adjusted                     */
#define 	DATA_DATA_OVERRUN       0x0020  /* Data Overrun                      */
#define 	DATA_DATA_INVALID       0x0040  /* Data Invalid                      */
#define		DATA_DATA_NODATA		0x0080  /* Data NoData, Example Profile Data Empty in Meter*/
#define		DATA_DATA_VALFAIL		0x0100	/* Data Validation Failed */
#define 	DATA_SYS_ERROR          0x8000  /* System Error Occured              */



/*读负荷曲线数据的结构*/
//input param
typedef struct _DATA_PROFILE_PARAM
{
	unsigned char	prof;		/* 上送给主站的逻辑负荷曲线号,从0开始 */
	unsigned char	padding[3];	/* 备用 */
	unsigned short  from;	    /* 开始通道号,从1开始 */
	unsigned short  to;			/* 结束通道号 */
	unsigned long   stamp;		/* 时间点 */
}DATA_PROFILE_PARAM;

//output data
typedef struct _DATA_PROFILE_REC
{
	unsigned short	chn_no;		/*方便DPA/WEB作为信息体/通道地址输出显示*/
	unsigned short	status;
	long			value;
}DATA_PROFILE_REC;

typedef struct _DATA_PROFILE_RESULTS
{
	unsigned long		stamp;			/* 负荷曲线数据时间 */
	unsigned long		period;			/* 负荷曲线积分周期*/
	unsigned short	    pf_status;		/* 负荷曲线状态 */
	unsigned short	    chn_nums;		/* 负荷曲线数据通道数 */
	DATA_PROFILE_REC	*results;  		/* 负荷曲线数据, 由调用者初始化此接口 */
}DATA_PROFILE_RESULTS;

typedef struct _T_READ_IT_PARAM
{
    UINT16                       itfrom;
    UINT16                       itto;
    UINT32                       stamp;
    UINT32                       *pstamp;
    DATA_PROFILE_RESULTS         	*rbuf;
    UINT16                       *pstatus;
    UINT8                        count;
}T_READ_IT_PARAM;



typedef union  u_word
{
	UINT16 w;
	struct { UINT8 lb, hb; } wb;
	UINT8 war[2];
} U_WORD;


/** cause of initilization - COI    **/
struct _T_IEC102_COI
{
    UINT8    cause           :7;         
    UINT8    parefect        :1;         
}__attribute__((packed));
typedef struct _T_IEC102_COI T_IEC102_COI;

/******************************/
/** Integradted totals - IT  **/
/* 
Sequence notation 
*/
struct _T_IEC102_IT_SEQ   
{  
    UINT8    seq:5;
    UINT8    cy:1;
    UINT8    ca:1;
    UINT8    iv:1;
}__attribute__((packed));
typedef struct _T_IEC102_IT_SEQ T_IEC102_IT_SEQ;



/* ************************************************************************************ */
/* *****                  Data Unit identifier - DUI                              ***** */
/* ************************************************************************************ */

/* Definitions of DUI fields */
struct _T_IEC102_VS_QUALIF_BF
{
    UINT8    num :7;                         /* Number of info. objects or elements     */
    UINT8    sq  :1;                         /* Single/Sequence                         */ 
}__attribute__((packed));
typedef struct _T_IEC102_VS_QUALIF_BF T_IEC102_vs_QUALIF_BF;

/*cause of transmition*/
struct _T_IEC102_CAUSE_BF
{
    UINT8    cause   :6;
    UINT8    confirm :1;                     /* <0>:= Positive,  <1>:= Negative confirm */
    UINT8    test    :1;                     /* Test. <0>:= No test, <1>:= Test         */
}__attribute__((packed));
typedef struct _T_IEC102_CAUSE_BF T_IEC102_CAUSE_BF;


union _T_IEC102_VS_QUALIFIER
{
    T_IEC102_vs_QUALIF_BF   bf;
    UINT8                b;
}__attribute__((packed));

typedef union _T_IEC102_VS_QUALIFIER U_IEC102_VS_QUALIFIER;

union _T_IEC102_CAUSE
{
    T_IEC102_CAUSE_BF       bf;
    UINT8                b;
}__attribute__((packed));

typedef union _T_IEC102_CAUSE U_IEC102_CAUSE;

/* the control field and address field */
struct _T_IEC102_FLF_FIELDS
{
    U_IEC102_CAUSE          ctrl; 
    UINT8                addrhi; 
    UINT8                addrlo;    
}__attribute__((packed));

typedef struct _T_IEC102_FLF_FIELDS T_IEC102_FLF_FIELDS;


/* ***** Definition of DUI ***** */
struct _T_IEC102_DUI
{
    UINT8                        type;       /* Type identification                          */
    U_IEC102_VS_QUALIFIER           vsq;        /* Variable structure qualifier               */
    U_IEC102_CAUSE                  cause;      /* Cause of transmition                       */
    UINT8                        dte_addr_lo;/* Address of integrated total-DTE low  */
    UINT8                        dte_addr_hi;/* Address of integrated total-DTE high */
    UINT8                        rec_addr;   /* Record address                               */
}__attribute__((packed));
typedef struct _T_IEC102_DUI T_IEC102_DUI;


/* ************************ IEC870_5_102 data frame ****************************************** */
struct _T_IEC102_DATA
{
    UINT8                    ctrl;       
    UINT8                    addrlo;
    UINT8                    addrhi;     
    UINT8                    type;       /* Type identification */
    U_IEC102_VS_QUALIFIER       vsq;        /* Variable structure qualifier */
    U_IEC102_CAUSE              cause;      /* Cause of transmition  */
    UINT8                    dte_addr_lo;/* Address of integreted total-DTE low */
    UINT8                    dte_addr_hi;/* Address of integreted total-DTE high */
    UINT8                    rec_addr;   /* Record address                       */
    UINT8                    udata[IEC870_5_102_UDATASIZE];/*user data*/
}__attribute__((packed));
typedef struct _T_IEC102_DATA T_IEC102_DATA;


/* ***** Functions ******************************************************************** */

/*************************************/
/** Time A		  minutes to year  **/
struct _T_IEC102_TIMEA
{
	UINT8	minute		 :6;
	UINT8	tis 		 :1;
	UINT8	iv			 :1;
	UINT8	hour		 :5;
	UINT8	res1		 :2;
	UINT8	summer_time  :1;
	UINT8	day_of_month :5;
	UINT8	day_of_week  :3;
	UINT8	month		 :4;
	UINT8	eti 		 :2;
	UINT8	pti 		 :2;
	UINT8	year		 :7;
	UINT8	res2		 :1;
}__attribute__((packed));
typedef struct _T_IEC102_TIMEA T_IEC102_TIMEA;

/*************************************/
/** Time B	   miliseconds to year		**/
struct _T_IEC102_TIMEB
{
	UINT16 millisecond  :10;
	UINT16	 second 	  :6;
	UINT16	 minute 	  :6;
	UINT16	 tis		  :1;
	UINT16	 iv 		  :1;
	UINT16	 hour		  :5;
	UINT16	 res1		  :2;
	UINT16	 summer_time  :1;
	UINT16	 day_of_month :5;
	UINT16	 day_of_week  :3;
	UINT16	 month		  :4;
	UINT16	 eti		  :2;
	UINT16	 pti		  :2;
	UINT16	 year		  :7;
	UINT16	 res2		  :1;
}__attribute__((packed));
typedef struct _T_IEC102_TIMEB T_IEC102_TIMEB;


/*************************************/
/* ASDU structures of primary station */

/* Data type ident is 102, means read information with time tag of a selected time range */
struct _T_IEC102_INF_OBJ_102
{
	T_IEC102_TIMEA	taf;
	T_IEC102_TIMEA	tat;
}__attribute__((packed));
typedef struct _T_IEC102_INF_OBJ_102 T_IEC102_INF_OBJ_102;

/* Data type ident is 106, means read the specific past period */
struct _T_IEC102_INF_OBJ_106
{
	T_IEC102_TIMEA	ta;
}__attribute__((packed));
typedef struct _T_IEC102_INF_OBJ_106 T_IEC102_INF_OBJ_106;

/* Data type ident is 102, means read the specific 
 * integration period and of selected range of addresses 
 */
struct _T_IEC102_INF_OBJ_107
{
	UINT8			itfrom;
	UINT8			itto;
	T_IEC102_TIMEA		ta;
}__attribute__((packed));
typedef struct _T_IEC102_INF_OBJ_107 T_IEC102_INF_OBJ_107;

/* Data type ident is 102, means read the oldest 
 * integration period and of selected range of addresses 
 */
struct _T_IEC102_INF_OBJ_105
{
	UINT8			itfrom;
	UINT8			itto;
}__attribute__((packed));
typedef struct _T_IEC102_INF_OBJ_105 T_IEC102_INF_OBJ_105;

/* Data type ident is 102, means read of selected 
 * time range and of a selected range of addresses 
 */
struct _T_IEC102_INF_OBJ_120
{
	UINT8			itfrom;
	UINT8			itto;
	T_IEC102_TIMEA		taf;
	T_IEC102_TIMEA		tat;
}__attribute__((packed));
typedef struct _T_IEC102_INF_OBJ_120 T_IEC102_INF_OBJ_120;


union _U_IEC102_INFOOBJ
{
    T_IEC102_INF_OBJ_102    io102;
    T_IEC102_INF_OBJ_106    io106;
    T_IEC102_INF_OBJ_107    io107;
    T_IEC102_INF_OBJ_105    io105;
    T_IEC102_INF_OBJ_120    io120;
    UINT8                synchtime[IEC870_TIMEB_SIZE];
}__attribute__((packed));
typedef union _U_IEC102_INFOOBJ U_IEC102_INFOOBJ; 

/*Iec102 asdu define*/
struct _T_IEC102_ASDU
{
	T_IEC102_DUI				dui;
	U_IEC102_INFOOBJ			infobj;
}__attribute__((packed));
typedef struct _T_IEC102_ASDU T_IEC102_ASDU;



/************************* Process structures ****************************************** */
/* Implements 102 communication sequence and extended communication in process */
struct _T_IEC102_COMM_SEQ      
{  
    UINT8	cseq:1;
    UINT8    extcom:1;
    UINT8    class1ind:1;
    UINT8    class1req:1;
    UINT8    initind:1;
    UINT8    commrun:1;
    UINT8    eventind:1;
    UINT8    req_all:1;
}__attribute__((packed));
typedef struct _T_IEC102_COMM_SEQ T_IEC102_COMM_SEQ;


/*******************add by hyptek********************************/

typedef struct _T_IEC870ATTR   
{
    UINT8 ldid;                                /* Link Data Ident                   */
    UINT8 udid;                                /* User Data Ident                   */
    UINT16 laddr;                                  /* Link address                      */
}T_IEC870ATTR;


typedef struct _T_IEC102_ATTR   
{
    UINT8                ldid;           /* Link Data Ident   */
    UINT8                udid;           /* User Data Ident  */
    UINT16		     laddr;          /* Link address  */
    UINT32		lastsstamp;     /* Stamp of last sent profile data   */
	
   T_IEC102_COMM_SEQ       iec870seq;      /* Indicates communication sequence in progress */
    T_IEC102_ASDU    		lastasdu;       /* Last Data Request asdu */
    T_IEC102_IT_SEQ         status;         /* For Sequence Number */
}T_IEC102_ATTR;


typedef struct _T_IEC102_SEQUENCE                  /* Data Identification               */
{                 
	UINT8 did;                             /* Data  Id  */
	UINT8 lctrl;                           /* Last received control field of FT12 */
	union 
	{
	   T_IEC870ATTR 		  iec870;
	   T_IEC102_ATTR		  iec870_102attr;
	}attr;
}T_IEC102_SEQUENCE;


typedef struct _IEC102_FRAME_BUF
{
    UINT8 databuf[MAX_DATA_LEN_102];
    UINT16 datalen;
    UINT16 dataflags;
}T_IEC102_FRAME_BUF;


/* communication port, include serial + connect tcp */
typedef struct _DPA_CONN_CONTEXT
{
	//struct _DPA_PORT_DATA  *pPort;
	
	T_IEC102_FRAME_BUF			rxframe;
	T_IEC102_FRAME_BUF			txframe;
	T_IEC102_FRAME_BUF			lasttxframe;

	UINT32		rxtime;					/* lastest receive time */
	char		masterIP[16];				/* display in io fifo */
	UINT8	initflag;
	UINT8    sync_time_flag;         /* whether sync systemtime, 1='true';0='false' */

	UINT16	orig_itfrom;		/*recored original infounit start-id*/

	UINT8	ret_immedt; 		/*indicate if return immediatly*/
	UINT8	ioa_not_finish; 	/*indicate if ioa is finished*/
	UINT32 	data_period;            /* data real time period(s) */

	DATA_PROFILE_RESULTS  	prof_data;              /* profile data, need malloc 'results' */
	
	T_IEC102_SEQUENCE       seq;    
}DPA_CONN_CONTEXT;



#endif


