#include  "test_645.h"
#include  <string.h>
#include <stdlib.h>
#include <stdio.h>

//#define DEBUG
//#define DEBUG_MSG
const char ENERGY_A_POS_DI_2007[]  = {0x00,0x01,0x00,0x00};  //正向有功总电量
const char ENERGY_A_NEG_DI_2007[] = {0x00,0x02,0x00,0x00}; 
const char  ENERGY_R_POS_DI_2007[] = {0x00,0x03,0x00,0x00}; 
const char  ENERGY_R_NEG_DI_2007[] = {0x00,0x04,0x00,0x00}; 
const char  POWER_U_A_DI_2007[] = {0x02,0x01,0x01,0x00}; 
const char  POWER_U_B_DI_2007[] = {0x02,0x01,0x02,0x00}; 
const char  POWER_U_C_DI_2007[] = {0x02,0x01,0x03,0x00}; 
const char  POWER_I_A_DI_2007[] = {0x02,0x02,0x01,0x00}; 
const char  POWER_I_B_DI_2007[] = {0x02,0x02,0x02,0x00}; 
const char  POWER_I_C_DI_2007[] = {0x02,0x02,0x03,0x00}; 
const char  POWER_P_DI_2007[] = {0x02,0x03,0x00,0x00}; 
const char  POWER_Q_DI_2007[] = {0x02,0x04,0x00,0x00}; 
const char  POWER_FACTOR_DI_2007[]={0x02,0x06,0x00,0x00}; 
const char  POWER_FEQ_DI_2007[]={0x02,0x80,0x02,0x00}; 

static int tim_read_645_2007(int fd,int timeout,unsigned char * rcv_buf);
static int tim_packet_check_645_2007(unsigned char * rcv_buf,unsigned char * meter_add,unsigned char * di,int rcv_num);
static float tim_get_e_data(unsigned char* data);
static float tim_get_u_data(unsigned char* data);
static float tim_get_i_data(unsigned char * data);
static float tim_get_p_data(unsigned char * data);
static float tim_get_f_data(unsigned char* data);
static float tim_get_frq_data(unsigned char* data);

int tim_register_data_2007(T_REG_DATA * data)  
{  
       unsigned char snd_buf[100] = {0xfe,0xfe,0xfe,0x68,0x00,0x00,0x00,0x00,0x00,0x00,0x68,0x11,0x04,0x00,0x00,0x00,0x00,0x00,0x16};
	unsigned char rcv_buf[100] = "";
       unsigned char meter_add[12] = "";
	unsigned char meter_add_temp[12] = "";
	unsigned char temp_add[2] = "";
	unsigned char temp_di[4] = "";
	char info[100] = "";
	int i,j,di_code;
	ssize_t ret,ck_ret;  

	memcpy(meter_add_temp,data->address,strlen(data->address));
	if(strlen(meter_add_temp) < 12)
	{
		int len = 12 - strlen(meter_add_temp);
		for(i = 0; i < len ; i++)
		{
			meter_add[i] = '0';
		}
		memcpy(meter_add + len,meter_add_temp,len);
	}
	else
	{
		memcpy(meter_add,meter_add_temp,12);
	}

	for(i = 0 ; i < 6; i++)
	{
		if(( (meter_add[10 - i * 2] < 0x30) || (meter_add[10 - i * 2] > 0x39))
			||((meter_add[11 - i * 2] < 0x30) || (meter_add[11 - i * 2] > 0x39)))
		{
			return 0;
		}
		else
		{
			snd_buf[i + 4] = (meter_add[10 - i * 2] - 0x30) * 0x10 +  (meter_add[11 - i * 2] - 0x30);
		}
	}
	
	for(i = 0 ; i < data->channel_num ; i++)
	{
		switch(data->pchannel_code[i])
		{
			case ENERGY_A_POS_T:
				memcpy(temp_di, ENERGY_A_POS_DI_2007,4);
				di_code = 1;
				break;
			case ENERGY_A_NEG_T:
				memcpy(temp_di, ENERGY_A_NEG_DI_2007,4);
				di_code = 2;
				break;
			case ENERGY_R_POS_T:
				memcpy(temp_di, ENERGY_R_POS_DI_2007,4);
				di_code = 3;
				break;
			case ENERGY_R_NEG_T:
				memcpy(temp_di, ENERGY_R_NEG_DI_2007,4);
				di_code = 4;
				break;
			case POWER_U_A:
				memcpy(temp_di, POWER_U_A_DI_2007,4);
				di_code = 5;
				break;
			case POWER_U_B:
				memcpy(temp_di, POWER_U_B_DI_2007,4);
				di_code = 6;
				break;
			case POWER_U_C:
				memcpy(temp_di, POWER_U_C_DI_2007,4);
				di_code = 7;
				break;
			case POWER_I_A:
				memcpy(temp_di, POWER_I_A_DI_2007,4);
				di_code = 8;
				break;
			case POWER_I_B:
				memcpy(temp_di, POWER_I_B_DI_2007,4);
				di_code = 9;
				break;
			case POWER_I_C:
				memcpy(temp_di, POWER_I_C_DI_2007,4);
				di_code = 10;
				break;
			case POWER_P_T:
				memcpy(temp_di, POWER_P_DI_2007,4);
				di_code = 11;
				break;
			case POWER_Q_T:
				memcpy(temp_di, POWER_Q_DI_2007,4);
				di_code = 12;
				break;
			case POWER_FACTOR:
				memcpy(temp_di, POWER_FACTOR_DI_2007,4);
				di_code = 13;
				break;
			case POWER_FEQ:
				memcpy(temp_di, POWER_FEQ_DI_2007,4);
				di_code = 14;
				break;
			default:
				di_code = 0;
				break;
		}
		if(di_code == 0)
		{
			return -1;
		}
		for(j = 0 ; j < 4; j ++)
		{
			snd_buf[13 + j] = temp_di[3 - j] + 0x33;
		}
		snd_buf[17] = 0;
		for(j = 3 ; j < 17; j++)
		{
			snd_buf[17] += snd_buf[j];
		}
#ifdef DEBUG
		printf("***Meter:%s begin to read:",meter_add);
		switch(di_code)
		{
			case 0:
				printf("DI Error\n");
				break;
			case 1:
				printf("ENERGY_A_POS\n");
				break;
			case 2:
				printf("ENERGY_A_NEG\n");
				break;
			case 3:
				printf("ENERGY_R_POS\n");
				break;
			case 4:
				printf("ENERGY_R_NEG\n");
				break;
			case 5:
				printf("POWER_U_A\n");
				break;
			case 6:
				printf("POWER_U_B\n");
				break;
			case 7:
				printf("POWER_U_C\n");
				break;
			case 8:
				printf("POWER_I_A\n");
				break;
			case 9:
				printf("POWER_I_B\n");
				break;
			case 10:
				printf("POWER_I_C\n");
				break;
			case 11:
				printf("POWER_P_T\n");
				break;
			case 12:
				printf("POWER_Q_T\n");
				break;
			case 13:
				printf("POWER_FACTOR\n");
				break;
			case 14:
				printf("POWER_FEQ\n");
				break;
		}
		//printf("===Meter:%s begin to read:%02X %02X %02X %02X\n",meter_add,temp_di[0],temp_di[1],temp_di[2],temp_di[3]);
#endif

#if 0
		printf("->Send:");
		for(j = 0; j < 19; j++)
		{
			printf("%02x",(unsigned char)snd_buf[j]);
			printf(" ");
		}
		printf("\n");
#endif
		
		ret = write(data->fd, snd_buf, 19);
		//+++++start ,edit by hyptek 20150907
		//ENNOS_DelayTask(100);
		//+++++end
		ret = tim_read_645_2007(data->fd,2000,rcv_buf);
		if(ret > 0)
		{

#ifdef DEBUG_MSG
			printf("<-Received:");
			for(j = 0; j < ret; j++)
			{
				printf("%02x",(unsigned char)rcv_buf[j]);
				printf(" ");
			}
			printf("\n");
#endif
			//printf("check result:%d\n",tim_packet_check_645_2007(rcv_buf,snd_buf + 4, snd_buf + 13,  ret));
			ck_ret = tim_packet_check_645_2007(rcv_buf,snd_buf + 4,snd_buf + 13,  ret);
			if(ck_ret == 0 )
			{
				switch(di_code)
				{
					case 1:
					case 2:
					case 3:
					case 4:
						if(rcv_buf[9] <  8)
						{
							data->pchannel_data[i].status = 1;
						}
						else
						{
							data->pchannel_data[i].value = tim_get_e_data(rcv_buf + 14);
							data->pchannel_data[i].status = 1;
						}
						break;
					case 5:
					case 6:
					case 7:
						if(rcv_buf[9] <  6)
						{
							data->pchannel_data[i].status = 1;
						}
						else
						{
							data->pchannel_data[i].value = tim_get_u_data(rcv_buf + 14);
							data->pchannel_data[i].status = 1;
						}
						break;
					case 8:
					case 9:
					case 10:
						if(rcv_buf[9] <  7)
						{
							data->pchannel_data[i].status = 1;
						}
						else
						{
							data->pchannel_data[i].value = tim_get_i_data(rcv_buf + 14);
							data->pchannel_data[i].status = 1;
						}
						break;
					case 11:
					case 12:
						if(rcv_buf[9] <  7)
						{
							data->pchannel_data[i].status = 1;
						}
						else
						{
							data->pchannel_data[i].value = tim_get_p_data(rcv_buf + 14);
							data->pchannel_data[i].status = 1;
						}
						break;
					case 13:
						if(rcv_buf[9] <  6)
						{
							data->pchannel_data[i].status = 1;
						}
						else
						{
							data->pchannel_data[i].value = tim_get_f_data(rcv_buf + 14);
							data->pchannel_data[i].status = 1;
						}
						break;
					case 14:
						if(rcv_buf[9] <  6)
						{
							data->pchannel_data[i].status = 1;
						}
						else
						{
							data->pchannel_data[i].value = tim_get_frq_data(rcv_buf + 14);
							data->pchannel_data[i].status = 1;
						}
						break;
					default:
						data->pchannel_data[i].status = 0;
						break;
				}
#ifdef DEBUG
				printf("===Read Success, value = %f\n",data->pchannel_data[i].value);
#endif
			}
			else
			{
				data->pchannel_data[i].status = 0;
#ifdef DEBUG
				printf("===Read Fail, Reason = %d\n",(int)ck_ret);
#endif
			}
		}
		else
		{
			data->pchannel_data[i].status = 0;
#ifdef DEBUG
				printf("===Read Fail, Reson = Not Receive Correct Data!\n");
#endif
		}
		usleep(10000);
	}
	return ret;
}

static float tim_get_e_data(unsigned char* data)
{
	float val = 0.0;
	
	val = (float) ( data[3]  / 0x10 * 10 + data[3] % 0x10) * 10000 +
		 (float) ( data[2]  / 0x10 * 10 + data[2] % 0x10) * 100 +
		 (float) ( data[1]  / 0x10 * 10 + data[1] % 0x10)  +
		 (float) ( data[0]  / 0x10 * 10 + data[0] % 0x10) / 100;
	return val;
}

static float tim_get_u_data(unsigned char* data)
{
	float val = 0.0;
	
	val =( (float) ( data[1]  / 0x10 * 10 + data[1] % 0x10)  * 100 +
		 (float) ( data[0]  / 0x10 * 10 + data[0] % 0x10)) / 10.0;
	return val;
}

static float tim_get_i_data(unsigned char* data)
{
	float val = 0.0;
	
	val =( (float) ( data[2]  / 0x10 * 10 + data[2] % 0x10) * 10000 +
		   (float) ( data[1]  / 0x10 * 10 + data[1] % 0x10) * 100 +
		   (float) ( data[0]  / 0x10 * 10 + data[0] % 0x10) ) / 1000.0;
	return val;
}

static float tim_get_p_data(unsigned char* data)
{
	float val = 0.0;
	
	val =( (float) ( data[2]  / 0x10 * 10 + data[2] % 0x10) * 10000 +
		   (float) ( data[1]  / 0x10 * 10 + data[1] % 0x10) * 100 +
		   (float) ( data[0]  / 0x10 * 10 + data[0] % 0x10) ) / 10000.0;
	return val;
}

static float tim_get_f_data(unsigned char* data)
{
	float val = 0.0;
	
	val =(  (float) ( data[1]  / 0x10 * 10 + data[1] % 0x10) * 100 +
		   (float) ( data[0]  / 0x10 * 10 + data[0] % 0x10) ) / 1000.0;
	return val;
}

static float tim_get_frq_data(unsigned char* data)
{
	float val = 0.0;
	
	val =(  (float) ( data[1]  / 0x10 * 10 + data[1] % 0x10) * 100 +
		   (float) ( data[0]  / 0x10 * 10 + data[0] % 0x10) ) / 100.0;
	return val;
}

static int tim_packet_check_645_2007(unsigned char * rcv_buf,unsigned char * meter_add,unsigned char * di,int rcv_num)
{
      unsigned char sum = 0,i;
	  
	if((rcv_buf[0] != 0x68) &&( rcv_buf[7] != 0x68))
	{
		return -1;
	}
       else
       if(rcv_buf[8] != 0x91)
	{
		return -2;
	}
	else
	if(memcmp(meter_add,rcv_buf +  1, 6) != 0)
		/*(meter_add[0] != rcv_buf[6]) || (meter_add[1] != rcv_buf[5])  || (meter_add[2] != rcv_buf[4]) 
		||(meter_add[3] != rcv_buf[3]) || (meter_add[4] != rcv_buf[2]) || (meter_add[5] != rcv_buf[1]) )*/
	{
		return -3;
	}
	else
       if(rcv_buf[9]  > 10)
	{
		return -4;
	}
      	else
       if(rcv_buf[9]  < 4)
	{
		return -5;
	}
	else
      	if((di[0] != rcv_buf[10]) || (di[1] != rcv_buf[11])  || (di[2] != rcv_buf[12]) ||(di[3] != rcv_buf[13]))
	{
		return -6;
	}   
	else
	{
		for(i = 0 ; i < rcv_buf[9]  + 10; i++)
		{
			sum += rcv_buf[i];
		}
		
		if(sum != rcv_buf[ rcv_buf[9]  + 10])
		{
			return -7;
		}
	}
	

	for(i = 10 ; i < 10 + rcv_buf[9] ; i++)
	{
		rcv_buf[i] -= 0x33;
	}	
	return 0;
	
}


static int tim_read_645_2007(int fd,int timeout,unsigned char * rcv_buf)
{
	int retval;  
	fd_set rfds;  
	struct timeval tv;  
	int ret, pos = 0,i;  
	unsigned char rcv_temp[100] = "";
	
	tv.tv_sec = timeout / 1000;  //set the rcv wait time  
	tv.tv_usec = timeout % 1000 * 1000;  //100000us = 0.1s  

	while (1)  
	{  
		FD_ZERO(&rfds); //每次循环都要清空集合，否则不能检测描述符变化  
		FD_SET(fd, &rfds);  //添加描述符  
        	retval = select(fd + 1, &rfds, NULL, NULL, &tv);    //fd+1:描述符最大值加1 返回负数：出错  0:timeout  >0:有文件可读写  
        	if (retval == -1)  
        	{  
            		break;  
        	}  
        	else if (retval)  
        	{  
            		ret = read(fd, rcv_temp + pos, 100); 
							
        		 if (-1 == ret)  
            		{  
                		break;  
            		}  
            		pos += ret;  
  
#ifdef DEBUG_2
			printf("tim_read_645_2007 --Received:");
			for(i = 0; i < pos; i++)
			{
				printf("%02x",(unsigned char)rcv_temp[i]);
				printf(" ");
			}
			printf("\n");


		}
#endif 

#ifndef DEBUG_2
           		for(i = 0 ; i < pos; i++)
			{
				if(((unsigned char)rcv_temp[i]) != 0xfe)
				{
					break;
				}
			}

			if(i < pos)
			{
				if((unsigned char)rcv_temp[i] == 0x68)
				{
					if( (i + 10) < pos)
					{
						if((i + ((unsigned char)rcv_temp[i + 9]) + 12 ) <=  pos)
						{
							pos = pos - i;
							memcpy(rcv_buf,rcv_temp + i , pos);
							break;
						}
					}
				}
				else
				{
					pos = 0;
					break;
				}
			}
					
        	}  
        	else  
        	{  
			pos = -1;
            		break;  
        	}  
#endif 
    	}  
  
	return pos;  
}




