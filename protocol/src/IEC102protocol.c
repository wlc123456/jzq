
#include "IEC102protocol.h"
#include "ennDatabase.h"


UINT8 iec102_asdu_tbl[4]={IEC870_TYPE_M_IT_TA_2, IEC870_TYPE_M_IT_TD_2, IEC870_TYPE_M_IT_TG_2, IEC870_TYPE_M_IT_TK_2};
#define YEAR_START_NUM    90    /*from year 1990, (1990-1900=90)*/
#define YEAR_END_NUM      189   /*end year 2089, (2089-1900=189)*/
static UINT8             Month_Days[12]  = {31,28,31,30,31,30,31,31,30,31,30,31};

//UINT16 MP_PERIOD = 180;
extern int fd_index_to_master[MAXCLINE];
extern UINT8 currentIndex;
extern DPA_SLAVE_PARAM *slave_table_Head;




ENN_VOID IEC102_DEBUG(ENN_CHAR *fmt, ...)
{
#if 1
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	vfprintf(stdout, fmt, arg_ptr);
	va_end(arg_ptr);
#endif	
}

UINT8 FT12FLF_102_frame_signal(DPA_CONN_CONTEXT *context)
{
    context->txframe.datalen = FT12_FLF_SIZE + 3;
    context->txframe.dataflags |= FBF_EOR;

    return 0;
}

UINT8 FT12VLF_102_frame_signal(UINT16 n, DPA_CONN_CONTEXT *context)
{
    context->txframe.datalen = n + 6;
    context->txframe.dataflags |= FBF_EOR;

    return 0;
}


UINT8 FT12VLF_102_framedata_fill(UINT8 *pbuf, UINT16 n)
{
    if (NULL == pbuf)
    {
        IEC102_DEBUG("ERROR : NULL pointer. %s,%d\n",__FUNCTION__,__LINE__);
        return 1;
    }

    if (n > FT12_LDATASIZE)
    {
	IEC102_DEBUG("ERROR : n > FT12_LDATASIZE. %s,%d\n",__FUNCTION__,__LINE__);
        return 1;
    }

    pbuf[0]   = FT12_VLF_START;
    pbuf[1]   = (UINT8) n;
    pbuf[2]   = (UINT8) n;
    pbuf[3]   = FT12_VLF_START;
    pbuf[n+4] = byte_checksum(&pbuf[4], n);
    pbuf[n+5] = FT12_VLF_END;

    return 0;
}


UINT8 FT12VLF_102_linkdata_fill(UINT8 *pbuf, UINT16 ldataoff, UINT8 *ldata, UINT16 n)
{
    if (ldataoff+n > FT12_LDATASIZE)
    {
	IEC102_DEBUG("ERROR : ldataoff+n > FT12_LDATASIZE. %s,%d\n",__FUNCTION__,__LINE__);
        return 1;
    }

    memcpy(&pbuf[4+ldataoff], ldata, n);

    return 0;
}


UINT8 FT12FLF_102_linkdata_fill(UINT8* pbuf, UINT8 *ldata)
{
    if ((NULL == pbuf) || (NULL == ldata))
    {
        IEC102_DEBUG("ERROR : NULL pointer. %s,%d\n",__FUNCTION__,__LINE__);
        return 1;
    }
    memcpy(&pbuf[1], ldata, FT12_FLF_SIZE);

    return 0;
}

UINT8 FT12FLF_102_framedata_fill(UINT8 *pbuf)
{
    if (NULL == pbuf)
    {
        IEC102_DEBUG("ERROR : NULL pointer. %s,%d\n",__FUNCTION__,__LINE__);
        return 1;
    }

    pbuf[0] = FT12_FLF_START;
    pbuf[FT12_FLF_SIZE+1] = byte_checksum(&pbuf[1], FT12_FLF_SIZE);
    pbuf[FT12_FLF_SIZE+2] = FT12_FLF_END;

    return 0;
}

static UINT8 iec102_deactivation(UINT8 *bufp, DPA_CONN_CONTEXT *context)
{
    u_int8_t            *pb;
    T_IEC102_DUI        *plastdui = NULL;
    U_IEC102_INFOOBJ    *pinfobj = NULL;
    T_IEC102_DUI        *pasdudui = NULL;

    if (NULL == bufp || NULL == context)
    {
        IEC102_DEBUG("ERROR : NULL pointer. %s,%d\n",__FUNCTION__,__LINE__);
        return 0;
    }

    plastdui = &context->seq.attr.iec870_102attr.lastasdu.dui;
    pinfobj = &context->seq.attr.iec870_102attr.lastasdu.infobj;

    /*copy type ident,VSQ,COT,addr(high and low) and record addr (total six char) to asdu*/
    pasdudui = (T_IEC102_DUI *)bufp;

    if (pasdudui->type        != plastdui->type        ||
        pasdudui->vsq.b       != plastdui->vsq.b       ||
        pasdudui->dte_addr_lo != plastdui->dte_addr_lo ||
        pasdudui->dte_addr_hi != plastdui->dte_addr_hi ||
        pasdudui->rec_addr    != plastdui->rec_addr)
    {
        return 0;
    }

    /*pb point to user data after record addr*/
    pb = bufp + IEC870_5_102_DUI_SIZE;

    switch (bufp[0])
    {
    case IEC870_TYPE_C_SP_NA_2:
    case IEC870_TYPE_C_CI_NA_2:
    case IEC870_TYPE_C_CI_NE_2:
        break;

    case IEC870_TYPE_C_SP_NB_2:
        if (memcmp(pb, &pinfobj->io102.taf, IEC870_TIMEA_SIZE) ||
            memcmp(pb + IEC870_TIMEA_SIZE, &pinfobj->io102.tat,
                   IEC870_TIMEA_SIZE))
        {
            return 0;
        }
        break;

    case IEC870_TYPE_C_CI_NB_2:
    case IEC870_TYPE_C_CI_NF_2:
        if (memcmp(pb, &pinfobj->io105, IEC870_SIZE_C_CI_NB_2))
        {
            return 0;
        }
        break;

    case IEC870_TYPE_C_CI_NC_2:
    case IEC870_TYPE_C_CI_NG_2:
        if (memcmp(pb, &pinfobj->io106,	IEC870_SIZE_C_CI_NC_2))
        {
            return 0;
        }
        break;

    case IEC870_TYPE_C_CI_ND_2:
    case IEC870_TYPE_C_CI_NH_2:
        if (memcmp(pb, &pinfobj->io107, IEC870_SIZE_C_CI_ND_2))
        {
            return 0;
        }
        break;

    case IEC870_TYPE_C_CI_NR_2:
    case IEC870_TYPE_C_CI_NS_2:
        if (memcmp(pb, &pinfobj->io120, IEC870_TIMEA_SIZE+2) ||
            memcmp(pb + IEC870_TIMEA_SIZE + 2, &pinfobj->io120.tat,
                   IEC870_TIMEA_SIZE))
        {
            return 0;
        }
        break;

    default:
        return 0;
    };

    return 1;
}


UINT8 FT12_102_tx_put(UINT8 *ldata, UINT16 n, DPA_CONN_CONTEXT *context)
{
    UINT8* pdata;
    UINT8* pbuf;

    if (NULL == context)
    {
        IEC102_DEBUG("ERROR : NULL pointer. %s,%d\n",__FUNCTION__,__LINE__);
        return DPA_FAIL;
    }

    if (n > FT12_LDATASIZE)
    {
        return DPA_FAIL;
    }

    IEC102_DEBUG( "%s,%d,start FT12_102_tx_put, n=%d\n",__FUNCTION__,__LINE__, n);
    
    /* Fixed length frame */
    if (n == FT12_FLF_SIZE)
    {
        if (FT12FLF_102_linkdata_fill(context->txframe.databuf, ldata) != 0 ||
            FT12FLF_102_framedata_fill(context->txframe.databuf) != 0 ||
            FT12FLF_102_frame_signal(context) != 0)
        {
            return DPA_FAIL;
        }
    }

    /* Variable length frame */
    else
    {
        if((n == 1)&&(*ldata == DLCF_SC_ACK_E5))
        {
            /************* FT12_linkdata_fill ******************/
            pbuf = context->txframe.databuf;
            //memcpy(&pbuf[0], ldata, 1);
            pbuf[0] = DLCF_SC_ACK_E5;
            /************* FT12_frame_signal *******************/
            context->txframe.datalen = 1;
            context->txframe.dataflags |= FBF_EOR;
        }
        else
        /******************************************************/
        {
            if (FT12VLF_102_linkdata_fill(context->txframe.databuf, 0, ldata, n) != 0 ||
                FT12VLF_102_framedata_fill(context->txframe.databuf, n) != 0 ||
                FT12VLF_102_frame_signal(n, context) != 0)
            {
                return DPA_FAIL;
            }
        }
    }

    IEC102_DEBUG( "txframe.datalen=%d\n", context->txframe.datalen);

    /*store the current frame information into the lasttxframe*/
    context->lasttxframe.datalen = 0;

    pdata = context->txframe.databuf;
    memcpy(context->lasttxframe.databuf, pdata, context->txframe.datalen);
    context->lasttxframe.datalen = context->txframe.datalen;

    return DPA_SUCCESS;
}

static UINT16 iec102_end_init(DPA_CONN_CONTEXT *context, T_IEC102_DATA *dd)
{
    T_IEC102_COI initcause;

    dd->vsq.bf.num = 1;
    dd->cause.bf.cause = IEC870_CAUSE_INIT;
    dd->rec_addr = 0;
    dd->udata[0] = 0;

    initcause.cause = LOCAL_PWS_ON;
    memcpy(&dd->udata[1], &initcause, sizeof(initcause));

    context->initflag = 0;

    return (IEC870_SIZE_M_EI_NA_2);
}

static UINT16 iec102_mstationtime_to_systime(struct tm *pdatetime,UINT8 *pudata,UINT8 type)
{
    UINT8* pdata;

    pdata = pudata;

    if (type == IEC870_5_102_TIMEB)
    {
        pdata++;
        pdatetime->tm_sec = *pdata++ >> 2;
    }
    else
    {
        pdatetime->tm_sec = 0;
    }

    pdatetime->tm_min   = *pdata++ & 0x3F;
    pdatetime->tm_hour  = *pdata & 0x1F;
    pdatetime->tm_isdst = (*pdata++ & 0x80) ? 1:0;
    pdatetime->tm_mday  = *pdata & 0x1F;
    pdatetime->tm_wday  = (*pdata++ >> 5) - 1;
    pdatetime->tm_mon   = ((*pdata++ & 0x0F) - 1);
    pdatetime->tm_year  = *pdata++ & 0x7F;

    /* for example, pdatetime->tm_year=08(means year 2008),
     * we should get 108(how many years from year 1900 till 2008)
     */
    if (pdatetime->tm_year < 90)
    {
        pdatetime->tm_year += 100;
    }

    return(pdata - pudata);
}

UINT32 iec102_date_to_stamp(struct tm *pdatetime)
{
    UINT32 stamp;

    stamp = (UINT32)timelocal(pdatetime);
    return stamp;
}


static UINT32 iec102_systimea_stamp(void* systimea)
{
    UINT8* stimea;
    struct tm datetime;

    stimea = (UINT8*) systimea;

    iec102_mstationtime_to_systime(&datetime,stimea,IEC870_5_102_TIMEA);

    return (iec102_date_to_stamp(&datetime));

}


static UINT32 iec102_get_sys_timestamp(void)
{
    return (UINT32)(time(NULL));
}

static UINT16 sysdatetime_to_IEC870_time(struct tm *pdatetime, UINT8 *pudata)
{
	UINT8* pdata;

	if (NULL == pdatetime || NULL == pudata)
	{
		IEC102_DEBUG( "NULL pointer!,%s,%d\n",__FUNCTION__,__LINE__);
		return 0;
	}

	pdata = pudata;

	*pdata++   = pdatetime->tm_min;
	*pdata     = pdatetime->tm_hour;

	if (pdatetime->tm_isdst)
	{
		*pdata |= 0x80;
	}
	pdata++;
//edit by hyptek
//	*pdata++   = pdatetime->tm_mday | ((pdatetime->tm_wday + 1)<< 5);
	*pdata++   = pdatetime->tm_mday | ((pdatetime->tm_wday)<< 5);

	*pdata++   = pdatetime->tm_mon + 1;
	//*pdata++   = pdatetime->tm_year % 100;
	*pdata++   = 15;	//not rtc ,


	return(pdata - pudata);
}


static UINT16 sysdatetime_to_IEC870_timeb(struct tm *pdatetime, UINT32 milisec,
                                 UINT8 *pudata)
{
	UINT8* pdata;

	if (NULL == pdatetime || NULL == pudata)
	{
		IEC102_DEBUG( "NULL pointer!,%s,%d\n",__FUNCTION__,__LINE__);
		return 0;
	}

	pdata = pudata;

	*pdata++ = (UINT8)milisec;
	*pdata++ = ((UINT8)(milisec>>8) & 0x03) | (pdatetime->tm_sec << 2);
/*	IEC102_DEBUG("[%s], %d, milisec= %x, pudata[0]=%x, pudata[1]=%x\n",
		__FUNCTION__,__LINE__,milisec,pudata[0],pudata[1]);*/

    return (pdata-pudata+sysdatetime_to_IEC870_time(pdatetime, pdata));
}

static UINT16 sysdatetime_to_IEC870_timeb_hyptek(struct tm *pdatetime, UINT32 milisec,
                                 UINT8 *pudata)
{
	UINT8* pdata;

	if (NULL == pdatetime || NULL == pudata)
	{
		IEC102_DEBUG( "NULL pointer!,%s,%d\n",__FUNCTION__,__LINE__);
		return 0;
	}

	pdata = pudata;

//hyptek edit
	//*pdata++ = (UINT8)milisec;
	//*pdata++ = ((UINT8)(milisec>>8) & 0x03) | (pdatetime->tm_sec << 2);
	*pdata++ = (UINT8)0;
	*pdata++ = ((UINT8)0);
//hyptek edit

/*	IEC102_DEBUG("[%s], %d, milisec= %x, pudata[0]=%x, pudata[1]=%x\n",
		__FUNCTION__,__LINE__,milisec,pudata[0],pudata[1]);*/

    return (pdata-pudata+sysdatetime_to_IEC870_time(pdatetime, pdata));
}


static UINT32 timer_getsecms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (tv.tv_usec /1000);
}

UINT8 iec102_valid_date(UINT32 Year, UINT32 Month, UINT32 Day)
{
   UINT8 Leap;
   UINT8 Ml;

    if ((Year<YEAR_START_NUM) || (Year>YEAR_END_NUM) ||
        (Month<0) || (Month>11))
    {
        WARN("Year or month invalid:Year=%d, Month=%d\n", Year,Month);
        return 0;
    }
    Leap = ((Year % 4)==0);

    if (Month==1)
    {
        Ml = Month_Days[Month] + Leap;
    }
    else
    {
        Ml = Month_Days[Month];
    }

    if ( (Day<1) || (Day>Ml))
    {
        WARN("Day invalid:Day=%d\n", Day);
        return 0;
    }

    return 1;
}


/* *********************************************************************** *
* Function    : valid_time
* Input       :
* Output      :
* Returns     :1-ok,0-no
* Description :
* *********************************************************************** */
bool  iec102_valid_time(UINT8 hour, UINT8 minute, UINT8 second)
{
   if ((hour<24) && (minute<60) && (second<60))
   {
        return(1);
   }
   else
   {
        WARN("time invalid:hour=%d, min=%d, sec=%d\n", hour,minute,second);
        return(0);
   }
}


static void  iec102_stamp_to_date(UINT32 stamp, struct tm  *pdatetime)
{
    time_t      timestamp;
    struct tm   *pdt;

    if (NULL == pdatetime)
    {
        ERR( "NULL pointer!\n");
        return;
    }

    timestamp = (time_t)stamp;
    memset(pdatetime, 0, sizeof(struct tm));

    pdt = localtime(&timestamp);

    if (NULL == pdt)
    {
        ERR( "NULL pointer!\n");
        return;
    }

    memcpy(pdatetime, pdt, sizeof(struct tm));
    return;
}

/* timeval tv expressed in UTC time */
int sl_settime( struct timeval *tv )
{
	int ret;
	int rtc_fd;
	struct timezone tz={0,0};
	struct tm tm;
	//char cmd[64];
	//struct stat fileattr;
		
	/* set the system time and date. superuser may needed*/
	tzset();
	tz.tz_minuteswest = 0;
	//tv->tv_sec = tv->tv_sec + tz.tz_minuteswest*60;//convert to UTC seconds: utc - local = tz_minuteswest*60
	ret = settimeofday(tv, &tz);
	if (ret){//if on error, -1 returned.
		return ret;
	}

	#if 0
	/* Normally the RTC is kept in UTC time.
	 * next time, we can check "/etc/adjtime" to test rtc time format
 	 */
	do{
		rtc_fd = open("/dev/rtc", O_WRONLY);
		if (rtc_fd != -1)
			break;
		rtc_fd = open("/dev/rtc0", O_WRONLY);
		if (rtc_fd != -1)
			break;
		rtc_fd = open("/dev/misc/rtc", O_WRONLY);
		if (rtc_fd != -1)
			break;
	}while(0);
	
	if (rtc_fd != -1){
		gmtime_r(&tv->tv_sec,&tm);
		tm.tm_isdst = 0;
		ioctl(rtc_fd, RTC_SET_TIME, &tm);
		close(rtc_fd);
	}
	#endif
	
	return 0;
}


static void iec102_syn_datetime(struct tm *newdatetime, UINT16 syn_timelimit,
                                    UINT32 new_secms)
{
    UINT32       newstamp, oldstamp;
    int             ret;
    UINT32  		absdeltatime;
    struct timeval     timeVal;
    struct timezone    timeZone;

    if (NULL == newdatetime)
    {
        ERR( "NULL pointer!\n");
        return;
    }


	oldstamp = iec102_get_sys_timestamp();
	newstamp = mktime(newdatetime);
	absdeltatime = (newstamp > oldstamp) ?
        (newstamp - oldstamp) : (oldstamp - newstamp);

    if ((syn_timelimit != 0) && (absdeltatime >= syn_timelimit))
    {
        ERR("check sync limit fail!, syn_timelimit=%d\n", syn_timelimit);
        return;
    }

    memset(&timeVal, 0, sizeof(timeVal));
    memset(&timeZone, 0, sizeof(timeZone));
    
    timeVal.tv_sec = newstamp;
    timeVal.tv_usec = new_secms*1000UL;
    timeZone.tz_minuteswest = 0;
    timeZone.tz_dsttime = 0;
    
    ret = sl_settime(&timeVal);
    if (ret != 0)
    {
        ERR("sl_settime() Error, ret=%d!\n", ret);
        return;
    }
}


static void iec102_syn_datetime_ms(struct tm newdatetime,
                    UINT32 new_secms, UINT32 sysdatetime_stamp, UINT16 syn_timelimit)
{
    UINT32   systime_ms;
    SINT32		timediffms;
    UINT32   timediffabs;
    UINT32   newstamp;

    newstamp = iec102_date_to_stamp(&newdatetime);
    systime_ms = timer_getsecms();
    if ( timer_getsecms() < systime_ms )
    {
        systime_ms = timer_getsecms();
    }

    if (sysdatetime_stamp >= newstamp)
    {
        timediffms = (SINT32) ((sysdatetime_stamp - newstamp) * 1000L) +
                     (SINT32) systime_ms -
                     (SINT32) new_secms;
    }
    else
    {
        timediffms = (SINT32) ((newstamp - sysdatetime_stamp) * 1000L) +
                     (SINT32) new_secms -
                     (SINT32) systime_ms;
    }

    if (timediffms >= 0)
    {
        timediffabs = timediffms;
    }
    else
    {
        timediffabs = -timediffms;
    }

    if (timediffabs < TIMEDIFF_SYNCH_LIMIT)
    {
        return;
    }

    iec102_syn_datetime(&newdatetime, syn_timelimit, new_secms);
}


static UINT16 iec102_time(UINT8 *udata)
{
    UINT32		sys_ms;
    time_t          systimestamp;
    struct tm       *psystimedate = NULL;

    systimestamp = (time_t)iec102_get_sys_timestamp();

    psystimedate = localtime(&systimestamp);

    if (NULL == psystimedate)
    {
        IEC102_DEBUG( "NULL pointer!,%s,%d\n",__FUNCTION__,__LINE__);
        return 0;
    }
    sys_ms = timer_getsecms();
    if (timer_getsecms() < sys_ms)
    {
        sys_ms = timer_getsecms();
    }

    /*convert system time to master station timeb*/
    sysdatetime_to_IEC870_timeb(psystimedate, sys_ms, &udata[0]);

    return IEC870_TIMEB_SIZE;
}

/**********FUNCTION :设备响应主站的镜像帧***********/
static void iec102_activ_conf(DPA_CONN_CONTEXT *context, UINT8 iolength, T_IEC102_DATA *dd)
{
    U_IEC102_CAUSE              *plastcause = NULL;
    T_IEC102_ATTR               *p102attr = NULL;

    if (NULL == dd || NULL == context)
    {
        ERR( "NULL pointer!\n");
        return;
    }

    plastcause = &context->seq.attr.iec870_102attr.lastasdu.dui.cause;
    p102attr = &context->seq.attr.iec870_102attr;

    iolength += IEC870_5_102_DUI_SIZE;
    memcpy(&dd->type,&context->seq.attr.iec870_102attr.lastasdu.dui,iolength);
    FT12_102_tx_put((UINT8 *)dd, (UINT16)(iolength+3U), context);

    if (plastcause->bf.cause == IEC870_CAUSE_DEACTIV_CON)
    {
        plastcause->bf.cause = IEC870_CAUSE_ACTIV_TERM;
        return;
    }

    if (((plastcause->bf.cause != IEC870_CAUSE_ACTIV_CON) &&
        (plastcause->bf.cause != IEC870_CAUSE_REQ)) ||
        (p102attr->iec870seq.cseq == 0))
    {
        p102attr->iec870seq.extcom = 0;
        p102attr->iec870seq.cseq = 0;
    }
}


static void iec102_check_resp(DPA_CONN_CONTEXT *context, UINT16 length, T_IEC102_DATA *dd)
{
    U_IEC102_CAUSE              *plastcause = NULL;
    T_IEC102_ATTR               *p102attr = NULL;
    T_IEC102_DUI                *plastdui = NULL;
    UINT8		iolength=0;

    WARN("length=%d, pPort->ret_immedt = %d\n", length, context->ret_immedt);
    if (length == IEC870_5_102_DUI_SIZE)
    {
        dd->ctrl = DLCF_SR_NACK;
        FT12_102_tx_put((UINT8 *)dd,sizeof(T_IEC102_FLF_FIELDS), context);
        context->seq.attr.iec870_102attr.iec870seq.cseq = 0;
        context->seq.attr.iec870_102attr.iec870seq.extcom = 0;
    }
    else if (context->ret_immedt == 1)
    {
        p102attr = &context->seq.attr.iec870_102attr;
        plastcause = &context->seq.attr.iec870_102attr.lastasdu.dui.cause;
        plastdui = &context->seq.attr.iec870_102attr.lastasdu.dui;

        dd->type = plastdui->type;
        switch(dd->type)
        {
            case IEC870_TYPE_C_SP_NB_2:
                iolength = IEC870_SIZE_C_SP_NB_2;
            break;
            case IEC870_TYPE_C_RD_NA_2:
            case IEC870_TYPE_C_TI_NA_2:
            case IEC870_TYPE_C_SP_NA_2:
            case IEC870_TYPE_C_CI_NA_2:
            case IEC870_TYPE_C_CI_NE_2:
            case IEC870_TYPE_C_CI_NJ_2:
            case IEC870_TYPE_C_CI_NN_2:
                iolength = IEC870_SIZE_C_CI_NA_2;
            break;

            case IEC870_TYPE_C_CI_NB_2:
            case IEC870_TYPE_C_CI_NF_2:
            case IEC870_TYPE_C_CI_NK_2:
            case IEC870_TYPE_C_CI_NO_2:
                iolength = IEC870_SIZE_C_CI_NB_2;
            break;

            case IEC870_TYPE_C_CI_NC_2:
            case IEC870_TYPE_C_CI_NG_2:
            case IEC870_TYPE_C_CI_NL_2:
            case IEC870_TYPE_C_CI_NP_2:
                iolength = IEC870_SIZE_C_CI_NC_2;
            break;

            case IEC870_TYPE_C_CI_ND_2:
            case IEC870_TYPE_C_CI_NH_2:
            case IEC870_TYPE_C_CI_NM_2:
            case IEC870_TYPE_C_CI_NQ_2:
                iolength =IEC870_SIZE_C_CI_ND_2;
                break;

            case IEC870_TYPE_C_CI_NR_2:
            case IEC870_TYPE_C_CI_NS_2:
            case IEC870_TYPE_C_CI_NT_2:
            case IEC870_TYPE_C_CI_NU_2:
                iolength =IEC870_SIZE_C_CI_NR_2;
                break;

            default:
                iolength=length;
            break;
        }

        iolength += IEC870_5_102_DUI_SIZE;
        memcpy(&dd->type,&context->seq.attr.iec870_102attr.lastasdu,iolength);

        dd->type = plastdui->type;
        dd->cause.bf.cause = context->seq.attr.iec870_102attr.lastasdu.dui.cause.bf.cause;//IEC870_CAUSE_REQ;
        WARN("dd->cause.bf.cause = %d\n", dd->cause.bf.cause);

        dd->ctrl = dd->ctrl & (~DLC_FCB_ACD);

        FT12_102_tx_put((UINT8 *)dd, (UINT16)(iolength+3U), context);
        context->ret_immedt = 0;
        context->ioa_not_finish = 0;

        if (((plastcause->bf.cause != IEC870_CAUSE_ACTIV_CON) &&
            (plastcause->bf.cause != IEC870_CAUSE_REQ)) ||
            (p102attr->iec870seq.cseq == 0))
        {
            p102attr->iec870seq.extcom = 0;
            p102attr->iec870seq.cseq = 0;
        }
    }
    else
    {
        dd->cause.bf.cause = IEC870_CAUSE_REQ;
        FT12_102_tx_put((UINT8 *)dd, (UINT16)(length+3U), context);
    }
}

static void iec102_synchro(DPA_CONN_CONTEXT *context, T_IEC102_DATA *dd)
{
    UINT32       sys_ms;
    UINT32       len = 0;
    time_t          stamp;
    struct tm       dt;
    UINT8        type;

    if (NULL == context)
    {
        ERR( "NULL pointer!\n");
        return;
    }

    sys_ms = timer_getsecms();
    if (timer_getsecms() < sys_ms)
    {
        sys_ms = timer_getsecms();
    }
    stamp = iec102_get_sys_timestamp();
    iec102_stamp_to_date(stamp, &dt);

    /*convert system time to master station timeb*/
    sysdatetime_to_IEC870_timeb(&dt, sys_ms,
        (UINT8*)&(context->seq.attr.iec870_102attr.lastasdu.infobj.synchtime));

    iec102_activ_conf(context, IEC870_TIMEB_SIZE, dd);
    return;
}


static UINT8 iec102_check_IT(DPA_CONN_CONTEXT *context)
{
    UINT8            mp;
    UINT32		stampfrom;
    UINT32           stampto;
    UINT8            ret = IEC870_CAUSE_OBJ_NAVAILABLE;
    UINT8            prof;
    UINT16           configed_itto;
    UINT32           writestamp = 0;
    UINT8            type;
    UINT8            rec_addr;
    T_IEC102_DUI        *plastdui = NULL;
    U_IEC102_INFOOBJ    *pinfobj = NULL;
    UINT32			period;

	if (NULL == context)
	{
		IEC102_DEBUG( "NULL pointer!,[%s],%d\n",__FUNCTION__,__LINE__);
		return IEC870_CAUSE_NONE;
	}
	plastdui = &context->seq.attr.iec870_102attr.lastasdu.dui;
	pinfobj = &context->seq.attr.iec870_102attr.lastasdu.infobj;
	type = plastdui->type;
	rec_addr = plastdui->rec_addr;

	mp = rec_addr;

	//configed_itto = pPort->pMProfCfg->chn_count;
	//period = MP_PERIOD;	//60s, pPort->pMProfCfg->mp_period;
	period = context->data_period;
	stampfrom = 0;

    /* Check valid record address */

    if ((rec_addr < IEC870_RECADDR_IT_PER_1) || (rec_addr > IEC870_RECADDR_IT_PER_3))
    {
        if (((rec_addr >  1) && (rec_addr < 11)) || ((rec_addr > 13)
        && (rec_addr < 21)) || ((rec_addr > 23) && (rec_addr < 31))
        || ((rec_addr > 33) && (rec_addr < 41)) || ((rec_addr > 43)
        && (rec_addr < 51)) || ((rec_addr > 55)))
        {
            IEC102_DEBUG("ERROR :[%s],%d,Record number in the ASDU sent by the controlling station is not known!\n",__FUNCTION__,__LINE__);
            return IEC870_CAUSE_RECNO_NKNOWN;
        }

        IEC102_DEBUG("ERROR :[%s],%d, Requsted data record not available!\n",__FUNCTION__,__LINE__);
        return IEC870_CAUSE_REC_NAVAILABLE;
    }

    switch (type)
    {
    case IEC870_TYPE_C_CI_NC_2:
    case IEC870_TYPE_C_CI_NG_2:
    case IEC870_TYPE_C_CI_NE_2:
    case IEC870_TYPE_C_CI_NA_2:
        break;

    case IEC870_TYPE_C_CI_ND_2:
    case IEC870_TYPE_C_CI_NH_2:
    case IEC870_TYPE_C_CI_NR_2:
    case IEC870_TYPE_C_CI_NS_2:
    case IEC870_TYPE_C_CI_NF_2:
    case IEC870_TYPE_C_CI_NB_2:
        if ((pinfobj->io105.itfrom > pinfobj->io105.itto) || (pinfobj->io105.itfrom == 0)
           /* || (pinfobj->io105.itfrom > configed_itto)*/)
        {
            WARN( "pinfobj->io105.itfrom=%d pinfobj->io105.itto=%d \n", pinfobj->io105.itfrom, pinfobj->io105.itto);
            return IEC870_CAUSE_OBJ_NAVAILABLE;
        }
        break;
    default:
        ERR("Requsted ASDU-type not available!\n");
        return IEC870_CAUSE_TYPE_NAVAILABLE;
    }

    ret = 0;
    writestamp = iec102_get_sys_timestamp(); //iec102_get_prof_writestamp(ptbl, prof);
    writestamp = writestamp/period*period;

    /* Check for valid time information */
    switch (type)
    {
    case IEC870_TYPE_C_CI_NC_2:
    case IEC870_TYPE_C_CI_NG_2:
        stampfrom = iec102_systimea_stamp(&pinfobj->io106.ta);
        if (stampfrom > writestamp)
        {
            WARN( "stampfrom=%d > writestamp=%d \n", stampfrom, writestamp);
            ret = IEC870_CAUSE_OBJ_NAVAILABLE;
        }

    case IEC870_TYPE_C_CI_ND_2:
    case IEC870_TYPE_C_CI_NH_2:
	    stampfrom = iec102_systimea_stamp(&pinfobj->io107.ta);
        if (stampfrom > writestamp)
        {
            WARN( "stampfrom=%d > writestamp=%d \n", stampfrom, writestamp);
            ret = IEC870_CAUSE_OBJ_NAVAILABLE;
        }
        break;

    case IEC870_TYPE_C_CI_NR_2:
    case IEC870_TYPE_C_CI_NS_2:
        stampfrom = iec102_systimea_stamp(&pinfobj->io120.taf);
        stampto   = iec102_systimea_stamp(&pinfobj->io120.tat);
        if ((stampfrom > stampto) || (stampfrom > writestamp))
        {
            IEC102_DEBUG("[%s],%d, stampfrom=%d > stampto=%d or >writestamp=%d \n", 
				__FUNCTION__,__LINE__,stampfrom, stampto, writestamp);
            ret = IEC870_CAUSE_OBJ_NAVAILABLE;
        }
        break;
    default:
        break;
    }

    if (!ret)
    {
        return(IEC870_CAUSE_ACTIV_CON);
    }

    return ret;
}

int InfoNum_to_RegNum(UINT16	infonum)
{
	int regnum = -1;
	int master_no;
	DPA_MASTER_PARAM *current_master_table = NULL;
	INFO_UNIT_MAP *p_info_unit_map=NULL;
	
	master_no = fd_index_to_master[currentIndex];
	if(master_no >0)
	{
		current_master_table = slave_table_Head->master_table;
		while(current_master_table != NULL)
		{
			if(master_no == current_master_table->master_no){
				/*TCP_DEBUG("%s, %d, found master_no = %d, IP = %s\n",__FUNCTION__,__LINE__, 
					current_master_table->master_no, current_master_table->master_ip_addr);*/
				break;
			}
			current_master_table = current_master_table->next;
		}

		if(current_master_table != NULL){
			/*IEC102_DEBUG("[%s], %d, master_no = %d, IP = %s, infonum =%d\n",__FUNCTION__,__LINE__, 
					current_master_table->master_no, current_master_table->master_ip_addr,infonum);*/
			p_info_unit_map = current_master_table->p_info_unit_map;
			while(p_info_unit_map != NULL)
			{
				if(p_info_unit_map->infoNum == infonum)
				{
					regnum = p_info_unit_map->u16RegID;
					break;
				}
				p_info_unit_map = p_info_unit_map->next;
			}
			
		}else{
			IEC102_DEBUG("WARNING : %s, %d, not found link_addr\n",__FUNCTION__,__LINE__);
		}
	}

	return regnum;
}

UINT32 data_profile_read(DPA_CONN_CONTEXT *context,DATA_PROFILE_PARAM		*prof_para , 
							DATA_PROFILE_RESULTS 	*prof_data	
							)
{
	//DATA_FD		*pfd;
	UINT16	chn;
	UINT32	period_sec;
	UINT32	profstamp;
	//DATA_MPROFILE_CFG_TBL	*pMProfCfg;
	POINT_DATA result;
	UINT16 infonum;
	int regnum = -1;
	float k = 1, d = 0;
	ENN_ErrorCode_t returnCode;
	UINT32 tmp;


	if ((prof_para->from > prof_para->to) || (prof_para->from == 0))
	{
	    return -1;
	}
	
	//init
	//WARN("mp_period=%d\n", pMProfCfg->mp_period);
	period_sec = prof_para->stamp % (context->data_period);
	if (period_sec == 0)
	{
		profstamp = prof_para->stamp;
	}
	else
	{
		profstamp = prof_para->stamp - period_sec + context->data_period;
	}

	prof_data->stamp = profstamp;
	prof_data->period = context->data_period;		//pMProfCfg->mp_period;
	prof_data->pf_status = DATA_DATA_INVALID;
	prof_data->chn_nums = prof_para->to - prof_para->from + 1;

	infonum = prof_para->from;

	
	//WARN("prof_data->chn_nums=%d, profstamp=%d, infonum=%d\n", prof_data->chn_nums, profstamp,infonum);
	for (chn=0; chn<prof_data->chn_nums; chn++)
	{
		memset(&result, 0, sizeof(result));
		regnum = InfoNum_to_RegNum(infonum);
		INFO("[%s],%d,infonum =%d\n",__FUNCTION__,__LINE__,infonum);
		if(regnum <= 0)
		{
			WARN("WARNING : [%s], %d, not found RegNum ,regnum =%d !\n",__FUNCTION__,__LINE__,regnum);
			infonum++;
			continue;
		}
		//IEC102_DEBUG("[%s], %d, regnum =%d \n",__FUNCTION__,__LINE__,regnum);
		returnCode = ENNIEC102_get_RegKD(regnum, &k, &d);
		if(ENN_SUCCESS != returnCode)
		{
			ERR("ERROR : %s, %d, not found k and d , %d\n",__FUNCTION__,__LINE__,returnCode);
			//return -1;
		}
		Database_select_period(profstamp, regnum, &result);
		prof_data->results[chn].status = DATA_DATA_INVALID;
		//prof_data->results[chn].value = profstamp % 86400;
		prof_data->results[chn].chn_no = chn;
		if(result.type > 0)
		{
			if(result.type == 1){
				tmp = (UINT32)(result.data.u32data* k + d);
				memcpy(&(prof_data->results[chn].value), &tmp, sizeof(result.data));
			}
			else if(result.type == 2)
			{
				tmp = (UINT32)(result.data.fdata * k + d);
				memcpy(&(prof_data->results[chn].value), &tmp, sizeof(result.data));
			}else if(result.type == 3)
			{
				//memcpy(&(prof_data->results[chn].value), result->pdata, strlen(result->pdata));
				memcpy(&(prof_data->results[chn].value), result.pdata, sizeof(long));
				free(result.pdata);
				result.pdata = NULL;
			}
			if(infonum == 3 ||infonum ==4)
				IEC102_DEBUG("[%s], %d, get regnum[%d] data=%d \n",__FUNCTION__,__LINE__,regnum,prof_data->results[chn].value);
			//else
				//IEC102_DEBUG("[%s], %d, get regnum[%d] data=%f \n",__FUNCTION__,__LINE__,regnum,*((float*)&(prof_data->results[chn].value)));
		}else
		{
			IEC102_DEBUG("WARNING : [%s], %d, get regnum[%d] Error !\n",__FUNCTION__,__LINE__,regnum);
		}
		infonum++;
	}
	
	return ENN_SUCCESS;
}


static UINT8 iec102_put_rbuf_IT(DPA_CONN_CONTEXT *context, UINT16 ioa,  T_IEC102_DATA *dd,
                                UINT8 metsize, T_READ_IT_PARAM *preaditparam)
{
	T_IEC102_IT_SEQ         status;
	long                 value;
	UINT16               itfrom;
	UINT8                num;
	DATA_PROFILE_RESULTS 	*rbuf;

	itfrom = preaditparam->itfrom;
	num = preaditparam->count;
	rbuf = preaditparam->rbuf;

	dd->vsq.bf.num++;
	dd->udata[num] = ioa;
	num++;

	/*get the intergrated total's value*/
	value = rbuf->results[ioa-itfrom].value;
	//value = 60;

	memcpy(&dd->udata[num],&value, metsize);

	//INFO( "dd->udata[%d]=%d\n",num, dd->udata[num]);
	num += metsize;

	/* Get status of fetched integrated total */
	{
		status.seq = context->seq.attr.iec870_102attr.status.seq;
		//status.iv = (rbuf->results[ioa-itfrom].status & (DATA_DATA_INVALID | DATA_DATA_NODATA)) ? 1 : 0 ;
		status.iv = 1;
		//status.ca =  (rbuf->results[ioa-itfrom].status & DATA_DATA_ADJUST) ? 1 : 0 ;
		status.ca = 0;
		//status.cy =  (rbuf->results[ioa-itfrom].status & DATA_DATA_OVERRUN) ? 1 : 0 ;
		status.cy = 0;
		memcpy(&dd->udata[num],&status,1); /* Status of integrated total */
	}
	num++;

	if (IEC870_5_102_SIGNATURE_ON)
	{
		if (dd->type < 8)
		{
			dd->udata[num] = byte_checksum(&dd->udata[num-6], 6); /* Signature */
		}
		else
		{
			dd->udata[num] = 0;
		}
		num++;
	}

	WARN("num=%d\n", num);
	return num;
}


static UINT16 iec102_getprof_fromdb(DPA_CONN_CONTEXT *context, DATA_PROFILE_PARAM *pprof_para,
                                          DATA_PROFILE_RESULTS *pprof_datas)
{
	//DATA_HANDLE	pdatahdl;
	UINT32		ret = 0;


	if (NULL == context || NULL == pprof_para || NULL == pprof_datas)
	{
	    ERR( "NULL pointer!\n");
	    return -1;
	}

/*    pdatahdl = pPort->ptbl->pdatahdl;
    if (NULL == pdatahdl)
    {
        ERR( "NULL pointer!\n");
	    return -1;
    }*/

	ret = data_profile_read(context, pprof_para, pprof_datas);
	if (ret == ENN_SUCCESS)
	{
		WARN("data_profile_read() return success\n");
        	ret = 0;
	}
	else
	{
		ERR("data_profile_read() return %d\n", ret);
		ret = -1;
	}

	return ret;
}


static UINT8 iec102_get_profile(DPA_CONN_CONTEXT *context, T_READ_IT_PARAM *preaditparam)
{
    UINT32                 ret = 0;
    //u_int32_t               oldeststamp = 0;
    UINT32               readstamp = 0;
    //u_int32_t               stamp = 0;
    DATA_PROFILE_PARAM  	prof_para;
    UINT32               meaperiod=60;

    if ((NULL == preaditparam) || (NULL == preaditparam->pstamp) ||
        (NULL == preaditparam->pstatus) || (NULL == preaditparam->rbuf) ||
        (NULL == context))
    {
        ERR( "NULL pointer!\n");
        return 0xff;
    }
    *(preaditparam->pstamp)  = 0;
    *(preaditparam->pstatus) = 0;

    readstamp = preaditparam->stamp;
    //oldeststamp = iec102_get_prof_oldeststamp(ptbl, prof);
    //readstamp = stamp < oldeststamp ? oldeststamp : stamp;

    /* Check for valid stamp */
    if (0 == readstamp)
    {
        return 0xff;
    }

    /* Clear register buffer, valid flags, and stamp*/
    //prof_para.prof = prof;
    prof_para.stamp = readstamp;
    prof_para.from = preaditparam->itfrom;
    prof_para.to = preaditparam->itto;

    meaperiod = context->data_period;	//pPort->pMProfCfg->mp_period;

    if (prof_para.stamp%meaperiod != 0)
    {
        prof_para.stamp = (prof_para.stamp/meaperiod)*meaperiod + meaperiod;
    }

    WARN("Really read prof's stamp: prof_para.stamp=%ld\n", prof_para.stamp);
    ret = iec102_getprof_fromdb(context, &prof_para, preaditparam->rbuf);
    if ( ret < 0)
    {
        ERR( "iec102_getprof_fromdb return ret=%d <0\n", ret);
        return 0xff;
    }
    if (preaditparam->rbuf->chn_nums <= 0)
    {
        INFO( "profile_result.chn_nums=%d <=0\n", preaditparam->rbuf->chn_nums);
        return 0xff;
    }

    *(preaditparam->pstamp) = preaditparam->rbuf->stamp;
    *(preaditparam->pstatus) = preaditparam->rbuf->pf_status;
    return  1;
}


static UINT8 iec102_read_IT(DPA_CONN_CONTEXT *context, T_READ_IT_PARAM *preaditparam)
{
	UINT8                prof;
	UINT8                mp;
	UINT8                found = 0;
	UINT8                profresult = 0xff;
	char                    bstampvalid = FALSE;
	UINT32               oldeststamp = 0;
	UINT32               readstamp = 0;
	UINT32               stamp = 0;
	UINT32               *pstamp = NULL;
	UINT32				period;

	if (NULL == preaditparam || NULL == context)
	{
		ERR( "NULL pointer!\n");
		return 0;
	}

	found = 0;
	//mp = pPort->context.seq.attr.iec870_102attr.lastasdu.dui.rec_addr % 10;
	//prof = pPort->pCfgMaster->lp_offset;
	readstamp = preaditparam->stamp;

	profresult = iec102_get_profile(context, preaditparam);
	WARN("preaditparam->stamp = %d, preaditparam->pstamp = %d\n",preaditparam->stamp,*(preaditparam->pstamp));
	pstamp = preaditparam->pstamp;
	period = context->data_period;	//pPort->pMProfCfg->mp_period;
	bstampvalid = ((*pstamp>readstamp ? *pstamp-readstamp : readstamp-*pstamp) < period);
	if ((profresult != 0xFF) && bstampvalid)
	{
		found = 1;
	}

	return found;
}


static UINT8 iec102_put_ALL_IT(DPA_CONN_CONTEXT *context, UINT8 type, T_READ_IT_PARAM *preaditparam,
                                       T_IEC102_DATA *dd)
{
	UINT8                mp, metsize, prof;
	UINT16               status,status2;
	UINT32               currstamp = 0, timea_stamp=0, sys_stamp=0;
	UINT32               stamp2;
	UINT8                it_valid = 1;
	T_READ_IT_PARAM         readitparam;
	T_READ_IT_PARAM         readitparam2;
	UINT16               itfrom, itto;
	UINT16               configed_itto;
	int                     i;
	UINT8                count;
	UINT32               *pstamp = NULL;
	T_IEC102_DUI            *plastdui = NULL;
	T_IEC102_ATTR           *p102attr = NULL;
	U_IEC102_INFOOBJ        *pinfobj = NULL;
	UINT32               flash_writestamp=0;
	UINT8                ifsub = 0xff;
	UINT16               period; 
	
	if (NULL == preaditparam || NULL == dd || NULL == context)
	{
		ERR( "NULL pointer!\n");
		return IEC870_CAUSE_NONE;
	}

	plastdui = &context->seq.attr.iec870_102attr.lastasdu.dui;
	p102attr = &context->seq.attr.iec870_102attr;
	pinfobj = &context->seq.attr.iec870_102attr.lastasdu.infobj;
	metsize = 4 - ((context->seq.attr.iec870_102attr.udid + 1) % 3);

	dd->vsq.bf.num = 0;

	//mp = plastdui->rec_addr % 10;
	mp = plastdui->rec_addr;

	currstamp = preaditparam->stamp;
	itfrom = preaditparam->itfrom;
	itto = preaditparam->itto;
	count = preaditparam->count;
	memset(&readitparam, 0, sizeof(readitparam));

	/* Search profiles with matched period */
	configed_itto = 100;	//pPort->pMProfCfg->chn_count;
	period = context->data_period;	//60s, pPort->pMProfCfg->mp_period;
	if (type == IEC870_TIME_AND_RANGE)
	{
	    if (p102attr->lastsstamp != 0)
	    {
	        currstamp = p102attr->lastsstamp;
	        WARN( "p102attr->lastsstamp=%d\n",p102attr->lastsstamp);
	    }
	}

	/*check the range of IT addr, get the intersection of itfrom~itto and 1~configed_itto*/
	if ((itfrom < 1) || (itfrom > configed_itto) || (itfrom > itto))
	{
		it_valid = FALSE;
	}

	if (itto > configed_itto)
	{
		itto = configed_itto;
	}

	WARN( "currstamp=%d, it_valid=%d, configed_itto=%d,itto=%d\n",
	        currstamp,it_valid,configed_itto, itto);
	if (it_valid)
	{
	        readitparam.itfrom = itfrom;
	        readitparam.itto = itto;
	        readitparam.stamp = currstamp;
	        readitparam.pstamp = preaditparam->pstamp;
	        readitparam.rbuf = &context->prof_data;
	        readitparam.pstatus = &status;

		//WARN("readitparam.pstamp = %d\n",*(preaditparam->pstamp));
	        if (!iec102_read_IT(context, &readitparam))
	        {
			ERR("iec102_read_IT() return 0\n");
			memset(&readitparam.rbuf, 0, sizeof(readitparam.rbuf));
			plastdui->cause.bf.cause  = IEC870_CAUSE_ACTIV_TERM;//IEC870_CAUSE_REQ;
			context->ret_immedt = 1;
			p102attr->iec870seq.cseq=0;

			return count;
	        }

		for (i = itfrom; i <= itto; i++)
		{
			if ((count + metsize + 2 + IEC870_5_102_SIGNATURE_ON + IEC870_TIMEA_SIZE) > IEC870_5_102_UDATASIZE)
			{
				WARN( "count > IEC870_5_102_UDATASIZE\n");
				preaditparam->itfrom = i; 
				plastdui->cause.bf.cause = IEC870_CAUSE_REQ;

				if (type == IEC870_OLDEST || type == IEC870_PAST)
				{
					context->ioa_not_finish = 1;
				}
				return count;
			}

			count = iec102_put_rbuf_IT(context, i, dd, metsize, &readitparam);
			readitparam.count = count;

			WARN("iec102_put_rbuf_IT() return count=%d\n", count);
		}
	}

	if ((type == IEC870_OLDEST) || (type == IEC870_OLDEST_AND_RANGE)||(type == IEC870_PAST))
	{
		p102attr->lastsstamp = 0;
		context->ioa_not_finish = 0;
	}

	if (type == IEC870_TIME_AND_RANGE)
	{
		pstamp = preaditparam->pstamp;
		timea_stamp = iec102_systimea_stamp(&pinfobj->io120.tat);
		sys_stamp = iec102_get_sys_timestamp();
		/*WARN( "*pstamp+period=%d, timea_stamp=%d, sys_stamp=%d\n",
		    *pstamp+period,timea_stamp,sys_stamp);*/
		/* Increment time stamp */
		if ((*pstamp+period) > timea_stamp
		|| (*pstamp+period) > sys_stamp)
		{
			WARN("*pstamp+pmeaperiod[mp]) > timea_stamp or >sys_stamp");
			plastdui->cause.bf.cause = IEC870_CAUSE_ACTIV_TERM;
			p102attr->lastsstamp = 0;
		}
		else
		{
			plastdui->cause.bf.cause = IEC870_CAUSE_REQ;
			p102attr->lastsstamp = *pstamp + period;
			WARN( "p102attr->lastsstamp=%d, *pstamp=%d, period=%d\n", 
				p102attr->lastsstamp, *pstamp, period);
			preaditparam->itfrom = context->orig_itfrom;
		}
	}
	else
	{
		plastdui->cause.bf.cause = IEC870_CAUSE_ACTIV_TERM;
		WARN("type is %d, cause is %d\n", type, plastdui->cause.bf.cause);
	}

	/* For Sequence Number*/
	p102attr->status.seq++;
	if (p102attr->status.seq > 31)
	{
		p102attr->status.seq = 0;
	}

	return count;
}


static UINT8 iec102_prepare_IT(DPA_CONN_CONTEXT *context, T_IEC102_DATA *dd)
{
    UINT16           ioa;
    UINT8            c, mp, itdemand;
    UINT32           itstamp = 0;
    UINT32           stamp = 0;
    UINT8            timecs, prof;
    UINT16           itfrom = 0;
    UINT16           itto = 0;
    UINT16           configed_itto;
    struct tm           dt;
    T_READ_IT_PARAM     readitparam;
    U_IEC102_INFOOBJ    *plastinfobj = NULL;
    T_IEC102_DUI        *plastdui = NULL;

	if ((NULL == dd) || (NULL == context))
	{
		ERR( "NULL pointer!\n");
		return 0;
	}

	plastdui = &context->seq.attr.iec870_102attr.lastasdu.dui;
	plastinfobj = &context->seq.attr.iec870_102attr.lastasdu.infobj;
	c = 0;

	/* Search profiles with matched period */
	configed_itto =100; 	//pPort->pMProfCfg->chn_count;

	itdemand = (context->seq.attr.iec870_102attr.lastasdu.dui.type < 120) ?
	    (plastdui->type % 4) : IEC870_TIME_AND_RANGE;
	//IEC102_DEBUG("[%s],%d, itdemand= %d\n",__FUNCTION__,__LINE__,itdemand);

	switch (itdemand)
	{
		/**** IT of oldest integration period ****/
		case IEC870_OLDEST:
			itstamp = 0;
			dd->vsq.bf.num = 0;

			if (context->ioa_not_finish == 1)
			{
				itfrom = plastinfobj->io120.itfrom;
			}
			else
			{
				itfrom = 1;
			}

			itto = configed_itto;
		break;

		/**** IT of oldest integration period and range of addresses ****/
		case IEC870_OLDEST_AND_RANGE:
			itstamp = 0;
			itfrom = plastinfobj->io105.itfrom;
			itto = plastinfobj->io105.itto;
		break;

		/**** IT of specific past integration period ****/
		case IEC870_PAST:
			itstamp = iec102_systimea_stamp(&plastinfobj->io106.ta);
			dd->vsq.bf.num = 0;

			WARN("itstamp=%d\n", itstamp);

			if (context->ioa_not_finish == 1)
			{
			itfrom  = plastinfobj->io120.itfrom;
			//itstamp = pPort->last_stamp;
			}
			else
			{
			itfrom = 1;
			//pPort->last_stamp = itstamp;
			}

			itto = configed_itto;
		break;

		/**** IT of specific past integration period and range of addresses ****/
		case IEC870_PAST_AND_RANGE:
			itstamp = iec102_systimea_stamp(&plastinfobj->io107.ta);
			itfrom = plastinfobj->io107.itfrom;
			itto = plastinfobj->io107.itto;
		break;

		/**** IT of time range and range of addresses ****/
		case IEC870_TIME_AND_RANGE:
			itstamp = iec102_systimea_stamp(&plastinfobj->io120.taf);
			itfrom = plastinfobj->io120.itfrom;
			itto = plastinfobj->io120.itto;
		break;
		default:
		break;
	};

	readitparam.itfrom = itfrom;
	readitparam.itto = itto;
	readitparam.stamp = itstamp;
	readitparam.pstamp = &stamp;
	readitparam.count = c;

	WARN( "readitparam.itfrom=%d, readitparam.itto=%d, readitparam.stamp=%d\n",
	         readitparam.itfrom, readitparam.itto,readitparam.stamp);
	//WARN("readitparam.pstamp = %d\n",*(readitparam.pstamp));
	c = iec102_put_ALL_IT(context, itdemand, &readitparam, dd);

	plastinfobj->io120.itfrom = readitparam.itfrom;

	/**** Attach TimeA information: COMMON TIME TAG OF ASDU ****/
	iec102_stamp_to_date(stamp,&dt);

	sysdatetime_to_IEC870_time(&dt, &dd->udata[c]);

    /*the sum of (asdu addr + record addr + info obj addr + intergrated totals(4 bytes) + status of  intergrated totals
        + time info + data type ident) mod 256 */
    if ((dd->type < 8) && IEC870_5_102_SIGNATURE_ON)
    {
        timecs = byte_checksum(&dd->udata[c], IEC870_TIMEA_SIZE);
        for (ioa=6; ioa<c; ioa+=7)
        {
            dd->udata[ioa] = dd->udata[ioa] + timecs + dd->type +
                    dd->dte_addr_hi + dd->dte_addr_lo + dd->rec_addr;
        }
    }

    c += IEC870_TIMEA_SIZE;

    return(c);
}


void iec102_slave(DPA_CONN_CONTEXT *context, UINT8* pdui)
{
	UINT8	bufp;
	UINT8	respix;
	UINT32	milisec;
	struct tm   dt;
	UINT16         sync_timelimit = 0;
	T_IEC102_COMM_SEQ           *piec870seq = NULL;
	T_IEC102_SEQUENCE	seq;
	T_IEC102_COMM_SEQ           *piec102comseq = NULL;
	T_IEC102_ASDU               *pasdu = NULL;
	T_IEC102_DUI                *pasdudui = NULL;
	T_IEC102_DUI                *plastdui = NULL;
	U_IEC102_INFOOBJ            *pinfobj = NULL;
	U_WORD                      mpa;
	
	 if (NULL == context|| NULL == pdui)
	{
	    IEC102_DEBUG( "NULL pointer!,%s,%d\n",__FUNCTION__,__LINE__);
	    return;
	}

	seq.lctrl = context->seq.lctrl;
	memcpy(&seq.attr.iec870_102attr,&context->seq.attr.iec870_102attr,
	       sizeof(seq.attr.iec870_102attr));
	
	//IEC102_DEBUG("[%s],%d,laddr=%d\n", __FUNCTION__,__LINE__,context->seq.attr.iec870_102attr.laddr);

	piec102comseq = &(context->seq.attr.iec870_102attr.iec870seq);
	pasdu = &seq.attr.iec870_102attr.lastasdu;
	pasdudui = &pasdu->dui;
	pinfobj = &pasdu->infobj;
	piec870seq = &seq.attr.iec870_102attr.iec870seq;
	plastdui = &context->seq.attr.iec870_102attr.lastasdu.dui;
	
	switch(pdui[0] & DLC_FUNC)
	{
		case DLCF_PSC_RESLINK:
		        memset(pasdu, 0, sizeof(T_IEC102_ASDU));
		        piec870seq->cseq = 0;
		        piec870seq->extcom = 0;
		        seq.attr.iec870_102attr.ldid = DLCF_SC_ACK;
		        seq.attr.iec870_102attr.status.seq = 0;

			//add by hyptek,20150710
			context->initflag = 1;

		        //if (piec102comseq->initind == 1)
		        {
		            piec870seq->commrun = 1;
		            piec870seq->initind = 0;
		            piec870seq->class1ind = 0;
		            piec870seq->eventind = 0;

		            if (context->initflag == 1)
		            {
		                piec870seq->eventind = 1;
		                piec870seq->initind = 1;
		            }
		        }
		        IEC102_DEBUG("[%s],%d,piec870seq->initind=%d,piec870seq->commrun=%d\n", 
					__FUNCTION__,__LINE__,piec870seq->initind,piec870seq->commrun);
		        //WARN("piec870seq->commrun=%d\n", piec870seq->commrun);
		        break;
		case DLCF_PRR_LINKSTA:
		        piec870seq->cseq = 0;
		        piec870seq->extcom = 0;
		        seq.attr.iec870_102attr.ldid = DLCF_SR_STAT;
		        piec870seq->initind = 1;
		        piec870seq->class1ind = 0;
		        piec870seq->eventind = 0;
		        break;

		case DLCF_PSC_DATA:{
			IEC102_DEBUG("[%s],%d : piec102comseq->commrun = %d, piec102comseq->cseq = %d, pasdudui->type = %d\n",
				__FUNCTION__,__LINE__,piec102comseq->commrun,piec102comseq->cseq , pasdudui->type);
			if (piec102comseq->commrun == 0)
			{
				IEC102_DEBUG( "piec102comseq->commrun = 0,%s,%d\n",__FUNCTION__,__LINE__);
				return;
			}

			if ((piec102comseq->cseq == 1) &&
			    (pdui[5] != IEC870_CAUSE_DEACTIV)) /*pdui[5] is the cause of transmition*/
			{
			    seq.attr.iec870_102attr.ldid = DLCF_SC_NACK;
			    break;
			}

			seq.attr.iec870_102attr.ldid = DLCF_SC_ACK;
			/***********************************************/

			if (pdui[5] == IEC870_CAUSE_DEACTIV)
			{
			    /*pdui[3] is the data type identify*/
			    if ((iec102_deactivation(&pdui[3], context)) &&
			        (piec102comseq->cseq == 1))
			    {
			        pasdudui->cause.bf.confirm = 0;  /*Positive ack*/
			        pasdudui->cause.bf.cause = IEC870_CAUSE_DEACTIV_CON;
			    }
			    else
			    {
			        seq.attr.iec870_102attr.ldid = DLCF_SC_NACK;                     /*Negative ack*/
			    }
			    break;
			}

			piec870seq->cseq = 1;
			piec870seq->req_all = 0;
			seq.attr.iec870_102attr.lastsstamp = 0;
			//seq.attr.iec870_102attr.lastevi = 0;
			memcpy(pasdu, &pdui[3], IEC870_5_102_DUI_SIZE);

			if ((pasdudui->type == IEC870_TYPE_C_RD_NA_2) ||
			 (pasdudui->type == IEC870_TYPE_C_TI_NA_2))
			{
			    /* Polling sequence */
			    if (pasdudui->cause.bf.cause != IEC870_CAUSE_REQ)
			    {
			        pasdudui->cause.bf.cause = IEC870_CAUSE_TYPE_NAVAILABLE;
			    }
			}
			else if (pasdudui->type == IEC870_C_SYN_TA_2)
			{
			    /* Time synchronization sequence */
			    if (pasdudui->cause.bf.cause != IEC870_CAUSE_CHINA_SYNCH)
			    {
			        pasdudui->cause.bf.cause = IEC870_CAUSE_TYPE_NAVAILABLE;
			    }
			}
			else
			{
			    /* ACT, ACTCON, ACTTERM sequence */
			    if (pasdudui->cause.bf.cause != IEC870_CAUSE_ACTIV)
			    {
			        pasdudui->cause.bf.cause = IEC870_CAUSE_TYPE_NAVAILABLE;
			    }
			}

			if ((pasdudui->dte_addr_lo == pdui[1]) &&
			    (pasdudui->dte_addr_hi == pdui[2]))
			{
			    /* If Link address & DTE address equal */
			    mpa.wb.hb = pasdudui->dte_addr_hi;
			    mpa.wb.lb = pasdudui->dte_addr_lo;
			    if (mpa.w == 0)
			    {
			        pasdudui->cause.bf.cause = IEC870_CAUSE_ADDR_NKNOWEN;
			    }
			}

			/* Check DTE address, for general information demands equal to link address */
			if ((pasdudui->type == IEC870_TYPE_C_RD_NA_2) ||
			    (pasdudui->type == IEC870_TYPE_C_TI_NA_2) ||
			    (pasdudui->type == IEC870_TYPE_C_SP_NA_2) ||
			    (pasdudui->type == IEC870_TYPE_C_SP_NB_2))
			{
			    if ((pasdudui->dte_addr_lo != pdui[1]) ||
			        (pasdudui->dte_addr_hi != pdui[2]))
			    {
			        pasdudui->cause.bf.cause = IEC870_CAUSE_ADDR_NKNOWEN;
			    }
			}

			/* ASDU-s with information objects */
			bufp = 3+IEC870_5_102_DUI_SIZE;
			switch(pasdudui->type)
			{
			case IEC870_TYPE_C_RD_NA_2:
			case IEC870_TYPE_C_SP_NA_2:
			case IEC870_TYPE_C_TI_NA_2:
			case IEC870_TYPE_C_CI_NA_2: /* ASDU-s with no Information objects go here */
			case IEC870_TYPE_C_CI_NE_2:
			case IEC870_TYPE_C_CI_NJ_2:
			case IEC870_TYPE_C_CI_NN_2:
				seq.attr.iec870_102attr.ldid = DLCF_SC_ACK_E5;
				IEC102_DEBUG("[%s],%d,pasdudui->type = %d\n",__FUNCTION__,__LINE__,pasdudui->type);
			    break;

			case IEC870_TYPE_C_SP_NB_2:
			    memcpy(&pinfobj->io102.taf, &pdui[bufp], IEC870_TIMEA_SIZE);
			    bufp += IEC870_TIMEA_SIZE;
			    memcpy(&pinfobj->io102.tat, &pdui[bufp], IEC870_TIMEA_SIZE);
			    //seq.attr.iec870_102attr.lastsstamp = iec102_systimea_stamp(&pasdu->infobj.io102.taf);
			    break;

			case IEC870_TYPE_C_CI_NC_2:
			case IEC870_TYPE_C_CI_NG_2:
			case IEC870_TYPE_C_CI_NL_2:
			case IEC870_TYPE_C_CI_NP_2:
			    memcpy(pinfobj, &pdui[bufp], IEC870_SIZE_C_CI_NC_2);
			    break;

			case IEC870_TYPE_C_CI_ND_2:
			case IEC870_TYPE_C_CI_NH_2:
			case IEC870_TYPE_C_CI_NM_2:
			case IEC870_TYPE_C_CI_NQ_2:
			    memcpy(pinfobj, &pdui[bufp], IEC870_SIZE_C_CI_ND_2);
			    break;

			case IEC870_TYPE_C_CI_NB_2:
			case IEC870_TYPE_C_CI_NF_2:
			case IEC870_TYPE_C_CI_NK_2:
			case IEC870_TYPE_C_CI_NO_2:
			    memcpy(pinfobj, &pdui[bufp], IEC870_SIZE_C_CI_NB_2);
			    break;

			case IEC870_TYPE_C_CI_NR_2:
			case IEC870_TYPE_C_CI_NS_2:
			case IEC870_TYPE_C_CI_NT_2:
			case IEC870_TYPE_C_CI_NU_2:
				seq.attr.iec870_102attr.ldid = DLCF_SC_ACK_E5;
				IEC102_DEBUG("[%s],%d,pasdudui->type = %d\n",__FUNCTION__,__LINE__,pasdudui->type);
				memcpy(&pinfobj->io120, &pdui[bufp], IEC870_TIMEA_SIZE+2);
				bufp = bufp + 2 + IEC870_TIMEA_SIZE;
				memcpy(&pinfobj->io120.tat, &pdui[bufp], IEC870_TIMEA_SIZE);
				//pPort->orig_itfrom = pinfobj->io120.itfrom;
				context->orig_itfrom = pinfobj->io120.itfrom;
			    break;

			/* Time Set */
			case IEC870_C_SYN_TA_2:
			    seq.attr.iec870_102attr.ldid	= DLCF_SC_ACK_E5;

			    memcpy(pinfobj, &pdui[3+IEC870_5_102_DUI_SIZE], IEC870_TIMEB_SIZE);
			    iec102_mstationtime_to_systime(&dt, &pdui[3+IEC870_5_102_DUI_SIZE], IEC870_5_102_TIMEB);

			    milisec = pdui[3+IEC870_5_102_DUI_SIZE] + ((pdui[4+IEC870_5_102_DUI_SIZE] & 0x03)<<8);

			    if (pasdudui->cause.bf.cause != IEC870_CAUSE_CHINA_SYNCH)
			    {
			        WARN( "cause != IEC870_CAUSE_CHINA_SYNCH\n");
			        break;
			    }

			    if (!iec102_valid_time(dt.tm_hour, dt.tm_min, dt.tm_sec) ||
			        !iec102_valid_date(dt.tm_year, dt.tm_mon, dt.tm_mday))
			    {
			        pasdudui->cause.bf.cause = IEC870_CAUSE_CHINA_SYNCH;
			        pasdudui->cause.bf.confirm = 1;

			        break;
			    }

			    /*if the main sync_flag is not set, do not need to synchronize time*/
			    if (context->sync_time_flag)
			    {
			        iec102_syn_datetime_ms(dt, milisec, iec102_get_sys_timestamp(), sync_timelimit);
			        pasdudui->cause.bf.confirm = 0;
			    }
			    else
			    {
			        pasdudui->cause.bf.confirm = 1;
			    }
			    pasdudui->cause.bf.cause = IEC870_CAUSE_CHINA_SYNCH;
			    break;
			default:
			    break;
			}
			break;
		}
		case DLCF_PRR_DATA1:{
			IEC102_DEBUG("[%s],%d : piec102comseq->initind = %d, context->initflag = %d, pasdudui->type = %d\n",
				__FUNCTION__,__LINE__,piec102comseq->initind,context->initflag, pasdudui->type);
			if (piec102comseq->commrun == 0)
			{
			    IEC102_DEBUG("Error : piec102comseq->commrun == 0!,[%s],%d\n",__FUNCTION__,__LINE__);
			    return;
			}

			seq.attr.iec870_102attr.ldid = DLCF_SR_DATA;

			if ((piec102comseq->initind == 1) &&
			    (context->initflag == 1))
			{
			    seq.attr.iec870.udid = IEC870_TYPE_M_EI_NA_2;
			    piec870seq->eventind = 0;
			    break;
			}

			if (piec102comseq->eventind == 1)
			{
			    seq.attr.iec870_102attr.udid = IEC870_TYPE_M_SP_TA_2;
			    break;
			}

			if (piec102comseq->cseq == 0)
			{
			    memset(pasdu, 0, sizeof(seq.attr.iec870_102attr.lastasdu));
			    seq.attr.iec870_102attr.ldid = DLCF_SR_NACK;
			    piec870seq->extcom = 0;
			    break;
			}
			//IEC102_DEBUG("[%s],%d : pasdudui->type= %d\n",__FUNCTION__,__LINE__,pasdudui->type);
			switch(pasdudui->type)
			{

				/****** Implemented standard ASDU-s ******/
				case IEC870_TYPE_C_RD_NA_2:  /* Read manufacturer and product specification */
				    piec870seq->cseq = 0;
				    if (pasdudui->cause.bf.cause != IEC870_CAUSE_REQ)
				    {
				        seq.attr.iec870_102attr.udid = IEC870_TYPE_C_RD_NA_2;
				    }
				    else
				    {
				        seq.attr.iec870_102attr.udid = IEC870_TYPE_P_MP_NA_2;
				    }
				    break;

				case IEC870_TYPE_C_TI_NA_2: /* Read System Time Request */
				    piec870seq->cseq = 0;
				    if (pasdudui->cause.bf.cause != IEC870_CAUSE_REQ)
				    {
				        seq.attr.iec870_102attr.udid = IEC870_TYPE_C_TI_NA_2;
				    }
				    else
				    {
				        seq.attr.iec870_102attr.udid = IEC870_TYPE_M_TI_TA_2;
				    }
				    break;

				case IEC870_TYPE_C_SP_NA_2:
				case IEC870_TYPE_C_SP_NB_2:

				    switch (plastdui->cause.bf.cause)
				    {
				    case IEC870_CAUSE_ACTIV:
				        seq.attr.iec870_102attr.udid = plastdui->type;
				        pasdudui->cause.bf.cause = IEC870_CAUSE_OBJ_NAVAILABLE;
				        piec870seq->cseq = 0;
				        break;

				    case IEC870_CAUSE_ACTIV_CON:
				    case IEC870_CAUSE_REQ:
				        seq.attr.iec870_102attr.udid = IEC870_TYPE_M_SP_TA_2;
				        break;

				    case IEC870_CAUSE_DEACTIV_CON:
				        seq.attr.iec870_102attr.udid = plastdui->type;
				        break;

				    default:
				        seq.attr.iec870_102attr.udid = plastdui->type;
				        piec870seq->cseq = 0;
				        break;
				    }
				    break;

				case IEC870_TYPE_C_CI_NA_2:
				case IEC870_TYPE_C_CI_NB_2:
				case IEC870_TYPE_C_CI_NC_2:
				case IEC870_TYPE_C_CI_ND_2:
				case IEC870_TYPE_C_CI_NE_2:
				case IEC870_TYPE_C_CI_NF_2:
				case IEC870_TYPE_C_CI_NG_2:
				case IEC870_TYPE_C_CI_NH_2:
				case IEC870_TYPE_C_CI_NR_2:
				case IEC870_TYPE_C_CI_NS_2:
					IEC102_DEBUG("[%s],%d : plastdui->cause.bf.cause = %d, plastdui->type= %d\n",
						__FUNCTION__,__LINE__,plastdui->cause.bf.cause,plastdui->type);
					switch (plastdui->cause.bf.cause)
					{
						case IEC870_CAUSE_ACTIV:
							seq.attr.iec870_102attr.udid = plastdui->type;
							pasdudui->cause.bf.cause = iec102_check_IT(context);
							IEC102_DEBUG("[%s],%d : pasdudui->cause.bf.cause = %d\n",__FUNCTION__,__LINE__,pasdudui->cause.bf.cause);
							if (pasdudui->cause.bf.cause != IEC870_CAUSE_ACTIV_CON)
							{
								piec870seq->cseq = 0;
							}
						break;

						case IEC870_CAUSE_ACTIV_CON:
						case IEC870_CAUSE_REQ:
							respix =((plastdui->type - IEC870_TYPE_C_CI_NA_2) < 15) ?
							    ((plastdui->type - IEC870_TYPE_C_CI_NA_2) /  4) :
							    (plastdui->type % 4);
							seq.attr.iec870_102attr.udid = iec102_asdu_tbl[respix];
							IEC102_DEBUG("[%s],%d : seq.attr.iec870_102attr.udid= %d\n",__FUNCTION__,__LINE__,seq.attr.iec870_102attr.udid);

						break;

						case IEC870_CAUSE_DEACTIV_CON:
							seq.attr.iec870_102attr.udid = plastdui->type;
						break;

						default:
							IEC102_DEBUG("WARNIG : plastdui->cause.bf.cause=%d,%s,%d\n", 
							plastdui->cause.bf.cause,__FUNCTION__,__LINE__);
							IEC102_DEBUG("plastdui->type=%d\n", plastdui->type);
							IEC102_DEBUG("piec870seq->cseq=%d\n", piec870seq->cseq);
							seq.attr.iec870_102attr.udid = plastdui->type;
							piec870seq->cseq = 0;
						break;
					}
					break;
			    
			case IEC870_C_SYN_TA_2:
			    seq.attr.iec870_102attr.udid = plastdui->type;
			    piec870seq->cseq = 0;
			    break;

			/* Standard ASDU-s which are not implemented - NOT AVAILABLE */
			default:
			    if (plastdui->type == 0)	/* No data was requested */
			    {
			        seq.attr.iec870_102attr.ldid = DLCF_SR_NACK;
			        piec870seq->cseq = 0;
			    }
			    else
			    {
			        seq.attr.iec870_102attr.udid = plastdui->type;
			        pasdudui->cause.bf.cause = IEC870_CAUSE_TYPE_NAVAILABLE;
			        piec870seq->cseq = 0;
			    }
			break;
			}
        		break;
		}
		case DLCF_PRR_DATA2:
		    seq.attr.iec870_102attr.ldid = DLCF_SR_NACK;
		    break;
		case FT12_LL_ERROR_ADDRESS:
		    IEC102_DEBUG("FT12_LL_ERROR_ADDRESS!,%s,%d\n",__FUNCTION__,__LINE__);
		    return;
		default:
			break;
	}
	memcpy(&context->seq, &seq, sizeof(seq));
	//IEC102_DEBUG("[%s],%d,linkaddr = %d\n",__FUNCTION__,__LINE__,seq.attr.iec870_102attr.laddr);
	IEC102_DEBUG("[%s],%d, iec870_102attr.ldid=%d, piec102comseq = %d\n", 
		__FUNCTION__,__LINE__,seq.attr.iec870_102attr.ldid,piec102comseq->commrun);
}

void iec102_slave_seq(DPA_CONN_CONTEXT *context)
{
	UINT16			length;
	U_WORD			linkaddr;
	UINT8			res_e5 ;
	T_IEC102_DATA		dd;
	T_IEC102_ATTR			*p102attr = NULL;
	T_IEC102_DUI			*plastdui = NULL;
	U_IEC102_INFOOBJ		*pinfobj = NULL;

	if (NULL == context)
	{
		IEC102_DEBUG( "NULL pointer!,%s,%d\n",__FUNCTION__,__LINE__);
		return;
	}

	p102attr = &context->seq.attr.iec870_102attr;
	plastdui = &context->seq.attr.iec870_102attr.lastasdu.dui;
	pinfobj = &context->seq.attr.iec870_102attr.lastasdu.infobj;

	linkaddr.w = p102attr->laddr;
	IEC102_DEBUG("%s,%d,linkaddr = %d\n",__FUNCTION__,__LINE__,linkaddr.w);

	/* Prepare Link Control and Address Fields */
	dd.addrhi = linkaddr.wb.hb;
	dd.addrlo = linkaddr.wb.lb;
	dd.ctrl   = p102attr->ldid;

	/* Prepare Data Unit Identifier Fields */
	dd.type         = p102attr->udid;
	dd.cause.b      = IEC870_CAUSE_NONE;
	dd.dte_addr_lo  = plastdui->dte_addr_lo;
	dd.dte_addr_hi  = plastdui->dte_addr_hi;
	dd.vsq.b        = 0;
	dd.rec_addr     = 0;

	p102attr->iec870seq.class1ind = p102attr->iec870seq.eventind || p102attr->iec870seq.cseq;

	IEC102_DEBUG( "[%s],%d,p102attr->iec870seq.eventind=%d, cseq=%d, ldid=%d, udid = %d\n", 
    		 __FUNCTION__,__LINE__,p102attr->iec870seq.eventind, p102attr->iec870seq.cseq, p102attr->ldid, p102attr->udid);

	switch (p102attr->ldid)
	{
		case DLCF_SC_ACK_E5:
			res_e5 = DLCF_SC_ACK_E5;
			FT12_102_tx_put(&res_e5,1, context);
			break;

		case DLCF_SR_NACK:
		case DLCF_SR_STAT:
		case DLCF_SC_ACK:
		case DLCF_SC_NACK:
			dd.ctrl = (p102attr->iec870seq.class1ind == 1) ? (p102attr->ldid | DLC_FCB_ACD) : p102attr->ldid;
			FT12_102_tx_put((UINT8 *)&dd,sizeof(T_IEC102_FLF_FIELDS), context);
			break;

		case DLCF_SR_DATA:
			dd.ctrl = (p102attr->iec870seq.class1ind == 1) ? (p102attr->ldid | DLC_FCB_ACD) :
			            p102attr->ldid;
			length = IEC870_5_102_DUI_SIZE;

			IEC102_DEBUG("[%s],%d,p102attr->iec870seq.eventind=%d, p102attr->iec870seq.cseq=%d,p102attr->udid = %d\n",
			                __FUNCTION__,__LINE__,p102attr->iec870seq.eventind, p102attr->iec870seq.cseq,p102attr->udid);
			switch (p102attr->udid)
			{
				case IEC870_TYPE_M_EI_NA_2:
				    length += iec102_end_init(context, &dd);
				    FT12_102_tx_put((UINT8 *)&dd, (UINT16)(length+3U), context);
				    break;

				case IEC870_TYPE_M_SP_TA_2:
				    dd.rec_addr = plastdui->rec_addr;
				    //iec102_check_resp(pPort, length, &dd);
				    break;

				 case IEC870_TYPE_P_MP_NA_2:
				    dd.vsq.b = 1;
				    dd.cause.bf.cause = IEC870_CAUSE_REQ;
				    //length += iec102_manufacturer(&dd.udata[0], IEC870_5_102_UDATASIZE);
				    FT12_102_tx_put((UINT8 *)&dd, (UINT16)(length+3U), context);
				    break;

				 case IEC870_TYPE_M_TI_TA_2:
				    dd.vsq.b = 1;
				    dd.cause.b = IEC870_CAUSE_REQ;
				    length += iec102_time(&dd.udata[0]);
				    FT12_102_tx_put((UINT8 *)&dd, (UINT16)(length+3U), context);
				    break;

				/* Respond with Integrated totals */
				case IEC870_TYPE_M_IT_TA_2:
				case IEC870_TYPE_M_IT_TD_2:

				    /*find data and respond */
				    dd.rec_addr = plastdui->rec_addr;

				    length += iec102_prepare_IT(context, &dd);

				    iec102_check_resp(context, length, &dd);

				    break;

				/* Respond with ativation confirmation or non-confirmation */
				case IEC870_TYPE_C_SP_NB_2:
				    //iec102_activ_conf(pPort, IEC870_SIZE_C_SP_NB_2, &dd);
				    break;

				case IEC870_TYPE_C_RD_NA_2:
				case IEC870_TYPE_C_TI_NA_2:
				case IEC870_TYPE_C_SP_NA_2:
				case IEC870_TYPE_C_CI_NA_2:
				case IEC870_TYPE_C_CI_NE_2:
				case IEC870_TYPE_C_CI_NJ_2:
				case IEC870_TYPE_C_CI_NN_2:
				    //iec102_activ_conf(pPort, IEC870_SIZE_C_CI_NA_2, &dd);
				    break;

				case IEC870_TYPE_C_CI_NB_2:
				case IEC870_TYPE_C_CI_NF_2:
				case IEC870_TYPE_C_CI_NK_2:
				case IEC870_TYPE_C_CI_NO_2:
				    //iec102_activ_conf(pPort, IEC870_SIZE_C_CI_NB_2, &dd);
				    break;

				case IEC870_TYPE_C_CI_NC_2:
				case IEC870_TYPE_C_CI_NG_2:
				case IEC870_TYPE_C_CI_NL_2:
				case IEC870_TYPE_C_CI_NP_2:
				    //iec102_activ_conf(pPort, IEC870_SIZE_C_CI_NC_2, &dd);
				    break;

				case IEC870_TYPE_C_CI_ND_2:
				case IEC870_TYPE_C_CI_NH_2:
				case IEC870_TYPE_C_CI_NM_2:
				case IEC870_TYPE_C_CI_NQ_2:
				    //iec102_activ_conf(pPort, IEC870_SIZE_C_CI_ND_2, &dd);
				    break;

				case IEC870_TYPE_C_CI_NR_2:
				case IEC870_TYPE_C_CI_NS_2:
				case IEC870_TYPE_C_CI_NT_2:
				case IEC870_TYPE_C_CI_NU_2:
				    iec102_activ_conf(context, IEC870_SIZE_C_CI_NR_2, &dd);
				    break;

				 /* Respond to chinese time set request */
				case IEC870_C_SYN_TA_2:
					iec102_synchro(context, &dd);
				break;

				default:
				    dd.type = plastdui->type;
				    dd.cause.bf.cause = IEC870_CAUSE_TYPE_NAVAILABLE;
				    length = IEC870_5_102_DUI_SIZE;
				    FT12_102_tx_put((UINT8 *)&dd, (UINT16)(length+3U), context);
				    p102attr->iec870seq.cseq = 0;
				    break;
			}
			break;
		default:
			break;

	    }
    
}

