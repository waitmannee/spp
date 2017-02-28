//
//  util_function.c
//  iPrepose
//
//  Created by wu on 14-11-26.
//  
#include "unixhdrs.h"
#include "extern_function.h"

/*
*	启动一个子进程，参数为strExeFile,arg1
*/
int Start_Process(const char * strExeFile, const char * arg1)
{
	int nRet,pidchild = -1;
	char path[512];
	pidchild= fork();

	if(pidchild< 0)
	{
		dcs_log(0,0,"cannot fork child  '%s'!\n",strExeFile);
		return -1;
	}

	if(pidchild >0)
	{
		dcs_log(0,0,"going to load process '%s' ...",strExeFile);
		return pidchild;
	}
	//child her
	memset(path,0x00,sizeof(path));
	
	sprintf(path,"%s/run/bin/%s",getenv("EXECHOME"),strExeFile);
	dcs_log(0,0,"exec %s:%s",path,arg1);
	nRet =execlp(path,strExeFile,arg1,(char *)0);
	if(nRet< 0)
    	{
       	dcs_log(0,0,"cannot exec executable '%s':%s!",path,strerror(errno));
        	exit(249);
    	}

   	 return 0;	
}

/*
 * 根据进程id检查进程是否存在
 * 返回值: 0:存在; -1:不存在
 */
int check_process_exist(int pid)
{
	if (kill(pid, 0) == 0)
	{ /* process is running or a zombie */
		return 0;
	}
	else if (errno == ESRCH)
	{ /* no such process with the given pid is running */
		return -1;
	}
	else
	{  /* some other error... use perror("...") or strerror(errno) to report */
		dcs_log(0,0,"some other error.. %s\n",strerror(errno));
		return -2;
	}
	return -1;
}


/*
*函数功能:循环检查队列id为*qid的队列为中有无无主信息
*  通过获取队列中第一个信息的megtype(即信息拥有者的进程号)来查询进程是否存在
*/
void *Check_Queue(void * qid)
{
	struct msqid_ds info;
	long msgtype = 0;
	char buf[24000];
	int ret= 0 , len = 0;
	int queue_id = *(int *)qid;
	while(1)
	{
		sleep(1); /*每秒查询一次*/
		memset(&info, 0, sizeof(struct msqid_ds));
		//读取队列属性
		if (msgctl(queue_id, IPC_STAT, &info) != 0)
		{
			dcs_log(0,0,"<Check_Queue>msgctl IPC_STAT error[%s] ",strerror(errno));
		}
		//先设置为4个
		if(info.msg_qnum > 4)
		{	
			//获取队列中第一条消息的msgtype,并判断进程号为msgtype的进程是否存在
			msgtype = 0;
			len = queue_recv_by_msgtype(queue_id, buf, sizeof(buf), &msgtype, 1);
			if(len == -1)
			{
				dcs_log(0,0,"<Check_Queue>获取第一条信息失败");
				continue;
			}
			ret = check_process_exist(msgtype);
			if(ret == 0)/*进程仍然存在,将消息重新写回队列*/
			{
				dcs_log(buf,len ,"<Check_Queue>进程[%d] 仍然存在,将消息重新写回队列",msgtype);
				queue_send_by_msgtype(queue_id, buf,len, msgtype, 0);
			}
			else /*进程已不存在*/
			{
				dcs_debug(buf,len ,"<Check_Queue>移除队列中msgtype =[%d] 的消息,长度为%d ",msgtype,len);
			}
		}
	}	
}


/**************************************************************
功能：从字符串src中分析出网站地址和端口，并得到对象路径
***************************************************************/
void GetHost(char * src, char * web, char * file, int * port) 
{
	char * pA;
	char * pB;
	
	*port = 0;
	if(!(*src)) return;
	pA = src;
	if(!strncmp(pA, "http://", strlen("http://"))) 
		pA = src+strlen("http://");
	else if(!strncmp(pA, "https://", strlen("https://"))) 
		pA = src+strlen("https://");
	pB = strchr(pA, '/');
	if(pB) 
	{
		memcpy(web, pA, strlen(pA) - strlen(pB));
		if(pB+1) 
		{
			memcpy(file, pB + 1, strlen(pB) - 1);
			file[strlen(pB) - 1] = 0;
		}
	}
	else 
		memcpy(web, pA, strlen(pA));
	
	if(pB) 
		web[strlen(pA) - strlen(pB)] = 0;
	else 
		web[strlen(pA)] = 0;
	
	pA = strchr(web, ':');
	
	if(pA)
	{
		*port = atoi(pA + 1);
		*pA =0;
		//web[strlen(web)-strlen(pA)] =0;
	}
	else 
		*port = 80;
}



