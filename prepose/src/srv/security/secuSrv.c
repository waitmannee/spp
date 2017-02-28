#define _MainProg

// 程序名称：secuSrv.c
// 功能描述：
//          安全管理系统服务通讯程序
// 函数清单：
//           main()


#include  "dccglobl.h"
#include  "dccpgdef.h"
#include  "dccdcs.h"
#include  "dccsock.h"
#include  "secuSrv.h"
//#include  "queue.h"

#define DCS_SEC_LOG  "secusrv2.log"
#define DCS_SEC_NAME "secsrv2.conf"

//////////Freax Add TO Less Warning //////
extern int  AddLink(char *caNum ,struct secLinkInfo *pLink );

//////////////////////////////////////////


static   char            *g_pcIseHome = NULL;
static   int              g_SecShmId = -1;

// Functions declaration
void SecSrvExit(int signo);
int SecSrvOpenLog(char *IDENT);
void MakeConnection(struct secLinkInfo* pLink);
static int LoadCommLnk();

static  int g_LinkCnt,gs_myfid;
struct SECSRVCFG * g_pSecCfg;
int InitShmCfg();
void DoServices();
void TransProc ();
struct secLinkInfo *GetFailTcpLink( );
struct secLinkInfo *GetFreeTcpLink( );
int DeleteLink(struct secLinkInfo *pLink);
int LoadCommCnt();
int StopLink( char *num);
int ModifyLink(struct secLinkInfo *pLink );

/*
 函数名称：main
 参数说明：
 argv[0] 可执行程序名称
 argv[1] 和本通信程序相连的文件夹名
 返 回 值：
 无
 功能描述：
 1.接收来自终端管理系统客户端的报文,并转交给相应的应用进程
 2.接收本地应用的报文,并将其发往网络
 */

int main(int argc, char *argv[])
{
	int i;
	//struct monilinkinfo *arg; //Modify:by Freax comment
  	catch_all_signals(SecSrvExit);
    pthread_t sthread;
    
  	//prepare the log stuff
  	if (0 > SecSrvOpenLog(argv[0]))
        exit(0);
    
    if (fold_initsys() < 0)
    {
        dcs_log(0,0," cannot attach to FOLDER !");    
        exit(0);
    }
    
    gs_myfid = fold_create_folder("SECSRV2");
    
    if(gs_myfid < 0)  gs_myfid = fold_locate_folder("SECSRV2");
    
    dcs_log(0, 0, "gs_myfid = %d", gs_myfid);
    
    if(gs_myfid < 0)
    {
        dcs_log(0,0,"cannot create folder 'SECSRV2':%s\n",  strerror(errno));
        exit(0);
    }
    
    if(fold_get_maxmsg(gs_myfid) <2) fold_set_maxmsg(gs_myfid, 50) ;
    
  	if ( (g_LinkCnt = LoadCommCnt())<=0)
  	{
        dcs_log(0,0, "<SecSrv LoadCommLnk> Failure to load secsrvlnk.ini !" );
        exit(0);
  	}
  	
  	g_pSecSrvShmPtr = shm_connect("SECSRV2",&g_SecShmId);
  	
  	if ( g_pSecSrvShmPtr == NULL)
  	{
        dcs_log(0,0, "heqq 获取共享内存首地址错误,创建连接...");
        dcs_log(0,0, "heqq shm_connect SECSRV2 error ,g_SecShmId=%d", g_SecShmId);
        g_pSecSrvShmPtr = shm_create("SECSRV2", g_LinkCnt *sizeof(struct secLinkInfo) +sizeof(struct SECSRVCFG)+3,&g_SecShmId);
  		
        if ( g_pSecSrvShmPtr == NULL)
        {
            dcs_log(0,0, " heqq Can not create Shm name=SECSRV2,exit !" );
            exit(0);
        }else
            dcs_log(0,0, "heqq shm_create SECSRV2 success ,g_SecShmId=%d", g_SecShmId);
  	}else
  	{
  		dcs_log(0,0, "heqq shm_connect SECSRV2 success ,g_SecShmId=%d", g_SecShmId);
  	}
  	
  	//memset(g_pSecSrvShmPtr,0,g_LinkCnt * sizeof(struct secLinkInfo) + sizeof(struct SECSRVCFG)+3);//Freax Modify    
    
  	g_pSecCfg = (struct SECSRVCFG*)g_pSecSrvShmPtr;
    memset(g_pSecCfg,0,g_LinkCnt * sizeof(struct secLinkInfo) + sizeof(struct SECSRVCFG)+3); //Freax Add
  	g_pSecCfg->maxLink = g_LinkCnt;
  	g_pSecCfg->pNextFreeSlot=0;
    
  	dcs_debug(0,0,"sem id =%d",g_pSecCfg->secSemId);
  	
  	//该函数创建一个命名的信号量
  	g_pSecCfg->secSemId = sem_connect("SECSEM2", 2);
  	
  	if ( g_pSecCfg->secSemId <0)
  	{
  		dcs_log(0,0, "heqq sem_connect SECSEM2 error !");
  		dcs_log(0,0, "heqq 获取SECSEM2信号量错误,重新重建...");
  		g_pSecCfg->secSemId = sem_create("SECSEM2", 2);
  		if(g_pSecCfg->secSemId <0)
  		{
  			dcs_log(0,0, "heqq Can not create Sem name=SECSEM2, exit !" );
  			goto lblExit;
  		}else
  			dcs_log(0,0, "heqq sem_create SECSEM2 success!" );
  	}else
  	{
  		dcs_log(0,0, "heqq sem_connect SECSEM2 success!" );
	}
    
  	dcs_debug(0,0,"sem id =%d",g_pSecCfg->secSemId);
  	dcs_debug(0,0,"begining Init SHM !");
    
  	if ( 0 >= InitShmCfg() )
  	{
  		dcs_log(0,0, "Can not Init SHM config  !" );
  		goto lblExit;
  	}
    dcs_debug(0,0,"Init SHM succ!");
    
    dcs_log(0, 0, "g_pSecCfg->maxLink = %d", g_pSecCfg->maxLink);
  	
  	for ( i =0; i< g_pSecCfg->maxLink ;i++)
  	{
        if ( g_pSecCfg->secLink[i].lFlags == DCS_FLAG_USED &&  strlen(g_pSecCfg->secLink[i].caRemoteIp)>0)
        {
            if ( pthread_create(&sthread, NULL,( void * (*)(void *))MakeConnection,&g_pSecCfg->secLink[i]) !=0)
            {
                dcs_log(0,0, "Can not create thread ");
                goto lblExit;
            }
            pthread_detach(sthread);
        }
	}
//Freax TO DO:    
	for ( i =0 ;i < g_pSecCfg->maxLink; i++)
	{
        if ( pthread_create(&sthread, NULL,( void * (*)(void *))TransProc,NULL) !=0)
        {
            dcs_log(0,0, "Can not create thread ");
            goto lblExit;
        }
        pthread_detach(sthread);
	}
    
	dcs_debug(0,0,"create enctry process group succ!");
	
  	g_pSecCfg->srvStatus=1;
    
    DoServices();
    
lblExit:
    //	PidUnregister();
  	SecSrvExit(0);
    
    return 0;
}//end main()

/*
 函数名称：MoniTcpipExit
 参数说明：
 signo：捕获的信号量
 返 回 值：
 无
 功能描述：
 当捕获的信号为SIGTERM时，该函数释放资源后终止当前进程；否则不做任何处理
 */
void SecSrvExit(int signo)
{
	int i;
  	dcs_debug(0,0,"catch a signal %d !", signo);
    
    
  	//parent process
  	switch (signo)
  	{
    	case SIGUSR2: //monimnt want me to stop
     		break;
            
    	case 0:
    	case SIGTERM:
      		break;
            
    	case SIGCHLD:
            return;
            
    	default:
      		
            break;
            
    }
    
  	g_pSecCfg->srvStatus = 0;
  	
  	for ( i=0; i< g_pSecCfg->maxLink;i++)
  	{
        if( !(g_pSecCfg->secLink[i].lFlags &DCS_FLAG_USED))
            continue;
        if ( g_pSecCfg->secLink[i].iSocketId >=0)
            close(g_pSecCfg->secLink[i].iSocketId);
  	}
    sem_delete(g_pSecCfg->secSemId);
    shm_delete(g_SecShmId);
  	exit(signo);
}

/*
 函数名称：SecSrvOpenLog
 参数说明：
 无
 返 回 值：
 0：成功
 -1：失败
 功能描述：
 1.获得系统主目录,将之放在全局变量g_pcIseHome
 2.读取系统配置文件system.conf
 3.设置日志文件和日志方式
 */

int SecSrvOpenLog(char *IDENT)
{
  	char   caLogName[256];
    char logfile[400];
    
    memset(caLogName,0,sizeof(caLogName));
    strcpy(caLogName,"log/secusrv2.log");
    if(u_fabricatefile(caLogName,logfile,sizeof(logfile)) < 0)
    {
        return -1;
    }
    //dcs_log_open(logfile, "Encrypt");
    //	return 0;
        return dcs_log_open(logfile, IDENT);

 
}


/*
 函数名称：MakeConnection
 参数说明：
 无
 返 回 值：
 >0：成功，返回连接上的socket描述符
 -1：失败
 功能描述：
 根据共享内存中的配置信息判断：
 若通讯进程为服务器，则侦听本地端口号，一旦对方连接上，则返回该链接的socket描述符；
 若通讯进程为客户端，则主动连接对方服务器，连接成功后，返回该链接的socket描述符。
 */

void MakeConnection(struct secLinkInfo* pLink )
{  	  	
  	//char buffer[100]; //Modify:by Freax
  	int i;
  	int   iConnSock;    
	
	dcs_debug(0,0,"thread begin building tcp link ,sem_id=%d",g_pSecCfg->secSemId);
    sem_lock( g_pSecCfg->secSemId, 1, 1);
    dcs_debug(0,0,"lock sem succ!");
    if ( pLink->iStatus == DCS_STAT_CALLING)
    {
        sem_unlock( g_pSecCfg->secSemId, 1, 1);
        dcs_debug(0,0,"pLink status=%d, calling",pLink->iStatus);
        return;
    }
    if ( pLink->iSocketId >=0)
    {
        close(pLink->iSocketId);
        pLink->iSocketId = -1;
    }
    pLink->iStatus = DCS_STAT_CALLING;
    sem_unlock( g_pSecCfg->secSemId, 1, 1);
    i=0;
    for ( ; ; )
    {
        dcs_log(0, 0, "MakeConnection...........");
        dcs_log(0, 0, "conect server:%s:%d localPort = %d",pLink->caRemoteIp,pLink->iRemotePort,pLink->iLocalPort);
        iConnSock = tcp_connet_server(pLink->caRemoteIp,pLink->iRemotePort,pLink->iLocalPort);
        if (iConnSock < 0)
        {
            if (!(i % 10))
                dcs_debug(0,0,"tcp_connet_server() failed:%s! ip=%s, port=%d",strerror(errno),pLink->caRemoteIp,pLink->iRemotePort);
            
            sleep(5);
            if ( i >=60000 )
                i=0;
            i++;
            
            continue;
        }
        else
            break;
        
    }
    pLink->iSocketId =  iConnSock;
    pLink->iStatus = DCS_STAT_CONNECTED;
    dcs_log(0,0,"TCP connect is ESTABLISHED . ip=%s,port=%d ，sock id =%d",pLink->caRemoteIp,pLink->iRemotePort,iConnSock);
}


int LoadCommCnt()
{
  	char   caFileName[256];
  	int    iFd=-1,iCommCnt;
  	  	
  	//设定commlnk定义文件的绝对路径，并打开
  	memset(caFileName, 0, sizeof(caFileName) );
    if(u_fabricatefile("config/secsrv2.conf",caFileName,sizeof(caFileName)) < 0)
    {
        dcs_log(0,0,"cannot get full path name of 'isdcmlnk.conf'\n");
        return -1;
    }
  	if ((iFd = conf_open(caFileName)) < 0)
  	{
    	dcs_log( 0,0, "<HstSrv> open secsrv2.conf fail ! ");
		return -1;
  	}
    
  	//读取配置项'comm.max'
    if ( 0 > conf_get_first_number(iFd, "comm.max",&iCommCnt) || iCommCnt < 0 )
    {
        dcs_log(0,0, "<Sec> load configuration item 'comm.max' failed!");
        conf_close(iFd);
        return (-1);
    }
    
  	conf_close(iFd);
    
  	return(iCommCnt);
}


int InitShmCfg()
{	
    char   caFileName[256],caBuffer[512];
  	int    iFd=-1,iRc,iCommCnt,iSlotIndex;
//  	struct secLinkInfo * pLink; //Modify:by Freax
  	char *pEncType,*pAddr,*pPort,*pPort1,*pTime,*pNotiFlag,*pNotiFold,*pNum;
  	
  	//设定commlnk定义文件的绝对路径，并打开
  	memset(caFileName, 0, sizeof(caFileName) );
//  	sprintf(caFileName, "%s/config/%s",getenv("ICS_HOME"),DCS_SEC_NAME);
    
    if(u_fabricatefile("config/secsrv2.conf",caFileName,sizeof(caFileName)) < 0)
    {
        dcs_log(0,0,"cannot get full path name of 'isdcmlnk.conf'\n");
        return -1;
    }
    
    dcs_log(0, 0, "open file = %s", caFileName);
    
  	if ((iFd = conf_open(caFileName)) < 0)
  	{
        dcs_log( 0,0, "<HstSrv> open secsrv2.conf fail ! ");
        return -1;
  	}    
	
     if ( 0 > conf_get_first_number(iFd, "comm.max",&iCommCnt) || iCommCnt < 0 )
    {
        dcs_log(0,0, "<Sec> load configuration item 'comm.max' failed!");
        conf_close(iFd);
        return (-1);
    }
	  
    iSlotIndex=0;
    for( iRc = conf_get_first_string(iFd, "comm",caBuffer);
        iRc == 0 && iSlotIndex < g_pSecCfg->maxLink;
        iRc = conf_get_next_string(iFd, "comm",caBuffer))
    {   
	    dcs_log(0, 0, "caBuffer = [%s]", caBuffer);
        pNum = strtok(caBuffer," \t\r");  
        //pNum        = strtok(NULL," \t\r");   //Freax Modify
        pEncType    = strtok(NULL," \t\r");  //通道类型
        pPort1      = strtok(NULL," \t\r");  //本地端口
        pAddr       = strtok(NULL   ," :\t\r");  //地址串
        pPort       = strtok(NULL   ," \t\r");  //端口号
        pTime        = strtok(NULL   ," \t\r\n");//通信超时检测时间
        pNotiFlag     = strtok(NULL," \t\r"); //通讯链路状态通知标志
        pNotiFold    = strtok(NULL," \t\r"); //通讯链路状态通知目标文件夹

                
        if (pEncType == NULL || pAddr==NULL || pTime==NULL || pNotiFlag==NULL || pNotiFold==NULL ||
            pPort==NULL  )
        {
            //invalid address:skip this line
			dcs_log(0, 0, "invalid address:skip this line !");
			dcs_log(0, 0, "pNum=[%s], pEncType=[%s], pPort1=[%s], pAddr=[%s], pPort=[%s], pTime=[%s], pNotiFlag=[%s], pNotiFold=[%s]", pNum, pEncType, pPort1, pAddr, pPort, pTime, pNotiFlag, pNotiFold);
            continue;
        }
		dcs_log(0, 0, "pNum=[%s], pEncType=[%s], pPort1=[%s], pAddr=[%s], pPort=[%s], pTime=[%s], pNotiFlag=[%s], pNotiFold=[%s]", pNum, pEncType, pPort1, pAddr, pPort, pTime, pNotiFlag, pNotiFold);
		
        //                dcs_debug(0,0,"给变量赋值");
        g_pSecCfg->secLink[iSlotIndex].lFlags |= DCS_FLAG_USED;
        //                dcs_debug(0,0,"赋值成功1");
        g_pSecCfg->secLink[iSlotIndex].iStatus = DCS_STAT_STOPPED;
        //                dcs_debug(0,0,"赋值成功2");
        strcpy(g_pSecCfg->secLink[iSlotIndex].caRemoteIp,pAddr);
        //                dcs_debug(0,0,"赋值成功3");
        memcpy(g_pSecCfg->secLink[iSlotIndex].cEncType,pEncType,3);
        //                dcs_debug(0,0,"赋值成功4");
        if ( u_stricmp(pPort1,"null") ==0)
            g_pSecCfg->secLink[iSlotIndex].iLocalPort =0;
        else
            g_pSecCfg->secLink[iSlotIndex].iLocalPort = atoi(pPort1);
        //                	 dcs_debug(0,0,"赋值成功5");
        g_pSecCfg->secLink[iSlotIndex].iRemotePort = atoi(pPort);
        //                dcs_debug(0,0,"赋值成功6");
        g_pSecCfg->secLink[iSlotIndex].iSocketId = -1;
        //                dcs_debug(0,0,"赋值成功7");
        g_pSecCfg->secLink[iSlotIndex].iTimeOut = atoi(pTime);
        //                dcs_debug(0,0,"赋值成功8");
        g_pSecCfg->secLink[iSlotIndex].iNum = atoi(pNum);
        //                dcs_debug(0,0,"赋值成功9");
        iSlotIndex++;
    }    
    
    conf_close(iFd); //Freax Add
    return 1;
}

void DoServices()
{
	struct secLinkInfo * pLink;
	pthread_t sthread;
	int fromfid,nread;
	char caBuffer[200];
    	int fid;
	fid=fold_open(gs_myfid);
	while (1)
	{
        dcs_log(0, 0, "DoServices........");
	 	memset(caBuffer, 0, sizeof(caBuffer));
        nread = fold_read( gs_myfid,fid, &fromfid, caBuffer, sizeof(caBuffer), 1);
        if ( nread <0)
        {
            dcs_log(0,0,"Monitor Security service exit!");
            break;
        }
        dcs_debug(caBuffer,nread,"recv msg");
        if ( memcmp(caBuffer,"DELE",4)==0)
        {
            pLink = GetFailTcpLink(atoi(caBuffer+4) );
            if ( pLink ==  NULL)
            {
                dcs_log(0,0,"该链路不存在，删除失败!");
                continue;
            }
            DeleteLink(pLink);
        }
        else if ( memcmp(caBuffer,"ADDE",4)==0)
        {
            pLink = GetFailTcpLink(atoi(caBuffer+4) );
            if ( pLink !=  NULL)
            {
                dcs_log(0,0,"该链路已存在，不能再增加！");
                continue;
            }
            pLink = GetFreeTcpLink( );
            if ( pLink ==  NULL)
            {
                dcs_log(0,0,"没有空闲链路，不能建链！");
                continue;
            }
            if ( AddLink(caBuffer+4,pLink) ==0)
            {
                dcs_log(0,0,"不能从参数文件中找到对应链路信息!");
                continue;
            }
            if ( pthread_create(&sthread, NULL,( void * (*)(void *))MakeConnection,pLink) !=0)
            {
                dcs_log(0,0, "Can not create thread ");
                break;
            }
            pthread_detach(sthread);
        }
        else if ( memcmp(caBuffer,"MODI",4)==0)
        {
            pLink = GetFailTcpLink(atoi(caBuffer+4) );
            if ( pLink ==  NULL)
            {
                dcs_log(0,0,"该链路不存在，不能修改 link id= [%s]",caBuffer+4);
                continue;
            }
            if (StopLink(caBuffer+4) ==0)
            {
                continue;
            }
            ModifyLink(pLink);
            if ( pthread_create(&sthread, NULL,( void * (*)(void *))MakeConnection,pLink) !=0)
            {
                dcs_log(0,0, "Can not create thread ");
                break;
            }
            pthread_detach(sthread);
        } else if ( memcmp(caBuffer,"STOP",4)==0)
        {
            StopLink(caBuffer+4);
        } else if ( memcmp(caBuffer,"STAR",4)==0)
        {
            
            pLink = GetFailTcpLink(atoi(caBuffer+4) );
            if ( pLink ==  NULL)
            {
                dcs_log(0,0,"链路不存在，不能建链！");
                continue;
            }
            if ( pLink->iSocketId >=0)
            {
                close( pLink->iSocketId);
                pLink->iSocketId =-1;
            }
            if ( pthread_create(&sthread, NULL,( void * (*)(void *))MakeConnection,pLink) !=0)
            {
                dcs_log(0,0, "Can not create thread ");
                break;
            }
            pthread_detach(sthread);
        }
	}
	close(fid);
}

struct secLinkInfo *GetFailTcpLink( int num )
{
	int i ;
	for ( i=0; i<g_pSecCfg->maxLink;i++)
	{
        if( !(g_pSecCfg->secLink[i].lFlags & DCS_FLAG_USED))
            continue;
        if ( g_pSecCfg->secLink[i].iNum ==  num)
            break;
        
	}
	if ( i == g_pSecCfg->maxLink)
	    return NULL;
	return &g_pSecCfg->secLink[i];
}

struct secLinkInfo *GetFreeTcpLink( )
{
	int i ;
	for ( i=0; i<g_pSecCfg->maxLink;i++)
	{
        if( g_pSecCfg->secLink[i].lFlags & DCS_FLAG_USED)
            continue;
        else
            break;
	}
	if ( i == g_pSecCfg->maxLink)
	    return NULL;
	return &g_pSecCfg->secLink[i];
}

int StopLink( char *num)
{
	int i;
    for (i=0; i <= g_pSecCfg->maxLink-1; i++)
  	{
        if (!(g_pSecCfg->secLink[i].lFlags & DCS_FLAG_USED))
            continue;
        if (atoi(num) !=g_pSecCfg->secLink[i].iNum)
            continue;
        
        break;
  	}
    
  	if (i>= g_pSecCfg->maxLink)
  	{
        dcs_log(0,0,"Link '%s' not exists!\n",num);
        return 0;
  	}
  	sem_lock( g_pSecCfg->secSemId,1,1);
    
    /*  	if(g_pSecCfg->secLink[i].iWorkFlag == 1)
     {
     sem_unlock( g_pSecCfg->secSemId,1,1);
     //  			sleep(1);
     dcs_log(0,0,"当前链路正在工作状态，等待一秒\n");
     return 0;
     }
     */
  	if ( g_pSecCfg->secLink[i].iStatus == DCS_STAT_CONNECTED )
  	{
        g_pSecCfg->secLink[i].iStatus = DCS_STAT_STOPPED;
        close(g_pSecCfg->secLink[i].iSocketId);
        dcs_log(0,0,"close a link , sockid =%d \n",g_pSecCfg->secLink[i].iSocketId);
        g_pSecCfg->secLink[i].iSocketId=-1;
        sem_unlock( g_pSecCfg->secSemId,1,1);
  	}
  	return 1;
}

int ModifyLink(struct secLinkInfo *pLink )
{
    char   caFileName[256],caBuffer[512];
  	int    iFd=-1,iRc/*,iCommCnt*/,iSlotIndex;
  	char *pEncType,*pAddr,*pPort,*pPort1,*pTime,*pNotiFlag,*pNotiFold,*pNum;
  	
  	//设定commlnk定义文件的绝对路径，并打开
  	memset(caFileName, 0, sizeof(caFileName) );
  	sprintf(caFileName, "%s/config/%s",getenv("ICS_HOME"),DCS_SEC_NAME);
  	if ((iFd = conf_open(caFileName)) < 0)
  	{
        dcs_log( 0,0, "<HstSrv> open secusrv.conf fail ! ");
        return -1;
  	}
    iSlotIndex=0;
    for( iRc = conf_get_first_string(iFd, "comm",caBuffer);\
        iRc == 0 ;\
        iRc = conf_get_next_string(iFd, "comm",caBuffer))
    {
        pNum        = strtok(caBuffer," \t\r");
        pEncType    = strtok(NULL," \t\r");  //通道类型
        pPort1      = strtok(NULL," \t\r");  //本地端口
        pAddr       = strtok(NULL   ," :\t\r");  //地址串
        pPort       = strtok(NULL   ," \t\r");  //端口号
        pTime        = strtok(NULL   ," \t\r\n");//通信超时检测时间
        pNotiFlag     = strtok(NULL," \t\r"); //通讯链路状态通知标志
        pNotiFold    = strtok(NULL," \t\r"); //通讯链路状态通知目标文件夹
        if (pEncType==NULL || pAddr==NULL || pTime==NULL || pNotiFlag==NULL || pNotiFold==NULL ||
            pPort==NULL  )
        {
            //invalid address:skip this line
            continue;
        }
        if ( atoi(pNum) != pLink->iNum) continue;
        iSlotIndex =1;
        //                dcs_debug(0,0,"给变量赋值");
        pLink->lFlags |= DCS_FLAG_USED;
        //                dcs_debug(0,0,"赋值成功1");
        pLink->iStatus = DCS_STAT_STOPPED;
        //                dcs_debug(0,0,"赋值成功2");
        strcpy(pLink->caRemoteIp,pAddr);
        //                dcs_debug(0,0,"赋值成功3");
        memcpy(pLink->cEncType,pEncType,3);
        //                dcs_debug(0,0,"赋值成功4");
        if ( u_stricmp(pPort1,"null") ==0)
            pLink->iLocalPort =0;
        else
            pLink->iLocalPort = atoi(pPort1);
        //                	 dcs_debug(0,0,"赋值成功5");
        pLink->iRemotePort = atoi(pPort);
        //                dcs_debug(0,0,"赋值成功6");
        pLink->iSocketId=-1;
        //                dcs_debug(0,0,"赋值成功7");
        pLink->iTimeOut=atoi(pTime);
        
    }
    return iSlotIndex;
}

int  AddLink(char *caNum ,struct secLinkInfo *pLink )
{
	char   caFileName[256],caBuffer[512];
  	int    iFd=-1,iRc,iCommCnt,iSlotIndex;
  	char *pEncType,*pAddr,*pPort,*pPort1,*pTime,*pNotiFlag,*pNotiFold,*pNum;
  	
  	//设定commlnk定义文件的绝对路径，并打开
  	memset(caFileName, 0, sizeof(caFileName) );
  	sprintf(caFileName, "%s/config/%s",getenv("ICS_HOME"),DCS_SEC_NAME);
  	if ((iFd = conf_open(caFileName)) < 0)
  	{
        dcs_log( 0,0, "<HstSrv> open secusrv.conf fail ! ");
        return -1;
  	}
    iSlotIndex=0;
    for( iRc = conf_get_first_string(iFd, "comm",caBuffer);
        iRc == 0 ;
        iRc = conf_get_next_string(iFd, "comm",caBuffer))
    {
        pNum        = strtok(caBuffer," \t\r");
        pEncType    = strtok(NULL," \t\r");  //通道类型
        pPort1      = strtok(NULL," \t\r");  //本地端口
        pAddr       = strtok(NULL   ," :\t\r");  //地址串
        pPort       = strtok(NULL   ," \t\r");  //端口号
        pTime        = strtok(NULL   ," \t\r\n");//通信超时检测时间
        pNotiFlag     = strtok(NULL," \t\r"); //通讯链路状态通知标志
        pNotiFold    = strtok(NULL," \t\r"); //通讯链路状态通知目标文件夹
        if (pEncType==NULL || pAddr==NULL || pTime==NULL || pNotiFlag==NULL || pNotiFold==NULL ||
            pPort==NULL  )
        {
            //invalid address:skip this line
            continue;
        }
        if ( atoi(pNum) != atoi(caNum)) continue;
        iSlotIndex =1;
        //                dcs_debug(0,0,"给变量赋值");
        pLink->lFlags |= DCS_FLAG_USED;
        //                dcs_debug(0,0,"赋值成功1");
        pLink->iStatus = DCS_STAT_STOPPED;
        //                dcs_debug(0,0,"赋值成功2");
        strcpy(pLink->caRemoteIp,pAddr);
        //                dcs_debug(0,0,"赋值成功3");
        memcpy(pLink->cEncType,pEncType,3);
        //                dcs_debug(0,0,"赋值成功4");
        if ( u_stricmp(pPort1,"null") ==0)
            pLink->iLocalPort =0;
        else
            pLink->iLocalPort = atoi(pPort1);
        //                	 dcs_debug(0,0,"赋值成功5");
        pLink->iRemotePort = atoi(pPort);
        //                dcs_debug(0,0,"赋值成功6");
        pLink->iSocketId=-1;
        //                dcs_debug(0,0,"赋值成功7");
        pLink->iTimeOut=atoi(pTime);
        pLink->iNum = atoi(pNum);
        
    }
    return iSlotIndex;
}
int DeleteLink(struct secLinkInfo *pLink)
{
	char caBuf[10];
	sprintf(caBuf,"%d",pLink->iNum);
	if ( StopLink(caBuf) ==0)
	{
        return 0;
	}
	pLink->lFlags &= ~DCS_FLAG_USED;
	return 1;
}
void TransProc ()
{
	int qid,qid1,nRcvLen,iSockId;
	char caBuf[PACKAGE_LENGTH+1];
	char szTmp[20];
	struct MSG_STRU *pMsg;
	long iType;
	long messType = 0;
	
	qid = queue_create("SECURVR");
	if ( qid <0)
        qid= queue_connect("SECURVR");
    if ( qid <0)
    {
  		dcs_log(0,0,"Can not connect message queue !");
  		return ;
    }
	while (1)
	{
        
        memset(caBuf,0,sizeof(caBuf));
        
        //printf("TransProc......\n");
        dcs_log(0, 0, "TransProc......qid = %d", qid);
        messType = 0;
        nRcvLen = queue_recv_by_msgtype( qid, caBuf, sizeof(caBuf), &messType, 0);
        
       //dcs_log(0, 0, "TransProc......receive msg = %s", caBuf);
        
        if ( nRcvLen <0)
        {
		 	dcs_log(0,0,"Recive Message fail!");
		 	break;
            
        }
        pMsg = (struct MSG_STRU *)caBuf;
        iType = pMsg->iPid;
        iSockId =  atoi(pMsg->caSocketId);
		
	 memset(szTmp,0,sizeof(szTmp));	
	 sprintf(szTmp,"%d",iType);
	 
        qid1= queue_connect(szTmp);
        if ( qid1 <0)
		qid1 = queue_create(szTmp);
	 if ( qid1 <0)
	 {
	  	dcs_log(0,0,"Can not connect message queue %s!",szTmp);
	  	return ;
	 }	
        //	 iNum =  pMsg->iNum;
        
        //dcs_debug(caBuf,nRcvLen,"recv data");
        dcs_debug(pMsg->caMsg,nRcvLen-sizeof(long)-5,"发送到加密机的数据包 sockid=%d,messtype=%d",iSockId,messType);
        
        if( 0>= write_msg_to_AMP( iSockId, pMsg->caMsg, nRcvLen-sizeof(long)-5,300))
        {
		  	memcpy(caBuf+sizeof(long),"FF",2);
		  	queue_send_by_msgtype( qid1, caBuf, sizeof(long)+2, messType,1);
		  	dcs_log(pMsg->caMsg,nRcvLen-sizeof(long)-5,"Send Data fail !");
		  	continue;
            //		  	return ;
        }
        memset(caBuf,0,sizeof(caBuf));
        memcpy(caBuf,&iType,sizeof(long));
        memcpy(caBuf+sizeof(long),"00",2);
        //nRcvLen = read_msg_from_AMP(iSockId, caBuf+sizeof(long)+2, sizeof(caBuf),-1);
        //nRcvLen = read_msg_from_AMP(iSockId, caBuf+sizeof(long)+2, sizeof(caBuf),2000);
        nRcvLen = read_msg_from_AMP(iSockId, caBuf+sizeof(long)+2, sizeof(caBuf),8000);
        if ( nRcvLen <=0)
        {
            memcpy(caBuf,&iType,sizeof(long));
            memcpy(caBuf+sizeof(long),"FF",2);
            queue_send_by_msgtype( qid1, caBuf, sizeof(long)+2, messType,1);
            dcs_log(0,0,"接受数据错误=%d",nRcvLen);
            dcs_log(0,0,"Recv Data fail");
            continue;
            //		  		return ;
        }
        dcs_debug(caBuf+sizeof(long)+2,nRcvLen,"从加密机收到的数据 len =%d,messtype=%d,qid1=%d",nRcvLen,messType,qid1);
        //dcs_debug(caBuf, nRcvLen + sizeof(long)+5,"从加密机收到的数据 len =%d",nRcvLen); //Freax Modify
        queue_send_by_msgtype( qid1, caBuf, nRcvLen+sizeof(long)+2, messType,1);
	}
}
