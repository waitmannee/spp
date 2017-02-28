 /*  
 *程序名称：syn_httpc_cmd.c
 *功能描述：
 *用于http同步通讯客户端各个子程序的管理
*/
#include "syn_httpc.h"

static struct httpc_Ast *s_http_Ctrl = NULL;
static void Syn_Httpc_Cmd_Exit(int signo);
static int Add_Syn_Httpc_Link();
static void List_Syn_Httpc_Info(int arg_c, char *arg_v[]);
static int Reload_Syn_Httpc_Cfg(struct httpc_Ast *pLink, char *pname, int mode);
static int Stop_Syn_Httpc_Link();
static int Get_Syn_Httpc_Link(char *pname, int *pFlag);
static int Open_Syn_Httpc_Cmd_Log();
static int Start_Syn_Httpc_Link();
static int Update_Syn_Httpc_Link();
//==========================================

int main(int argc, char *argv[])
{
	catch_all_signals(Syn_Httpc_Cmd_Exit);

	if(Open_Syn_Httpc_Cmd_Log()< 0)
		return -1;
	
	if(s_http_Ctrl == NULL)
	{ 
		s_http_Ctrl = (struct httpc_Ast*)shm_connect(SYN_HTTPC_SHM_NAME, NULL);
		if(s_http_Ctrl == NULL)
		{
		    dcs_log(0, 0, "cannot connect SHM '%s':%s!\n", SYN_HTTPC_SHM_NAME, strerror(errno));
		    return -1;
		}
	}

	cmd_init("lsynHttpCCmd>>", Syn_Httpc_Cmd_Exit);
	cmd_add("list", (cmdfunc_t *)List_Syn_Httpc_Info, "list link info");
	cmd_add("start", (cmdfunc_t *)Start_Syn_Httpc_Link, "start a link");
	cmd_add("add", (cmdfunc_t *)Add_Syn_Httpc_Link, "Add a link ");
	cmd_add("stop", (cmdfunc_t *)Stop_Syn_Httpc_Link, "stop a link");
	cmd_add("update", (cmdfunc_t *)Update_Syn_Httpc_Link, "update a link");
	
	cmd_shell(argc, argv);

	Syn_Httpc_Cmd_Exit(0);
	return 0;
}


static void Syn_Httpc_Cmd_Exit(int signo)
{
	shm_detach((char *)s_http_Ctrl);
	exit(signo);
}

static void List_Syn_Httpc_Info(int arg_c, char *arg_v[])
{
	int i = 0;
	char sBuf[200];
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf, sizeof(sBuf), stdin);

	printf("maxNum=%d\n", s_http_Ctrl->i_conn_num);
	printf("id    RecvQueue        SendQueue     Status   ServerAddr   timeOut    maxThreadNum\n");
	printf("==================================================================================\n");
	for( ; i<s_http_Ctrl->i_conn_num; i++)
	{
		if(strlen(s_http_Ctrl->httpc[i].recv_queue) > 0)
		{
			printf("[%03d] ", i+1);
			printf("%-16s", s_http_Ctrl->httpc[i].recv_queue);
			printf("%-16s", s_http_Ctrl->httpc[i].send_queue);
			
			//link Status
			if((s_http_Ctrl->httpc[i].iStatus == DCS_STAT_RUNNING))
				printf("%-8s", "running");
			else if(s_http_Ctrl->httpc[i].iStatus ==DCS_STAT_STOPPED)
				printf("%-8s", "stoped");
			else
				printf("%s ", " ");

			printf("%-30s ", s_http_Ctrl->httpc[i].serverAddr);
			printf("%-3d  ", s_http_Ctrl->httpc[i].timeout);
			printf("%-3d", s_http_Ctrl->httpc[i].maxthreadnum);
			printf("\n");
		}
	}
	return ;
}

static int Add_Syn_Httpc_Link()
{
	char queue_name[20+1];
	char sBuf[200];
	int nRet = 0, nFlag = 0;
	
	memset(queue_name, 0, sizeof(queue_name));
	scanf("%s", queue_name);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf, sizeof(sBuf), stdin);
	
	dcs_log(0, 0, "添加Http同步通讯服务, queue_name=[%s]", queue_name);
	//先查找是否是已存在的链路
	nRet = Get_Syn_Httpc_Link(queue_name, &nFlag);
	if(nRet >=0)
	{
		dcs_log(0, 0, "Http同步通讯服务已经存在, queue_name=[%s]", queue_name);
		printf("Http同步通讯服务已经存在, queue_name=[%s]\n", queue_name);
		return -1;
	}
	//不存在的话，从配置文件中 读出配置信息
	nRet = Reload_Syn_Httpc_Cfg(s_http_Ctrl, queue_name, 1);
	if(nRet <0)
	{
		return -1;
	}

	nRet = Start_Process("lsynHttpC", queue_name);
	if(nRet < 0)
	{
		dcs_log(0, 0, "Http同步通讯服务queue_name=[%s]添加失败", queue_name);
		printf("Http同步通讯服务queue_name=[%s]添加失败\n", queue_name);
		return -1;
	}
	dcs_log(0, 0, "Http同步通讯服务queue_name=[%s]添加成功", queue_name);
	return 0;
}


/*重新导入配置文件
*mode =1表示是在共享内存的后面添加接收队列名为pname的新纪录,
*mode =0 表示更新共享内存中接收队列名为pname的已有记录的内容
*/
static int Reload_Syn_Httpc_Cfg(struct httpc_Ast *pLink, char *pname, int mode)
{
	char caBuffer[512];
	int iFd=-1, iRc = 0, iSlotIndex = 0, iCnt = 0;
	char cfgfile[256];
	int nFlag=0;
	
	char *recv_queue = NULL,*send_queue = NULL,*server_address = NULL, * timeout = NULL, *MaxthreadNum = NULL;

	memset(caBuffer, 0, sizeof(caBuffer));
	memset(cfgfile, 0, sizeof(cfgfile));
	if(u_fabricatefile("config/lsynhttpc.conf", cfgfile, sizeof(cfgfile)) < 0)
	{
		dcs_log(0, 0, "cannot get full path of 'lsynhttpc.conf'\n");
		return -1;
	}

	dcs_log(0, 0, "full path name cfgfile:%s", cfgfile);

	if(0 > (iFd = conf_open(cfgfile)) )
	{
		dcs_log(0, 0, "open file '%s' failed.", cfgfile);
		return -1;
	}

	//读取配置项中的配置项,不可去掉
	if(0 > conf_get_first_number(iFd, "conn.max", &iCnt) || iCnt < 0 )
	{
		dcs_log(0, 0, "<lsynhttpcmsg> load config item 'conn.max' failed!");
		conf_close(iFd);
		return -1;
	}

	iRc = conf_get_first_string(iFd, "conn", caBuffer);
	dcs_log(0, 0, "conf_get_first_string conn = %d, caBuffer = %s", iRc, caBuffer);

	//mode =1 时需要在共享内存的最后添加内容，所以要找到最后一条空记录
	if(mode == 1)
	{
		for( ; iSlotIndex<s_http_Ctrl->i_conn_num; iSlotIndex++)
		{
			if(strlen(s_http_Ctrl->httpc[iSlotIndex].recv_queue) == 0)
			{
				break;
			}
		}
		if(iSlotIndex ==s_http_Ctrl->i_conn_num )
		{
			dcs_log(0,0,"共享内存没有空余空间存储新的配置信息,最多%d条",iSlotIndex);
			printf("共享内存没有空余空间存储新的配置信息,最多%d条\n",iSlotIndex);
			return -1;
		}
	}
	

	for( ; iRc == 0 && iSlotIndex < s_http_Ctrl->i_conn_num; iRc = conf_get_next_string(iFd, "conn", caBuffer))
	{
		//接收队列
		recv_queue = strtok(caBuffer, " \t\r");
		//返回队列
		send_queue = strtok(NULL, " \t\r");
		//服务器地址
		server_address = strtok(NULL, " \t\r");
		//超时时间
		timeout      = strtok(NULL, " \t\r");
		//设置每条短链最大的线程数
		MaxthreadNum = strtok(NULL, "  \t\r");

		if(strcmp(recv_queue, pname) != 0)
		{
			continue;
		}
		//如果是更新,需要找到共享内存中的位置
		if(mode == 0)
		{
			iSlotIndex = Get_Syn_Httpc_Link(pname,&iRc);
		}

		nFlag =1;//找到配置

		dcs_log(0, 0, "recv_queue = [%s], send_queue = [%s], server_address = [%s], timeout = [%s], MaxthreadNum =[%s]",
			recv_queue, send_queue, server_address, timeout, MaxthreadNum);

		if (recv_queue == NULL || send_queue == NULL || server_address == NULL || timeout == NULL || MaxthreadNum == NULL )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}
		
		strcpy(s_http_Ctrl->httpc[iSlotIndex].recv_queue, recv_queue);
		strcpy(s_http_Ctrl->httpc[iSlotIndex].send_queue, send_queue);
		strcpy(s_http_Ctrl->httpc[iSlotIndex].serverAddr, server_address);

		s_http_Ctrl->httpc[iSlotIndex].timeout= atoi(timeout);
		s_http_Ctrl->httpc[iSlotIndex].maxthreadnum = atoi(MaxthreadNum);

		dcs_log(0, 0, "s_http_Ctrl->linkArray[%d].name = %s", iSlotIndex, s_http_Ctrl->httpc[iSlotIndex].recv_queue);

		iSlotIndex++;
	}
	dcs_log(0, 0, "load config end");
	conf_close(iFd);
	if(nFlag == 0)
	{
		dcs_log(0, 0, "queue_name=[%s]配置未找到", pname);
		printf("queue_name=[%s]配置未找到\n", pname);
		return -1;
	}
	return 0;
}


/*关闭相关短链服务
*/
int Stop_Syn_Httpc_Link()
{
	char queue_name[20+1];
	char sBuf[200];
	int nRet = 0;
	
	memset(queue_name, 0, sizeof(queue_name));
	int nFlag = -1;
	
	scanf("%s", queue_name);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf, sizeof(sBuf), stdin);

	dcs_log(0, 0, "停止[%s]服务", queue_name);
	nRet = Get_Syn_Httpc_Link(queue_name, &nFlag);
	if(nRet == -1)
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务不存在", queue_name);
		printf("Http 同步通讯[%s]服务不存在\n", queue_name);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务已是停止状态", queue_name);
		printf("Http同步通讯[%s]服务已是停止状态\n", queue_name);
		return 0;
	}
	//链路存在,向链路进程发送终止信号
	dcs_log(0, 0, "kill 进程[%d].", s_http_Ctrl->httpc[nRet].pidComm);
	nRet = kill(s_http_Ctrl->httpc[nRet].pidComm, SIGUSR2);
	if(nRet == 0)
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务关闭成功", queue_name);
	}
	else
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务关闭失败, err:%s", queue_name, strerror(errno));
		printf("Http同步通讯[%s]服务关闭失败, err:%s\n", queue_name, strerror(errno));
	}
	
	return 0;
}
/*获取链路状态
*输入参数:客户端的FoldName
*输出参数: 如果找到FoldName的链路，将该链路状态放在pFlag
* 返回值: 当找到需要的链路时返回其在共享内存中的位置，否则返回-1
*/
int Get_Syn_Httpc_Link(char *pname, int *pFlag)
{
	int i = 0;
	
	for( ;i<s_http_Ctrl->i_conn_num; i++)
	{ 
		if(strcmp(s_http_Ctrl->httpc[i].recv_queue, pname) == 0)
		{
			*pFlag = s_http_Ctrl->httpc[i].iStatus;
			return i;
		}	
	}
	return -1;
}


static int Open_Syn_Httpc_Cmd_Log()
{
    char logfile[256];
    memset(logfile, 0, sizeof(logfile));
	
    if(u_fabricatefile("log/lsynHttpCCmd.log", logfile, sizeof(logfile)) < 0)
        return -1;

    return dcs_log_open(logfile, "lsynHttpCCmd");
}


int  Start_Syn_Httpc_Link()
{
	char queue_name[20+1];
	char sBuf[200];
	int nRet = -1;
	int nFlag=-1;
	memset(queue_name,0,sizeof(queue_name));
	
	scanf("%s",queue_name);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf, sizeof(sBuf), stdin);

	dcs_log(0, 0, "启动Http同步通讯[%s]", queue_name);
	nRet = Get_Syn_Httpc_Link(queue_name, &nFlag);
	if(nRet == -1)
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务不存在", queue_name);
		printf("Http同步通讯[%s]服务不存在\n", queue_name);
		return 0;
	}
	else if(nFlag !=DCS_STAT_STOPPED)
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务已经在运行", queue_name);
		printf("Http 同步通讯[%s]服务已经在运行\n", queue_name);
		return 0;
	}
	nRet=Start_Process("lsynHttpC", queue_name);
	if(nRet < 0)
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务启动失败", queue_name);
		printf("Http同步通讯[%s]服务启动失败\n", queue_name);
	}
	dcs_log(0, 0, "Http同步通讯[%s]服务启动成功", queue_name);
	return 0;
}

/*更新相关短链服务
*/
int Update_Syn_Httpc_Link()
{
	char queue_name[20+1];
	char sBuf[200];
	int nRet = 0, nPid = 0;
	
	memset(queue_name, 0, sizeof(queue_name));
	int nFlag=-1;
	
	scanf("%s",queue_name);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf, sizeof(sBuf), stdin);

	dcs_log(0, 0, "更新Http同步通讯[%s]服务", queue_name);
	nRet = Get_Syn_Httpc_Link(queue_name, &nFlag);
	if(nRet == -1)
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务不存在", queue_name);
		printf("Http同步通讯[%s]服务不存在\n", queue_name);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务已是停止状态", queue_name);
		printf("Http同步通讯[%s]服务已是停止状态\n", queue_name);
		return 0;
	}

	//链路存在,向链路进程发送终止信号
	dcs_log(0, 0, "kill 进程[%d].", s_http_Ctrl->httpc[nRet].pidComm);
	nRet=kill(s_http_Ctrl->httpc[nRet].pidComm, SIGUSR2);
	if(nRet == 0)
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务关闭成功", queue_name);
	}
	else
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务关闭失败, err:%s", queue_name, strerror(errno));
		printf("Http同步通讯[%s]服务关闭失败,err:%s\n", queue_name, strerror(errno));
		return 0;
	}
	
	//重新从配置文件中 读出相应配置信息
	nRet=Reload_Syn_Httpc_Cfg(s_http_Ctrl, queue_name, 0);
	if(nRet <0)
	{
		dcs_log(0, 0, "更新Http同步通讯[%s]服务失败", queue_name);
		printf("更新Http同步通讯%s]服务失败\n", queue_name);
		return -1;
	}	
	sleep(1);// 等待一秒，等待kill信号量先被处理
	nPid=Start_Process("lsynHttpC", queue_name);
	if(nPid < 0)
	{
		dcs_log(0, 0, "Http同步通讯[%s]服务启动失败", queue_name);
		printf("Http同步通讯[%s]服务启动失败\n", queue_name);
	}
	
	dcs_log(0, 0, "更新Http同步通讯[%s]服务成功", queue_name);
	return 0;
}



