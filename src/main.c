#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <limits.h>
#include <asm/ioctls.h>
#include <time.h>
#include <pthread.h>
#include "ennOS.h"
#include "ennAPI.h"
#include "ModBus_TCP.h"


int DBG_LEVEL;

#define HYPTEK_DRIVER_CONFIG 1
#define PRINT_ENABLED 1


enum{
	MX257_P2_21_OUTPUT = 8,
	MX257_P3_15_OUTPUT,
	MX257_P3_16_OUTPUT,
	MX257_P3_17_OUTPUT,
	MX257_P2_10_OUTPUT,
	MX257_P2_11_OUTPUT,
	MX257_P2_8_OUTPUT,
	MX257_P2_9_OUTPUT,
	MX257_P2_6_OUTPUT,
	MX257_P2_7_OUTPUT,

	MX257_P2_21_INPUT,
	MX257_P3_15_INPUT,
	MX257_P2_10_INPUT,
	MX257_P2_11_INPUT,
	MX257_P2_8_INPUT,
	MX257_P2_9_INPUT,
	MX257_P2_6_INPUT,
	MX257_P2_7_INPUT,

#ifdef HYPTEK_DRIVER_CONFIG
	MX257_P2_21_GVALUE,
	MX257_P3_16_GVALUE,

	MX257_P1_6_SVALUE,
	MX257_P1_7_SVALUE,
	MX257_P1_29_SVALUE,
	MX257_P1_30_SVALUE,
	MX257_P1_31_SVALUE,
	MX257_P2_21_SVALUE,
	MX257_P3_15_SVALUE,
	MX257_P4_21_SVALUE,

#endif
};



#define MAX_RGE_SV_INTERVAL 3600
#define MIN_RGE_SV_INTERVAL 10
//static int g_ModBusTaskID = 0;

DPA_Mode_t CurrentDPAMode ;
UINT16 timer = 0;
DEVICE_IP_PARAM *pDEVICE_IP_PARAM = NULL;

extern pthread_mutex_t MBS_mutexs[8];

void Test_Func()
{
	ENN_U8 tmp[8];
	int i = 0;
	ENN_U32 len;
	ENN_U16 u16CRC;
	FILE *fp;
	static int count = 0;

#if 1	
	fp = fopen("/home/1.txt", "rw");
	if(fp == NULL)
	{
		perror("Fail to open");
		exit(1);
	}
	printf("\n%s, %d, fd = %d\n",__FUNCTION__,__LINE__,fp);


	while(1)
	{
		printf("\ncount = %d\n",count);
		count++;
		printf("\n%s, %d\n",__FUNCTION__,__LINE__);
		memset(tmp, 0, 8);
		tmp[0] = 0x05;
		tmp[1] = 0x03;
		tmp[2] = 0x00;
		tmp[3] = 0x00;
		tmp[4] = 0x00;
		tmp[5] = 0x04;
		u16CRC = CRC16(tmp, 6);
		printf("u16CRC = 0x%x\n",u16CRC);
		tmp[7] = (ENN_U8)(u16CRC >> 8);
		tmp[6] = (ENN_U8)(u16CRC & 0x00FF);
		for(i = 0; i < 8; i++) 
		{
			//tmp[i] = i%0xFF;
			printf("%2.2x, ",tmp[i]);
		}
		printf("\n");
		UART_Write(CHANNEL1, tmp, 8);
		printf("\n%s, %d\n",__FUNCTION__,__LINE__);
		memset(tmp, 0, 8);
	#if 1
		len = UART_Read(CHANNEL4, tmp, 13);
		printf("\n%s, %d\n",__FUNCTION__,__LINE__);
		for(i = 0; i < len; i++)
			printf(" %2.2x", tmp[i]);
		printf("\n");
		fwrite(tmp, len, 1, fp);
	#endif
		ENNOS_DelayTask(100);
	}
#else
	while(1)
	{
		printf("%s, %d\n",__FUNCTION__,__LINE__);
		ENNOS_DelayTask(100);
	}
#endif
}

ENN_ErrorCode_t InitTest_Task()
{
	ENNOS_TASK_t taskID;
	ENN_ErrorCode_t returnCode = ENN_SUCCESS;

	returnCode = ENNOS_CreateTask("TEST", ENNOS_TASK_PRIORITY_MIDDLE, 4*1024,&taskID, ENNOS_TASK_START, (void*)Test_Rx, NULL);
	if(ENN_SUCCESS != returnCode)
	{
		printf("\nCreate TCP TEST task fail!\n");
		return returnCode;
	}

	return ENN_SUCCESS;
}

void File_Test()
{
	FILE *fpConfig = NULL;
	
	fpConfig = fopen("/home/345.txt", "w+");
	perror("open configure file: ");
	if(NULL == fpConfig)
	{
		return ENN_FAIL;
	}

	fputs("ÂØÑÂ≠òÂô®Âú∞ÂùÄ\tÈïøÂ∫¶\tREG_ADDR/SLAVE_ID/CHANNEL\tÂØÑÂ≠òÂô®ÂêçÂ≠ó\t\tÂØÑÂ≠òÂô®ÊèèËø∞\r\n", fpConfig);
	perror("puts configure file: ");
	//printf("%s, %d, u16Ret = %d\n",__FUNCTION__,__LINE__, u16Ret);
	if(0 != fclose(fpConfig))
	{
		perror("save configure file fail ");
		printf("%s, %d\n",__FUNCTION__,__LINE__);
		return ;
	}
	perror("save configure file: ");
	return ;
}

#if 0
int main()
{
	char *name = "ÂçïÂª∫Ë∂ÖÊµãËØï";
	unsigned char slave_name[20];
	
	printf("Content-type:text/html\n\n");

	printf("{\"namelen\":\"%d\",", strlen(name));
	printf("\"name\":\"%s\",", name);

	memset(slave_name, 0, 20);
	GB2312_To_UTF8(name, strlen(name), slave_name, 20);
	printf("\"slave_name\":\"%s\"}", slave_name);
	return 0;
}
#endif
#if 1

int TL16c754B_UART_Select(int s32Handle, int timeout)
{
	struct timeval timout;
	fd_set rset;
	int    ret = 0;

	if(timeout <= 0)
	{
		return -1;
	}

	FD_ZERO(&rset);
	FD_SET (s32Handle, &rset);
	
	timout.tv_sec  =  timeout / 1000;
	timout.tv_usec = (timeout % 1000) * 1000;

	ret = select(s32Handle + 1, &rset, NULL, NULL, &timout);
	printf("\n%s, %d, ret = %d\n",__FUNCTION__, __LINE__, ret);
	if(ret > 0)
	{
		return 0; /* timeout */
	}
	printf("\n%s, %d, ret = %d\n",__FUNCTION__, __LINE__, ret);
	
	return -1;

}

char TL16c754B_UART_ReadChar(int s32Handle)
{
	ENN_S32  ret;
	ENN_U8 buf[2];

	ret = read(s32Handle, buf, 1);

	if(ret == 1) return buf[0];
	if(ret == 0) return -2;
	return -1;
}


int TL16c754B_UART_ReadBlock(int s32Handle, ENN_U8 *buf, ENN_U32 len, ENN_U32 timeout)
{
	ENN_U32 c, retlen;
	ENN_U32 i = 0;

	retlen = 0;
	for(i=0; i<len; i++)
	{
		if(timeout >= 0)
		{
			if(TL16c754B_UART_Select(s32Handle, timeout) < 0) 
			{
				break; // timeout
			}
		}
		c = TL16c754B_UART_ReadChar(s32Handle);
		printf("%s, %d, c = 0x%X\n",__FUNCTION__,__LINE__,c);
		if(c < 0) return c;
		buf[i] = (ENN_U8) c;
		retlen = i+1;
	}
	if(retlen <= 0) return -1;
	return retlen;
}

void TL16c754B_UART_TEST(void)
{
	ENN_S32 s32Handle = 0;
	ENN_S32 ret = 0;
	//ENN_U8 tel[8] = {0x02,0x03,0x00,0x02,0x00,0x05,0x24,0x3A};
	ENN_U8 tel[8] = {0x05,0x03,0x00,0x00,0x00,0x0E,0xC5,0x8A};
	ENN_U8 test[3] = {0x55, 0x66, 0x77};
	int i = 0;
	ENN_U8 tel1[20] = {0};
	ENN_U8 aReadData[512];
	int len = 0;
	int s32Len = 0;
	ENN_U8 slave = 0;
	ENN_U8 function = 0;
	ENN_U8 byte_count = 0;
	//char *name = "/dev/ttymxc1";
	char *name = "/dev/ttyS3";
	
	//DEBUG_UART("%s, %d, %s\n\n",__FUNCTION__,__LINE__, name);
	//s32Handle = UART_Open(name, BAUDRATE_4800, 8, 1, 1);
	s32Handle = UART_Open(name, BAUDRATE_9600, 8, 1, PARITY_NONE);
	if(s32Handle < 0)
	{
		DEBUG_UART("%s, %d, %d\n",__FUNCTION__,__LINE__,s32Handle);
		return ;
	}
	DEBUG_UART("%s, %d, %d\n",__FUNCTION__,__LINE__,s32Handle);
	
	for(i=0; i<8; i++)
	{
		DEBUG_UART("%X ", tel[i]);
	}
	ret = write(s32Handle, tel, 8);
	//ret = write(s32Handle, test, 2);
	DEBUG_UART("\n%s, %d, ret = %d\n",__FUNCTION__, __LINE__, ret);
	//return ;

	ENNModBus_Sleep((4*1000)/48);

	ret = TL16c754B_UART_Select(s32Handle, 1000);
	DEBUG_UART("\n%s, %d, ret = %d\n",__FUNCTION__, __LINE__, ret);
	if(ret < 0)
	{
		DEBUG_UART("%s, %d, %d\n",__FUNCTION__,__LINE__,s32Handle);
        perror("select error");
		close(s32Handle);
		return ;
	}
	
	if(TL16c754B_UART_ReadBlock(s32Handle, aReadData, 2, 1000) <= 0)
	{
		DEBUG_UART("%s, %d, %d\n",__FUNCTION__,__LINE__,s32Handle);
        perror("select error");
		close(s32Handle);
		return ;
	}

	slave     = aReadData[len++];
	function  = aReadData[len++];
	DEBUG_UART("%s, %d, slave = 0x%X\n",__FUNCTION__,__LINE__,slave);
	DEBUG_UART("%s, %d, function = 0x%X\n",__FUNCTION__,__LINE__,function);
	
	ret = TL16c754B_UART_Select(s32Handle, 1000);
	if(ret < 0)
	{
		DEBUG_UART("%s, %d, ret = %d\n",__FUNCTION__,__LINE__,ret);
		close(s32Handle);
		return ;
	}
	if(TL16c754B_UART_ReadBlock(s32Handle, &aReadData[len], 1, 1000) <= 0)
	{
		close(s32Handle);
		return 0;
	}
	byte_count = aReadData[len++];
	DEBUG_UART("%s, %d, byte_count = 0x%X\n",__FUNCTION__,__LINE__,byte_count);

	ret = TL16c754B_UART_Select(s32Handle, 1000);
	if(ret < 0)
	{
		DEBUG_UART("%s, %d, ret = %d\n",__FUNCTION__,__LINE__,ret);
		close(s32Handle);
		return ;
	}

	s32Len = TL16c754B_UART_ReadBlock(s32Handle, &aReadData[len], byte_count+2, 2000);
	DEBUG_UART("%s, %d, s32Len = %d\n",__FUNCTION__,__LINE__,s32Len);
	if((s32Len <= 0) && (s32Len != (byte_count+2)))
	{
		DEBUG_UART("%s, %d, s32Len = %d\n",__FUNCTION__,__LINE__,s32Len);
		close(s32Handle);
		return ;
	}
	len += byte_count + 2;
	DEBUG_UART("len = %d\n", len);
	for(i=0; i<len; i++)
	{
		printf("0x%2.2X ",aReadData[i]);
	}
	DEBUG_UART("\nlen = %d\n", len);
	
	close(s32Handle);
	return ;
}

#define I2C_SLAVE	0x0703
#define I2C_TENBIT	0x0704

#define I2C_ADDR 	0xC0

int CAT9552_LED_TEST(void)
{
	int GiFd = 0;
	int uiRet = 0;
	unsigned char addr[3];
	unsigned char tx_buf[5];
	unsigned char rx_buf[5];
	
	addr[0] = 0;
	
#if 1
	int fd;
	fd = open("/dev/mx25_gpio", 0);
	if (fd < 0) 
	{
		perror("open /dev/mx25_gpio");
		return -1;
	}

	uiRet = ioctl(fd, 11, 1);
	printf("%s, %d, uiRet = %d\n",__FUNCTION__,__LINE__, uiRet);
	ENNOS_DelayTask(100);
	
	uiRet = ioctl(fd, 11, 0);
	printf("%s, %d, uiRet = %d\n",__FUNCTION__,__LINE__, uiRet);
	ENNOS_DelayTask(100);

	uiRet = ioctl(fd, 11, 1);
	printf("%s, %d, uiRet = %d\n",__FUNCTION__,__LINE__, uiRet);
	ENNOS_DelayTask(100);

	//return 0;
#endif

	GiFd = open("/dev/i2c-1", O_RDWR);	 
	if(GiFd < 0)
	{
		perror("open i2c error\n");
		return -1;
	}
	
  	uiRet = ioctl(GiFd, I2C_SLAVE, I2C_ADDR >> 1);  //ËÆæÁΩÆ‰ªéÊú∫Âú∞ÂùÄ
	if (uiRet < 0) 
	{
		printf("setenv address faile ret: %x \n", uiRet);
		return -1;
 	}
	
	uiRet = ioctl(GiFd, I2C_TENBIT, 0);
	if (uiRet < 0) 
	{
		printf("setenv length ret: %x \n", uiRet);
		return -1;
 	}
	
	/*addr[0] = 0x17;
	uiRet = write(GiFd, addr, 1);
	printf("%s, %d, uiRet = %d\n",__FUNCTION__,__LINE__, uiRet);
	printf("%s, %d, addr[0] = 0x%X\n",__FUNCTION__,__LINE__, addr[0]);
	rx_buf[0] = 0x66;
	uiRet = read(GiFd, rx_buf, 1);
	printf("%s, %d, uiRet = %d\n",__FUNCTION__,__LINE__, uiRet);

	printf("%s, %d, rx_buf[0] = 0x%X\n",__FUNCTION__,__LINE__, rx_buf[0]);
	close(GiFd);
	return 0;*/
	
	addr[0] = 0x12;
	addr[1] = 0x2B;
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);

	addr[0] = 0x13;
	addr[1] = 0x80;
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);
	
	addr[0] = 0x14;
	addr[1] = 0x0A;
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);

	addr[0] = 0x15;
	addr[1] = 0xC0;
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);

	addr[0] = 0x16;
	addr[1] = 0x40;
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);

	addr[0] = 0x17;
	addr[1] = 0xAA;
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);

	addr[0] = 0x18;
	addr[1] = 0xAA;
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);

	addr[0] = 0x19;
	addr[1] = 0xAA;
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);

	return 0;
}

/*************************************************************************
*  √˚◊÷:  ENN_Get_DevInfo
*  Àµ√˜:  ¥”≈‰÷√Œƒº˛÷–∂¡»°…Ë±∏–≈œ¢
*   ‰»Î≤Œ ˝£∫void
*         
*           
*  ∑µªÿ÷µ: ENN_SUCCESS£∫¥¶¿Ì≥…π¶
*         ENN_FAIL£∫¥¶¿Ì ß∞‹
 *************************************************************************/

ENN_ErrorCode_t ENN_Get_DevInfo()
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
	int modnum = -1, r = -1;

	pDEVICE_IP_PARAM = (DEVICE_IP_PARAM *)malloc(sizeof(DEVICE_IP_PARAM));
	if(NULL == pDEVICE_IP_PARAM)
	{
		printf("ERROR : [%s], %d, malloc fail \n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	memset((void *)pDEVICE_IP_PARAM, 0, sizeof(DEVICE_IP_PARAM) );

	fpConfig = fopen("/home/MAIN.ini","r");
	//perror("open configure file: ");
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

		printf("[%s], %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aBuffer),aBuffer);
		cptr = strchr(aBuffer,0x0d);                 //\r
		if(cptr != NULL) 
		{
			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aBuffer),aBuffer);
			*cptr = '\0';
		}
		cptr = strchr(aBuffer,0x0a);               //\n
		if(cptr != NULL) 
		{
			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aBuffer),aBuffer);
			*cptr = '\0';
		}

		pStr1 = aBuffer; 
		ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pStr1),pStr1);
	        while((' ' == *pStr1) || ('\t' == *pStr1))//¿˚”√—≠ª∑Ã¯π˝ø’∞◊ƒ⁄»›
	    	{
	            pStr1++; 
	    	}

		if((0x0d == *pStr1) ||(0x0a == *pStr1))//≈–∂œ «∑ÒµΩ––Œ≤£¨»Áπ˚’‚∏ˆµΩ––Œ≤£¨Àµ√˜’‚ «“ª∏ˆ√ª”– ˝æ›µƒø’––£¨÷ÿ–¬∂¡»°œ¬“ª––µƒ ˝æ›
		{
			continue;
		}
		// »Áπ˚≤ª «ø’––
	        if('[' == *pStr1)  
	        {  
			pStr1++;  
			while((' ' == *pStr1) || ('\t' == *pStr1))
			{
				pStr1++;
			}

			pStr2 = pStr1;  //÷∏œÚµ⁄“ª∏ˆ◊÷ƒ∏µƒŒª÷√[MAIN]
			while((']' != *pStr1) && ('\0' != *pStr1))
			{
				pStr1++;
			}

			if('\0' == *pStr1)
			{
				//continue;
				r = fclose(fpConfig);
				fpConfig = NULL;
				printf("%s, %d, fclose(/home/MAIN.ini) ret = %d\n",__FUNCTION__,__LINE__,r);
				return ENN_FAIL;
			}

			while(' ' == *(pStr1-1))
			{
				pStr1--;
			}
			*pStr1 = '\0';  

			memset(aSection, 0, IEC102_READ_FILE_BUF_LEN);
			strcpy(aSection, pStr2);
			INFO("%s, %d, aSection = %s\n",__FUNCTION__,__LINE__, aSection);
			//ENNModBus_Set_Text(aSection, NULL, NULL);
		        
		}
		else if(('#' != *pStr1) && (*pStr1 > ' ')) // name identifier
		{
			ENNTRACE("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(pStr1),pStr1);
			memset(aName, 0, IEC102_READ_FILE_BUF_LEN);
			ret = ENNCopy_Name(aName, pStr1);
			if(ENN_SUCCESS != ret)
			{
				perror("get name fail ");
				r = fclose(fpConfig);
				fpConfig = NULL;
				printf("%s, %d, fclose(/home/MAIN.ini) ret = %d\n",__FUNCTION__,__LINE__,r); 
				return ENN_FAIL;
			}
			
			INFO("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aName),aName);
			memset(aParam, 0, IEC102_READ_FILE_BUF_LEN);
			ret = ENNCopy_Param(aParam, pStr1);
			if(ENN_SUCCESS != ret)
			{
				perror("get param fail ");
				r = fclose(fpConfig);
				fpConfig = NULL;
				printf("%s, %d, fclose(/home/MAIN.ini) ret = %d\n",__FUNCTION__,__LINE__,r);
				return ENN_FAIL;
			}
			INFO("%s, %d, %d, %s\n",__FUNCTION__,__LINE__,strlen(aParam),aParam);
			if((strcmp(aName, "SLAVE_IP") == 0)&&(strlen(aParam) <= 15))
			{	
				memcpy(pDEVICE_IP_PARAM->ip, aParam, strlen(aParam));
			}else if((strcmp(aName, "SLAVE_MASK") == 0)&&(strlen(aParam) <= 15))
			{
				memcpy(pDEVICE_IP_PARAM->mask, aParam, strlen(aParam));
			}else if((strcmp(aName, "SLAVE_GATEWAY") == 0)&&(strlen(aParam) <= 15))
			{
				memcpy(pDEVICE_IP_PARAM->gateway, aParam, strlen(aParam));
			}else if(strcmp(aName, "SLAVE_TCPPORT") == 0){
				if(strlen(aParam) > 0)
				{
					pDEVICE_IP_PARAM->serverport = atoi(aParam);
				}
			}
			ENNTRACE("%s, %d, %s, %d, %s, %d, %s\n",__FUNCTION__,__LINE__,aSection,strlen(aName),aName,strlen(aParam),aParam);
			//ENNModBus_Set_Text(aSection, aName, aParam);
			if(strcmp(aSection, "MAIN") == 0)
			{
				if(strlen(aParam) > 0)
				{
					modnum = atoi(aParam);
					if(modnum == ENN_MODBUS_MODE)
						CurrentDPAMode = ENN_MODBUS_MODE;
					else if(modnum == ENN_IEC102_MODE)
						CurrentDPAMode = ENN_IEC102_MODE;
					else
						perror("Get CurrentDPAMode fail ");
				}
			}
		}
	}
	r = fclose(fpConfig);
	fpConfig = NULL;
	//printf("%s, %d, fclose(/home/MAIN.ini) ret = %d\n",__FUNCTION__,__LINE__,r);
	
	//test_int();

	return ENN_SUCCESS;
}

/*************************************************************************
*  √˚◊÷:  ENN_Set_DevIP
*  Àµ√˜:  …Ë÷√…Ë±∏IP £¨Õ¯πÿµ»
*   ‰»Î≤Œ ˝£∫void
*         
*           
*  ∑µªÿ÷µ: ENN_SUCCESS£∫¥¶¿Ì≥…π¶
*         ENN_FAIL£∫¥¶¿Ì ß∞‹
 *************************************************************************/
ENN_ErrorCode_t ENN_Set_DevIP()
{
	ENN_ErrorCode_t returnCode;
	
	if(pDEVICE_IP_PARAM == NULL )
	{
		printf("ERROR : [%s], %d, pDEVICE_IP_PARAM == NULL\n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}
	
	returnCode = ENNSock_EthernetIPAddressSet(pDEVICE_IP_PARAM->ip);
	returnCode = ENNSock_EthernetSubNetmaskSet(pDEVICE_IP_PARAM->mask);
	returnCode = ENNSock_EthernetGatewaySet(pDEVICE_IP_PARAM->gateway);
	
	return returnCode;
}

int ENN_Reset_Check()
{
	int ret = -1;
	char DefaultIP[] = "192.168.12.205";
	struct timeval ctv, ftv;

	int fd, value = -1;

/*Start***** reset button check *****/
	fd = open("/dev/mx25_gpio", 0);
	if (fd < 0) 
	{
		perror("open /dev/mx25_gpio");
		printf("ERROR :%s, %d, %d\n",__FUNCTION__,__LINE__,fd);
		return -1;
	}
	gettimeofday(&ctv, NULL);
	gettimeofday(&ftv, NULL);
	while((ctv.tv_sec - ftv.tv_sec) <= 3)	//3s
	{
		ret= ioctl(fd, MX257_P3_16_GVALUE, &value);
		if(ret < 0){
			printf("Get MX257_P2_21_GVALUE,ioctl err !");
			return ret;
		}
		//printf("%s, %d, value =%d\n",__FUNCTION__,__LINE__,value);
		if(value > 0)
			break;

		ENNOS_DelayTask(200);	//200ms
		gettimeofday(&ctv, NULL);
	}
	if((ctv.tv_sec - ftv.tv_sec) >= 3)	//3s
	{
		//
		char cmd[50];
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "ifconfig eth0 %s", DefaultIP);
		ret = system(cmd);
		//printf("Test*******ret = %d\n",ret);
		if( ret < 0) 
		{
			perror("Reset default IP: ");
			close(fd);
			return ret;
		}
		printf("[%s], %d, Reset-BTN long clicked, board will reset to default\n",__FUNCTION__,__LINE__);
		close(fd);
		return 1;
	}
	close(fd);
/*End***** reset button check ******/
	return 0;

}

void testminiboardLED()	
{
	int fd;

	fd = open("/dev/mx25_gpio", 0);
	if (fd < 0) {
		perror("open /dev/mx25_gpio");
	}

	while ( 1 ) {
		ioctl(fd, MX257_P4_21_SVALUE, 0);
        	ioctl(fd, MX257_P1_7_SVALUE, 0);
		sleep(1);	
		ioctl(fd, MX257_P4_21_SVALUE, 1);
        	ioctl(fd, MX257_P1_7_SVALUE, 1);
		sleep(1);
			
        	ioctl(fd, MX257_P1_6_SVALUE, 0);
        	ioctl(fd, MX257_P1_31_SVALUE, 0);
		sleep(1);
		ioctl(fd, MX257_P1_6_SVALUE, 1);
        	ioctl(fd, MX257_P1_31_SVALUE, 1);
		sleep(1);
			
        	ioctl(fd, MX257_P1_30_SVALUE,  0);
        	ioctl(fd, MX257_P1_29_SVALUE, 0);
		sleep(1);
		ioctl(fd, MX257_P1_30_SVALUE,  1);
		ioctl(fd, MX257_P1_29_SVALUE,  1);
		sleep(1);
			
        	ioctl(fd, MX257_P3_15_SVALUE,  0);
        	ioctl(fd, MX257_P2_21_SVALUE,  0);
		sleep(1);
		ioctl(fd, MX257_P3_15_SVALUE,  1);
        	ioctl(fd, MX257_P2_21_SVALUE,  1);
		sleep(1);
	}
}

int main()
{
	//static unsigned int i = 100;
	ENN_ErrorCode_t returnCode;

	struct timeval ctv, ftv;
	time_t timep;
	struct tm *p;
	char date[10];
	int resflag = -1;

#if 0
	int fd;
	fd = open("/dev/mx25_gpio", 0);
	if (fd < 0) 
	{
		perror("open /dev/mx25_gpio");
		return -1;
	}

	ioctl(fd, 10, 0);
	ENNOS_DelayTask(100);
	
	ioctl(fd, 10, 1);
	ENNOS_DelayTask(100);

	ioctl(fd, 10, 0);
	ENNOS_DelayTask(100);

	return 0;
#endif

	DBG_LEVEL= DBG_E;

	//resflag = ENN_Reset_Check();

	returnCode = ENNOS_Init();
	
	ENNLED_Init();
	//testminiboardLED();

#if 1
	returnCode = ENNModBus_History_Init();
	
	//printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
	//returnCode = InitTest_Task();

	returnCode = ENN_Get_DevInfo();
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return -1;
	}
	
	//if(resflag != 1)
	{
		returnCode = ENN_Set_DevIP();
		if(ENN_SUCCESS != returnCode)
		{
			printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
			//return -1;
		}
	}
	
	if(CurrentDPAMode == ENN_IEC102_MODE)
	{
		returnCode = ENNIEC102_init();
		if(ENN_SUCCESS != returnCode)
		{
			printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
			return -1;
		}
	}
	else if(CurrentDPAMode == ENN_MODBUS_MODE)
	{
		returnCode = ENNModBus_DPA_readConfig();
		if(ENN_SUCCESS != returnCode)
		{
			printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
			return -1;
		}
	}
	
	returnCode = ENNModBus_Read_Configure();
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return -1;
	}

	returnCode = ENNModBus_Slave_Init();
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return -1;
	}
	
	returnCode = ENNModBus_Build_File();
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return -1;
	}

	//returnCode = UART_Init(CHANNEL1, 4800, 8, 1, PARITY_ODD);
	//returnCode = ENNModBus_Create_Channel_Task();
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return -1;
	}

	//testminiboardLED();
	
	returnCode = InitENNModBus_Task();
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return -1;
	}

	returnCode = initENNBacMstp_Task();
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return -1;
	}
	
	//returnCode = initENNBacNet_Task();
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return -1;
	}

	//returnCode =  ENNAmmeter_Server_Tcp(void);
#endif

	returnCode = ENNModBus_Create_PIPE_Task();
	printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
	if(ENN_SUCCESS != returnCode)
	{
		printf("%s, %d, %d\n",__FUNCTION__,__LINE__,returnCode);
		return -1;
	}

	if(CurrentDPAMode == ENN_IEC102_MODE)
	{
		ENNIEC102_Data_init();
				
		Enn_Database_init();
		//ENNOS_DelayTask(2000);
		//ENNIEC102_Data_Map();
		//Database_select("00:28:00", "00:28:40", 25);	//test
	}
	else
	{
		printf("%s,%d\n",__FUNCTION__,__LINE__);
		ENNIEC102_Data_init();
		printf("%s,%d\n",__FUNCTION__,__LINE__);		
		Enn_Database_init();
	}

	//Enn_DeleteOldTable();
	
	while(1)
	{
		if(CurrentDPAMode == ENN_IEC102_MODE){
			gettimeofday(&ctv, NULL);
			gettimeofday(&ftv, NULL);
			while(1)
			{
				if(((ctv.tv_sec - ftv.tv_sec) >= 1) /*&& (ctv.tv_sec%MIN_RGE_SV_INTERVAL == 0)*/)
				{
					time(&timep);
					p = localtime(&timep); //»°µ√µ±µÿ ±º‰
					memset(date, 0, sizeof(date));
					//printf("%d/%02d/%02d ", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday);
					sprintf(date, "%02d:%02d:%02d", p->tm_hour, p->tm_min, p->tm_sec);
		
					Database_insert(date);
					
					ftv.tv_sec = ctv.tv_sec;

					timer ++;
					if(timer > MAX_RGE_SV_INTERVAL)
						timer = 1;	//≤ªø…µ»”⁄0£¨0∂‘»Œ“‚ ˝»°”‡∂º «0
				}
				ENNOS_DelayTask(100);	//100ms
				gettimeofday(&ctv, NULL);
			}
		}else{
			//ENNOS_DelayTask(10000);
			//printf("%s, %d, Modbus\n",__FUNCTION__,__LINE__);
			gettimeofday(&ctv, NULL);
			gettimeofday(&ftv, NULL);
			while(1)
			{
				if(((ctv.tv_sec - ftv.tv_sec) >= 1) /*&& (ctv.tv_sec%MIN_RGE_SV_INTERVAL == 0)*/)
				{
					time(&timep);
					p = localtime(&timep); //»°µ√µ±µÿ ±º‰
					memset(date, 0, sizeof(date));
					//printf("%d/%02d/%02d ", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday);
					sprintf(date, "%02d:%02d:%02d", p->tm_hour, p->tm_min, p->tm_sec);
					//printf("wlc%02d:%02d:%02d   \n", p->tm_hour, p->tm_min, p->tm_sec);
					Database_insert(date);
					
					ftv.tv_sec = ctv.tv_sec;

					timer ++;
					if(timer > MAX_RGE_SV_INTERVAL)
						timer = 1;	//≤ªø…µ»”⁄0£¨0∂‘»Œ“‚ ˝»°”‡∂º «0
				}
				ENNOS_DelayTask(100);	//100ms
				gettimeofday(&ctv, NULL);
		          }

	}

		return 0;
	}
}
#endif
  

