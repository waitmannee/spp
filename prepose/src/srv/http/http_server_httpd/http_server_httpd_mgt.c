 /*  
 *程序名称：http_server_mgt.c
 *功能描述：
 *用于http服务器端程序的初始化,如创建共享内存，读配置文件，起子进程
*/
#include "http_server_httpd.h"

static struct http_s_Ast *p_http_s_Ctrl = NULL;
//======= Funtion Forward delection =============//
static int Open_Http_Server_Mgt_Log(char *IDENT);
static int Load_Http_Server_Cfg();
static int Create_Http_Server_Shm(int likCnt);
static int Load_Http_Server_Cfg_cnt();
static int Start_Http_Server_All_Processes();
//==============================================//

/*
 * http协议短链服务器端系统启动服务
 */
int main(int argc, char *argv[]) 
{
	int lfd;

	if(0 > Open_Http_Server_Mgt_Log(argv[0])) 
		exit(1);

	dcs_log(0,0, "Http Server Httpd Mgt is starting up...");

	u_daemonize(NULL);

	int linkCnt = Load_Http_Server_Cfg_cnt();
	//创建http短链接服务器的共享内存

	dcs_log(0,0, "Http Server Httpd  config item count =%d",linkCnt);

	if(Create_Http_Server_Shm(linkCnt) < 0)
	{
	 	dcs_log(0,0, "Http Server Httpd Mgt Create shm err!");
		 exit(0);
	}

	dcs_log(0,0, "Http Server Httpd Mgt Create shm success!");
	//加载短连接配置文件
	if(Load_Http_Server_Cfg() < 0)
	{
		dcs_log(0,0,"Http Server Httpd Mgt load Server config err!");
		exit(0);
	}

	dcs_log(0,0,"**********************************************\n"
	            "       Http Server Httpd begin  startup all processes!\n"
	            "**********************************************\n");

	Start_Http_Server_All_Processes();

	dcs_log(0,0,"*************************************************\n"
			 "!******** Http Server Httpds startup completed.*********!!\n"
			"*************************************************\n");
    
    return 0;
}
/*
 * 加载http服务器端的个数
 */
static int Load_Http_Server_Cfg_cnt()
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iCnt,iSlotIndex;
	char cfgfile[256];

	if(u_fabricatefile("config/http_server_httpd.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'http_server_httpd.conf'\n");
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
		dcs_log(0,0,"<short_serverSvr> load config item 'comm.max' failed!");
		conf_close(iFd);
		return -1;
	}

	conf_close(iFd);
	return iCnt;
}

/*
 * 加载http短链接服务器端的配置文件
 */
static int Load_Http_Server_Cfg(void)
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iSlotIndex,iCnt;
	char cfgfile[256];

	char *name = NULL,*fold_name = NULL,*msgtype = NULL,*port = NULL, *timeout = NULL, *MaxthreadNum = NULL;

	if(u_fabricatefile("config/http_server_httpd.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'http_server_httpd.conf'\n");
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
		dcs_log(0,0,"load config item 'conn.max' failed!");
		conf_close(iFd);
		return -1;
	}

	iSlotIndex = 0;

	iRc = conf_get_first_string(iFd, "conn",caBuffer);
	dcs_log(0, 0, "conf_get_first_string conn = %d, caBuffer = %s", iRc, caBuffer);

	for( ;
	iRc == 0 && iSlotIndex < p_http_s_Ctrl->i_http_num;
	iRc = conf_get_next_string(iFd, "conn",caBuffer))
	{
		name = strtok(caBuffer," \t\r");/*链路FOLDER名字*/
		fold_name      = strtok(NULL," \t\r");/*链路对应业务处理进程的名字*/
		msgtype        = strtok(NULL," \t\r");/*报文类型*/
		port   = strtok(NULL," \t\r");/*监听端口*/
		//超时时间
		timeout = strtok(NULL, " \t\r");
		//设置每条短链最大的线程数
		MaxthreadNum = strtok(NULL, "  \t\r");

		dcs_log(0, 0, "name = [%s], fold_name = [%s], msgtype = [%s], port = [%s], timeout = [%s], MaxthreadNum =[%s]",
			name,fold_name,msgtype,port, timeout, MaxthreadNum);

		if (name == NULL || fold_name == NULL ||msgtype == NULL ||  port == NULL || timeout == NULL || MaxthreadNum == NULL )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}
		
		strcpy(p_http_s_Ctrl->https[iSlotIndex].name,name);
		strcpy(p_http_s_Ctrl->https[iSlotIndex].fold_name,fold_name);
		strcpy(p_http_s_Ctrl->https[iSlotIndex].msgtype,msgtype);
		p_http_s_Ctrl->https[iSlotIndex].port = atoi(port);

		dcs_log(0, 0, "p_http_s_Ctrl->linkArray[%d].name = %s", iSlotIndex, p_http_s_Ctrl->https[iSlotIndex].name);
		//caFoldName
		int myFid = fold_locate_folder(name);
		if(myFid < 0) 
			myFid = fold_create_folder(name);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s\n",name, ise_strerror(errno) );
		}
		
		p_http_s_Ctrl->https[iSlotIndex].iFid =myFid;
		
		myFid = fold_locate_folder(fold_name);
		if(myFid < 0) 
			myFid = fold_create_folder(fold_name);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s\n",fold_name, ise_strerror(errno) );
		}
		p_http_s_Ctrl->https[iSlotIndex].iApplFid =myFid;
		p_http_s_Ctrl->https[iSlotIndex].timeout = atoi(timeout);
		p_http_s_Ctrl->https[iSlotIndex].maxthreadnum = atoi(MaxthreadNum);

		iSlotIndex++;
	}
	dcs_log(0,0," load config end");
	conf_close(iFd);
	return 0;
}

static int Open_Http_Server_Mgt_Log(char *IDENT)
{
	char logfile[256];
	memset(logfile, 0, sizeof(logfile));
	if(u_fabricatefile("log/httpSvrHttpd.log",logfile,sizeof(logfile)) < 0)
		return -1;

	return dcs_log_open(logfile, IDENT);
}

/*
 * 创建http服务器端的共享内存
 */
int Create_Http_Server_Shm(int likCnt)
{
	char *ptr = NULL;
	int shmid =-1;
	int size = likCnt * sizeof(struct http_s_Ast);

	dcs_log(0, 0, "shm create http_server SHM size = %d ", size);
	ptr = shm_create(HTTP_SERVER_HTTPD_SHM_NAME, size,&shmid);
	dcs_log(0, 0, "shm create http_server shm_id = %d ", shmid);
	if(ptr == NULL)
	{
		dcs_log(0,0,"cannot create SHM '%s':%s!\n", HTTP_SERVER_HTTPD_SHM_NAME,strerror(errno));
		return -1;
	}
	//get all the pointers and do initialization
	memset(ptr, 0x00, size);
	p_http_s_Ctrl   = (struct http_s_Ast *)ptr;
	p_http_s_Ctrl->i_http_num= likCnt;
	
	return 0;
}

static int Start_Http_Server_All_Processes()
{
	int i;
	for(i=0;i< p_http_s_Ctrl->i_http_num;i++)
	{
		struct http_s sc_link = p_http_s_Ctrl->https[i];
		if(sc_link.port > 0 )
		{
			dcs_log(0,0,"name =[%s],fold_name = [%s],iApplFid=[%d],iFid=[%d],port=[%d]",sc_link.name,sc_link.fold_name,sc_link.iApplFid,sc_link.iFid,sc_link.port);
			Start_Process("httpServerHttpd",sc_link.name);
			u_sleep_us(100000);/*等待100ms*/
		}	
	}
	return 0;
}

