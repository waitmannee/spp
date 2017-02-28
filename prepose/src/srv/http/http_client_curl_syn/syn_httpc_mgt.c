/*  
 *文件名称：syn_httpc_mgt.c
 *功能描述：
 *HTTP协议的客户端短链同步服务启动进程,用于初始化共享内存，
 * 根据配置文件lsynhttpc.conf 启动各个服务进程 lsynHttpC
*/
#include "syn_httpc.h"

static struct httpc_Ast *sp_synhttpc_ctrl = NULL;
//======= Funtion Forward delection =============//
static int Open_Syn_Httpc_Mgt_Log(char *IDENT);
static int Load_Syn_Httpc_Cfg();
static int Create_Syn_Httpc_Shm(int likCnt);
static int Load_Syn_Httpc_Cfg_Cnt();
static int Start_Syn_Httpc_All_Processes();
//==============================================//

/*
 * http 同步通讯客户端系统启动服务
 */
int main(int argc, char *argv[]) 
{
	int linkCnt = 0;
	if(0 > Open_Syn_Httpc_Mgt_Log(argv[0])) 
		exit(1);

	dcs_log(0, 0, "lSynHttpC Management is starting up...");

	linkCnt = Load_Syn_Httpc_Cfg_Cnt();
	//创建http 同步通讯配置的共享内存

	dcs_log(0, 0, "lSynHttpC config item count =%d", linkCnt);

	if(Create_Syn_Httpc_Shm(linkCnt) < 0)
	{
	 	dcs_log(0, 0, "Create lSynHttpC config shm err!");
		exit(0);
	}

	dcs_log(0, 0, "Create lSynHttpC config shm success!");
	//加载http 同步通讯配置文件
	if(Load_Syn_Httpc_Cfg() < 0)
	{
		dcs_log(0, 0, "Load lSynHttpC config err!");
		exit(0);
	}

	dcs_log(0, 0, "**********************************************\n"
	            "       lSynHttpC Management  Start all processes!\n"
	            "**********************************************\n");
	Start_Syn_Httpc_All_Processes();

	dcs_log(0, 0, "*************************************************\n"
			 "!********All  Syn HttpC processes startup completed.*********!!\n"
			"*************************************************\n");
    return 0;
}
/*
 * 加载配置信息个数
 */
static int Load_Syn_Httpc_Cfg_Cnt()
{
	int iFd=-1, iCnt = 0;
	char cfgfile[256];
	
	memset(cfgfile, 0, sizeof(cfgfile));

	if(u_fabricatefile("config/lsynhttpc.conf", cfgfile, sizeof(cfgfile)) < 0)
	{
		dcs_log(0, 0, "cannot get full path of 'lsynhttpc.conf'\n");
		return -1;
	}

	dcs_log(0, 0,"full path name cfgfile [%s]", cfgfile);

	if(0 > (iFd = conf_open(cfgfile)))
	{
		dcs_log(0, 0, "open file '%s' failed.\n", cfgfile);
		return -1;
	}

	//读取配置项中的配置项
	if(0 > conf_get_first_number(iFd, "conn.max", &iCnt) || iCnt < 0 )
	{
		dcs_log(0, 0, "load config item 'comm.max' failed!");
		conf_close(iFd);
		return -1;
	}
	conf_close(iFd);
	return iCnt;
}

/*
 * 加载配置文件
 */
static int Load_Syn_Httpc_Cfg(void)
{
	char caBuffer[512];
	int iFd=-1, iRc = 0, iSlotIndex = 0, iCnt = 0;
	char cfgfile[256];

	char *recv_queue = NULL, *send_queue = NULL, *server_address = NULL, *timeout = NULL, *MaxthreadNum = NULL;
	
	memset(caBuffer, 0, sizeof(caBuffer));
	memset(cfgfile, 0, sizeof(cfgfile));

	if(u_fabricatefile("config/lsynhttpc.conf", cfgfile, sizeof(cfgfile)) < 0)
	{
		dcs_log(0, 0, "cannot get full path of 'lsynhttpc.conf'\n");
		return -1;
	}

	if(0 > (iFd = conf_open(cfgfile)))
	{
		dcs_log(0, 0, "open file '%s' failed.", cfgfile);
		return -1;
	}

	//读取配置项中的配置项,不可去掉
	if(0 > conf_get_first_number(iFd, "conn.max", &iCnt) || iCnt < 0 )
	{
		dcs_log(0, 0, "<lsynhttpcmgt> load config item 'conn.max' failed!");
		conf_close(iFd);
		return -1;
	}
	iRc = conf_get_first_string(iFd, "conn", caBuffer);
	dcs_log(0, 0, "conf_get_first_string conn = %d, caBuffer = %s", iRc, caBuffer);

	for( ; iRc == 0 && iSlotIndex < sp_synhttpc_ctrl->i_conn_num; iRc = conf_get_next_string(iFd, "conn", caBuffer))
	{
		//接收队列
		recv_queue = strtok(caBuffer, " \t\r");
		//返回队列
		send_queue = strtok(NULL, " \t\r");
		//服务器地址
		server_address = strtok(NULL, " \t\r");
		//超时时间
		timeout = strtok(NULL, " \t\r");
		//设置每条短链最大的线程数
		MaxthreadNum = strtok(NULL, "  \t\r");

		dcs_log(0, 0, "recv_queue = [%s], send_queue = [%s], server_address = [%s], timeout = [%s], MaxthreadNum =[%s]",
			recv_queue, send_queue, server_address, timeout, MaxthreadNum);

		if (recv_queue == NULL || send_queue == NULL || server_address == NULL
			|| timeout == NULL || MaxthreadNum == NULL )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}
		
		strcpy(sp_synhttpc_ctrl->httpc[iSlotIndex].recv_queue, recv_queue);
		strcpy(sp_synhttpc_ctrl->httpc[iSlotIndex].send_queue, send_queue);
		strcpy(sp_synhttpc_ctrl->httpc[iSlotIndex].serverAddr, server_address);

		sp_synhttpc_ctrl->httpc[iSlotIndex].timeout = atoi(timeout);
		sp_synhttpc_ctrl->httpc[iSlotIndex].maxthreadnum = atoi(MaxthreadNum);
		
		iSlotIndex++;
	}
	dcs_log(0, 0, "load config end");
	conf_close(iFd);
	return 0;
}
/*
 * 打开日志
 */
static int Open_Syn_Httpc_Mgt_Log(char *IDENT)
{
	char logfile[256];
	memset(logfile, 0, sizeof(logfile));
	if(u_fabricatefile("log/lsynHttpcMgt.log", logfile, sizeof(logfile)) < 0)
		return -1;

	return dcs_log_open(logfile, IDENT);
}

/*
 * 创建SYN_HTTPC共享内存 
 */
int Create_Syn_Httpc_Shm(int likCnt)
{
	char *ptr = NULL;
	int shmid =-1;
	int size = likCnt * sizeof(struct httpc_Ast);

	dcs_log(0, 0, "shm create SYN_HTTPC SHM size = %d ", size);
	ptr = shm_create(SYN_HTTPC_SHM_NAME, size, &shmid);
	dcs_log(0, 0, "shm create SYN_HTTP shm_id = %d ", shmid);
	if(ptr == NULL)
	{
		dcs_log(0, 0, "cannot create SHM '%s':%s!", SYN_HTTPC_SHM_NAME, strerror(errno));
		return -1;
	}
	//get all the pointers and do initialization
	memset(ptr, 0x00, size);
	sp_synhttpc_ctrl = (struct httpc_Ast *)ptr;
	sp_synhttpc_ctrl->i_conn_num = likCnt;
	
	return 0;
}

static int Start_Syn_Httpc_All_Processes()
{
	struct syn_httpc_struct link;
	int i = 0;
	for( ; i< sp_synhttpc_ctrl->i_conn_num; i++)
	{
		memset(&link, 0x00, sizeof(struct syn_httpc_struct));
		link = sp_synhttpc_ctrl->httpc[i];
		if(strlen(sp_synhttpc_ctrl->httpc[i].serverAddr) > 0)
		{
			dcs_log(0, 0, "recv_queue =[%s]", link.recv_queue);
			Start_Process("lsynHttpC", link.recv_queue);
			u_sleep_us(100000);/*等待100ms*/
		}	
	}
	return 0;
}


