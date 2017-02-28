/*  
 *文件名称：syn_http_mgt.c
 *功能描述：
 *HTTP协议的客户端短链同步服务启动进程,用于初始化共享内存，
 * 根据配置文件syn_http.conf 启动各个服务进程 synHttp
*/
#include "syn_http.h"

static struct http_Ast *sp_synhttp_ctrl = NULL;
//======= Funtion Forward delection =============//
static int Open_Syn_Http_Mgt_Log(char *IDENT);
static int Load_Syn_Http_Cfg();
static int Create_Syn_Http_Shm(int likCnt);
static int Load_Syn_Http_Cfg_Cnt();
static int Start_Syn_Http_All_Processes();
//==============================================//

/*
 * http 同步通讯客户端系统启动服务
 */
int main(int argc, char *argv[]) 
{
	if(0 > Open_Syn_Http_Mgt_Log(argv[0])) 
		exit(1);

	dcs_log(0,0, "Syn Http Management is starting up...");

	u_daemonize(NULL);

	int linkCnt = Load_Syn_Http_Cfg_Cnt();
	//创建http 同步通讯配置的共享内存

	dcs_log(0,0, "Syn Http  config item count =%d",linkCnt);

	if(Create_Syn_Http_Shm(linkCnt) < 0)
	{
	 	dcs_log(0,0, " Create Syn Http  config shm err!");
		 exit(0);
	}

	dcs_log(0,0, "Create Syn Http  config shm success!");
	//加载http 同步通讯配置文件
	if(Load_Syn_Http_Cfg() < 0)
	{
		dcs_log(0,0,"Load Syn Http config err!");
		exit(0);
	}

	dcs_log(0,0,"**********************************************\n"
	            "       Syn Http Management  Start all processes!\n"
	            "**********************************************\n");

	Start_Syn_Http_All_Processes();

	dcs_log(0,0,"*************************************************\n"
			 "!********All  Syn Http processes startup completed.*********!!\n"
			"*************************************************\n");
    
    return 0;
}
/*
 * 加载配置信息个数
 */
static int Load_Syn_Http_Cfg_Cnt()
{
	int    iFd=-1,iCnt;
	char cfgfile[256];

	if(u_fabricatefile("config/syn_http.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'syn_http.conf'\n");
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
static int Load_Syn_Http_Cfg(void)
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iSlotIndex,iCnt;
	char cfgfile[256];

	char *recv_queue = NULL,*send_queue = NULL,*server_address = NULL, * timeout = NULL, *MaxthreadNum = NULL;

	if(u_fabricatefile("config/syn_http.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'syn_http.conf'\n");
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
	iRc == 0 && iSlotIndex < sp_synhttp_ctrl->i_conn_num;
	iRc = conf_get_next_string(iFd, "conn",caBuffer))
	{
		//接收队列
		recv_queue = strtok(caBuffer," \t\r");
		//返回队列
		send_queue = strtok(NULL," \t\r");
		//服务器地址
		server_address    = strtok(NULL," \t\r");
		//超时时间
		timeout      = strtok(NULL, " \t\r");
		//设置每条短链最大的线程数
		MaxthreadNum = strtok(NULL , "  \t\r");

		dcs_log(0, 0, "recv_queue = [%s], send_queue = [%s],server_address = [%s], timeout = [%s], MaxthreadNum =[%s]\n",
			recv_queue,send_queue,server_address, timeout, MaxthreadNum);

		if (recv_queue == NULL || send_queue == NULL || server_address == NULL || 
			timeout == NULL || MaxthreadNum == NULL )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}
		
		strcpy(sp_synhttp_ctrl->httpc[iSlotIndex].recv_queue,recv_queue);
		strcpy(sp_synhttp_ctrl->httpc[iSlotIndex].send_queue,send_queue);
		strcpy(sp_synhttp_ctrl->httpc[iSlotIndex].serverAddr,server_address);

		sp_synhttp_ctrl->httpc[iSlotIndex].timeout= atoi(timeout);
		sp_synhttp_ctrl->httpc[iSlotIndex].maxthreadnum = atoi(MaxthreadNum);
		
		iSlotIndex++;
	}
	dcs_log(0,0,"load config end");
	conf_close(iFd);
	return 0;
}
/*
 * 打开日志
 */
static int Open_Syn_Http_Mgt_Log(char *IDENT)
{
	char logfile[256];
	memset(logfile, 0, sizeof(logfile));
	if(u_fabricatefile("log/synHttpMgt.log",logfile,sizeof(logfile)) < 0)
		return -1;

	return dcs_log_open(logfile, IDENT);
}

/*
 * 创建SYN_HTTP共享内存 
 */
int Create_Syn_Http_Shm(int likCnt)
{
	char *ptr = NULL;
	int shmid =-1;
	int size = likCnt * sizeof(struct http_Ast);

	dcs_log(0, 0, "shm create SYN_HTTP SHM size = %d ", size);
	ptr = shm_create(SYN_HTTP_SHM_NAME, size,&shmid);
	dcs_log(0, 0, "shm create SYN_HTTP shm_id = %d ", shmid);
	if(ptr == NULL)
	{
		dcs_log(0,0,"cannot create SHM '%s':%s!\n", SYN_HTTP_SHM_NAME,strerror(errno));
		return -1;
	}
	//get all the pointers and do initialization
	memset(ptr, 0x00, size);
	sp_synhttp_ctrl   = (struct http_Ast *)ptr;
	sp_synhttp_ctrl->i_conn_num= likCnt;
	
	return 0;
}

static int Start_Syn_Http_All_Processes()
{
	int i;
	for(i=0;i< sp_synhttp_ctrl->i_conn_num;i++)
	{
		struct syn_http_struct link = sp_synhttp_ctrl->httpc[i];
		if(strlen(sp_synhttp_ctrl->httpc[i].serverAddr) > 0)
		{
			dcs_log(0,0,"recv_queue =[%s],serverAddr=[%s],",link.recv_queue,sp_synhttp_ctrl->httpc[i].serverAddr);
			Start_Process("synHttp",link.recv_queue);
			u_sleep_us(100000);/*等待100ms*/
		}	
	}
	return 0;
}


