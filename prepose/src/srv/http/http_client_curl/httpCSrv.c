#include "httpC.h"

static struct httpc_Ast *p_http_c_Ctrl = NULL;
//======= Funtion Forward delection =============//
static int OpenHttpCLog(char *IDENT);
static int load_httpclientcfg();
static int create_httpclient_shm(int likCnt);
static int load_httpclientcfg_cnt();
static int StartHttpCAllProcesses();
//==============================================//

/*
 * http协议短链客户端系统启动服务
 */
int main(int argc, char *argv[]) 
{
	int linkCnt = 0;
	if(0 > OpenHttpCLog(argv[0])) 
		exit(1);

	dcs_log(0, 0, "HttpC Srv is starting up...");
	linkCnt = load_httpclientcfg_cnt();
	//创建短链接客户端的共享内存

	dcs_log(0, 0, "HttpC config item count =%d", linkCnt);

	if(create_httpclient_shm(linkCnt) < 0)
	{
	 	dcs_log(0, 0, "HttpC Srv Create shm err!");
		 exit(0);
	}

	dcs_log(0, 0, "HttpC Srv Create shm success!");
	//加载短连接配置文件
	if(load_httpclientcfg() < 0)
	{
		dcs_log(0, 0, "HttpC Srv load Client config err!");
		exit(0);
	}

	dcs_log(0, 0, "HttpC Srv load  config success!");

	dcs_log(0, 0, "**********************************************\n"
	            "       HttpC Srv  Start all processes!\n"
	            "**********************************************\n");

	StartHttpCAllProcesses();

	dcs_log(0, 0, "*************************************************\n"
			 "!******** HttpClients startup completed.*********!!\n"
			"*************************************************\n");
    
    return 0;
}
/*
 * 加载http客户端的个数
 */
static int load_httpclientcfg_cnt()
{
	int iFd = -1, iCnt = 0;
	char cfgfile[256];
	
	memset(cfgfile, 0, sizeof(cfgfile));

	if(u_fabricatefile("config/lhttpclient.conf", cfgfile, sizeof(cfgfile)) < 0)
	{
		dcs_log(0, 0, "cannot get full path of 'httpclient.conf'");
		return -1;
	}

	dcs_log(0, 0, "full path name cfgfile [%s]", cfgfile);

	if(0 > (iFd = conf_open(cfgfile)) )
	{
		dcs_log(0, 0, "open file '%s' failed", cfgfile);
		return -1;
	}

	//读取配置项中的配置项
	if(0 > conf_get_first_number(iFd, "conn.max", &iCnt) || iCnt < 0 )
	{
		dcs_log(0, 0, "<short_httpCSrv> load config item 'comm.max' failed!");
		conf_close(iFd);
		return -1;
	}
	
	conf_close(iFd);
	return iCnt;
}

/*
 * 加载http短链接客户端的配置文件
 */
static int load_httpclientcfg(void)
{
	char caBuffer[512];
	int iFd=-1, iRc = 0, iSlotIndex = 0, iCnt = 0;
	int myFid = 0;
	char cfgfile[256];

	char *name = NULL, *fold_name = NULL, *msgtype = NULL, *server_addr = NULL, *timeout = NULL, *MaxthreadNum = NULL;
	
	memset(caBuffer, 0, sizeof(caBuffer));

	if(u_fabricatefile("config/lhttpclient.conf", cfgfile, sizeof(cfgfile)) < 0)
	{
		dcs_log(0, 0, "cannot get full path of 'httpclient.conf'");
		return -1;
	}

	dcs_log(0, 0, "full path name cfgfile[%s]", cfgfile);

	if(0 > (iFd = conf_open(cfgfile)) )
	{
		dcs_log(0, 0, "open file '%s' failed", cfgfile);
		return -1;
	}

	//读取配置项中的配置项,不可去掉
	if(0 > conf_get_first_number(iFd, "conn.max", &iCnt) || iCnt < 0 )
	{
		dcs_log(0, 0, "load config item 'conn.max' failed!");
		conf_close(iFd);
		return -1;
	}
	
	iRc = conf_get_first_string(iFd, "conn", caBuffer);
	dcs_log(0, 0, "conf_get_first_string conn = %d, caBuffer = %s", iRc, caBuffer);

	for( ; iRc == 0 && iSlotIndex < p_http_c_Ctrl->i_http_num; iRc = conf_get_next_string(iFd, "conn", caBuffer))
	{
		name = strtok(caBuffer, " \t\r");		/*链路FOLDER名字*/
		fold_name = strtok(NULL, " \t\r");		/*链路对应业务处理进程的名字*/
		msgtype = strtok(NULL, " \t\r");		/*报文类型*/
		server_addr = strtok(NULL, " \t\r");	/*服务器信息*/
		timeout = strtok(NULL, " \t\r");		/*超时时间*/
		MaxthreadNum = strtok(NULL, "  \t\r");	/*设置每条短链最大的线程数*/

		dcs_log(0, 0, "name = [%s], fold_name = [%s], msgtype = [%s], server_addr = [%s], timeout = [%s], MaxthreadNum =[%s]", \
			name, fold_name, msgtype, server_addr, timeout, MaxthreadNum);

		if (name == NULL || fold_name == NULL ||msgtype == NULL || server_addr == NULL 
			||timeout == NULL || MaxthreadNum == NULL)
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}
		
		strcpy(p_http_c_Ctrl->httpc[iSlotIndex].name, name);
		strcpy(p_http_c_Ctrl->httpc[iSlotIndex].fold_name, fold_name);
		strcpy(p_http_c_Ctrl->httpc[iSlotIndex].msgtype, msgtype);
		strcpy(p_http_c_Ctrl->httpc[iSlotIndex].serverAddr, server_addr);
		p_http_c_Ctrl->httpc[iSlotIndex].timeout= atoi(timeout);
		p_http_c_Ctrl->httpc[iSlotIndex].maxthreadnum = atoi(MaxthreadNum);

		dcs_log(0, 0, "p_http_c_Ctrl->linkArray[%d].name = %s", iSlotIndex, p_http_c_Ctrl->httpc[iSlotIndex].name);

		myFid = fold_locate_folder(name);
		if(myFid < 0) 
			myFid = fold_create_folder(name);

		if(myFid < 0)
		{
			dcs_log(0, 0, "cannot create folder '%s':%s", name, ise_strerror(errno));
		}
		
		p_http_c_Ctrl->httpc[iSlotIndex].iFid = myFid;
		
		myFid = fold_locate_folder(fold_name);
		if(myFid < 0) 
			myFid = fold_create_folder(fold_name);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s", fold_name, ise_strerror(errno) );
		}
		p_http_c_Ctrl->httpc[iSlotIndex].iApplFid = myFid;

		iSlotIndex++;
	}
	dcs_log(0, 0, "load config end");
	conf_close(iFd);
	return 0;
}

static int OpenHttpCLog(char *IDENT)
{
	char logfile[256];
	
	memset(logfile, 0, sizeof(logfile));
	
	if(u_fabricatefile("log/lhttpC.log", logfile, sizeof(logfile)) < 0)
		return -1;

	return dcs_log_open(logfile, IDENT);
}

/*
 * 创建http客户端的共享内存
 */
int create_httpclient_shm(int likCnt)
{
	char *ptr = NULL;
	int shmid = -1;
	int size = likCnt * sizeof(struct httpc_Ast);

	dcs_log(0, 0, "shm create httpC SHM size = %d ", size);
	ptr = shm_create(HTTPCLIENT_SHMNAME, size, &shmid);
	dcs_log(0, 0, "shm create httpC shm_id = %d ", shmid);
	if(ptr == NULL)
	{
		dcs_log(0, 0, "cannot create SHM '%s':%s!\n", HTTPCLIENT_SHMNAME, strerror(errno));
		return -1;
	}
	//get all the pointers and do initialization
	memset(ptr, 0x00, size);
	p_http_c_Ctrl = (struct httpc_Ast *)ptr;
	p_http_c_Ctrl->i_http_num = likCnt;
	
	return 0;
}

static int StartHttpCAllProcesses()
{
	struct httpc sc_link;
	int i = 0;
	for(; i< p_http_c_Ctrl->i_http_num; i++)
	{
		memset(&sc_link, 0, sizeof(struct httpc));
		sc_link = p_http_c_Ctrl->httpc[i];
		if(strlen(p_http_c_Ctrl->httpc[i].serverAddr))
		{
			dcs_log(0, 0, "name =[%s], fold_name = [%s], iApplFid=[%d], iFid=[%d], serverAddr=[%s]", \
			sc_link.name, sc_link.fold_name, sc_link.iApplFid, sc_link.iFid, sc_link.serverAddr);
			Start_Process("lhttpC", sc_link.name);
			u_sleep_us(100000);/*等待100ms*/
		}	
	}
	return 0;
}


