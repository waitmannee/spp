#include "httpC.h"

static struct httpc_Ast *pLinkHttpcCtrl = NULL;
static void httpCCmdExit(int signo);
static int AddhttpCLink();
static void ListhttpCInfo(int arg_c, char *arg_v[]);
static int reload_httpC_cfg(struct httpc_Ast *pLink, char *pFoldName, int mode);
static int StophttpCLink();
static int GethttpCLink(char *pFoldName, int *pFlag);
static int OpenhttpCCmdLog();
static int  StarthttpCLink();
static int UpdatehttpCLink();
//==========================================

int main(int argc, char *argv[])
{
	catch_all_signals(httpCCmdExit);

	if(OpenhttpCCmdLog()< 0)
		return -1;
	if(pLinkHttpcCtrl == NULL)
	{ 
		pLinkHttpcCtrl = (struct httpc_Ast*)shm_connect(HTTPCLIENT_SHMNAME, NULL);
		if(pLinkHttpcCtrl == NULL)
		{
		    dcs_log(0, 0, "cannot connect SHM '%s':%s!\n", HTTPCLIENT_SHMNAME, strerror(errno));
		    return -1;
		}
	}


	cmd_init("httpCCmd>>", httpCCmdExit);
	cmd_add("list", (cmdfunc_t *)ListhttpCInfo, "list client info");
	cmd_add("start", (cmdfunc_t *)StarthttpCLink, "start a client");
	cmd_add("add", (cmdfunc_t *)AddhttpCLink, "Add a client ");
	cmd_add("stop", (cmdfunc_t *)StophttpCLink, "stop a client");
	cmd_add("update", (cmdfunc_t *)UpdatehttpCLink, "update  a client");
	
	cmd_shell(argc, argv);

	httpCCmdExit(0);
	return 0;
}


static void httpCCmdExit(int signo)
{
	shm_detach((char *)pLinkHttpcCtrl);
	exit(signo);
}

static void ListhttpCInfo(int arg_c, char *arg_v[])
{
	int i = 0;
	char sBuf[200];
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf, sizeof(sBuf), stdin);

	printf("maxNum=%d\n", pLinkHttpcCtrl->i_http_num);
	printf("id     FdName        AppFdName     Status   ServerAddr   timeOut    maxThreadNum\n");
	printf("=============================================================================\n");
	for(; i<pLinkHttpcCtrl->i_http_num; i++)
	{
		if(strlen(pLinkHttpcCtrl->httpc[i].name) > 0)
		{
			printf("[%03d] ", i+1);
			printf("%-10s", pLinkHttpcCtrl->httpc[i].name);
			printf("%-15s", pLinkHttpcCtrl->httpc[i].fold_name);
			
			//link Status
			if((pLinkHttpcCtrl->httpc[i].iStatus == DCS_STAT_RUNNING))
				printf("%-8s ", "running");
			else if(pLinkHttpcCtrl->httpc[i].iStatus == DCS_STAT_STOPPED)
				printf("%-8s ", "stoped");
			else
				printf("%s ", " ");
			printf("%30s  ", pLinkHttpcCtrl->httpc[i].serverAddr);
			printf("%-3d  ", pLinkHttpcCtrl->httpc[i].timeout);
			printf("%-3d", pLinkHttpcCtrl->httpc[i].maxthreadnum);
			printf("\n");
		}
	}
	return ;
}

static int AddhttpCLink()
{
	char sFoldName[20+1];
	char sBuf[200];
	int nRet = 0, nFlag = 0;
	
	memset(sFoldName, 0, sizeof(sFoldName));
	scanf("%s", sFoldName);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf, sizeof(sBuf), stdin);
	
	dcs_log(0, 0, "添加http客户端, folderName=[%s]", sFoldName);
	//先查找是否是已存在的链路
	nRet = GethttpCLink(sFoldName, &nFlag);
	if(nRet >= 0)
	{
		dcs_log(0, 0, "http客户端服务已经存在, folderName=[%s]", sFoldName);
		printf("http客户端服务已经存在,folderName=[%s]\n", sFoldName);
		return -1;
	}
	//不存在的话，从配置文件中 读出配置信息
	nRet=reload_httpC_cfg(pLinkHttpcCtrl, sFoldName, 1);
	if(nRet <0)
	{
		return -1;
	}

	nRet = Start_Process("lhttpC", sFoldName);
	if(nRet < 0)
	{
		dcs_log(0, 0, "http客户端服务folderName=[%s]添加失败", sFoldName);
		printf("http客户端服务folderName=[%s]添加失败\n", sFoldName);
		return -1;
	}
	dcs_log(0, 0, "http客户端服务folderName=[%s]添加成功", sFoldName);
	return 0;
}

/*重新导入配置文件
*mode =1表示是在共享内存的后面添加fold名为pFoldName的新纪录,
*mode =0 表示更新共享内存中fold名为pFoldName的已有记录的内容
*/
static int reload_httpC_cfg(struct httpc_Ast *pLink, char *pFoldName, int mode)
{
	char caBuffer[512];
	int iFd=-1, iRc = 0, iSlotIndex =0, i = 0, iCnt = 0;
	char cfgfile[256];

	char *name = NULL, *fold_name = NULL, *msgtype = NULL, *serveraddr = NULL, *timeout = NULL, *MaxthreadNum = NULL;
	int nFlag = 0;
	int myFid = 0;
	
	memset(caBuffer, 0, sizeof(caBuffer));
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
	
	//读取配置项中的配置项,不可去掉
	if(0 > conf_get_first_number(iFd, "conn.max", &iCnt) || iCnt < 0 )
	{
		dcs_log(0, 0, "<httpCCmd> load config item 'conn.max' failed!");
		conf_close(iFd);
		return -1;
	}

	
	//mode =1 时需要在共享内存的最后添加内容，所以要找到最后一条空记录
	if(mode == 1)
	{
		for(; iSlotIndex<pLinkHttpcCtrl->i_http_num; iSlotIndex++)
		{
			if(strlen(pLinkHttpcCtrl->httpc[iSlotIndex].name) == 0)
			{
				break;
			}
		}
		
		if(iSlotIndex == pLinkHttpcCtrl->i_http_num )
		{
			dcs_log(0, 0, "共享内存没有空余空间存储新的配置信息, 最多%d条", iSlotIndex);
			printf("共享内存没有空余空间存储新的配置信息, 最多%d条\n", iSlotIndex);
			return -1;
		}
	}

	iRc = conf_get_first_string(iFd, "conn", caBuffer);
	for(; iRc == 0 && i < pLinkHttpcCtrl->i_http_num; iRc = conf_get_next_string(iFd, "conn", caBuffer))
	{
		name = strtok(caBuffer, " \t\r");		//链路FOLDER名字
		fold_name = strtok(NULL, " \t\r");	//链路对应业务处理进程的名字
		msgtype = strtok(NULL, " \t\r");	//报文类型
		serveraddr = strtok(NULL, " \t\r");
		timeout = strtok(NULL, " \t\r");	//超时时间
		MaxthreadNum = strtok(NULL , "  \t\r");	//设置每条http最大的线程数

		if(strcmp(name, pFoldName) != 0)
		{
			i++;
			continue;
		}
		//如果是更新,需要找到共享内存中的位置
		if(mode == 0)
		{
			iSlotIndex = GethttpCLink(pFoldName, &iRc);
		}
		nFlag = 1;	//找到配置

		dcs_log(0, 0, "name = [%s], fold_name = [%s], msgtype = [%s], serveraddr = [%s], timeout = [%s], MaxthreadNum =[%s]", \
		name, fold_name, msgtype, serveraddr, timeout, MaxthreadNum);

		if (name == NULL || fold_name == NULL ||msgtype == NULL ||  serveraddr == NULL 
		|| timeout == NULL || MaxthreadNum == NULL )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}

		strcpy(pLinkHttpcCtrl->httpc[iSlotIndex].name, name);
		strcpy(pLinkHttpcCtrl->httpc[iSlotIndex].fold_name, fold_name);
		strcpy(pLinkHttpcCtrl->httpc[iSlotIndex].msgtype, msgtype);
		strcpy(pLinkHttpcCtrl->httpc[iSlotIndex].serverAddr, serveraddr);

		pLinkHttpcCtrl->httpc[iSlotIndex].timeout= atoi(timeout);
		pLinkHttpcCtrl->httpc[iSlotIndex].maxthreadnum = atoi(MaxthreadNum);
		
		dcs_log(0, 0, "p_conn_c_Ctrl->linkArray[%d].name = %s", iSlotIndex, pLinkHttpcCtrl->httpc[iSlotIndex].name);
		//caFoldName
		myFid = fold_locate_folder(name);
		if(myFid < 0) 
			myFid = fold_create_folder(name);

		if(myFid < 0)
		{
			dcs_log(0, 0, "cannot create folder '%s':%s", name, ise_strerror(errno));
		}
		
		pLinkHttpcCtrl->httpc[iSlotIndex].iFid = myFid;
		
		myFid = fold_locate_folder(fold_name);
		if(myFid < 0) 
			myFid = fold_create_folder(fold_name);

		if(myFid < 0)
		{
			dcs_log(0, 0, "cannot create folder '%s':%s", fold_name, ise_strerror(errno));
		}
		pLinkHttpcCtrl->httpc[iSlotIndex].iApplFid = myFid;
		
		conf_close(iFd);
		return iSlotIndex;
	}
	dcs_log(0, 0, "reload config end");
	conf_close(iFd);
	if(nFlag ==0 )
	{
		dcs_log(0, 0, "没有该folder [%s] 的配置信息", pFoldName);
		printf("没有该folder [%s] 的配置信息\n", pFoldName);
		return -1;
	}
	return 0;
}

/*
关闭相关http服务
*/
int StophttpCLink()
{
	char sFoldName[20+1];
	char sBuf[200];
	int nRet = 0;
	int nFlag=-1;
	
	memset(sFoldName, 0, sizeof(sFoldName));
	
	scanf("%s", sFoldName);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf, sizeof(sBuf), stdin);

	dcs_log(0, 0, "停止http客户端[%s]服务", sFoldName);
	nRet = GethttpCLink(sFoldName, &nFlag);
	if(nRet == -1)
	{
		dcs_log(0, 0, "http客户端[%s]服务不存在", sFoldName);
		printf("http客户端[%s]服务不存在\n", sFoldName);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0, 0, "http客户端[%s]服务已是停止状态", sFoldName);
		printf("http客户端[%s]服务已是停止状态\n", sFoldName);
		return 0;
	}
	//链路存在,向链路进程发送终止信号
	nRet = kill(pLinkHttpcCtrl->httpc[nRet].pidComm, SIGUSR2);
	if(nRet == 0)
	{
		dcs_log(0, 0, "http客户端[%s]服务关闭成功", sFoldName);
	}
	else
	{
		dcs_log(0, 0, "http客户端[%s]服务关闭失败", sFoldName);
		printf("http客户端[%s]服务关闭失败\n", sFoldName);
	}
	
	return 0;
}
/*获取链路状态
*输入参数:http客户端的FoldName
*输出参数: 如果找到FoldName的链路，将该链路状态放在pFlag
* 返回值: 当找到需要的链路时返回其在共享内存中的位置，否则返回-1
*/
int GethttpCLink(char *pFoldName, int *pFlag)
{
	int i = 0;
	
	for(; i<pLinkHttpcCtrl->i_http_num; i++)
	{ 
		if(strcmp(pLinkHttpcCtrl->httpc[i].name, pFoldName) == 0)
		{
			*pFlag = pLinkHttpcCtrl->httpc[i].iStatus;
			return i;
		}	
	}
	return -1;
}


static int OpenhttpCCmdLog()
{
    char logfile[256];
	
    memset(logfile, 0, sizeof(logfile));
	
    if(u_fabricatefile("log/lhttpCCmd.log", logfile, sizeof(logfile)) < 0)
        return -1;

    return dcs_log_open(logfile, "httpCCmd");
}


int  StarthttpCLink()
{
	char sFoldName[20+1];
	char sBuf[200];
	int nRet = -1;
	int nFlag=-1;
	memset(sFoldName, 0, sizeof(sFoldName));
	
	scanf("%s", sFoldName);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf, sizeof(sBuf), stdin);

	dcs_log(0, 0, "启动http客户端[%s]服务", sFoldName);
	nRet = GethttpCLink(sFoldName, &nFlag);
	if(nRet == -1)
	{
		dcs_log(0, 0, "http客户端[%s]服务不存在", sFoldName);
		printf("http客户端[%s]服务不存在\n", sFoldName);
		return 0;
	}
	else if(nFlag !=DCS_STAT_STOPPED)
	{
		dcs_log(0, 0, "http客户端[%s]服务已经在运行", sFoldName);
		printf("http客户端[%s]服务已经在运行\n", sFoldName);
		return 0;
	}
	nRet=Start_Process("lhttpC", sFoldName);
	if(nRet < 0)
	{
		dcs_log(0, 0, "http客户端[%s]服务启动失败", sFoldName);
		printf("http客户端[%s]服务启动失败\n", sFoldName);
	}
	dcs_log(0, 0, "http客户端[%s]服务启动成功", sFoldName);
	return 0;
}

/*更新相关http服务
*/
int UpdatehttpCLink()
{
	char sFoldName[20+1];
	char sBuf[200];
	int nRet = 0;
	
	memset(sFoldName, 0, sizeof(sFoldName));
	int nFlag=-1;
	
	scanf("%s", sFoldName);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf, sizeof(sBuf), stdin);

	dcs_log(0, 0, "更新http客户端[%s]服务", sFoldName);
	nRet = GethttpCLink(sFoldName, &nFlag);
	if(nRet == -1)
	{
		dcs_log(0, 0, "http客户端[%s]服务不存在", sFoldName);
		printf("http客户端[%s]服务不存在\n", sFoldName);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0, 0, "http客户端[%s]服务已是停止状态", sFoldName);
		printf("http客户端[%s]服务已是停止状态\n", sFoldName);
		return 0;
	}
	//链路存在,向链路进程发送终止信号
	nRet = kill(pLinkHttpcCtrl->httpc[nRet].pidComm, SIGUSR2);
	if(nRet == 0)
	{
		dcs_log(0, 0, "http客户端[%s]服务关闭成功", sFoldName);
	}
	else
	{
		dcs_log(0, 0, "http客户端[%s]服务关闭失败", sFoldName);
		printf("http客户端[%s]服务关闭失败\n", sFoldName);
	}
	//重新从配置文件中读出相应配置信息
	nRet = reload_httpC_cfg(pLinkHttpcCtrl, sFoldName, 0);
	if(nRet <0)
	{
		dcs_log(0, 0, "更新http客户端[%s]服务失败", sFoldName);
		printf("更新http客户端[%s]服务失败\n", sFoldName);
		return -1;
	}
	sleep(1);// 等待一秒，等待kill信号量先被处理
	//启动
	nRet = Start_Process("lhttpC", sFoldName);
	if(nRet < 0)
	{
		dcs_log(0, 0, "http客户端[%s]服务启动失败", sFoldName);
		printf("http客户端[%s]服务启动失败\n", sFoldName);
	}
	dcs_log(0, 0, "更新http客户端[%s]服务成功", sFoldName);
	return 0;
}



