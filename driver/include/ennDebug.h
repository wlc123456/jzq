/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：ennDebug.h
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/
#ifndef _ENNDEBUG_H_
#define _ENNDEBUG_H_


#ifdef __cplusplus
extern "C"
{
#endif


#include <unistd.h>


typedef enum ENNAPI_ERRLEVEL_E
{
    ENNAPI_TRACE_DEBUG = 1,  /* debug-level                       */
    ENNAPI_TRACE_WARNING,    /* warning information               */
    ENNAPI_TRACE_ERROR,      /* error conditions                  */
    ENNAPI_TRACE_INFO
} ENNAPI_ERRLEVEL_E;

//#define ENNAPI_DEBUG
#define ENN_API_TRACE_LEVEL ENNAPI_TRACE_DEBUG


#ifdef ENNAPI_DEBUG
#define enn_api_trace( level, fmt, args... )\
    do { \
        if(level >= ENN_API_TRACE_LEVEL)\
        {\
            printf("[ENNAPI]%04d@%s(): ", __LINE__, __FUNCTION__);\
            printf( fmt, ##args );\
        }\
    } while (0)

#else
#define enn_api_trace( level, fmt, args... )
#endif

#define ENNAPI_ASSERT(expr)  do {      \
    if(!(expr)) { \
        ENNTRACE( "Assertion [%s] failed! %s:%s(line=%d)\n",\
#expr,__FILE__,__FUNCTION__,__LINE__); \
        _exit(0);     \
    } } while(0)


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

