#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */


#include "ennDatabase.h"
#include "sqlite3.h"
#include "cJSON.h"
#include "IEC102_Main.h"


#define DB_Debug 

//#define DB_DATA_INTERVAL 10	//seconed
#define HistoricalData_RETENTION_TIME		30	//delete 30 days ago table

static sqlite3* db = NULL;
char todayTable[15];
ENN_U32 gLast_History_Time;
extern UINT16 timer;
extern DEVICE_REG_DATA *p_Reg_Data;
extern UINT16 DevRegNum_Total;



//字节流转换为十六进制字符串  
void ByteToHexStr(const unsigned char* source, char* dest, int sourceLen)  
{  
    short i;  
    unsigned char highByte, lowByte;  
  
    for (i = 0; i < sourceLen; i++)  
    {  
        highByte = source[i] >> 4;  
        lowByte = source[i] & 0x0f ;  
  
        highByte += 0x30;  
  
        if (highByte > 0x39)  
                dest[i * 2] = highByte + 0x07;  
        else  
                dest[i * 2] = highByte;  
  
        lowByte += 0x30;  
        if (lowByte > 0x39)  
            dest[i * 2 + 1] = lowByte + 0x07;  
        else  
            dest[i * 2 + 1] = lowByte;  
    }  
    return ;  
}  

//十六进制字符串转换为字节流  
void HexStrToByte(const char* source, unsigned char* dest, int sourceLen)  
{  
	short i;  
	unsigned char highByte, lowByte;  

	for (i = 0; i < sourceLen; i += 2)  
	{  
		highByte = toupper(source[i]);  
		lowByte  = toupper(source[i + 1]);  

		if (highByte > 0x39)  
			highByte -= 0x37;  
		else  
			highByte -= 0x30;  

		if (lowByte > 0x39)  
			lowByte -= 0x37;  
		else  
			lowByte -= 0x30;  

		dest[i / 2] = (highByte << 4) | lowByte;  
	}  
	return ;  
}  




/*************************************************************************
*  名字:  Enn_CheckTableExist
*  说明:  检查数据库中某数据表是否已经存在
*  输入参数：tableName，所要查找的数据表名字
*         
*           
*  返回值: TRUE：存在该数据表
*         		FALSE：不存在该数据表
 *************************************************************************/
bool Enn_CheckTableExist(char *tableName)
{
	bool ret = FALSE;
	time_t timep;
	struct tm *p;
	char cdate[20];
	char sqlCmd[200], *errmsg, *sql;
	int n;

	char** pResult;
	int nRow;
	int nCol;

	/*memset(cdate , 0, sizeof(cdate));
	time(&timep);
	p=localtime(&timep); //取得当地时间
	n = sprintf(cdate, "%d%02d%02d", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday);
	DB_Debug("[%s],%d, cdate = %s, len = %d\n",__FUNCTION__,__LINE__,cdate,n);*/

	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s';",tableName);
	n = sqlite3_get_table(db,sqlCmd,&pResult,&nRow,&nCol,&errmsg);//pResult->count(*),0

	printf("%s\n",pResult[0]);
	printf("%s\n",pResult[1]);
	
	DB_Debug("[%s],%d, nRow = %d, nCol = %d, pResult = %s\n",__FUNCTION__,__LINE__,nRow,nCol, pResult[1]);
	if (n != SQLITE_OK)  
	{  
		sqlite3_close(db); 
		DB_Debug("[%s],%d, errmsg = %s\n",__FUNCTION__,__LINE__,errmsg);
		//cout<<errmsg; 
		sqlite3_free(errmsg);
		errmsg = NULL;
		return FALSE;  
	}

	n = atoi(pResult[1]);
	sqlite3_free_table(pResult);
	if(n == 0)
		return FALSE;
	else if(n == 1)
		return TRUE;
}

/*************************************************************************
*  名字:  Enn_DeleteOldTable
*  说明:  检查HistoricalData_RETENTION_TIME 天之前的表是否存在，
*			存在就删除它
*  输入参数：void
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
ENN_ErrorCode_t Enn_DeleteOldTable()
{
	//struct timeval ctv, ftv;
	//time_t timep;
	struct tm *p;
	char date[15];
	ENN_U32 timesecs;
	int nResult;
	char sqlCmd[200], *errmsg;

	timesecs = time(NULL);
	p = localtime(&timesecs); //timepoint - interval
	memset(date, 0 , sizeof(date));
	sprintf(date, "%d-%02d-%02d",1900+p->tm_year, 1+p->tm_mon, p->tm_mday);
	printf("[%s],%d, date=%s\n",__FUNCTION__,__LINE__,date);

	timesecs = timesecs - (HistoricalData_RETENTION_TIME* 24 * 3600);
	p = localtime(&timesecs); //timepoint - interval	
	/*memset(date, 0 , sizeof(date));
	sprintf(date, "%d-%02d-%02d",1900+p->tm_year, 1+p->tm_mon, p->tm_mday);
	printf("[%s],%d, date=%s\n",__FUNCTION__,__LINE__,date);*/

	memset(date, 0 , sizeof(date));
	nResult = sprintf(date, "t%d%02d%02d", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday);
	DB_Debug("[%s],%d, date =%s, len = %d\n",__FUNCTION__,__LINE__,date,nResult);

	if(Enn_CheckTableExist(date) == TRUE)
	{
		DB_Debug("[%s],%d, old table %s exist, will delete it\n",__FUNCTION__,__LINE__,date);
		//drop old table
		sprintf(sqlCmd, "DROP TABLE %s;",date);
		nResult = sqlite3_exec(db,sqlCmd,NULL,NULL,&errmsg);  
		if (nResult != SQLITE_OK)
		{  
			sqlite3_close(db);  
			//cout<<errmsg;
			DB_Debug("[%s],%d, errmsg = %s\n",__FUNCTION__,__LINE__,errmsg);
			sqlite3_free(errmsg);
			errmsg = NULL;
			return ENN_FAIL;  
		}
	}else
	{
		DB_Debug("[%s],%d, old table %s not exist\n",__FUNCTION__,__LINE__,date);
	}

	return ENN_SUCCESS;
}

/*************************************************************************
*  名字:  Enn_Database_init
*  说明:  历史数据库的初始化，包括打开数据库，创建
*			最新一天的数据表
*  输入参数：void
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
ENN_ErrorCode_t Enn_Database_init()
{	
	ENN_ErrorCode_t returnCode;
	int n, nResult;

	time_t timep;
	struct tm *p;
	char cdate[15];
	char sqlCmd[200], *errmsg, *sql;

	returnCode = sqlite3_open("/home/iec102.db",&db);//打开指定的数据库文件,如果不存在将创建一个同名的数据库文件
	if( returnCode )
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return ENN_FAIL;
	}
	else {
		printf("You have opened a sqlite3 database named iec102.db successfully!\n");
	}

	memset(cdate , 0, sizeof(cdate));
	memset(todayTable , 0, sizeof(todayTable));
	time(&timep);
	p=localtime(&timep); //取得当地时间
	n = sprintf(cdate, "t%d%02d%02d", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday);
	DB_Debug("[%s],%d, cdate = %s, len = %d\n",__FUNCTION__,__LINE__,cdate,n);
	memcpy(todayTable, cdate, strlen(cdate));

	
	if(Enn_CheckTableExist(todayTable) == FALSE)
	{
		Enn_DeleteOldTable();
		DB_Debug("[%s],%d, todayTable is not exist, will create it\n",__FUNCTION__,__LINE__);
		//create today table
		sprintf(sqlCmd, "CREATE TABLE %s(Time VARCHAR(12) PRIMARY KEY, Data TEXT);",todayTable);
		//sprintf(sqlCmd, "create table %s(Time varchar(12) primary key,Data text)",todayTable);
		DB_Debug("Tesss*****:%s\n",sqlCmd);
		nResult = sqlite3_exec(db,sqlCmd,NULL,NULL,&errmsg);  
		if (nResult != SQLITE_OK)  
		{  
			sqlite3_close(db);  
			//cout<<errmsg;
			DB_Debug("[%s],%d, errmsg = %s\n",__FUNCTION__,__LINE__,errmsg);
			sqlite3_free(errmsg);
			errmsg = NULL;
			return ENN_FAIL;  
		}
	}else
	{
		DB_Debug("[%s],%d, todayTable exist\n",__FUNCTION__,__LINE__);
	}
	
	//sqlite3_close(db); //关闭数据库
	return ENN_SUCCESS;
	
}

/*************************************************************************
*  名字:  Database_tablecheck
*  说明:  检查是否需要新建最新一天的表
*  输入参数：void
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
ENN_ErrorCode_t Database_tablecheck()
{
	time_t timep;
	struct tm *p;
	char cdate[15];
	char sqlCmd[200], *errmsg;
	int n, nResult;

	memset(cdate , 0, sizeof(cdate));
	time(&timep);
	p=localtime(&timep); //取得当地时间
	n = sprintf(cdate, "t%d%02d%02d", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday);
	//DB_Debug("[%s],%d, cdate = %s, len = %d\n",__FUNCTION__,__LINE__,cdate,n);
	//memcpy(todayTable, cdate, strlen(cdate));

	if(strncmp(cdate, todayTable, strlen(cdate)) != 0)
	{
		DB_Debug("[%s],%d, After a day, will create a new table\n",__FUNCTION__,__LINE__,cdate,n);
		memcpy(todayTable, cdate, strlen(cdate));
		if(Enn_CheckTableExist(todayTable) == FALSE)
		{
			Enn_DeleteOldTable();
			DB_Debug("[%s],%d, todayTable is not exist, will create it\n",__FUNCTION__,__LINE__);
			//create today table
			sprintf(sqlCmd, "CREATE TABLE %s(Time VARCHAR(12) PRIMARY KEY, Data TEXT);",todayTable);
			nResult = sqlite3_exec(db,sqlCmd,NULL,NULL,&errmsg);  
			if (nResult != SQLITE_OK)
			{  
				sqlite3_close(db);  
				//cout<<errmsg;
				DB_Debug("[%s],%d, nResult= %d ,errmsg = %s\n",__FUNCTION__,__LINE__,nResult,errmsg);
				sqlite3_free(errmsg);
				errmsg = NULL;
				return ENN_FAIL;  
			}
		}else
		{
			DB_Debug("WARNING : [%s],%d, todayTable exist\n",__FUNCTION__,__LINE__);
		}
	}
	//sqlite3_close(db); //关闭数据库
	return ENN_SUCCESS;
}

/*************************************************************************
*  名字:  _Database_insert
*  说明:  向数据库插入数据
*  输入参数：time，时标；data，所要插入数据
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
ENN_ErrorCode_t _Database_insert(char *time, char *data)
{
	char sqlCmd[2048], *errmsg, *sql;
	int nResult;

	if(todayTable == NULL || time == NULL || data == NULL)
	{
		DB_Debug("ERROR : [%s], %d, Param == NULL\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	//printf("%s,%s",time, data);

	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values('%s','%s');",todayTable, time, data);
	//DB_Debug("Test********:sqlcmd =%s\n", sqlCmd);
	
	nResult = sqlite3_exec(db,sqlCmd,NULL,NULL,&errmsg); 
	//nResult = sqlite3_exec(db,"select * from t19700101",callback,NULL,&errmsg);
	if (nResult != SQLITE_OK)
	{  
		sqlite3_close(db);  
		//cout<<errmsg;
		DB_Debug("[%s],%d, nResult=%d,errmsg = %s\n",__FUNCTION__,__LINE__,nResult,errmsg);
		sqlite3_free(errmsg);
		errmsg = NULL;
		return ENN_FAIL;  
	}

	return ENN_SUCCESS;
}


/*************************************************************************
*  名字:  Database_insert
*  说明:  向数据库插入数据
*  输入参数：cdate，时标
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
ENN_ErrorCode_t Database_insert(char *cdate)
{
	ENN_ErrorCode_t returnCode;
	cJSON *root;
	int i,j;
	char num[6], date[10];
	char *out;
	time_t timep;
	struct tm *p;
	char	*ptmp = NULL;

	if(DevRegNum_Total == 0 || p_Reg_Data == NULL)
	{
		DB_Debug("ERROR : [%s], %d, DevRegNum_Total == 0\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	
	returnCode = Database_tablecheck();
	if(ENN_SUCCESS != returnCode)
	{
		DB_Debug("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return ENN_FAIL;
	}

	for(i = 0; i < DevRegNum_Total; i++)
	{
		if(p_Reg_Data[i].u16SvInterval == 0)
				continue;
		if(timer%p_Reg_Data[i].u16SvInterval == 0 ||p_Reg_Data[i].u16SvInterval == 1){
			returnCode = ENNIEC102_Data_Map();
			if(ENN_SUCCESS != returnCode)
			{
				DB_Debug("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
				return ENN_FAIL;
			}
			break;
		}
	}
	if(i == DevRegNum_Total)
		return ENN_SUCCESS;
	
	root = cJSON_CreateObject();

	for(i = 0; i < DevRegNum_Total; i++)
	{
		if(p_Reg_Data[i].valid == 1)
		{	
			//printf("test*************:timer =%d,p_Reg_Data[%d].u16SvInterval = %d ,p_Reg_Data[i].valid =%d\n",timer, i, p_Reg_Data[i].u16SvInterval,p_Reg_Data[i].valid);
			if(p_Reg_Data[i].u16SvInterval == 0)
				continue;
			if(timer%p_Reg_Data[i].u16SvInterval == 0 ||p_Reg_Data[i].u16SvInterval == 1)
			{
				memset(num, 0, sizeof(num));
				sprintf(num, "%d", p_Reg_Data[i].u16RegID);
				//char tmp[500];
				//printf("test*************:timer =%d,p_Reg_Data[%d].u16SvInterval = %d \n",timer, i, p_Reg_Data[i].u16SvInterval);
				ptmp = (char *)malloc(p_Reg_Data[i].datalen * 2+1);
				if(ptmp == NULL)
				{
					printf("ERROR : %s, %d\n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				memset(ptmp, 0 , p_Reg_Data[i].datalen * 2 + 1);
				/*for(j = 0; j < p_Reg_Data[i].datalen; j++)
					sprintf(&ptmp[j * 2],"%02x",*(p_Reg_Data[i].pdata + ));*/
				ByteToHexStr(p_Reg_Data[i].pdata, ptmp, p_Reg_Data[i].datalen);
				//memcpy(ptmp, p_Reg_Data[i].pdata, p_Reg_Data[i].datalen);
				if(p_Reg_Data[i].u16RegID == 17)
					printf("test******************: datalen =%d, p_Reg_Data[%d].pdata=[%x,%x]\n",
						p_Reg_Data[i].datalen,i,*(p_Reg_Data[i].pdata),*(p_Reg_Data[i].pdata+1));
				if(p_Reg_Data[i].type == 0)
				{
					//cJSON_AddNumberToObject(root, num, ptmp);
					cJSON_AddStringToObject(root, num, ptmp);
				}
				else if(p_Reg_Data[i].type == 1)
				{
					//cJSON_AddNumberToObject(root, num, *((float*)&(p_Reg_Data[i].data)));
					//cJSON_AddNumberToObject(root, num, p_Reg_Data[i].data);
					cJSON_AddStringToObject(root, num, ptmp);
				}
				free(ptmp);
				ptmp = NULL;
				memset(p_Reg_Data[i].pdata, 0, p_Reg_Data[i].datalen);
				p_Reg_Data[i].valid = 0;
			}
		}
	}
	out = cJSON_Print(root);
	if(strlen(out) <= 3)//{}
	{
		cJSON_Delete(root);
		free(out);
		return ENN_SUCCESS;
	}
	DB_Debug("\n%s, len=%d\n", out,strlen(out));
	cJSON_Minify(out);

	_Database_insert(cdate, out);
	
	cJSON_Delete(root);
	free(out);

	return ENN_SUCCESS;
}

/*************************************************************************
*  名字:  select_callback
*  说明:  数据库操作的回调函数
*  输入参数：arg，操作结果；nCount，
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
int select_callback(void* arg, int nCount,char** pValue,char** pName)  
{  
	char *JSON_str;
	char num[6];
	int i, data, strleng;
	float fdata, ftmp;
	cJSON *root, *item = NULL;
	UINT16 regnum;
	DPA_REG_DATA *p_Dpa_Reg_Data;
	REG_DATA_VALUE *p_Reg_Data_Value;
	REG_DATA_VALUE *pLast_Reg_Data_Value;
	UINT8 *pstr;
	
	if(arg == NULL)
	{
		DB_Debug("ERROR : [%s], %d, Param == NULL\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	p_Dpa_Reg_Data = (DPA_REG_DATA *)arg;
		
	JSON_str = pValue[1];
	regnum = p_Dpa_Reg_Data->RegID;
/*	DB_Debug("[%s], %d, regnum = %d, type = %d, JSON_str= %s\n",
		__FUNCTION__,__LINE__, regnum,p_Dpa_Reg_Data->type,JSON_str);*/

	//DB_Debug("Test***********3:regnum=%d\n",regnum);

	memset(num, 0, sizeof(num));
	sprintf(num, "%d", regnum);

	root = cJSON_Parse(JSON_str);
	item = cJSON_GetObjectItem(root, num);
	if(item == NULL)
		return ENN_SUCCESS;
	//DB_Debug("Test***********2\n");

	strleng = strlen(item->valuestring);
	pstr = (UINT8 *)malloc(strleng / 2 + 1);
	if(NULL == pstr)
	{
		printf("ERROR : [%s], %d, malloc fail \n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	memset(pstr, 0 ,strleng / 2 + 1);

	HexStrToByte(item->valuestring, pstr, strleng);

	p_Reg_Data_Value = (REG_DATA_VALUE *)malloc(sizeof(REG_DATA_VALUE) );
	if(NULL == p_Reg_Data_Value)
	{
		printf("ERROR : [%s], %d, malloc fail \n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	memset(p_Reg_Data_Value, 0, sizeof(REG_DATA_VALUE));
	p_Reg_Data_Value->pdata = NULL;
	p_Reg_Data_Value->next = NULL;

	if(p_Dpa_Reg_Data->type == 0)
	{
		p_Reg_Data_Value->pdata = (char *)malloc(strleng / 2 + 1);
		if(NULL == p_Reg_Data_Value->pdata)
		{
			printf("ERROR : [%s], %d, malloc fail \n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}
		memset(p_Reg_Data_Value->pdata, 0, strleng / 2 + 1);
		memcpy(p_Reg_Data_Value->pdata, pstr, strlen(pstr));
	}
	else if(p_Dpa_Reg_Data->type == 1)
	{
		memcpy(&ftmp, pstr, 4);
		//p_Reg_Data_Value->fdata = (float)item->valuedouble;
		p_Reg_Data_Value->fdata = ftmp;
	}
	//DB_Debug("[%s], %d, fdata = %f\n",__FUNCTION__,__LINE__, p_Reg_Data_Value->fdata);

	//find a empty position 
	/*pLast_Reg_Data_Value = p_Dpa_Reg_Data->p_Reg_Data_Values;
	while(pLast_Reg_Data_Value != NULL){
		DB_Debug("Test**********00\n");
		pLast_Reg_Data_Value = pLast_Reg_Data_Value->next;
	}
	pLast_Reg_Data_Value = p_Reg_Data_Value;*/
	if(p_Dpa_Reg_Data->p_Reg_Data_Values == NULL)
	{
		p_Dpa_Reg_Data->p_Reg_Data_Values = p_Reg_Data_Value;
	}else{
		pLast_Reg_Data_Value = p_Dpa_Reg_Data->p_Reg_Data_Values;
		while(pLast_Reg_Data_Value->next != NULL)
			pLast_Reg_Data_Value = pLast_Reg_Data_Value->next;
		pLast_Reg_Data_Value->next = p_Reg_Data_Value;
	}

	/*DB_Debug("Test**********00:pLast_Reg_Data_Value %p, p_Dpa_Reg_Data->p_Reg_Data_Values =%p\n",
		pLast_Reg_Data_Value,p_Dpa_Reg_Data->p_Reg_Data_Values);*/
	//p_Dpa_Reg_Data->p_Reg_Data_Values = p_Reg_Data_Value;
	//DB_Debug("[%s], %d, fdata = %f\n",__FUNCTION__,__LINE__, p_Dpa_Reg_Data->p_Reg_Data_Values->fdata);

	//cJSON_Delete(item);
	free(pstr);
	pstr = NULL;
	cJSON_Delete(root);
	return ENN_SUCCESS;  
 }


ENN_ErrorCode_t Database_select(char *stime, char *etime, UINT16 RegNum)
{
	char sqlCmd[200], *errmsg;
	int n, nResult;
	DPA_REG_DATA *p_DPA_REG_DATA;
	REG_DATA_VALUE *pLast_Reg_Data_Value;
	
	if(todayTable == NULL ||stime == NULL ||etime == NULL)
	{
		DB_Debug("ERROR : [%s], %d, Param == NULL\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}

	if((strlen(stime) != strlen(etime)) ||(strlen(stime) != 8) ||
		(strcmp(etime,stime) < 0))	//00:00:00 -> len = 8
	{
		DB_Debug("ERROR : [%s], %d, Param error\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}

	//DB_Debug("Test***********1\n");

	p_DPA_REG_DATA = (DPA_REG_DATA *)malloc(sizeof(DPA_REG_DATA) );
	if(NULL == p_DPA_REG_DATA)
	{
		DB_Debug("ERROR : [%s], %d, malloc fail \n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	//DB_Debug("Test***********2\n");
	memset(p_DPA_REG_DATA, 0, sizeof(p_DPA_REG_DATA));
	p_DPA_REG_DATA->p_Reg_Data_Values = NULL;
	p_DPA_REG_DATA->RegID = RegNum;
	//DB_Debug("Test***********3\n");
	p_DPA_REG_DATA->type = p_Reg_Data[RegNum].type;
	//DB_Debug("Test***********4\n");

	sprintf(sqlCmd, "select * from %s where Time>=\'%s\' and Time<=\'%s\' ;",todayTable, stime, etime);
	//DB_Debug("Test********:sqlcmd =%s\n", sqlCmd);
	
	nResult = sqlite3_exec(db, sqlCmd, select_callback, p_DPA_REG_DATA, &errmsg); 
	//nResult = sqlite3_exec(db,"select * from t19700101",callback,NULL,&errmsg);
	if (nResult != SQLITE_OK)  
	{
		sqlite3_close(db);  
		//cout<<errmsg;
		DB_Debug("[%s],%d, errmsg = %s\n",__FUNCTION__,__LINE__,errmsg);
		sqlite3_free(errmsg);
		errmsg = NULL;
		return ENN_FAIL;  
	}

	if(p_DPA_REG_DATA->type ==1)
	{
		//DB_Debug("Test***********4\n");
		pLast_Reg_Data_Value = p_DPA_REG_DATA->p_Reg_Data_Values;
		while(pLast_Reg_Data_Value != NULL)
		{
			DB_Debug("[%s], %d, fdata = %f\n",__FUNCTION__,__LINE__, pLast_Reg_Data_Value->fdata);
			pLast_Reg_Data_Value = pLast_Reg_Data_Value->next;
		}
	}
	
	return ENN_SUCCESS;
}

/*************************************************************************
*  名字:  _Database_select_period
*  说明:  从某时间点的period 区间选取接近的值 
*  输入参数：table，表名；stime，起始时间；etime，终止时间
*					index，寄存器index；result，查询结果         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
ENN_ErrorCode_t _Database_select_period(char *table, char *stime, char *etime, 
												UINT16 index,
												POINT_DATA *result)
{
	char sqlCmd[200], *errmsg;
	int n, nResult;
	DPA_REG_DATA *p_DPA_REG_DATA;
	REG_DATA_VALUE *pLast_Reg_Data_Value;
	REG_DATA_VALUE *pLast_Reg_Data_next;
	
	if(table == NULL ||stime == NULL ||etime == NULL ||result == NULL)
	{
		DB_Debug("ERROR : [%s], %d, Param == NULL\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}

	if((strlen(stime) != strlen(etime)) ||(strlen(stime) != 8) ||
		(strcmp(etime,stime) < 0))	//00:00:00 -> len = 8
	{
		DB_Debug("ERROR : [%s], %d, Param error\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}

	p_DPA_REG_DATA = (DPA_REG_DATA *)malloc(sizeof(DPA_REG_DATA) );
	if(NULL == p_DPA_REG_DATA)
	{
		DB_Debug("ERROR : [%s], %d, malloc fail \n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	memset(p_DPA_REG_DATA, 0, sizeof(p_DPA_REG_DATA));
	p_DPA_REG_DATA->p_Reg_Data_Values = NULL;
	p_DPA_REG_DATA->RegID = p_Reg_Data[index].u16RegID;
	p_DPA_REG_DATA->type = p_Reg_Data[index].type;
	//DB_Debug("Test***********4\n");

	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "select * from %s where Time>=\'%s\' and Time<=\'%s\';",table, stime, etime);
	DB_Debug("Test********:sqlcmd =%s\n", sqlCmd);
	
	nResult = sqlite3_exec(db, sqlCmd, select_callback, p_DPA_REG_DATA, &errmsg); 
	//nResult = sqlite3_exec(db,"select * from t19700101",callback,NULL,&errmsg);
	if (nResult != SQLITE_OK)  
	{
		sqlite3_close(db);	
		//cout<<errmsg;
		DB_Debug("[%s],%d, errmsg = \n",__FUNCTION__,__LINE__);
		//sqlite3_free(errmsg);
		//errmsg = NULL;
		return ENN_FAIL; 
	}

	//DB_Debug("Test***********4\n");
	//if(p_DPA_REG_DATA->type ==1)
	pLast_Reg_Data_Value = p_DPA_REG_DATA->p_Reg_Data_Values;
	/*while(pLast_Reg_Data_Value != NULL)
	{
		DB_Debug("[%s], %d, fdata = %f\n",__FUNCTION__,__LINE__, pLast_Reg_Data_Value->fdata);
		pLast_Reg_Data_Value = pLast_Reg_Data_Value->next;
	}*/
	if(pLast_Reg_Data_Value != NULL)
	{
		if(p_DPA_REG_DATA->type ==1)
		{
			result->data.fdata= pLast_Reg_Data_Value->fdata;
			result->type = 2;
			//result->u32data = 0;
		}else if(p_DPA_REG_DATA->type ==0)
		{
			//result->data.u = 0;
			if((pLast_Reg_Data_Value->pdata != NULL) && (strlen(pLast_Reg_Data_Value->pdata) <= 4))
			{
				int i = 0, len, temp = 0;
				char *p = pLast_Reg_Data_Value->pdata;
				
				len = strlen(p);
				//result->data.u32data= pLast_Reg_Data_Value->u32data;
				printf(" [%s], %d,test***********:len =%d , data=",__FUNCTION__,__LINE__,len);
				for(i = 0; i < len; i++)
				{
					printf("%x ",p[i]);
					n = p[i];
					temp = temp + (n << (i * 8));
				}
				//printf("\n\rtest***********:temp =%d\n",temp);
				result->data.u32data= temp;

				result->type = 1;
			}
			else if((pLast_Reg_Data_Value->pdata != NULL) && (strlen(pLast_Reg_Data_Value->pdata) > 4))
			{
				result->type = 3;
				result->pdata = (UINT8 *)malloc(strlen(pLast_Reg_Data_Value->pdata)+1);
				if(NULL == result->pdata)
				{
					printf("ERROR : [%s], %d, malloc fail \n",__FUNCTION__,__LINE__);
					return ENN_FAIL;
				}
				memset(result->pdata, 0, strlen(pLast_Reg_Data_Value->pdata)+1);
				//result->data.u32data= pLast_Reg_Data_Value->u32data;
				memcpy(result->pdata, pLast_Reg_Data_Value->pdata, strlen(pLast_Reg_Data_Value->pdata));
			}
		}
	}else
	{
		WARN("WARNING : Not found RegID[%d] result in DB !!!\n", p_Reg_Data[index].u16RegID);
	}
//free mem
	pLast_Reg_Data_Value = p_DPA_REG_DATA->p_Reg_Data_Values;
	while( pLast_Reg_Data_Value){
		pLast_Reg_Data_next = pLast_Reg_Data_Value->next;
		if(pLast_Reg_Data_Value->pdata != NULL)
		{	
			free(pLast_Reg_Data_Value->pdata);
			pLast_Reg_Data_Value->pdata = NULL;
		}
		free(pLast_Reg_Data_Value);
		pLast_Reg_Data_Value = pLast_Reg_Data_next;
	}
	p_DPA_REG_DATA->p_Reg_Data_Values = NULL;
	free(p_DPA_REG_DATA);
	
	return ENN_SUCCESS;
}



/***********************************************
* FUNC: 从某时间点的period 区间选最接近的值 

* PARAM: timepoint->从1970年1月1日0时0分0秒到现在时刻的秒数
		

* RETURN: 

***********************************************/
ENN_ErrorCode_t Database_select_period(UINT32 timepoint, UINT16 RegNum, POINT_DATA *result)
{
	//char *ptmp, tmp[20];
	char  table[10], sdate[12], edate[12];
	UINT16 Reginterval = 60;
	time_t timep;
	struct tm *p;
	ENN_ErrorCode_t returnCode;
	int i;
	
	if(timepoint == 0 ||result == NULL)
	{
		DB_Debug("ERROR : [%s], %d, Param == NULL\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}

	//memset(tmp, 0, sizeof(tmp));
	memset(table, 0, sizeof(table));
	memset(sdate, 0, sizeof(sdate));
	memset(edate, 0, sizeof(edate));

	for(i = 0; i < DevRegNum_Total; i++)
	{
		if(p_Reg_Data[i].u16RegID == RegNum)
		{
			break;
		}
	}
	if(i == DevRegNum_Total)
	{
		printf("WARNING : %s, %d, not found p_Reg_Data[i].u16RegID = %d\n",__FUNCTION__,__LINE__,RegNum);	
		result->type = 0;
		result->data.u32data= 0;
		return ENN_FAIL;
	}

	if(p_Reg_Data[i].valid == 1)
	{
		Reginterval = p_Reg_Data[i].u16SvInterval;
	}else
	{
		WARN("WARNNING : RegNum is not found !\n");
	}
	
	timep = (time_t)timepoint;
	p = localtime(&timep); //取得当地时间
	//sprintf(tmp, "t%d%02d%02d;%02d:%02d",1900+p->tm_year, 1+p->tm_mon, p->tm_mday,p->tm_hour,p->tm_min);
	sprintf(table, "t%d%02d%02d",1900+p->tm_year, 1+p->tm_mon, p->tm_mday);

	timep = (time_t)(timepoint - Reginterval);
	p = localtime(&timep); //timepoint - interval
	sprintf(sdate, "%02d:%02d:%02d",p->tm_hour, p->tm_min, p->tm_sec);

	timep = (time_t)(timepoint + Reginterval);
	p = localtime(&timep); //timepoint + interval
	sprintf(edate, "%02d:%02d:%02d",p->tm_hour, p->tm_min, p->tm_sec);


	DB_Debug("[%s], %d, table =%s, sdate =%s, edate =%s, Reginterval =%d\n",__FUNCTION__,__LINE__,table, sdate, edate, Reginterval);

	returnCode = _Database_select_period(table, sdate, edate, i, result);
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return ENN_FAIL;
	}

	return ENN_SUCCESS;
}




#ifdef __cplusplus
#if __cplusplus
    }

#endif /* __cpluscplus */
#endif /* __cpluscplus */

