static uint8* pack_Data_JASON(uint8 maxPackNo)
{
	uint8 totalNo, no[HYCFG_MAX_JASON_PACK_NO];	
	uint16 i, j, lineNo;
	uint16 mapIndex;
	uint8 *out;  
	cJSON *root,*group[HYCFG_MAX_JASON_PACK_NO],*array, *object;
	uint8 timeStr[10];
	HY_TYPE_TIME timeStruct;

	if( maxPackNo > HYCFG_MAX_JASON_PACK_NO ){
		maxPackNo = HYCFG_MAX_JASON_PACK_NO;
	}
	Time_Get(&timeStruct);		
	
	for( i = 0, totalNo = 0; i < p_meter_amount; i++ ){
		if(meterDataArray[i].valid ==  HY_EN){
			totalNo++;
			no[totalNo-1] = i; // 真实表号从1开始，即no[]+1
			if( totalNo >= maxPackNo ){
				break;
			}
		}
	}
	if( totalNo == 0 ){
		return NULL;
	}
	if( totalNo > maxPackNo ){
		totalNo = maxPackNo;
	}

	root = cJSON_CreateObject();
	object = cJSON_CreateObject(); 	
	array = cJSON_CreateArray();

	for( i = 0; i < totalNo; i++){
		group[i] = cJSON_CreateObject(); 


	}

	cJSON_AddStringToObject(root,"imei",g_PhoneNo);
	cJSON_AddStringToObject(root,"hu",g_Hu);
	
	if( totalNo == 1 ){
		cJSON_AddItemToObject(root,"data",object);
		cJSON_AddNumberToObject(object,"Line",(no[0]+1) + lineOffset); 
		for( i = 0; i < METER_DATA_NUM; i++ ){
			for( j = 0; j < regArrayMapLen; j++ ){
				if( i == RegArrayMap[j].index){
					cJSON_AddNumberToObject(object,RegArrayMap[j].name,meterDataArray[no[0]].floatDataArray[i]);
					break;
				}
			}	
		}
		cJSON_AddNumberToObject(root,"T",sensorT);
		cJSON_AddNumberToObject(root,"S",sensorH);
		sprintf( timeStr, "%0.2d:%0.2d:%0.2d",timeStruct.HH , timeStruct.MM, timeStruct.SS);
		cJSON_AddStringToObject(root,"Time",timeStr);
		cJSON_AddStringToObject(root,"v","XXXXXXXXXXXXXXXX");

	}else{
		cJSON_AddItemToObject(root,"data",array);
		for( lineNo = 0; lineNo < totalNo; lineNo++){			
			cJSON_AddItemToObject(array,NULL,group[lineNo]);
			cJSON_AddNumberToObject(group[lineNo],"Line",no[lineNo]+1 + lineOffset); 
			for( i = 0; i < METER_DATA_NUM; i++ ){
				for( j = 0; j < regArrayMapLen; j++ ){
					if( i == RegArrayMap[j].index){
						cJSON_AddNumberToObject(group[lineNo],RegArrayMap[j].name,meterDataArray[no[lineNo]].floatDataArray[i]);
						break;
					}
				}
			}
		}
		cJSON_AddNumberToObject(root,"T",sensorT);
		cJSON_AddNumberToObject(root,"S",sensorH);
		sprintf( timeStr, "%0.2d:%0.2d:%0.2d",timeStruct.HH , timeStruct.MM, timeStruct.SS);
		cJSON_AddStringToObject(root,"Time",timeStr);
		cJSON_AddStringToObject(root,"v","XXXXXXXXXXXXXXXX");
	}

	out = cJSON_Print(root);
	cJSON_Minify(out);
	calHTTPDataMD5(out);

	for( i = 0; i < totalNo; i++ ){
		meterDataArray[no[i]].valid = HY_DISEN;
	}

	cJSON_Delete(root);  
	cJSON_Delete(object);  
	cJSON_Delete(array);
	root = 0;
	object = 0;
	array = 0;
	for( i = 0; i < totalNo; i++){
		cJSON_Delete(group[i]); 
		group[i] = 0;
	}

	return out;

}

