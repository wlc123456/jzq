#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */

#include "IEC102_Main.h"


#define IEC102_Debug	printf
//#define IEC102_Debug
#define DLT645_RegDataLen	4

rlSection *masterSection_Head = NULL;
DPA_SLAVE_PARAM *slave_table_Head = NULL;
UINT16 DevRegNum_Total = 0;      //设备寄存器的数量
UINT16 Current_Reg_index = 0;    //用于统计所有物理子设备拥有寄存器的数量
DEVICE_REG_DATA *p_Reg_Data = NULL;

extern FunCode_List *gFunCode_List_head;
extern Channel_List *gChannel_List_Head;

void test_102_print()
{
	int i = 0;
	int k = 0;
	rlSection 		*tmp = NULL;
	rlSectionName 	*tmp1 = NULL;
	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	tmp = masterSection_Head;
  	while(NULL != tmp)
	{
	  	IEC102_Debug("%s\n",tmp->name);
		tmp1 = tmp->firstName;
		i = 0;
		while(NULL != tmp1)
		{
			IEC102_Debug("   %s=%s\n",tmp1->name,tmp1->param);
			tmp1 = tmp1->next;
			i++;
		}
		IEC102_Debug("i = %d\n", i);
		IEC102_Debug("\n");
		tmp = tmp->next;
		k++;
	}
	IEC102_Debug("k = %d\n", k);

}

ENN_ErrorCode_t ENNIEC102_Set_Text(const ENN_CHAR *section, const ENN_CHAR *name, const ENN_CHAR *text)
{
	rlSection		*current_section = NULL; 
	rlSection		*last_section = NULL;
	rlSection		*add_section = NULL;
	rlSectionName	*current_name = NULL;
	rlSectionName	*last_name = NULL;
	rlSectionName		*add_name = NULL;
	
	ENNAPI_ASSERT(NULL != section);

	current_section = masterSection_Head;
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

			add_name = (rlSectionName *)malloc(sizeof(rlSectionName));
			if(NULL == add_name)
			{
				return ENN_FAIL;
			}
			memset((void *)add_name, 0, sizeof(rlSectionName));
			
			if(last_name == NULL)
			{
				current_section->firstName = add_name;
				current_name = current_section->firstName;
			}
			else
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

	add_section = (rlSection *)malloc(sizeof(rlSection));
	if(NULL == add_section)
	{
		return ENN_FAIL;
	}
	memset((void *)add_section, 0, sizeof(rlSection));
	if(NULL == last_section)
	{
		masterSection_Head = add_section;
		last_section = masterSection_Head;
	}
	else
	{
		last_section->next = add_section;
		last_section = add_section;
	}
	
	last_section->name = (ENN_CHAR *)malloc(strlen(section)+1);
	if(NULL == last_section->name)
	{
		free(add_section);
		add_section = NULL;
		
		return ENN_FAIL;
	}
	memset((void *)last_section->name, 0, strlen(section)+1);
	strcpy((char *)last_section->name, (char *)section);
	last_section->next = NULL;
	
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

ENN_CHAR *ENNIEC102_Get_Text(const ENN_CHAR *section, const ENN_CHAR *name)
{
	rlSection *current_section;
	rlSectionName *current_name;

	ENNAPI_ASSERT(NULL != section);
	ENNAPI_ASSERT(NULL != name);

	current_section = masterSection_Head;
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


ENN_ErrorCode_t ENNIEC102_Read_MasterInfo()
{
	ENN_CHAR	aBuffer[IEC102_READ_FILE_BUF_LEN];
	ENN_CHAR	aSection[IEC102_READ_FILE_BUF_LEN];
	ENN_CHAR	aName[IEC102_READ_FILE_BUF_LEN];
	ENN_CHAR	aParam[IEC102_READ_FILE_BUF_LEN];
	ENN_CHAR	*pStr1 = NULL;
	ENN_CHAR	*pStr2 = NULL;
	ENN_CHAR	*cptr = NULL;

	FILE 	*fpConfig;
	ENN_ErrorCode_t ret = ENN_SUCCESS;

	fpConfig = fopen("/home/DPA_IEC102.ini","r");
	if(NULL == fpConfig)
	{
		perror("open configure file fail ");
		return ENN_FAIL;
	}
	while(0 == feof(fpConfig))
	{
		memset(aBuffer, 0, IEC102_READ_FILE_BUF_LEN);
	    	if(NULL == fgets(aBuffer, IEC102_READ_FILE_BUF_LEN, fpConfig)) 
		{
	        	break;
		}

		//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aBuffer),aBuffer);
		cptr = strchr(aBuffer,0x0d);
		if(cptr != NULL) 
		{
			//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aBuffer),aBuffer);
			*cptr = '\0';
		}
		cptr = strchr(aBuffer,0x0a);
		if(cptr != NULL) 
		{
			//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aBuffer),aBuffer);
			*cptr = '\0';
		}

		pStr1 = aBuffer; 
		//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pStr1),pStr1);
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

			memset(aSection, 0, IEC102_READ_FILE_BUF_LEN);
			strcpy(aSection, pStr2);
			IEC102_Debug("%s, %d, aSection = %s\n",__FUNCTION__,__LINE__, aSection);
			ENNIEC102_Set_Text(aSection, NULL, NULL);
		        
		}
		else if(('#' != *pStr1) && (*pStr1 > ' ')) // name identifier
		//else if('#' != *pStr1) // name identifier
		{
			//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pStr1),pStr1);
			memset(aName, 0, IEC102_READ_FILE_BUF_LEN);
			ret = ENNCopy_Name(aName, pStr1);
			if(ENN_SUCCESS != ret)
			{
				perror("get name fail ");
				return ENN_FAIL;
			}
			
			//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aName),aName);
			memset(aParam, 0, IEC102_READ_FILE_BUF_LEN);
			ret = ENNCopy_Param(aParam, pStr1);
			if(ENN_SUCCESS != ret)
			{
				perror("get param fail ");
				return ENN_FAIL;
			}
			//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aParam),aParam);
			//ENNTRACE("%s, %d, %s, %d, %s, %d, %s\n",__FUNCTION__,__LINE__,aSection,strlen(aName),aName,strlen(aParam),aParam);
			ENNIEC102_Set_Text(aSection, aName, aParam);
		}
		else
		{
			//ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pStr1),pStr1);
			ENNIEC102_Set_Text(aSection, pStr1, NULL);
		}
	}
	fclose(fpConfig);  
	
	//test_102_print();

	return ENN_SUCCESS;
}

ENN_ErrorCode_t Analyze_InfoUnitMap(char *strbuf, DPA_MASTER_PARAM *p_add_master)
{
	ENN_CHAR *pTmp = NULL;
	ENN_CHAR *pTmp1 = NULL;
	ENN_CHAR *pBuf = NULL;
	ENN_CHAR *pb;
	ENN_U8 u8Num = 0;
	ENN_U8 u8Len = 0;
	int i;
	INFO_UNIT_MAP *pLast_info_unit_map = NULL;
	INFO_UNIT_MAP *pAdd_info_unit_map = NULL;

	pTmp = strbuf;
	while('\0' != *pTmp)
	{
		if('{' == *pTmp)
		{
			u8Num++;
		}
		pTmp++;
	}
	if(u8Num <= 1)
		return ENN_SUCCESS;
	pTmp = strbuf;
	u8Num--;

	while('{' != *pTmp)
	{
		pTmp++;
	}
	pTmp++;

	for(i=0; i<u8Num; i++)
	{
		u8Len = 0;
		pTmp = strchr(pTmp, '{');
		if(NULL == pTmp)
		{
			IEC102_Debug("ERROR :%s, %d, pTmp = NULL !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}
		pTmp++;

		pTmp1 = pTmp;
		while('}' != *pTmp1)
		{
			u8Len++;
			pTmp1++;
		}

		pBuf = (ENN_CHAR *)malloc(u8Len+1);
		if(NULL == pBuf)
		{
			IEC102_Debug("Error********* :%s, %d, malloc faile !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}
		memset((void *)pBuf, 0, u8Len+1);
		memcpy((void *)pBuf, (void *)pTmp, u8Len);
		pTmp = pTmp1;

		pAdd_info_unit_map = (INFO_UNIT_MAP*)malloc(sizeof(INFO_UNIT_MAP));
		if(NULL == pAdd_info_unit_map)
		{
			IEC102_Debug("Error********* :%s, %d, malloc faile !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}
		memset(pAdd_info_unit_map, 0, sizeof(INFO_UNIT_MAP));
		pAdd_info_unit_map->next = NULL;

		pb = strchr(pBuf, ',');
		*pb = '\0';
		pb++;
		pAdd_info_unit_map->infoNum = atoi(pBuf);
		pAdd_info_unit_map->u16RegID = atoi(pb);
		if(p_add_master->p_info_unit_map == NULL)
		{
			p_add_master->p_info_unit_map = pAdd_info_unit_map;
		}else{
			pLast_info_unit_map->next = pAdd_info_unit_map;
		}
		pLast_info_unit_map = pAdd_info_unit_map;

		free(pBuf);
		pBuf = NULL;
	}
	return ENN_SUCCESS;
}


ENN_ErrorCode_t Analyze_MasterInfo()
{
	ENN_CHAR *pParam = NULL;
	ENN_U8 master_no;
	ENN_U8 masterNo;
	ENN_U16 slave_port;
	int i;
	UINT8 sync_time_flag;
	ENN_CHAR  aSection[MODBUS_STRING_MAX_LEN];
	DPA_MASTER_PARAM *master_table_add = NULL;
	DPA_MASTER_PARAM *pLast_master_table = NULL;
	DPA_MASTER_PARAM *current_master_table = NULL;

	if(slave_table_Head == NULL)
	{
		slave_table_Head =  (DPA_SLAVE_PARAM *)malloc(sizeof(DPA_SLAVE_PARAM));
		if(NULL == slave_table_Head)
		{
			IEC102_Debug("Error********* :%s, %d, malloc faile !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}
	}

	memset((void *)slave_table_Head, 0, sizeof(DPA_SLAVE_PARAM));
	slave_table_Head->master_table = NULL;
	pParam = ENNIEC102_Get_Text("SLAVE", "MASTER_NUM");
	if(NULL != pParam)
	{
		master_no = atoi(pParam);
	}
	slave_table_Head->master_no = master_no;

	/*pParam = ENNIEC102_Get_Text("SLAVE", "SLAVE_IP");
	if(NULL != pParam)
	{
		strcpy((void *)slave_table_Head->slave_ip_addr, pParam);
		IEC102_Debug("%s, %d, name = %s, len = %d\n",__FUNCTION__,__LINE__, pParam, strlen(pParam));
	}

	pParam = ENNIEC102_Get_Text("SLAVE", "SLAVE_PORT");
	if(NULL != pParam)
	{
		slave_port = atoi(pParam);
	}
	slave_table_Head->slave_port= slave_port;*/

	for(i = 1; i <= master_no; i++)
	{
		master_table_add = (DPA_MASTER_PARAM *)malloc(sizeof(DPA_MASTER_PARAM));
		if(NULL == master_table_add)
		{
			IEC102_Debug("Error********* :%s, %d, malloc faile !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}
		memset((void *)master_table_add, 0, sizeof(DPA_MASTER_PARAM));
		master_table_add->p_info_unit_map = NULL;
		
		memset((void *)aSection, 0, MODBUS_STRING_MAX_LEN);
		sprintf(aSection, "MASTER_%d", i);

		masterNo = (UINT8)atoi(ENNIEC102_Get_Text(aSection, "MASTER_NO"));
		master_table_add->master_no = masterNo;

		pParam = ENNIEC102_Get_Text(aSection, "MASTER_NAME");
		if(NULL != pParam)
		{
			strcpy(master_table_add->master_name,pParam);
		}

		pParam = ENNIEC102_Get_Text(aSection, "MASTER_IP");
		if(NULL != pParam)
		{
			strcpy(master_table_add->master_ip_addr,pParam);
			IEC102_Debug("%s, %d, master_ip_addr = %s, len = %d\n",
				__FUNCTION__,__LINE__, master_table_add->master_ip_addr, strlen(master_table_add->master_ip_addr));
		}

		sync_time_flag = atoi(ENNIEC102_Get_Text(aSection, "SET_TIME"));
		master_table_add->sync_time_flag = sync_time_flag;

		pParam = ENNIEC102_Get_Text(aSection, "RecvTimeOut");
		if(NULL != pParam)
			master_table_add->master_timeout = atoi(pParam);
		else{
			IEC102_Debug("Error********* :%s, %d, ENNIEC102_Get_Text() fail !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}

		pParam = ENNIEC102_Get_Text(aSection, "LinkAddress");
		if(NULL != pParam){
			master_table_add->link_addr= atoi(pParam);
			IEC102_Debug("Test********* :%s, %d, LinkAddress = %d \n",__FUNCTION__,__LINE__,
				master_table_add->link_addr);
		}
		else{
			IEC102_Debug("Error********* :%s, %d, ENNIEC102_Get_Text() fail !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}

		pParam = ENNIEC102_Get_Text(aSection, "DATA_PERIOD");
		if(NULL != pParam)
			master_table_add->data_period= atoi(pParam);
		else{
			IEC102_Debug("Error********* :%s, %d, ENNIEC102_Get_Text() fail !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}

		pParam = ENNIEC102_Get_Text(aSection, "MAP");
		if(NULL == pParam)
		{
			IEC102_Debug("Error********* :%s, %d, ENNIEC102_Get_Text() fail !\n",__FUNCTION__,__LINE__);
			return ENN_FAIL;
		}
		Analyze_InfoUnitMap(pParam, master_table_add);
		
		master_table_add->next = NULL;

		if(slave_table_Head->master_table == NULL)
		{
			slave_table_Head->master_table = master_table_add;
		}
		else
		{
			pLast_master_table->next = master_table_add;
		}
		pLast_master_table = master_table_add;
	}

	return ENN_SUCCESS;
}

/*************************************************************************
*  名字:  ENNIEC102_init
*  说明:  IEC102协议，从配置文件读取主站信息并生成
*			主站信息链表的数据结构
*  输入参数：void
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/

ENN_ErrorCode_t ENNIEC102_init()
{
	ENN_ErrorCode_t returnCode;

	returnCode = ENNIEC102_Read_MasterInfo();
	if(ENN_SUCCESS != returnCode)
	{
		printf("[%s], %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return ENN_FAIL;
	}

	returnCode = Analyze_MasterInfo();
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return ENN_FAIL;
	}

	return ENN_SUCCESS;
}

/*************************************************************************
*  名字:  ENNIEC102_Data_init
*  说明:  为缓存数据申请空间
*  输入参数：void
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
ENN_ErrorCode_t ENNIEC102_Data_init()
{
	if(DevRegNum_Total == 0)
	{
		printf("ERROR : [%s], %d, DevRegNum_Total == 0\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	
	p_Reg_Data = (DEVICE_REG_DATA *)malloc(sizeof(DEVICE_REG_DATA) * DevRegNum_Total);
	if(NULL == p_Reg_Data)
	{
		printf("ERROR : [%s], %d, malloc fail \n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	memset((void *)p_Reg_Data, 0, sizeof(DEVICE_REG_DATA) * DevRegNum_Total);
	
	ENNIEC102_RegIndex_init();
	
	return ENN_SUCCESS;
}

ENN_ErrorCode_t ENNIEC102_get_RegKD(UINT16 RegID, float *k, float *d)
{
	Register_List *pRegister_List_Temp = NULL;
	Channel_List *pChannel_Temp = NULL;
	Slave_List	*pSlave_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;

	Dev645_List *pDev645_List_Temp = NULL;
	Code_645_List *pCode_645_List_Temp = NULL;
	
	if(RegID == 0 ||NULL == gChannel_List_Head ||k ==NULL ||d ==NULL)
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
					else{
						pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
						continue;
					}

					//printf("test:%s, %d, pSlave_FunCode_Temp.fun =%d \n",__FUNCTION__,__LINE__, pSlave_FunCode_Temp->u8MBFunCode);
					while(NULL != pRegister_List_Temp)
					{
						/*p_Reg_Data[Current_Reg_index].u16RegID = pRegister_List_Temp->u16RegID;
						p_Reg_Data[Current_Reg_index].type = 0;
						p_Reg_Data[Current_Reg_index].u16SvInterval = pRegister_List_Temp->u16SvInterval;
						Current_Reg_index++;*/
						if(pRegister_List_Temp->u16RegID == RegID)
						{
							*k = pRegister_List_Temp->fRegK;
							*d = pRegister_List_Temp->fRegD;
							return ENN_SUCCESS;
						}

						pRegister_List_Temp = pRegister_List_Temp->next;
					}
					
					pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
				}
				pSlave_Temp = pSlave_Temp->next;
			}
		}
		else if((PROTOCOL_645_1997 == pChannel_Temp->u8Protocol) || (PROTOCOL_645_2007 == pChannel_Temp->u8Protocol))
		{
			pDev645_List_Temp = pChannel_Temp->unDeviceType.pDev645_List;
			while(NULL != pDev645_List_Temp)
			{
				pCode_645_List_Temp = pDev645_List_Temp->pCode_645_List;
				while(NULL != pCode_645_List_Temp)
				{
					/*p_Reg_Data[Current_Reg_index].u16RegID = pCode_645_List_Temp->u16RegID;
					p_Reg_Data[Current_Reg_index].type = 1;
					p_Reg_Data[Current_Reg_index].u16SvInterval = pCode_645_List_Temp->u16SvInterval;
					Current_Reg_index++;*/
					if(pCode_645_List_Temp->u16RegID == RegID)
					{
						*k = pCode_645_List_Temp->fRegK;
						*d = pCode_645_List_Temp->fRegD;
						return ENN_SUCCESS;
					}
					
					pCode_645_List_Temp = pCode_645_List_Temp->next;
				}
				pDev645_List_Temp = pDev645_List_Temp->next;
			}
		}
		pChannel_Temp = pChannel_Temp->next;
	}
	return ENN_FAIL;
}


/*************************************************************************
*  名字:  ENNIEC102_RegIndex_init
*  说明:  初始化数据缓存buf，并计数物理子设备所有的
*			寄存器数量
*  输入参数：void
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
ENN_ErrorCode_t ENNIEC102_RegIndex_init()
{
	Register_List *pRegister_List_Temp = NULL;
	Channel_List *pChannel_Temp = NULL;
	Slave_List 	*pSlave_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;

	Dev645_List *pDev645_List_Temp = NULL;
	Code_645_List *pCode_645_List_Temp = NULL;
	
	if(DevRegNum_Total == 0 || p_Reg_Data == NULL ||NULL == gChannel_List_Head)
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

					//printf("test:%s, %d, pSlave_FunCode_Temp.fun =%d \n",__FUNCTION__,__LINE__, pSlave_FunCode_Temp->u8MBFunCode);
					while(NULL != pRegister_List_Temp)
					{
						p_Reg_Data[Current_Reg_index].u16RegID = pRegister_List_Temp->u16RegID;
						p_Reg_Data[Current_Reg_index].type = 0;
						p_Reg_Data[Current_Reg_index].u16SvInterval = pRegister_List_Temp->u16SvInterval;
						p_Reg_Data[Current_Reg_index].pdata = (UINT8 *)malloc(pRegister_List_Temp->u16RegNum * 2);
						if(NULL == p_Reg_Data[Current_Reg_index].pdata)
						{
							ENNTRACE("ERROR : %s, %d\n",__FUNCTION__,__LINE__);
							return ENN_FAIL;
							//continue;
						}
						memset(p_Reg_Data[Current_Reg_index].pdata, 0, pRegister_List_Temp->u16RegNum * 2);
						Current_Reg_index++;

						pRegister_List_Temp = pRegister_List_Temp->next;
					}
					//printf("[%s],%d test*****\n",__FUNCTION__,__LINE__);
					printf("%s, %d, Current_Reg_index =%d p_Reg_Data[Current_Reg_index].u16RegID =%d\n",
						__FUNCTION__,__LINE__, Current_Reg_index,p_Reg_Data[Current_Reg_index-1].u16RegID);
					pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
				}
				pSlave_Temp = pSlave_Temp->next;
			}
		}
		else if((PROTOCOL_645_1997 == pChannel_Temp->u8Protocol) || (PROTOCOL_645_2007 == pChannel_Temp->u8Protocol))
		{
			pDev645_List_Temp = pChannel_Temp->unDeviceType.pDev645_List;
			while(NULL != pDev645_List_Temp)
			{
				pCode_645_List_Temp = pDev645_List_Temp->pCode_645_List;
				while(NULL != pCode_645_List_Temp)
				{
					p_Reg_Data[Current_Reg_index].u16RegID = pCode_645_List_Temp->u16RegID;
					if(pCode_645_List_Temp->eData_Type == Data_DECIMAL)
						p_Reg_Data[Current_Reg_index].type = 1;
					else
						p_Reg_Data[Current_Reg_index].type = 0;
					p_Reg_Data[Current_Reg_index].u16SvInterval = pCode_645_List_Temp->u16SvInterval;
					p_Reg_Data[Current_Reg_index].pdata = (UINT8 *)malloc(DLT645_RegDataLen);
					if(NULL == p_Reg_Data[Current_Reg_index].pdata)
					{
						ENNTRACE("ERROR : %s, %d\n",__FUNCTION__,__LINE__);
						return ENN_FAIL;
						//continue;
					}
					memset(p_Reg_Data[Current_Reg_index].pdata, 0, DLT645_RegDataLen);
					Current_Reg_index++;
					
					pCode_645_List_Temp = pCode_645_List_Temp->next;
				}
				printf("%s, %d, Current_Reg_index =%d p_Reg_Data[Current_Reg_index].u16RegID =%d\n",
						__FUNCTION__,__LINE__, Current_Reg_index,p_Reg_Data[Current_Reg_index-1].u16RegID);
				pDev645_List_Temp = pDev645_List_Temp->next;
			}
		}
		pChannel_Temp = pChannel_Temp->next;
	}
}


/*************************************************************************
*  名字:  ENNIEC102_Data_Map
*  说明:  IEC102协议，从底层数据buf中提取数据并
*			映射到中间层缓存buf
*  输入参数：void
*         
*           
*  返回值: ENN_SUCCESS：处理成功
*         ENN_FAIL：处理失败
 *************************************************************************/
ENN_ErrorCode_t ENNIEC102_Data_Map()
{
	ENN_ErrorCode_t returnCode;
	UINT8 Existflag;
	ENN_U8 u8FunCode = 0;
	ENN_U8 u8Channelx = 0;
	ENN_U16 u16Offset = 0;
	ENN_U16 u16RegNum = 0;
	ENN_U16 u16ByteNum = 0;
	ENN_U16 RegID;
	int i;
	ENN_U32 u32tmp = 0;

	Register_List *pRegister_List_Temp = NULL;
	Channel_List *pChannel_Temp = NULL;
	FunCode_List *pFunCode_Temp = NULL;
	Slave_List 	*pSlave_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_Temp = NULL;

	Dev645_List *pDev645_List_Temp = NULL;
	Code_645_List *pCode_645_List_Temp = NULL;
	bool channeloffsetflag = FALSE;

	if(DevRegNum_Total == 0 || p_Reg_Data == NULL)
	{
		printf("ERROR : [%s], %d, DevRegNum_Total == 0\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}

	if(NULL == gFunCode_List_head ||NULL == gChannel_List_Head)
	{
		printf("ERROR : [%s], %d, NULL == gFunCode_List_head ||NULL == gChannel_List_Head\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	pChannel_Temp = gChannel_List_Head;
	//pFunCode_Temp = gFunCode_List_head;
	while(NULL != pChannel_Temp)
	{
		Existflag = 0;
		u8Channelx = pChannel_Temp->u8Channel;
		channeloffsetflag = TRUE;
		if(PROTOCOL_MODBUS == pChannel_Temp->u8Protocol)
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
					if(NULL != pSlave_FunCode_Temp->pRegister_List)
					{
						pRegister_List_Temp = pSlave_FunCode_Temp->pRegister_List;
					}
					else{
						pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
						continue;
					}
					u8FunCode = pSlave_FunCode_Temp->u8MBFunCode;

					pFunCode_Temp = gFunCode_List_head;
					while((pFunCode_Temp != NULL)&&(pFunCode_Temp->u8MBFunCode != u8FunCode))
						pFunCode_Temp = pFunCode_Temp->next;
					if(pFunCode_Temp == NULL)
					{
						//IEC102_DEBUG("WARNING : %s, %d, not found pFunCode_Temp u8FunCode =%d\n",__FUNCTION__,__LINE__,u8FunCode);
						pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
						continue;
					}
					if(channeloffsetflag == TRUE){
						u16Offset = pFunCode_Temp->Offset[u8Channelx];
						if((MB_FUNC_READ_HOLDING_REGISTER == u8FunCode)  
						|| (MB_FUNC_READ_INPUT_REGISTER == u8FunCode))
						{
							u16Offset = u16Offset * 2;
						}
						else if((MB_FUNC_WRITE_SINGLE_COIL == u8FunCode)  
							 || (MB_FUNC_WRITE_MULTIPLE_REGISTERS == u8FunCode))
						{
							//pFunCode_Temp = pFunCode_Temp->next;
							pSlave_FunCode_Temp = pSlave_FunCode_Temp->next;
							continue;
						}
						channeloffsetflag = FALSE;
					}
					if((0x03 == u8FunCode) || (0x04 == u8FunCode)){
						while(NULL != pRegister_List_Temp)
						{
							//IEC102_DEBUG("[%s], %d, Channel = %d, u16Offset = %d\n",__FUNCTION__,__LINE__,u8Channelx,u16Offset);
							RegID = pRegister_List_Temp->u16RegID;

							u16RegNum = pRegister_List_Temp->u16RegNum;
							u16ByteNum = u16RegNum * 2;
							
							if(NULL != pFunCode_Temp->pData)
							{
								for(i = 0; i < DevRegNum_Total; i++)
								{
									if(p_Reg_Data[i].u16RegID == RegID)
									{
										//IEC102_DEBUG("[%s], %d, Channel = %d, u16Offset = %d\n",__FUNCTION__,__LINE__,u8Channelx,u16Offset);
//处理板子大小端问题
										//ENN_U8 *tmp;
										ENN_U32 tempU32;
										int j=0;

										ENN_U8 *tmp = (ENN_U8 *)malloc(u16ByteNum);
										if(NULL == tmp)
										{
											ENNTRACE("ERROR : %s, %d\n",__FUNCTION__,__LINE__);
											return ENN_FAIL;
											//continue;
										}
										memset(tmp, 0, u16ByteNum);

										/*tempU32 = 0;
										tmp = &tempU32;*/

										//memcpy(&u32tmp, (void *)(pFunCode_Temp->pData + u16Offset), u16ByteNum);
										//tmp = malloc( u16ByteNum);
										for(j = 0; j<u16ByteNum; j++){
											tmp[j] =  *(ENN_U8*)(pFunCode_Temp->pData + u16Offset+(u16ByteNum-j-1));
										}

										memset(pFunCode_Temp->pData + u16Offset, 0 , u16ByteNum);
										//tempU32 = *(ENN_U32*)tmp;								
										
										memcpy(p_Reg_Data[i].pdata, tmp, u16ByteNum);
										if(RegID == 20){
											printf("Test******:[%s],%d, u16ByteNum =%d, tmp[0,1,2,3]=",__FUNCTION__,__LINE__,u16ByteNum);
											int n = 0;
											for(; n < u16ByteNum; n++)
												printf("%x,",tmp[n]);
											printf("\n");
										}
										free(tmp);
										tmp = NULL;
										//p_Reg_Data[i].data =tempU32 ;
										/*if(RegID == 5)
											printf("[%s],%d, p_Reg_Data[%d].data=%d,pData = %x %x, u16Offset =%d\n",__FUNCTION__,__LINE__,
											i,p_Reg_Data[i].data,*(pFunCode_Temp->pData + u16Offset),*(pFunCode_Temp->pData + u16Offset+1),u16Offset);*/
										p_Reg_Data[i].valid = 1;
										p_Reg_Data[i].datalen = u16ByteNum;
										break;
									}
								}
								if(i == DevRegNum_Total)
									IEC102_DEBUG("WARNING : %s, %d, not found p_Reg_Data[i].u16RegID \n",__FUNCTION__,__LINE__);
							}
							u16Offset += u16ByteNum;
							//IEC102_DEBUG("[%s], %d, Channel = %d, u16Offset = %d\n",__FUNCTION__,__LINE__,u8Channelx,u16Offset);
							pRegister_List_Temp = pRegister_List_Temp->next;
						}
					}else if((0x01 == u8FunCode) || (0x02 == u8FunCode))
					{
						while(NULL != pRegister_List_Temp)
						{
							//u16RegNum = pRegister_List_Temp->u16RegNum;
							RegID = pRegister_List_Temp->u16RegID;
							if(NULL != pFunCode_Temp->pData)
							{
								/*memcpy(&p_Reg_Data[RegID].data, (void *)(pFunCode_Temp->pData + u16Offset), 1);
								p_Reg_Data[RegID].valid = 1;
								p_Reg_Data[RegID].type = 0;
								p_Reg_Data[RegID].u16SvInterval = pRegister_List_Temp->u16SvInterval;*/
								for(i = 0; i < DevRegNum_Total; i++)
								{
									if(p_Reg_Data[i].u16RegID == RegID)
									{
										memcpy(p_Reg_Data[i].pdata, (void *)(pFunCode_Temp->pData + u16Offset), 1);
										p_Reg_Data[i].valid = 1;
										p_Reg_Data[i].datalen = 1;
										memset(pFunCode_Temp->pData + u16Offset, 0 , 1);
										break;
									}
								}
								if(i == DevRegNum_Total)
									IEC102_DEBUG("WARNING : %s, %d, not found p_Reg_Data[i].u16RegID \n",__FUNCTION__,__LINE__);	
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
					/*IEC102_DEBUG("[%s], %d, pCode_645_List_Temp->u16RegID = %d,u16Offset = %d\n",
						__FUNCTION__,__LINE__,pCode_645_List_Temp->u16RegID,u16Offset);*/
					/*memcpy(&p_Reg_Data[RegID].data,(void *)(pFunCode_Temp->pData + u16Offset), 4);
					p_Reg_Data[RegID].valid = 1;
					p_Reg_Data[RegID].type = 1;
					p_Reg_Data[RegID].u16SvInterval = pCode_645_List_Temp->u16SvInterval;*/
					for(i = 0; i < DevRegNum_Total; i++)
					{
						if(p_Reg_Data[i].u16RegID == RegID)
						{							
							memcpy(p_Reg_Data[i].pdata,(void *)(pFunCode_Temp->pData + u16Offset), DLT645_RegDataLen);
							p_Reg_Data[i].valid = 1;
							p_Reg_Data[i].datalen = DLT645_RegDataLen;
							/*if(RegID == 40)
								printf("Test****** : %s, %d, data =%f \n",__FUNCTION__,__LINE__,*((float*)&(p_Reg_Data[i].data)));*/
							memset(pFunCode_Temp->pData + u16Offset, 0 , DLT645_RegDataLen);
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
		}
		pChannel_Temp = pChannel_Temp->next;
	}
/*	IEC102_DEBUG("[%s], %d, p_Reg_Data[0,1] = %x,%x, u16SvInterval[1,2] = [%d,%d]\n",
		__FUNCTION__,__LINE__,p_Reg_Data[0].data,p_Reg_Data[1].data, p_Reg_Data[0].u16SvInterval,p_Reg_Data[1].u16SvInterval);*/
	return ENN_SUCCESS;
}

void test_102_print1()
{
	float ftmp,ftmp1;
	memcpy(&ftmp,p_Reg_Data[3].pdata,4);
	memcpy(&ftmp1,p_Reg_Data[4].pdata,4);
	IEC102_DEBUG("[%s], %d, p_Reg_Data[32,33] = %x,%x\n",__FUNCTION__,__LINE__,*(p_Reg_Data[3].pdata),*(p_Reg_Data[4].pdata));
	/*IEC102_DEBUG("[%s], %d, p_Reg_Data[24,27] =  %f, %f, u16SvInterval[25] = [%d]\n",
		__FUNCTION__,__LINE__,*((float*)&(p_Reg_Data[24].data)),*((float*)&(p_Reg_Data[27].data)),p_Reg_Data[25].u16SvInterval);*/
}

#ifdef __cplusplus
#if __cplusplus
    }

#endif /* __cpluscplus */
#endif /* __cpluscplus */

