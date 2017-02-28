#include "short_conn.h"

#define   DCS_FLAG_NACREADER      0x00000004


static struct conn_s_Ast *pLinkScsCtrl = NULL;
static void scServerCmdExit(int signo);
static int AddScServerLink();
static void ListScServerInfo(int arg_c, char *arg_v[]);
static int reload_sc_server_cfg(struct conn_s_Ast *pLink,char *port,char * tpdu);
static int StopScServerLink();
static int GetScServerLink(char *port,int *pFlag);
static int OpenscServerCmdLog();
static int  StartScServerLink();
static int StartScServerProcess(const char * strExeFile, const char * arg1);
static int AddTpdu();
static int StopTpdu();
static int  StartTpdu();
static int IsPort(char * port);
static int IsTpdu(char * tpdu);
//==========================================

int main(int argc, char *argv[])
{
	
	catch_all_signals(scServerCmdExit);

	if(OpenscServerCmdLog()< 0)
		return -1;

	//连接共享内存
	if(pLinkScsCtrl == NULL)
	{
		pLinkScsCtrl = (struct conn_s_Ast*)shm_connect(SHORT_CONN_SHM_NAME, NULL);
		if(pLinkScsCtrl == NULL)
		{
			dcs_log(0, 0, "cannot connect SHM '%s':%s!\n", SHORT_CONN_SHM_NAME, strerror(errno));
			return ;
		}
	}
	cmd_init("scServerCmd>>",scServerCmdExit);
	cmd_add("list", (cmdfunc_t *)ListScServerInfo, "list link info");
	cmd_add("start", (cmdfunc_t *)StartScServerLink, "start linkName --start a link");
	cmd_add("add", (cmdfunc_t *)AddScServerLink, "Add linkName --add a link");
	cmd_add("stop", (cmdfunc_t *)StopScServerLink, "stop linkName --stop a link");
	cmd_add("addTpdu", (cmdfunc_t *)AddTpdu, "add  tpdu --add a tpdu with a port ");
	cmd_add("stopTpdu", (cmdfunc_t *)StopTpdu, "stop tpdu --stop a tpdu with a port");
	cmd_add("startTpdu", (cmdfunc_t *)StartTpdu, "start tpdu --start a tpdu with a port");
	

	cmd_shell(argc,argv);

	scServerCmdExit(0);
	return 0;
}


static void scServerCmdExit(int signo)
{
	shm_detach((char *)pLinkScsCtrl);
	exit(signo);
}

static void ListScServerInfo(int arg_c, char *arg_v[])
{
	int i = 0;
	char sBuf[200]={0};
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	printf("maxNum=%d\n",pLinkScsCtrl->i_conn_num);
	printf("id     FdName        AppFdName      tpdu     tpduflag   Status   listenPort\n");
	printf("=============================================================================\n");
	for(i = 0; i<pLinkScsCtrl->i_conn_num; i++)
	{
		if(strlen(pLinkScsCtrl->conns[i].name) > 0)
		{
			printf("[%03d] ", i+1);
			printf("%-15s", pLinkScsCtrl->conns[i].name);
			printf("%-15s", pLinkScsCtrl->conns[i].fold_name);
			
			printf("%-10s  ", pLinkScsCtrl->conns[i].tpdu);

			//tpdu Status
			if(pLinkScsCtrl->conns[i].tpduFlag==1)
				printf("%-8s ", "active");
			else if( pLinkScsCtrl->conns[i].tpduFlag==-1)
				printf("%-8s ", "invalid");
			else
				printf("%s ", " ");
			//link Status
			if(pLinkScsCtrl->conns[i].iStatus == DCS_STAT_LISTENING )
				printf("%-8s ", "listen");
			else if(pLinkScsCtrl->conns[i].iStatus ==DCS_STAT_STOPPED )
				printf("%-8s ", "stoped");
			else
				printf("%s ", " ");

			printf("%d", pLinkScsCtrl->conns[i].listen_port);
			printf("\n");
		}
	}
	return ;
}

static int AddScServerLink()
{
	char port[6];
	char sBuf[200]={0};
	int nRet = 0,i,nPid;
	
	memset(port, 0, sizeof(port));
	scanf("%s", port);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);
	if(IsPort(port) <0)
	{
		printf("请使用以下格式:\n");
		printf("add portNum\n");
		return -1;
	}
	dcs_log(0,0,"添加端口监听服务port=[%s] .",port);
	//先查找是否是已存在的链路
	for(i=0;i<pLinkScsCtrl->i_conn_num;i++)
	{ 
		if(atoi(port) == pLinkScsCtrl->conns[i].listen_port)
			break;
	}

	if(i < pLinkScsCtrl->i_conn_num)
	{
		dcs_log(0,0,"端口 port=[%s] 服务已经存在.",port);
		printf("端口 port=[%s] 服务已经存在.\n",port);
		return -1;
	}

	//不存在的话，从配置文件中 读出配置信息
	nRet=reload_sc_server_cfg(pLinkScsCtrl,port,NULL);
	if(nRet <0)
	{
		return -1;
	}

	//
	for(i=0;i<pLinkScsCtrl->i_conn_num;i++)
	{ 
		if(atoi(port) == pLinkScsCtrl->conns[i].listen_port)
		{
			nPid=StartScServerProcess("shortTcp",port);
			if(nPid < 0)
			{
				dcs_log(0,0,"端口 port=[%s],监听服务启动失败",port);
				printf("端口 port=[%s],监听服务启动失败\n",port);
				return -1;
			}
			dcs_log(0,0,"端口 port=[%s],监听服务启动成功",port);
			return 0;
		}
	}
	return 0;
}


static int StartScServerProcess(const char * strExeFile, const char * arg1)
{
	int nRet,pidchild = -1;
	int i;
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
		
		for(i=0;i<pLinkScsCtrl->i_conn_num;i++)
		{
			if(atoi(arg1) == pLinkScsCtrl->conns[i].listen_port)
			{
				pLinkScsCtrl->conns[i].pidComm = pidchild;
			}		
		}
		return pidchild;
	}
	//child her
	memset(path,0x00,sizeof(path));
	
	sprintf(path,"%s/run/bin/%s",getenv("EXECHOME"),strExeFile);
	dcs_log(0,0,"exec %s:%s",strExeFile,arg1);
	nRet =execlp(path,strExeFile,arg1,(char *)0);
	if(nRet< 0)
    	{
       	dcs_log(0,0,"cannot exec executable '%s':%s!",strExeFile,strerror(errno));
        	exit(249);
    	}

   	 return 0;	
}


static int reload_sc_server_cfg(struct conn_s_Ast *pLink,char *port,char * pTpdu)
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iSlotIndex=0,i,iCnt;
	char cfgfile[256];
	int rt_fid=-1;

	char *name = NULL,*tpdu = NULL,*fold_name = NULL,*msgtype = NULL,*listen_ip = NULL;
	int listen_port = 0;

	char sLocalPort[20];
	char sRemotePort[20];

	int nFlag=0;

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


	for(iSlotIndex=0;iSlotIndex<pLinkScsCtrl->i_conn_num;iSlotIndex++)
	{
		if(strlen(pLinkScsCtrl->conns[iSlotIndex].name) == 0)
		{
			break;
		}
	}
	if(iSlotIndex ==pLinkScsCtrl->i_conn_num )
	{
		dcs_log(0,0,"共享内存没有空余空间存储新的配置信息,最多%d条",iSlotIndex);
		printf("共享内存没有空余空间存储新的配置信息,最多%d条\n",iSlotIndex);
		return -1;
	}

	iRc = conf_get_first_string(iFd, "conn",caBuffer);
	for(i=0 ;iRc == 0 && i < pLinkScsCtrl->i_conn_num;iRc = conf_get_next_string(iFd, "conn",caBuffer))
	{
		//dcs_log(0, 0, "iRc=%d, caBuffer = %s", iRc, caBuffer);

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

		if(strcmp(dst_port,port) != 0)
		{
			i++;
			continue;
		}

		if(pTpdu != NULL && strcmp(tpdu,pTpdu) != 0 )
		{
			i++;
			continue;
		}
		
		nFlag =1;//找到配置

		dcs_log(0, 0, "tpdu =[%s],name = [%s], fold_name = [%s], msgtype = [%s], listen_ip = [%s], listen_port = [%d]\n",
		tpdu,name,fold_name,msgtype,listen_ip, listen_port);

		if (tpdu == NULL ||name == NULL || fold_name == NULL ||msgtype == NULL ||  listen_ip == NULL || listen_port == 0 )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}

		strcpy(pLinkScsCtrl->conns[iSlotIndex].tpdu,tpdu);
		pLinkScsCtrl->conns[iSlotIndex].tpduFlag =1;
		strcpy(pLinkScsCtrl->conns[iSlotIndex].name,name);
		strcpy(pLinkScsCtrl->conns[iSlotIndex].fold_name,fold_name);
		strcpy(pLinkScsCtrl->conns[iSlotIndex].msgtype,msgtype);
		strcpy(pLinkScsCtrl->conns[iSlotIndex].listen_ip,listen_ip);

		pLinkScsCtrl->conns[iSlotIndex].listen_port = listen_port;
		
		//name
		dcs_log(0, 0, "p_conn_s_Ctrl->linkArray[%d].name = %s", iSlotIndex, pLinkScsCtrl->conns[iSlotIndex].name);
		//caFoldName
		int myFid = fold_locate_folder(name);
		if(myFid < 0) 
			myFid = fold_create_folder(name);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s\n",name, ise_strerror(errno) );
		}
		
		pLinkScsCtrl->conns[iSlotIndex].iFid = myFid;
		pLinkScsCtrl->conns[iSlotIndex].iApplFid = get_fold_id_from_share_memory_by_name(pLinkScsCtrl->conns[iSlotIndex].fold_name,&rt_fid);

		iSlotIndex++;
		i++;
	}
	dcs_log(0,0,"reload config end");
	conf_close(iFd);
	if(nFlag ==0 && pTpdu == NULL)
	{
		dcs_log(0,0,"没有该端口 port=[%s] 的配置信息",port);
		printf("没有该端口 port=[%s] 的配置信息\n",port);
		return -1;
	}
	else if (nFlag ==0 && pTpdu != NULL)
	{
		dcs_log(0,0,"没有该端口 port=[%s] tpdu[%s]的配置信息",port,pTpdu);
		printf("没有该端口 port=[%s] tpdu[%s]的配置信息\n",port,pTpdu);
		return -1;
	}
	return 0;
}
/*关闭相关短链服务
*/
int StopScServerLink()
{
	char port[6];
	char sBuf[200]={0};
	int nRet = -1;
	int nFlag=-1;
	memset(port,0,sizeof(port));
	
	scanf("%s",port);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	if(IsPort(port) <0)
	{
		printf("请使用以下格式:\n");
		printf("stop portNum\n");
		return -1;
	}
	dcs_log(0,0,"停止监听端口[%s] ",port);
	nRet = GetScServerLink(port,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"端口[%s] 监听服务不存在",port);
		printf("端口[%s] 监听服务不存在\n",port);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"端口[%s]  已是停止状态.",port);
		printf("端口[%s]  已是停止状态.\n",port);
		return 0;
	}
	//链路存在,向链路进程发送终止信号
	nRet=kill(pLinkScsCtrl->conns[nRet].pidComm,SIGUSR2);
	dcs_log(0,0,"kill 进程[%d].",pLinkScsCtrl->conns[nRet].pidComm);
	if(nRet == 0)
	{
		dcs_log(0,0,"端口[%s] 监听服务 关闭成功",port);
	}
	else
	{
		dcs_log(0,0,"端口[%s] 监听服务 关闭失败,err:%s",port,strerror(errno));
		printf("端口[%s] 监听服务 关闭失败,err:%s\n",port,strerror(errno));
	}
	
	return 0;
}
/*获取链路状态
*输入参数:port监听的端口号
*输出参数: 如果找到port的链路，将该链路状态放在pFlag
* 返回值: 当找到需要的链路时返回其在共享内存中的位置，否则返回-1
*/
int GetScServerLink(char *port,int *pFlag)
{
	int i;
	
	for(i=0;i<pLinkScsCtrl->i_conn_num;i++)
	{ 
		if(atoi(port) ==pLinkScsCtrl->conns[i].listen_port && strlen(pLinkScsCtrl->conns[i].fold_name) >0 )
		{
			*pFlag = pLinkScsCtrl->conns[i].iStatus;
			return i;
		}
			
	}
	
	return -1;
}


static int OpenscServerCmdLog()
{
    char logfile[256];
    memset(logfile, 0, sizeof(logfile));
    if(u_fabricatefile("log/scServerCmd.log",logfile,sizeof(logfile)) < 0)
        return -1;

    return dcs_log_open(logfile, "scServerCmd");
}


int  StartScServerLink()
{
	char port[6];
	char sBuf[200]={0};
	int nRet = -1;
	int nFlag=-1,nPid=0;
	memset(port,0,sizeof(port));
	
	scanf("%s",port);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);
	if(IsPort(port) <0)
	{
		printf("请使用以下格式:\n");
		printf("start portNum\n");
		return -1;
	}
	dcs_log(0,0,"启动监听端口[%s] ",port);
	nRet = GetScServerLink(port,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"端口[%s] 监听服务不存在",port);
		printf("端口[%s] 监听服务不存在\n",port);
		return 0;
	}
	else if(nFlag !=DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"端口[%s]  已是监听状态了.",port);
		printf("端口[%s]  已是监听状态了.\n",port);
		return 0;
	}
	nPid=StartScServerProcess("shortTcp",port);
	if(nPid < 0)
	{
		dcs_log(0,0,"端口[%s]  监听启动失败.",port);
		printf("端口[%s]  监听启动失败.\n",port);
	}
	dcs_log(0,0,"端口[%s]  监听启动成功.",port);		
	return 0;
}
//添加某个正在监听的端口已经停止的TPDU
static int AddTpdu()
{
	char port[6];
	char tpdu[11];
	char sBuf[200]={0};
	int nRet = 0,i,nFlag=-1;
	
	memset(port, 0, sizeof(port));
	memset(tpdu, 0, sizeof(tpdu));
	scanf("%s %s", port,tpdu);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);
	if(IsPort(port) <0 || IsTpdu(tpdu) < 0)
	{
		printf("请使用以下格式:\n");
		printf("add portNum TPDU\n");
		return -1;
	}
	dcs_log(0,0,"添加TPDU [%s] 到监听端口服务port=[%s] .",tpdu,port);
	//先查找是否是已存在的链路
	nRet = GetScServerLink(port,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"端口[%s] 监听服务不存在,不允许添加新的TPDU",port);
		printf("端口[%s] 监听服务不存在,不允许添加新的TPDU\n",port);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"端口[%s]  已是停止状态,不允许添加新的TPDU.",port);
		printf("端口[%s]  已是停止状态,不允许添加新的TPDU.\n",port);
		return 0;
	}
	
	for(i=0;i<pLinkScsCtrl->i_conn_num;i++)
	{ 
		if((atoi(port) == pLinkScsCtrl->conns[i].listen_port) && memcmp(tpdu,pLinkScsCtrl->conns[i].tpdu,10) ==0)
		{
			dcs_log(0,0,"端口[%s]  TPDU[%s] 的配置已经存在.",port,tpdu);
			printf("端口[%s]  TPDU[%s] 的配置已经存在.\n",port,tpdu);
			return 0;
		}
	}

	//从配置文件中 读出配置信息
	nRet=reload_sc_server_cfg(pLinkScsCtrl,port,tpdu);
	if(nRet <0)
	{
		dcs_log(0,0,"添加TPDU [%s] 失败.",tpdu);
		printf("添加TPDU [%s] 失败.\n",tpdu);
		return -1;
	}
	for(i=0;i<pLinkScsCtrl->i_conn_num;i++)
	{ 
		if((atoi(port) == pLinkScsCtrl->conns[i].listen_port) && memcmp(tpdu,pLinkScsCtrl->conns[i].tpdu,10) ==0)
		{
			pLinkScsCtrl->conns[i].iStatus = DCS_STAT_LISTENING;
			return 0;
		}
	}

	dcs_log(0,0,"添加TPDU [%s] 成功.",tpdu);
	return 0;
}
//停止某个正在监听的端口已经停止的TPDU
int StopTpdu()
{
	char port[6];
	char tpdu[11];
	char sBuf[200]={0};
	int nRet = 0,i,nFlag=-1;
	
	memset(port, 0, sizeof(port));
	memset(tpdu, 0, sizeof(tpdu));
	scanf("%s %s", port,tpdu);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);
	if(IsPort(port) <0 || IsTpdu(tpdu) < 0)
	{
		printf("请使用以下格式:\n");
		printf("add portNum TPDU\n");
		return -1;
	}
	dcs_log(0,0,"停止port=[%s] TPDU [%s] 的端口服务.",port,tpdu);
	nRet = GetScServerLink(port,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"端口[%s] 监听服务不存在",port);
		printf("端口[%s] 监听服务不存在\n",port);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"端口[%s]  已是停止状态.",port);
		printf("端口[%s]  已是停止状态.\n",port);
		return 0;
	}

	for(i=0;i<pLinkScsCtrl->i_conn_num;i++)
	{ 
		if((atoi(port) == pLinkScsCtrl->conns[i].listen_port) && memcmp(tpdu,pLinkScsCtrl->conns[i].tpdu,10) ==0)
		{
			if( pLinkScsCtrl->conns[i].tpduFlag == -1)
			{
				dcs_log(0,0,"端口[%s]  TPDU [%s]已是停止状态.",port,tpdu);
				printf("端口[%s]  TPDU [%s]已是停止状态.\n",port,tpdu);
				return 0;
			}
			else
			{
				pLinkScsCtrl->conns[i].tpduFlag =-1;
				dcs_log(0,0,"停止port=[%s] TPDU [%s] 的端口服务成功.",port,tpdu);
				return 0;
			}	
		}		
	}
	dcs_log(0,0,"停止port=[%s] TPDU [%s] 的端口服务失败,没有相匹配信息.",port,tpdu);
	printf("停止port=[%s] TPDU [%s] 的端口服务失败,没有相匹配信息.\n",port,tpdu);
	return 0;
}
//启动某个正在监听的端口已经停止的TPDU
int  StartTpdu()
{
	char port[6];
	char tpdu[11];
	char sBuf[200]={0};
	int nRet = 0,i,nFlag=-1;
	
	memset(port, 0, sizeof(port));
	memset(tpdu, 0, sizeof(tpdu));
	scanf("%s %s", port,tpdu);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);
	if(IsPort(port) <0 || IsTpdu(tpdu) < 0)
	{
		printf("请使用以下格式:\n");
		printf("add portNum TPDU\n");
		return -1;
	}
	dcs_log(0,0,"启动port=[%s] TPDU [%s] 的端口服务.",port,tpdu);
	nRet = GetScServerLink(port,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"端口[%s] 监听服务不存在",port);
		printf("端口[%s] 监听服务不存在\n",port);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"端口[%s]  已是停止状态.",port);
		printf("端口[%s]  已是停止状态.\n",port);
		return 0;
	}
	for(i=0;i<pLinkScsCtrl->i_conn_num;i++)
	{ 
		if((atoi(port) == pLinkScsCtrl->conns[i].listen_port) && memcmp(tpdu,pLinkScsCtrl->conns[i].tpdu,10) ==0)
		{
			if( pLinkScsCtrl->conns[i].tpduFlag == 1)
			{
				dcs_log(0,0,"端口[%s]  TPDU [%s]已是状态启动.",port,tpdu);
				printf("端口[%s]  TPDU [%s]已是状态启动.\n",port,tpdu);
				return 0;
			}
			else
			{
				pLinkScsCtrl->conns[i].tpduFlag =1;
				dcs_log(0,0,"启动port=[%s] TPDU [%s] 的端口服务成功.",port,tpdu);
				return 0;
			}	
		}		
	}
	dcs_log(0,0,"启动port=[%s] TPDU [%s] 的端口服务失败,,没有相匹配信息.",port,tpdu);
	printf("启动port=[%s] TPDU [%s] 的端口服务失败,,没有相匹配信息.\n",port,tpdu);
	return 0;
}


int IsPort(char * port)
{
	int i;
	if(strlen(port) > 5)
		return -1;
	for(i =0;i<strlen(port);i++)
	{
		if(port[i] <'0'||port[i] >'9')
			return -1;
	}
	return 0;
}
int IsTpdu(char * tpdu)
{
	int i;
	if(strlen(tpdu) != 10)
		return -1;
	for(i =0;i<10;i++)
	{
		if(tpdu[i] <'0'||tpdu[i] >'9')
			return -1;
	}
	return 0;
}


