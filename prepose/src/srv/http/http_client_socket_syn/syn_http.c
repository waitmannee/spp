 /*  
 *程序名称：syn_http.c
 *功能描述：
 *http 客户端同步通讯服务
*/
#include   "syn_http.h"
static   char             s_queue_recv[20+1]={0x00};
static   int                s_timeout = 0;
static   int           s_threadNum= 0;
static   int            s_recv_queue_id =  -1; /*接收队列*/
static   int             s_send_queue_id = -1;/*发送队列*/
static   char           g_addr[60]= {0x00};
static   char           g_file[60] = {0x00};

//共享内存SYN_HTTP中的对应ip和端口的信息
struct syn_http_struct *s_pHttp_Ctrl = NULL;

static void Do_Http_Service();
static void  * Deal_Http_Socket(void * message);
static void Get_Syn_Http_Info();
static int Open_Syn_Http_Log( char * buf);
static void Syn_Http_Exit(int signo);
    
/* 函数名称：main()
* 参数说明：
*           argv[1] 要监听的接收队列名
* 返 回 值：
*           无
* 功能描述：
*         1.通过读取接收队列来接收本地应用的报文,将其发往网络，
*		并接受返回报文将其写入返回队列
*/
int main(int argc, char *argv[])
{
	catch_all_signals(Syn_Http_Exit);
	
	if (argc < 2)
	{
		dcs_log(0,0,"Usage: %s recv_queue.",argv[0]);
		goto lblExit;
	}
	else 
	{
		strcpy(s_queue_recv, argv[1]);
	}

	if (0 > Open_Syn_Http_Log(argv[1]))
		 goto lblExit;

	//连接共享内存SYN_HTTP,获得所有的link信息
	Get_Syn_Http_Info();

	if (strlen(s_pHttp_Ctrl->serverAddr) == 0)
	{
		dcs_log(0,0," cannot get corresponding item in link info array !");
		goto lblExit;
	}

	dcs_log(0,0,"link role get succ ");
	//解析服务器地址
	if(s_pHttp_Ctrl->serverAddr[0] !='h' && s_pHttp_Ctrl->serverAddr[0] !='H' )//ip:port
	{
		char port[10];
		memset(port,0,sizeof(port));
		util_split_ip_with_port_address(s_pHttp_Ctrl->serverAddr, s_pHttp_Ctrl->server_ip, port);
		s_pHttp_Ctrl->server_port= atoi(port);
	}	
	else
	{
		GetHost(s_pHttp_Ctrl->serverAddr, s_pHttp_Ctrl->server_ip,g_file,&(s_pHttp_Ctrl->server_port));
	}

	dcs_log(0,0,"addr:%s,ip:%s,port:%d ,file:%s",s_pHttp_Ctrl->serverAddr, s_pHttp_Ctrl->server_ip, s_pHttp_Ctrl->server_port,g_file);

	dcs_log(0,0, "starting up  recvqueue=[%s],sendqueue[%s]....",s_queue_recv,s_pHttp_Ctrl->send_queue);
	
	//创建接收队列
	s_recv_queue_id = queue_create(s_queue_recv);
	if(s_recv_queue_id < 0)
	{
		dcs_log(0,0,"queue [%s] 已存在,直接连接",s_queue_recv);
		s_recv_queue_id = queue_connect(s_queue_recv);
		if(s_recv_queue_id < 0)
		{
			dcs_log(0,0,"connect queue [%s] err",s_queue_recv);
			return ;
		}
	}

	//创建发送队列
	s_send_queue_id= queue_create(s_pHttp_Ctrl->send_queue);
	if(s_send_queue_id < 0)
	{
		dcs_log(0,0,"queue [%s] 已存在,直接连接",s_pHttp_Ctrl->send_queue);
		s_send_queue_id = queue_connect(s_pHttp_Ctrl->send_queue);
		if(s_send_queue_id < 0)
		{
			dcs_log(0,0,"connect queue [%s] err",s_pHttp_Ctrl->send_queue);
			return ;
		}
	}

	//起一个线程检测发送队列中是否有无主信息,并将其移除
	pthread_t  thread_id =0;
	if(pthread_create(&thread_id, NULL, Check_Queue,(void *)&s_send_queue_id) <0)
	{
		dcs_log(0, 0, "pthread_create fail");
		goto lblExit;
	}
	pthread_detach(thread_id);

	s_pHttp_Ctrl->iStatus = DCS_STAT_RUNNING;
	s_pHttp_Ctrl->pidComm= getpid();

	
	//处理具体的服务
	Do_Http_Service();
	
lblExit:
	dcs_log(0,0,"lblExit() failed:%s",strerror(errno));
	Syn_Http_Exit(0);
	return 0;
}//main()
/*
* 处理服务:不断读取接收队列，将报文转发给网络，接收网络返回再写回发送队列
*/
void Do_Http_Service()
{
	pthread_t threadid =0;
	int flag =1;
	int len = 0;
	
	RECV_MESS_T *recv_buf;
				
	while (1)
	{
		recv_buf = (RECV_MESS_T *)malloc(sizeof(RECV_MESS_T));
		if(recv_buf == NULL)
		{
			dcs_log(0,0,"malloc failed:%s",ise_strerror(errno));
			break;
		}
		//阻塞读取接收队列第一条信息
		memset(recv_buf->mtext, 0 , sizeof(recv_buf->mtext));
		recv_buf->mestype = 0;
		dcs_log(0,0,"waiting for read the queue [%s],qid [%d]",s_queue_recv,s_recv_queue_id);
		len = queue_recv_by_msgtype(s_recv_queue_id,recv_buf->mtext,sizeof(recv_buf->mtext),&(recv_buf->mestype),0);
		if(len < 0)
		{
			dcs_log(0,0,"queue_recv_by_msgtype() failed:%s",ise_strerror(errno));
			free(recv_buf);
			break;
		}
		
		recv_buf->len = len;
		
		dcs_debug(recv_buf->mtext, len, "read message from queue, len = %d,msgtype = %ld", len, recv_buf->mestype);
		
		//处理通信超时时间
		if(s_pHttp_Ctrl->timeout == 0)
		{
			s_timeout = -1;
		}
		else
		{
			s_timeout = s_pHttp_Ctrl->timeout * 1000;
		}
		
		//判断是否还有可用线程进行处理
		while(1)
		{
			if(s_threadNum >= s_pHttp_Ctrl->maxthreadnum)
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
		if(pthread_create(&threadid, NULL, Deal_Http_Socket,(void *)(recv_buf)) != 0)
		{
			dcs_log(0,0,"pthread_create  fail");
			free(recv_buf);
		 	continue;
		}
		pthread_detach(threadid);
		s_threadNum++;
	}
}

void  * Deal_Http_Socket(void * message)
{
	int len = 0,sockid = 0,ret = 0;
	long msgtype = 0;
	char buf[PACKAGE_LENGTH+450];

	RECV_MESS_T mess;
	memcpy(&mess , message ,sizeof(RECV_MESS_T));
	free(message);
	message = NULL;
	//建立和对方的网络链接,获得连接套接字
	sockid= tcp_connect_sc_server(s_pHttp_Ctrl->server_ip, s_pHttp_Ctrl->server_port);

	dcs_log(0, 0, "create sockid = %d", sockid);

	if(sockid < 0) {
		dcs_log(0, 0, "连接服务器失败...[%s]",strerror(errno));
		queue_send_by_msgtype(s_send_queue_id, "FF", 2, mess.mestype, 0);
		s_threadNum--;
		return (void *)-1;
	}	

	//发送报文
	memset(buf, 0x00, sizeof(buf));
	sprintf(buf, "POST /%s HTTP/1.1\r\nAccept: */*\r\nUser-Agent: Mozilla/4.0\r\nContent-Length:%d\r\nConnection:close\r\nAccept-Language:zh-cn\r\nAccept-Encoding: gzip, deflate\r\nHost:%s\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n", g_file, mess.len,s_pHttp_Ctrl->server_ip);
	len = strlen(buf);
	memcpy(buf+len, mess.mtext, mess.len);
	len = len+mess.len;
	
	ret = write_msg_to_shortTcp_AMP(sockid, buf, len);
	if(ret < mess.len)
	{
		dcs_log(0, 0, "发送数据失败...[%s]",strerror(errno));
		queue_send_by_msgtype(s_send_queue_id, "FF", 2, mess.mestype, 0);
		close(sockid);
		s_threadNum--;
		return (void *)-1;
	}
	
	dcs_log(0,0,"write msg to net succ");
	//等待接收报文先默认成功
	memset(buf, 0x00, sizeof(buf));
	memcpy(buf, "00", 2);
	len = 0;
	len = http_read_msg(sockid, buf+2, sizeof(buf) -2,s_timeout);
	if(len < 0)
	{
		dcs_log(0, 0, "接受返回数据失败...[%s]",strerror(errno));
		memset(buf, 0x00, sizeof(buf));
		memcpy(buf, "FF", 2);
		len =2;
	}
	else
	{
		dcs_debug(buf+2,len,"recv mess from net succ,len=%d",len);
		len+=2;
	}
	
	close(sockid);

	//将收到的返回报文阻塞写进返回队列
	ret = queue_send_by_msgtype(s_send_queue_id, buf, len, mess.mestype, 0);
	if(ret <0)
	{
		dcs_log(0, 0, "write queue %s fail", s_pHttp_Ctrl->send_queue);
	}
	dcs_log(0,0,"write queue %s succ,msgtype = %ld,qid=%d", s_pHttp_Ctrl->send_queue, mess.mestype,s_send_queue_id);
	s_threadNum--;
	return (void *)0;
		
}

// 函数名称  Get_Syn_Http_Info
// 返 回 值：
//         无
// 功能描述：
//          1.连接共享内存SYN_HTTP

void Get_Syn_Http_Info()
{
	int i;
	//连接共享内存HSTCOM
        struct http_Ast *pCtrl = (struct http_Ast*)shm_connect(SYN_HTTP_SHM_NAME, NULL);
  	if (pCtrl == NULL)
  	{
	        dcs_log(0,0,"cannot connect share memory %s!",SYN_HTTP_SHM_NAME);
	        return ;
  	}

	for(i=0;i<pCtrl->i_conn_num;i++)
	{
		if(strcmp(s_queue_recv,pCtrl->httpc[i].recv_queue) == 0)
		{
			memset(&s_pHttp_Ctrl, 0x00, sizeof(s_pHttp_Ctrl));
			s_pHttp_Ctrl = &(pCtrl->httpc[i]);
			break;
		}			
	}
}


// 函数名称  Open_Syn_Http_Log
// 参数说明：
//          无
// 返 回 值：
//          成功返回0,否则-1
// 功能描述：
//          1.获得系统主目录,将之放在全局变量g_pcIseHome
//          2.设置日志文件和日志方式

int Open_Syn_Http_Log( char * buf)
{
	char  caLogName[256];
	char logfile[256];

	memset(caLogName,0,sizeof(caLogName));
	memset(logfile,0,sizeof(logfile));
	sprintf(caLogName,"log/synHttp_%s.log",buf);
	if(u_fabricatefile(caLogName,logfile,sizeof(logfile)) < 0)
	{
		dcs_log(0, 0, "can not open log file:[%s]",caLogName);
		return -1;
	}
	return  dcs_log_open(logfile, buf);
}


static void Syn_Http_Exit(int signo)
{
	dcs_log(0, 0, "syn http[%s]catch sinal %d...\n",s_queue_recv,signo);    

	switch (signo)
	{
		case SIGUSR2: 
			dcs_log(0,0,"管理工具停服务");
			s_pHttp_Ctrl->iStatus = DCS_STAT_STOPPED;
			break;
		case 0:
		case 11:
			if ( s_pHttp_Ctrl->iStatus != DCS_STAT_STOPPED )
			{
				s_pHttp_Ctrl->iStatus = DCS_STAT_STOPPED;
				dcs_log(0,0,"重起服务");
				Start_Process("synHttp",s_pHttp_Ctrl->recv_queue);
			}
			break;
		case SIGTERM:
			dcs_log(0,0,"脚本停服务");
			break;
		default:
		      break;
	}
	dcs_debug(0,0,"exit system");
	exit(signo);
} 



