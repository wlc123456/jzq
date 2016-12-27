#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>       /* for time */
#include <errno.h>

#include "ennAPI.h"
#include "ennSocket.h"
#include "ModBus_TCP.h"
#include "ModBus_Slave_Table.h"
#include "IEC102_Main.h"
#include "IEC102protocol.h"

#include "bactext.h"
#include "iam.h"
#include "address.h"
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "device-client.h"
#include "datalink.h"
#include "bactext.h"
#include "whois.h"
#include "arf.h"
#include "tsm.h"
#include "bacerror.h"
#include "bacdef.h"


/* some demo stuff needed */
#include "rpm.h"
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"
#if defined(BACDL_MSTP)
#include "rs485.h"
#endif
#include "dlenv.h"
#include "net.h"

int register_number = 0;
extern int  Bacnet_Register_List_Table_Number ;
extern Bacnet_Register_List  Bacnet_Register_List_Table[256] ;

int FLAG  = 0;
/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static int32_t Target_Object_Instance_Min = 0;
static int32_t Target_Object_Instance_Max = 100;
static bool Error_Detected = false;

#define BAC_ADDRESS_INUSE 1
#define BAC_ADDRESS_MULT 2

static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;  //(device)Éè±¸¶ÔÏóÊµÀýºÅ
static uint32_t Target_Object_Instance = BACNET_MAX_INSTANCE;//¶ÔÏóÊµÀýºÅ
static BACNET_OBJECT_TYPE Target_Object_Type = OBJECT_ANALOG_INPUT;//¶ÔÏóÀàÐÍ
static BACNET_PROPERTY_ID Target_Object_Property = PROP_ACKED_TRANSITIONS;//¶ÔÏóÊôÐÔ
static int32_t Target_Object_Index = BACNET_ARRAY_ALL;//¶ÔÏóÊôÐÔË÷ÒýÖµ(Õë¶ÔËØ×éÀàÐÍ)
/* the invoke id is needed to filter incoming messages */
static BACNET_ADDRESS Target_Address;
static uint8_t Request_Invoke_ID = 0;

pthread_mutex_t mutex;

typedef  struct  Object_Instance_Value_Units
{
	BACNET_OBJECT_TYPE   object_type;                       //bacnet¶ÔÏóÀàÐÍ
	ENN_U32	object_instance;                               //bacnet¶ÔÏóÊµÀý
	float value;
	BACNET_ENGINEERING_UNITS   units;                   //¶ÔÓ¦¼Ä´æÆ÷Êý¾ÝµÄµ¥Î»
	struct  Object_Instance_Value_Units   *next;
}OBJECT_INSTANCE_VALUE_UNITS;


OBJECT_INSTANCE_VALUE_UNITS  *pHEAD_OBJECT_DATA = NULL;
OBJECT_INSTANCE_VALUE_UNITS  *pTAIL_OBJECT_DATA = NULL;

BACNET_READ_ACCESS_DATA *pHead_List_Read_Access_Data = NULL;
BACNET_READ_ACCESS_DATA *pTail_List_Read_Access_Data = NULL;

BACNET_READ_PROPERTY_DATA data;


uint32_t  Object_Num = 0;
uint16_t type_temp = 0;
uint32_t instance_temp = 0;

typedef struct BACnet_Object_Id_X {
    uint16_t type;
    uint32_t instance; 
    struct BACnet_Object_Id_X *next;
} BACNET_OBJECT_ID_X;


typedef struct address_entry {
    struct address_entry *next;
    uint8_t Flags;
    uint32_t device_id;
    unsigned max_apdu;
    BACNET_ADDRESS address;
    uint32_t  Object_Num;
    BACNET_OBJECT_ID_X *HEAD;
    BACNET_OBJECT_ID_X *TAIL;    
} ADDRESS_ENTRY;

 struct address_table {
    struct address_entry *first;
    struct address_entry *last;
} Address_Table = {0};

struct address_entry *alloc_address_entry(void)
{
    struct address_entry *rval;
    rval = (struct address_entry *) calloc(1, sizeof(struct address_entry));
    if (Address_Table.first == 0) 
    {
        Address_Table.first = Address_Table.last = rval;
    } 
    else 
    {
        Address_Table.last->next = rval;
        Address_Table.last = rval;
    }
    return rval;
}

bool bacnet_address_match(BACNET_ADDRESS * a1,BACNET_ADDRESS * a2)
{
    int i = 0;
    if (a1->net != a2->net)
        return false;
    if (a1->len != a2->len)
        return false;
    for (; i < a1->len; i++)
        if (a1->adr[i] != a2->adr[i])
            return false;
    return true;
}

void address_table_add(uint32_t device_id,unsigned max_apdu,BACNET_ADDRESS * src)
{
    struct address_entry *pMatch;
    uint8_t flags = 0;

    pMatch = Address_Table.first;
    printf("%s,%d,pMatch = %d\n",__FUNCTION__,__LINE__,pMatch);
    while (pMatch) 
    {
	debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
        if (pMatch->device_id == device_id) 
	{
            if (bacnet_address_match(&pMatch->address, src))
                return;
            flags |= BAC_ADDRESS_MULT;
            pMatch->Flags |= BAC_ADDRESS_MULT;
        }
        pMatch = pMatch->next;
    }

    pMatch = alloc_address_entry();

    pMatch->Flags = BAC_ADDRESS_INUSE ;
    pMatch->device_id = device_id;
    pMatch->max_apdu = max_apdu;
    pMatch->address = *src;
    pMatch->HEAD = NULL;
    pMatch->TAIL = NULL;

    return;
}

void my_i_am_handler(uint8_t * service_request,uint16_t service_len,BACNET_ADDRESS * src)
{
    int len = 0;
    uint32_t device_id = 0;
    unsigned max_apdu = 0;
    int segmentation = 0;
    uint16_t vendor_id = 0;
    int i = 0;

    //(void) service_len;
    len = iam_decode_service_request(service_request, &device_id, &max_apdu,&segmentation, &vendor_id);
    debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
    debug_printf("device_id = %u,max_apdu = %u,segmentation = %u,vendor_id = %u\n",device_id,max_apdu,segmentation,vendor_id);
#if PRINT_ENABLED
    fprintf(stderr, "Received I-Am Request");
#endif
    if (len != -1) 
    {
#if PRINT_ENABLED
/*
        fprintf(stderr, " from %lu, MAC = %d.%d.%d.%d.%d.%d\n",
            	  (unsigned long) device_id, src->mac[0], src->mac[1], src->mac[2],
                  src->mac[3], src->mac[4], src->mac[5]);
                  */
#endif
        debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
    	printf("src->mac_len = %u\n",src->mac_len);
    	for(i = 0; i< MAX_MAC_LEN;i++)
    	{
		printf("%u:",src->mac[i]);
    	}
    	printf("\n");
    	printf("src->net = %u\n",src->net);
    	printf("src->len = %u\n",src->len);
    	for(i = 0; i< MAX_MAC_LEN;i++)
    	{
		printf("%u:",src->adr[i]);
    	}
    	printf("\n");
        address_table_add(device_id, max_apdu, src);
    } 
    else 
    {
#if PRINT_ENABLED
        fprintf(stderr, ", but unable to decode it.\n");
#endif
    }
    return;
}

void MyAbortHandler(BACNET_ADDRESS * src,uint8_t invoke_id,uint8_t abort_reason,bool server)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    (void) server;
    fprintf(stderr, "BACnet Abort: %s\r\n",
        bactext_abort_reason_name(abort_reason));
    Error_Detected = true;
}

void MyRejectHandler(BACNET_ADDRESS * src,uint8_t invoke_id,uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    fprintf(stderr, "BACnet Reject: %s\r\n",
        bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

/** Handler for a ReadProperty ACK.
 * @ingroup DSRP
 * Doesn't actually do anything, except, for debugging, to
 * print out the ACK data of a matching request.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
 /********************************************************************
* º¯ Êý Ãû    :My_Read_Property_Ack_Handler
* º¯Êý½éÉÜ:	
* ÊäÈë²ÎÊý:	service_request:Ö¸Ïòapdu ÓÃ»§Êý¾Ý²¿·ÖµÄÊ×µØÖ·µÄÖ¸Õë
*					service_request_len:apduÐ­ÒéÊý¾Ýµ¥ÔªµÄÓÃ»§Êý¾Ý²¿·ÖÊý¾ÝµÄ³¤¶
*					src:·¢ËÍ·¢µÄµØÖ·ÐÅÏ¢
*					service_data:Ó¦´ðÐÅÏ¢µÄapciµÄÐÅÏ¢ÄÚÈÝ
* Êä³ö²ÎÊý:	ÎÞ	
* ·µ »Ø Öµ    :	ÎÞ	
********************************************************************/
void My_Read_Property_Ack_Handler(uint8_t * service_request,
    												   uint16_t service_len,
    												   BACNET_ADDRESS * src,
                           									   BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data)
{
    int len = 0;
    BACNET_READ_PROPERTY_DATA data;
    debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
    if (address_match(&Target_Address, src) &&(service_data->invoke_id == Request_Invoke_ID)) 
    {
	 debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
        len = rp_ack_decode_service_request(service_request, service_len, &data);
        if (len > 0) 
	{
	       debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
               rp_ack_print_data(&data);
          
        }
    }
}

void information_copy(BACNET_READ_ACCESS_DATA *rpm_data)
{
	OBJECT_INSTANCE_VALUE_UNITS  *object_data_add = NULL; 
    	BACNET_PROPERTY_REFERENCE * rpm_property_temp = NULL;

	object_data_add = (OBJECT_INSTANCE_VALUE_UNITS *)calloc(1,sizeof(OBJECT_INSTANCE_VALUE_UNITS));
	if(object_data_add)
	{
		object_data_add->object_type = rpm_data->object_type;
		object_data_add->object_instance = rpm_data->object_instance;
		rpm_property_temp = rpm_data->listOfProperties;
		if(rpm_property_temp)
		{
			object_data_add->value = rpm_property_temp->value->type.Real;
			rpm_property_temp = rpm_property_temp->next;
			object_data_add->units = rpm_property_temp->value->type.Enumerated;
		}
		debug_printf("%s,%d,object_data_add->object_type = %d,object_data_add->object_instance = %u,object_data_add->value = %f,object_data_add->units = %d\n ",
					__FUNCTION__,__LINE__,object_data_add->object_type,object_data_add->object_instance,object_data_add->value,object_data_add->units );
	}
	else
	{
				
	}
	
	Bacnet_Register_List_Table[register_number].Object_Type = object_data_add->object_type ;
	Bacnet_Register_List_Table[register_number].Object_Instance = object_data_add->object_instance;
	Bacnet_Register_List_Table[register_number].Present_Value = object_data_add->value;
	Bacnet_Register_List_Table[register_number].Units = object_data_add->units;
	debug_printf("%s,%d, %d,%d,%u,%f,%d\n",__FUNCTION__,__LINE__,register_number,
										Bacnet_Register_List_Table[register_number].Object_Type,
										Bacnet_Register_List_Table[register_number].Object_Instance ,
										Bacnet_Register_List_Table[register_number].Present_Value,
										Bacnet_Register_List_Table[register_number].Units);
	register_number++;
	
	
	if(  pHEAD_OBJECT_DATA ==  NULL)
	{
		pHEAD_OBJECT_DATA = object_data_add;
		pTAIL_OBJECT_DATA = object_data_add;
	}
	else
	{
		pTAIL_OBJECT_DATA->next = object_data_add;
		pTAIL_OBJECT_DATA = object_data_add;
	}
	
}
/** Handler for a ReadPropertyMultiple ACK.
 * @ingroup DSRPM
 * For each read property, print out the ACK'd data,
 * and free the request data items from linked property list.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
 /********************************************************************
* º¯ Êý Ãû    :My_Read_Property_Multiple_Ack_Handler
* º¯Êý½éÉÜ:	
* ÊäÈë²ÎÊý:	service_request:Ö¸Ïòapdu ÓÃ»§Êý¾Ý²¿·ÖµÄÊ×µØÖ·µÄÖ¸Õë
*					service_request_len:apduÐ­ÒéÊý¾Ýµ¥ÔªµÄÓÃ»§Êý¾Ý²¿·ÖÊý¾ÝµÄ³¤¶
*					src:·¢ËÍ·¢µÄµØÖ·ÐÅÏ¢
*					service_data:Ó¦´ðÐÅÏ¢µÄapciµÄÐÅÏ¢ÄÚÈÝ
* Êä³ö²ÎÊý:	ÎÞ	
* ·µ »Ø Öµ    :	ÎÞ	
********************************************************************/
void My_Read_Property_Multiple_Ack_Handler(uint8_t * service_request,
    															     uint16_t service_len,
                                        										     BACNET_ADDRESS * src,
    															     BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data)
{
    int len = 0;
    BACNET_READ_ACCESS_DATA *rpm_data;
    BACNET_READ_ACCESS_DATA *old_rpm_data;
    BACNET_PROPERTY_REFERENCE *rpm_property;
    BACNET_PROPERTY_REFERENCE *old_rpm_property;
    BACNET_APPLICATION_DATA_VALUE *value;
    BACNET_APPLICATION_DATA_VALUE *old_value;
   

    if (address_match(&Target_Address, src) &&(service_data->invoke_id == Request_Invoke_ID)) 
    {
        rpm_data = calloc(1, sizeof(BACNET_READ_ACCESS_DATA));
        if (rpm_data) 
	{
            	len = rpm_ack_decode_service_request(service_request, service_len,rpm_data);
		information_copy(rpm_data);
        }
        if (len > 0) 
	{
            	while (rpm_data) 
		{
                	rpm_ack_print_data(rpm_data);
                	rpm_property = rpm_data->listOfProperties;
                	while (rpm_property) 
			{
                    		value = rpm_property->value;
                    		while (value) 
				{
                        		old_value = value;
                        		value = value->next;
                        		free(old_value);
                   		}
                    		old_rpm_property = rpm_property;
                    		rpm_property = rpm_property->next;
                    		free(old_rpm_property);
                	}
                	old_rpm_data = rpm_data;
                	rpm_data = rpm_data->next;
                	free(old_rpm_data);
            	}
        } 
	else 
	{
            	fprintf(stderr, "RPM Ack Malformed! Freeing memory...\n");
            	while (rpm_data) 
		{
                	rpm_property = rpm_data->listOfProperties;
                	while (rpm_property) 
			{
                    		value = rpm_property->value;
                    		while (value) 
				{
                        		old_value = value;
                        		value = value->next;
                        		free(old_value);
                    		}
                    		old_rpm_property = rpm_property;
                    		rpm_property = rpm_property->next;
                    		free(old_rpm_property);
                	}
                	old_rpm_data = rpm_data;
                	rpm_data = rpm_data->next;
                	free(old_rpm_data);
            	}
        }
    }
}

static void init_service_handlers(void)
{
    Device_Init_client(NULL);
    /* Note: this applications doesn't need to handle who-is
       it is confusing for the user! */
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    //apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,handler_read_property);
    /* handle the reply (request) coming back */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, my_i_am_handler);
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        							 My_Read_Property_Ack_Handler);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
        							 My_Read_Property_Multiple_Ack_Handler);
    /* handle any errors coming back */
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}



ENN_ErrorCode_t ENNBacNET_Client_BacNet_Mstp_Handler(void)
{
	BACNET_ADDRESS src = {0};  /* address where message came from */
    	uint16_t pdu_len = 0;
    	unsigned timeout = 1000;     /* milliseconds *///Õâ¸öÊ±¼äÊÇÓÃÀ´±íÊ¾½ÓÊÜµÈ´ýÊ±¼äµÄ£¬Ê±¼äµ½Ã»ÓÐÏûÏ¢¾Í·µ»Ø¡£
    	time_t total_seconds = 0;
    	time_t elapsed_seconds = 0;
    	time_t last_seconds = 0;
    	time_t current_seconds = 0;
    	time_t timeout_seconds = 0;
	time_t onetime =0;
	time_t twotime = 0;
    	BACNET_ADDRESS dest;
    	int argi;
	unsigned max_apdu = 0;
	uint8_t buffer[MAX_PDU] = {0};
	
	int i = 0;
	uint32_t objectnum = 0;
        ADDRESS_ENTRY *p_addr_entry;
	uint32_t device_id = 0;
	BACNET_OBJECT_ID_X *pObject_Id_Add = NULL;
	BACNET_OBJECT_ID_X *pObject_Id_Temp = NULL;
	BACNET_OBJECT_ID_X *pObject_Id_Temp_Old = NULL;

	BACNET_READ_ACCESS_DATA *pRead_Access_Data_Add = NULL;
	BACNET_READ_ACCESS_DATA *rpm_object = NULL;
    	BACNET_READ_ACCESS_DATA *old_rpm_object = NULL;
    	BACNET_PROPERTY_REFERENCE *rpm_property = NULL;
    	BACNET_PROPERTY_REFERENCE *old_rpm_property = NULL;
	OBJECT_INSTANCE_VALUE_UNITS *pObject_Inatance_Value_Units_Temp = NULL;
	OBJECT_INSTANCE_VALUE_UNITS *pObject_Inatance_Value_Units_Temp_Old = NULL;
	 
	datalink_get_broadcast_address_bacnet_mstp(&dest);
	Device_Set_Object_Instance_Number_client(BACNET_MAX_INSTANCE);
    	init_service_handlers();
    	address_init();
    	dlenv_init_bacnet_mstp();
	atexit(datalink_cleanup_bacnet_mstp);
    	/* configure the timeout values */
    	last_seconds = time(NULL);
   	timeout_seconds = apdu_timeout() / 1000;
	/* send the request */
    	Send_WhoIs_To_Network(&dest, Target_Object_Instance_Min,Target_Object_Instance_Max);
	
	pthread_mutex_init(&mutex,NULL);
    	/* loop forever */
	
   	 while(1)
    	{	
		
		/* returns 0 bytes on timeout */
		//src :ÐÅÏ¢·¢ËÍ·¢µÄsrcµØÖ·
        	pdu_len = datalink_receive_bacnet_mstp(&src, &Rx_Buf[0], MAX_MPDU, timeout);
		//debug_printf("%s,%d,mac = %d,pdu_len = %d\n",__FUNCTION__,__LINE__,src.mac[0],pdu_len);
        	/* process */
        	if (pdu_len) 
		{
            		npdu_handler(&src, &Rx_Buf[0], pdu_len);
        	}
        	if (Error_Detected)
        	{
			debug_printf("%s,%d,Error_Detected = %d\n",__FUNCTION__,__LINE__,Error_Detected);
			break;
		}
		
		p_addr_entry = Address_Table.first;
		while(p_addr_entry)
		{
			
			device_id = p_addr_entry->device_id;
			Target_Address= p_addr_entry->address;
				
			
			/* increment timer - exit if timed out */
			current_seconds = time(NULL);
			/* at least one second has passed */
        		if (current_seconds != last_seconds)
        		{
				debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
				tsm_timer_milliseconds((uint16_t) ((current_seconds -last_seconds) * 1000));
			}
			
	    		Request_Invoke_ID = Send_Read_Property_Request(device_id ,8, device_id,76, 0);
			debug_printf("%s,%d,Request_Invoke_ID =  %u\n",__FUNCTION__,__LINE__,Request_Invoke_ID);
           		
	    		 /* returns 0 bytes on timeout */
        		pdu_len = datalink_receive_bacnet_mstp(&src, &Rx_Buf[0], MAX_MPDU, timeout);
			/* process */
        		if (pdu_len) 
			{
            			npdu_handler(&src, &Rx_Buf[0], pdu_len);
        		}
        		/* keep track of time for next check */
        		last_seconds = current_seconds;
			p_addr_entry->Object_Num = Object_Num;
			
			for(objectnum = 2;objectnum < p_addr_entry->Object_Num+1;objectnum ++)
			{
				/* increment timer - exit if timed out */
				current_seconds = time(NULL);
				/* at least one second has passed */
        			if (current_seconds != last_seconds)
        			{
					debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
					tsm_timer_milliseconds((uint16_t) ((current_seconds -last_seconds) * 1000));
				}
		
                		Request_Invoke_ID = Send_Read_Property_Request(device_id ,8, device_id,76, objectnum );
				debug_printf("%s,%d,Request_Invoke_ID =  %u\n",__FUNCTION__,__LINE__,Request_Invoke_ID);
           		  	
	    		 	/* returns 0 bytes on timeout */
        			pdu_len = datalink_receive_bacnet_mstp(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        			/* process */
        			if (pdu_len) 
				{
            				npdu_handler(&src, &Rx_Buf[0], pdu_len);
        			}
				
				pObject_Id_Add = (BACNET_OBJECT_ID_X *)calloc(1,sizeof(BACNET_OBJECT_ID_X));
				pObject_Id_Add ->type = type_temp ;
				pObject_Id_Add ->instance = instance_temp;
				type_temp = 0;
				instance_temp = 0;
				if(p_addr_entry->HEAD == NULL)
				{
					p_addr_entry->HEAD = pObject_Id_Add;
					p_addr_entry->TAIL = pObject_Id_Add;
				}
				else
				{
					p_addr_entry->TAIL->next = pObject_Id_Add;
					p_addr_entry->TAIL = pObject_Id_Add;
				}
				/* keep track of time for next check */
				
        			last_seconds = current_seconds;
			}
			
			pObject_Id_Temp = p_addr_entry->HEAD;
			while(pObject_Id_Temp)
			{
				debug_printf("%s,%d,type = %u,instance = %u\n",__FUNCTION__,__LINE__,pObject_Id_Temp->type,pObject_Id_Temp->instance);
				pObject_Id_Temp = pObject_Id_Temp->next;
			}

			pObject_Id_Temp = p_addr_entry->HEAD;
			while(pObject_Id_Temp)
			{
				pRead_Access_Data_Add = (BACNET_READ_ACCESS_DATA *)calloc(1, sizeof(BACNET_READ_ACCESS_DATA));
				
				if(pRead_Access_Data_Add == NULL)
				{
					perror("calloc\n");
					return -1;
				}
				else
				{
					pRead_Access_Data_Add ->object_type = pObject_Id_Temp->type;
					debug_printf("%s,%d,%u\n",__FUNCTION__,__LINE__,pRead_Access_Data_Add ->object_type);
					pRead_Access_Data_Add->object_instance = pObject_Id_Temp->instance;
					debug_printf("%s,%d,%u\n",__FUNCTION__,__LINE__,pRead_Access_Data_Add->object_instance);
					rpm_property = (BACNET_PROPERTY_REFERENCE *)calloc(1, sizeof(BACNET_PROPERTY_REFERENCE));
					pRead_Access_Data_Add->listOfProperties = rpm_property;
					rpm_property->propertyIdentifier = 85;
					rpm_property->propertyArrayIndex = BACNET_ARRAY_ALL;
					old_rpm_property = (BACNET_PROPERTY_REFERENCE *)calloc(1, sizeof(BACNET_PROPERTY_REFERENCE));
					rpm_property->next = old_rpm_property;
					old_rpm_property->propertyIdentifier = 117;
					old_rpm_property->propertyArrayIndex = BACNET_ARRAY_ALL;
					old_rpm_property->next = NULL;
				}
				
				if(pHead_List_Read_Access_Data ==  NULL)
				{
					pHead_List_Read_Access_Data = pRead_Access_Data_Add;
					pTail_List_Read_Access_Data = pRead_Access_Data_Add;
				}
				else
				{
					pTail_List_Read_Access_Data->next = pRead_Access_Data_Add;
					pTail_List_Read_Access_Data = pRead_Access_Data_Add;
				}
				
				
				 /* increment timer - exit if timed out */
				current_seconds = time(NULL);
				 /* at least one second has passed */
        			if (current_seconds != last_seconds)
            				tsm_timer_milliseconds(((current_seconds - last_seconds) * 1000));
					
				Request_Invoke_ID = Send_Read_Property_Multiple_Request(&buffer[0],
                    													      		      sizeof(buffer), 
                    													                      device_id,
															                      pRead_Access_Data_Add);
				debug_printf("%s,%d,Request_Invoke_ID =  %u\n",__FUNCTION__,__LINE__,Request_Invoke_ID);	
				
				/* returns 0 bytes on timeout */
        			pdu_len = datalink_receive_bacnet_mstp(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        			/* process */
        			if (pdu_len) 
				{
            				npdu_handler(&src, &Rx_Buf[0], pdu_len);
        			}
				/* keep track of time for next check */
        			last_seconds = current_seconds;
				
				
				pObject_Id_Temp = pObject_Id_Temp->next;
			}
			
			Bacnet_Register_List_Table_Number = register_number;
			register_number = 0;
			rpm_object = pHead_List_Read_Access_Data;
			while(rpm_object)
			{
				rpm_property = rpm_object->listOfProperties;
				while(rpm_property)
				{
					old_rpm_property = rpm_property;
					rpm_property = rpm_property->next;
					free(old_rpm_property);
				}
					
				old_rpm_object = rpm_object;
				rpm_object = rpm_object->next;
				free(old_rpm_object);
			}
			pHead_List_Read_Access_Data = NULL;
			pTail_List_Read_Access_Data = NULL;
			
			pObject_Id_Temp = p_addr_entry->HEAD;
			while(pObject_Id_Temp)
			{
				pObject_Id_Temp_Old = pObject_Id_Temp;
				pObject_Id_Temp = pObject_Id_Temp->next;
				free(pObject_Id_Temp_Old);
			}
			p_addr_entry->HEAD = NULL;
			p_addr_entry->TAIL = NULL;

			pObject_Inatance_Value_Units_Temp  = pHEAD_OBJECT_DATA;
			while(pObject_Inatance_Value_Units_Temp)
			{
				pObject_Inatance_Value_Units_Temp_Old = pObject_Inatance_Value_Units_Temp;
				pObject_Inatance_Value_Units_Temp = pObject_Inatance_Value_Units_Temp->next;
				free(pObject_Inatance_Value_Units_Temp_Old);
			}
			pHEAD_OBJECT_DATA = NULL;
			pTAIL_OBJECT_DATA = NULL;
			p_addr_entry = p_addr_entry->next;

			if(!p_addr_entry)
			{
				if(FLAG == 0)
				{
					initENNBacNet_Task();
					FLAG = 1;
				}
			}
			
		}
		
    	}
	return ENN_SUCCESS;
}

ENN_ErrorCode_t initENNBacMstp_Task()
{
	ENNOS_TASK_t taskID = 0;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;
	ENNTRACE("%s, %d\n",__FUNCTION__,__LINE__);
	returnCode = ENNOS_CreateTask("BACNET_MSTP_HANDLER", ENNOS_TASK_PRIORITY_MIDDLE, 
								    4*1024, &taskID, ENNOS_TASK_START, (void*)ENNBacNET_Client_BacNet_Mstp_Handler, NULL);
	if(ENN_SUCCESS != returnCode)
	{
		ENNTRACE("\nCreate TCP SERVER task fail!\n");
		return returnCode;
	}

	/*
	returnCode = ENNOS_CreateTask("BACNET_MSTP_REQUEST", ENNOS_TASK_PRIORITY_MIDDLE, 
								    4*1024, &taskID, ENNOS_TASK_START, (void*)ENNBacNET_Client_BacNet_Mstp_Request, NULL);
	if(ENN_SUCCESS != returnCode)
	{
		ENNTRACE("\nCreate TCP SERVER task fail!\n");
		return returnCode;
	}
	*/

	return ENN_SUCCESS;
}

