/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
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
#include <stdlib.h>
#include "config.h"
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "address.h"
#include "tsm.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
#include "bactext.h"
#include "rp.h"
/* some demo stuff needed */
#include "handlers.h"
#include "txbuf.h"
static bool append_str(
    char **str,
    size_t * rem_str_len,
    const char *add_str)
{
    bool retval;
    uint16_t bytes_written;

    bytes_written = snprintf(*str, *rem_str_len, "%s", add_str);
    if ((bytes_written < 0) || (bytes_written >= *rem_str_len)) {
        /* If there was an error or output was truncated, return error */
        retval = false;
    } else {
        /* Successfully wrote the contents to the string. Let's advance the
         * string pointer to the end, and account for the used space */
        *str += bytes_written;
        *rem_str_len -= bytes_written;
        retval = true;
    }

    return retval;
}

int bacapp_snprintf_valuex(char *str,size_t str_len,BACNET_OBJECT_PROPERTY_VALUE * object_value)

{
    size_t len = 0, i = 0;
    char *char_str;
    uint8_t *octet_str;
    BACNET_APPLICATION_DATA_VALUE *value;
    BACNET_PROPERTY_ID property = PROP_ALL;
    BACNET_OBJECT_TYPE object_type = MAX_BACNET_OBJECT_TYPE;
    int ret_val = -1;
    char *p_str = str;
    size_t rem_str_len = str_len;
    char temp_str[32];

    if (object_value && object_value->value) 
    {
        value = object_value->value;
        property = object_value->object_property;
        object_type = object_value->object_type;
        switch (value->tag) 
	{
            case BACNET_APPLICATION_TAG_NULL:
                ret_val = snprintf(str, str_len, "Null");
                break;
            case BACNET_APPLICATION_TAG_BOOLEAN:
                ret_val = (value->type.Boolean) ? snprintf(str, str_len,"TRUE") : snprintf(str, str_len, "FALSE");
                break;
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
		debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
                ret_val = snprintf(str, str_len, "%lu",(unsigned long) value->type.Unsigned_Int);
                break;
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                ret_val =
                    snprintf(str, str_len, "%ld",
                    (long) value->type.Signed_Int);
                break;
            case BACNET_APPLICATION_TAG_REAL:
                ret_val =
                    snprintf(str, str_len, "%f", (double) value->type.Real);
                break;
#if defined (BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                ret_val = snprintf(str, str_len, "%f", value->type.Double);
                break;
#endif
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                len = octetstring_length(&value->type.Octet_String);
                octet_str = octetstring_value(&value->type.Octet_String);
                for (i = 0; i < len; i++) {
                    snprintf(temp_str, sizeof(temp_str), "%02X", *octet_str);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                    octet_str++;
                }
                if (i == len) {
                    /* Everything went fine */
                    ret_val = str_len - rem_str_len;
                }
                break;
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                len = characterstring_length(&value->type.Character_String);
                char_str =
                    characterstring_value(&value->type.Character_String);
                if (!append_str(&p_str, &rem_str_len, "\""))
                    break;
                for (i = 0; i < len; i++) {
                    if (isprint(*((unsigned char *) char_str))) {
                        snprintf(temp_str, sizeof(temp_str), "%c", *char_str);
                    } else {
                        snprintf(temp_str, sizeof(temp_str), "%c", '.');
                    }
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                    char_str++;
                }
                if ((i == len) && append_str(&p_str, &rem_str_len, "\"")
                    ) {
                    /* Everything is fine. Indicate how many bytes were */
                    /* written */
                    ret_val = str_len - rem_str_len;
                }
                break;
            case BACNET_APPLICATION_TAG_BIT_STRING:
                len = bitstring_bits_used(&value->type.Bit_String);
                if (!append_str(&p_str, &rem_str_len, "{"))
                    break;
                for (i = 0; i < len; i++) {
                    snprintf(temp_str, sizeof(temp_str), "%s",
                        bitstring_bit(&value->type.Bit_String,
                            (uint8_t) i) ? "true" : "false");
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                    if (i < len - 1) {
                        if (!append_str(&p_str, &rem_str_len, ","))
                            break;
                    }
                }
                if ((i == len) && append_str(&p_str, &rem_str_len, "}")
                    ) {
                    /* Everything is fine. Indicate how many bytes were */
                    /* written */
                    ret_val = str_len - rem_str_len;
                }
                break;
            case BACNET_APPLICATION_TAG_ENUMERATED:
                switch (property) {
                    case PROP_OBJECT_TYPE:
                        if (value->type.Enumerated < MAX_ASHRAE_OBJECT_TYPE) {
                            ret_val =
                                snprintf(str, str_len, "%s",
                                bactext_object_type_name(value->type.
                                    Enumerated));
                        } else if (value->type.Enumerated < 128) {
                            ret_val =
                                snprintf(str, str_len, "reserved %lu",
                                (unsigned long) value->type.Enumerated);
                        } else {
                            ret_val =
                                snprintf(str, str_len, "proprietary %lu",
                                (unsigned long) value->type.Enumerated);
                        }
                        break;
                    case PROP_EVENT_STATE:
                        ret_val =
                            snprintf(str, str_len, "%s",
                            bactext_event_state_name(value->type.Enumerated));
                        break;
                    case PROP_UNITS:
                        if (value->type.Enumerated < 256) {
                            ret_val =
                                snprintf(str, str_len, "%s",
                                bactext_engineering_unit_name(value->
                                    type.Enumerated));
                        } else {
                            ret_val =
                                snprintf(str, str_len, "proprietary %lu",
                                (unsigned long) value->type.Enumerated);
                        }
                        break;
                    case PROP_POLARITY:
                        ret_val =
                            snprintf(str, str_len, "%s",
                            bactext_binary_polarity_name(value->
                                type.Enumerated));
                        break;
                    case PROP_PRESENT_VALUE:
                    case PROP_RELINQUISH_DEFAULT:
                        if (object_type < OBJECT_PROPRIETARY_MIN) {
                            ret_val =
                                snprintf(str, str_len, "%s",
                                bactext_binary_present_value_name(value->type.
                                    Enumerated));
                        } else {
                            ret_val =
                                snprintf(str, str_len, "%lu",
                                (unsigned long) value->type.Enumerated);
                        }
                        break;
                    case PROP_RELIABILITY:
                        ret_val =
                            snprintf(str, str_len, "%s",
                            bactext_reliability_name(value->type.Enumerated));
                        break;
                    case PROP_SYSTEM_STATUS:
                        ret_val =
                            snprintf(str, str_len, "%s",
                            bactext_device_status_name(value->
                                type.Enumerated));
                        break;
                    case PROP_SEGMENTATION_SUPPORTED:
                        ret_val =
                            snprintf(str, str_len, "%s",
                            bactext_segmentation_name(value->type.Enumerated));
                        break;
                    case PROP_NODE_TYPE:
                        ret_val =
                            snprintf(str, str_len, "%s",
                            bactext_node_type_name(value->type.Enumerated));
                        break;
                    default:
                        ret_val =
                            snprintf(str, str_len, "%lu",
                            (unsigned long) value->type.Enumerated);
                        break;
                }
                break;
            case BACNET_APPLICATION_TAG_DATE:
                if (!append_str(&p_str, &rem_str_len,
                        bactext_day_of_week_name(value->type.Date.wday)
                    )
                    )
                    break;
                if (!append_str(&p_str, &rem_str_len, ", "))
                    break;

                if (!append_str(&p_str, &rem_str_len,
                        bactext_month_name(value->type.Date.month)
                    )
                    )
                    break;
                if (value->type.Date.day == 255) {
                    if (!append_str(&p_str, &rem_str_len, " (unspecified), "))
                        break;
                } else {
                    snprintf(temp_str, sizeof(temp_str), " %u, ",
                        (unsigned) value->type.Date.day);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                }
                if (value->type.Date.year == 2155) {
                    if (!append_str(&p_str, &rem_str_len, "(unspecified)"))
                        break;
                } else {
                    snprintf(temp_str, sizeof(temp_str), "%u",
                        (unsigned) value->type.Date.year);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                }
                /* If we get here, then everything is OK. Indicate how many */
                /* bytes were written. */
                ret_val = str_len - rem_str_len;
                break;
            case BACNET_APPLICATION_TAG_TIME:
                if (value->type.Time.hour == 255) {
                    if (!append_str(&p_str, &rem_str_len, "**:"))
                        break;
                } else {
                    snprintf(temp_str, sizeof(temp_str), "%02u:",
                        (unsigned) value->type.Time.hour);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                }
                if (value->type.Time.min == 255) {
                    if (!append_str(&p_str, &rem_str_len, "**:"))
                        break;
                } else {
                    snprintf(temp_str, sizeof(temp_str), "%02u:",
                        (unsigned) value->type.Time.min);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                }
                if (value->type.Time.sec == 255) {
                    if (!append_str(&p_str, &rem_str_len, "**."))
                        break;
                } else {
                    snprintf(temp_str, sizeof(temp_str), "%02u.",
                        (unsigned) value->type.Time.sec);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                }
                if (value->type.Time.hundredths == 255) {
                    if (!append_str(&p_str, &rem_str_len, "**"))
                        break;
                } else {
                    snprintf(temp_str, sizeof(temp_str), "%02u",
                        (unsigned) value->type.Time.hundredths);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                }
                /* If we get here, then everything is OK. Indicate how many */
                /* bytes were written. */
                ret_val = str_len - rem_str_len;
                break;
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                if (!append_str(&p_str, &rem_str_len, "("))
                    break;
                if (value->type.Object_Id.type < MAX_ASHRAE_OBJECT_TYPE) {
                    if (!append_str(&p_str, &rem_str_len,bactext_object_type_name(value->type.Object_Id.type)))
                        break;
                    snprintf(temp_str, sizeof(temp_str), ", %lu",(unsigned long) value->type.Object_Id.instance);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                } else if (value->type.Object_Id.type < 128) {
                    if (!append_str(&p_str, &rem_str_len, "reserved "))
                        break;
                    snprintf(temp_str, sizeof(temp_str), "%u, ",(unsigned) value->type.Object_Id.type);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                    snprintf(temp_str, sizeof(temp_str), "%lu",
                        (unsigned long) value->type.Object_Id.instance);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                } else {
                    if (!append_str(&p_str, &rem_str_len, "proprietary "))
                        break;
                    snprintf(temp_str, sizeof(temp_str), "%u, ",
                        (unsigned) value->type.Object_Id.type);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                    snprintf(temp_str, sizeof(temp_str), "%lu",
                        (unsigned long) value->type.Object_Id.instance);
                    if (!append_str(&p_str, &rem_str_len, temp_str))
                        break;
                }
                if (!append_str(&p_str, &rem_str_len, ")"))
                    break;
                /* If we get here, then everything is OK. Indicate how many */
                /* bytes were written. */
                ret_val = str_len - rem_str_len;
                break;
            default:
                ret_val = 0;
                break;
        }
    }

    return ret_val;
}

bool bacapp_print_valuex(FILE * stream,BACNET_OBJECT_PROPERTY_VALUE * object_value)
{
    char *str;
    bool retval = false;
    size_t str_len = 32;
    int status;

    while (true) 
    {
        /* Try to allocate memory for the output string. Give up if unable. */
        str = (char *) calloc(sizeof(char), str_len);
        if (!str)
            break;
	debug_printf("%s,%d\n",__FUNCTION__,__LINE__);	
        /* Try to extract the value into allocated memory. If unable, try again */
        /* another time with a string that is twice as large. */
        status = bacapp_snprintf_valuex(str, str_len, object_value);
        if ((status < 0) || (status >= str_len)) 
	{
		debug_printf("%s,%d\n",__FUNCTION__,__LINE__);	
            free(str);
            str_len *= 2;
        } 
	else if (status == 0) 
	{
		debug_printf("%s,%d\n",__FUNCTION__,__LINE__);	
            free(str);
            break;
        } 
	else 
	{
		debug_printf("%s,%d\n",__FUNCTION__,__LINE__);	
            if (stream)
                fprintf(stream, "%s\n", str);
            free(str);
            retval = true;
            break;
        }
    }
    return retval;
}

/** @file h_rp_a.c  Handles Read Property Acknowledgments. */

/** For debugging...
 * @param [in] data portion of the ACK
 */
  /********************************************************************
* 函 数 名    :rp_ack_print_data
* 函数介绍:	
* 输入参数:	data 读属性的结构体数据信息
* 输出参数:	无	
* 返 回 值    :	无	
********************************************************************/
void rp_ack_print_data(BACNET_READ_PROPERTY_DATA * data)
{
    BACNET_OBJECT_PROPERTY_VALUE object_value;  /* for bacapp printing */
    BACNET_APPLICATION_DATA_VALUE value;        /* for decode value data */
    int len = 0;
    uint8_t *application_data;
    int application_data_len;
    bool first_value = true;
    bool print_brace = false;
    debug_printf("%s,%d\n",__FUNCTION__,__LINE__);

    if (data) 
    {
	 debug_printf("%s,%d\n",__FUNCTION__,__LINE__);	
        application_data = data->application_data;
        application_data_len = data->application_data_len;
	debug_printf("%s,%d,application_data = %x %x %x %x %x\n",__FUNCTION__,__LINE__,application_data[0],application_data[1],
																			   application_data[2]);
	debug_printf("%s,%d,application_data_len = %d\n",__FUNCTION__,__LINE__,application_data_len);
        /* FIXME: what if application_data_len is bigger than 255? */
        /* value? need to loop until all of the len is gone... */
        while(1) 
	{
	    debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
            len = bacapp_decode_application_data(application_data,(uint8_t) application_data_len, &value);
            if (first_value && (len < application_data_len)) 
	    {
	        debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
                first_value = false;
#if PRINT_ENABLED
                fprintf(stdout, "{");
#endif
                print_brace = true;
            }
            object_value.object_type = data->object_type;
            object_value.object_instance = data->object_instance;
            object_value.object_property = data->object_property;
            object_value.array_index = data->array_index;
            object_value.value = &value;
	    debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
            bacapp_print_value(stdout, &object_value);
            if (len > 0) 
	    {
		debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
                if (len < application_data_len) 
		{
		debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
                    application_data += len;
                    application_data_len -= len;
                    /* there's more! */
#if PRINT_ENABLED
                    fprintf(stdout, ",");
#endif
                } 
		else 
		{
			debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
                    break;
                }
            } 
	    else 
	    {
			debug_printf("%s,%d\n",__FUNCTION__,__LINE__);
                break;
            }
        }
#if PRINT_ENABLED
        if (print_brace)
            fprintf(stdout, "}");
        fprintf(stdout, "\r\n");
#endif
    }
}


/** Handler for a ReadProperty ACK.
 * @ingroup DSRP
 * Doesn't actually do anything, except, for debugging, to
 * print out the ACK message.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void handler_read_property_ack(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data)
{
    int len = 0;
    BACNET_READ_PROPERTY_DATA data;

    (void) src;
    (void) service_data;        /* we could use these... */
    len = rp_ack_decode_service_request(service_request, service_len, &data);
#if 0
    fprintf(stderr, "Received Read-Property Ack!\n");
#endif
    if (len > 0)
        rp_ack_print_data(&data);
}

/** Decode the received RP data into a linked list of the results, with the
 *  same data structure used by RPM ACK replies.
 *  This function is provided to provide common handling for RP and RPM data,
 *  and fully decodes the value(s) portion of the data for one property.
 * @ingroup DSRP
 * @see rp_ack_decode_service_request(), rpm_ack_decode_service_request()
 *
 * @param apdu [in] The received apdu data.
 * @param apdu_len [in] Total length of the apdu.
 * @param read_access_data [out] Pointer to the head of the linked list
 * 			where the RP data is to be stored.
 * @return Number of decoded bytes (could be less than apdu_len),
 * 			or -1 on decoding error.
 */
int rp_ack_fully_decode_service_request(
    uint8_t * apdu,
    int apdu_len,
    BACNET_READ_ACCESS_DATA * read_access_data)
{
    int decoded_len = 0;        /* return value */
    BACNET_READ_PROPERTY_DATA rp1data;
    BACNET_PROPERTY_REFERENCE *rp1_property;    /* single property */
    BACNET_APPLICATION_DATA_VALUE *value, *old_value;
    uint8_t *vdata;
    int vlen, len;

    decoded_len = rp_ack_decode_service_request(apdu, apdu_len, &rp1data);
    if (decoded_len > 0) {
        /* Then we have to transfer to the BACNET_READ_ACCESS_DATA structure
         * and decode the value(s) portion
         */
        read_access_data->object_type = rp1data.object_type;
        read_access_data->object_instance = rp1data.object_instance;
        rp1_property = calloc(1, sizeof(BACNET_PROPERTY_REFERENCE));
        read_access_data->listOfProperties = rp1_property;
        if (rp1_property == NULL) {
            /* can't proceed if calloc failed. */
            return BACNET_STATUS_ERROR;
        }
        rp1_property->propertyIdentifier = rp1data.object_property;
        rp1_property->propertyArrayIndex = rp1data.array_index;
        /* Is there no Error case possible here, as there is when decoding RPM? */
        /* rp1_property->error.error_class = ?? */
        /* rp_ack_decode_service_request() processing already removed the
         * Opening and Closing '3' Tags.
         * note: if this is an array, there will be
         more than one element to decode */
        vdata = rp1data.application_data;
        vlen = rp1data.application_data_len;
        value = calloc(1, sizeof(BACNET_APPLICATION_DATA_VALUE));
        rp1_property->value = value;
        old_value = value;
        while (value && vdata && (vlen > 0)) {
            if (IS_CONTEXT_SPECIFIC(*vdata)) {
                len =
                    bacapp_decode_context_data(vdata, vlen, value,
                    rp1_property->propertyIdentifier);
            } else {
                len = bacapp_decode_application_data(vdata, vlen, value);
            }
            if (len < 0) {
                /* unable to decode the data */
                while (value) {
                    /* free the linked list of values */
                    old_value = value;
                    value = value->next;
                    free(old_value);
                }
                free(rp1_property);
                read_access_data->listOfProperties = NULL;
                return len;
            }
            decoded_len += len;
            vlen -= len;
            vdata += len;
            /* If unexpected closing tag here: */
            if (vlen && decode_is_closing_tag_number(vdata, 3)) {
                decoded_len++;
                vlen--;
                vdata++;
                break;
            } else {
                if (len == 0) {
                    /* nothing decoded and no closing tag, so malformed */
                    while (value) {
                        /* free the linked list of values */
                        old_value = value;
                        value = value->next;
                        free(old_value);
                    }
                    free(rp1_property);
                    read_access_data->listOfProperties = NULL;
                    return BACNET_STATUS_ERROR;
                }
                if (vlen > 0) {
                    /* If more values */
                    old_value = value;
                    value = calloc(1, sizeof(BACNET_APPLICATION_DATA_VALUE));
                    old_value->next = value;
                }
            }
        }
    }

    return decoded_len;
}
