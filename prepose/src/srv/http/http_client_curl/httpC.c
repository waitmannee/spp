#include <time.h>
#include <math.h>
#include "httpC.h"

static char g_myfoldName[20+1]={0x00};
static int g_myfoldid = 0;
static int g_timeout;
static int g_threadNum = 0;
struct httpc *pHttp_cCtrl = NULL; //共享内存HTTPCLIENT中的对应ip和端口的短链信息

static void httpCexit(int signo);
static void DoLoop();
static void *DealSocket(void *message);
static void GetHttpCInfo();
static int HttpCOpenLog(char *buf);
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
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
	catch_all_signals(httpCexit);

	//check the command line and get my folder's name
	if(argc < 2)
	{
		dcs_log(0, 0, "Usage: %s foldName.", argv[0]);
		goto lblExit;
	}
	else 
	{
		strcpy(g_myfoldName, argv[1]);
	}

	if(0 > HttpCOpenLog(argv[1]))
		goto lblExit;

	
	//连接folder系统
	dcs_log(0, 0, "connect folder system");

	if(fold_initsys() < 0)
	{
		dcs_log(0, 0, "cannot attach to FOLDER !");
		goto lblExit;
	}

	//=============== 开启链路服务 ================//
	//连接共享内存HttpCLIENT,获得所有的短链link信息

	GetHttpCInfo();

	if(strlen(pHttp_cCtrl->serverAddr) == 0)
	{
		dcs_log(0, 0, "cannot get corresponding item in link info array !");
		goto lblExit;
	}

	dcs_log(0, 0, "link role get succ ");
	
	pHttp_cCtrl->iStatus = DCS_STAT_RUNNING;
	pHttp_cCtrl->pidComm = getpid();
	curl_global_init(CURL_GLOBAL_ALL); //初始化libcurl
	DoLoop();
	
lblExit:
	dcs_log(0, 0, "lblExit()failed:%s", strerror(errno));
	curl_global_cleanup();//在结束libcurl使用的时候，用来对curl_global_init做的工作清理。
	httpCexit(0);
	return 0;
}//main()

void DoLoop()
{
	int fid = 0, org_foldid = 0;
	int len = 0;
	pthread_t threadid;
	int flag =1;
	struct httpMesBuf *mess;

	fid =fold_open(g_myfoldid);
	while (1)
	{
		//读取自己的fold
		dcs_log(0, 0, "waiting for  the fold request [%s]", g_myfoldName);
		mess = (struct httpMesBuf *)malloc(sizeof(struct httpMesBuf));
		if(mess == NULL)
		{
			dcs_log(0, 0, "malloc failed:%s", ise_strerror(errno));
			break;
		}
		memset(mess->buf, 0, sizeof(mess->buf));
		len = fold_read(g_myfoldid, fid, &org_foldid, mess->buf, sizeof(mess->buf), 1);
		if(len < 0)
		{
			dcs_log(0, 0, "fold_read() failed:%s", ise_strerror(errno));
			free(mess);
			break;
		}

		if(len == 0)
		{
			free(mess);
			continue;
		}
		
		mess->len = len;
		dcs_debug(mess->buf, len, "read fold buf len %d", len);
		
		//处理通信超时时间
		if(pHttp_cCtrl->timeout == 0)
		{
			g_timeout = -1;
		}
		else
		{
			g_timeout = pHttp_cCtrl->timeout;
		}
		
		//判断是否还有可用线程进行处理
		while(1)
		{
			if(g_threadNum >= pHttp_cCtrl->maxthreadnum)
			{
				if(flag ==1)
				{
					dcs_log(0, 0, "已经达到最大进程数, 请等待");
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
		if(pthread_create(&threadid, NULL, DealSocket, (void *)(mess)) != 0)
		{
			dcs_log(0, 0, "pthread_create  fail");
			free(mess);
		 	continue;
		}
		pthread_detach(threadid);
		g_threadNum++;
	}
}

void  *DealSocket(void *message)
{
	CURL *curl; 
	CURLcode ret; 
	int rtn = 0;
	char recvBuf[2048];
	struct httpMesBuf localMess;
	char buf[2048+5];
	
	memset(buf, 0, sizeof(buf));
	memset(recvBuf, 0, sizeof(recvBuf));
	memset(&localMess, 0, sizeof(struct httpMesBuf));
	memcpy(&localMess, message, sizeof(struct httpMesBuf));
	free(message);
	message = NULL;
	curl = curl_easy_init(); 
	if(!curl) 
	{ 
		dcs_log(0, 0, "couldn't init curl");
		g_threadNum--;
		return (void *)-1;		
	} 

	curl_easy_setopt(curl, CURLOPT_URL, pHttp_cCtrl->serverAddr);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, localMess.buf);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, localMess.len);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10 );//连接超时，这个数值如果设置太短可能导致数据请求不到就断开了
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_timeout );//接收数据时超时设置，如果g_timeout秒内数据未接收完，直接退出
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)recvBuf);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); 
	
	ret = curl_easy_perform(curl); 
	if(ret != 0)
	{
		dcs_log(0, 0, "ret = %d,error[%s]", ret, curl_easy_strerror(ret));
		g_threadNum--;
		curl_easy_cleanup(curl);
		return (void *)-1;
	}
	curl_easy_cleanup(curl);
	dcs_log(0, 0, "recv from server \n%s", recvBuf);
	
	memcpy(buf, pHttp_cCtrl->msgtype, 4);
	memcpy(buf+4, recvBuf, strlen(recvBuf));
	//将收到的返回报文写进业务处理FOLD
	rtn = fold_write(pHttp_cCtrl->iApplFid, g_myfoldid, buf, strlen(buf));
	if(rtn <0)
	{
		dcs_log(0, 0, "write fold %s fail", pHttp_cCtrl->fold_name);
		g_threadNum--;
		return (void *)-1;
	}
	dcs_log(0, 0, "write fold %s succ", pHttp_cCtrl->fold_name);
	g_threadNum--;
	return (void *)0;	
}

// 函数名称  GetHttpCInfo
// 返 回 值：
//          若找到成功,则返回对应信息槽位的指针,否则NULL
// 功能描述：
//          1.连接共享内存HTTPCLIENT

void GetHttpCInfo()
{
	int i = 0;
	//连接共享内存HTTPCLIENT
    struct httpc_Ast *pHttpClientCtrl = (struct httpc_Ast*)shm_connect(HTTPCLIENT_SHMNAME, NULL);
  	if(pHttpClientCtrl == NULL)
  	{
	        dcs_log(0, 0, "cannot connect share memory %s!", HTTPCLIENT_SHMNAME);
	        return ;
  	}

	for(; i<pHttpClientCtrl->i_http_num; i++)
	{
		if(strcmp(g_myfoldName, pHttpClientCtrl->httpc[i].name) == 0)
		{
			memset(&pHttp_cCtrl, 0x00, sizeof(pHttp_cCtrl));
			g_myfoldid = pHttpClientCtrl->httpc[i].iFid;
			pHttp_cCtrl = &(pHttpClientCtrl->httpc[i]);
			break;
		}	
	}
}

// 函数名称  HttpCOpenLog
// 参数说明：
//          无
// 返 回 值：
//          成功返回0, 否则-1
// 功能描述：
//         设置日志文件和日志方式

int HttpCOpenLog(char *buf)
{
	char caLogName[256];
	char logfile[256];

	memset(caLogName, 0, sizeof(caLogName));
	memset(logfile, 0, sizeof(logfile));
	
	sprintf(caLogName, "log/lhttpC_%s.log", buf);
	if(u_fabricatefile(caLogName, logfile, sizeof(logfile)) < 0)
	{
		dcs_log(0, 0, "can not open log file:[%s]", caLogName);
		return -1;
	}
	return  dcs_log_open(logfile, buf);
}


static void httpCexit(int signo)
 {
	dcs_log(0, 0, "lhttpC[%s]catch sinal %d...\n", g_myfoldName, signo);    
	//parent process
	switch (signo)
	{
		case SIGUSR2: //HttpClientCmd want me to stop
			pHttp_cCtrl->iStatus = DCS_STAT_STOPPED;
			break;
		case 0:
		case 11:
			if(pHttp_cCtrl->iStatus != DCS_STAT_STOPPED )
			{
				pHttp_cCtrl->iStatus = DCS_STAT_STOPPED;
				Start_Process("lhttpC", pHttp_cCtrl->name);//重启服务
			}
			break;
		case SIGTERM://脚本停服务
			break;
		default:
		    break;
	}
	dcs_log(0, 0, "exit system");
	exit(signo);
} 

 //回调函数
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) 
{ 
	int segsize = size * nmemb; 
	if(userp == NULL)
		return 0;
	memcpy(userp, buffer, (size_t)segsize);
	return segsize; 
} 
