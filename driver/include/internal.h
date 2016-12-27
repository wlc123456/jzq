#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#include <ennAPI.h>

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K 1
#endif
#include <semaphore.h>
#include <sys/time.h>
#include <sys/sysinfo.h>


#define CYDEBUG(...) fprintf( stderr, __VA_ARGS__)

typedef struct ENNTask
{
    ENN_U8                 *name;            //线程名字
    ENNOS_TASK_Priority_t    prio;   //线程等级
    ENN_U32                 stack;             //线程堆栈的大小
    ENNOS_TASK_CreateMode_t  mode;   //线程任务的创建模式
    ENNOS_TASKENTRY_f        fuc;          //线程任务事件处理函数
    ENN_VOID               *param;                  //线程任务事件处理函数的参数
    pthread_t              pid;                              //当前线程的线程号
    ENN_U32                taskid;                      //线程的任务ID
}ENNTask;


#if 1
typedef struct ENNSemaphore {
    sem_t                  sem;
    ENNOS_SEMA_CreateMode_t  semaMode;
}ENNSemaphore;


typedef struct ENNMessage {
    sem_t                  sem;                  //信号量
    pthread_mutex_t        mutex;        //线程间互斥锁
    ENN_U32                 elementSize;   //DATAQ_FRAME结构体的大小
    ENN_U32                 noElements;    //DATAQ_FRAME结构体的个数的值
    ENNOS_MSG_CreateMode_t   msgMode;  //消息工作模式
    // msg's buf
    ENN_VOID               *msgs;//消息内容的位置地址mmsg+1
    // current position
    ENN_U32                 begin;     //消息队列当前所处的位置，从消息队列中读取数据的时候加1，
    // the number of existing msgs in the message queue
    ENN_U32                 num;       //消息队列中存在DATAQ_FRAME 结构体的个数，向消息队列写入数据的时候加1
}ENNMessage;
#endif


#endif

