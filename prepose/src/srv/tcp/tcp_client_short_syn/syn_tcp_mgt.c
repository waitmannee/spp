/*  
 *文件名称：syn_tcp_mgt.c
 *功能描述：
 *TCP协议的客户端短链同步服务启动进程,用于初始化共享内存，
 * 根据配置文件syn_tcp.conf 启动各个服务进程 synTcp
*/
#include "syn_tcp.h"

static struct tcp_Ast *sp_syntcp_ctrl = NULL;
//======= Funtion Forward delection =============//
static int Open_Syn_Tcp_Mgt_Log(char *IDENT);
static int Load_Syn_Tcp_Cfg();
static int Create_Syn_Tcp_Shm(int likCnt);
static int Load_Syn_Tcp_Cfg_Cnt();
static int Start_Syn_Tcp_All_Processes();
//==============================================//

/*
 * tcp 同步通讯客户端系统启动服务
 */
int main(int argc, char *argv[]) 
{
	if(0 > Open_Syn_Tcp_Mgt_Log(argv[0])) 
		exit(1);

	dcs_log(0,0, "Syn Tcp Management is starting up...");

	u_daemonize(NULL);

	int linkCnt = Load_Syn_Tcp_Cfg_Cnt();
	//创建tcp 同步通讯配置的共享内存

	dcs_log(0,0, "Syn Tcp  config item count =%d",linkCnt);

	if(Create_Syn_Tcp_Shm(linkCnt) < 0)
	{
	 	dcs_log(0,0, " Create Syn Tcp  config shm err!");
		 exit(0);
	}

	dcs_log(0,0, "Create Syn Tcp  config shm success!");
	//加载tcp 同步通讯配置文件
	if(Load_Syn_Tcp_Cfg() < 0)
	{
		dcs_log(0,0,"Load Syn Tcp config err!");
		exit(0);
	}

	dcs_log(0,0,"**********************************************\n"
	            "       Syn Tcp Management  Start all processes!\n"
	            "**********************************************\n");

	Start_Syn_Tcp_All_Processes();

	dcs_log(0,0,"*************************************************\n"
			 "!********All  Syn Tcp processes startup completed.*********!!\n"
			"*************************************************\n");
    
    return 0;
}
/*
 * 加载配置信息个数
 */
static int Load_Syn_Tcp_Cfg_Cnt()
{
	int    iFd=-1,iCnt;
	char cfgfile[256];

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

	//读取配置项中的配置项
	if(0 > conf_get_first_number(iFd,"conn.max",&iCnt)|| iCnt < 0 )
	{
		dcs_log(0,0,"load config item 'comm.max' failed!");
		conf_close(iFd);
		return -1;
	}

	conf_close(iFd);
	return iCnt;
}

/*
 * 加载配置文件
 */
static int Load_Syn_Tcp_Cfg(void)
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iSlotIndex,iCnt;
	char cfgfile[256];

	char *commType = NULL,*recv_queue = NULL,*send_queue = NULL,*server_address = NULL, * timeout = NULL, *MaxthreadNum = NULL;
	char server_ip[67];
	char server_port[6];

	if(u_fabricatefile("config/syn_tcp.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'syn_tcp.conf'\n");
		return -1;
	}

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

	iSlotIndex = 0;

	iRc = conf_get_first_string(iFd, "conn",caBuffer);
	dcs_log(0, 0, "conf_get_first_string conn = %d, caBuffer = %s", iRc, caBuffer);

	for( ;
	iRc == 0 && iSlotIndex < sp_syntcp_ctrl->i_conn_num;
	iRc = conf_get_next_string(iFd, "conn",caBuffer))
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

		dcs_log(0, 0, "commType = [%s], recv_queue = [%s], send_queue = [%s],server_ip = [%s], server_port = [%s],timeout = [%s], MaxthreadNum =[%s]\n",
			commType,recv_queue,send_queue,server_ip, server_port, timeout, MaxthreadNum);

		if (commType == NULL || recv_queue == NULL || send_queue == NULL || server_address == NULL || 
			timeout == NULL || MaxthreadNum == NULL )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}
		
		strcpy(sp_syntcp_ctrl->tcpc[iSlotIndex].commType,commType);
		strcpy(sp_syntcp_ctrl->tcpc[iSlotIndex].recv_queue,recv_queue);
		strcpy(sp_syntcp_ctrl->tcpc[iSlotIndex].send_queue,send_queue);
		strcpy(sp_syntcp_ctrl->tcpc[iSlotIndex].server_ip,server_ip);

		sp_syntcp_ctrl->tcpc[iSlotIndex].server_port = atoi(server_port);
		sp_syntcp_ctrl->tcpc[iSlotIndex].timeout= atoi(timeout);
		sp_syntcp_ctrl->tcpc[iSlotIndex].maxthreadnum = atoi(MaxthreadNum);
		
		iSlotIndex++;
	}
	dcs_log(0,0,"load config end");
	conf_close(iFd);
	return 0;
}


/*
 * 打开日志
 */
static int Open_Syn_Tcp_Mgt_Log(char *IDENT)
{
	char logfile[256];
	memset(logfile, 0, sizeof(logfile));
	if(u_fabricatefile("log/synTcpMgt.log",logfile,sizeof(logfile)) < 0)
		return -1;

	return dcs_log_open(logfile, IDENT);
}

/*
 * 创建SYN_TCP共享内存 
 */
int Create_Syn_Tcp_Shm(int likCnt)
{
	char *ptr = NULL;
	int shmid =-1;
	int size = likCnt * sizeof(struct tcp_Ast);

	dcs_log(0, 0, "shm create SYN_TCP SHM size = %d ", size);
	ptr = shm_create(SYN_TCP_SHM_NAME, size,&shmid);
	dcs_log(0, 0, "shm create SYN_TCP shm_id = %d ", shmid);
	if(ptr == NULL)
	{
		dcs_log(0,0,"cannot create SHM '%s':%s!\n", SYN_TCP_SHM_NAME,strerror(errno));
		return -1;
	}
	//get all the pointers and do initialization
	memset(ptr, 0x00, size);
	sp_syntcp_ctrl   = (struct tcp_Ast *)ptr;
	sp_syntcp_ctrl->i_conn_num= likCnt;
	
	return 0;
}

static int Start_Syn_Tcp_All_Processes()
{
	int i;
	for(i=0;i< sp_syntcp_ctrl->i_conn_num;i++)
	{
		struct syn_tcp_struct link = sp_syntcp_ctrl->tcpc[i];
		if(((strlen(sp_syntcp_ctrl->tcpc[i].server_ip)) > 0)&&(sp_syntcp_ctrl->tcpc[i].server_port > 0))
		{
			dcs_log(0,0,"recv_queue =[%s],server_ip=[%s],server_port=[%d]",link.recv_queue,link.server_ip,link.server_port);
			Start_Process("synTcp",link.recv_queue);
			u_sleep_us(100000);/*等待100ms*/
		}	
	}
	return 0;
}


