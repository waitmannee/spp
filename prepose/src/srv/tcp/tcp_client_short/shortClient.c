#include <time.h>
#include <math.h>
#include "short_conn_client.h"

static   char             g_myfoldName[20+1]={0x00};
static   int                g_myfoldid=0;
static   int                g_timeout;
static   int           g_threadNum= 0;
struct conn_c  *pConn_cCtrl = NULL;//共享内存SC_CLIENT中的对应ip和端口的短链信息

static void do_loop();
static void  * deal_socket(void * message);
static void GetScClientInfo();
static int ScClientOpenLog( char * buf);
static void shortconnClientexit(int signo);
    
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
	catch_all_signals(shortconnClientexit);

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

	if (0 > ScClientOpenLog(argv[1]))
		 goto lblExit;

	
	//连接folder系统
	dcs_log(0,0,"connect folder system");

	if (fold_initsys() < 0)
	{
		dcs_log(0,0," cannot attach to FOLDER !");
		goto lblExit;
	}

	//=============== 开启链路服务 ================//
	//连接共享内存SC_CLIENT,获得所有的短链link信息
	//dcs_log(0,0,"locate link msg");

	GetScClientInfo();

	if (strlen(pConn_cCtrl->server_ip) == 0)
	{
		dcs_log(0,0," cannot get corresponding item in link info array !");
		goto lblExit;
	}

	dcs_log(0,0," link role get succ ");

	pConn_cCtrl->iStatus = DCS_STAT_RUNNING;
	pConn_cCtrl->pidComm= getpid();
	do_loop();
	
lblExit:
	dcs_log(0,0,"lblExit() failed:%s",strerror(errno));
	shortconnClientexit(0);
	return 0;
}//main()

void do_loop()
{
	int fid,org_foldid;
	int len,timeout;
	pthread_t threadid;
	int flag =1;
	struct messBuf *mess;

	fid =fold_open(g_myfoldid);
	while (1)
	{

		//读取自己的fold
		dcs_log(0,0,"waiting for  the fold request [%s]",g_myfoldName);
		mess = (struct messBuf *)malloc(sizeof(struct messBuf));
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
		//dcs_log(0, 0, "name = [%s], fold_name = [%s], msgtype = [%s], server_ip = [%s], server_port = [%d],timeout = [%d], MaxthreadNum =[%d]\n",
			//g_myfoldName,pConn_cCtrl->fold_name,pConn_cCtrl->msgtype,pConn_cCtrl->server_ip, pConn_cCtrl->server_port, pConn_cCtrl->timeout , pConn_cCtrl->maxthreadnum);

		mess->len = len;
		dcs_debug(mess->buf, len, "read fold buf len %d", len);
		
		//处理通信超时时间
		if(pConn_cCtrl->timeout == 0)
		{
			g_timeout = -1;
		}
		else
		{
			g_timeout = pConn_cCtrl->timeout * 1000;
		}
		
		//判断是否还有可用线程进行处理
		while(1)
		{
			if(g_threadNum >= pConn_cCtrl->maxthreadnum)
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
		if(pthread_create(&threadid, NULL, deal_socket,(void *)(mess)) != 0)
		{
			dcs_log(0,0,"pthread_create fail");
			free(mess);
		 	continue;
		}
		pthread_detach(threadid);
		g_threadNum++;
	}
}



void  * deal_socket(void * message)
{
	int len,ret,sockid;
	char readBuf[PACKAGE_LENGTH+4+1];
	struct messBuf localMess;
	memcpy(&localMess , message ,sizeof(struct messBuf));
	free(message);
	message = NULL;
	//建立和对方的网络链接,获得连接套接字
	//dcs_debug(0,0,"create socket");
	sockid= tcp_connect_sc_server(pConn_cCtrl->server_ip, pConn_cCtrl->server_port);

	dcs_log(0, 0, "create sockid = %d", sockid);

	if(sockid < 0) {
		dcs_log(0, 0, "connection error....[%s]",strerror(errno));
		g_threadNum--;
		return (void *)-1;
	}

	//发送报文
	if(memcmp(pConn_cCtrl->commType,"trm",3) == 0)
	{
		ret = write_msg_to_shortTcp(sockid, localMess.buf, localMess.len);
	}
	else if(memcmp(pConn_cCtrl->commType,"bnk",3) == 0)
	{
		ret = write_msg_to_shortTcp_bnk(sockid, localMess.buf, localMess.len);
	}
	else if(memcmp(pConn_cCtrl->commType,"con",3) == 0)
	{
		ret = write_msg_to_shortTcp_CON(sockid, localMess.buf, localMess.len);
	}
	else if(memcmp(pConn_cCtrl->commType,"amp",3) == 0)
	{
		ret = write_msg_to_shortTcp_AMP(sockid, localMess.buf, localMess.len);
	}
	if(ret <localMess.len)
	{
		dcs_log(0, 0, "send message fail %s",ise_strerror(errno));
		close(sockid);
		g_threadNum--;
		return (void *)-1;
	}
	dcs_log(0,0,"write_msg_to_shortTcp succ");
	//等待接收报文
	memset(readBuf, 0x00, sizeof(readBuf));
	memcpy(readBuf, pConn_cCtrl->msgtype, 4);
	if(memcmp(pConn_cCtrl->commType,"trm",3) == 0)
	{
		len = read_msg_from_NAC(sockid, readBuf+4, sizeof(readBuf)-4,g_timeout);
	}
	else if(memcmp(pConn_cCtrl->commType,"bnk",3) == 0)
	{
		len = read_msg_from_net(sockid, readBuf+4, sizeof(readBuf)-4,g_timeout);
	}
	else if(memcmp(pConn_cCtrl->commType,"con",3) == 0)
	{
		len = read_msg_from_CON(sockid, readBuf+4, sizeof(readBuf)-4,g_timeout);
	}
	else if(memcmp(pConn_cCtrl->commType,"amp",3) == 0)
	{
		len = read_msg_from_AMP(sockid, readBuf+4, sizeof(readBuf)-4,g_timeout);
	}

	if(len < 0)
	{
		dcs_log(0, 0, "read message fail %s",ise_strerror(errno));
		close(sockid);
		g_threadNum--;
		return (void *)-1;
	}
	dcs_debug(readBuf+4,len,"read_msg_from_shortTcp succ,len=%d",len);
	close(sockid);
	//将收到的返回报文写进业务处理FOLD
	ret = fold_write(pConn_cCtrl->iApplFid, g_myfoldid, readBuf, len+4);
	if(ret <0)
	{
		dcs_log(0, 0, "write fold %s fail", pConn_cCtrl->fold_name);
	}
	dcs_log(0,0,"write fold %s succ", pConn_cCtrl->fold_name);
	g_threadNum--;
	return (void *)0;
		
}

// 函数名称  GetScClientInfo
// 返 回 值：
//          若找到成功,则返回对应信息槽位的指针,否则NULL
// 功能描述：
//          1.连接共享内存SC_CLIENT

void GetScClientInfo()
{
	int i,num=0;
	//连接共享内存HSTCOM
        struct conn_c_Ast *pScClientCtrl = (struct conn_c_Ast*)shm_connect(SHORT_CONN_CLIENT_SHM_NAME, NULL);
  	if (pScClientCtrl == NULL)
  	{
	        dcs_log(0,0,"cannot connect share memory %s!",SHORT_CONN_CLIENT_SHM_NAME);
	        return ;
  	}

	for(i=0;i<pScClientCtrl->i_conn_num;i++)
	{
		if(strcmp(g_myfoldName,pScClientCtrl->connc[i].name) == 0)
		{
			memset(&pConn_cCtrl, 0x00, sizeof(pConn_cCtrl));
			g_myfoldid= pScClientCtrl->connc[i].iFid;
			pConn_cCtrl = &(pScClientCtrl->connc[i]);
			break;
		}
			
	}
	
}


// 函数名称  ScClientOpenLog
// 参数说明：
//          无
// 返 回 值：
//          成功返回0,否则-1
// 功能描述：
//          1.获得系统主目录,将之放在全局变量g_pcIseHome
//          2.设置日志文件和日志方式

int ScClientOpenLog( char * buf)
{
	char  caLogName[256];
	char logfile[256];

	memset(caLogName,0,sizeof(caLogName));
	memset(logfile,0,sizeof(logfile));
	sprintf(caLogName,"log/ScCli_%s.log",buf);
	if(u_fabricatefile(caLogName,logfile,sizeof(logfile)) < 0)
	{
		dcs_log(0, 0, "can not open log file:[%s]",caLogName);
		return -1;
	}
	return  dcs_log_open(logfile, buf);
}


static void shortconnClientexit(int signo)
 {
	  dcs_log(0, 0, "short conn tcp[%s]catch sinal %d...\n",g_myfoldName,signo);    

	//parent process
	switch (signo)
	{
		case SIGUSR2: //ScClientCmd want me to stop
			pConn_cCtrl->iStatus = DCS_STAT_STOPPED;
			break;
		case 0:
		case 11:
			if ( pConn_cCtrl->iStatus != DCS_STAT_STOPPED )
			{
				pConn_cCtrl->iStatus = DCS_STAT_STOPPED;
				Start_Process("shortClient",pConn_cCtrl->name);//重起链路
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


