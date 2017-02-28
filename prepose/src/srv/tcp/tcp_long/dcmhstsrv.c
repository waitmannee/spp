//
//  dcmhstsrv.c
//
//  Created by Freax on 13-3-11.
//  Copyright (c) 2013年 Chinaebi. All rights reserved.
//

/*
 * 链路管理系统server
 */
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "folder.h"
#include  "dccglobl.h"
#include  "dccpgdef.h"
#include  "dccdcs.h"
#include  "dccsock.h"

static struct hstcom *pLinksCtrl = NULL;
static int  StartAllProcesses();
static int StartProcess(const char *strExeFile, const char *arg1, const char *arg2);
static int creat_hst_svr_shm(int likCnt);
static int load_hst_server_cfg(struct hstcom *pLink);
static int OpenHstSrvLog(char *IDENT);
static int load_hst_server_cfg_cnt();
static int HstMonitor();
static void HstSvrExit(int signo);
int main(int argc, char *argv[])
{
	signal(SIGCLD,SIG_IGN);
	signal(SIGTERM,HstSvrExit);
	signal(SIGSEGV,HstSvrExit);
	//prepare the logging stuff
	if(0 > OpenHstSrvLog(argv[0])) 
		exit(1);

	dcs_log(0,0,"Start dcm hst server... \n");
	int linkCnt = load_hst_server_cfg_cnt();

	dcs_log(0, 0, "dcm hst svr config item count = %d!", linkCnt);

	if(creat_hst_svr_shm(linkCnt) < 0)
	{
		dcs_log(0, 0, "dcm hst svr create shm error!");
		exit(0);
	}

	dcs_log(0, 0, "dcm hst svr create shm suceess!");
	dcs_log(0,0,"dcm hst server load config");

	if (load_hst_server_cfg(pLinksCtrl) < 0) 
	{
		dcs_log(0, 0, "dcm hst svr create load hst svrver config error!");
		exit(0);
	}

    	dcs_log(0,0,"dcm hst server load config sucess...");
	if(queue_create("monitor")<0)
	{
		dcs_log(0, 0, "dcm hst svr create monitor queue error!");
		exit(0);
	}
    	dcs_log(0,0,"*************************************************\n"
            "!******** dcm hst svr  load all process .*********!!\n"
            "*************************************************\n");

   	 StartAllProcesses();

    	dcs_log(0,0,"*************************************************\n"
            "!********dcm hst svr  startup completed.*********!!\n"
            "*************************************************\n");
	//长链接链路重连
       HstMonitor();
	HstSvrExit(0);
    return 0;
}

int creat_hst_svr_shm(int likCnt)
{
	char *ptr = NULL;
	int shmid;
	int size = 2*sizeof(int)+likCnt * sizeof(struct linkinfo);

	dcs_log(0, 0, "shm create DCS_HSTCOM_NAME size = %d ", size);
	ptr = shm_create(DCS_HSTCOM_NAME, size,&shmid);

	dcs_log(0, 0, "shm create DCS_HSTCOM_NAME shm_id = %d ", shmid);
	if(ptr == NULL)
	{
		dcs_log(0,0,"cannot create SHM '%s':%s!\n", DCS_HSTCOM_NAME,strerror(errno));
		return -1;
	}

	//get all the pointers and do initialization
	memset(ptr, 0, size);
	pLinksCtrl   = (struct hstcom *)ptr;
	pLinksCtrl->iLinkCnt = likCnt;

	return 0;
}

static int load_hst_server_cfg(struct hstcom *pLink)
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iSlotIndex,iCnt,nFid;
	char cfgfile[256];

	char *caFoldName = NULL,*commType = NULL,*commNO = NULL,*iRole = NULL,*caLocalIp = NULL,*caRemoteIp = NULL,
	*iTimeOut = NULL,*iNotifyFlag  = NULL,*caMsgType = NULL,*caNotiFold = NULL, *heart_check_flag = NULL, 
	*heart_check_time = NULL;

	char sLocalPort[20];
	char sRemotePort[20];

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
		dcs_log(0,0,"<dcmhstsvr> load config item 'comm.max' failed!");
		conf_close(iFd);
		return -1;
	}
	iSlotIndex = 0;

	iRc = conf_get_first_string(iFd, "comm",caBuffer);

	for( ;
	iRc == 0 && iSlotIndex < pLinksCtrl->iLinkCnt;
	iRc = conf_get_next_string(iFd, "comm",caBuffer))
	{
		commType = strtok(caBuffer," \t\r"); /* 通道类型*/
		commNO      = strtok(NULL," \t\r");     /* 通道顺序号*/
		caFoldName        = strtok(NULL," \t\r");      /* 该ink文件夹名字*/
		caNotiFold    = strtok(NULL," \t\r");     /* 对应业务文件夹名字*/
		iRole    = strtok(NULL," \t\r");      /* 该link在连接建立过程中的角色*/
		caLocalIp         = strtok(NULL," \t\r");      /* 本地端口号*/  
		caRemoteIp    = strtok(NULL   ," \t\r"); /* 远地端口号*/
		caMsgType   = strtok(NULL   ," \t\r");   /* 消息类型*/
		iTimeOut     = strtok(NULL   ," \t\r");   /* 超时*/
		iNotifyFlag      = strtok(NULL   ," \t\r"); /* 链路状态通知标志*/
		heart_check_flag = strtok(NULL, " \t\r"); /* 心跳包检测标识*/
		heart_check_time = strtok(NULL, " \t\r"); /* 心跳 包检测时间*/

		// dcs_log(0, 0, "iRc=%d, commType = [%s], commNO = [%s], caNotiFold = [%s], caFoldName = [%s], iRole = [%s],  caLocalIp = [%s], caRemoteIp = [%s], caMsgType= [%s], iTimeOut = [%s], iNotifyFlag = [%s]\n",
		//      iRc,commType, commNO, caNotiFold,caFoldName, iRole,caLocalIp, caRemoteIp,
		//   caMsgType,iTimeOut,iNotifyFlag);

		char dst_ip[256];
		char dst_port[20];
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

		dcs_log(0, 0, "commType = [%s], commNO = [%s], caNotiFold = [%s], caFoldName = [%s], iRole = [%s]\ncaLocalIp = [%s], sLocalPort = [%s], caRemoteIp = [%s], sRemotePort = [%s],  caMsgType= [%s]\niTimeOut = [%s], iNotifyFlag = [%s], heart_check_flag = [%s], heart_check_time = [%s]\n",
			commType, commNO, caNotiFold,caFoldName, iRole,caLocalIp, sLocalPort,caRemoteIp,sRemotePort,
			caMsgType,iTimeOut,iNotifyFlag, heart_check_flag, heart_check_time);

		if (commType == NULL || commNO == NULL || caNotiFold == NULL || caFoldName == NULL || iRole==NULL 
		|| caLocalIp==NULL || sLocalPort==NULL || caRemoteIp==NULL || sRemotePort==NULL || caMsgType==NULL
		|| iTimeOut==NULL ||heart_check_flag==NULL ||  heart_check_time==NULL )
		{
			dcs_log(0, 0, "invalid address:skip this line");
			continue;
		}
					
		strcpy(pLinksCtrl->linkArray[iSlotIndex].commType,     commType);
		strcpy(pLinksCtrl->linkArray[iSlotIndex].commNO,       commNO);
		strcpy(pLinksCtrl->linkArray[iSlotIndex].caFoldName,   caFoldName);
		strcpy(pLinksCtrl->linkArray[iSlotIndex].caNotiFold,   caNotiFold);

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
		pLinksCtrl->linkArray[iSlotIndex].heart_check_time =atoi(heart_check_time);
		
		pLinksCtrl->linkArray[iSlotIndex].lFlags = DCS_FLAG_NACREADER;

		iSlotIndex++;
	}
	dcs_log(0,0,"<dcmhstsvr> load config ");
	conf_close(iFd);

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
		dcs_log(0,0, "going to load process '%s'...\n", strExeFile);
		return pidChld;
	}
	u_sleep_us(10000);/*休息10ms等待父进程先返回*/
	memset(path_exe, 0x00, sizeof(path_exe));
	sprintf(path_exe, "%s/run/bin/%s", getenv("EXECHOME"),strExeFile);
	dcs_log(0,0,"exec '%s':%s, %s!",strExeFile, arg1, arg2);

	//child here
	ret = execlp (path_exe, strExeFile,arg1, arg2,(char *)0);
	if(ret < 0)
	{
		dcs_log(0,0,"cannot exec executable '%s':%s!",strExeFile,strerror(errno));
		return -1;
	}

	return 0;
}


static char* g_proctab_[]=
{
	"dcxhstcp",   //makefile: create by from dcmhsttcp.c
	NULL
};

static int  StartAllProcesses()
{
	int i;
	for(i=0; i < pLinksCtrl->iLinkCnt &&strlen(pLinksCtrl->linkArray[i].caFoldName) > 0; i++)
	{
		struct linkinfo link = pLinksCtrl->linkArray[i];
		dcs_log(0, 0, "caNotiFold =>> %s, iAppFid = %d, caFoldName => %s, caFoldFid = %d", link.caNotiFold,link.iApplFid, link.caFoldName, link.iFid);
		pLinksCtrl->linkArray[i].pidComm = StartProcess(g_proctab_[0], pLinksCtrl->linkArray[i].caFoldName, pLinksCtrl->linkArray[i].commNO);
		u_sleep_us(100000);/*休息100ms*/
	}//for
	return 0;
}


static int OpenHstSrvLog(char *IDENT)
{
	char logfile[256];
       memset(logfile, 0, sizeof(logfile));
	//assuming the log file is "$FEP_RUNPATH/log/appsvr1.log"
	if(u_fabricatefile("log/hstsvr.log",logfile,sizeof(logfile)) < 0)
		return -1;

	return dcs_log_open(logfile, IDENT);
}


static int load_hst_server_cfg_cnt()
{
	char   caBuffer[512];
	int    iFd=-1,iRc,iCnt,iSlotIndex;
	char cfgfile[256];

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
		dcs_log(0,0,"<dcmhstsvr> load config item 'comm.max' failed!");
		conf_close(iFd);
		return -1;
	}
	conf_close(iFd);
	return iCnt;
}

int HstMonitor()
{
	int qid,nRecvLen,i;
	char szBuf[100+1];
	char *pCommName = NULL,*pCommNo = NULL,*pStatus = NULL;
	qid =queue_connect("monitor");
	if(qid <0)
		qid =queue_create("monitor");
	if ( qid <0)
	{
	  	dcs_log(0,0,"Can not connect message queue  monitor!");
	  	return ;
	}
	while(1)
	{
		dcs_log(0,0,"do monitoring .....");
		memset(szBuf,0,sizeof(szBuf));
		nRecvLen=queue_recv(qid, szBuf,sizeof(szBuf), 0);
		if ( nRecvLen <0)
	        {
			 	dcs_log(0,0,"Recive Message fail!");
			 	break;  
	        }
		 if(memcmp(szBuf,"SYSICOMML",9) !=0)
		 {
		 	dcs_log(0,0,"Recive Message not compare [%s]!",szBuf);
			 continue;  
		 }
		 strtok(szBuf," \t\r"); 
       	 pCommName = strtok(NULL," \t\r");
		 pCommNo = strtok(NULL," \t\r");
		 pStatus = strtok(NULL," \t\r");
		 if(pCommName == NULL||pCommNo == NULL||pStatus == NULL)
		 {
		 	dcs_log(0,0,"Recive Message not compare [%s],CommName=[%s],CommNo=[%s],Status=[%s]!",szBuf,pCommName,pCommNo,pStatus);
			 continue;  
		 }
		 
		if(pLinksCtrl == NULL)
		{ 
			pLinksCtrl = (struct hstcom *)shm_connect(DCS_HSTCOM_NAME, NULL);
			if(pLinksCtrl == NULL)
			{
			    dcs_log(0, 0, "cannot connect SHM '%s':%s!\n", DCS_HSTCOM_NAME, strerror(errno));
			    continue;  
			}
		}
		
		for(i=0;i<pLinksCtrl->iLinkCnt;i++)
		{ 
			if(strcmp(pCommName,pLinksCtrl->linkArray[i].caFoldName)==0 
				&& strcmp(pLinksCtrl->linkArray[i].commNO,pCommNo)==0)
				break;
		}
		if(i == pLinksCtrl->iLinkCnt)
		{
			 dcs_log(0,0,"link CommName=[%s],CommNo=[%s] not exist!",pCommName,pCommNo);
			 continue;  
		}
		
		 dcs_log(0,0,"RESTART CommName=[%s],CommNo=[%s]!",pCommName,pCommNo);
		 pLinksCtrl->linkArray[i].pidComm =StartProcess(g_proctab_[0], pCommName,pCommNo);
	}
	return 0;
}

static void HstSvrExit(int signo)
{
    dcs_log(0,0,"catch a signal %d",signo);
        
    if(signo !=0 && signo != SIGTERM && signo != SIGSEGV)
        return;

    dcs_log(0,0,"dcm hst server terminated.");
    
    shm_detach((char *)pLinksCtrl);
    exit(signo);
}



