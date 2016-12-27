/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>


#include "ennAPI.h"
#include "ennSocket.h"
#include "ModBus_TCP.h"
#include "ModBus_Slave_Table.h"
#include "IEC102_Main.h"
#include "IEC102protocol.h"


#include "config.h"
#include "server.h"
#include "address.h"
#include "bacdef.h"
#include "handlers.h"
#include "client.h"
#include "dlenv.h"
#include "bacdcode.h"
#include "npdu.h"
#include "apdu.h"
#include "iam.h"
#include "tsm.h"
#include "device.h"
#include "bacfile.h"
#include "datalink.h"
#include "dcc.h"
#include "getevent.h"
#include "net.h"
#include "txbuf.h"
#include "lc.h"
#include "version.h"
/* include the device object */
#include "device.h"
#include "trendlog.h"
#if defined(INTRINSIC_REPORTING)
#include "nc.h"
#endif /* defined(INTRINSIC_REPORTING) */
#if defined(BACFILE)
#include "bacfile.h"
#endif /* defined(BACFILE) */

#include "ai.h"
#include "av.h"

int Bacnet_Register_List_Table_Number = 0;
Bacnet_Register_List Bacnet_Register_List_Table[256] = {0};

extern Channel_List *gChannel_List_Head;
extern FunCode_List *gFunCode_List_head;


extern unsigned int Analog_Input_Number ;
extern unsigned int Analog_Output_Number ;
extern unsigned int Analog_Value_Number ;

extern unsigned int Binary_Input_Number;
extern unsigned int Binary_Output_Number;
extern unsigned int Binary_Value_Number;

extern unsigned int Multistate_Input_Number;
extern unsigned int Multistate_Output_Number;
extern unsigned int Multistate_Value_Number;

 extern ANALOG_INPUT_DESCR AI_Descr[4];
 extern ANALOG_VALUE_DESCR AV_Descr[4];

 extern unsigned int AV_Descr_Instance[4];
 extern unsigned int AI_Descr_Instance[4];




 void ENNBacnet_Object_instance_Statistical(Channel_List *pChannel_Temp,
 															  Bacnet_Register_List *Bacnet_Register_List_Table)
 {
	Channel_List *pChannel_List_Temp = NULL;
	Slave_List *pSlave_List_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_List_Temp = NULL;
	Register_List *pRegister_List_Temp = NULL;
	int i = 0;

	pChannel_List_Temp = pChannel_Temp;
		
	while(pChannel_List_Temp != NULL)
	{
		if(PROTOCOL_MODBUS == pChannel_List_Temp->u8Protocol)
		{
			pSlave_List_Temp = pChannel_List_Temp->unDeviceType.pModBus_List;
			while(pSlave_List_Temp != NULL)
			{
				pSlave_FunCode_List_Temp = pSlave_List_Temp->pFunCode_List;
				if(pSlave_FunCode_List_Temp->u8MBFunCode == 0x03)
				{
					while(pSlave_FunCode_List_Temp != NULL)
					{
						pRegister_List_Temp = pSlave_FunCode_List_Temp->pRegister_List;
						while(pRegister_List_Temp != NULL)
						{
							if(pRegister_List_Temp->object_type == OBJECT_ANALOG_INPUT)
								Analog_Input_Number += 1;
							else if(pRegister_List_Temp->object_type == OBJECT_ANALOG_OUTPUT)
								Analog_Output_Number += 1;
							else if(pRegister_List_Temp->object_type == OBJECT_ANALOG_VALUE)
								Analog_Value_Number += 1;
							else if(pRegister_List_Temp->object_type == OBJECT_BINARY_INPUT)
								Binary_Input_Number += 1;
							else if(pRegister_List_Temp->object_type == OBJECT_BINARY_OUTPUT)
								Binary_Output_Number += 1;
							else if(pRegister_List_Temp->object_type == OBJECT_BINARY_VALUE)
								Binary_Value_Number += 1;
							else if(pRegister_List_Temp->object_type == OBJECT_MULTI_STATE_INPUT)
								Multistate_Input_Number += 1;
							else if(pRegister_List_Temp->object_type == OBJECT_MULTI_STATE_OUTPUT)
								Multistate_Output_Number += 1;
							else if(pRegister_List_Temp->object_type == OBJECT_MULTI_STATE_VALUE)
								Multistate_Value_Number += 1;
							pRegister_List_Temp = pRegister_List_Temp->next;
						}
						pSlave_FunCode_List_Temp = pSlave_FunCode_List_Temp->next;
					}
				}
				pSlave_List_Temp = pSlave_List_Temp->next;
			}
		}
		pChannel_List_Temp = pChannel_List_Temp->next;
	}
	
	for( i = 0 ; i <  Bacnet_Register_List_Table_Number ; i++)
	{
		if(Bacnet_Register_List_Table[i].Object_Type == OBJECT_ANALOG_INPUT)
			Analog_Input_Number += 1;
		else if(Bacnet_Register_List_Table[i].Object_Type == OBJECT_ANALOG_OUTPUT)
			Analog_Output_Number += 1;
		else if(Bacnet_Register_List_Table[i].Object_Type == OBJECT_ANALOG_VALUE)
			Analog_Value_Number += 1;
		else if(Bacnet_Register_List_Table[i].Object_Type == OBJECT_BINARY_INPUT)
			Binary_Input_Number += 1;
		else if(Bacnet_Register_List_Table[i].Object_Type == OBJECT_BINARY_OUTPUT)
			Binary_Output_Number += 1;
		else if(Bacnet_Register_List_Table[i].Object_Type == OBJECT_BINARY_VALUE)
			Binary_Value_Number += 1;
		else if(Bacnet_Register_List_Table[i].Object_Type == OBJECT_MULTI_STATE_INPUT)
			Multistate_Input_Number += 1;
		else if(Bacnet_Register_List_Table[i].Object_Type == OBJECT_MULTI_STATE_OUTPUT)
			Multistate_Output_Number += 1;
		else if(Bacnet_Register_List_Table[i].Object_Type == OBJECT_MULTI_STATE_VALUE)
			Multistate_Value_Number += 1;
	}
 }

void ENNBacnet_Object_instance_Acquire_Data(Channel_List *pChannel_Temp,
																   FunCode_List  *pFuncCode_List,
																   Bacnet_Register_List *Bacnet_Register_List_Table)
{
	ENN_U8	u8ChannelX = 0;
	Channel_List *pChannel_List_Temp = NULL;
	Slave_List *pSlave_List_Temp = NULL;
	Slave_FunCode_List *pSlave_FunCode_List_Temp = NULL;
	Register_List *pRegister_List_Temp = NULL;
	FunCode_List  *pFuncCode_List_Temp = NULL;
	ENN_U8 *pData =NULL;
	ENN_U16 Offset = 0;
	ENN_U16  pTemp = 0;
	ENN_U16 Len = 0;
	int num = 0;

	int i = 0;
	int j = 0;
	int bacnet_register_number = 0;
	
	pFuncCode_List_Temp = pFuncCode_List;
	/*
	while(pFuncCode_List_Temp != NULL && pFuncCode_List_Temp ->u8MBFunCode != 0x03)
	{
		pFuncCode_List_Temp = pFuncCode_List_Temp->next;
	}
	pData = pFuncCode_List_Temp->pData;
	//debug_printf("%s,%d,pFuncCode_List_Temp->u8MBFunCode = %d\n",__FUNCTION__,__LINE__,pFuncCode_List_Temp->u8MBFunCode);	
	pChannel_List_Temp = pChannel_Temp;
	while(pChannel_List_Temp != NULL)
	{
		u8ChannelX = pChannel_List_Temp->u8Channel;
		Offset += pFuncCode_List_Temp->Offset[u8ChannelX];
		Len = 0;
		if(PROTOCOL_MODBUS == pChannel_List_Temp->u8Protocol)
		{
			pSlave_List_Temp = pChannel_List_Temp->unDeviceType.pModBus_List;
			while(pSlave_List_Temp != NULL)
			{
				pSlave_FunCode_List_Temp = pSlave_List_Temp->pFunCode_List;
				if(pSlave_FunCode_List_Temp->u8MBFunCode == 0x03)
				{
					while(pSlave_FunCode_List_Temp != NULL)
					{
						pRegister_List_Temp = pSlave_FunCode_List_Temp->pRegister_List;
						while(pRegister_List_Temp != NULL)
						{
							if(pRegister_List_Temp->object_type == OBJECT_ANALOG_INPUT)
							{
								int Reglen = 0;
								Reglen = pRegister_List_Temp->u16RegNum*2;
								memcpy(&pTemp,pData+Offset+Len,Reglen);
								Len += Reglen;
								//debug_printf("%s,%d,%x,%\n",__FUNCTION__,__LINE__,pTemp);
								pTemp = htons(pTemp);
     								AI_Descr[i].Present_Value = (float)pTemp*0.01;
								AI_Descr_Instance[i] = pRegister_List_Temp->object_instance;
								AI_Descr[i].Units = pRegister_List_Temp->units;
								i++;
								//debug_printf("%s,%d,%f,%d\n",__FUNCTION__,__LINE__,AI_Descr[i].Present_Value,AI_Descr[i].Units);
							}
							else if(pRegister_List_Temp->object_type == OBJECT_ANALOG_OUTPUT)
							{

							}
							else if(pRegister_List_Temp->object_type == OBJECT_ANALOG_VALUE)
							{
								int Reglen = 0;
								Reglen = pRegister_List_Temp->u16RegNum*2;
								memcpy(&pTemp,pData+Offset+Len,Reglen);
								Len += Reglen;
								//debug_printf("%s,%d,%x,%\n",__FUNCTION__,__LINE__,pTemp);
								pTemp = htons(pTemp);
     								AV_Descr[j].Present_Value = (float)pTemp*0.1;
								AV_Descr_Instance[j] = pRegister_List_Temp->object_instance;
								AV_Descr[j].Units =  pRegister_List_Temp->units;
								j++;
								//debug_printf("%s,%d,%f,%d\n",__FUNCTION__,__LINE__,AV_Descr[j].Present_Value,AV_Descr[j].Units);
							}
							else if(pRegister_List_Temp->object_type == OBJECT_BINARY_INPUT)
							{

							}
							else if(pRegister_List_Temp->object_type == OBJECT_BINARY_OUTPUT)
							{

							}
							else if(pRegister_List_Temp->object_type == OBJECT_BINARY_VALUE)
							{

							}
							else if(pRegister_List_Temp->object_type == OBJECT_MULTI_STATE_INPUT)
							{

							}
							else if(pRegister_List_Temp->object_type == OBJECT_MULTI_STATE_OUTPUT)
							{

							}
							else if(pRegister_List_Temp->object_type == OBJECT_MULTI_STATE_VALUE)
							{

							}
							pRegister_List_Temp = pRegister_List_Temp->next;
						}
						pSlave_FunCode_List_Temp = pSlave_FunCode_List_Temp->next;
					}
				}
				pSlave_List_Temp = pSlave_List_Temp->next;
			}
		}
		pChannel_List_Temp = pChannel_List_Temp->next;
	}
	*/
	for(bacnet_register_number = 0;bacnet_register_number < Bacnet_Register_List_Table_Number;  bacnet_register_number++)
	{
		if(Bacnet_Register_List_Table[bacnet_register_number].Object_Type == OBJECT_ANALOG_INPUT)
		{
     			AI_Descr[i].Present_Value = Bacnet_Register_List_Table[bacnet_register_number].Present_Value;
			AI_Descr_Instance[i] = Bacnet_Register_List_Table[bacnet_register_number].Object_Instance;
			AI_Descr[i].Units = Bacnet_Register_List_Table[bacnet_register_number].Units;
			
			debug_printf("%s,%d,i = %d,%f,%d\n",__FUNCTION__,__LINE__,i,AI_Descr[i].Present_Value,AI_Descr[i].Units);
			i++;
		}
		else if(Bacnet_Register_List_Table[bacnet_register_number].Object_Type == OBJECT_ANALOG_OUTPUT)
		{

		}
		else if(Bacnet_Register_List_Table[bacnet_register_number].Object_Type == OBJECT_ANALOG_VALUE)
		{

		}
		else if(Bacnet_Register_List_Table[bacnet_register_number].Object_Type == OBJECT_BINARY_INPUT)
		{

		}
		else if(Bacnet_Register_List_Table[bacnet_register_number].Object_Type == OBJECT_BINARY_OUTPUT)
		{

		}
		else if(Bacnet_Register_List_Table[bacnet_register_number].Object_Type == OBJECT_BINARY_VALUE)
		{

		}
		else if(Bacnet_Register_List_Table[bacnet_register_number].Object_Type == OBJECT_MULTI_STATE_INPUT)
		{

		}
		else if(Bacnet_Register_List_Table[bacnet_register_number].Object_Type == OBJECT_MULTI_STATE_OUTPUT)
		{

		}
		else if(Bacnet_Register_List_Table[bacnet_register_number].Object_Type == OBJECT_MULTI_STATE_VALUE)
		{

		}
		
	}
	
	return ;
}


/** @file server/main.c  Example server application using the BACnet Stack. */

/* (Doxygen note: The next two lines pull all the following Javadoc
 *  into the ServerDemo module.) */
/** @addtogroup ServerDemo */
/*@{*/

/** Buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = {0};

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* handle i-am to support binding to other devices */
    //apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        						 handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
        						 handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
        						 handler_write_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE,
        						 handler_write_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_RANGE,
        						 handler_read_range);
#if defined(BACFILE)
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_READ_FILE,
        						 handler_atomic_read_file);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_WRITE_FILE,
        						 handler_atomic_write_file);
#endif
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
        						 handler_reinitialize_device);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION,
        						     handler_timesync_utc);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION,
        						     handler_timesync);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV,
       							 handler_cov_subscribe);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_COV_NOTIFICATION,
       							     handler_ucov_notification);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        						 handler_device_communication_control);
    /* handle the data coming back from private requests */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_PRIVATE_TRANSFER,
        						     handler_unconfirmed_private_transfer);
#if defined(INTRINSIC_REPORTING)
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM,
        						 handler_alarm_ack);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_GET_EVENT_INFORMATION,
        						 handler_get_event_information);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_GET_ALARM_SUMMARY,
        						 handler_get_alarm_summary);
#endif /* defined(INTRINSIC_REPORTING) */
}

ENN_ErrorCode_t ENNBacNET_Server_BacNet_IP_Handler(void)
{
	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	
	BACNET_ADDRESS src = {0};	
	/* address where message came from */
	uint16_t pdu_len = 0;
	unsigned timeout = 1;		/* milliseconds */
	time_t last_seconds = 0;
	time_t current_seconds = 0;
	uint32_t elapsed_seconds = 0;
	uint32_t elapsed_milliseconds = 0;
	uint32_t address_binding_tmr = 0;
	uint32_t recipient_scan_tmr = 0;
	
	Channel_List *pChannel_Temp = NULL;
	FunCode_List  *pFuncCode_List_Temp = NULL;
	pChannel_Temp = gChannel_List_Head;
	pFuncCode_List_Temp = gFunCode_List_head;
	
	int i = 0;
	/* allow the device ID to be set */
	Device_Set_Object_Instance_Number(strtol("1234", NULL, 0));
	printf("BACnet Server Demo\n" "BACnet Stack Version %s\n"
       	           "BACnet Device ID: %u\n" "Max APDU: %d\n", BACnet_Version,
                   Device_Object_Instance_Number(), MAX_APDU);

	/* load any static address bindings to show up in our device bindings list */
	//address_init();
        Init_Service_Handlers();
        dlenv_init_bacnet_ip();
        atexit(datalink_cleanup_bacnet_ip);
	 /* configure the timeout values */
        last_seconds = time(NULL);
	 /* broadcast an I-Am on startup */
	//Send_I_Am(&Handler_Transmit_Buffer[0]);
	/* loop forever */
	 while(1)
	 {
        	/* input */
        	current_seconds = time(NULL);
		//更新对象的数据及单位	
		ENNBacnet_Object_instance_Acquire_Data(pChannel_Temp,pFuncCode_List_Temp,Bacnet_Register_List_Table);
		
        	/* returns 0 bytes on timeout */
		//Rx_Buf返回updu和APDU内容，pdu_len是返回自己长度，src是请求方的源地址
        	pdu_len = datalink_receive_bacnet_ip(&src, &Rx_Buf[0], MAX_MPDU, timeout);
		debug_printf("%s,%d,npdu_len = %d\n",__FUNCTION__,__LINE__,pdu_len);
		for(i = 0; i < pdu_len; i++)
		{
			debug_printf("%02x ",Rx_Buf[i]);
		}
		debug_printf("\n");
        	/* process */
        	if (pdu_len) 
		{
            		npdu_handler(&src, &Rx_Buf[0], pdu_len);
        	}
        	/* at least one second has passed */
        	elapsed_seconds = (uint32_t) (current_seconds - last_seconds);
        	if (elapsed_seconds) 
		{
            		last_seconds = current_seconds;
            		dcc_timer_seconds(elapsed_seconds);
#if defined(BACDL_BIP) && BBMD_ENABLED
            		bvlc_maintenance_timer(elapsed_seconds);
#endif
            		dlenv_maintenance_timer(elapsed_seconds);
            		Load_Control_State_Machine_Handler();
            		elapsed_milliseconds = elapsed_seconds * 1000;
            		handler_cov_timer_seconds(elapsed_seconds);
            		tsm_timer_milliseconds(elapsed_milliseconds);
            		trend_log_timer(elapsed_seconds);
#if defined(INTRINSIC_REPORTING)
            		Device_local_reporting();
#endif
        	}
        	handler_cov_task();
        	/* scan cache address */
        	address_binding_tmr += elapsed_seconds;
        	if (address_binding_tmr >= 60) 
		{
            		address_cache_timer(address_binding_tmr);
            		address_binding_tmr = 0;
        	}
#if defined(INTRINSIC_REPORTING)
        	/* try to find addresses of recipients */
        	recipient_scan_tmr += elapsed_seconds;
        	if (recipient_scan_tmr >= NC_RESCAN_RECIPIENTS_SECS) 
		{
            		Notification_Class_find_recipient();
            		recipient_scan_tmr = 0;
        	}
#endif
        /* output */

        /* blink LEDs, Turn on or off outputs, etc */
	}
	return ENN_SUCCESS;
}

ENN_ErrorCode_t initENNBacNet_Task()
{
	ENNOS_TASK_t taskID = 0;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	Channel_List *pChannel_Temp = NULL;
	FunCode_List  *pFuncCode_List_Temp = NULL;
	
	pChannel_Temp = gChannel_List_Head;
	pFuncCode_List_Temp = gFunCode_List_head;

	ENNBacnet_Object_instance_Statistical(pChannel_Temp,Bacnet_Register_List_Table);
	
	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	returnCode = ENNOS_CreateTask("BACNET_IP_HANDLER", ENNOS_TASK_PRIORITY_MIDDLE, 
								    4*1024, &taskID, ENNOS_TASK_START, (void*)ENNBacNET_Server_BacNet_IP_Handler, NULL);
	if(ENN_SUCCESS != returnCode)
	{
		ENNTRACE("\nCreate TCP SERVER task fail!\n");
		return returnCode;
	}

	return ENN_SUCCESS;
}


