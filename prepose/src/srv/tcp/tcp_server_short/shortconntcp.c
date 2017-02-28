/*
*  shortconntcp.c
*
*   Created by Freax
*  Copyright (c) Chinaebi. All rights reserved.
*/
#include <time.h>
#include <math.h>
#include "short_conn.h"

static   char             g_port[6]={0x00};
static   char             g_iType[5];
static   int                g_foldid=0;
static   char             g_foldname[20]={0x00};
struct conn_s_Ast *pScLinkCtrl = NULL;//共享内存SC_SERVER中的短链信息

static process processes[MAX_PORCESS];
static int listen_sock;
static int current_total_processes;
static int current_max_tpdu;

pthread_mutex_t process_mutex;

static void shortconnexit(int signo);
static int MakeConnection( struct conn_s *);
static int accept_sock(int sock);
static struct conn_s_Ast * GetScLinkInfo();
static void *deal_request( void* args);
static int SocketConnected( process* proce,process *currProc) ;
static void do_folder_rev_service();
static void do_service(int lfd);
static void cleanup(process *process);
static void init_processes();
static int find_process_by_tpdu(char *buf);
static int setScStatue(int iStatus);
static int getScStatue(int *iStatus);
static int ReStartProcess(const char * strExeFile, const char * arg1);

    
// 函数名称：main()
// 参数说明：
//           argv[1] 要监听的端口号
// 返 回 值：
//           无
// 功能描述：
//           1.接收来自成员行的报文,并转交给相应的应用进程
//           2.接收本地应用的报文,并将其发往网络

int main(int argc, char *argv[])
{
	catch_all_signals(shortconnexit);

	if (0 > ScOpenLog(argv[1]))
		 goto lblExit;

	//check the command line and get my folder's name
	if (argc < 2)
	{
		dcs_log(0,0,"<ScTcpip> Usage: %s portId.",argv[0]);
		goto lblExit;
	}
	else //argv[1] is the folder name
	{
		memset(g_port, 0, sizeof(g_port));
		strcpy(g_port, argv[1]);
	}
	//连接folder系统
	dcs_debug(0,0,"connect folder system");

	if (fold_initsys() < 0)
	{
		dcs_log(0,0,"<ScTcpip--%s> cannot attach to FOLDER !",g_port);
		goto lblExit;
	}

	//=============== 开启链路服务 ================//
	//连接共享内存SC_SERVER,获得所有的短链link信息
	dcs_debug(0,0,"locate link msg");

	pScLinkCtrl= GetScLinkInfo();

	if (pScLinkCtrl == NULL)
	{
		dcs_log(0,0,"<ScTcpip--%s> cannot get corresponding item in link info array !",g_port);
		goto lblExit;
	}

	dcs_debug(0,0,"<ScTcpip--%s> link role ",g_port);
	//建立和对方的网络链接,获得连接套接字
	dcs_debug(0,0,"create socket");

	listen_sock= MakeScConnection(atoi(argv[1]));

	dcs_log(0, 0, "iListenSock = %d", listen_sock);

	if(listen_sock < 0) {
		dcs_log(0, 0, "connection error....");
		goto lblExit;
	}
	
	setScStatue(DCS_STAT_LISTENING);
	
	int res = pthread_mutex_init(&process_mutex, NULL);
	if(res != 0) {
		printf("process_mutex initialzation failed...\n");
		goto lblExit;
	}
	memset(processes,0,sizeof(process)*MAX_PORCESS);
	init_processes();
	//开启接受folder过来的数据的线程,从folder中读应答报文，并转发到对应的网络上
	pthread_t sthread;
	if (pthread_create(&sthread, NULL,( void * (*)(void *))do_folder_rev_service, NULL) !=0)
	{
		 dcs_log(0,0,"pthread_create fail");
		 goto lblExit;
	}
	pthread_detach(sthread);
	//=========================================//

	//父进程处理从网络读数据，并发给业务处理进程
	do_service(listen_sock);
lblExit:
	dcs_log(0,0,"lblExit() failed:%s\n",strerror(errno));
	shortconnexit(0);
	return 0;
}//main()

static void do_service(int lfid)
{
	int cfd;
	int nLocation=0;
	int lfd =listen_sock;
	while (1)
	{
		dcs_log(0,0,"waiting for connect request!");

		cfd = accept (lfd, (struct sockaddr *)NULL, NULL);
		if (cfd == -1) 
		{
			dcs_log(0,0,"fail to accept!");
			continue;
		}
			
		nLocation = accept_sock(cfd);
		if(nLocation < 0)
			continue;
		pthread_t sthread;
		if ( pthread_create(&sthread, NULL,deal_request, (void *)&processes[nLocation]) !=0)
		{
			 dcs_log(0,0,"create socket fail");
			close(cfd);	
		}

		pthread_detach(sthread);
	}
	close(lfd);
}

void init_processes()
{
	int i = 0;
	current_total_processes =0;
	current_max_tpdu = 0;
	for (;i < MAX_PORCESS; i ++)
	{
	    processes[i].sock = NO_SOCK;
	    processes[i].location= NO_SOCK;
	}
}

void cleanup(process *process) 
{
	int s;
	pthread_mutex_lock(&process_mutex);

	if (process->sock != NO_SOCK) 
	{
		s = close(process->sock);
		if (s == NO_SOCK) 
		{
			perror("close sock");
		}
		current_total_processes--;
		//dcs_log(0, 0, "close sock=[%d]",process->sock);
	}
	process->sock = NO_SOCK;
	process->location= NO_SOCK;
	pthread_mutex_unlock(&process_mutex);
}

int  find_empty_process_for_sock(int sock) 
{
	int i;
	if(current_max_tpdu == MAX_TPDU)
		current_max_tpdu =0;
	 
    	for (i = 0; i < MAX_PORCESS; i++) 
	{
        	if (processes[i].sock == NO_SOCK) 
		{
            		return i;
        	}
    	}
  	return -1;
}

static int accept_sock(int sock) 
{
	int location;
   	if (current_total_processes >= MAX_PORCESS) 
	{
		// 请求已满， accept 之后直接挂断
		dcs_log(0,0,"当前线程数已达到最大值[%d]",MAX_PORCESS);
		close(sock);
		return -1;
    	}
	pthread_mutex_lock(&process_mutex);
	location = find_empty_process_for_sock(sock);
	if(location <0)
	{
		close(sock);
		return -1;
	}
      	processes[location].sock = sock;
	processes[location].location= current_max_tpdu;
	current_total_processes++;
	current_max_tpdu++;
	pthread_mutex_unlock(&process_mutex);
	//dcs_log(0,0,"handle_request TPDU [%d]processes[%d].sock=[%d]",current_max_tpdu-1,location,processes[location].sock);
       return location;
}

//接受到处理数据，发送到请求者。
void do_folder_rev_service()
{
	int iOrgFid,location;
	int   nBytesRead, nBytesWritten;
	char  sBuffer[PACKAGE_LENGTH+1];
	process * process=NULL;
	int fold_id =g_foldid;
	int fid;
	fid=fold_open(g_foldid);
	while (1)
	{
		memset(sBuffer,0x00,sizeof(sBuffer));
		dcs_log(0, 0, "do_folder_rev_service while .....iFid = %d", fold_id);	
		nBytesRead= fold_read(fold_id,fid,&iOrgFid,sBuffer,sizeof(sBuffer),TRUE);
		dcs_debug(sBuffer,nBytesRead,"Read From short_link_folder [%s] data_buf len=%d,foldId=%d", g_foldname, nBytesRead, fold_id);

		if (nBytesRead < 0)
		{
			dcs_log(0,0,"<Tcpip--%s> fold_read() failed:%s",g_port,strerror(errno));
			break;
		}
		if (nBytesRead <= 5)
		{
			dcs_log(0,0,"读到的报文太短");
			continue;
		}
		
		//根据TPDU找到原socket信息
		location =find_process_by_tpdu(sBuffer);

		if(location== NO_SOCK)
		{
			dcs_log(sBuffer,nBytesRead,"socket已经关闭,不在转发");
			continue;
		}
		process =&processes[location];
		nBytesWritten = write_msg_to_shortTcp(process->sock,sBuffer,nBytesRead);
		if (nBytesWritten < nBytesRead)
		{
			dcs_log(0,0,"<Tcpip--%s> write_msg_to_net() failed:%s",g_port,strerror(errno));
			cleanup(process);
			continue;
		}
		dcs_log(0,0,"<Tcpip--%s> write_msg_to_net() succ!",g_port);

		// 读写完毕
		cleanup(process);
        }
	close(fid);
}


// 函数名称  GetScLinkInfo
// 返 回 值：
//          若找到成功,则返回对应信息槽位的指针,否则NULL
// 功能描述：
//          1.连接共享内存SC_SERVER

struct conn_s_Ast * GetScLinkInfo()
{
	int i,num=0;
	//连接共享内存HSTCOM
        pScLinkCtrl= (struct conn_s_Ast*)shm_connect(SHORT_CONN_SHM_NAME, NULL);
  	if (pScLinkCtrl == NULL)
  	{
	        dcs_log(0,0,"<ScTcpip--%s> cannot connect share memory %s!",g_port,SHORT_CONN_SHM_NAME);
	        return NULL;
  	}
       dcs_log(0, 0, "<ScTcpip--%s>pScLink->i_conn_num = %d",g_port, pScLinkCtrl->i_conn_num);

	for(i=0;i<pScLinkCtrl->i_conn_num;i++)
	{
		if(atoi(g_port) == pScLinkCtrl->conns[i].listen_port)
		{
			g_foldid= pScLinkCtrl->conns[i].iFid;
			strcpy(g_foldname,pScLinkCtrl->conns[i].name);
		}
			
	}
  	return pScLinkCtrl;
	
}

// 函数名称  MakeScConnection
// 参数说明：
//          无
// 返 回 值：
//          成功返回监听socket描述符，否则返回-1
// 功能描述：
//          建立和对方的套接字连接

int MakeScConnection( int port)
{
        //we are the server end, create a listening socket 
        int   iListenSock;
	
        dcs_log(0,0, "begin open socket.. port = %d", port);
	
        iListenSock = tcp_open_sc_server(NULL,port);
		
        if (iListenSock < 0)
        {
            dcs_log(0,0,"<ScTcpip--%s> tcp_open_server() failed: %s! port:%d",g_port,strerror(errno),port);
            return -1;
        }
        
        dcs_debug(0,0, " open socket succ");
        return iListenSock; 	
}

// 函数名称  ScOpenLog
// 参数说明：
//          无
// 返 回 值：
//          成功返回0,否则-1
// 功能描述：
//          1.获得系统主目录,将之放在全局变量g_pcIseHome
//          2.设置日志文件和日志方式

int ScOpenLog( char * buf)
{
	char  caLogName[256];
	char logfile[256];

	memset(caLogName,0,sizeof(caLogName));
	memset(logfile,0,sizeof(logfile));
	sprintf(caLogName,"log/ScServer%s.log",buf);
	if(u_fabricatefile(caLogName,logfile,sizeof(logfile)) < 0)
	{
		dcs_log(0, 0, "can not open log file:[%s]",caLogName);
		return -1;
	}
	return  dcs_log_open(logfile, buf);
}

//处理网络的请求,并发往对应的业务处理FOLDER
void *deal_request( void* args) 
{
	char buf[PACKAGE_LENGTH+1],writebuf[PACKAGE_LENGTH+4+1];
	char temp[100];
	int time =-1,nReadBytes=0,nWrittenBytes=0;
	int i,len=0,iMaxTry=5,iRet=0;
	process localprocess,*proce = NULL;
	struct conn_s *pscLink = NULL;
	proce = (process *)args;
	memset(buf,0x00,sizeof(buf));
	memset(&localprocess,0x00,sizeof(process));
	memcpy(&localprocess,proce,sizeof(process));
	
	//dcs_log(0,0,"socketid = [%d],TPDU=[%d]",localprocess.sock,localprocess.location);
	nReadBytes = read_msg_from_NAC(localprocess.sock, buf,(int)sizeof(buf)-1,10* 1000);
	if(nReadBytes < 0)
	{
		//read error
		 dcs_log(0,0,"<ScTcpip--%s> read msg from net failed:%s!",g_port,strerror(errno));
		cleanup(proce);
		return NULL;
	}
	if(nReadBytes < 5)
	{
		dcs_debug(buf,nReadBytes,"<ScTcpip--%s> read msg from net no tpdu!",g_port);
		cleanup(proce);
		return NULL;
	}
	dcs_debug(buf,nReadBytes,"<ScTcpip--%s> read msg from net:TPDU=[%d]!",g_port,localprocess.location);
	memset(temp,0x00,sizeof(temp));
	bcd_to_asc((unsigned char * )temp, (unsigned char * )buf,10, 0);
	for(i=0;i<pScLinkCtrl->i_conn_num;i++)
	{
		//找到与端口和TPDU对应的短链信息
		if(pScLinkCtrl->conns[i].listen_port ==atoi(g_port )&&(memcmp(temp,pScLinkCtrl->conns[i].tpdu,10) == 0)
			&& pScLinkCtrl->conns[i].tpduFlag == 1)
		{
			pscLink = &(pScLinkCtrl->conns[i]);
			break;
		}
		continue;
	}
	if(i>= pScLinkCtrl->i_conn_num)
	{
		//读到的信息与配置不符
		 dcs_log(0,0,"<ScTcpip--%s> tpdu[%s] the msg does not accord with conf!",g_port,temp);
		 cleanup(proce);
		 return NULL;
	}
	memset(writebuf,0x00,sizeof(writebuf));
	memcpy(writebuf,pscLink->msgtype,4);
	len +=4;
	sprintf(temp+6,"%04x",localprocess.location);//把socketId所在的location放在TPDU的后4位,业务进程会返回，用于返回给终端时找到正确的socket
	asc_to_bcd((unsigned char * )writebuf+4, (unsigned char * )temp,10, 0);
	memcpy(writebuf+9,buf+5,nReadBytes-5);
	len +=nReadBytes;

	nWrittenBytes = fold_write(pscLink->iApplFid,pscLink->iFid,writebuf,len);
	if(nWrittenBytes == -1)
	{
		 dcs_log(0,0,"<ScTcpip--%s> fold_write() failed:%s errno=%d,TPDU=[%d]",g_port,strerror(errno),errno,localprocess.location);
	     	 cleanup(proce);
		 return NULL;
	}
	
	//处理超时，关闭SOCKET
	for(i=0;i<100;i++)
	{
		sleep(1);
		iRet =SocketConnected(proce,&localprocess);
		if(iRet == -1)
		{
			//数据已经返回,服务端断开
			dcs_log(0, 0, "sockid[%d]数据已经返回,服务端断开",localprocess.sock);
			return NULL;
		}
		else if(iRet == 0)
		{
			//客户端已经断开
			dcs_log(0, 0, "sockid[%d]客户端已经断开",localprocess.sock);
			break;	
		}
	}
	if(i>=100)
		dcs_log(0, 0, "sockid[%d]连接超时,断开该连接",localprocess.sock);
	 cleanup(proce);
	return NULL;
}

static void Close_ALL_Sockets()
{
	int i = 0;
	close(listen_sock);
	for (;i < MAX_PORCESS; i ++)
	{
		if(processes[i].sock !=NO_SOCK )
	    		close(processes[i].sock);
	}
}

static void shortconnexit(int signo)
{
	int iStatus=-1;
	getScStatue(&iStatus);
	 dcs_log(0, 0, "short conn tcp[%s]catch sinal %d...\n",g_port,signo);    

	//parent process
	switch (signo)
	{
		case SIGUSR2: 
			setScStatue(DCS_STAT_STOPPED);
			dcs_log(0, 0, "ScServerCmd停服务");
			break;
		case 0:
		case 11:
			if ( iStatus != DCS_STAT_STOPPED && iStatus != DCS_STAT_DISCONNECTED)
			{
				if ( iStatus != DCS_STAT_STOPPED)
					setScStatue(DCS_STAT_DISCONNECTED);
				Close_ALL_Sockets();
				dcs_log(0, 0, "重起链路");
				ReStartProcess("shortTcp", g_port);
				dcs_debug(0,0,"exit system");
				exit(signo);
			}
			break;
		case SIGTERM:
			dcs_log(0, 0, "脚本停服务");
			break;
		default:
		      break;
	}

	//close the socket
	Close_ALL_Sockets();
	dcs_debug(0,0,"exit system");
	exit(signo);
}
int SocketConnected( process* proce,process *currProc)  
{  
	struct tcp_info info;  

	int len=sizeof(info);  
	if(currProc->location!=proce->location)
	{
		return -1;//原链接已经结束,数据已返回
	}
	memset(&info,0,sizeof(info));  
	getsockopt(currProc->sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);  
	if(info.tcpi_state== 1) {  
	    //printf("socket connected\n");  
	    return 1;  
	} else {  
	    //printf("socket disconnected\n");  
	    return 0;  
	}  
}  
int find_process_by_tpdu(char *buf)
{
	char szLocation[4+1]={0};
	int nlocation,i;
	char bcd[2+1]={0};
	memcpy(bcd,buf+1,2);
	bcd_to_asc((unsigned char * )szLocation, (unsigned char * )bcd, 4, 0);
	sscanf(szLocation,"%x",&nlocation);    
	for(i=0;i<MAX_PORCESS;i++)
	{
		if(processes[i].location == nlocation)
			return i;
	}
	return NO_SOCK;
}
//设置链路状态
int setScStatue(int iStatus)
{
	int i;

	for(i=0;i<pScLinkCtrl->i_conn_num;i++)
	{
		if(atoi(g_port) == pScLinkCtrl->conns[i].listen_port)
		{
			pScLinkCtrl->conns[i].iStatus = iStatus;
		}		
	}
  	return 0;	
}

//获取链路状态
int getScStatue(int *iStatus)
{
	int i;

	for(i=0;i<pScLinkCtrl->i_conn_num;i++)
	{
		if(atoi(g_port) == pScLinkCtrl->conns[i].listen_port)
		{
			*iStatus=pScLinkCtrl->conns[i].iStatus;
		}		
	}
  	return 0;	
}


static int ReStartProcess(const char * strExeFile, const char * arg1)
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
		
		for(i=0;i<pScLinkCtrl->i_conn_num;i++)
		{
			if(atoi(g_port) == pScLinkCtrl->conns[i].listen_port)
			{
				pScLinkCtrl->conns[i].pidComm = pidchild;
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



