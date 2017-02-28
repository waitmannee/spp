#include <time.h>
#include <math.h>
#include "http_client.h"

static   char             g_myfoldName[20+1]={0x00};
static   int                g_myfoldid=0;
static   int                g_timeout;
static   int           g_threadNum= 0;
static   char           g_addr[60]= {0x00};
static   char           g_file[60] = {0x00};
struct http_c  *pHttp_cCtrl = NULL;//共享内存HTTP_CLIENT中的对应ip和端口的短链信息

static void httpClientexit(int signo);
static void doIoop();
static void  * dealSocket(void * message);
static void GetHttpClientInfo();
static int HttpClientOpenLog( char * buf);
void GetHost(char * src, char * web, char * file, int * port);
// 函数名称：main()
// 参数说明：
//           argv[1] 要监听的folder名
// 返 回 值：
//           无
// 功能描述：
//           1.接收来自成员行的报文,并转交给相应的应用进程
//           2.接收本地应用的报文,并将其发往网络

int main(int argc, char *argv[])
{
	catch_all_signals(httpClientexit);

	//check the command line and get my folder's name
	if (argc < 2)
	{
		dcs_log(0,0,"Usage: %s foldName.",argv[0]);
		goto lblExit;
	}
	else 
	{
		strcpy(g_myfoldName, argv[1]);
	}

	if (0 > HttpClientOpenLog(argv[1]))
		 goto lblExit;

	
	//连接folder系统
	dcs_log(0,0,"connect folder system");

	if (fold_initsys() < 0)
	{
		dcs_log(0,0," cannot attach to FOLDER !");
		goto lblExit;
	}

	//=============== 开启链路服务 ================//
	//连接共享内存Http_CLIENT,获得所有的短链link信息

	GetHttpClientInfo();

	if (strlen(pHttp_cCtrl->serverAddr) == 0)
	{
		dcs_log(0,0," cannot get corresponding item in link info array !");
		goto lblExit;
	}

	dcs_log(0,0," link role get succ ");
	
	if(pHttp_cCtrl->serverAddr[0] >='1' && pHttp_cCtrl->serverAddr[0] <='9' )//ip:port
	{
		char port[10];
		memset(port,0,sizeof(port));
		util_split_ip_with_port_address(pHttp_cCtrl->serverAddr, pHttp_cCtrl->server_ip, port);
		pHttp_cCtrl->server_port= atoi(port);
	}	
	else
	{
		GetHost(pHttp_cCtrl->serverAddr, pHttp_cCtrl->server_ip,g_file,&(pHttp_cCtrl->server_port));
	}

	dcs_log(0,0,"addr:%s,ip:%s,port:%d ,file:%s",pHttp_cCtrl->serverAddr, pHttp_cCtrl->server_ip, pHttp_cCtrl->server_port,g_file);

	pHttp_cCtrl->iStatus = DCS_STAT_RUNNING;
	pHttp_cCtrl->pidComm= getpid();
	doIoop();
	
	
lblExit:
	dcs_log(0,0,"lblExit() failed:%s",strerror(errno));
	httpClientexit(0);
	return 0;
}//main()

void doIoop()
{
	int fid,org_foldid;
	int len,timeout;
	pthread_t threadid;
	int flag =1;
	struct httpMessBuf *mess;

	fid =fold_open(g_myfoldid);
	while (1)
	{

		//读取自己的fold
		dcs_log(0,0,"waiting for  the fold request [%s]",g_myfoldName);
		mess = (struct httpMessBuf *)malloc(sizeof(struct httpMessBuf));
		if(mess == NULL)
		{
			dcs_log(0,0,"malloc failed:%s",ise_strerror(errno));
			break;
		}
		len = fold_read(g_myfoldid, fid, &org_foldid, mess->buf, sizeof(mess->buf), 1);
		if(len < 0)
		{
			dcs_log(0,0,"fold_read() failed:%s",ise_strerror(errno));
			free(mess);
			break;
		}

		if(len == 0)
			continue;
		
		mess->len = len;
		dcs_debug(mess->buf, len, "read fold buf len %d", len);
		
		//处理通信超时时间
		if(pHttp_cCtrl->timeout == 0)
		{
			g_timeout = -1;
		}
		else
		{
			g_timeout = pHttp_cCtrl->timeout * 1000;
		}
		
		//判断是否还有可用线程进行处理
		while(1)
		{
			if(g_threadNum >= pHttp_cCtrl->maxthreadnum)
			{
				if(flag ==1)
				{
					dcs_log(0,0,"已经达到最大进程数,请等待");
					flag =0;
				}
				u_sleep_us(100000); //sleep 100ms 
				continue;
			}
			else
			{
				flag =1;
				break;
			}
		}

		//开启线程处理该笔报文
		if(pthread_create(&threadid, NULL, dealSocket,(void *)(mess)) != 0)
		{
			dcs_log(0,0,"pthread_create  fail");
			free(mess);
		 	continue;
		}
		pthread_detach(threadid);
		g_threadNum++;
	}
}

void  * dealSocket(void * message)
{
	int len=0,ret,sockid;
	char buf[PACKAGE_LENGTH+360];
	struct httpMessBuf localMess;
	memcpy(&localMess , message ,sizeof(struct httpMessBuf));
	free(message);
	message = NULL;
	
	//建立和对方的网络链接,获得连接套接字
	sockid= tcp_connect_sc_server(pHttp_cCtrl->server_ip, pHttp_cCtrl->server_port);

	dcs_log(0, 0, "create sockid = %d", sockid);

	if(sockid < 0) {
		dcs_log(0, 0, "connection error....[%s]",strerror(errno));
		g_threadNum--;
		return (void *)-1;
	}

	//发送报文
	sprintf(buf, "POST /%s HTTP/1.1\r\nAccept: */*\r\nUser-Agent: Mozilla/4.0\r\nContent-Length:%d\r\nConnection:close\r\nAccept-Language:zh-cn\r\nAccept-Encoding: gzip, deflate\r\nHost:%s\r\nContent-Type: text/html\r\n\r\n", g_file, localMess.len,pHttp_cCtrl->server_ip);
	len = strlen(buf);
	memcpy(buf+len,localMess.buf,localMess.len);
	len = len+localMess.len;
	
	ret = write_msg_to_shortTcp_AMP(sockid, buf, len);
	if(ret <localMess.len)
	{
		dcs_log(0, 0, "send message fail errno=%d,%s",errno,ise_strerror(errno));
		close(sockid);
		g_threadNum--;
		return (void *)-1;
	}
	dcs_log(0,0,"write_msg_to_shortTcp succ");
	//等待接收报文
	memset(buf, 0x00, sizeof(buf));
	memcpy(buf, pHttp_cCtrl->msgtype, 4);
	len = http_read_msg(sockid, buf+4, sizeof(buf) -4,g_timeout);
	if(len < 0)
	{
		close(sockid);
		g_threadNum--;
		dcs_log(0, 0, "recv  message from server  fail errno=%d,%s",errno,ise_strerror(errno));
		return (void *)-1;
	}
	dcs_debug(buf+4,len,"http_read_msg succ,len=%d",len);
	close(sockid);
	//将收到的返回报文写进业务处理FOLD
	ret = fold_write(pHttp_cCtrl->iApplFid, g_myfoldid, buf, len+4);
	if(ret <0)
	{
		dcs_log(0, 0, "write fold %s fail", pHttp_cCtrl->fold_name);
	}
	dcs_log(0,0,"write fold %s succ", pHttp_cCtrl->fold_name);
	g_threadNum--;
	return (void *)0;
		
}

// 函数名称  GetHttpClientInfo
// 返 回 值：
//          若找到成功,则返回对应信息槽位的指针,否则NULL
// 功能描述：
//          1.连接共享内存Http_CLIENT

void GetHttpClientInfo()
{
	int i,num=0;
	//连接共享内存HSTCOM
        struct http_c_Ast *pHttpClientCtrl = (struct http_c_Ast*)shm_connect(HTTP_CLIENT_SHM_NAME, NULL);
  	if (pHttpClientCtrl == NULL)
  	{
	        dcs_log(0,0,"cannot connect share memory %s!",HTTP_CLIENT_SHM_NAME);
	        return ;
  	}

	for(i=0;i<pHttpClientCtrl->i_http_num;i++)
	{
		if(strcmp(g_myfoldName,pHttpClientCtrl->httpc[i].name) == 0)
		{
			memset(&pHttp_cCtrl, 0x00, sizeof(pHttp_cCtrl));
			g_myfoldid= pHttpClientCtrl->httpc[i].iFid;
			pHttp_cCtrl = &(pHttpClientCtrl->httpc[i]);
			break;
		}
			
	}
	
}


// 函数名称  HttpClientOpenLog
// 参数说明：
//          无
// 返 回 值：
//          成功返回0,否则-1
// 功能描述：
//          1.获得系统主目录,将之放在全局变量g_pcIseHome
//          2.设置日志文件和日志方式

int HttpClientOpenLog( char * buf)
{
	char  caLogName[256];
	char logfile[256];

	memset(caLogName,0,sizeof(caLogName));
	memset(logfile,0,sizeof(logfile));
	sprintf(caLogName,"log/httpCli_%s.log",buf);
	if(u_fabricatefile(caLogName,logfile,sizeof(logfile)) < 0)
	{
		dcs_log(0, 0, "can not open log file:[%s]",caLogName);
		return -1;
	}
	return  dcs_log_open(logfile, buf);
}


static void httpClientexit(int signo)
 {
	  dcs_log(0, 0, "http client[%s]catch sinal %d...\n",g_myfoldName,signo);    

	//parent process
	switch (signo)
	{
		case SIGUSR2: //HttpClientCmd want me to stop
			pHttp_cCtrl->iStatus = DCS_STAT_STOPPED;
			break;
		case 0:
		case 11:
			if ( pHttp_cCtrl->iStatus != DCS_STAT_STOPPED )
			{
				pHttp_cCtrl->iStatus = DCS_STAT_STOPPED;
				Start_Process("httpClient",pHttp_cCtrl->name);//重起服务
			}
			break;
		case SIGTERM://脚本停服务
			break;
		default:
		      break;
	}
	dcs_debug(0,0,"exit system");
	exit(signo);
} 

