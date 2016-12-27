/**************************** (C) COPYRIGHT 2014 ENNO ****************************
 * 文件名	：ennOS.c
 * 描述	：          
 * 时间     	：
 * 版本    	：
 * 变更	：
 * 作者	：  
**********************************************************************************/	

#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include <sched.h>
#include <sys/reboot.h>
#include <sys/resource.h>

#include "ennDebug.h"
#include "ennAPI.h"
#include "internal.h"


#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */

/******************* global variable   ******************/
static pthread_key_t sg_ennOS_taskkey;
ENN_BOOL g_ennOS_InitFlag = ENN_FALSE;



static pthread_mutex_t sg_TaskMutex = PTHREAD_MUTEX_INITIALIZER;


//1.设置线程私有数据
//2.设置线程系统等级
//3.执行线程私有数据的函数和函数参数
static void* m_thread(void* arg)
{
    ENNTask *task;

    if(NULL == arg)
    {
        pthread_exit(NULL);
    }

    task = (ENNTask *)arg;
    pthread_setspecific(sg_ennOS_taskkey, (void *)task);

    if(ENNOS_TASK_SUSPEND == task->mode)
    {
        pthread_kill(task->pid, SIGSTOP );
    }
    ENNTRACE("\nm_thread: task->fuc=0x%08x  PID:%d\n\n",  task->fuc, getpid());

    if(ENNOS_TASK_PRIORITY_LOW == task->prio)
    {
        if(0 != setpriority(PRIO_PROCESS, getpid(), 10))
        {
            perror("setpriority: ");
        }
    }
    else if(ENNOS_TASK_PRIORITY_MIDDLE == task->prio)
    {
	      if(0 != setpriority(PRIO_PROCESS, getpid(), 5))
        {
            perror("setpriority: ");
        }
    }
	else
    {
        if(0 != setpriority(PRIO_PROCESS, getpid(), 1))
        {
            perror("setpriority: ");
        }
    }

    task->fuc( task->param );

    //memset((void *)task, 0, sizeof(cyTask));
    //free( task );
    //pthread_setspecific( sg_cyOS_taskkey, NULL );
    //task = NULL;

    return NULL;
}

ENN_U32 gWriteTraceEnable = 0;
ENN_VOID ENNWRITETRACE(ENN_CHAR *fmt, ...)
{
	if(gWriteTraceEnable)
	{
		va_list arg_ptr;
		va_start(arg_ptr, fmt);
		vfprintf(stdout, fmt, arg_ptr);
		va_end(arg_ptr);
	}
}



ENN_U32 gTraceEnable = 0;
ENN_VOID ENNTRACE(ENN_CHAR *fmt, ...)
{
	if(gTraceEnable)
	{
		va_list arg_ptr;
		va_start(arg_ptr, fmt);
		vfprintf(stdout, fmt, arg_ptr);
		va_end(arg_ptr);
	}
}


ENN_S32 ENN_strtol(const ENN_CHAR *nptr, ENN_CHAR **endptr, ENN_S32 base)
{
	const ENN_CHAR *p = nptr;
	ENN_ULONG ret;
	ENN_S32 ch;
	ENN_ULONG Overflow;
	ENN_S32 sign = 0, flag, LimitRemainder;

	/*跳过前面多余的空格，并判断正负符号。

	如果base是0，允许以0x开头的十六进制数，

	以0开头的8进制数。

	如果base是16，则同时也允许以0x开头。*/
	do
	{
		ch = *p++;
	}while (isspace(ch));
  
	if(ch == '-')
	{
		sign = 1;
		ch = *p++;
	}
	else if(ch == '+')
	{
		ch = *p++;
	}

	if((base == 0 || base == 16) &&
      ch == '0' && (*p == 'x' || *p == 'X'))
	{
      ch = p[1];
      p += 2;
      base = 16;
	}

	if(base == 0)
	{
		base = ch == '0' ? 8 : 10;
	}
	
	Overflow = sign ? -(unsigned long)LONG_MIN : LONG_MAX;
	LimitRemainder = Overflow % (unsigned long)base;
	Overflow /= (unsigned long)base;
	
	for(ret = 0, flag = 0;; ch = *p++)
	{
		/*把当前字符转换为相应运算中需要的值。*/
		if (isdigit(ch))
			ch -= '0';
		else if (isalpha(ch))
	        ch -= isupper(ch) ? 'A' - 10 : 'a' - 10;
		else
			break;

		if(ch >= base)
			break;

		/*如果产生溢出，则置标志位，以后不再做计算。*/
		if(flag < 0 || ret > Overflow || (ret == Overflow && ch > LimitRemainder))
			flag = -1;
		else
		{
			flag = 1;
			ret *= base;
			ret += ch;
		}
	}

	/*
	如果溢出，则返回相应的Overflow的峰值。
	没有溢出，如是符号位为负，则转换为负数。
	*/
	if(flag < 0)
		ret = sign ? LONG_MIN : LONG_MAX;
	else if (sign)
		ret = -ret;
	
	/*
	如字符串不为空，则*endptr等于指向nptr结束
	符的指针值；否则*endptr等于nptr的首地址。
	*/

	if (endptr != 0)
		*endptr = (char *)(flag ?(p - 1) : nptr);

	return ret;
}

ENN_CHAR *ENN_Int_To_Format(ENN_S32 num, ENN_CHAR *str, ENN_S32 radix)   
{    
    /* 索引表 */   
    ENN_CHAR index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";   
    ENN_U32 unum; /* 中间变量 */   
    ENN_S32 i=0,j,k; 
	
    /* 确定unum的值 */   
    if((10 == radix) && (num < 0)) /* 十进制负数 */   
    {   
        unum = (ENN_U32)-num;   
        str[i++] = '-';   
    }   
    else 
	{
		unum=(ENN_U32)num; /* 其他情况 */  
	}
	
    /* 逆序 */   
    do    
    {   
        str[i++] = index[unum%(ENN_U32)radix];   
        unum /= radix;   
    }while(unum);
	
    str[i] = '\0';   
    /* 转换 */   
    if('-' == str[0]) 
	{
		k = 1; /* 十进制负数 */ 
	}
    else 
	{
		k = 0;
	}
	
    /* 将原来的“/2”改为“/2.0”，保证当num在16~255之间，radix等于16时，也能得到正确结果 */   
    ENN_CHAR temp;   
    for(j=k; j<=(i-k-1)/2.0; j++)   
    {   
        temp = str[j];   
        str[j] = str[i-j-1];   
        str[i-j-1] = temp;   
    }
	
    return str;   
}


ENN_U32 ENNStr_To_Hex(ENN_CHAR *str)
{
	ENN_U32 i,sum = 0;
	
	ENNAPI_ASSERT(NULL != str);
	
	for(i = 0;str[i] != '\0';i++)
	{
		if(str[i]>='0' && str[i]<='9')
		{
	        	sum = sum*16 + str[i]-'0';
		}
		else if( str[i]>='a'&&str[i]<='f' )
	    	{
		        sum = sum*16 + str[i]-'a'+10;
	    	}
		else if( str[i]>='A'&&str[i]<='F' )
	    	{
		        sum = sum*16 + str[i]-'A'+10;
	    	}

	}
	return sum;
} 


ENN_U32 ENNOS_GetSysTime(ENN_VOID)
{
    struct timeval tm_now;
    gettimeofday(&tm_now, NULL);
    return (ENN_U32)((tm_now.tv_sec)*1000 + tm_now.tv_usec/1000);
}



/********************************************************************
* 函 数 名：		ENNOS_Init
* 函数介绍:		OS初始化
* 输入参数:		无
* 输出参数:		无
* 返 回 值:		成功返回ENNO_SUCCESS
*					失败返回ENNO_FAILURE
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_Init(ENN_VOID)
{
	ENNTask *task;

    if(ENN_FALSE == g_ennOS_InitFlag)
    {
        task = (ENNTask *)malloc(sizeof(ENNTask));
        if(NULL == task)
        {
            return ENN_FAIL;
        }
        memset((void *)task, 0, sizeof(ENNTask));

        /* main task for MAPI init caller */
        task->name  = "Main thread";
        task->prio  = ENNOS_TASK_PRIORITY_MIDDLE;
        task->mode  = ENNOS_TASK_START;
        task->stack = 0x10000;  //64KB
        task->fuc 	= NULL;
        task->param = NULL;
        task->pid 	= pthread_self();

        /* init TSD */
        if(0 != pthread_key_create(&sg_ennOS_taskkey, NULL))
        {
            free(task);
			task = NULL;
            return ENN_FAIL;
        }

        /* set TSD to global variable */
        pthread_setspecific(sg_ennOS_taskkey, (void *)task);
	#if 0
		if ( 0 != timer_init())
		{
			free( task );
		    return PL_FAIL;
		 }
	#endif

        g_ennOS_InitFlag = ENN_TRUE;
        ENNTRACE("os init ok!\n");
        return ENN_SUCCESS;
    }

    return ENN_SUCCESS;
}


/********************************************************************
* 函 数 名：		ENNOS_CreateTask
* 函数介绍:		按照接口的输入参数创建一个任务
*					（Linux下为一个独立线程），线程
*					函数作为该接口的参数传入，当线
*					程函数返回或者退出时创建的任务
*					也同时销毁
* 输入参数:	*name:		任务名称
*					prio     :		任务优先级
*					stack   :		任务堆栈大小
*					mode  :		任务创建模式，可参考cyOS_TASK_CreateMode_t
*					fuc	    :		任务入口函数
*					*param:	任务创建时出入fuc的参数
* 输出参数:		*id:		调用成功时返回的任务ID
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_CreateTask(ENN_CHAR *name, 
									   	       ENNOS_TASK_Priority_t prio,
									               ENN_U32 stack, 
									               ENNOS_TASK_t *id,
									               ENNOS_TASK_CreateMode_t mode, 
									               ENNOS_TASKENTRY_f fuc,
									               ENN_VOID *param )
{
	ENNTask *task;
	pthread_attr_t attr;
	struct sched_param s_param;
	ENN_U32 stacksize = stack;
	ENN_S32 s_policy;
	static ENN_U32 taskcnt = 0;

	ENNAPI_ASSERT(NULL != id);

	pthread_mutex_lock(&sg_TaskMutex);                   //加锁处理，放置创建线程过程中的视图同步
	task = (ENNTask *)malloc(sizeof(ENNTask));
	if(NULL == task)
	{
		printf("ENNOS_CreateTask fail!\n");
		pthread_mutex_unlock(&sg_TaskMutex);
		return ENN_FAIL;
	}

	memset((void *)task , 0 , sizeof(ENNTask));
	pthread_attr_init(&attr);

	if((stacksize > 0) && (stacksize < PTHREAD_STACK_MIN))
	{
		stacksize = PTHREAD_STACK_MIN;
	}
	else if(0 == stacksize)
	{
		pthread_attr_getstacksize(&attr, &stacksize);
	}

	if((prio < ENNOS_TASK_PRIORITY_LOW) || (prio > ENNOS_TASK_PRIORITY_HIGH))
	{
		prio = ENNOS_TASK_PRIORITY_MIDDLE;
	}
	printf("%s,%d,prio = %d\n",__FUNCTION__,__LINE__,prio);
	/* set sched policy */
	//s_policy = SCHED_RR;
	//s_policy = SCHED_OTHER;
	s_policy = SCHED_FIFO;
	pthread_attr_setschedpolicy(&attr, s_policy);

	/* set input task params */
	task->name	= name;
	task->prio	= prio;
	task->stack = stacksize;
	task->mode	= mode; /* Notes: mode not supported in linux! */
	task->fuc	= fuc;
	task->param = param;
        task->taskid = (ENN_U32)task | (taskcnt++ & 0x03);

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED );
	pthread_attr_setstacksize(&attr, stacksize);

	pthread_attr_getschedparam(&attr, &s_param);
	s_param.sched_priority = prio;
	pthread_attr_setschedparam(&attr, &s_param);

	if(0 != pthread_create(&task->pid, &attr, m_thread ,task))
	{
		pthread_attr_destroy(&attr);
		free(task);
		pthread_mutex_unlock(&sg_TaskMutex);
		return ENN_FAIL;
	}

	ENNTRACE("ENNOS_CreateTask task=0x%x, id = 0x%x, pid=%d name=%s priority=%d func=0x%08x\n\n", task, task->taskid, task->pid, task->name, task->prio, task->fuc);

	*id=(ENNOS_TASK_t)task->taskid;
	pthread_mutex_unlock(&sg_TaskMutex);  //释放锁
	return ENN_SUCCESS;
}



/********************************************************************
* 函 数 名：		ENNOS_ExitTask
* 函数介绍:		调用该接口退出当前任务的运行，即当前任务自我删除
* 输入参数:		无
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_ExitTask(ENN_VOID)
{
	ENNTask *task = NULL;

	pthread_mutex_lock(&sg_TaskMutex);
	task = (ENNTask *)pthread_getspecific(sg_ennOS_taskkey);
	if(NULL == task)
	{
		printf("ENNOS_ExitTask fail!");
		pthread_mutex_unlock(&sg_TaskMutex);
		return ENN_FAIL;
	}
	/* add by SJC for debug*/
	ENNTRACE("ENNOS_ExitTask task=0x%x, id = 0x%x, pid=%d name=%s priority=%d func=0x%08x\n\n", task, task->taskid, task->pid, task->name, task->prio, task->fuc);

	memset((void *)task, 0, sizeof(ENNTask));
	free(task);
	task = NULL;

	pthread_mutex_unlock(&sg_TaskMutex);
	pthread_exit( NULL );
	return ENN_SUCCESS;/* just make the compiler smile */
}




/********************************************************************
* 函 数 名：		ENNOS_DeleteTask
* 函数介绍:		删除一个任务，应用可以调用该接口
*					终止其它任务，也可以终止当前任务
*					本身，出入的参数为每个任务的ID号；
*					删除任务前要确保被删除的任务所占
*					用的资源已经被释放
* 输入参数:		tid:  		线程ID号
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_DeleteTask(ENNOS_TASK_t tid)
{
	ENNTask *task = NULL;

	pthread_mutex_lock(&sg_TaskMutex);
	task = (ENNTask *)(tid & 0xfffffffc);
	if(NULL == task)
	{
		printf("ENNOS_DeleteTask fail!");
		pthread_mutex_unlock(&sg_TaskMutex);
		return ENN_FAIL;
	}
	/* add by SJC for debug*/
	ENNTRACE("ENNOS_DeleteTask task=0x%x, id=0x%x, pid=%d name=%s priority=%d func=0x%08x\n\n", task, task->taskid, task->pid, task->name, task->prio, task->fuc);

	if(task->taskid != tid)
	{
		pthread_mutex_unlock(&sg_TaskMutex);
		return ENN_SUCCESS;
	}

	if(task->pid == 0)
	{
		pthread_mutex_unlock(&sg_TaskMutex);
		return ENN_SUCCESS;
	}

	if(task->fuc == NULL)
	{
		pthread_mutex_unlock(&sg_TaskMutex);
		return ENN_SUCCESS;
	}

	if(task->pid ==  pthread_self())
	{
		pthread_mutex_unlock(&sg_TaskMutex);
		pthread_exit(NULL);
		return ENN_SUCCESS;
	}

	pthread_cancel(task->pid);
	//pthread_join(task->pid, NULL);

	memset((void *)task, 0, sizeof(ENNTask));
	free(task);
	task = NULL;

	pthread_mutex_unlock(&sg_TaskMutex);
	
	return ENN_SUCCESS;
}



/********************************************************************
* 函 数 名：		ENNOS_SuspendTask
* 函数介绍:		暂停一个运行的任务，以任务ID为标
*					识，如果任务已经暂停，直接返回成功
* 输入参数:		tid:  		线程ID号
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_SuspendTask (ENNOS_TASK_t tid)
{
	ENNTask *task;
	
	task = (ENNTask *)(tid & 0xfffffffc);
	if(NULL == task)
	{
		printf("ENNOS_SuspendTask fail!");
		return ENN_FAIL;
	}
	/* add by SJC for debug*/
	ENNTRACE("ENNOS_SuspendTask id=%d name=%s priority=%d func=0x%08x\n\n", task->pid, task->name, task->prio, task->fuc);
	if(0 == pthread_kill(task->pid, SIGSTOP))
	{
		return ENN_SUCCESS;
	}
	else
	{
		return ENN_FAIL;
	}
}



/********************************************************************
* 函 数 名：		ENNOS_ResumeTask
* 函数介绍:		继续运行暂停的任务。如果任务处于运行状态，则直接返回
* 输入参数:		tid:  		线程ID号
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_ResumeTask(ENNOS_TASK_t tid)
{
	ENNTask *task = NULL;

	task = (ENNTask *)(tid & 0xfffffffc);
	if(NULL == task)
	{
		printf("ENNOS_ResumeTask fail!");
		return ENN_FAIL;
	}
	/* add by SJC for debug*/
	ENNTRACE("ENNOS_ResumeTask id=%d name=%s priority=%d func=0x%08x\n\n", task->pid, task->name, task->prio, task->fuc);
	if(0 == pthread_kill(task->pid , SIGCONT))
	{
		return ENN_SUCCESS;
	}
	else
	{
		return ENN_FAIL;
	}
}



/********************************************************************
* 函 数 名：		ENNOS_DelayTask
* 函数介绍:		调用的任务挂起若干时间。如果设置的时间小于系统最小时间片并且不等于0（Linux最小时间片为10毫秒），可以设置为系统最小时间片，输入的参数以毫秒为单位
* 输入参数:		delay:  		延时时间，以毫秒为单位
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_DelayTask(ENN_U32 delay)
{
	if(0 == delay)
	{
		sched_yield();
	}
	else
	{
		usleep(delay*1000);
	}

	return ENN_SUCCESS;
}



/********************************************************************
* 函 数 名：		ENNOS_GetTaskPriority
* 函数介绍:		获取指定任务的优先级，共分高、中、低三类优先级，具体定义可参考cyOS_TASK_Priority_t
* 输入参数:		tid:  		线程ID号
* 输出参数:		*priority:		获取的线程优先级
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_GetTaskPriority(ENNOS_TASK_t tid, ENNOS_TASK_Priority_t *priority)
{
	ENNTask *task;
	ENNAPI_ASSERT(NULL != priority);

	task  = (ENNTask *)(tid & 0xfffffffc);
	if ((task->prio < ENNOS_TASK_PRIORITY_LOW)||(task->prio > ENNOS_TASK_PRIORITY_HIGH))
	{
		printf("tid:%d task prio error!\n",tid);
		return ENN_FAIL;
	}

	*priority = task->prio;
	return ENN_SUCCESS;
}



/********************************************************************
* 函 数 名：		ENNOS_SetTaskPriority
* 函数介绍:		设置指定任务的优先级
* 输入参数:		tid:  		线程ID号
*                          	newPriority:	新的线程优先级
* 输出参数:		无
* 返 回 值:		成功返回0
*					失败返回-1
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_SetTaskPriority(ENNOS_TASK_t tid, ENNOS_TASK_Priority_t newPriority)
{
	ENNTask *task;

	task = (ENNTask *)(tid & 0xfffffffc);

	if ((newPriority < ENNOS_TASK_PRIORITY_LOW)||(newPriority > ENNOS_TASK_PRIORITY_HIGH))
	{
		printf("tid:%d task prio error!\n",tid);
		return ENN_FAIL;
	}

	task->prio = newPriority;

	return ENN_SUCCESS;
}


/********************************************************************
* 函 数 名：		ENNOS_CreateSemaphore
* 函数介绍:		创建信号量，分为FIFO类型和优先级类型，调用成功则获取到该信号量的ID
* 输入参数:		value: 		初始值
*                          	      semaMode:	信号量模式
* 输出参数:		semaId:		获取的信号量
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_CreateSemaphore(ENN_U32 value,
                                              ENNOS_SEMA_CreateMode_t semaMode,
                                              ENNOS_SEMA_t *semaId)
{
    ENNSemaphore *msem;

    msem = (ENNSemaphore *)malloc(sizeof(ENNSemaphore));
    if(NULL == msem)
    {
        return ENN_FAIL;
    }
    memset((void *)msem, 0, sizeof(ENNSemaphore));

    msem->semaMode = semaMode ;
    if(0 != sem_init(&msem->sem , 0, value))
    {
        free(msem);
		msem = NULL;
        return ENN_FAIL;
    }

    *semaId = (ENNOS_SEMA_t)msem;
    return ENN_SUCCESS;
}


/********************************************************************
* 函 数 名：		ENNOS_DeleteSemaphore
* 函数介绍:		删除信号量，以信号量的ID为标识
* 输入参数:		semaphore: 		信号量ID
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_DeleteSemaphore(ENNOS_SEMA_t semaphore)
{
    ENNSemaphore *msem = NULL;
    int rt = -1;

	if(0 == semaphore) 
	{
		printf("semaphore is NULL\n");
		return ENN_FAIL;
	}
    msem = (ENNSemaphore *)semaphore;

    rt = sem_destroy(&msem->sem);

    if(0 == rt)
    {
        free(msem);
		msem = NULL;
        return ENN_SUCCESS;
    }
    else
    {
        perror("sema del::");
        return ENN_FAIL;
    }
}


/********************************************************************
* 函 数 名：		ENNOS_WaitSemaphore
* 函数介绍:		等待指定ID的信号量，如果指定的超时时间到时信号量仍然没有获得就返回失败
* 输入参数:		semaphore: 		信号量ID
*                          	timeout:			信号量等待超时时间,以毫秒为单位
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_WaitSemaphore(ENNOS_SEMA_t semaphore, ENN_U32 timeout)
{
    struct timeval   now;
    struct timespec  ts;
    int              rt;
    ENNSemaphore      *msem = NULL;

	if(0 == semaphore)
	{
		printf("semaphore is NULL\n");
		return ENN_FAIL;
	}

    msem = (ENNSemaphore *)semaphore;
    switch(timeout)
    {
        case ENNOS_TIMEOUT_IMMEDIATE:
        {
            rt = sem_trywait(&msem->sem);
            break;
        }
        case ENNOS_TIMEOUT_INFINITY:
        {
            rt = sem_wait(&msem->sem);
            break;
        }
        default:
        {
            gettimeofday( &now, NULL );
            ts.tv_sec   = now.tv_sec + timeout/1000 ;
            ts.tv_nsec  = now.tv_usec * 1000 + timeout%1000*1000000;
            ts.tv_sec  += ts.tv_nsec/1000000000;
            ts.tv_nsec = ts.tv_nsec%1000000000;
            rt     = sem_timedwait( &msem->sem ,&ts );
            break;
        }
    }

    if(0 == rt)
    {
        return ENN_SUCCESS;
    }
    else
    {
        return ENN_FAIL;
    }
}


/********************************************************************
* 函 数 名：		ENNOS_SignalSemaphore
* 函数介绍:		释放一个信号量，以信号量ID作为标识
* 输入参数:		semaphore: 		信号量ID
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_SignalSemaphore (ENNOS_SEMA_t semaphore)
{
    ENNSemaphore   *msem = NULL;
    int          rt = -1;

	if(0 == semaphore) 
	{
		printf("semaphore is NULL\n");
		return ENN_FAIL;
	}

    msem = (ENNSemaphore *)semaphore;
    rt = sem_post(&msem->sem);
    if(0 == rt)
    {
        return ENN_SUCCESS;
    }
    else
    {
        return ENN_FAIL;
    }
}


/********************************************************************
* 函 数 名：		ENNOS_CreateMessage
* 函数介绍:		创建一个消息队列，用户输入要创建的消息队列的消息大小，消息个数和消息等待方式，执行成功时输出得到的消息队列ID
* 输入参数:		elementSize:DATAQ_FRAME结构体的大小
*                          		noElements:	DATAQ_FRAME结构体的个数
*                          		msgMode:	消息队列的模式
* 输出参数:		message:
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_CreateMessage (ENN_U32 elementSize,ENN_U32 noElements, 
												    ENNOS_MSG_CreateMode_t msgMode, ENNOS_MSG_t *message)
{
    ENNMessage *mmsg = NULL;

    ENNAPI_ASSERT(NULL != message);

    mmsg = (ENNMessage *)malloc(sizeof(ENNMessage) + elementSize * noElements);
    if(NULL== mmsg)
    {
        return ENN_FAIL;
    }
    memset((void *)mmsg , 0 , sizeof(ENNMessage) );

    mmsg->elementSize = elementSize;
    mmsg->noElements  = noElements;
    mmsg->msgMode     = msgMode;
    mmsg->msgs        	= (ENN_VOID*)(mmsg+1);
    mmsg->begin             = 0;
    mmsg->num              = 0;

    pthread_mutex_init(&mmsg->mutex , NULL);  //互斥量初始化
    sem_init(&mmsg->sem, 0, 0);                          //信号量初始化

    *message = (ENNOS_MSG_t)mmsg;
    return ENN_SUCCESS;
}


/********************************************************************
* 函 数 名：		ENNOS_DeleteMessage
* 函数介绍:		删除指定的消息队列，以消息队列ID作为标识。当消息队列删除以后，所有仍在等待接收该消息的任务都应该返回失败
* 输入参数:		messageQueue: 
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_DeleteMessage(ENNOS_MSG_t messageQueue)
{
    ENNMessage   *mmsg = NULL;

    mmsg = (ENNMessage *)messageQueue;
    if(0 != sem_destroy(&mmsg->sem))
    {
        perror("msg del::");
        return ENN_FAIL;
    }

    pthread_mutex_destroy(&mmsg->mutex);
    free(mmsg);
	mmsg = NULL;
    return ENN_SUCCESS;
}


/********************************************************************
* 函 数 名：		ENNOS_ReceiveMessage
* 函数介绍:		从指定的消息队列接收一个消息。如果在指定的时间内没有收到消息，将返回失败
* 输入参数:		messageQueue: 	接收消息ID号
*                          	      timeout:			接收消息超时时限
* 输出参数:		msg:			接收到的消息存放指针
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t	ENNOS_ReceiveMessage(ENNOS_MSG_t messageQueue,
													ENN_U32 timeout, ENN_VOID *msg)
{
	int 		rt = ENN_SUCCESS;
	ENNMessage	 *mmsg = NULL;
	char		*dst = NULL;

	struct timeval	 now;
	struct timespec  ts;

	mmsg = (ENNMessage *)messageQueue;

	switch (timeout)
	{
		case ENNOS_TIMEOUT_IMMEDIATE:
		{
			rt = sem_trywait(&mmsg->sem);
			break;
		}
		case ENNOS_TIMEOUT_INFINITY:
		{
			rt = sem_wait(&mmsg->sem);
			break;
		}
		default:
		{
			gettimeofday( &now, NULL );
			ts.tv_sec	= now.tv_sec + timeout/1000 ;
			ts.tv_nsec	= now.tv_usec * 1000 + timeout%1000*1000000;
			ts.tv_sec  += ts.tv_nsec/1000000000;
			ts.tv_nsec = ts.tv_nsec%1000000000;
			rt	   = sem_timedwait(&mmsg->sem, &ts);
			break;
		}
	}

	if (0 != rt)
	{
		return ENN_FAIL;
	}

	pthread_mutex_lock(&mmsg->mutex);
	dst = (char *)mmsg->msgs;
	dst += mmsg->begin * mmsg->elementSize;

	memcpy(msg, dst, mmsg->elementSize);

	mmsg->begin = (mmsg->begin + 1) % mmsg->noElements;
	mmsg->num--;
	pthread_mutex_unlock(&mmsg->mutex);

	return ENN_SUCCESS;
}


/********************************************************************
* 函 数 名：		ENNOS_SendMessage
* 函数介绍:		往指定的队列发送一个消息，以消息队列ID为标
                    识，如果队列已满，返回失败
* 输入参数:		messageQueue:  		接收消息ID号
*                          	msg:	发送的消息存放指针
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_SendMessage(ENNOS_MSG_t messageQueue, ENN_VOID * msg)
{
	ENNMessage		*mmsg = NULL;
	ENN_ErrorCode_t	rt = ENN_SUCCESS;
	char		   *dst = NULL;

	mmsg = (ENNMessage *)messageQueue;

	pthread_mutex_lock(&mmsg->mutex);

	// too many msgs ?
	if(mmsg->num >= mmsg->noElements)
	{
		rt = ENN_FAIL;
	}
	else
	{
		/* look for the position that can store msg */
		dst = (char *) mmsg->msgs;
		if(mmsg->msgMode == ENNOS_MSG_CREATE_FIFO)
		{
			dst += ( (mmsg->begin + mmsg->num) % mmsg->noElements ) * mmsg->elementSize;
		}
		else
		{
			/* CYOS_MSG_CREATE_PRIORITY  ??? */
			mmsg->begin = (mmsg->begin>0) ? (mmsg->begin-1) : (mmsg->noElements-1);
			dst += (mmsg->begin) * mmsg->elementSize;
		}

		memcpy( dst, msg, mmsg->elementSize );
		mmsg->num++;
	}

	pthread_mutex_unlock(&mmsg->mutex);

	if(ENN_SUCCESS == rt)
	{
		sem_post(&mmsg->sem);
	}
	return rt;
}


/********************************************************************
* 函 数 名：		ENNOS_EmptyMessage
* 函数介绍:		清空指定的消息队列中的所有消息，以消息队列ID为标识
* 输入参数:		messageQueue:  		接收消息ID号
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_EmptyMessage(ENNOS_MSG_t messageQueue)
{
    ENNMessage        *mmsg = NULL;

    mmsg = (ENNMessage *)messageQueue;
    while (0 == sem_trywait(&mmsg->sem))
    {
        pthread_mutex_lock(&mmsg->mutex);
        mmsg->num--;
        pthread_mutex_unlock(&mmsg->mutex);
    }

    return  ENN_SUCCESS;
}


/********************************************************************
* 函 数 名：		ENNOS_AllocMemory
* 函数介绍:		分配指定大小的内存，分配失败返回空指针，调用本接口得到的内存必须用cyOS_FreeMemory接口释放，否则会造成内存泄漏
* 输入参数:		requested:  		申请的内存块大小，以字节为单位
* 输出参数:		无
* 返 回 值:		直接将malloc的返回值作为接口返回值。
* 修          改:
********************************************************************/
ENN_VOID* ENNOS_AllocMemory(ENN_U32 requested)
{
	ENN_VOID *pTmp = NULL;
    pTmp = (ENN_VOID *)malloc(requested);
	if (NULL == pTmp)
	{
		printf("alloc %d bytes fail!\n",requested);
	}
	return pTmp;
}


/********************************************************************
* 函 数 名：		ENNOS_ReallocMemory
* 函数介绍:		在原分配的内存基础上重新获取新的内存大小，输入为原来内存块的首地址和需要重新分配的内存大小，分配成功则返回新分配的内存块指针，否则返回空指针
* 输入参数:		block:			 原内存块指针
*					requested:  		申请的内存块大小，以字节为单位
* 输出参数:		无
* 返 回 值:		成功返回追加内存后的新内存块指针。
* 修          改:
********************************************************************/
ENN_VOID* ENNOS_ReallocMemory(ENN_VOID *block, ENN_U32 requested)
{
    ENNAPI_ASSERT(block != NULL);
    return realloc(block, requested);
}


/********************************************************************
* 函 数 名：		ENNOS_FreeMemory
* 函数介绍:		释放指定的内存块，该内存块必须时通过ENNOS_AllocMemory和cyOS_ReallocMemory分配得到的
* 输入参数:		block:			 释放的内存块指针
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_ErrorCode_t ENNOS_FreeMemory(ENN_VOID * block)
{
    ENNAPI_ASSERT(block != NULL);
    free(block);
	block = NULL;
    return ENN_SUCCESS;
}


/********************************************************************
* 函 数 名：		ENNOS_GetFreeMemory
* 函数介绍:		获取系统当前的空余内存大小，以字节为单位
* 输入参数:		无
* 输出参数:		无
* 返 回 值:		成功返回当前系统可用的总内存大小。
* 修          改:
********************************************************************/
ENN_S32 ENNOS_GetFreeMemory(ENN_VOID)
{
    struct sysinfo info;

    if (0 != sysinfo(&info))
    {
        return ENN_FAIL;
    }
    return (ENN_S32)info.freeram;
}


/********************************************************************
* 函 数 名：		ENNOS_GetUsedMemory
* 函数介绍:		获取系统当前使用的内存大小，以字节为单位
* 输入参数:		无
* 输出参数:		无
* 返 回 值:		成功返回ENN_SUCCESS
*					失败返回ENN_FAIL
* 修          改:
********************************************************************/
ENN_S32 ENNOS_GetUsedMemory(ENN_VOID)
{
    struct sysinfo info;

    if (0 != sysinfo(&info))
    {
        return ENN_FAIL;
    }
    return (ENN_S32)(info.totalram - info.freeram);
}


/********************************************************************
* 函 数 名：		ENNOS_GetMaxFreeMemory
* 函数介绍:		获得当前空余的最大块内存的大小，即当前系统可以申请的最大的内存，以字节为单位
* 输入参数:		无
* 输出参数:		无
* 返 回 值:		返回当前系统总内存容量。
* 修          改:
* 其	      它: 	暂不支持本功能
********************************************************************/
ENN_S32 ENNOS_GetMaxFreeMemory(ENN_VOID)
{
    printf("dummy call!\n");
    return ENN_FAIL;
}


/********************************************************************
* 函 数 名：		ENNOS_SwReset
* 函数介绍:		复位系统，即重新启动
* 输入参数:		无
* 输出参数:		无
* 返 回 值:		无
* 修          改:
********************************************************************/
ENN_VOID ENNOS_SwReset(ENN_VOID)
{
    //sleep(1);
	printf("%s, %d\n",__FUNCTION__,__LINE__);
    	//reboot(RB_AUTOBOOT);
    	system("reboot");
}



ENN_VOID cyTRACE_Enable(ENN_BOOL bEnable)
{
	if (bEnable)
	{
		gTraceEnable = ENN_TRUE;
	}
	else
	{
		gTraceEnable = ENN_FALSE;
	}
}




#ifdef __cplusplus
#if __cplusplus
    }
#endif /* __cpluscplus */
#endif /* __cpluscplus */



