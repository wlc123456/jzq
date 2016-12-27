#ifndef _TEST_645_H_
#define _TEST_645_H_


#include     <stdio.h>      /*标准输入输出定义*/  
#include     <stdlib.h>     /*标准函数库定义*/  
#include     <unistd.h>     /*Unix 标准函数定义*/  
#include     <sys/types.h>  
#include     <sys/stat.h>  
#include     <fcntl.h>      /*文件控制定义*/  
#include     <termios.h>    /*POSIX 终端控制定义*/  
#include     <errno.h>      /*错误号定义*/  
#include     <unistd.h>
#include     <strings.h>

typedef struct _T_CHANNEL_VALUE
{
  float  value;
  short status;
  short padding;
}T_CHANNEL_VALUE;


typedef struct _T_REG_DATA
{
  int fd;             //set by dca, port handle
  char address[16];
  char user[16];
  char password[16];
  short port_type;    //enum SERIAL/NET
  unsigned short channel_num;    //set by dca
  unsigned int *pchannel_code; //channel_code[MAX_CHANNEL], malloc and set by dca
  T_CHANNEL_VALUE *pchannel_data; //malloc by dca, tim set value
}T_REG_DATA;


typedef enum _T_DATA_TYPE
{
  ENERGY_A_POS_T=0x0101,   //正向有功总电量
  ENERGY_A_NEG_T=0x0102,   //反向有功总电量
  ENERGY_R_POS_T=0x0103,   //正向无功总电量
  ENERGY_R_NEG_T=0x0104,   //反向无功总电量
  POWER_U_A=0x0201,        //A相电压
  POWER_U_B=0x0202,        //B相电压
  POWER_U_C=0x0203,        //C相电压
  POWER_I_A=0x0204,        //A相电流
  POWER_I_B=0x0205,        //B相电流
  POWER_I_C=0x0206,        //C相电流
  POWER_P_T=0x0207,        //有功功率总
  POWER_Q_T=0x0208,        //无功功率总
  POWER_FACTOR=0x0209,     //功率因数
  POWER_FEQ=0x020A         //频率
}T_DATA_TYPE;

int tim_register_data_1997(T_REG_DATA * data);
int tim_register_data_2007(T_REG_DATA * data);


#endif 

