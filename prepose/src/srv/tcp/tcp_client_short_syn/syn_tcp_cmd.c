 /*  
 *程序名称：syn_tcp_cmd.c
 *功能描述：
 *用于tcp同步通讯客户端各个子程序的管理
*/
#include "syn_tcp.h"

static struct tcp_Ast *s_Link_Ctrl = NULL;
static void Syn_Tcp_Cmd_Exit(int signo);
static int Add_Syn_Tcp_Link();
static void List_Syn_Tcp_Info(int arg_c, char *arg_v[]);
static int Reload_Syn_Tcp_Cfg(struct tcp_Ast *pLink,char *pname,int mode);
static int Stop_Syn_Tcp_Link();
static int Get_Syn_Tcp_Link(char *pname,int *pFlag);
static int Open_Syn_Tcp_Cmd_Log();
static int  Start_Syn_Tcp_Link();
static int Update_Syn_Tcp_Link();
//==========================================

int main(int argc, char *argv[])
{
	catch_all_signals(Syn_Tcp_Cmd_Exit);

	if(Open_Syn_Tcp_Cmd_Log()< 0)
		return -1;
	
	if(s_Link_Ctrl == NULL)
	{ 
		s_Link_Ctrl = (struct tcp_Ast*)shm_connect(SYN_TCP_SHM_NAME, NULL);
		if(s_Link_Ctrl == NULL)
		{
		    dcs_log(0, 0, "cannot connect SHM '%s':%s!\n", SYN_TCP_SHM_NAME, strerror(errno));
		    return -1;
		}
	}

	cmd_init("synTcpCmd>>",Syn_Tcp_Cmd_Exit);
	cmd_add("list", (cmdfunc_t *)List_Syn_Tcp_Info, "list link info");
	cmd_add("start", (cmdfunc_t *)Start_Syn_Tcp_Link, "start a link");
	cmd_add("add", (cmdfunc_t *)Add_Syn_Tcp_Link, "Add a link ");
	cmd_add("stop", (cmdfunc_t *)Stop_Syn_Tcp_Link, "stop a link");
	cmd_add("update", (cmdfunc_t *)Update_Syn_Tcp_Link, "update  a link");
	
	cmd_shell(argc,argv);

	Syn_Tcp_Cmd_Exit(0);
	return 0;
}


static void Syn_Tcp_Cmd_Exit(int signo)
{
	shm_detach((char *)s_Link_Ctrl);
	exit(signo);
}

static void List_Syn_Tcp_Info(int arg_c, char *arg_v[])
{
	int i = 0;
	char sBuf[200]={0};
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	printf("maxNum=%d\n",s_Link_Ctrl->i_conn_num);
	printf("id    RecvQueue        SendQueue     Status   ServerAddr   timeOut    maxThreadNum\n");
	printf("==================================================================================\n");
	for(i = 0; i<s_Link_Ctrl->i_conn_num; i++)
	{
		if(strlen(s_Link_Ctrl->tcpc[i].recv_queue) > 0)
		{
			printf("[%03d] ", i+1);
			printf("%-16s", s_Link_Ctrl->tcpc[i].recv_queue);
			printf("%-16s", s_Link_Ctrl->tcpc[i].send_queue);
			
			//link Status
			if((s_Link_Ctrl->tcpc[i].iStatus == DCS_STAT_RUNNING ))
				printf("%-8s ", "running");
			else if(s_Link_Ctrl->tcpc[i].iStatus ==DCS_STAT_STOPPED)
				printf("%-8s ", "stoped");
			else
				printf("%s ", " ");

			printf("%15s:%-5d  ", s_Link_Ctrl->tcpc[i].server_ip,s_Link_Ctrl->tcpc[i].server_port);
			printf("%-3d  ", s_Link_Ctrl->tcpc[i].timeout);
			printf("%-3d", s_Link_Ctrl->tcpc[i].maxthreadnum);
			printf("\n");
		}
	}
	return ;
}

static int Add_Syn_Tcp_Link()
{
	char queue_name[20+1];
	char sBuf[200]={0};
	int nRet = 0,nFlag= 0;
	
	memset(queue_name, 0, sizeof(queue_name));
	scanf("%s", queue_name);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);
	
	dcs_log(0,0,"添加Tcp 同步通讯服务,queue_name=[%s]",queue_name);
	//先查找是否是已存在的链路
	nRet = Get_Syn_Tcp_Link(queue_name,&nFlag);
	if(nRet >=0)
	{
		dcs_log(0,0,"Tcp 同步通讯服务已经存在,queue_name=[%s]",queue_name);
		printf("Tcp 同步通讯服务已经存在,queue_name=[%s]\n",queue_name);
		return -1;
	}

	//不存在的话，从配置文件中 读出配置信息
	nRet=Reload_Syn_Tcp_Cfg(s_Link_Ctrl,queue_name,1);
	if(nRet <0)
	{
		return -1;
	}

	nRet=Start_Process("synTcp",queue_name);
	if(nRet < 0)
	{
		dcs_log(0,0,"Tcp 同步通讯服务queue_name=[%s]添加失败",queue_name);
		printf("Tcp 同步通讯服务queue_name=[%s]添加失败\n",queue_name);
		return -1;
	}
	dcs_log(0,0,"Tcp 同步通讯服务queue_name=[%s]添加成功",queue_name);
	return 0;
}


/*重新导入配置文件
*mode =1表示是在共享内存的后面添加接收队列名为pname的新纪录,
*mode =0 表示更新共享内存中接收队列名为pname的已有记录的内容
* 返回值: 操作失败-1 ,成功时返回该条记录在共享内存中的位置
*/
static int Reload_Syn_Tcp_Cfg(struct tcp_Ast *pLink,char *pname,int mode)
{
	char   caBuffer[512];
	int    iFd=-1,iRc = 0,iSlotIndex = 0,iCnt = 0;
	char cfgfile[256];
	int nFlag=0 ;
	
	char *commType = NULL,*recv_queue = NULL,*send_queue = NULL,*server_address = NULL, * timeout = NULL, *MaxthreadNum = NULL;
	char server_ip[67];
	char server_port[6];

	if(u_fabricatefile("config/syn_tcp.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'syn_tcp.conf'\n");
		return -1;
	}

	dcs_log(0,0,"cfgfile full path name %s\n", cfgfile);

	if(0 > (iFd = conf_open(cfgfile)) )
	{
		dcs_log(0,0,"open file '%s' failed.\n",cfgfile);
		return -1;
	}

	//读取配置项中的配置项,不可去掉
	if(0 > conf_get_first_number(iFd,"conn.max",&iCnt)|| iCnt < 0 )
	{
		dcs_log(0,0,"<dcmhstsvr> load config item 'conn.max' failed!");
		conf_close(iFd);
		return -1;
	}

	iRc = conf_get_first_string(iFd, "conn",caBuffer);
	dcs_log(0, 0, "conf_get_first_string conn = %d, caBuffer = %s", iRc, caBuffer);

	//mode =1 时需要在共享内存的最后添加内容，所以要找到最后一条空记录
	if(mode == 1)
	{
		for(iSlotIndex=0;iSlotIndex<s_Link_Ctrl->i_conn_num;iSlotIndex++)
		{
			if(strlen(s_Link_Ctrl->tcpc[iSlotIndex].recv_queue) == 0)
			{
				break;
			}
		}
		if(iSlotIndex ==s_Link_Ctrl->i_conn_num )
		{
			dcs_log(0,0,"共享内存没有空余空间存储新的配置信息,最多%d条",iSlotIndex);
			printf("共享内存没有空余空间存储新的配置信息,最多%d条\n",iSlotIndex);
			return -1;
		}
	}
	
	for( ;iRc == 0 && iSlotIndex < s_Link_Ctrl->i_conn_num;iRc = conf_get_next_string(iFd, "conn",caBuffer))
	{
		//通讯模式
		commType = strtok(caBuffer," \t\r");
		//接收队列
		recv_queue = strtok(NULL," \t\r");
		//返回队列
		send_queue = strtok(NULL," \t\r");
		//服务器地址
		server_address    = strtok(NULL," \t\r");
		//超时时间
		timeout      = strtok(NULL, " \t\r");
		//设置每条短链最大的线程数
		MaxthreadNum = strtok(NULL , "  \t\r");

		memset(server_ip, 0x00, strlen(server_ip));
		memset(server_port, 0x00, sizeof(server_port));
		util_split_ip_with_port_address(server_address, server_ip, server_port);

		if(strcmp(recv_queue, pname) != 0)
		{
			continue;
		}
		//如果是更新,需要找到共享内存中的位置
		if(mode == 0)
		{
			iSlotIndex =Get_Syn_Tcp_Link(pname,&iRc);
		}
		
		nFlag =1;//找到配置

		dcs_log(0, 0, "commType = [%s], recv_queue = [%s], send_queue = [%s],server_ip = [%s], server_port = [%s],timeout = [%s], MaxthreadNum =[%s]\n",
			commType,recv_queue,send_queue,server_ip, server_port, timeout, MaxthreadNum);

		if (commType == NULL || recv_queue == NULL || send_queue == NULL || server_address == NULL || 
			timeout == NULL || MaxthreadNum == NULL )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}
		
		strcpy(s_Link_Ctrl->tcpc[iSlotIndex].commType,commType);
		strcpy(s_Link_Ctrl->tcpc[iSlotIndex].recv_queue,recv_queue);
		strcpy(s_Link_Ctrl->tcpc[iSlotIndex].send_queue,send_queue);
		strcpy(s_Link_Ctrl->tcpc[iSlotIndex].server_ip,server_ip);

		s_Link_Ctrl->tcpc[iSlotIndex].server_port = atoi(server_port);
		s_Link_Ctrl->tcpc[iSlotIndex].timeout= atoi(timeout);
		s_Link_Ctrl->tcpc[iSlotIndex].maxthreadnum = atoi(MaxthreadNum);

		dcs_log(0, 0, "s_Link_Ctrl->linkArray[%d].name = %s", iSlotIndex, s_Link_Ctrl->tcpc[iSlotIndex].recv_queue);
		conf_close(iFd);
		return iSlotIndex;
	}
	dcs_log(0,0," load config end");
	conf_close(iFd);
	if(nFlag == 0)
	{
		dcs_log(0,0,"queue_name=[%s]配置未找到",pname);
		printf("queue_name=[%s]配置未找到\n",pname);
		return  -1;
	}
	return 0;
}


/*关闭相关短链服务
*/
int Stop_Syn_Tcp_Link()
{
	char queue_name[20+1];
	char sBuf[200]={0};
	int nRet = 0;
	
	memset(queue_name, 0, sizeof(queue_name));
	int nFlag=-1;
	
	scanf("%s",queue_name);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	dcs_log(0,0,"停止[%s] 服务",queue_name);
	nRet = Get_Syn_Tcp_Link(queue_name,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务不存在",queue_name);
		printf("Tcp 同步通讯服务不存在\n",queue_name);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务已是停止状态",queue_name);
		printf("Tcp 同步通讯[%s] 服务已是停止状态\n",queue_name);
		return 0;
	}
	//链路存在,向链路进程发送终止信号
	dcs_log(0,0,"kill 进程[%d].",s_Link_Ctrl->tcpc[nRet].pidComm);
	nRet=kill(s_Link_Ctrl->tcpc[nRet].pidComm,SIGUSR2);
	if(nRet == 0)
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务关闭成功",queue_name);
	}
	else
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务关闭失败,err:%s",queue_name,strerror(errno));
		printf("Tcp 同步通讯[%s] 服务关闭失败,err:%s\n",queue_name,strerror(errno));
	}
	
	return 0;
}
/*获取链路状态
*输入参数:客户端的FoldName
*输出参数: 如果找到FoldName的链路，将该链路状态放在pFlag
* 返回值: 当找到需要的链路时返回其在共享内存中的位置，否则返回-1
*/
int Get_Syn_Tcp_Link(char *pname,int *pFlag)
{
	int i;
	
	for(i=0;i<s_Link_Ctrl->i_conn_num;i++)
	{ 
		if(strcmp(s_Link_Ctrl->tcpc[i].recv_queue, pname) == 0)
		{
			*pFlag = s_Link_Ctrl->tcpc[i].iStatus;
			return i;
		}	
	}
	return -1;
}


static int Open_Syn_Tcp_Cmd_Log()
{
    char logfile[256];
    memset(logfile, 0, sizeof(logfile));
    if(u_fabricatefile("log/synTcpCmd.log",logfile,sizeof(logfile)) < 0)
        return -1;

    return dcs_log_open(logfile, "synTcpCmd");
}


int  Start_Syn_Tcp_Link()
{
	char queue_name[20+1];
	char sBuf[200]={0};
	int nRet = -1;
	int nFlag=-1,nPid=0;
	memset(queue_name,0,sizeof(queue_name));
	
	scanf("%s",queue_name);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	dcs_log(0,0,"启动Tcp 同步通讯[%s] 服务",queue_name);
	nRet = Get_Syn_Tcp_Link(queue_name,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务不存在",queue_name);
		printf("Tcp 同步通讯[%s] 服务不存在\n",queue_name);
		return 0;
	}
	else if(nFlag !=DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务已经在运行",queue_name);
		printf("Tcp 同步通讯[%s] 服务已经在运行\n",queue_name);
		return 0;
	}
	nRet=Start_Process("synTcp",queue_name);
	if(nRet < 0)
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务启动失败",queue_name);
		printf("Tcp 同步通讯[%s] 服务启动失败\n",queue_name);
	}
	dcs_log(0,0,"Tcp 同步通讯[%s] 服务启动成功",queue_name);
	return 0;
}

/*更新相关短链服务
*/
int Update_Syn_Tcp_Link()
{
	char queue_name[20+1];
	char sBuf[200]={0};
	int nRet = 0;
	int nFlag=-1;

	memset(queue_name, 0, sizeof(queue_name));
	scanf("%s",queue_name);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	dcs_log(0,0,"更新Tcp 同步通讯[%s] 服务",queue_name);
	nRet = Get_Syn_Tcp_Link(queue_name,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务不存在",queue_name);
		printf("Tcp 同步通讯[%s] 服务不存在\n",queue_name);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务已是停止状态",queue_name);
		printf("Tcp 同步通讯[%s] 服务已是停止状态\n",queue_name);
		return 0;
	}

	//链路存在,向链路进程发送终止信号
	dcs_log(0,0,"kill 进程[%d].",s_Link_Ctrl->tcpc[nRet].pidComm);
	nRet=kill(s_Link_Ctrl->tcpc[nRet].pidComm,SIGUSR2);
	if(nRet == 0)
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务关闭成功",queue_name);
	}
	else
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务关闭失败,err:%s",queue_name,strerror(errno));
		printf("Tcp 同步通讯[%s] 服务关闭失败,err:%s\n",queue_name,strerror(errno));
		return -1;
	}
	
	//重新从配置文件中 读出相应配置信息
	nRet=Reload_Syn_Tcp_Cfg(s_Link_Ctrl,queue_name,0);
	if(nRet <0)
	{
		dcs_log(0,0,"更新Tcp 同步通讯[%s] 服务失败",queue_name);
		printf("更新Tcp 同步通讯%s] 服务失败\n",queue_name);
		return -1;
	}	
	sleep(1);// 等待一秒，等待kill信号量先被处理
	nRet=Start_Process("synTcp",queue_name);
	if(nRet < 0)
	{
		dcs_log(0,0,"Tcp 同步通讯[%s] 服务启动失败",queue_name);
		printf("Tcp 同步通讯[%s] 服务启动失败\n",queue_name);
	}	
	dcs_log(0,0,"更新Tcp 同步通讯[%s] 服务成功",queue_name);
	return 0;
}



