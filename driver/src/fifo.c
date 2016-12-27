/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：FIFO.c
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/	
#include <iconv.h>

#include "ennAPI.h"
#include "fifo.h"

#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */

static int code_convert(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
    iconv_t cd;
    char **pin = &inbuf;
    char **pout = &outbuf;
	printf("%s, %s\n",from_charset,to_charset);
    cd = iconv_open(to_charset, from_charset);
    if (cd == -1)
    {
	printf("iconv_open return -1!!!!!!\n");
        perror("iconv_open:");
        return -1;
    }
    memset(outbuf,0,outlen);
    if (iconv(cd, pin, &inlen, pout, &outlen) == -1)
    {
        //printf("%s: call iconv failed!\n", __FUNCTION__);
        //printf("errno=%d\n", errno);
	printf("iconv_iconv failed!!!!!!\n");
        perror("iconv failed:\n");
        iconv_close(cd);
        return -1;
    }
    printf("iconv close!!!!!!\n");
    iconv_close(cd);
    return 0;
}

int UTF8_To_GB2312(char* inbuf, size_t inlen, char* outbuf, size_t outlen)
{
    return code_convert("utf-8", "gb2312", inbuf, inlen, outbuf, outlen);
}
int GB2312_To_UTF8(char* inbuf, size_t inlen, char* outbuf, size_t outlen)
{
    return code_convert("ISO_8859-1", "utf-8", inbuf, inlen, outbuf, outlen);
    //return code_convert("gb2312", "UTF-8", inbuf, inlen, outbuf, outlen);
}

ENN_S32 ENNFIFO_Open(const ENN_CHAR *fifoname, ENN_S32 flags)
{
    int fd;
	//printf("%s, %d, %s, %d\n",__FUNCTION__,__LINE__, fifoname , flags);	
	
    fd = open((char *)fifoname, (int)flags);
	//printf("%s, %d, fd = %d\n",__FUNCTION__,__LINE__, fd);	
    if(fd < 0)
    {
        //printf("FIFO open %s error\n",fifoname);
        //perror("FIFO open");
        return -1;
    }
    else
    {
        //printf("FIFO %s:Opened sucess!\n",fifoname);
    }
    
    return (ENN_S32)fd;
}

ENN_S32 ENNFIFO_create(ENN_CHAR *name)
{
    int ret = 0;
	
    if(access(name,F_OK) != 0)
    {
	    ret = mkfifo((char *)name, S_IRUSR|S_IWUSR);
	    if(ret != 0)
	    {
	        perror(name);            
            return -1;
	    }
		else
		{
            printf("make fifo:%s success!\n",name);
		}
	}
	
    return (ENN_S32)ret;
}


ENN_S32 ENNFIFO_write(ENN_S32 fd, ENN_CHAR *buff_w, ENN_S32 len_w)
{
    int ret = 0;

	ret = write((int)fd, (char *)buff_w, (int)len_w);
	
	return (ENN_S32)ret;
}

ENN_S32 ENNFIFO_read(ENN_S32 fd, ENN_CHAR *buff_r, ENN_S32 len_r)
{
      int n = read((int)fd, (char *)buff_r, (int)len_r);
      //if(n > 0)
          //printf("[%s]read fifo:%s\n",__FUNCTION__,buff_r);    
      return (ENN_S32)n;
}


#ifdef __cplusplus
#if __cplusplus
    }
#endif /* __cpluscplus */
#endif /* __cpluscplus */


