/*
   功能描述:
   本文件主要实现超时控制功能，通过folder接收到业务处理进程发过来的报文
   ,根据收到的要求处理其超时重发.
    add by wu at 2014/12/29
*/

#include "unixhdrs.h"
#include "extern_function.h"
#include "timer.h"
#include <sys/syscall.h>

static int g_myFid = 1;
static char sg_ProcessName[20]="timerProc";

static int Do_Timer_Proc();
static int Open_timer_Log(char * buf);
static int Create_Myfolder();
static void Timer_Exit(int signo);
static void *Deal_Timer_Control( void *ptr);
static void Init_key_Control();
static int Add_Key_Control(char *key,pthread_t pid);
static int Delete_Key_Control(char *key,int flag);
static int Empty_Key_Control();
static int Check_Process_IsExist_ByName( char * processName);
struct Key_struct
{
	char key[20+1];
	pthread_t pid;
	struct Key_struct *next;
}*head,*tail;

pthread_mutex_t work_mutex;

int main(int arvc , char *argv[])
{
	catch_all_signals(Timer_Exit);
	
	if(Open_timer_Log(argv[0]) < 0)
		return -1;

	if(Check_Process_IsExist_ByName(sg_ProcessName) != 1)
	{
		dcs_log(0 , 0, "进程 %s 已经存在.",sg_ProcessName);
		return -1;
	}
	
	if(Create_Myfolder() < 0)
	{
		dcs_log(0 , 0, "creat my folder fail");
		return -1;
	}	
	Init_key_Control();

	if(pthread_mutex_init(&work_mutex, NULL) < 0)
	{
		dcs_log(0 , 0, "pthread_mutex_init failed");
		return -1;
	}
	dcs_log(0,0,"*************************************************\n"
			"!!        超时控制进程启动成功.        !!\n"
			"*************************************************\n");
	Do_Timer_Proc();	
	
}

void Print_Key_Control()
{
    struct Key_struct *ptr =head;
    //加线程锁
    pthread_mutex_lock(&work_mutex);
    while(ptr != NULL)
    {
        dcs_log(0,0,"key=%s,id=%ld\n",ptr->key,ptr->pid);
        ptr = ptr->next;
    }
    pthread_mutex_unlock(&work_mutex);
}

int Do_Timer_Proc()
{
	int fid = 0,org_folderId = 0,ret = 0;
	char buf[sizeof(struct Timer_struct)];
	char tmp[20+1];
	struct Timer_struct *ptr = NULL;
	pthread_t pid = 0;
	
	fid = fold_open(g_myFid);
	
	while(1)
	{	
		memset(buf, 0, sizeof(buf));
		ret =fold_read(g_myFid, fid ,&org_folderId, buf, sizeof(buf), 1);
		if(ret <= 0)
			continue;

		if(buf[0] == TIMER_ADD)
		{
			//新增一个超时控制
			dcs_log( 0, 0, "新增一个超时控制");
			ptr = (struct Timer_struct *)malloc(sizeof(struct Timer_struct));
			if(ptr == NULL)
			{
				dcs_log(0, 0, "malloc fail ");
			}
			
			//拆分超时控制信息
			memcpy(ptr->key, buf+1, 20);//key 
			trim(ptr->key);
			ptr->timeout = buf[21];  //超时时间 单位秒
			ptr->sendnum = buf[22]; //重发次数
			ptr->foldid =  buf[23];  //重发folder 索引

			memset(tmp, 0, sizeof(tmp));
			memcpy(tmp,buf+24,4);
			ptr->length = atoi(tmp);//重发报文长度
			memcpy(ptr->data,buf+28,ptr->length); //重发报文内容
			memset(tmp, 0 ,sizeof(tmp));
			strcpy(tmp, ptr->key);//保存key,后面使用防止被线程释放掉
			if(pthread_create(&pid, NULL,Deal_Timer_Control, (void *)ptr) < 0)
			{
				free(ptr);
				dcs_log(0, 0, "pthread_creat fail ");
				continue;
			}
			pthread_detach(pid);
			//修改key控制区
			Add_Key_Control(tmp,pid);
			Print_Key_Control();
		}
		else if(buf[0] == TIMER_REMOVE)
		{
			//去除一个超时控制
			dcs_log( 0, 0, "移除一个超时控制,key=%s",buf+1);
			Delete_Key_Control(buf+1,1);
			Print_Key_Control();
		}
		else
		{
			//非法内容,丢弃
			dcs_debug(buf, ret, "非法内容,丢弃");
		}		
	}
	close(fid);
	return 0;
}

// 函数名称  Open_timer_Log
// 参数说明：
//          buf 本进程名
// 返 回 值：
//          成功返回0,否则-1
// 功能描述：
//          创建一个日志文件并打开

int Open_timer_Log(char * buf)
{
	char  caLogName[256];
	char logfile[256];

	memset(caLogName,0,sizeof(caLogName));
	memset(logfile,0,sizeof(logfile));
	sprintf(caLogName,"log/timerProc.log");
	if(u_fabricatefile(caLogName,logfile,sizeof(logfile)) < 0)
	{
		printf("can not open log file:[%s]",caLogName);
		return -1;
	}
	return  dcs_log_open(logfile, buf);
}

// 函数名称  Create_Myfolder
// 参数说明：
//        创建folder
// 返 回 值：
//          退出进程
// 功能描述：
//       处理捕获到的信号
int Create_Myfolder()
{
	if(fold_initsys() < 0)
	{
		dcs_log(0,0, "cannot attach to folder system!");
		return -1;
	}

	g_myFid = fold_create_folder(TIMER_FOLDER);

	if(g_myFid < 0)
		g_myFid = fold_locate_folder(TIMER_FOLDER);    

	if(g_myFid < 0)        
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", TIMER_FOLDER, strerror(errno) );
		return -1;
	} 

	dcs_log(0,0, "folder myFid=%d\n", g_myFid);
	return 0;
}

// 函数名称  Timer_Exit
// 参数说明：
//          捕获的信号
// 返 回 值：
//          退出进程
// 功能描述：
//       处理捕获到的信号
static void Timer_Exit(int signo)
{
	dcs_log(0, 0, "timerProc catch sinal %d...\n",signo);    

	switch (signo)
	{
		case 0:
		case 11:
			dcs_log(0,0,"重起服务");
			Start_Process(sg_ProcessName,NULL);
			break;
		case SIGTERM:
			dcs_log(0,0,"脚本停服务");
			break;
		default:
		      break;
	}
	dcs_debug(0,0,"exit system");
	Empty_Key_Control();
	pthread_mutex_destroy(&work_mutex);
	exit(signo);
}

// 函数名称  Deal_Timer_Control
// 参数说明：
//        传入的包的信息
// 返 回 值：
//          无
// 功能描述：
//       处理收到的包，超时重发
void *Deal_Timer_Control( void *ptr)
{
	struct Timer_struct  strTimer;
	int i,org_folderId=0;
	memcpy(&strTimer,ptr,sizeof(struct Timer_struct));
	free(ptr);
	ptr = NULL;
	dcs_log(0, 0, "key=%s 重发foldid=%d次数为%d,超时时间为%d",strTimer.key,strTimer.foldid,strTimer.sendnum,strTimer.timeout);
	//测试
	//org_folderId = fold_locate_folder("TIMETEST");
	for(i = 0 ; i<strTimer.sendnum; i ++)
	{
		sleep(strTimer.timeout);//开始休息
		if(fold_write(strTimer.foldid, org_folderId, strTimer.data, strTimer.length) < 0)
		{
			dcs_log(0 , 0, "fold_write key=%s fail",strTimer.key);
		}
	}
	dcs_debug(strTimer.data, strTimer.length, "key=%s 重发次数用完",strTimer.key);
	Delete_Key_Control(strTimer.key,0);
	Print_Key_Control();
}


/*初始化key控制链表
*/
void Init_key_Control()
{
	head = NULL;
	tail = NULL;
}

/*在链表的最后添加一个新的key信息
*/
int Add_Key_Control(char *key, pthread_t pid)
{
	struct Key_struct *ptr = NULL;
	ptr =(struct Key_struct *)malloc(sizeof(struct Key_struct));
	if( ptr == NULL )
	{
		dcs_log(0, 0, "malloc fail %s",strerror(errno));
		return -1;
	}

	strcpy(ptr->key,key);
	ptr->pid = pid;
	ptr->next = NULL;
	//加线程锁
	pthread_mutex_lock(&work_mutex);
	if(head == NULL)
	{
		head = ptr;
		tail =ptr;
	}
	else
	{
		tail->next = ptr;
		tail = tail ->next;
	}
	
	pthread_mutex_unlock(&work_mutex);
	return 0;
}

/*删除链表中一个key的信息
参数:key  为要删除的节点的key值;
	     falg 为是否调用取消线程的函数,flag =1 表示需要调用.其他值表示线程本身调用该函数无需取消
*/
int Delete_Key_Control(char *key,int flag)
{
	struct Key_struct *ptr = NULL,*previous = NULL;
	//加线程锁
	pthread_mutex_lock(&work_mutex);
	if(head == NULL)
	{
		dcs_log(0, 0, "没有该key %s的信息",key);
		pthread_mutex_unlock(&work_mutex);
		return -1;
	}
	else
	{
		ptr = head;
		previous = head;
		while(ptr != NULL)
		{
			if(strcmp(ptr->key,key) == 0)
			{
				if(flag == 1)
					pthread_cancel(ptr->pid);//结束线程
					
				if(ptr == head)
					head = head->next;
				else if(ptr == tail)
				{
					tail = previous;
					tail->next = NULL;
				}
				else
					previous->next=ptr->next;	
				
				free(ptr);
				pthread_mutex_unlock(&work_mutex);
				return 0;
			}
			
			if(ptr != head )
				previous = previous ->next;
			ptr = ptr->next;
		}
		pthread_mutex_unlock(&work_mutex);
		dcs_log(0, 0, "没有该key %s的信息",key);
		return -1;
	}
}


/*清空链表
*/
int Empty_Key_Control()
{
	struct Key_struct *ptr = NULL;
	//加线程锁
	pthread_mutex_lock(&work_mutex);
	if(head == NULL )
	{
		pthread_mutex_unlock(&work_mutex);
		return 0;
	}
	ptr = head;
	while(ptr->next)
	{
		head = head->next;
		free(ptr);
		ptr= head;
	}
	free(ptr);
	pthread_mutex_unlock(&work_mutex);
	return 0;
}

int Check_Process_IsExist_ByName( char * processName)
{
     FILE* fp;
     int count;
     char buf[10];
     char command[80];
     memset(buf, 0, sizeof(buf));
     memset(command, 0, sizeof(command));
     sprintf(command, "pgrep -x %s|wc -l",processName);

     if((fp = popen(command,"r")) == NULL)
     {
          dcs_log(0,0,"<Check_Queue>popen error %s",strerror(errno));
          return -1;
     }
     if((fgets(buf,10,fp))!= NULL )
     {
          count = atoi(buf);
          if(count > 1)
          {
               dcs_log(0,0,"<Check_Queue>process %s is  exist!\n",processName);
               pclose(fp);
               return 0;
          }
          else
          {
               pclose(fp);
               return 1;
          }
     }
     pclose(fp);
     return -1;
}


