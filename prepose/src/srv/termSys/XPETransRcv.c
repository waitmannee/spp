#include "ibdcs.h"
#include "tmcibtms.h"
#include "iso8583.h"
#include "pub_function.h"

static int s_myFid    = -1;//主进程读取管道的id
static int gs_qid = 0;//主进程和子线程通讯队列的id

static void XPETransRcvSvrExit(int signo);

extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];
struct ISO_8583 iso8583conf[128];
struct ISO_8583 iso8583posconf[128];

static int OpenLog(char *IDENT);
static int CreateMyFolder();
static int DoLoop();
static void * ThreadDeal (void *arg);
int GetFreeThread(pthread_t * pthread_id);
void ThreadPoolInit(int max_thread_num);

extern pthread_key_t iso8583_conf_key;//线程私有变量
extern pthread_key_t iso8583_key;//线程私有变量

/*线程池结构*/
typedef struct
{
    pthread_mutex_t queue_lock;/*线程锁*/
    pthread_t *threadid;/*threadid 线程号*/
    volatile int *threadstate; /*threadstate对应的线程状态,FREE_STAT 表示free,BUSINESS_STAT表示business*/
    int max_thread_num; /*线程池中允许的活动线程数目*/
} CThread_pool;

//share resource
static CThread_pool *pool = NULL;

int main(int argc, char *argv[])
{
	int qid;
	char szTmp[20];
	catch_all_signals(XPETransRcvSvrExit);

	if(0 > OpenLog(argv[0]))
		exit(1);

	dcs_log(0,0, "XPETranRcv Servers is starting up...");

	u_daemonize(NULL);

	//加载前置与终端的8583配置文件
	if(IsoLoad8583config(&iso8583posconf[0], "iso8583_pos.conf") < 0)
	{
		dcs_log(0,0,"Load iso8583_pos.conf failed:%s",strerror(errno));
		XPETransRcvSvrExit(0);
	}

	//加载前置与核心的8583配置文件
	if(IsoLoad8583config(&iso8583conf[0], "iso8583.conf") < 0)
	{
		dcs_log(0,0,"Load iso8583.conf failed:%s",strerror(errno));
		XPETransRcvSvrExit(0);
	}
	//设置为多线程
	SetIsoMultiThreadFlag(1);
	pthread_key_create(&iso8583_conf_key,NULL);
	pthread_key_create(&iso8583_key,NULL);

	//创建进程接收报文的管道
	if( CreateMyFolder() < 0 )
		XPETransRcvSvrExit(0);

	//创建与加密机通讯的接收队列,用进程号做队列名
	memset(szTmp,0,sizeof(szTmp));
	sprintf(szTmp,"%d",getpid());
	if(queue_create(szTmp)<0)
		dcs_log(0,0,"create queue %s err,errno=%d",szTmp,errno);

	//创建主进程与子线程通讯的队列，用于主进程向子线程分发处理报文
	memset(szTmp,0,sizeof(szTmp));
	strcpy(szTmp,"xpetransrcv_queue");
	gs_qid = queue_create(szTmp);
	if(gs_qid < 0)
		dcs_log(0,0,"create queue %s err,errno=%d",szTmp,errno);

	//创建线程池，包含10个线程
	ThreadPoolInit(10);

	//创建mysql 连接池，
	if(mysql_pool_init(10)< 0)
		XPETransRcvSvrExit(0);

	dcs_log(0,0,"*************************************************\n"
				"!!        XPETranRcv servers startup completed.        !!\n"
				"*************************************************\n");
	DoLoop();
	XPETransRcvSvrExit(0);
}

static void XPETransRcvSvrExit(int signo)
{
	dcs_log(0,0,"catch a signal %d",signo);
	    
	if(signo  == 15)
	    dcs_log(0,0,"脚本停服务");
	else if(signo == 11)
	{
		dcs_log(0,0,"内存溢出");
	}

	dcs_log(0,0,"XPETransRcv exit.");

	PoolDestroy();
	DestroyLinkPool();
	    
	exit(signo);
}

static int OpenLog(char *IDENT)
{
	char logfile[256];
	memset(logfile, 0, sizeof(logfile));
	
	if(u_fabricatefile("log/TransRcv.log",logfile,sizeof(logfile)) < 0)
		return -1;
	return dcs_log_open(logfile, IDENT);
}

/**
 * 创建进程自己接收的管道
 */
static int CreateMyFolder()
{
	if(fold_initsys() < 0)
	{
		dcs_log(0,0, "cannot attach to folder system!");
		return -1;
	}

	s_myFid = fold_create_folder("XPETRANSRCV");

	if(s_myFid < 0)
		s_myFid = fold_locate_folder("XPETRANSRCV");

	if(s_myFid < 0)
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", "XPETRANSRCV", ise_strerror(errno) );
		return -1;
	} 

	dcs_log(0,0, "folder myFid=%d\n", s_myFid);

	if(fold_get_maxmsg(s_myFid) <2)
		fold_set_maxmsg(s_myFid, 150) ;

	return 0;
}

static int DoLoop()
{
    int iRead =0, i =0;
    char caBuffer[PUB_MAX_SLEN+4+1];
    int fid = 0, fromFid = 0;
    pthread_t pthread_id = 0;
    int ret = 0;
    fid =fold_open(s_myFid);

    while(1)
    { 
        memset(caBuffer, 0, sizeof(caBuffer));
        iRead = fold_read(s_myFid,fid, &fromFid,caBuffer, sizeof(caBuffer), 1);
        if(iRead < 0) 
        {
            dcs_log(0,0,"fold_read() failed:%s\n",ise_strerror(errno));
            break;
        }
        if(memcmp( caBuffer, "BANK",4) !=0) /*非核心返回*/
        	continue;
        //在最前面添加4个字节formid
        sprintf(caBuffer, "%04d%s", fromFid,caBuffer+4);
        //流程待讨论
        for(i =0 ; i< 50; i++)
        {
        	//获取空闲的线程
			ret = GetFreeThread(&pthread_id);
			if(ret < 0)
			{
				//当前没有空闲线程
				u_sleep_us(100000);//100ms
				//sleep(1);//1s
				dcs_log(0,0,"not found free thread,please wait 0.1s\n");
				continue;
			}
			else
				break;
        }
        if(i >= 50)
        {
        	dcs_log(0,0,"sorry ,not found free thread,give up the message!\n");
         	continue;
        }

        //向子线程发送报文
#ifdef __TEST__
        dcs_log(0,0,"send to thread%lu",pthread_id);
#endif
        queue_send_by_msgtype(gs_qid, caBuffer, iRead , pthread_id, 0);
#ifdef __TEST__
        dcs_log(0,0,"send to thread%lu success",pthread_id);
#endif
    }
    close(fid);
    return -1;
}

/**
 *初始化线程池
 *参数 max_thread_num 最大线程数
 */
void ThreadPoolInit(int max_thread_num)
{
    pool = (CThread_pool *) malloc (sizeof (CThread_pool));
    pthread_mutex_init (&(pool->queue_lock), NULL);

    pool->max_thread_num = max_thread_num;
    pool->threadid = (pthread_t *) malloc (max_thread_num * sizeof (pthread_t));

    pool->threadstate = (int *) malloc (max_thread_num * sizeof (int));
    int i = 0;
    for (i = 0; i < max_thread_num; i++)
    {
        pthread_create (&(pool->threadid[i]), NULL, ThreadDeal,NULL);
        pool->threadstate[i] = FREE_STAT;//初始化为free
    }
}
/**
 * 获取一个空闲的线程,
 * 参数：pthread_id 当返回值等于0 时表示获取到的空闲的线程ID
 * 返回值：-1 没找到空闲线程
 * 等于0  表示获取到空闲线程的id
 */
int GetFreeThread(pthread_t * pthread_id){
	int i =0;
//	pthread_mutex_lock(&(pool->queue_lock));//上锁
	for(i = 0; i< pool->max_thread_num; i++)
	{
#ifdef __TEST__
		dcs_log(0,0,"threadstate=%d,threadid=%lu",pool->threadstate[i],pool->threadid[i]);
#endif
		if(pool->threadstate[i] == FREE_STAT)
		{
			pool->threadstate[i] = BUSINESS_STAT ;//将其状态改为忙碌
			//pthread_mutex_unlock(&(pool->queue_lock));
			*pthread_id = pool->threadid[i];
			return 0;
		}
	}
	//返回-1表示没有空闲的线程
//	pthread_mutex_unlock(&(pool->queue_lock));
	return -1;
}

/*销毁线程池
 * */
int PoolDestroy()
{
    free(pool->threadid);
    free(pool->threadstate);
    pthread_mutex_destroy(&(pool->queue_lock));
    free(pool);
    pool=NULL;
    return 0;
}
/**
 * 释放线程，将线程置为空闲状态
 */
void FreeThread()
{
	int i = 0;
//    pthread_mutex_lock(&(pool->queue_lock));
	for(i = 0; i< pool->max_thread_num; i++)
	{
#ifdef __TEST__
		dcs_log(0,0,"111FreeThread=%lu,threadid=%lu",pool->threadid[i],pthread_self());
#endif
		if(pool->threadid[i] == pthread_self())
		{
			pool->threadstate[i] = FREE_STAT;
			break;
		}
	}
#ifdef __TEST__
	dcs_log(0,0,"FreeThread");
#endif
//	pthread_mutex_unlock(&(pool->queue_lock));
    
}

/**
 * 线程处理函数,循环接收主进程的报文，处理结束后改变状态为free
 */
void * ThreadDeal (void *arg)
{
	char caBuffer[PUB_MAX_SLEN+4+1];
	char buf[10];
	pthread_t msgtype=0;
	int len = 0;
	int fromid = 0;
	msgtype = pthread_self();
#ifdef __TEST__
	dcs_log(0,0,"pthread is [%lu],[%ld]",pthread_self(), msgtype);
#endif
    while(1)
    {
    	memset(caBuffer , 0, sizeof(caBuffer));
    	len = queue_recv_by_msgtype(gs_qid,caBuffer,sizeof(caBuffer),&msgtype,0);
    	if(len < 0){
    		FreeThread();
    		continue;
    	}
    	//取出前面的formid
    	memset(buf , 0, sizeof(buf));
    	memcpy(buf, caBuffer, 4);
    	fromid = atoi(buf);
    	eposAppProc(caBuffer+4, fromid , len - 4);
    	FreeThread();
    }
}



