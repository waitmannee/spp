 /*  
 *程序名称：http_server.c
 *功能描述：
 *支持http协议的服务器端短链服务
*/
#include <time.h>
#include <math.h>
#include "http_server_httpd.h"

static   int      s_port = 0;
static   int      s_foldid=0;
static   char   s_foldname[22]={0x00};
static   int      s_timeout = 0;
struct http_s  *s_pHttp_sCtrl = NULL;/*共享内存HTTP_SERVER中的对应端口的短链信息*/

static   int     s_max_process = 1024;

static httpd_process *processes =NULL;
static int listen_sock;
static int current_total_processes;
static int current_max_tpdu;
static httpd	*server;
static pthread_mutex_t process_mutex;


static int Open_Http_Server_Log();
static int Accept_Connect(httpReq	*request);
static int Check_Socket( httpd_process* proce,httpd_process *currProc) ;
static int Find_Process_By_Tpdu(char *buf);
static void Recive_Message_From_Client();
static void Clean_Process(httpd_process *process);
static void Init_Processes();  
static int Get_Http_Server_Info();
static void Http_Server_Exit(int signo);
static void *Deal_Client_Request( void* args);
static void * Send_Message_To_Client();
static FILE  * getFileFp(char *buf);
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

	FILE * fp = getFileFp(argv[1]);
	
	//连接folder系统
	dcs_log(0,0,"connect folder system");

	if (fold_initsys() < 0)
	{
		dcs_log(0,0," cannot attach to FOLDER !");
		goto lblExit;
	}

	//=============== 开启链路服务 ================//
	//连接共享内存Http_SERVER,获得所有的短链link信息

	if(Get_Http_Server_Info()<0)
	{
		goto lblExit;
	}

	if (s_port == 0)
	{
		dcs_log(0,0," cannot get corresponding item in link info array !");
		goto lblExit;
	}

	//建立和对方的网络链接,获得连接套接字
	dcs_log(0,0, "begin open socket.. port = %d", s_port);
	
	server = httpdCreate(NULL,s_port);
	if (server == NULL)
	{
		 dcs_log(0,0,"<httpServerHttpd--%d> tcp_open_server() failed: %s!",s_port,strerror(errno));
		 return -1;
	}
	httpdSetAccessLog(server, fp);
	httpdSetErrorLog(server, fp);
	if (listen_sock < 0)
	{
		 dcs_log(0,0,"httpServerHttpd--%d> tcp_open_server() failed: %s!",s_port,strerror(errno));
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
	memset(processes,0,sizeof(httpd_process)*s_max_process);
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
	int cfd = 0,result;
	int nLocation=0;
	pthread_t sthread = 0;
	httpReq	*request;
	while (1)
	{
		dcs_log(0,0,"waiting for connect request!");

		request = httpdReadRequest(server, NULL, &result);
		if (request == NULL && result == 0)
		{
			dcs_log(0,0,"Timeout ... ");
			continue;
		}
		if (result < 0)
		{
			dcs_log(0,0,"Error ... ");
			continue;
		}
		nLocation = Accept_Connect(request);
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
	for (;i < s_max_process; i ++)
	{
	    processes[i].request = NULL;
	    processes[i].location= NO_SOCK;
	}
}

void Clean_Process(httpd_process *process)
{
	pthread_mutex_lock(&process_mutex);

	if (process->request != NULL)
	{
		httpdEndRequest(server, process->request);
		current_total_processes--;
	}
	process->request = NULL;
	process->location= NO_SOCK;
	pthread_mutex_unlock(&process_mutex);
}

int  Find_Empty_Process() 
{
	int i;
	if(current_max_tpdu == MAX_TPDU)
		current_max_tpdu =0;
	 
    	for (i = 0; i < s_max_process; i++)
	{
        	if (processes[i].request == NULL)
		{
            		return i;
        	}
    	}
  	return -1;
}

static int Accept_Connect(httpReq	*request)
{
	int location = 0;
   	if (current_total_processes >= s_max_process)
	{
		// 请求已满， accept 之后直接挂断
		dcs_log(0,0,"当前线程数已达到最大值[%d]",s_max_process);
		httpdEndRequest(server, request);
		return -1;
    }
	pthread_mutex_lock(&process_mutex);
	location = Find_Empty_Process();
	if(location <0)
	{
		httpdEndRequest(server, request);
		return -1;
	}
    processes[location].request = request;
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
	int   nBytesRead = 0,len = 0;
	char  sBuffer[PACKAGE_LENGTH+1];/*存放folder里读到的报文*/
	httpd_process * process=NULL;
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
			dcs_log(0,0,"<httpServerHttpd--%d> fold_read() failed:%s",s_port,strerror(errno));
			break;
		}
		if (nBytesRead <= 5 )
		{
			dcs_log(0,0,"<httpServerHttpd--%d> 读到的报文内容太短",s_port);
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
		httpdPrintf(server, process->request,sBuffer+5);
		dcs_log(0,0,"<httpServerHttpd--%d> write_msg_to_net() succ!",s_port);

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
	
	httpd_process localprocess,*proce = NULL;
	proce = (httpd_process *)args;
	memset(buf,0x00,sizeof(buf));
	memset(&localprocess,0x00,sizeof(httpd_process));
	memcpy(&localprocess,proce,sizeof(httpd_process));

	getContent(proce->request,buf,sizeof(buf)-1);
	nReadBytes = strlen(buf);
	dcs_log(0,0,"<httpServerHttpd--%d>sock:%d,TPDU=[%d],read_content:\n%s",s_port,localprocess.request->clientSock,localprocess.location,buf);
	
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
		 dcs_log(0,0,"<httpServerHttpd--%d> fold_write() failed:%s errno=%d,TPDU=[%d]",s_port,strerror(errno),errno,localprocess.location);
	     	 Clean_Process(proce);
		 return NULL;
	}
	dcs_log(0, 0, "SC Socket...write..buffer..succ TPDU=[%d]",localprocess.location);
	
	/*处理超时，关闭SOCKET */
	for(i=0;i<s_timeout;i++)
	{
		sleep(1);
		iRet =Check_Socket(proce,&localprocess);
		if(iRet == -1)
		{
			//数据已经返回,服务端断开
			dcs_log(0, 0, "sockid[%d]数据已经返回,服务端断开",localprocess.request->clientSock);
			return NULL;
		}
		else if(iRet == 0)
		{
			//客户端已经断开
			dcs_log(0, 0, "sockid[%d]客户端已经断开",localprocess.request->clientSock);
			break;	
		}
	}
	if(i>=100)
	 	dcs_log(0, 0, "sockid[%d]连接超时,断开该连接",localprocess.request->clientSock);
	 Clean_Process(proce);
	return NULL;
}


int Check_Socket( httpd_process* proce,httpd_process *currProc)
{  
	struct tcp_info info;  

	if(currProc->location!=proce->location)
	{
		return -1;/*原链接已经结束,location 数据已被覆盖,数据已返回*/
	}
	int len=sizeof(info);
	memset(&info,0,len);  
	getsockopt(currProc->request->clientSock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
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
	for(i=0;i<s_max_process;i++)
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

int Get_Http_Server_Info()
{
	int i,num=0;
	//连接共享内存HSTCOM
        struct http_s_Ast *pHttpServerCtrl = (struct http_s_Ast*)shm_connect(HTTP_SERVER_HTTPD_SHM_NAME, NULL);
  	if (pHttpServerCtrl == NULL)
  	{
	        dcs_log(0,0,"cannot connect share memory %s!",HTTP_SERVER_HTTPD_SHM_NAME);
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
			s_timeout = pHttpServerCtrl->https[i].timeout;
			s_max_process = pHttpServerCtrl->https[i].maxthreadnum;
			processes= (httpd_process *)malloc(sizeof(httpd_process) * s_max_process);
			if(processes == NULL)
			{
				dcs_log(0,0,"malloc 失败");
				return -1;
			}

			return 0;
		}		
	}
	dcs_log(0,0,"没找到对应的配置信息");
	return -1;
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
	sprintf(caLogName,"log/httpSvrHttpd_%s.log",buf);
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
	for (;i < s_max_process; i ++)
	{
		if(processes[i].request != NULL )
			httpdEndRequest(server, processes[i].request);;
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
				Start_Process("httpServerHttpd",s_pHttp_sCtrl->name);
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

FILE  * getFileFp(char *buf)
{
	char  caLogName[256];
	char logfile[256];

	memset(caLogName,0,sizeof(caLogName));
	memset(logfile,0,sizeof(logfile));
	sprintf(caLogName,"log/httpSvrHttpd_%s.log",buf);
	if(u_fabricatefile(caLogName,logfile,sizeof(logfile)) < 0)
	{
		dcs_log(0, 0, "can not open log file:[%s]",caLogName);
		return NULL;
	}
	return fopen(logfile,"a+");
}

/* 获取http请求报文体，从request结构体中 将请求内容取出重新组成key-value的格式
 * */
int getContent(httpReq *request,char *buf,int len)
{
	char buffer[2048];
	httpVar *curVar=NULL;
	memset(buffer, 0, sizeof(buffer));
	if(request == NULL || request->variables == NULL)
		return 0;
	curVar = request->variables;
	while(curVar)
	{
		sprintf(buffer+strlen(buffer),"%s=%s&", curVar->name, curVar->value);
		curVar = curVar->nextVariable;
	}

	if(buffer[strlen(buffer)-1] == '&')
		buffer[strlen(buffer)-1] ='\0';

	if(len >= strlen(buffer))
		strcpy(buf,buffer);
	else
		memcpy(buf,buffer,len);
	return 0;
}
