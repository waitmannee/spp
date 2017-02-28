#include "short_conn.h"

static int gs_myFid;


typedef struct
{
	in_port_t port;
	char root[1024];
} cnf_t;

static char* g_scproctab_[]=
{
	"shortTcp",   //makefile: create by from shortconnect.c
	NULL
};

typedef struct 
{
	int num;//已监听端口数
	int port[100];//已监听端口数组
}sc_port;

static  sc_port p_sc_port;
static struct conn_s_Ast *p_conn_s_Ctrl = NULL;
//======= Funtion Forward delection =============//
static void sighandler(int sig);
static int OpenLog(char *IDENT);
static int load_sc_server_cfg();
static int create_sc_server_shm(int likCnt);
static int load_short_conn_server_cfg_cnt();
static int StartAllProcesses();
static int IsStartPort(int port);
//==============================================//

/*
 * 短链系统Server
 */
int main(int argc, char *argv[]) 
{
	cnf_t arg;
	int lfd;

	if(0 > OpenLog(argv[0])) exit(1);

	dcs_log(0,0, "Short Connnection Server is starting up...");

	catch_all_signals(sighandler);

	u_daemonize(NULL);

	int linkCnt = load_short_conn_server_cfg_cnt();
	//创建短链接服务器的共享内存

	dcs_log(0,0, "Short Connnection Server config item count =%d",linkCnt);

	if(create_sc_server_shm(linkCnt) < 0)
	{
	 	dcs_log(0,0, "Short Connnection Server Create shm err!");
		 exit(0);
	}

	dcs_log(0,0, "Short Connnection Server Create shm success!");
	//加载短连接配置文件
	if(load_sc_server_cfg() < 0)
	{
		dcs_log(0,0,"Short Connnection Server load server config err!");
		exit(0);
	}

	dcs_log(0,0,"Short Connnection Server load server config success!");

	dcs_log(0,0,"**********************************************\n"
	            "       Short Connnection Server Start all processes!\n"
	            "**********************************************\n");

	StartAllProcesses();

	dcs_log(0,0,"*************************************************\n"
			 "!******** Short Connnection startup completed.*********!!\n"
			"*************************************************\n");
    
    return 0;
}
/*
 * 加载短链路的个数
 */
static int load_short_conn_server_cfg_cnt()
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iCnt,iSlotIndex;
	char cfgfile[256];

	if(u_fabricatefile("config/shortconnect.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'shortconnect.conf'\n");
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
		dcs_log(0,0,"<dcmhstsvr> load config item 'comm.max' failed!");
		conf_close(iFd);
		return -1;
	}

	conf_close(iFd);
	return iCnt;
}

/*
 * 加载短连接服务配置文件
 */
static int load_sc_server_cfg(void)
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iSlotIndex = 0,iCnt;
	char cfgfile[256];

	char *name = NULL,*tpdu = NULL,*fold_name = NULL,*msgtype = NULL,*listen_ip = NULL;
	int listen_port = 0;

	char sLocalPort[20];
	char sRemotePort[20];

	if(u_fabricatefile("config/shortconnect.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'shortconnect.conf'\n");
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

	for( ;
	iRc == 0 && iSlotIndex < p_conn_s_Ctrl->i_conn_num;
	iRc = conf_get_next_string(iFd, "conn",caBuffer))
	{
		tpdu = strtok(caBuffer," \t\r");//tpdu
		name = strtok(NULL," \t\r");//链路FOLDER名字
		fold_name      = strtok(NULL," \t\r");//链路对应业务处理进程的名字
		msgtype        = strtok(NULL," \t\r");//报文类型
		listen_ip    = strtok(NULL," \t\r");

		char dst_ip[256];
		char dst_port[20];
		memset(dst_ip, 0x00, sizeof(dst_ip));
		memset(dst_port, 0x00, sizeof(dst_port));
		util_split_ip_with_port_address(listen_ip, dst_ip, dst_port);

		memset(listen_ip, 0x00, sizeof(listen_ip));
		memcpy(listen_ip, dst_ip, strlen(dst_ip));
		listen_port = atoi(dst_port);

		dcs_log(0, 0, "tpdu =[%s],name = [%s], fold_name = [%s], msgtype = [%s], listen_ip = [%s], listen_port = [%d]\n",
		tpdu,name,fold_name,msgtype,listen_ip, listen_port);
		if (tpdu == NULL ||name == NULL || fold_name == NULL ||msgtype == NULL ||  listen_ip == NULL ||  listen_port == 0 )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}

		strcpy(p_conn_s_Ctrl->conns[iSlotIndex].tpdu,tpdu);
		p_conn_s_Ctrl->conns[iSlotIndex].tpduFlag =1;
		strcpy(p_conn_s_Ctrl->conns[iSlotIndex].name,name);
		strcpy(p_conn_s_Ctrl->conns[iSlotIndex].fold_name,fold_name);
		strcpy(p_conn_s_Ctrl->conns[iSlotIndex].msgtype,msgtype);
		strcpy(p_conn_s_Ctrl->conns[iSlotIndex].listen_ip,listen_ip);

		p_conn_s_Ctrl->conns[iSlotIndex].listen_port = listen_port;

		dcs_log(0, 0, "p_conn_s_Ctrl->linkArray[%d].name = %s", iSlotIndex, p_conn_s_Ctrl->conns[iSlotIndex].name);
		//caFoldName
		int myFid = fold_locate_folder(name);
		if(myFid < 0) 
			myFid = fold_create_folder(name);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s\n",name, ise_strerror(errno) );
		}
		int rt_fid=-1;
		p_conn_s_Ctrl->conns[iSlotIndex].iFid = myFid;
		p_conn_s_Ctrl->conns[iSlotIndex].iApplFid = get_fold_id_from_share_memory_by_name(p_conn_s_Ctrl->conns[iSlotIndex].fold_name,&rt_fid);

		iSlotIndex++;
	}
	dcs_log(0,0," load config end");
	conf_close(iFd);
	return 0;
}


static int OpenLog(char *IDENT)
{
	char logfile[256];
	memset(logfile, 0, sizeof(logfile));
	if(u_fabricatefile("log/ScServer.log",logfile,sizeof(logfile)) < 0)
		return -1;

	return dcs_log_open(logfile, IDENT);
}

/*
 * 创建短链服务器的共享内存
 */
int create_sc_server_shm(int likCnt)
{
	char *ptr = NULL;
	int shmid =-1;
	int size = sizeof(int) + likCnt * sizeof(struct conn_s_Ast);

	dcs_log(0, 0, "shm create SHORT_CONN_SHM_NAME size = %d ", size);
	ptr = shm_create(SHORT_CONN_SHM_NAME, size,&shmid);
	dcs_log(0, 0, "shm create SHORT_CONN_SHM_NAME shm_id = %d ", shmid);
	if(ptr == NULL)
	{
		dcs_log(0,0,"cannot create SHM '%s':%s!\n", SHORT_CONN_SHM_NAME,strerror(errno));
		return -1;
	}
	//get all the pointers and do initialization
	memset(ptr, 0x00, size);
	p_conn_s_Ctrl   = (struct conn_s_Ast *)ptr;
	p_conn_s_Ctrl->i_conn_num= likCnt;
	
	return 0;
}
int setNonblocking(int fd) 
{
	int flags;
	if (-1 ==(flags = fcntl(fd, F_GETFL, 0)))
		flags = 0;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void sighandler(int sig) 
{
	printf("catch a signal...\n");
	//exit(0);
}

static int StartAllProcesses()
{
	int i;
	char szPort[10+1];
	memset(&p_sc_port,0,sizeof(p_sc_port));
	for(i=0;i< p_conn_s_Ctrl->i_conn_num;i++)
	{
		struct conn_s sc_link = p_conn_s_Ctrl->conns[i];
		if(((strlen(p_conn_s_Ctrl->conns[i].name)) > 0)&&(IsStartPort(p_conn_s_Ctrl->conns[i].listen_port)))
		{
			dcs_log(0,0,"name =[%s],fold_name = [%s],iApplFid=[%d],iFid=[%d],listen_port=[%d]",sc_link.name,sc_link.fold_name,sc_link.iApplFid,sc_link.iFid,sc_link.listen_port);
			p_sc_port.num ++;
			p_sc_port.port[p_sc_port.num-1]=p_conn_s_Ctrl->conns[i].listen_port;
			memset(szPort,0,sizeof(szPort));
			sprintf(szPort,"%d", p_conn_s_Ctrl->conns[i].listen_port);
			p_conn_s_Ctrl->conns[i].pidComm = Start_Process(g_scproctab_[0],szPort);
			u_sleep_us(100000);/*等待100ms*/
		}	
	}
	return 0;
}

/*判断端口对应的链接是否已经启动
*未启动，返回1，已启动返回0
*/
static int IsStartPort(int port)
{
	int i ;
	for(i=0;i<p_sc_port.num;i++)
	{
		if(p_sc_port.port[i] == port)
			return 0;
	}
	
	return 1;
}

