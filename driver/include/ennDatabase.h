#ifndef _ENN_DATABASE_H_
#define _ENN_DATABASE_H_


#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>


#include "ennType.h"
#include "ennDebug.h"
#include "ennOS.h"

//#include "enn_os.h"


typedef struct _REG_DATA_VALUE{
	//UINT32 u32data;
	char * pdata;
	float fdata;
	struct _REG_DATA_VALUE *next;
}REG_DATA_VALUE;

typedef struct _DPA_REG_DATA{
	UINT16  RegID;
	UINT8 type; // 0 -> modbus		1 -> 645
	//UINT8 valid;	//0 -> invalid 	1->valid
	REG_DATA_VALUE *p_Reg_Data_Values;
}DPA_REG_DATA;

typedef union  u_data
{
	UINT32	u32data;
	float		fdata;
} U_DATA; 

typedef struct _POINT_DATA{
	UINT8	type; // 0 -> NULL	; 1 -> UINT32	; 2->float;	3-> datalen>4
	U_DATA	data;
	UINT8	*pdata;	//datalen > 4 bytes
}POINT_DATA;


ENN_ErrorCode_t Enn_Database_init();
ENN_ErrorCode_t Database_insert(char *cdate);
ENN_ErrorCode_t Database_select_period(UINT32 timepoint, UINT16 RegNum, POINT_DATA *result);




#ifdef __cplusplus
}

#endif /* __cplusplus */


#endif     /* _CYAPI_H_ */

