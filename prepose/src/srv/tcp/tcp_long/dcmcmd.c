#include "ibdcs.h"
#include "extern_function.h"
#include "dccdcs.h"
#define   DCS_FLAG_NACREADER      0x00000004

static char* g_proctab_[]=
{
    "dcxhstcp",   //makefile: create by from dcmhsttcp.c
    NULL
};

static struct hstcom *pLinksCtrl = NULL;
//forward declaration =======================
static void dcsCmdExit(int signo);
static int AddHstLink();
static void ListhstInfo(int arg_c, char *arg_v[]);
static int StartProcess(const char *strExeFile, const char *arg1, const char *arg2);
static int reload_hst_server_cfg(struct hstcom *pLink,char *pFoldName,char * sFoldNum,int mode);
static int StopHstLink();
static int GetLink(char * pFoldName,char *pCommNO,int *pFlag);
static int OpenDcmCmdLog();
static int  StartHstLink();
static int UpdateHstLink();
//==========================================

int main(int argc, char *argv[])
{	
	catch_all_signals(dcsCmdExit);

	if(OpenDcmCmdLog()< 0)
		return -1;
	if(pLinksCtrl == NULL)
	{ 
		pLinksCtrl = (struct hstcom *)shm_connect(DCS_HSTCOM_NAME, NULL);
		if(pLinksCtrl == NULL)
		{
		    dcs_log(0, 0, "cannot connect SHM '%s':%s!\n", DCS_HSTCOM_NAME, strerror(errno));
		    return -1;
		}
	}

	cmd_init("dcxCmd>>",dcsCmdExit);
	cmd_add("list", (cmdfunc_t *)ListhstInfo, "list link info");
	cmd_add("start", (cmdfunc_t *)StartHstLink, "start linkName --start a link");
	cmd_add("add", (cmdfunc_t *)AddHstLink, "Add linkName --add a link");
	cmd_add("stop", (cmdfunc_t *)StopHstLink, "stop linkName --stop a link");
	cmd_add("update", (cmdfunc_t *)UpdateHstLink, "update linkName --update a link");
	cmd_shell(argc,argv);

	dcsCmdExit(0);
	return 0;
}


static void dcsCmdExit(int signo)
{
	shm_detach((char *)pLinksCtrl);
	exit(signo);
}

static void ListhstInfo(int arg_c, char *arg_v[])
{
	int i = 0;
	char sBuf[200]={0};
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	printf("id    FdName        CommNo  AppFid  Role  Status  LocalPort  RemoteAddr    HbFlg  HbTime\n");
	printf("=======================================================================================\n");
	for(i = 0; i<pLinksCtrl->iLinkCnt; i++)
	{
		if(strlen(pLinksCtrl->linkArray[i].caFoldName) > 0)
		{
			printf("[%03d] ", i+1);
			printf("%-15s", pLinksCtrl->linkArray[i].caFoldName);
			printf("%-3s ", pLinksCtrl->linkArray[i].commNO);
			printf("%03d ", pLinksCtrl->linkArray[i].iApplFid);
			
			//link Role
			if(pLinksCtrl->linkArray[i].iRole == DCS_ROLE_PASSIVE)
				printf("%-11s", "passive");
			else if(pLinksCtrl->linkArray[i].iRole == DCS_ROLE_ACTIVE)
				printf("%-11s", "active");
			else if(pLinksCtrl->linkArray[i].iRole == DCS_ROLE_SIMPLEXSVR)
				printf("%-11s", "simplexsvr");
			else if(pLinksCtrl->linkArray[i].iRole == DCS_ROLE_SIMPLEXCLI)
				printf("%-11s", "simplexcli");

			//link Status
			if(pLinksCtrl->linkArray[i].iStatus == DCS_STAT_DISCONNECTED)
				printf("%-8s ", "disconct");
			else if(pLinksCtrl->linkArray[i].iStatus == DCS_STAT_CALLING)
				printf("%-8s ", "call");
			else if(pLinksCtrl->linkArray[i].iStatus == DCS_STAT_LISTENING)
				printf("%-8s ", "listen");
			else if(pLinksCtrl->linkArray[i].iStatus == DCS_STAT_CONNECTED)
				printf("%-8s ", "connct");
			else if(pLinksCtrl->linkArray[i].iStatus ==DCS_STAT_STOPPED)
				printf("%-8s ", "stoped");
			else
				printf("%s ", " ");
			printf(":%-5d ", pLinksCtrl->linkArray[i].sLocalPort);

			//IP and Port
			if(pLinksCtrl->linkArray[i].iRole == DCS_ROLE_SIMPLEXCLI)
			{
				memset(sBuf, 0, sizeof(sBuf));
				sprintf(sBuf,"%s:%d",pLinksCtrl->linkArray[i].caRemoteIp,pLinksCtrl->linkArray[i].sRemotePort);
				printf("%-21s ", sBuf);
			}
			else if(strlen(pLinksCtrl->linkArray[i].sActualIp) == 0)
			{ 
				printf("                      ");
			}
			else
			{	
				memset(sBuf, 0, sizeof(sBuf));
				sprintf(sBuf,"%s:%d",pLinksCtrl->linkArray[i].sActualIp,pLinksCtrl->linkArray[i].nActualPort);
				printf("%-21s ", sBuf);
			}
			
			printf("%c   ", pLinksCtrl->linkArray[i].heart_check_flag);
			printf("%d", pLinksCtrl->linkArray[i].heart_check_time);
			printf("\n");
		}
	}
	return ;
}
/*添加相关链路服务
*/
static int AddHstLink()
{
	char sFoldName[20];
	char sFoldNum[4];
	char sBuf[200]={0};
	int nRet = 0,i,nPid;
	
	memset(sFoldName,0,sizeof(sFoldName));
	memset(sFoldNum,0,sizeof(sFoldNum));
	
	scanf("%s %s",sFoldName,sFoldNum);
		
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);
	dcs_log(0,0,"添加链路 CommName=[%s],CommNo=[%s] ",sFoldName,sFoldNum);

	//先查找是否是已存在的链路
	for(i=0;i<pLinksCtrl->iLinkCnt;i++)
	{ 
		if( strcmp(sFoldName,pLinksCtrl->linkArray[i].caFoldName)==0  
		    && strcmp(sFoldNum,pLinksCtrl->linkArray[i].commNO)==0 )
			break;
	}

	if(i < pLinksCtrl->iLinkCnt)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 已经存在",sFoldName,sFoldNum);
		printf("链路 CommName=[%s],CommNo=[%s] 已经存在\n",sFoldName,sFoldNum);
		return -1;
	}

	//不存在的话，从配置文件中 读出配置信息
	nRet=reload_hst_server_cfg(pLinksCtrl,sFoldName,sFoldNum,1);
	if(nRet <0)
	{
		return -1;
	}

	nPid=StartProcess(g_proctab_[0],sFoldName,sFoldNum);
	if(nPid < 0)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 启动失败",sFoldName,sFoldNum);
		printf("链路 CommName=[%s],CommNo=[%s] 启动失败\n",sFoldName,sFoldNum);
	}
	pLinksCtrl->linkArray[nRet].pidComm = nPid;
	dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 启动成功",sFoldName,sFoldNum);
	
	return 0;
}

static int StartProcess(const char *strExeFile, const char *arg1, const char *arg2)
{
	int pidChld = -1, ret;
	char path_exe[512];
	
	pidChld = fork();
	if(pidChld < 0)
	{ 
		dcs_log(0,0,"cannot fork child '%s'!\n",strExeFile);
		return -1;
	}

	if(pidChld > 0)
	{
		return pidChld;
	}
	u_sleep_us(10000);/*休息10ms等待父进程先返回*/
	memset(path_exe, 0x00, sizeof(path_exe));
	sprintf(path_exe, "%s/run/bin/%s", getenv("EXECHOME"),strExeFile);

	ret = execlp (path_exe, strExeFile,arg1, arg2,(char *)0);
	if(ret < 0)
	{ 
		dcs_log(0,0,"cannot exec executable '%s':%s!",strExeFile,strerror(errno));
		exit(249);
	}

	return 0;
}

/*重新导入配置文件
*mode =1表示是在共享内存的后面添加fold名为pFoldName,序号为sFoldNum的新纪录,
*mode =0 表示更新共享内存中fold名为pFoldName,序号为sFoldNum的已有记录的内容
* 返回值: 成功返回在共享内存中的位置 失败返回-1
*/
static int reload_hst_server_cfg(struct hstcom *pLink,char *pFoldName,char * sFoldNum,int mode)
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iSlotIndex,i,iCnt,nFid=-1;
	char cfgfile[256];

	char *caFoldName = NULL,*commType = NULL,*commNO = NULL,*iRole = NULL,*caLocalIp = NULL,*caRemoteIp = NULL,
	*iTimeOut = NULL,*iNotifyFlag  = NULL,*caMsgType = NULL,*caNotiFold = NULL, *heart_check_flag = NULL, 
	*heart_check_time = NULL;

	char sLocalPort[20];
	char sRemotePort[20];
	char dst_ip[256];
	char dst_port[20];
	int nFlag=0;

	if(u_fabricatefile("config/isdcmlnk.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'isdcmlnk.conf'\n");
		return -1;
	}

	dcs_log(0,0,"cfgfile full path name %s\n", cfgfile);

	if(0 > (iFd = conf_open(cfgfile)) )
	{
		dcs_log(0,0,"open file '%s' failed.\n",cfgfile);
		return -1;
	}

	//读取配置项中的配置项
	if(0 > conf_get_first_number(iFd,"comm.max",&iCnt)|| iCnt < 0 )
	{
		dcs_log(0,0,"<dcxcmd> load config item 'comm.max' failed!");
		conf_close(iFd);
		return -1;
	}

	/*mode =1 时需要在共享内存的最后添加内容，所以要找到最后一条空记录*/
	if(mode == 1)
	{
		for(iSlotIndex=0;iSlotIndex<pLinksCtrl->iLinkCnt;iSlotIndex++)
		{
			if(strlen(pLinksCtrl->linkArray[iSlotIndex].caFoldName) == 0)
			{
				break;
			}
		}
		
		if(iSlotIndex ==pLinksCtrl->iLinkCnt )
		{
			dcs_log(0,0,"共享内存没有空余空间存储新的配置信息");
			return -1;
		}
	}

	iRc = conf_get_first_string(iFd, "comm",caBuffer);
	for(i=0 ;iRc == 0 && i < pLinksCtrl->iLinkCnt;iRc = conf_get_next_string(iFd, "comm",caBuffer))
	{
		commType   = strtok(caBuffer," \t\r");  /*通道类型*/
		commNO      = strtok(NULL," \t\r");      /*通道顺序号*/
		caFoldName  = strtok(NULL," \t\r");    /*该ink文件夹名字*/

		if(strcmp(caFoldName,pFoldName) != 0 || strcmp(commNO,sFoldNum) != 0)
		{
			i++;
			continue;
		}	
		//如果是更新,需要找到共享内存中的位置
		if(mode == 0)
		{
			iSlotIndex =GetLink(pFoldName, sFoldNum, &iRc);
		}

		nFlag =1;/*找到配置*/
		
		caNotiFold    = strtok(NULL," \t\r");     /* 对应业务文件夹名字*/
		iRole    = strtok(NULL," \t\r");      /* 该link在连接建立过程中的角色*/
		caLocalIp         = strtok(NULL," \t\r");       /* 本地端口号  */
		caRemoteIp    = strtok(NULL   ," \t\r");/* 远地端口号*/
		caMsgType   = strtok(NULL   ," \t\r");   /* 消息类型*/
		iTimeOut     = strtok(NULL   ," \t\r");   /* 超时*/
		iNotifyFlag      = strtok(NULL   ," \t\r");  /* 链路状态通知标志*/
		heart_check_flag = strtok(NULL, " \t\r"); /* 心跳包检测标识*/
		heart_check_time = strtok(NULL, " \t\r"); /* 心跳 包检测时间*/

		memset(dst_ip, 0x00, sizeof(dst_ip));
		memset(dst_port, 0x00, sizeof(dst_port));
		util_split_ip_with_port_address(caLocalIp, dst_ip, dst_port);
		memset(caLocalIp, 0x00, strlen(caLocalIp));
		memcpy(caLocalIp, dst_ip, strlen(dst_ip));
		memset(sLocalPort, 0x00, sizeof(sLocalPort));
		memcpy(sLocalPort, dst_port, strlen(dst_port));

		memset(dst_ip, 0x00, sizeof(dst_ip));
		memset(dst_port, 0x00, sizeof(dst_port));
		util_split_ip_with_port_address(caRemoteIp, dst_ip, dst_port);
		memset(caRemoteIp, 0x00, strlen(caRemoteIp));
		memcpy(caRemoteIp, dst_ip, strlen(dst_ip));
		memset(sRemotePort, 0x00, sizeof(sRemotePort));
		memcpy(sRemotePort, dst_port, strlen(dst_port));

		dcs_log(0, 0, "commType = [%s], commNO = [%s], caNotiFold = [%s], caFoldName = [%s], iRole = [%s]\ncaLocalIp = [%s], sLocalPort = [%s], caRemoteIp = [%s], sRemotePort = [%s],  caMsgType= [%s]\niTimeOut = [%s], iNotifyFlag = [%s], heart_check_flag = [%s], heart_check_time = [%s], heart_check_data = [%s]\n",
			commType, commNO, caNotiFold,caFoldName, iRole,caLocalIp, sLocalPort,caRemoteIp,sRemotePort,
			caMsgType,iTimeOut,iNotifyFlag, heart_check_flag, heart_check_time);

		if (commType == NULL || commNO == NULL || caNotiFold == NULL || caFoldName == NULL || iRole==NULL 
		|| caLocalIp==NULL || sLocalPort==NULL || caRemoteIp==NULL || sRemotePort==NULL || caMsgType==NULL
		|| iTimeOut==NULL ||heart_check_flag==NULL ||  heart_check_time==NULL )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}

		memset(&(pLinksCtrl->linkArray[iSlotIndex]), 0, sizeof(struct linkinfo));
		
		strcpy(pLinksCtrl->linkArray[iSlotIndex].commType,     commType);
		strcpy(pLinksCtrl->linkArray[iSlotIndex].commNO,       commNO);
		strcpy(pLinksCtrl->linkArray[iSlotIndex].caFoldName,   caFoldName);
		strcpy(pLinksCtrl->linkArray[iSlotIndex].caNotiFold,   caNotiFold);

		//caFoldName
		int myFid = fold_locate_folder(caFoldName);
		if(myFid < 0)  myFid = fold_create_folder(caFoldName);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s\n", "TRANSRCV", ise_strerror(errno) );
		}

		pLinksCtrl->linkArray[iSlotIndex].iFid = get_fold_id_from_share_memory_by_name(pLinksCtrl->linkArray[iSlotIndex].caFoldName,&nFid);
		pLinksCtrl->linkArray[iSlotIndex].iApplFid = get_fold_id_from_share_memory_by_name(caNotiFold,&nFid);


		if (strcmp(iRole, "passive") == 0) 
		{
			pLinksCtrl->linkArray[iSlotIndex].iRole = DCS_ROLE_PASSIVE;
		} 
		else if (strcmp(iRole, "active") == 0) 
		{
			pLinksCtrl->linkArray[iSlotIndex].iRole = DCS_ROLE_ACTIVE;
		} 
		else if (strcmp(iRole, "simplexsvr") == 0) 
		{
			pLinksCtrl->linkArray[iSlotIndex].iRole = DCS_ROLE_SIMPLEXSVR;
		} 
		else if (strcmp(iRole, "simplexcli") == 0) 
		{
			pLinksCtrl->linkArray[iSlotIndex].iRole = DCS_ROLE_SIMPLEXCLI;
		}

		strcpy(pLinksCtrl->linkArray[iSlotIndex].caLocalIp,   caLocalIp);

		if ( u_stricmp(sLocalPort,"null") ==0)
		{
			pLinksCtrl->linkArray[iSlotIndex].sLocalPort =0;
		}
		else
		{
			pLinksCtrl->linkArray[iSlotIndex].sLocalPort = atoi(sLocalPort);
		}
		strcpy(pLinksCtrl->linkArray[iSlotIndex].caRemoteIp,   caRemoteIp);
		if ( u_stricmp(sRemotePort,"null") ==0)
		{
			pLinksCtrl->linkArray[iSlotIndex].sRemotePort =0;
		}
		else
		{	
			pLinksCtrl->linkArray[iSlotIndex].sRemotePort = atoi(sRemotePort);
		}
		strcpy(pLinksCtrl->linkArray[iSlotIndex].caMsgType,   caMsgType);
		pLinksCtrl->linkArray[iSlotIndex].iTimeOut = atoi(iTimeOut);

		if (strcmp(iNotifyFlag, "no") == 0) 
		{
			pLinksCtrl->linkArray[iSlotIndex].iNotifyFlag = 0;
		}
		else if (strcmp(iNotifyFlag, "yes") == 0) 
		{
			pLinksCtrl->linkArray[iSlotIndex].iNotifyFlag = 1;
		}
		pLinksCtrl->linkArray[iSlotIndex].heart_check_flag = heart_check_flag[0];
		pLinksCtrl->linkArray[iSlotIndex].heart_check_time = atoi(heart_check_time);
		pLinksCtrl->linkArray[iSlotIndex].lFlags = DCS_FLAG_NACREADER;

		break;
	}
	dcs_log(0,0,"<dcxcmd> load config end");
	conf_close(iFd);
	if(nFlag ==0)
	{
		dcs_log(0,0,"没有该链路 CommName=[%s],CommNo=[%s]  的配置信息",pFoldName,sFoldNum);
		printf("没有该链路 CommName=[%s] ,CommNo=[%s] 的配置信息\n",pFoldName,sFoldNum);
		return -1;
	}
	//返回在共享内存中的位置
	return i;
}
/*关闭链路
*/
int StopHstLink()
{
	char sFoldName[20];
	char sFoldNum[4];
	char sBuf[200]={0};
	int nRet = -1,i=0;
	int nFlag=-1;
	memset(sFoldName,0,sizeof(sFoldName));
	memset(sFoldNum,0,sizeof(sFoldNum));
	
	scanf("%s %s",sFoldName,sFoldNum);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);
	dcs_log(0,0,"停止链路 CommName=[%s],CommNo=[%s] ",sFoldName,sFoldNum);
	nRet = GetLink(sFoldName,sFoldNum,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 不存在",sFoldName,sFoldNum);
		printf("链路 CommName=[%s],CommNo=[%s] 不存在\n",sFoldName,sFoldNum);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 已是停止状态.",sFoldName,sFoldNum);
		printf("链路 CommName=[%s],CommNo=[%s] 已是停止状态.\n",sFoldName,sFoldNum);
		return 0;
	}
	i=nRet;
	//链路存在,向链路进程发送终止信号
	nRet=kill(pLinksCtrl->linkArray[i].pidComm,SIGUSR2);
	dcs_log(0,0,"kill 进程[%d].",pLinksCtrl->linkArray[i].pidComm);
	if(nRet == 0)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 关闭成功",sFoldName,sFoldNum);
		//复位对端的ip和端口
		if(pLinksCtrl ->linkArray[i].iRole == DCS_ROLE_SIMPLEXSVR || pLinksCtrl ->linkArray[i].iRole ==DCS_ROLE_PASSIVE)
		{
			memset(pLinksCtrl ->linkArray[i].sActualIp, 0,sizeof(pLinksCtrl ->linkArray[i].sActualIp));
			pLinksCtrl ->linkArray[i].nActualPort = 0;
		}
		
	}
	else
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 关闭失败,err:%s",sFoldName,sFoldNum,strerror(errno));
		printf("链路 CommName=[%s],CommNo=[%s] 关闭失败,err:%s\n",sFoldName,sFoldNum,strerror(errno));
	}
	
	return 0;
}
/*获取链路状态
*输入参数:pFoldName链路fold 名，pCommNO通道序号
*输出参数: 如果找到pFoldName和pCommNO的链路，将该链路状态放在pFlag
* 返回值: 当找到需要的链路时返回其在共享内存中的位置，否则返回-1
*/
int GetLink(char * pFoldName,char *pCommNO,int *pFlag)
{
	int i;
	
	for(i=0;i<pLinksCtrl->iLinkCnt;i++)
	{ 
		if(strcmp(pFoldName,pLinksCtrl->linkArray[i].caFoldName)==0 
			&& strcmp(pLinksCtrl->linkArray[i].commNO,pCommNO)==0)
			break;
	}
	
	if(i== pLinksCtrl->iLinkCnt )
	{
		return -1;
	}
	else
	{
		*pFlag= pLinksCtrl->linkArray[i].iStatus;
		return i;
	}
}


static int OpenDcmCmdLog()
{
    char logfile[256];
    memset(logfile, 0, sizeof(logfile));
    if(u_fabricatefile("log/dcxCmd.log",logfile,sizeof(logfile)) < 0)
        return -1;

    return dcs_log_open(logfile, "dcxCmd");
}

/*启动相关链路服务
*/
int  StartHstLink()
{
	char sFoldName[20];
	char sFoldNum[4];
	char sBuf[200]={0};
	int nRet = -1;
	int nFlag=-1,nPid=0;
	memset(sFoldName,0,sizeof(sFoldName));
	memset(sFoldNum,0,sizeof(sFoldNum));
	
	scanf("%s %s",sFoldName,sFoldNum);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);
	dcs_log(0,0,"启动链路 CommName=[%s],CommNo=[%s] ",sFoldName,sFoldNum);
	nRet = GetLink(sFoldName,sFoldNum,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 不存在",sFoldName,sFoldNum);
		printf("链路 CommName=[%s],CommNo=[%s] 不存在\n",sFoldName,sFoldNum);
		return 0;
	}
	else if(nFlag !=DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 已经是启动状态了.",sFoldName,sFoldNum);
		printf("链路 CommName=[%s],CommNo=[%s] 已经是启动状态了.\n",sFoldName,sFoldNum);
		return 0;
	}
	nPid=StartProcess(g_proctab_[0],sFoldName,sFoldNum);
	if(nPid < 0)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 启动失败",sFoldName,sFoldNum);
		printf("链路 CommName=[%s],CommNo=[%s] 启动失败\n",sFoldName,sFoldNum);
	}
	pLinksCtrl->linkArray[nRet].pidComm = nPid;
	dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 启动成功",sFoldName,sFoldNum);
		
	return 0;
}


/*更新相关链路服务
*/
int UpdateHstLink()
{
	char sFoldName[20+1];
	char sFoldNum[4];
	char sBuf[200]={0};
	int nRet = 0,i,nPid;
	int nFlag=-1;
	
	memset(sFoldName, 0, sizeof(sFoldName));
	memset(sFoldNum,0,sizeof(sFoldNum));
	
	scanf("%s %s",sFoldName,sFoldNum);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	dcs_log(0,0,"更新链路 CommName=[%s],CommNo=[%s] ",sFoldName,sFoldNum);
	nRet = GetLink(sFoldName,sFoldNum,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 不存在",sFoldName,sFoldNum);
		printf("链路 CommName=[%s],CommNo=[%s] 不存在\n",sFoldName,sFoldNum);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 已是停止状态.",sFoldName,sFoldNum);
		printf("链路 CommName=[%s],CommNo=[%s] 已是停止状态.\n",sFoldName,sFoldNum);
		return 0;
	}
	i=nRet;
	//链路存在,向链路进程发送终止信号
	nRet=kill(pLinksCtrl->linkArray[i].pidComm,SIGUSR2);
	dcs_log(0,0,"kill 进程[%d].",pLinksCtrl->linkArray[i].pidComm);
	if(nRet == 0)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 关闭成功",sFoldName,sFoldNum);
		//复位对端的ip和端口
		if(pLinksCtrl ->linkArray[i].iRole == DCS_ROLE_SIMPLEXSVR || pLinksCtrl ->linkArray[i].iRole ==DCS_ROLE_PASSIVE)
		{
			memset(pLinksCtrl ->linkArray[i].sActualIp, 0,sizeof(pLinksCtrl ->linkArray[i].sActualIp));
			pLinksCtrl ->linkArray[i].nActualPort = 0;
		}
		
	}
	else
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 关闭失败,err:%s",sFoldName,sFoldNum,strerror(errno));
		printf("链路 CommName=[%s],CommNo=[%s] 关闭失败,err:%s\n",sFoldName,sFoldNum,strerror(errno));
	}

	//重新从配置文件中 读出相应配置信息
	nRet=reload_hst_server_cfg(pLinksCtrl,sFoldName,sFoldNum,0);
	if(nRet <0)
	{
		printf("重新读取配置失败\n");
		return -1;
	}

	sleep(1);// 等待一秒，等待kill信号量先被处理
	/*启动*/
	nPid=StartProcess(g_proctab_[0],sFoldName,sFoldNum);
	if(nPid < 0)
	{
		dcs_log(0,0,"链路 CommName=[%s],CommNo=[%s] 启动失败",sFoldName,sFoldNum);
		printf("链路 CommName=[%s],CommNo=[%s] 启动失败\n",sFoldName,sFoldNum);
	}
	pLinksCtrl->linkArray[i].pidComm = nPid;
	dcs_log(0,0,"更新 CommName=[%s],CommNo=[%s] 成功",sFoldName,sFoldNum);

	return 0;
}



