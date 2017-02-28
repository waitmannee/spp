#include "short_conn_client.h"

static int gs_myFid;

static struct conn_c_Ast *p_conn_c_Ctrl = NULL;
//======= Funtion Forward delection =============//
static int OpenScClientLog(char *IDENT);
static int load_sc_client_cfg();
static int create_sc_client_shm(int likCnt);
static int load_short_conn_client_cfg_cnt();
static int StartScClientAllProcesses();
//==============================================//

/*
 * 短链客户端系统启动服务
 */
int main(int argc, char *argv[]) 
{
	int lfd;

	if(0 > OpenScClientLog(argv[0])) exit(1);

	dcs_log(0,0, "Short Connnection Client Svr is starting up...");

	u_daemonize(NULL);

	int linkCnt = load_short_conn_client_cfg_cnt();
	//创建短链接客户端的共享内存

	dcs_log(0,0, "Short Connnection Client  config item count =%d",linkCnt);

	if(create_sc_client_shm(linkCnt) < 0)
	{
	 	dcs_log(0,0, "Short Connnection Client Create shm err!");
		 exit(0);
	}

	dcs_log(0,0, "Short Connnection Client Create shm success!");
	//加载短连接配置文件
	if(load_sc_client_cfg() < 0)
	{
		dcs_log(0,0,"Short Connnection Client load Client config err!");
		exit(0);
	}

	dcs_log(0,0,"Short Connnection Client load Client config success!");

	dcs_log(0,0,"**********************************************\n"
	            "       Short Connnection Client Start all processes!\n"
	            "**********************************************\n");

	StartScClientAllProcesses();

	dcs_log(0,0,"*************************************************\n"
			 "!******** Short Connnection client startup completed.*********!!\n"
			"*************************************************\n");
    
    return 0;
}
/*
 * 加载我方作为短链接客户端的个数
 */
static int load_short_conn_client_cfg_cnt()
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iCnt,iSlotIndex;
	char cfgfile[256];

	if(u_fabricatefile("config/shortconnect_client.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'shortconnect_client.conf'\n");
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
		dcs_log(0,0,"<short_clientSvr> load config item 'comm.max' failed!");
		conf_close(iFd);
		return -1;
	}

	conf_close(iFd);
	return iCnt;
}

/*
 * 加载我方作为短链接客户端的配置文件
 */
static int load_sc_client_cfg(void)
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iSlotIndex,iCnt;
	char cfgfile[256];

	char *commType = NULL,*name = NULL,*fold_name = NULL,*msgtype = NULL,*server_ip = NULL, * timeout = NULL, *MaxthreadNum = NULL;
	int server_port = 0;

	char sLocalPort[20];
	char sRemotePort[20];

	if(u_fabricatefile("config/shortconnect_client.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'shortconnect_client.conf'\n");
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

	iSlotIndex = 0;

	iRc = conf_get_first_string(iFd, "conn",caBuffer);
	dcs_log(0, 0, "conf_get_first_string conn = %d, caBuffer = %s", iRc, caBuffer);

	for( ;
	iRc == 0 && iSlotIndex < p_conn_c_Ctrl->i_conn_num;
	iRc = conf_get_next_string(iFd, "conn",caBuffer))
	{
		commType = strtok(caBuffer," \t\r");//通讯模式
		name = strtok(NULL," \t\r");//链路FOLDER名字
		fold_name      = strtok(NULL," \t\r");//链路对应业务处理进程的名字
		msgtype        = strtok(NULL," \t\r");//报文类型
		server_ip    = strtok(NULL," \t\r");
		timeout      = strtok(NULL, " \t\r");//超时时间
		MaxthreadNum = strtok(NULL , "  \t\r");//设置每条短链最大的线程数

		
		char dst_ip[256];
		char dst_port[20];
		memset(dst_ip, 0x00, sizeof(dst_ip));
		memset(dst_port, 0x00, sizeof(dst_port));
		util_split_ip_with_port_address(server_ip, dst_ip, dst_port);

		memset(server_ip, 0x00, strlen(server_ip));
		memcpy(server_ip, dst_ip, strlen(dst_ip));
		server_port = atoi(dst_port);

		dcs_log(0, 0, "commType = [%s], name = [%s], fold_name = [%s], msgtype = [%s], server_ip = [%s], server_port = [%d],timeout = [%s], MaxthreadNum =[%s]\n",
			commType,name,fold_name,msgtype,server_ip, server_port, timeout, MaxthreadNum);

		if (commType == NULL || name == NULL || fold_name == NULL ||msgtype == NULL ||  server_ip == NULL || server_port ==0||
			timeout == NULL || MaxthreadNum == NULL )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}
		
		strcpy(p_conn_c_Ctrl->connc[iSlotIndex].commType,commType);
		strcpy(p_conn_c_Ctrl->connc[iSlotIndex].name,name);
		strcpy(p_conn_c_Ctrl->connc[iSlotIndex].fold_name,fold_name);
		strcpy(p_conn_c_Ctrl->connc[iSlotIndex].msgtype,msgtype);
		strcpy(p_conn_c_Ctrl->connc[iSlotIndex].server_ip,server_ip);

		p_conn_c_Ctrl->connc[iSlotIndex].server_port = server_port;
		p_conn_c_Ctrl->connc[iSlotIndex].timeout= atoi(timeout);
		p_conn_c_Ctrl->connc[iSlotIndex].maxthreadnum = atoi(MaxthreadNum);

		dcs_log(0, 0, "p_conn_c_Ctrl->linkArray[%d].name = %s", iSlotIndex, p_conn_c_Ctrl->connc[iSlotIndex].name);
		//caFoldName
		int myFid = fold_locate_folder(name);
		if(myFid < 0) 
			myFid = fold_create_folder(name);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s\n",name, ise_strerror(errno) );
		}
		
		p_conn_c_Ctrl->connc[iSlotIndex].iFid =myFid;
		
		myFid = fold_locate_folder(fold_name);
		if(myFid < 0) 
			myFid = fold_create_folder(fold_name);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s\n",fold_name, ise_strerror(errno) );
		}
		p_conn_c_Ctrl->connc[iSlotIndex].iApplFid =myFid;

		iSlotIndex++;
	}
	dcs_log(0,0," load config end");
	conf_close(iFd);
	return 0;
}


static int OpenScClientLog(char *IDENT)
{
	char logfile[256];
	memset(logfile, 0, sizeof(logfile));
	if(u_fabricatefile("log/ScClient.log",logfile,sizeof(logfile)) < 0)
		return -1;

	return dcs_log_open(logfile, IDENT);
}

/*
 * 创建短链客户端的共享内存
 */
int create_sc_client_shm(int likCnt)
{
	char *ptr = NULL;
	int shmid =-1;
	int size = likCnt * sizeof(struct conn_c_Ast);

	dcs_log(0, 0, "shm create SC_CLIENT SHM size = %d ", size);
	ptr = shm_create(SHORT_CONN_CLIENT_SHM_NAME, size,&shmid);
	dcs_log(0, 0, "shm create SC_CLIENT shm_id = %d ", shmid);
	if(ptr == NULL)
	{
		dcs_log(0,0,"cannot create SHM '%s':%s!\n", SHORT_CONN_CLIENT_SHM_NAME,strerror(errno));
		return -1;
	}
	//get all the pointers and do initialization
	memset(ptr, 0x00, size);
	p_conn_c_Ctrl   = (struct conn_c_Ast *)ptr;
	p_conn_c_Ctrl->i_conn_num= likCnt;
	
	return 0;
}

static int StartScClientAllProcesses()
{
	int i;
	for(i=0;i< p_conn_c_Ctrl->i_conn_num;i++)
	{
		struct conn_c sc_link = p_conn_c_Ctrl->connc[i];
		if(((strlen(p_conn_c_Ctrl->connc[i].server_ip)) > 0)&&(p_conn_c_Ctrl->connc[i].server_port > 0))
		{
			dcs_log(0,0,"name =[%s],fold_name = [%s],iApplFid=[%d],iFid=[%d],server_ip=[%s],server_port=[%d]",sc_link.name,sc_link.fold_name,sc_link.iApplFid,sc_link.iFid,sc_link.server_ip,sc_link.server_port);
			Start_Process("shortClient",sc_link.name);
			u_sleep_us(100000);/*等待100ms*/
		}	
	}
	return 0;
}

