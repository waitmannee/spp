#define _MainProg

// 程序名称：memSrv.c
// 功能描述：
//          memcache系统服务通讯程序
// 函数清单：
//           main()


#include  "dccglobl.h"
#include  "dccpgdef.h"
#include  "dccdcs.h"
#include  "dccsock.h"
#include  "memSrv.h"
#include  <pthread.h>
#include  <libmemcached/memcached.h>    

#define DCS_MEM_NAME "memcached.conf"
#define TRY_COUNT   1

static int recv_queue_id =  -1; /*接收队列*/
static int send_queue_id = -1;  /*发送队列*/

// Functions declaration
void MemSrvExit(int signo);
int MemSrvOpenLog(char *IDENT);
int MemSaveData(char *srcBuf, char *revBuf, memcached_st *memc);
int MemGetKey(char *srcBuf, char *revBuf, memcached_st *memc);
int DeleteKey(char *srcBuf, char *revBuf, memcached_st *memc);
memcached_st *GetMemLink();
static int g_LinkCnt = 0;
struct MEMSRVCFG *g_pMemCfg;
int InitShmCfg();
void TransProc();
int LoadCommCnt();

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
	int i = 0;
  	catch_all_signals(MemSrvExit);
    pthread_t sthread;
    
  	if(0 > MemSrvOpenLog(argv[0]))
        exit(0);
    
  	if((g_LinkCnt = LoadCommCnt())<=0)
  	{
        dcs_log(0, 0, "<MemSrv LoadCommCnt> Failure to load memsrvlnk.ini !" );
        exit(0);
  	}
  	
  	g_pMemCfg =(struct MEMSRVCFG*)malloc(sizeof(struct memServerInfo)*g_LinkCnt + sizeof(struct MEMSRVCFG)+3);
    memset(g_pMemCfg, 0, g_LinkCnt * sizeof(struct memServerInfo) + sizeof(struct MEMSRVCFG)+3); 
  	g_pMemCfg->maxLink = g_LinkCnt;
  		
  	dcs_debug(0, 0, "begining Init CONF !");
    
  	if(0 >= InitShmCfg())
  	{
  		dcs_log(0, 0, "Can not Init CONF config  !" );
  		exit(0);
  	}
    dcs_debug(0, 0, "Init CONF succ!");
    
    dcs_log(0, 0, "g_pMemCfg->maxLink = %d", g_pMemCfg->maxLink);
  	

    recv_queue_id = queue_create("MEMRVR");
	if(recv_queue_id < 0)
	   recv_queue_id = queue_connect("MEMRVR");
	if(recv_queue_id < 0)
	{
		dcs_log(0, 0, "Can not connect mem recv queue !");
		exit(0);
	}

	send_queue_id = queue_create("MEMRVS");
	if(send_queue_id < 0)
	   send_queue_id = queue_connect("MEMRVS");
	if(send_queue_id < 0)
	{
		dcs_log(0, 0, "Can not connect mem send queue !");
		exit(0);
	}
	memcached_st *memc[g_pMemCfg->maxLink];
	for ( i =0 ;i < g_pMemCfg->maxLink -1; i++)
	{
		memc[i] = GetMemLink();
        if ( pthread_create(&sthread, NULL,( void * (*)(void *))TransProc,(void *)memc[i]) !=0)
        {
            dcs_log(0,0, "Can not create thread ");
            goto lblExit;
        }
        pthread_detach(sthread);
	}
	memc[i] = GetMemLink();
	TransProc((void *)memc[i]);

lblExit:
	for ( i =0 ;i < g_pMemCfg->maxLink -1; i++)
	{
		memcached_free(memc[i]);
	}

  	MemSrvExit(0);
    return 0;
}

/*
 函数名称：MemSrvExit
 参数说明：
 signo：捕获的信号量
 返 回 值：
 无
 功能描述：
 当捕获的信号为SIGTERM时，该函数释放资源后终止当前进程；否则不做任何处理
 */
void MemSrvExit(int signo)
{
  	dcs_debug(0, 0, "catch a signal %d !", signo);
 
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
  	exit(signo);
}

/*
 函数名称：MemSrvOpenLog
 参数说明：
	IDENT->当前的进程名
 返 回 值：
 0：成功
 -1：失败
 功能描述：
 1.获得系统主目录
 2.设置日志文件和日志方式
 */

int MemSrvOpenLog(char *IDENT)
{
  	char   caLogName[256];
    char logfile[400];
    
    memset(caLogName, 0, sizeof(caLogName));
	memset(logfile, 0, sizeof(logfile));
	
    strcpy(caLogName, "log/memsrv.log");
    if(u_fabricatefile(caLogName, logfile, sizeof(logfile)) < 0)
    {
        return -1;
    }
    return dcs_log_open(logfile, IDENT);
}

int LoadCommCnt()
{
  	char caFileName[256];
  	int iFd=-1, iCommCnt = 0;
  	  	
  	//设定config定义文件的绝对路径，并打开
  	memset(caFileName, 0, sizeof(caFileName));
	
    if(u_fabricatefile("config/memcached.conf", caFileName, sizeof(caFileName)) < 0)
    {
        dcs_log(0, 0, "cannot get full path name of 'memcached.conf'\n");
        return -1;
    }
  	if((iFd = conf_open(caFileName)) < 0)
  	{
    	dcs_log(0, 0, "<HstSrv> open memcached.conf fail ! ");
		return -1;
  	}
    
  	//读取配置项'comm.max'
    if ( 0 > conf_get_first_number(iFd, "comm.max", &iCommCnt) || iCommCnt < 0 )
    {
        dcs_log(0,0, "<Mem> load configuration item 'comm.max' failed!");
        conf_close(iFd);
        return (-1);
    }
  	conf_close(iFd);

  	return(iCommCnt);
}


int InitShmCfg()
{	
    char caFileName[256], caBuffer[512];
  	int iFd=-1, iRc = 0, iCommCnt = 0, iSlotIndex = 0;
  	char *pNum, *pAddr, *pPort;
  	
  	//设定config定义文件的绝对路径，并打开
  	memset(caFileName, 0, sizeof(caFileName) );
	memset(caBuffer, 0, sizeof(caBuffer) );
    
    if(u_fabricatefile("config/memcached.conf", caFileName, sizeof(caFileName)) < 0)
    {
        dcs_log(0, 0, "cannot get full path name of 'memcached.conf'\n");
        return -1;
    }
    
    dcs_log(0, 0, "open file = %s", caFileName);
    
  	if((iFd = conf_open(caFileName)) < 0)
  	{
        dcs_log(0, 0, "<MemSrv> open memcached.conf fail ! ");
        return -1;
  	}    
	
    if (0 > conf_get_first_number(iFd, "comm.max", &iCommCnt) || iCommCnt < 0 )
    {
        dcs_log(0, 0, "<Mem> load configuration item 'comm.max' failed!");
        conf_close(iFd);
        return (-1);
    }
	  
    for( iRc = conf_get_first_string(iFd, "comm", caBuffer);
        iRc == 0 && iSlotIndex < g_pMemCfg->maxLink;
        iRc = conf_get_next_string(iFd, "comm", caBuffer))
    {   
	    dcs_log(0, 0, "caBuffer = [%s]", caBuffer);
        pNum = strtok(caBuffer, " \t\r");  
        pAddr = strtok(NULL, " :\t\r");  //地址串
        pPort = strtok(NULL, " \t\r");  //端口号
		      
        if(pAddr==NULL || pPort==NULL  )
        {
            //invalid address:skip this line
			dcs_log(0, 0, "invalid address:skip this line !");
			dcs_log(0, 0, "pNum=[%s], pAddr=[%s], pPort=[%s]", pNum, pAddr, pPort);
            continue;
        }
		dcs_log(0, 0, "pNum=[%s], pAddr=[%s], pPort=[%s]", pNum, pAddr, pPort);
		
        strcpy(g_pMemCfg->memServerInfo[iSlotIndex].caRemoteIp, pAddr);
        g_pMemCfg->memServerInfo[iSlotIndex].iRemotePort = atoi(pPort);
        iSlotIndex++;
    }    
    conf_close(iFd); 
    return 1;
}

memcached_st *GetMemLink()
{
	int i = 0;
	memcached_st *memc;
	memcached_server_st *servers;
	memcached_return rc;
	memc = NULL;
	struct memServerInfo *pLink;

  	for( ; i< g_pMemCfg->maxLink; i++)
  	{
        if(strlen(g_pMemCfg->memServerInfo[i].caRemoteIp)>0)
        {
        	pLink = &g_pMemCfg->memServerInfo[i];
        	if(memc == NULL)
			{
				//dcs_log(0, 0, "memc is null");
				memc = memcached_create(NULL);
				dcs_log(0, 0, "ip:%s, port:%d\n", pLink->caRemoteIp, pLink->iRemotePort);
				servers = memcached_server_list_append(NULL, pLink->caRemoteIp, pLink->iRemotePort, &rc);
			}
			else
			{
				//dcs_log(0, 0, "memc is not null\n");
				dcs_log(0, 0, "ip:%s, port:%d\n", pLink->caRemoteIp, pLink->iRemotePort);
				servers = memcached_server_list_append(servers, pLink->caRemoteIp, pLink->iRemotePort, &rc);
			}
		}
	}
	memcached_behavior_set(memc,MEMCACHED_BEHAVIOR_DISTRIBUTION,MEMCACHED_DISTRIBUTION_CONSISTENT);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 1) ;
	memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_DEAD_TIMEOUT, 25) ;
	memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT, TRY_COUNT);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS, 1) ;

  	memcached_server_push(memc, servers);
	memcached_server_free(servers);
	//通过存取删除数据获取连接
	memcached_delete(memc, "trytogetconnusedeletefunctiontrytogetconnusedeletefunction", 58, (time_t)0);
	return memc;
}

void TransProc(void *varg)
{
	int nRcvLen = 0;
	char caBuf[PACKAGE_LENGTH+1];
	long MsgType = 0;
	char revBuf[PACKAGE_LENGTH+1];
	char operate[4];
	memcached_st *memc = (memcached_st *)varg;
	while (1)
	{  
        memset(caBuf, 0x00, sizeof(caBuf));
		memset(operate, 0x00, sizeof(operate));
        dcs_log(0, 0, "接收队列recv_queue_id = %d", recv_queue_id);
        MsgType = 0;
		nRcvLen = queue_recv_by_msgtype(recv_queue_id, caBuf, sizeof(caBuf), &MsgType, 0);
        if(nRcvLen <0)
        {
		 	dcs_log(0, 0, "Recive Message fail!");
		 	break;
        }
		
		dcs_log(0, 0, "接收队列MsgType=[%ld]]", MsgType);
		dcs_debug(caBuf, nRcvLen, "收到的数据包begin");
		memcpy(operate, caBuf, 3);
		dcs_log(0, 0, "operate=[%s]]", operate);
		if(memcmp(operate, "SET", 3) ==0)
		{
			//save data todo...
			memset(revBuf, 0x00, sizeof(revBuf));
			nRcvLen = MemSaveData(caBuf+3, revBuf, memc);
		}
	    else if(memcmp(operate, "GET", 3) ==0)
		{
			//get data todo...
			memset(revBuf, 0x00, sizeof(revBuf));
			nRcvLen = MemGetKey(caBuf+3, revBuf, memc);
		}
		else if(memcmp(operate, "DEL", 3) ==0)
		{
			//delete key todo...
			memset(revBuf, 0x00, sizeof(revBuf));
			nRcvLen = DelteKey(caBuf+3, revBuf, memc);
		}
        dcs_debug(revBuf, nRcvLen, "返回消息队列的数据 len =%d", nRcvLen);
	   	queue_send_by_msgtype(send_queue_id, revBuf, nRcvLen, MsgType, 0);
	}
}

/*
  保存数据到memcached
  参数：expiration_time为数据保存的时间，单位秒。如果为0，表示永远保存。
 */
int MemSaveData(char *srcBuf, char *revBuf, memcached_st *memc)
{
	int offset = 0, result_len = 0;
	char temp_buf[127];
	size_t key_length =0;
	size_t value_length = 0;
	memcached_return rc;
	char keys[127], values[1024];
    int temp_trycount = 0; /*处理memcached的重试次数*/
    int expiration_time;
	
	memset(keys, 0, sizeof(keys));
	memset(values, 0, sizeof(values));
	
	
    //获取过期时间
    memset(temp_buf, 0, sizeof(temp_buf));
    memcpy(temp_buf, srcBuf, 10);
    offset +=10;
    expiration_time = atoi(temp_buf);
    //dcs_log(0, 0, "expiration_time[%d]", expiration_time);

    memset(temp_buf, 0, sizeof(temp_buf));
	memcpy(temp_buf, srcBuf+offset, 3);
	offset +=3;
	key_length = atoi(temp_buf);
	memcpy(keys, srcBuf+offset, key_length);
	offset +=key_length;
	
	memset(temp_buf, 0, sizeof(temp_buf));
	memcpy(temp_buf, srcBuf+offset, 4);
	offset +=4;
	value_length = atoi(temp_buf);
	memcpy(values, srcBuf+offset, value_length);
	offset +=value_length;
	
	for(temp_trycount = TRY_COUNT+3; temp_trycount >0;)
	{
        //目前超时时间设置为0（永远）,原来是600秒
		rc = memcached_set(memc, keys, key_length, values, value_length, (time_t)expiration_time, (uint32_t)0);
		if(rc == MEMCACHED_SUCCESS)
		{
			dcs_log(0, 0, "Save keys [%s]:\"%s\"success.", keys, values);
			memcpy(revBuf, "00", 2);
			memcpy(revBuf+2, "SET", 3);
			memcpy(revBuf+5, "STORED", 6);
			result_len = strlen(revBuf);
			break;
		}
		else
		{
			dcs_log(0, 0, "Save keys [%s]:\"%s\"false. try again[%d]", keys, values, temp_trycount);
            //usleep功能把进程挂起一段时间， 单位是微秒（百万分之一秒）；
            //头文件： unistd.h
			usleep(100000);
			temp_trycount--;
		}
	}
	if(temp_trycount <= 0)
	{
		dcs_log(0, 0, "Save keys [%s]:\"%s\"false.", keys, values);
		memcpy(revBuf, "FF", 2);
		memcpy(revBuf+2, "SET", 3);
		memcpy(revBuf+5, "UNSTORED", 8);
		result_len = strlen(revBuf);
	}
	return result_len;
}
/*从memcached服务器取数据*/
int MemGetKey(char *srcBuf, char *revBuf, memcached_st *memc)
{
	char return_key[MEMCACHED_MAX_KEY];
	size_t return_key_length;
	char *return_value;
	size_t return_value_length;
	memcached_return rc;
	
	size_t key_length = 0;
	uint32_t flags;
	int result_len = 0;
    int temp_trycount = 0; /*处理memcached的重试次数*/

	key_length = strlen(srcBuf);
	dcs_log(0, 0, "srcBuf=[%s]", srcBuf);
	
	dcs_log(0, 0, "key_length=[%d]", key_length);
	for(temp_trycount = TRY_COUNT+3; temp_trycount >0;)
	{
		rc = memcached_mget(memc, &srcBuf, &key_length, 1);
		memset(return_key, 0x00, sizeof(return_key));
		return_value = memcached_fetch(memc, return_key, &return_key_length, &return_value_length, &flags, &rc);
		{
			if(rc == MEMCACHED_SUCCESS)
			{
				dcs_log(0, 0, "Fetch key:%s data:%s", return_key, return_value);
				memcpy(revBuf, "00", 2);
				memcpy(revBuf+2, return_value, return_value_length);
				result_len = return_value_length + 2;
				break;
			}
			else
			{
				dcs_log(0, 0, "Get key:apple data:\"%s\"false. try again [%d]", return_value, temp_trycount);
				usleep(100000);
				temp_trycount--;
			}
		}
	}
	if(temp_trycount <= 0)
	{
		dcs_log(0, 0, "Get key:apple data:\"%s\"false.", return_value);
		memcpy(revBuf, "FF", 2);
		memcpy(revBuf+2, "FALSE", 5);
		result_len = 7;
	}
	free(return_value);
	return_value = NULL;
	return result_len;
}

/*删除缓存memcached服务器上的数据*/
int DelteKey(char *srcBuf, char *revBuf, memcached_st *memc)
{
	size_t key_length = 0;
	int result_len = 0;
	key_length = strlen(srcBuf);
	memcached_return rc;
    int temp_trycount = 0; /*处理memcached的重试次数*/

	for(temp_trycount = TRY_COUNT+3; temp_trycount >0;)
	{
		rc = memcached_delete(memc, srcBuf, key_length, (time_t)0);	
		if(rc == MEMCACHED_SUCCESS)
		{
			dcs_log(0, 0, "delete key:[%s] success", srcBuf);
			memcpy(revBuf, "00", 2);
			memcpy(revBuf+2, "DELETED", 7);
			result_len = 9;	
			break;
		}
		else
		{
			dcs_log(0, 0, "Delete key \"%s\" false try again [%d].", srcBuf, temp_trycount);
			usleep(100000);
			temp_trycount--;
		}
	}
	if(temp_trycount <= 0)
	{
		dcs_log(0, 0, "Delete key \"%s\" false.", srcBuf);
		memcpy(revBuf, "FF", 2);
		memcpy(revBuf+2, "FALSE", 5);
		result_len = 7;
	}
	return result_len;
}
