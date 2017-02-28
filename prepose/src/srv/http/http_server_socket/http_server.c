 /*  
 *程序名称：http_server.c
 *功能描述：
 *支持http协议的服务器端短链服务
*/
#include <time.h>
#include <math.h>
#include "http_server.h"

static   int      s_port = 0;
static   int      s_foldid=0;
static   char   s_foldname[22]={0x00};
static   int      s_timeout = 0;
struct http_s  *s_pHttp_sCtrl = NULL;/*共享内存HTTP_SERVER中的对应端口的短链信息*/


static process processes[MAX_PORCESS];
static int listen_sock;
static int current_total_processes;
static int current_max_tpdu;

static pthread_mutex_t process_mutex;


static int Open_Http_Server_Log();
static int Accept_Connect(int sock);
static int Chect_Socket( process* proce,process *currProc) ;
static int Find_Process_By_Tpdu(char *buf);
static void Recive_Message_From_Client();
static void Clean_Process(process *process);
static void Init_Processes();  
static void Get_Http_Server_Info();
static void Http_Server_Exit(int signo);
static void *Deal_Client_Request( void* args);
static void * Send_Message_To_Client();
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
	catch_all_signals(Http_Server_Exit);

	//check the command line and get my folder's name
	if (argc < 2)
	{
		dcs_log(0,0,"Usage: %s foldName.",argv[0]);
		goto lblExit;
	}
	else 
	{
		strcpy(s_foldname, argv[1]);
	}

	if (0 > Open_Http_Server_Log(argv[1]))
		 goto lblExit;

	
	//连接folder系统
	dcs_log(0,0,"connect folder system");

	if (fold_initsys() < 0)
	{
		dcs_log(0,0," cannot attach to FOLDER !");
		goto lblExit;
	}

	//=============== 开启链路服务 ================//
	//连接共享内存Http_SERVER,获得所有的短链link信息

	Get_Http_Server_Info();

	if (s_port == 0)
	{
		dcs_log(0,0," cannot get corresponding item in link info array !");
		goto lblExit;
	}

	//建立和对方的网络链接,获得连接套接字
	dcs_log(0,0, "begin open socket.. port = %d", s_port);
	
        listen_sock = tcp_open_sc_server(NULL,s_port);
		
        if (listen_sock < 0)
        {
            dcs_log(0,0,"<httpServer--%d> tcp_open_server() failed: %s!",s_port,strerror(errno));
            return -1;
        }
        
	dcs_log(0, 0, "open socket succ ,listen_sock = %d", listen_sock);

	s_pHttp_sCtrl->iStatus = DCS_STAT_LISTENING;
	s_pHttp_sCtrl->pidComm= getpid();
	
	int res = pthread_mutex_init(&process_mutex, NULL);
	if(res != 0) 
	{
		printf("process_mutex initialzation failed...\n");
		goto lblExit;
	}
	memset(processes,0,sizeof(process)*MAX_PORCESS);
	Init_Processes();
	//开启接受folder过来的数据的线程,从folder中读应答报文，并转发到对应的网络上
	pthread_t sthread;
	if (pthread_create(&sthread, NULL,Send_Message_To_Client, NULL) !=0)
	{
		 dcs_log(0,0,"pthread_create  fail");
		 goto lblExit;
	}
	pthread_detach(sthread);
	//=========================================//

	//父进程处理从网络读数据，并发给业务处理进程
	Recive_Message_From_Client();
lblExit:
	dcs_log(0,0,"lblExit() failed:%s\n",strerror(errno));
	Http_Server_Exit(0);
	return 0;
}//main()

static void Recive_Message_From_Client()
{
	int cfd = 0;
	int nLocation=0;
	pthread_t sthread = 0;
	while (1)
	{
		dcs_log(0,0,"waiting for connect request!");

		cfd = accept(listen_sock, (struct sockaddr *)NULL, NULL);
		if (cfd == -1) 
		{
			dcs_log(0,0,"fail to accept!");
			continue;
		}
			
		nLocation = Accept_Connect(cfd);
		if(nLocation < 0)
			continue;
		
		if ( pthread_create(&sthread, NULL,Deal_Client_Request, (void *)&processes[nLocation]) !=0)
		{
			 dcs_log(0,0,"pthread_create fail ");
			close(cfd);	
		}

		pthread_detach(sthread);
	}
	close(listen_sock);
}

void Init_Processes()
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

void Clean_Process(process *process) 
{
	int ret= 0;
	pthread_mutex_lock(&process_mutex);

	if (process->sock != NO_SOCK) 
	{
		ret = close(process->sock);
		if (ret == -1) 
		{
			dcs_log(0,0,"close sock fail.error[%s] ",strerror(errno));
		}
		current_total_processes--;
	}
	process->sock = NO_SOCK;
	process->location= NO_SOCK;
	pthread_mutex_unlock(&process_mutex);
}

int  Find_Empty_Process() 
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

static int Accept_Connect(int sock) 
{
	int location = 0;
   	if (current_total_processes >= MAX_PORCESS) 
	{
		// 请求已满， accept 之后直接挂断
		dcs_log(0,0,"当前线程数已达到最大值[%d]",MAX_PORCESS);
		close(sock);
		return -1;
    	}
	pthread_mutex_lock(&process_mutex);
	location = Find_Empty_Process();
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
       return location;
}

/*接受到处理好的数据，发送到请求者*/
void * Send_Message_To_Client()
{
	int iOrgFid = 0,location = 0;
	int   nBytesRead = 0,len = 0, nBytesWritten = 0;
	char  sBuffer[PACKAGE_LENGTH+1];/*存放folder里读到的报文*/
	char sHttpBuffer[PACKAGE_LENGTH+250];/*存放添加http头的报文*/
	process * process=NULL;
	int fold_id =s_foldid;
	int fid = 0;
	fid=fold_open(s_foldid);
	while (1)
	{
		memset(sBuffer,0x00,sizeof(sBuffer));
		dcs_log(0, 0, " waiting read  folder[%s] iFid = %d",s_foldname, fold_id);	
		nBytesRead= fold_read(fold_id,fid,&iOrgFid,sBuffer,sizeof(sBuffer),TRUE);
		dcs_debug(sBuffer,nBytesRead,"Read From folder [%s] data_buf len=%d,foldId=%d", s_foldname, nBytesRead, fold_id);

		if (nBytesRead < 0)
		{
			dcs_log(0,0,"<HttpServer--%d> fold_read() failed:%s",s_port,strerror(errno));
			break;
		}
		if (nBytesRead <= 5 )
		{
			dcs_log(0,0,"<HttpServer--%d> 读到的报文内容太短",s_port);
			continue;
		}
		//根据TPDU找到原socket信息
		location =Find_Process_By_Tpdu(sBuffer);

		if(location== NO_SOCK)
		{
			dcs_log(sBuffer,nBytesRead,"socket已经关闭,不在转发");
			continue;
		}
		process =&processes[location];

		//发送报文，去除前面的5个字节TPDU
		memset(sHttpBuffer,0x00,sizeof(sHttpBuffer));
		sprintf(sHttpBuffer, "HTTP/1.1 200 OK\r\nServer: Microsoft-IIS/5.1\r\nContent-Type: text/html\r\nContent-Length:%d\r\n\r\n",nBytesRead-5);
		len = strlen(sHttpBuffer);
		
		memcpy(sHttpBuffer+len,sBuffer+5,nBytesRead-5);
		len = len+nBytesRead-5;
		
		nBytesWritten = write_msg_to_shortTcp_AMP(process->sock, sHttpBuffer, len);
		if (nBytesWritten < len)
		{
			dcs_log(0,0,"<HttpServer--%d> write_msg_to_net() failed:%s",s_port,strerror(errno));
			Clean_Process(process);
			continue;
		}
		dcs_log(0,0,"<HttpServer--%d> write_msg_to_net() succ!",s_port);

		// 读写完毕
		Clean_Process(process);
        }
	close(fid);
	return (void *)0;
}

//处理网络的请求,并发往对应的业务处理FOLDER
void *Deal_Client_Request( void* args) 
{
	char buf[PACKAGE_LENGTH+1],writebuf[PACKAGE_LENGTH+4+1];
	char temp[11];
	int nReadBytes=0,nWrittenBytes=0;
	int i,len = 0,iRet = 0;
	
	process localprocess,*proce = NULL;
	proce = (process *)args;
	memset(buf,0x00,sizeof(buf));
	memset(&localprocess,0x00,sizeof(process));
	memcpy(&localprocess,proce,sizeof(process));
	/*超时时间设置为10秒*/
	s_timeout = 10* 1000;
	nReadBytes = http_read_msg(localprocess.sock, buf, sizeof(buf),s_timeout);
	if(nReadBytes < 0)
	{
		dcs_log(0,0,"<httpServer--%d> http_read_msg() failed:%s!",s_port,strerror(errno));
		Clean_Process(proce);
		return NULL;
	}
	if(nReadBytes < 10)
	{
		dcs_debug(buf,nReadBytes,"<httpServer--%d> mess too small len =%d ",s_port,nReadBytes);
		Clean_Process(proce);
		return NULL;
	}
	dcs_debug(buf,nReadBytes,"<httpServer--%d> http_read_msg():socketid = [%d],TPDU=[%d]!",s_port,localprocess.sock,localprocess.location);
	
	memset(writebuf,0x00,sizeof(writebuf));
	memcpy(writebuf,s_pHttp_sCtrl->msgtype,4);
	len +=4;
	
	/*添加TPDU头放在报文头
	*把socketId所在的location放在TPDU的后4位
	*业务进程会返回，用于返回给终端时找到正确的socket
	*/
	memset(temp,0x00,sizeof(temp));
	sprintf(temp,"601114%04x",localprocess.location);
	asc_to_bcd((unsigned char * )writebuf+len, (unsigned char * )temp,10, 0);
	len +=5;
	memcpy(writebuf+len,buf,nReadBytes);
	len +=nReadBytes;

	/*将报文写入对应业务处理得folder*/
	nWrittenBytes = fold_write(s_pHttp_sCtrl->iApplFid,s_pHttp_sCtrl->iFid,writebuf,len);
	if(nWrittenBytes == -1)
	{
		 dcs_log(0,0,"<httpServer--%d> fold_write() failed:%s errno=%d,TPDU=[%d]",s_port,strerror(errno),errno,localprocess.location);
	     	 Clean_Process(proce);
		 return NULL;
	}
	dcs_log(0, 0, "SC Socket...write..buffer..succ TPDU=[%d]",localprocess.location);
	
	/*处理超时，关闭SOCKET */
	for(i=0;i<100;i++)
	{
		sleep(1);
		iRet =Chect_Socket(proce,&localprocess);
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
	 Clean_Process(proce);
	return NULL;
}


int Chect_Socket( process* proce,process *currProc)  
{  
	struct tcp_info info;  

	if(currProc->location!=proce->location)
	{
		return -1;/*原链接已经结束,location 数据已被覆盖,数据已返回*/
	}
	int len=sizeof(info);
	memset(&info,0,len);  
	getsockopt(currProc->sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);  
	if(info.tcpi_state== 1) {  
	    //printf("socket connected\n");  
	    return 1;  
	} else {  
	    //printf("socket disconnected\n");  
	    return 0;  
	}  
}  
int Find_Process_By_Tpdu(char *buf)
{
	char szLocation[4+1]={0};
	int nlocation = 0,i = 0;
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

// 函数名称  Get_Http_Server_Info
// 返 回 值：
//         void
// 功能描述：
//          1.连接共享内存Http_SERVER

void Get_Http_Server_Info()
{
	int i,num=0;
	//连接共享内存HSTCOM
        struct http_s_Ast *pHttpServerCtrl = (struct http_s_Ast*)shm_connect(HTTP_SERVER_SHM_NAME, NULL);
  	if (pHttpServerCtrl == NULL)
  	{
	        dcs_log(0,0,"cannot connect share memory %s!",HTTP_SERVER_SHM_NAME);
	        return ;
  	}

	for(i=0;i<pHttpServerCtrl->i_http_num;i++)
	{
		if(strcmp(s_foldname,pHttpServerCtrl->https[i].name) == 0)
		{
			memset(&s_pHttp_sCtrl, 0x00, sizeof(s_pHttp_sCtrl));
			s_foldid= pHttpServerCtrl->https[i].iFid;
			s_port = pHttpServerCtrl->https[i].port;
			s_pHttp_sCtrl = &(pHttpServerCtrl->https[i]);
			break;
		}		
	}
}


// 函数名称  Open_Http_Server_Log
// 参数说明：
//          无
// 返 回 值：
//          成功返回0,否则-1
// 功能描述：
//          1.获得系统主目录,将之放在全局变量g_pcIseHome
//          2.设置日志文件和日志方式

int Open_Http_Server_Log( char * buf)
{
	char  caLogName[256];
	char logfile[256];

	memset(caLogName,0,sizeof(caLogName));
	memset(logfile,0,sizeof(logfile));
	sprintf(caLogName,"log/httpSvr_%s.log",buf);
	if(u_fabricatefile(caLogName,logfile,sizeof(logfile)) < 0)
	{
		dcs_log(0, 0, "can not open log file:[%s]",caLogName);
		return -1;
	}
	return  dcs_log_open(logfile, buf);
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

static void Http_Server_Exit(int signo)
 {
	  dcs_log(0, 0, "http server[%d]catch sinal %d...\n",s_port,signo);    
	//parent process
	switch (signo)
	{
		case SIGUSR2: /*HttpServerCmd want me to stop */
			s_pHttp_sCtrl->iStatus = DCS_STAT_STOPPED;
			dcs_log(0,0,"httpServerCmd 停服务");
			break;
		case 0:
		case 11:
			if ( s_pHttp_sCtrl->iStatus != DCS_STAT_STOPPED )
			{
				s_pHttp_sCtrl->iStatus = DCS_STAT_STOPPED;
				dcs_log(0,0,"重起服务");
				Close_ALL_Sockets();
				Start_Process("httpServer",s_pHttp_sCtrl->name);
				dcs_log(0,0,"exit system");
				exit(signo);
			}
			break; 
		case SIGTERM:
			dcs_log(0,0,"脚本停服务");
			break;
		default:
		      break;
	}
	dcs_log(0,0,"exit system");
	Close_ALL_Sockets();
	exit(signo);
} 


