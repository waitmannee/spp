#include "short_conn_client.h"


static struct conn_c_Ast *pLinkSccCtrl = NULL;
static void scClientCmdExit(int signo);
static int AddScClientLink();
static void ListScClientInfo(int arg_c, char *arg_v[]);
static int reload_sc_Client_cfg(struct conn_c_Ast *pLink,char *pFoldName,int mode);
static int StopScClientLink();
static int GetScClientLink(char *pFoldName,int *pFlag);
static int OpenscClientCmdLog();
static int  StartScClientLink();
static int UpdateScClientLink();
//==========================================

int main(int argc, char *argv[])
{
	
	catch_all_signals(scClientCmdExit);

	if(OpenscClientCmdLog()< 0)
		return -1;
	if(pLinkSccCtrl == NULL)
	{ 
		pLinkSccCtrl = (struct conn_c_Ast*)shm_connect(SHORT_CONN_CLIENT_SHM_NAME, NULL);
		if(pLinkSccCtrl == NULL)
		{
		    dcs_log(0, 0, "cannot connect SHM '%s':%s!\n", SHORT_CONN_CLIENT_SHM_NAME, strerror(errno));
		    return -1;
		}
	}


	cmd_init("scClientCmd>>",scClientCmdExit);
	cmd_add("list", (cmdfunc_t *)ListScClientInfo, "list client info");
	cmd_add("start", (cmdfunc_t *)StartScClientLink, "start a client");
	cmd_add("add", (cmdfunc_t *)AddScClientLink, "Add a client ");
	cmd_add("stop", (cmdfunc_t *)StopScClientLink, "stop a client");
	cmd_add("update", (cmdfunc_t *)UpdateScClientLink, "update  a client");
	
	cmd_shell(argc,argv);

	scClientCmdExit(0);
	return 0;
}


static void scClientCmdExit(int signo)
{
	shm_detach((char *)pLinkSccCtrl);
	exit(signo);
}

static void ListScClientInfo(int arg_c, char *arg_v[])
{
	int i = 0;
	char sBuf[200]={0};
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	printf("maxNum=%d\n",pLinkSccCtrl->i_conn_num);
	printf("id     FdName        AppFdName     Status   ServerAddr   timeOut    maxThreadNum\n");
	printf("=============================================================================\n");
	for(i = 0; i<pLinkSccCtrl->i_conn_num; i++)
	{
		if(strlen(pLinkSccCtrl->connc[i].name) > 0)
		{
			printf("[%03d] ", i+1);
			printf("%-15s", pLinkSccCtrl->connc[i].name);
			printf("%-15s", pLinkSccCtrl->connc[i].fold_name);
			
			//link Status
			if((pLinkSccCtrl->connc[i].iStatus == DCS_STAT_RUNNING ))
				printf("%-8s ", "running");
			else if(pLinkSccCtrl->connc[i].iStatus ==DCS_STAT_STOPPED)
				printf("%-8s ", "stoped");
			else
				printf("%s ", " ");

			printf("%15s:%-5d  ", pLinkSccCtrl->connc[i].server_ip,pLinkSccCtrl->connc[i].server_port);
			printf("%-3d  ", pLinkSccCtrl->connc[i].timeout);
			printf("%-3d", pLinkSccCtrl->connc[i].maxthreadnum);
			printf("\n");
		}
	}
	return ;
}

static int AddScClientLink()
{
	char sFoldName[20+1];
	char sBuf[200]={0};
	int nRet = 0, nFlag = 0;
	
	memset(sFoldName, 0, sizeof(sFoldName));
	scanf("%s", sFoldName);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);
	
	dcs_log(0,0,"添加短链客户端,folderName=[%s]",sFoldName);
	//先查找是否是已存在的链路
	nRet = GetScClientLink(sFoldName,&nFlag);
	if(nRet >= 0)
	{
		dcs_log(0,0,"短链客户端 服务已经存在,folderName=[%s]",sFoldName);
		printf("短链客户端 服务已经存在,folderName=[%s]\n",sFoldName);
		return -1;
	}
	//不存在的话，从配置文件中 读出配置信息
	nRet=reload_sc_Client_cfg(pLinkSccCtrl,sFoldName,1);
	if(nRet <0)
	{
		return -1;
	}

	nRet=Start_Process("shortClient",sFoldName);
	if(nRet < 0)
	{
		dcs_log(0,0,"短链客户端 服务folderName=[%s]添加失败",sFoldName);
		printf("短链客户端 服务folderName=[%s]添加失败\n",sFoldName);
		return -1;
	}
	dcs_log(0,0,"短链客户端 服务folderName=[%s]添加成功",sFoldName);
	
	return 0;
}

/*重新导入配置文件
*mode =1表示是在共享内存的后面添加fold名为pFoldName的新纪录,
*mode =0 表示更新共享内存中fold名为pFoldName的已有记录的内容
*/
static int reload_sc_Client_cfg(struct conn_c_Ast *pLink,char *pFoldName,int mode)
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iSlotIndex =0 ,i, iCnt;
	char cfgfile[256];

	char *commType  =NULL, *name = NULL,*fold_name = NULL,*msgtype = NULL,*server_ip = NULL, * timeout = NULL, *MaxthreadNum = NULL;
	int server_port = 0;

	char sLocalPort[20];
	char sRemotePort[20];

	int nFlag=0;

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

	
	//mode =1 时需要在共享内存的最后添加内容，所以要找到最后一条空记录
	if(mode == 1)
	{
		for(iSlotIndex=0;iSlotIndex<pLinkSccCtrl->i_conn_num;iSlotIndex++)
		{
			if(strlen(pLinkSccCtrl->connc[iSlotIndex].name) == 0)
			{
				break;
			}
		}
		
		if(iSlotIndex ==pLinkSccCtrl->i_conn_num )
		{
			dcs_log(0,0,"共享内存没有空余空间存储新的配置信息,最多%d条",iSlotIndex);
			printf("共享内存没有空余空间存储新的配置信息,最多%d条\n",iSlotIndex);
			return -1;
		}
	}

	iRc = conf_get_first_string(iFd, "conn",caBuffer);
	for(i=0 ;iRc == 0 && i < pLinkSccCtrl->i_conn_num;iRc = conf_get_next_string(iFd, "conn",caBuffer))
	{
		//dcs_log(0, 0, "iRc=%d, caBuffer = %s", iRc, caBuffer);

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

		if(strcmp(name, pFoldName) != 0)
		{
			i++;
			continue;
		}
		//如果是更新,需要找到共享内存中的位置
		if(mode == 0)
		{
			iSlotIndex =GetScClientLink(pFoldName,&iRc);
		}
		nFlag =1;//找到配置

		dcs_log(0, 0, "commType = [%s], name = [%s], fold_name = [%s], msgtype = [%s], server_ip = [%s], server_port = [%d],timeout = [%s], MaxthreadNum =[%s]\n",
			commType,name,fold_name,msgtype,server_ip, server_port, timeout, MaxthreadNum);

		
		if (commType == NULL ||name == NULL || fold_name == NULL ||msgtype == NULL ||  server_ip == NULL || server_port == 0 || 
			timeout == NULL || MaxthreadNum == NULL )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}

		strcpy(pLinkSccCtrl->connc[iSlotIndex].commType,commType);
		strcpy(pLinkSccCtrl->connc[iSlotIndex].name,name);
		strcpy(pLinkSccCtrl->connc[iSlotIndex].fold_name,fold_name);
		strcpy(pLinkSccCtrl->connc[iSlotIndex].msgtype,msgtype);
		strcpy(pLinkSccCtrl->connc[iSlotIndex].server_ip,server_ip);

		pLinkSccCtrl->connc[iSlotIndex].server_port = server_port;
		pLinkSccCtrl->connc[iSlotIndex].timeout= atoi(timeout);
		pLinkSccCtrl->connc[iSlotIndex].maxthreadnum = atoi(MaxthreadNum);
		
		dcs_log(0, 0, "p_conn_c_Ctrl->linkArray[%d].name = %s", iSlotIndex, pLinkSccCtrl->connc[iSlotIndex].name);
		//caFoldName
		int myFid = fold_locate_folder(name);
		if(myFid < 0) 
			myFid = fold_create_folder(name);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s\n",name, ise_strerror(errno) );
		}
		
		pLinkSccCtrl->connc[iSlotIndex].iFid =myFid;
		
		myFid = fold_locate_folder(fold_name);
		if(myFid < 0) 
			myFid = fold_create_folder(fold_name);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s\n",fold_name, ise_strerror(errno) );
		}
		pLinkSccCtrl->connc[iSlotIndex].iApplFid =myFid;
		
		iSlotIndex++;
		i++;
	}
	dcs_log(0,0,"reload config end");
	conf_close(iFd);
	if(nFlag ==0 )
	{
		dcs_log(0,0,"没有该folder [%s] 的配置信息",pFoldName);
		printf("没有该folder [%s] 的配置信息\n",pFoldName);
		return -1;
	}
	return 0;
}
/*关闭相关短链服务
*/
int StopScClientLink()
{
	char sFoldName[20+1];
	char sBuf[200]={0};
	int nRet = 0,i,nPid;
	
	memset(sFoldName, 0, sizeof(sFoldName));
	int nFlag=-1;
	
	scanf("%s",sFoldName);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	dcs_log(0,0,"停止客户端[%s] 服务",sFoldName);
	nRet = GetScClientLink(sFoldName,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"客户端[%s] 服务不存在",sFoldName);
		printf("客户端[%s] 服务不存在\n",sFoldName);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"客户端[%s] 服务已是停止状态",sFoldName);
		printf("客户端[%s] 服务已是停止状态\n",sFoldName);
		return 0;
	}
	//链路存在,向链路进程发送终止信号
	nRet=kill(pLinkSccCtrl->connc[nRet].pidComm,SIGUSR2);
	dcs_log(0,0,"kill 进程[%d].",pLinkSccCtrl->connc[nRet].pidComm);
	if(nRet == 0)
	{
		dcs_log(0,0,"客户端[%s] 服务关闭成功",sFoldName);
	}
	else
	{
		dcs_log(0,0,"客户端[%s] 服务关闭失败,err:%s",sFoldName,strerror(errno));
		printf("客户端[%s] 服务关闭失败,err:%s\n",sFoldName,strerror(errno));
	}
	
	return 0;
}
/*获取链路状态
*输入参数:客户端的FoldName
*输出参数: 如果找到FoldName的链路，将该链路状态放在pFlag
* 返回值: 当找到需要的链路时返回其在共享内存中的位置，否则返回-1
*/
int GetScClientLink(char *pFoldName,int *pFlag)
{
	int i;
	
	for(i=0;i<pLinkSccCtrl->i_conn_num;i++)
	{ 
		if(strcmp(pLinkSccCtrl->connc[i].name, pFoldName) == 0)
		{
			*pFlag = pLinkSccCtrl->connc[i].iStatus;
			return i;
		}
			
	}
	
	return -1;
}


static int OpenscClientCmdLog()
{
    char logfile[256];
    memset(logfile, 0, sizeof(logfile));
    if(u_fabricatefile("log/scClientCmd.log",logfile,sizeof(logfile)) < 0)
        return -1;

    return dcs_log_open(logfile, "scClientCmd");
}


int  StartScClientLink()
{
	char sFoldName[20+1];
	char sBuf[200]={0};
	int nRet = -1;
	int nFlag=-1,nPid=0;
	memset(sFoldName,0,sizeof(sFoldName));
	
	scanf("%s",sFoldName);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	dcs_log(0,0,"启动客户端[%s] 服务",sFoldName);
	nRet = GetScClientLink(sFoldName,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"客户端[%s] 服务不存在",sFoldName);
		printf("客户端[%s] 服务不存在\n",sFoldName);
		return 0;
	}
	else if(nFlag !=DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"客户端[%s] 服务已经在运行",sFoldName);
		printf("客户端[%s] 服务已经在运行\n",sFoldName);
		return 0;
	}
	nRet=Start_Process("shortClient",sFoldName);
	if(nRet < 0)
	{
		dcs_log(0,0,"客户端[%s] 服务启动失败",sFoldName);
		printf("客户端[%s] 服务启动失败\n",sFoldName);
	}
	dcs_log(0,0,"客户端[%s] 服务启动成功",sFoldName);
	return 0;
}

/*更新相关短链服务
*/
int UpdateScClientLink()
{
	char sFoldName[20+1];
	char sBuf[200]={0};
	int nRet = 0;
	int nFlag=-1;

	memset(sFoldName, 0, sizeof(sFoldName));
	
	scanf("%s",sFoldName);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	dcs_log(0,0,"更新客户端[%s] 服务",sFoldName);
	nRet = GetScClientLink(sFoldName,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"客户端[%s] 服务不存在",sFoldName);
		printf("客户端[%s] 服务不存在\n",sFoldName);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"客户端[%s] 服务已是停止状态",sFoldName);
		printf("客户端[%s] 服务已是停止状态\n",sFoldName);
		return 0;
	}
	
	//重新从配置文件中 读出相应配置信息
	nRet=reload_sc_Client_cfg(pLinkSccCtrl,sFoldName,0);
	if(nRet <0)
	{
		dcs_log(0,0,"更新客户端[%s] 服务失败",sFoldName);
		printf("更新客户端[%s] 服务失败\n",sFoldName);
		return -1;
	}	
	dcs_log(0,0,"更新客户端[%s] 服务成功",sFoldName);
	return 0;
}



