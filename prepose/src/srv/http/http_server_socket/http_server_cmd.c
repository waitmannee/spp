 /*  
 *程序名称：http_server_cmd.c
 *功能描述：
 *用于http服务器端各个子程序的管理
*/
#include "http_server.h"

static struct http_s_Ast *pLinkHttpsCtrl = NULL;
static void Http_Server_Cmd_Exit(int signo);
static int Add_Http_Server_Link();
static void List_Http_Server_Info();
static int Reload_Http_Server_Cfg(struct http_s_Ast *pLink,char *pFoldName,int mode);
static int Stop_Http_Server_Link();
static int Get_Http_Server_Link(char *pFoldName,int *pFlag);
static int Open_Http_Server_Cmd_Log();
static int  Start_Http_Server_Link();
static int Update_Http_Server_Link();
//==========================================

int main(int argc, char *argv[])
{
	
	catch_all_signals(Http_Server_Cmd_Exit);

	if(Open_Http_Server_Cmd_Log()< 0)
		return -1;
	if(pLinkHttpsCtrl == NULL)
	{ 
		pLinkHttpsCtrl = (struct http_s_Ast*)shm_connect(HTTP_SERVER_SHM_NAME, NULL);
		if(pLinkHttpsCtrl == NULL)
		{
		    dcs_log(0, 0, "cannot connect SHM '%s':%s!\n", HTTP_SERVER_SHM_NAME, strerror(errno));
		    return -1;
		}
	}

	cmd_init("httpServerCmd>>",Http_Server_Cmd_Exit);
	cmd_add("list", (cmdfunc_t *)List_Http_Server_Info, "list server info");
	cmd_add("start", (cmdfunc_t *)Start_Http_Server_Link, "start a server");
	cmd_add("add", (cmdfunc_t *)Add_Http_Server_Link, "Add a server ");
	cmd_add("stop", (cmdfunc_t *)Stop_Http_Server_Link, "stop a server");
	cmd_add("update", (cmdfunc_t *)Update_Http_Server_Link, "update  a server");
	
	cmd_shell(argc,argv);

	Http_Server_Cmd_Exit(0);
	return 0;
}


static void Http_Server_Cmd_Exit(int signo)
{
	shm_detach((char *)pLinkHttpsCtrl);
	exit(signo);
}

static void List_Http_Server_Info()
{
	int i = 0;
	char sBuf[200]={0};
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	printf("maxNum=%d\n",pLinkHttpsCtrl->i_http_num);
	printf("id     FdName        AppFdName     Status   ListenPort   \n");
	printf("=============================================================\n");
	for(i = 0; i<pLinkHttpsCtrl->i_http_num; i++)
	{
		if(strlen(pLinkHttpsCtrl->https[i].name) > 0)
		{
			printf("[%03d] ", i+1);
			printf("%-10s", pLinkHttpsCtrl->https[i].name);
			printf("%-15s", pLinkHttpsCtrl->https[i].fold_name);
			
			//link Status
			if((pLinkHttpsCtrl->https[i].iStatus == DCS_STAT_LISTENING))
				printf("%-8s ", "listen");
			else if(pLinkHttpsCtrl->https[i].iStatus ==DCS_STAT_STOPPED)
				printf("%-8s ", "stoped");
			else
				printf("%s ", " ");
			printf("%5d  ", pLinkHttpsCtrl->https[i].port);
			printf("\n");
		}
	}
	return ;
}
/*添加一个新的http 服务监听进程
*/
static int Add_Http_Server_Link()
{
	char sFoldName[20+1];
	char sBuf[200]={0};
	int nRet = 0,nFlag = 0;
	
	memset(sFoldName, 0, sizeof(sFoldName));
	scanf("%s", sFoldName);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);
	
	dcs_log(0,0,"添加http服务器端,folderName=[%s]",sFoldName);
	//先查找是否是已存在的链路
	nRet = Get_Http_Server_Link(sFoldName,&nFlag);
	if(nRet >=0)
	{
		dcs_log(0,0,"http服务器端 服务已经存在,folderName=[%s]",sFoldName);
		printf("http服务器端 服务已经存在,folderName=[%s]\n",sFoldName);
		return -1;
	}
	//不存在的话，从配置文件中 读出配置信息
	nRet=Reload_Http_Server_Cfg(pLinkHttpsCtrl,sFoldName,1);
	if(nRet <0)
	{
		return -1;
	}

	nRet=Start_Process("httpServer",sFoldName);
	if(nRet < 0)
	{
		dcs_log(0,0,"http服务器端 服务folderName=[%s]添加失败",sFoldName);
		printf("http服务器端 服务folderName=[%s]添加失败\n",sFoldName);
		return -1;
	}
	dcs_log(0,0,"http服务器端 服务folderName=[%s]添加成功",sFoldName);
	return 0;

	
}

/*重新导入配置文件
*mode =1表示是在共享内存的后面添加fold名为pFoldName的新纪录,
*mode =0 表示更新共享内存中fold名为pFoldName的已有记录的内容
*/
static int Reload_Http_Server_Cfg(struct http_s_Ast *pLink,char *pFoldName,int mode)
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iSlotIndex =0 ,i, iCnt =0 ;
	char cfgfile[256];

	char *name = NULL,*fold_name = NULL,*msgtype = NULL,*port = NULL;
	int nFlag=0;

	if(u_fabricatefile("config/http_server.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'http_server.conf'\n");
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

	
	//mode =1 时需要在共享内存的最后添加内容，所以要找到最后一条空记录
	if(mode == 1)
	{
		for(iSlotIndex=0;iSlotIndex<pLinkHttpsCtrl->i_http_num;iSlotIndex++)
		{
			if(strlen(pLinkHttpsCtrl->https[iSlotIndex].name) == 0)
			{
				break;
			}
		}
		
		if(iSlotIndex ==pLinkHttpsCtrl->i_http_num )
		{
			dcs_log(0,0,"共享内存没有空余空间存储新的配置信息,最多%d条",iSlotIndex);
			printf("共享内存没有空余空间存储新的配置信息,最多%d条\n",iSlotIndex);
			return -1;
		}
	}	

	iRc = conf_get_first_string(iFd, "conn",caBuffer);
	for(i=0 ;iRc == 0 && i < pLinkHttpsCtrl->i_http_num;iRc = conf_get_next_string(iFd, "conn",caBuffer))
	{
		name = strtok(caBuffer," \t\r");/*链路FOLDER名字*/
		fold_name      = strtok(NULL," \t\r");/*链路对应业务处理进程的名字*/
		msgtype        = strtok(NULL," \t\r");/*报文类型*/
		port    = strtok(NULL," \t\r");/*监听端口*/
			
		if(strcmp(name, pFoldName) != 0)
		{
			i++;
			continue;
		}
		//如果是更新,需要找到共享内存中的位置
		if(mode == 0)
		{
			iSlotIndex =Get_Http_Server_Link(pFoldName,&iRc);
		}

		nFlag =1;//找到配置

		dcs_log(0, 0, " name = [%s], fold_name = [%s], msgtype = [%s], serveraddr = [%s]",name,fold_name,msgtype,port);
	
		if (name == NULL || fold_name == NULL ||msgtype == NULL ||  port == NULL )		
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}

		strcpy(pLinkHttpsCtrl->https[iSlotIndex].name,name);
		strcpy(pLinkHttpsCtrl->https[iSlotIndex].fold_name,fold_name);
		strcpy(pLinkHttpsCtrl->https[iSlotIndex].msgtype,msgtype);
		pLinkHttpsCtrl->https[iSlotIndex].port = atoi(port);
		
		dcs_log(0, 0, "linkArray[%d].name = %s", iSlotIndex, pLinkHttpsCtrl->https[iSlotIndex].name);
		//caFoldName
		int myFid = fold_locate_folder(name);
		if(myFid < 0) 
			myFid = fold_create_folder(name);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s\n",name, ise_strerror(errno) );
		}
		
		pLinkHttpsCtrl->https[iSlotIndex].iFid =myFid;
		
		myFid = fold_locate_folder(fold_name);
		if(myFid < 0) 
			myFid = fold_create_folder(fold_name);

		if(myFid < 0)
		{
			dcs_log(0,0,"cannot create folder '%s':%s\n",fold_name, ise_strerror(errno) );
		}
		pLinkHttpsCtrl->https[iSlotIndex].iApplFid =myFid;
		
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

/*关闭一个运行的http 服务监听进程
*/
int Stop_Http_Server_Link()
{
	char sFoldName[20+1];
	char sBuf[200]={0};
	int nRet = 0,i,nPid = 0;
	int nFlag=-1;
	
	memset(sFoldName, 0, sizeof(sFoldName));
	
	scanf("%s",sFoldName);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	dcs_log(0,0,"停止http服务器端[%s] 服务",sFoldName);
	nRet = Get_Http_Server_Link(sFoldName,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"http服务器端[%s] 服务不存在",sFoldName);
		printf("http服务器端[%s] 服务不存在\n",sFoldName);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"http服务器端[%s] 服务已是停止状态",sFoldName);
		printf("http服务器端[%s] 服务已是停止状态\n",sFoldName);
		return 0;
	}
	//链路存在,向链路进程发送终止信号
	nRet=kill(pLinkHttpsCtrl->https[nRet].pidComm,SIGUSR2);
	if(nRet == 0)
	{
		dcs_log(0,0,"http服务器端[%s] 服务关闭成功",sFoldName);
	}
	else
	{
		dcs_log(0,0,"http服务器端[%s] 服务关闭失败",sFoldName);
		printf("http服务器端[%s] 服务关闭失败\n",sFoldName);
	}
	
	return 0;
}

/*获取链路状态
*输入参数:http服务器端的FoldName
*输出参数: 如果找到FoldName的链路，将该链路状态放在pFlag
* 返回值: 当找到需要的链路时返回其在共享内存中的位置，否则返回-1
*/
int Get_Http_Server_Link(char *pFoldName,int *pFlag)
{
	int i;
	
	for(i=0;i<pLinkHttpsCtrl->i_http_num;i++)
	{ 
		if(strcmp(pLinkHttpsCtrl->https[i].name, pFoldName) == 0)
		{
			*pFlag = pLinkHttpsCtrl->https[i].iStatus;
			return i;
		}			
	}
	
	return -1;
}


static int Open_Http_Server_Cmd_Log()
{
    char logfile[256];
    memset(logfile, 0, sizeof(logfile));
    if(u_fabricatefile("log/httpSvrCmd.log",logfile,sizeof(logfile)) < 0)
        return -1;

    return dcs_log_open(logfile, "httpServerCmd");
}

/*启动一个停止的http 服务监听进程
*/
int  Start_Http_Server_Link()
{
	char sFoldName[20+1];
	char sBuf[200]={0};
	int nRet = -1;
	int nFlag=-1;
	memset(sFoldName,0,sizeof(sFoldName));
	
	scanf("%s",sFoldName);
	//清空stdin
	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf,sizeof(sBuf),stdin);

	dcs_log(0,0,"启动http服务器端[%s] 服务",sFoldName);
	nRet = Get_Http_Server_Link(sFoldName,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"http服务器端[%s] 服务不存在",sFoldName);
		printf("http服务器端[%s] 服务不存在\n",sFoldName);
		return 0;
	}
	else if(nFlag !=DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"http服务器端[%s] 服务已经在运行",sFoldName);
		printf("http服务器端[%s] 服务已经在运行\n",sFoldName);
		return 0;
	}
	nRet=Start_Process("httpServer",sFoldName);
	if(nRet < 0)
	{
		dcs_log(0,0,"http服务器端[%s] 服务启动失败",sFoldName);
		printf("http服务器端[%s] 服务启动失败\n",sFoldName);
	}
	dcs_log(0,0,"http服务器端[%s] 服务启动成功",sFoldName);
	return 0;
}

/*更新相关http服务
*/
int Update_Http_Server_Link()
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

	dcs_log(0,0,"更新http服务器端[%s] 服务",sFoldName);
	nRet = Get_Http_Server_Link(sFoldName,&nFlag);
	if(nRet == -1)
	{
		dcs_log(0,0,"http服务器端[%s] 服务不存在",sFoldName);
		printf("http服务器端[%s] 服务不存在\n",sFoldName);
		return 0;
	}
	else if(nFlag == DCS_STAT_STOPPED)
	{
		dcs_log(0,0,"http服务器端[%s] 服务已是停止状态",sFoldName);
		printf("http服务器端[%s] 服务已是停止状态\n",sFoldName);
		return 0;
	}
	//链路存在,向链路进程发送终止信号
	nRet=kill(pLinkHttpsCtrl->https[nRet].pidComm,SIGUSR2);
	if(nRet == 0)
	{
		dcs_log(0,0,"http服务器端[%s] 服务关闭成功",sFoldName);
	}
	else
	{
		dcs_log(0,0,"http服务器端[%s] 服务关闭失败",sFoldName);
		printf("http服务器端[%s] 服务关闭失败\n",sFoldName);
		return -1;
	}
	//重新从配置文件中 读出相应配置信息
	nRet=Reload_Http_Server_Cfg(pLinkHttpsCtrl,sFoldName,0);
	if(nRet <0)
	{
		dcs_log(0,0,"更新http服务器端[%s] 服务失败",sFoldName);
		printf("更新http服务器端[%s] 服务失败\n",sFoldName);
		return -1;
	}
	sleep(1);// 等待一秒，等待kill信号量先被处理
	//启动
	nRet=Start_Process("httpServer",sFoldName);
	if(nRet < 0)
	{
		dcs_log(0,0,"http服务器端[%s] 服务启动失败",sFoldName);
		printf("http服务器端[%s] 服务启动失败\n",sFoldName);
	}
	dcs_log(0,0,"更新http服务器端[%s] 服务成功",sFoldName);
	return 0;
}



