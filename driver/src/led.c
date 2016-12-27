/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：FIFO.c
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/	

#include "ennAPI.h"


#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */

#define I2C_SLAVE	0x0703
#define I2C_TENBIT	0x0704

#define I2C_ADDR 	0xC0

#define CHANNEL_COUNT 4

//#define LED_DEBUG printf
#define LED_DEBUG 

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


};



ENN_S32 gFd = -1;
ENN_U8  gRegAddr[8] = {0x16, 0x16, 0x17, 0x17, 0x18, 0x18, 0x19, 0x19};


LED_Status	eLED_Status[8] = {LED_NORMAL, LED_NORMAL, LED_NORMAL, LED_NORMAL, LED_NORMAL, LED_NORMAL, LED_NORMAL, LED_NORMAL};

ENN_S32 ENNLED_Init(ENN_VOID)
{
	int fd;
	int uiRet = 0;
	int GiFd = 0;
	unsigned char addr[3];
	int i;

	addr[0] = 0;
	
	GiFd = open("/dev/mx25_gpio", 0);
	if (GiFd < 0) 
	{
		perror("open /dev/mx25_gpio");
		return -1;
	}
/*
	uiRet = ioctl(fd, 11, 1);
	printf("%s, %d, uiRet = %d\n",__FUNCTION__,__LINE__, uiRet);
	ENNOS_DelayTask(100);
	
	uiRet = ioctl(fd, 11, 0);
	printf("%s, %d, uiRet = %d\n",__FUNCTION__,__LINE__, uiRet);
	ENNOS_DelayTask(100);

	uiRet = ioctl(fd, 11, 1);
	printf("%s, %d, uiRet = %d\n",__FUNCTION__,__LINE__, uiRet);
	ENNOS_DelayTask(100);*/
//miniboard LED
/*miniboard LED
	GiFd = open("/dev/i2c-1", O_RDWR);	 
	if(GiFd < 0)
	{
		perror("open i2c error\n");
		return -1;
	}
	
  	uiRet = ioctl(GiFd, I2C_SLAVE, I2C_ADDR >> 1);  //设置从机地址
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

	addr[0] = 0x12;		//PSC0 ADDR
	addr[1] = 0x2B;		//1HZ
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);

	addr[0] = 0x13;		//PWM0 ADDR
	addr[1] = 0x80;		//0.5  	duty cycle	
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);
	
	addr[0] = 0x14;		//PSC1 ADDR
	addr[1] = 0x0A;		//0.25HZ
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);

	addr[0] = 0x15;		//PWM1 ADDR
	addr[1] = 0xC0;		//0.25	duty cycle
	uiRet = write(GiFd, addr, 2);
	printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);
*/	
	gFd = GiFd;

	for(i=0; i< CHANNEL_COUNT; i++)
		ENNLED_On((CHANNEL_t)i, LED_OFF);

	return (ENN_S32)GiFd;
}

//ENN_S32 ENNLED_On(CHANNEL_t CHANNELX, LED_STATUS_t led_status)
ENN_S32 ENNLED_On(CHANNEL_t CHANNELX, LED_Status led_status)
{
	int ret = 0;
	unsigned char addr[3];
	int uiRet = 0;

	struct timeval ctv, ftv;

	if(gFd < 0)
	{
		printf("[error]   %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}

	if(led_status == eLED_Status[CHANNELX])
	{
		return 0;
	}
/*miniboard LED
	LED_DEBUG("%s, %d, CHANNELX = %d\n",__FUNCTION__,__LINE__,CHANNELX);
	addr[0] = gRegAddr[CHANNELX];
	LED_DEBUG("%s, %d, addr[0] = 0x%X    \n",__FUNCTION__,__LINE__, addr[0]);
	uiRet = write(gFd, addr, 1);
	//printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);
	uiRet = read(gFd, &addr[1], 1);
	//printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);
	LED_DEBUG("%s, %d, addr[1] = 0x%X    \n",__FUNCTION__,__LINE__, addr[1]);

	//printf("%s, %d, uiRet = %d    \n",__FUNCTION__,__LINE__, uiRet);
*/
	switch(CHANNELX)
	{
		case CHANNEL1:
			if(LED_NORMAL == led_status)	//0000 1001
			{
				ioctl(gFd, MX257_P4_21_SVALUE, 0);
				ioctl(gFd, MX257_P1_7_SVALUE, 1);
			}
			else if(LED_ERROR == led_status)
			{
				ioctl(gFd, MX257_P4_21_SVALUE, 1);
				ioctl(gFd, MX257_P1_7_SVALUE, 0);			
			}
			else	//LED_OFF
			{
				ioctl(gFd, MX257_P4_21_SVALUE, 1);
				ioctl(gFd, MX257_P1_7_SVALUE, 1);
			}
		break;
		case CHANNEL2:
			if(LED_NORMAL == led_status)	//0000 1001
			{
				ioctl(gFd, MX257_P1_6_SVALUE, 0);
				ioctl(gFd, MX257_P1_31_SVALUE, 1);
			}
			else if(LED_ERROR == led_status)
			{
				ioctl(gFd, MX257_P1_6_SVALUE, 1);
				ioctl(gFd, MX257_P1_31_SVALUE, 0);			
			}
			else	//LED_OFF
			{
				ioctl(gFd, MX257_P1_6_SVALUE, 1);
				ioctl(gFd, MX257_P1_31_SVALUE, 1);
			}
		break;
		case CHANNEL3:
			printf("TEST************ led_status=%d\n",led_status);
			if(LED_NORMAL == led_status)	//0000 1001
			{
				ioctl(gFd, MX257_P1_30_SVALUE, 0);
				ioctl(gFd, MX257_P1_29_SVALUE, 1);
			}
			else if(LED_ERROR == led_status)
			{
				ioctl(gFd, MX257_P1_30_SVALUE, 1);
				ioctl(gFd, MX257_P1_29_SVALUE, 0);			
			}
			else	//LED_OFF
			{
				ioctl(gFd, MX257_P1_30_SVALUE, 1);
				ioctl(gFd, MX257_P1_29_SVALUE, 1);
			}
		break;
		case CHANNEL4:
			if(LED_NORMAL == led_status)	//0000 1001
			{
				ioctl(gFd, MX257_P3_15_SVALUE, 0);
				ioctl(gFd, MX257_P2_21_SVALUE, 1);
			}
			else if(LED_ERROR == led_status)
			{
				ioctl(gFd, MX257_P3_15_SVALUE, 1);
				ioctl(gFd, MX257_P2_21_SVALUE, 0);			
			}
			else	//LED_OFF
			{
				ioctl(gFd, MX257_P3_15_SVALUE, 1);
				ioctl(gFd, MX257_P2_21_SVALUE, 1);
			}
		break;
		
		default:
			return -1;
	}	
	
	//LED_DEBUG("%s, %d, addr[0] = 0x%X    \n",__FUNCTION__,__LINE__, addr[0]);
	//LED_DEBUG("%s, %d, addr[1] = 0x%X    \n",__FUNCTION__,__LINE__, addr[1]);
	eLED_Status[CHANNELX] = led_status;
	//uiRet = write(gFd, addr, 2);
	LED_DEBUG("%s, %d, channel = %d    led_status =%d\n",__FUNCTION__,__LINE__, CHANNELX, led_status);

    return (ENN_S32)ret;
}

ENN_ErrorCode_t ENNLED_Close()
{
	int i =0;
	ENN_ErrorCode_t ret;
	
	if(gFd < 0)
	{
		printf("ERROR: %s, %d, gFd < 0 \n",__FUNCTION__,__LINE__);
		return ENN_FAIL;
	}

	for(i=0; i< CHANNEL_COUNT; i++)
		ENNLED_On((CHANNEL_t)i, LED_OFF);
	
	ret = close(gFd);
	gFd = -1;
	return ret;
}




#ifdef __cplusplus
#if __cplusplus
    }
#endif /* __cpluscplus */
#endif /* __cpluscplus */


