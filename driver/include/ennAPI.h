
#ifndef _ENNAPI_H_
#define _ENNAPI_H_


#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/tcp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
//#include <linux/i2c-dev.h>

#include "ennType.h"
#include "ennDebug.h"
#include "ennOS.h"
#include "ennSocket.h"
#include "uart.h"
#include "fifo.h"
#include "led.h"


#define DEBUG_UART	printf
//#define DEBUG_UART


#define APP_NAME "102DPA"
#define APP_NO   "102dpa"
#define APP_VER  "1.00"


#define DBG_OFF       0
#define DBG_E         1     //error
#define DBG_WE      2	  //warning
#define DBG_IWE     3     //information


extern int DBG_LEVEL;
extern int LOG_LEVEL;

#define DBG(dbg, fmt, arg...)  do{if(DBG_LEVEL>=dbg) {printf(/*APP_NAME" "APP_VER */"[%s]:%d: ", __FUNCTION__, __LINE__); printf(fmt, ##arg);}}while(0)
#define INFO(fmt, arg...)	DBG(DBG_IWE,fmt,##arg)
#define WARN(fmt, arg...)	DBG(DBG_WE,fmt,##arg)
#define ERR(fmt, arg...)	DBG(DBG_E,fmt,##arg)



#ifdef __cplusplus
}

#endif /* __cplusplus */


#endif     /* _CYAPI_H_ */


