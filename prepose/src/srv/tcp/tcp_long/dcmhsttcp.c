#define _MainProg

// 程序名称：dcmtcp.c
// 功能描述：
//           成员行TCP/IP通信程序
// 函数清单：
//           main()

#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include  "dccglobl.h"
#include  "dccpgdef.h"
#include  "dccdcs.h"
#include  "dccsock.h"
#include  "tmcibtms.h"


static   pid_t            g_pidChild = -1,g_pidParent=-1;
static   int              g_iSock     = -1;
static   struct linkinfo *g_pLinkEntry = NULL;
static   char             g_caIdent[64]={0x00};
static   char             g_iType[5];
static   int                g_sem_id =-1;
static struct hstcom *pLinkCtrl = NULL;

static void TcpipExit(int signo);
static struct linkinfo *  GetLinkInfo(char *pcFoldName,char * commNO);
static int MakeConnection( struct linkinfo *);
static int NotifyMoni ( char *buf );

static int TcpipOpenLog( char * buf);
static int ReadFromIse();
static int ReadFromNet();
static void * SendHeartData( );
void * TestLink();
int Lock_unlock(int lock);
    
// 函数名称：main()
// 参数说明：
//           argv[0] 可执行程序名称
//           argv[1] 和本通信程序相连的文件夹名
// 返 回 值：
//           无
// 功能描述：
//           1.接收来自成员行的报文,并转交给相应的应用进程
//           2.接收本地应用的报文,并将其发往网络

int main(int argc, char *argv[])
{
	catch_all_signals(TcpipExit);

	/*check the command line and get my folder's name*/
	if (argc < 3)
	{
		printf("<Tcpip> Usage: %s folderName.",argv[0]);
		exit(1);
	}
	else /*argv[1] is the folder name*/
	{
		memset(g_caIdent, 0, sizeof(g_caIdent));
		sprintf(g_caIdent, "%s%s", argv[1],argv[2]);
	}

	g_pidParent=-1;
	if (TcpipOpenLog(g_caIdent) < 0)
		exit(1);

	//连接folder系统
	dcs_log(0,0,"connect folder system");

	if (fold_initsys() < 0)
	{
		dcs_log(0,0,"<Tcpip--%s> cannot attach to FOLDER !",g_caIdent);
		goto lblExit;
	}

	/*连接共享内存HSTCOM,获得与自己对应的link信息*/
	dcs_log(0,0,"locate link msg");

	g_pLinkEntry = GetLinkInfo(argv[1],argv[2]);

	if (g_pLinkEntry == NULL)
	{
		dcs_log(0,0,"<Tcpip--%s> cannot get corresponding item in link info array !",g_caIdent);
		goto lblExit;
	}

	dcs_log(0,0,"<Tcpip--%s> link role [%d]",g_caIdent,g_pLinkEntry->iRole);

	memcpy(g_iType,g_pLinkEntry->caMsgType,4);

	//根据folder名建立信号量
	g_sem_id = sem_connect(argv[1], 2);				
	if (g_sem_id < 0) 					
		g_sem_id = sem_create(argv[1], 2);				
	if (g_sem_id < 0) 				
	{					
		dcs_log(0,0,"<Tcpip--%s>信号量%s创建失败,%s",g_caIdent,argv[1],strerror(errno));
		exit(1);
	}			


	/*建立和对方的网络链接,获得连接套接字*/
	dcs_log(0,0,"create socket");

	g_iSock = MakeConnection(g_pLinkEntry);

	dcs_log(0, 0, "g_iSock = %d", g_iSock);

	if(g_iSock < 0) 
	{
		dcs_log(0, 0, "connection error....");
		goto lblExit;
	}

	if ( g_pLinkEntry->iRole == DCS_ROLE_SIMPLEXCLI)
	{
		dcs_log(0, 0, "ReadFromIse  ...g_iSock = %d", g_iSock);
		ReadFromIse();
		goto lblExit; 
	}

	if ( g_pLinkEntry->iRole == DCS_ROLE_SIMPLEXSVR)
	{
		dcs_log(0, 0, "ReadFromNet  ...g_iSock = %d", g_iSock);
		ReadFromNet();
		goto lblExit; 
	}

	g_pidParent = getpid();

	if ((g_pidChild = fork()) < 0)
	{
		dcs_log(0,0,"<Tcpip--%s> cannot fork child process!",g_caIdent);
		goto lblExit;
	}

	dcs_log(0,0,"create Child process");

	/*利用两个进程来分别读/写网络*/
	if (g_pidChild > 0)
	{
		dcs_log(0, 0, "parent here,read from network! ");
		/*parent here,read from network*/
		g_pidParent =-1;
		ReadFromNet();
		goto lblExit;
	}
	else
	{
		/*child here,read from ISE*/
		ReadFromIse();
	}

lblExit:
	dcs_log(0,0,"lblExit() failed:%s\n",strerror(errno));
	TcpipExit(0);

	return 0;
}


/*
 函数名称  TcpipExit
 参数说明：
          signo   捕获的信号值
 返 回 值：
        无
功能描述：
          当捕获的信号为 SIGTERM时，该函数释放资源后终止当前进程;否则
          不做任何处理
*/
void TcpipExit(int signo)
{
	char buf[512];
	dcs_log(0,0,"<Tcpip--%s> catch a signal %d !",g_caIdent, signo);

	memset(buf,0,sizeof(buf));
	//parent process
	switch (signo)
	{
		case SIGUSR2: /*dcmcmd want me to stop */
			g_pLinkEntry->iStatus = DCS_STAT_STOPPED;
			dcs_log(0,0,"<Tcpip--%s> dcmcmd stop the link !",g_caIdent);
			break;
		case 0:	
		case 11:
		case SIGTSTP:/*线程通知重启*/
			if ( g_pLinkEntry->iStatus != DCS_STAT_STOPPED && g_pLinkEntry->iStatus != DCS_STAT_DISCONNECTED)
			{
				if (  g_pLinkEntry->iStatus != DCS_STAT_STOPPED)
					g_pLinkEntry->iStatus = DCS_STAT_DISCONNECTED;
				dcs_log(0,0,"current proc id=%d,child proc id=%d,parent proc id =%d",getpid(),g_pidChild, g_pidParent);
				sprintf(buf,"SYSICOMML %s %3.3s %d disconnect",g_pLinkEntry->caFoldName,g_pLinkEntry->commNO,g_pLinkEntry->iStatus);
				NotifyMoni( buf );
			}
			else
			{
				dcs_log(0,0,"<Tcpip--%s>sleep 10ms 等待其父进程或子进程发送重启命令给hstserv !",g_caIdent);
				u_sleep_us(10000);
			}
			break;
		case SIGTERM:/*脚本停服务*/
			dcs_log(0,0,"<Tcpip--%s>脚本停止服务 !",g_caIdent);
			break;
		case SIGCHLD:
			if (g_pidChild == wait(NULL))
				g_pidChild = -1;
			break;
		
			
		default:
			g_pLinkEntry->iStatus = DCS_STAT_STOPPED;
		break;
	}

	if (g_pidChild > 0 ) /*parent should notify its child*/
		kill(g_pidChild, SIGUSR1);

	if (g_pidParent > 0 ) /*child should notify its parent*/
		kill(g_pidParent, SIGUSR1);

	//close the socket
	tcp_close_socket(g_iSock);
	g_iSock = -1;
	dcs_log(0,0,"exit system");
	exit(signo);
}

// 函数名称  TcpipOpenLog
// 参数说明：
//          无
// 返 回 值：
//          成功返回0,否则-1
// 功能描述：
//          1.获得系统主目录,将之放在全局变量g_pcIseHome
//          2.设置日志文件和日志方式

int TcpipOpenLog( char * buf)
{
	char   caLogName[256];
	char logfile[256];

	memset(caLogName,0,sizeof(caLogName));
	memset(logfile, 0, sizeof(logfile));
	sprintf(caLogName,"log/hst%s.log",buf);
	if(u_fabricatefile(caLogName,logfile,sizeof(logfile)) < 0)
	{
		dcs_log(0, 0, "can not open log file:[%s]",caLogName);
		return -1;
	}
	return  dcs_log_open(logfile, buf);
}


// 函数名称  GetLinkInfo
// 参数说明：
//          pcFoldName:文件夹的名称,用作查询条件
// 返 回 值：
//          若找到成功,则返回对应信息槽位的指针,否则NULL
// 功能描述：
//          1.连接共享内存HSTCOM
//          2.在数组LinkArray[]中查找和给定文件夹名称匹配的项

struct linkinfo * GetLinkInfo(char *pcFoldName ,char *commNO)
{
	int    i,s;
	s = 0;

	//连接共享内存HSTCOM
	pLinkCtrl = (struct hstcom *)shm_connect(DCS_HSTCOM_NAME, NULL);

	if (pLinkCtrl == NULL)
	{
		dcs_log(0,0,"<Tcpip--%s> cannot connect share memory %s!",g_caIdent,DCS_HSTCOM_NAME);
		return NULL;
	}

	dcs_log(0, 0, "pLinkCtrl->iLinkCnt = %d", pLinkCtrl->iLinkCnt);

	for (i=0; i < pLinkCtrl->iLinkCnt; i++)
	{
		if (u_stricmp(pLinkCtrl->linkArray[i].caFoldName,pcFoldName) == 0 && memcmp(pLinkCtrl->linkArray[i].commNO,commNO,3)==0)
		{
			dcs_log(0,0,"<Tcpip--%s>%s sock[%d]!",g_caIdent,pLinkCtrl->linkArray[i].caFoldName,i);
			break;
		}
		else
			continue;
	}
	if ( i>=pLinkCtrl->iLinkCnt)
		return NULL;

	return &pLinkCtrl->linkArray[i];
	
}

// 函数名称  MakeConnection
// 参数说明：
//          无
// 返 回 值：
//          成功返回0,否则返回套接字的描述符
// 功能描述：
//          建立和对方的套接字连接

int MakeConnection( struct linkinfo *spLinkEntry)
{
	if (spLinkEntry->iRole == DCS_ROLE_PASSIVE || spLinkEntry->iRole == DCS_ROLE_SIMPLEXSVR)
	{
		//we are the server end, create a listening socket and
		//wait to accept the clients' connection request
		int   iListenSock, iConnSock, iCliAddr;
		int   iCliPort;

		dcs_debug(0,0, "begin open socket.. port = %d", spLinkEntry->sLocalPort);
		dcs_log(0, 0, "caRemoteIp = %s, port = %d, caFoldName = [%s],caLocalIp = [%s], sLocalPort = %d", spLinkEntry->caRemoteIp, spLinkEntry->sRemotePort, spLinkEntry->caFoldName, spLinkEntry->caLocalIp,  spLinkEntry->sLocalPort);
		iListenSock = tcp_open_server(NULL,spLinkEntry->sLocalPort);

		if (iListenSock < 0)
		{
			dcs_log(0,0,"<Tcpip--%s> tcp_open_server() failed: %s! port:%d",g_caIdent,strerror(errno),spLinkEntry->sLocalPort);
			return -1;
		}

		dcs_debug(0,0, "open socket succ");
		spLinkEntry->iStatus = DCS_STAT_LISTENING;

		for (;;)
		{
			//waiting for clients
			memset(spLinkEntry->sActualIp, 0,sizeof(spLinkEntry->sActualIp));
			spLinkEntry->nActualPort = 0;
			
			iConnSock = tcp_accept_client(iListenSock,&iCliAddr, &iCliPort);

			if (iConnSock < 0)
			{
				dcs_log(0,0,"<Tcpip--%s> tcp_accept_client() failed: %s!", g_caIdent,strerror(errno));
				tcp_close_socket(iListenSock);
				return (-1);
			}
			dcs_log(0,0, "accept socket succ");

			struct in_addr in;
			in.s_addr = iCliAddr;

			dcs_log(0, 0, "clientIp = %s, clientPort = %d",  inet_ntoa(in), iCliPort);
			
			/* validate the client */
			if (strcmp(spLinkEntry->caRemoteIp, "null") != 0)
			{
				int   iAddr, sPort;

				iAddr = tcp_pton(spLinkEntry->caRemoteIp);
				sPort = spLinkEntry->sRemotePort;

				if ( ((sPort >0) && (sPort != iCliPort)) || ((iAddr != INADDR_NONE) && (iCliAddr != iAddr)))
				{
					dcs_log(0,0,"<Tcpip--%s> caRemoteIp[%s] ,RemotePort[%d] refused!",g_caIdent,spLinkEntry->caRemoteIp,sPort);
					tcp_close_socket(iConnSock); 
					continue;  /* accept() again */
				}
			}

			/* the connection established successfully */
			spLinkEntry->iStatus = DCS_STAT_CONNECTED;
			strcpy(spLinkEntry->sActualIp, inet_ntoa(in));
			spLinkEntry->nActualPort = iCliPort;
			
			tcp_close_socket(iListenSock);
			return (iConnSock);
		}//for(;;)
	} 
	else
	{
		//we are the client end, create a socket and
		//try to connect the server
		int   iConnSock;
		dcs_log(0,0,"<Tcpip--%s> try connect to %s, port = %d...", g_caIdent, spLinkEntry->caRemoteIp,spLinkEntry->sRemotePort);
		while(1)
		{
			spLinkEntry->iStatus = DCS_STAT_CALLING;
			iConnSock = tcp_connet_server(spLinkEntry->caRemoteIp,spLinkEntry->sRemotePort,spLinkEntry->sLocalPort);

			if (iConnSock < 0)
			{
				sleep(1);
				continue;
			}
			else
			{
				dcs_log(0,0,"iConnSock = %d", iConnSock);
			       break;
			}
		}
		/*connection established successfully */
		spLinkEntry->iStatus = DCS_STAT_CONNECTED;
		strcpy(spLinkEntry->sActualIp, spLinkEntry->caRemoteIp);
		spLinkEntry->nActualPort = spLinkEntry->sRemotePort;
			
		return(iConnSock);
	}//else
}

// 函数名称  ReadFromNet
// 参数说明：
//          无
// 返 回 值：
//          always -1
// 功能描述：
//          从网络上读取交易报文,然后将其转交给相应的应用进程

int ReadFromNet()
{
	char  caBuffer[PACKAGE_LENGTH+4+1], *ptr=NULL;
	int   nBytesRead, nBytesWritten;
	int   ntimeout = 0;
	//time_t hearttime,t;
	
	memset(caBuffer,0x00,sizeof(caBuffer));
	if ( g_pLinkEntry == NULL)
	{
		dcs_log(0,0,"InValid para!");
		return (-1);
	}

	if ( memcmp(g_pLinkEntry->commType,"bnk",3)!=0 && memcmp(g_pLinkEntry->commType,"trm",3)!=0 
	&& memcmp(g_pLinkEntry->commType,"con",3)!=0)
	{
		dcs_log(0,0,"InValid comm type!");
		return (-1);
	}

	memcpy(caBuffer,g_iType,4);
	ptr = caBuffer+4;
	/*需要检测心跳包时,将超时时间设置为心跳包发送间隔的6倍*/
	if(g_pLinkEntry->heart_check_flag == '1')
	{
		ntimeout = g_pLinkEntry->heart_check_time * 6*1000;
		//time(&hearttime);
	}
	else
	{
		if (g_pLinkEntry->iTimeOut <=0 || g_pLinkEntry->iTimeOut >500)
			ntimeout = -1;
		else
			ntimeout = g_pLinkEntry->iTimeOut *1000; /*不知道有啥用*/

	}
	
	for (;;)
	{
		nBytesRead=0;
		 memset(caBuffer+4,0x00,sizeof(caBuffer)-4);
		
		if (memcmp(g_pLinkEntry->commType,"trm",3)==0)
			nBytesRead = read_msg_from_NAC(g_iSock, ptr,(int)sizeof(caBuffer)-4-1, ntimeout);
		else if (memcmp(g_pLinkEntry->commType,"bnk",3)==0)
			nBytesRead = read_msg_from_net(g_iSock, ptr,sizeof(caBuffer)-4-1, ntimeout);
		else if (memcmp(g_pLinkEntry->commType,"con",3)==0)
			nBytesRead = read_msg_from_CON(g_iSock, ptr,sizeof(caBuffer)-4-1, ntimeout);

		if (nBytesRead <0)
		{
			if((errno == ETIMEDOUT ) && (g_pLinkEntry->heart_check_flag == '1'))
			{
				dcs_log(0,0,"<Tcpip--%s>超时未收到客户端心跳包!",g_caIdent);
			}
			else
			{
				dcs_log(0,0,"<Tcpip--%s> read msg from net failed:%s!",g_caIdent,strerror(errno));
			}
				
			break;
		}
/*只要读到内容就认为链路是通的，无需必须收到心跳包
		if( g_pLinkEntry->heart_check_flag == '1' )
		{
			time(&t);
			if(t - hearttime > ntimeout/1000+1)
			{
				dcs_log(0,0,"超时未收到客户端心跳包!",g_caIdent);
				break;
			}
		}
*/
		if (nBytesRead == 0) //a echo test request from client
		{
			if( g_pLinkEntry->heart_check_flag == '1' )
			{
				dcs_log(0,0,"<Tcpip--%s>收到一个心跳包 ",g_caIdent);
				//time(&hearttime);
			}
			continue;    //read from net again
		}	

		ptr[nBytesRead] = 0;
		dcs_debug(caBuffer+4,nBytesRead,"<Tcpip--%s> Read Net %.4d bytes-->",g_caIdent,nBytesRead);

		/*将读到的报文写到对应处理进程的FOLDER*/
		nBytesWritten = fold_write(g_pLinkEntry->iApplFid,g_pLinkEntry->iFid,caBuffer,nBytesRead+4);

		if (nBytesWritten < nBytesRead+4)
		{
			dcs_log(0,0,"<Tcpip--%s> fold_write() failed:%s errno=%d",g_caIdent,strerror(errno),errno);
			goto lblReturn;
		}

	}//for(;;)

lblReturn:
	//fold_purge_msg(g_pLinkEntry->iFid);
	tcp_close_socket(g_iSock);
	g_iSock = -1;
	return (-1);
}

// 函数名称  ReadFromIse
// 参数说明：
//          无
// 返 回 值：
//          always -1
// 功能描述：
//          从文件夹读取应用进程产生的交易报文,然后将其发向网络

int ReadFromIse( )
{
  	char  caBuffer[PACKAGE_LENGTH+1];
  	int   nBytesRead, nBytesWritten;
  	int   iOrgFid;
    	int fid,time;
	pthread_t threadid;    

	/*判断是否需要发送心跳包*/
	if(g_pLinkEntry->heart_check_flag == '1')
	{
		if(pthread_create(&threadid, NULL, SendHeartData, NULL) != 0 )
		{
			dcs_log(0, 0,"creat pthread fail");
			return -1;
		}
		pthread_detach(threadid);
	}
	
	if (g_pLinkEntry->iTimeOut <=0 || g_pLinkEntry->iTimeOut >500)
		time = -1;
	else
		time = g_pLinkEntry->iTimeOut *1000;
	fid = fold_open(g_pLinkEntry->iFid);
	
  	fold_set_maxmsg(g_pLinkEntry->iFid, 500) ;
 	
       dcs_log(0, 0, "ReadFromIse while .....g_pLinkEntry->iFid = %d", g_pLinkEntry->iFid);

	/*检测链路*/
	threadid =0;
	if(pthread_create(&threadid, NULL, TestLink, NULL) != 0 )
	{
		dcs_log(0, 0,"creat pthread fail");
		return -1;
	}
	pthread_detach(threadid);
	
  	while(1)
  	{
		Lock_unlock(1);
		memset(caBuffer,0,sizeof(caBuffer));
		nBytesRead = fold_read(g_pLinkEntry->iFid,fid,&iOrgFid,caBuffer,sizeof(caBuffer),1);               
		 Lock_unlock(0);
		if(nBytesRead ==0)
		{	
			 continue; 
		}

		if (nBytesRead < 0)
		{
			dcs_log(0,0,"<Tcpip--%s> fold_read() failed:%s",g_caIdent,strerror(errno));
			break;
		}

		nBytesWritten = -1;
		
		if ( memcmp(g_pLinkEntry->commType,"trm",3)==0)
		{
			nBytesWritten = write_msg_to_NAC(g_iSock,caBuffer,nBytesRead,time);
		}
		else if ( memcmp(g_pLinkEntry->commType,"bnk",3)==0)
		{
		    	nBytesWritten = write_msg_to_net(g_iSock,caBuffer,nBytesRead,time);
		}
		else if (memcmp(g_pLinkEntry->commType,"con",3)==0)
		{
		    	nBytesWritten = write_msg_to_CON(g_iSock,caBuffer,nBytesRead,time);
		}
		if (nBytesWritten < nBytesRead)
		{
		   	 dcs_log(0,0,"<Tcpip--%s> write_msg_to_net() failed:%s",g_caIdent,strerror(errno));
			
		    	 break; 
		}
			
		if (nBytesWritten > 4)  //for debug
		{
		    	dcs_debug(caBuffer,nBytesWritten,"<Tcpip--%s> write Net %.4d bytes-->",g_caIdent,nBytesWritten);
		}
		dcs_log(0,0,"write Net succ");
		u_sleep_us(1000); //sleep 1ms 
  	}//for(;;)
	tcp_close_socket(g_iSock);
	//fold_purge_msg(g_pLinkEntry->iFid); //Freax TO DO: 
	close(fid);
	return (-1);
}

int NotifyMoni ( char *buf )
{
	int qid,iRc;
	struct msqid_ds MsgQid_Buf;
    //	dcs_debug(0,0,"entry notify");
	qid = queue_connect("monitor");
	if ( qid <=0 )
	{
	 	dcs_log(0,0,"File:%s,Line:%d: Connect Moni Error ",__FILE__,__LINE__);
		
        return -1;
	}
	iRc = msgctl(qid,IPC_STAT,&MsgQid_Buf);
	if(iRc < 0)
	{
		dcs_log(0,0,"File:%s,Line:%d: Get Stat Error,Error Code = %d",__FILE__,__LINE__,iRc);
		
		return -1;
	}
	if(MsgQid_Buf.msg_qnum > 40)
	{
		dcs_log(0,0,"File:%s,Line:%d: Send Moni Message limited  ",__FILE__,__LINE__);
        return -1;
	}
	queue_send( qid, buf, (int)strlen(buf),0);
	dcs_debug(0,0,"finilsh send data =[%s]",buf);
	return 0;
}
/*
*发送心跳包
*/
void * SendHeartData( )
{
	int time = 0;	
	char buffer[100];
	int len = 0;
	int sendlen = 0;
	
	if (g_pLinkEntry->iTimeOut <=0 || g_pLinkEntry->iTimeOut >500)
		time = -1;
	else
		time = g_pLinkEntry->iTimeOut *1000;

	/*发送一个空报文作为心跳包*/
	memset(buffer, 0, sizeof(buffer));	
	
	 for (;;)
  	{	            
		if ( memcmp(g_pLinkEntry->commType,"trm",3)==0)
		{
			sendlen = write_msg_to_NAC(g_iSock,buffer,len,time);
		}
		else if ( memcmp(g_pLinkEntry->commType,"bnk",3)==0)
		{
		    	sendlen = write_msg_to_net(g_iSock,buffer,len,time);
		}
		else if ( memcmp(g_pLinkEntry->commType,"amp",3)==0)
		{
		   	 sendlen = write_msg_to_AMP(g_iSock,buffer,len,time);
		}
		else if (memcmp(g_pLinkEntry->commType,"con",3)==0)
		{
		    	sendlen = write_msg_to_CON(g_iSock,buffer,len,time);
		}
		if (sendlen < len)
		{
		   	 dcs_log(0,0,"<Tcpip--%s> write_msg_to_net() failed:%s",g_caIdent,strerror(errno));
			 kill(getpid(), SIGTSTP);/*向本进程发送信号*/
		    	 break; /* 待处理 */
		}
		dcs_log(0,0,"<Tcpip--%s>发送一个心跳包 ",g_caIdent);
		sleep(g_pLinkEntry->heart_check_time);/* 休息一会再发送 */
		
  	}//for(;;)
  	return (void*)0;
}


/*链路检测
*/
void * TestLink()
{
	struct tcp_info info;  
	int len=sizeof(info); 

	while(1)
	{
		memset(&info,0,sizeof(info));  
		getsockopt(g_iSock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len); /*检测socket状态*/
		if(info.tcpi_state!= TCP_ESTABLISHED ) /* TCP_ESTABLISHED =1表示TCP链路正常*/
		{  
		     dcs_log(0,0,"<Tcpip--%s> socket disconnected",g_caIdent);
		     break;
		}  

		u_sleep_us(10000); //sleep 10ms 
	}
	kill(getpid(),SIGTSTP);//通知进程链路断了
}

/* 对信号量进行上锁或是解锁
* 参数:1表示上锁,0表示解锁
*/
int Lock_unlock(int lock)
{		
	if (lock) 					
	{		
		sem_lock(g_sem_id, 1, 1);	
		dcs_log(0, 0, "fold_read_lock_sem_id = %d", g_sem_id);						
	}					
	else					
	{						
		dcs_log(0, 0, "fold_read_unlock_sem_id = %d", g_sem_id);						
		sem_unlock(g_sem_id, 1, 1);	
	}		
	return 0;		
}


