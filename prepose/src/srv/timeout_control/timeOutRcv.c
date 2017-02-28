/*
   功能描述:
   本文件主要实现超时控制功能，通过folder接收到业务处理进程发过来的报文
   与业务进程交互以http协议的方式，配置http相关客户端和服务器端链路即可,报文采用key=value的方式
   
   ,根据收到的要求处理其超时重发.
    modify by li at 2017/01/04
*/

#include "unixhdrs.h"
#include "extern_function.h"
#include "timer.h"
#include <sys/syscall.h>
#include "memlib.h"
#include "trans.h"
#include "iso8583.h"

extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];
struct ISO_8583 iso8583conf[128];

extern pthread_key_t iso8583_conf_key;//线程私有变量
extern pthread_key_t iso8583_key;//线程私有变量

static int g_myFid = 1;
static char sg_ProcessName[20]="timeOutRcv";

static int do_timer_proc();
static int openTimerLog(char * buf);
static int createMyfolder();
static void timerExit(int signo);
static void *dealProc( void *ptr);
static int check_ProIsExist_ByName(char *processName);
static int analysis_request(char *buf, struct TIMEOUT_INFO *timeout_info);
static int search_tminfo(struct TIMEOUT_INFO *timeout_info);
static int GetValueByKey(char *source, char *key, char *value, int value_len);
static int getnode(char *srvBuf, char *node, char *retBuf);
static int busProc(ISO_data siso, struct TIMEOUT_INFO *timeout_info);
static int MsgUnpack( char *srcBuf, ISO_data *iso, int iLen);


int main(int arvc, char *argv[])
{
	catch_all_signals(timerExit);
	
	if(openTimerLog(argv[0]) < 0)
		return -1;
	
	if(check_ProIsExist_ByName(sg_ProcessName) != 1)
	{
		dcs_log(0, 0, "进程 %s 已经存在.", sg_ProcessName);
		return -1;
	}
	
	if(createMyfolder() < 0)
	{
		dcs_log(0, 0, "creat my folder fail");
		return -1;
	}
		
	//加载前置与核心的8583配置文件
	if(IsoLoad8583config(&iso8583conf[0], "iso8583.conf") < 0)
	{
		dcs_log(0,0,"Load iso8583.conf failed:%s", strerror(errno));
		timerExit(0);
	}
	
	//设置为多线程
	SetIsoMultiThreadFlag(1);
	pthread_key_create(&iso8583_conf_key,NULL);
	pthread_key_create(&iso8583_key,NULL);
	
	//创建mysql 连接池，
	if(mysql_pool_init(5)< 0)
		timerExit(0);

	dcs_log(0, 0, "*************************************************\n"
			"!!        超时控制进程启动成功.        !!\n"
			"*************************************************\n");
	do_timer_proc();	
	
}

int do_timer_proc()
{
	int fid = 0, org_folderId = 0, ret = 0;
	char buf[sizeof(struct TIMEOUT_INFO)];
	char rcvbuf[128];//返回报文
	char tm_tpdu[6];
	//保存到缓存数据memcached结构体
	KEY_VALUE_INFO key_value_info;
	struct TIMEOUT_INFO timeout_info;
	struct TIMEOUT_INFO *ptr = NULL;
	pthread_t pid = 0;
	
	memset(&key_value_info, 0x00, sizeof(KEY_VALUE_INFO));
	memset(&timeout_info, 0x00, sizeof(struct TIMEOUT_INFO));
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	memset(tm_tpdu, 0x00, sizeof(tm_tpdu));
	
	fid = fold_open(g_myFid);
	
	while(1)
	{	
		memset(buf, 0x00, sizeof(buf));
		ret = fold_read(g_myFid, fid ,&org_folderId, buf, sizeof(buf), 1);
		if(ret <= 0)
			continue;
		memset(&key_value_info, 0x00, sizeof(KEY_VALUE_INFO));
		memset(&timeout_info, 0x00, sizeof(struct TIMEOUT_INFO));
		//解析收到的报文key=value格式operation=&key=&timeout=&sendnum=&data=&length=&
		memcpy(tm_tpdu, buf+4, 5);
		ret = analysis_request(buf+4+5, &timeout_info);
		if(ret < 0)
		{
			dcs_log(0, 0, "请求报文解析失败");
			strcpy(rcvbuf, "response=E0");
			goto labex;
		}
		
		if(memcmp(timeout_info.operation, "ADD", 3)==0)
		{
			//新增一个超时控制
			dcs_log(0, 0, "新增一个超时控制");
			ptr = (struct TIMEOUT_INFO *)malloc(sizeof(struct TIMEOUT_INFO));
			if(ptr == NULL)
			{
				dcs_log(0, 0, "malloc fail!!");
				strcpy(rcvbuf, "response=E0");
				goto labex;
			}
			memset(buf, 0x00, sizeof(buf));
			asc_to_bcd((unsigned char *)buf, (unsigned char*)timeout_info.data, timeout_info.length, 0);
			timeout_info.length = timeout_info.length/2;
			memset(timeout_info.data, 0x00, sizeof(timeout_info.data));
			memcpy(timeout_info.data, buf, timeout_info.length);
			
			memcpy(ptr, &timeout_info, sizeof(struct TIMEOUT_INFO));
			
			if(pthread_create(&pid, NULL, dealProc, (void *)ptr) < 0)
			{
				dcs_log(0, 0, "pthread_creat fail ");
				strcpy(rcvbuf, "response=E0");
				goto labex;
			}
			pthread_detach(pid);
			//保存信息到memcached
			timeout_info.pid = pid;
			OrganizateData(timeout_info, &key_value_info);			
			ret = Mem_SavaKey(key_value_info.keys, key_value_info.value, key_value_info.value_length, 600);
			if(ret == -1)
			{
				dcs_log(0, 0, "save mem error");
				strcpy(rcvbuf, "response=E0");
				goto labex;	
			}
			else
				strcpy(rcvbuf, "response=00");			
		}
		else if(memcmp(timeout_info.operation, "DELETE", 6)==0)
		{
			//去除一个超时控制
			dcs_log(0, 0, "移除一个超时控制,key[%s]", timeout_info.key);
			//查找
			ret = search_tminfo(&timeout_info);
			if(ret ==-1)
			{
				dcs_log(0, 0, "没找到该key对应的超时控制");
				strcpy(rcvbuf, "response=00&status=0");
			}
			else
			{
				dcs_log(0, 0, "search ok, timeout_info.status=%c", timeout_info.status);
				if(timeout_info.status =='0')
				{
					dcs_log(0, 0, "该超时控制尚未处理");
					pthread_cancel(timeout_info.pid);//结束处理线程
					
					//修改memcached的该条记录的状态
					timeout_info.status ='2';
					OrganizateData(timeout_info, &key_value_info);
					ret = Mem_SavaKey(key_value_info.keys, key_value_info.value, key_value_info.value_length, 600);
					if(ret == -1)
					{
						dcs_log(0, 0, "save mem error");
					}
					
					strcpy(rcvbuf, "response=00&status=1");
				}
				else if(timeout_info.status =='1')
				{
					dcs_log(0, 0, "超时控制已经按照超时处理");
					strcpy(rcvbuf, "response=00&status=2");
				}	
				else if(timeout_info.status =='2')
				{
					dcs_log(0, 0, "超时控制已经移除");
					strcpy(rcvbuf, "response=00&status=1");
				}
			}
		}
		else
		{
			//非法内容,丢弃
			dcs_debug(buf, ret, "非法内容,丢弃");
			strcpy(rcvbuf, "response=E0");
		}	
		//需要写返回应答报文
labex:
		memset(buf, 0x00, sizeof(buf));
		memcpy(buf, tm_tpdu, 1);
		memcpy(buf+1, tm_tpdu+3, 2);
		memcpy(buf+3, tm_tpdu+1, 2);
		memcpy(buf+5, rcvbuf, strlen(rcvbuf));
		fold_write(org_folderId, g_myFid, buf, strlen(rcvbuf)+5);
	}
	close(fid);
	return 0;
}

// 函数名称  openTimerLog
// 参数说明：
//          buf 本进程名
// 返 回 值：
//          成功返回0,否则-1
// 功能描述：
//          创建一个日志文件并打开

int openTimerLog(char *buf)
{
	char caLogName[256];
	char logfile[256];

	memset(caLogName, 0x00, sizeof(caLogName));
	memset(logfile, 0x00, sizeof(logfile));
	sprintf(caLogName, "log/timeOutRcv.log");
	if(u_fabricatefile(caLogName, logfile, sizeof(logfile)) < 0)
	{
		dcs_log(0, 0, "can not open timer log file:[%s]", caLogName);
		return -1;
	}
	return  dcs_log_open(logfile, buf);
}

// 函数名称  createMyfolder
// 参数说明：
//        创建folder
// 返 回 值：
//          退出进程
// 功能描述：
//       处理捕获到的信号
int createMyfolder()
{
	if(fold_initsys() < 0)
	{
		dcs_log(0, 0, "cannot attach to folder system!");
		return -1;
	}

	g_myFid = fold_create_folder("TIMEOUTRCV");

	if(g_myFid < 0)
		g_myFid = fold_locate_folder("TIMEOUTRCV");    

	if(g_myFid < 0)        
	{
		dcs_log(0, 0, "cannot create folder '%s':%s\n", "TIMEOUTRCV", strerror(errno));
		return -1;
	} 

	dcs_log(0, 0, "folder myFid=%d\n", g_myFid);
	return 0;
}

// 函数名称  timerExit
// 参数说明：
//          捕获的信号
// 返 回 值：
//          退出进程
// 功能描述：
//       处理捕获到的信号
static void timerExit(int signo)
{
	dcs_log(0, 0, "timeOutRcv catch sinal %d...\n", signo);    

	switch (signo)
	{
		case 0:
		case 11:
			dcs_log(0, 0, "重起服务");
			Start_Process(sg_ProcessName, NULL);
			break;
		case SIGTERM:
			dcs_log(0, 0, "脚本停服务");
			break;
		default:
		    break;
	}
	DestroyLinkPool();
	dcs_debug(0, 0, "exit system");
	exit(signo);
}

// 函数名称  dealProc
// 参数说明：
//        传入的包的信息
// 返 回 值：
//          无
// 功能描述：
//       处理收到的包，超时重发
void *dealProc(void *ptr)
{
	struct TIMEOUT_INFO  timeout_info;
	ISO_data iso;
	KEY_VALUE_INFO key_value_info;
	int i = 0, org_folderId = 0, ret = 0;
	
	memset(&timeout_info, 0x00, sizeof(struct TIMEOUT_INFO));
	memset(&iso, 0x00, sizeof(ISO_data));
	memset(&key_value_info, 0x00, sizeof(KEY_VALUE_INFO));
	
	memcpy(&timeout_info, ptr, sizeof(struct TIMEOUT_INFO));
	free(ptr);
	ptr = NULL;
	dcs_log(0, 0, "key=[%s] 重发foldid=[%d]次数为[%d], 超时时间为[%d]", timeout_info.key, timeout_info.foldid, timeout_info.sendnum, timeout_info.timeout);
	if(memcmp(timeout_info.key, "PEX_COM", 7)==0)
	{
		//拆包
		if (0> MsgUnpack(timeout_info.data, &iso, timeout_info.length))
		{
			dcs_log(0, 0, "拆包失败");
			return;
		}
	}	
	for(i = 0; i<timeout_info.sendnum; i++)
	{
		sleep(timeout_info.timeout);//开始休息
		if(memcmp(timeout_info.key, "PEX_COM", 7)==0)
		{
			//重新组data，更新冲正标识
			if(busProc(iso, &timeout_info)<0)
			{
				dcs_log(0, 0, "业务处理失败");
				return;
			}
		}
		if(fold_write(timeout_info.foldid, org_folderId, timeout_info.data, timeout_info.length) < 0)
		{
			dcs_log(0, 0, "fold_write key=%s fail", timeout_info.key);
			return;
		}
	}
	dcs_log(0, 0, "key=[%s] 重发次数用完", timeout_info.key);
	//修改memcached中记录的状态为超时已经发送
	timeout_info.status = '1';
	OrganizateData(timeout_info, &key_value_info);
	ret = Mem_SavaKey(key_value_info.keys, key_value_info.value, key_value_info.value_length, 600);
	if(ret == -1)
	{
		dcs_log(0, 0, "save mem error");
		return;
	}
}

int check_ProIsExist_ByName( char *processName)
{
    FILE* fp = NULL;
    int count = 0;
    char buf[10];
    char command[80];
    memset(buf, 0x00, sizeof(buf));
    memset(command, 0x00, sizeof(command));
	
    sprintf(command, "pgrep -x %s|wc -l", processName);

    if((fp = popen(command, "r")) == NULL)
    {
       dcs_log(0, 0, "<Check_Queue>popen error %s", strerror(errno));
       return -1;
    }
    if((fgets(buf,10, fp))!= NULL )
    {
       count = atoi(buf);
       if(count > 1)
       {
         dcs_log(0, 0, "<Check_Queue>process %s is exist!\n", processName);
         pclose(fp);
         return 0;
       }
       else
       {
         pclose(fp);
         return 1;
       }
    }
    pclose(fp);
    return -1;
}


static int analysis_request(char *buffer, struct TIMEOUT_INFO *timeout_info)
{
	int rtn = 0;
	char tempbuf[128];
	
	memset(tempbuf, 0x00, sizeof(tempbuf));
	
	//解析operation数据
	rtn = GetValueByKey(buffer, "operation", timeout_info->operation, sizeof(timeout_info->operation));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析operation失败");
		return -1;
	}
	//解析key数据
	rtn = GetValueByKey(buffer, "key", timeout_info->key, sizeof(timeout_info->key));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析key失败");
		return -1;
	}
	if(memcmp(timeout_info->operation, "ADD", 3)==0)
	{
		//解析timeout数据
		memset(tempbuf, 0x00, sizeof(tempbuf));
		rtn = GetValueByKey(buffer, "timeout", tempbuf, sizeof(tempbuf));
		if(rtn == -1)
		{
			dcs_log(0, 0, "解析timeout失败");
			return -1;
		}
		timeout_info->timeout = atoi(tempbuf);
		//解析sendnum数据
		memset(tempbuf, 0x00, sizeof(tempbuf));
		rtn = GetValueByKey(buffer, "sendnum", tempbuf, sizeof(tempbuf));
		if(rtn == -1)
		{
			dcs_log(0, 0, "解析sendnum失败");
			return -1;
		}
		timeout_info->sendnum = atoi(tempbuf);
		//解析foldid数据
		memset(tempbuf, 0x00, sizeof(tempbuf));
		rtn = GetValueByKey(buffer, "foldid", tempbuf, sizeof(tempbuf));
		if(rtn == -1)
		{
			dcs_log(0, 0, "解析foldid失败");
			return -1;
		}
		timeout_info->foldid= atoi(tempbuf);
		//解析length数据
		rtn = GetValueByKey(buffer, "length", tempbuf, sizeof(tempbuf));
		if(rtn == -1)
		{
			dcs_log(0, 0, "解析length失败");
			return -1;
		}
		timeout_info->length = atoi(tempbuf);
		//解析data数据
		rtn = GetValueByKey(buffer, "data", timeout_info->data, sizeof(timeout_info->data));
		if(rtn == -1)
		{
			dcs_log(0, 0, "解析data失败");
			return -1;
		}
		timeout_info->status = '0';//状态初始化为0
	}
	return 0;
}

/**
*
函数名称：OrganizateData
函数功能：组key和value的长度和值，初始化结构体变量KEY_VALUE_INFO
输入参数：timeout_info
key:timeout_info里的key
values：<t>数据字典</t> xml格式数据
*
**/
int OrganizateData(struct TIMEOUT_INFO timeout_info, KEY_VALUE_INFO *key_value_info)
{
	char tempbuf[1024];
	int temp_len = 0;
	
	memset(tempbuf, 0x00, sizeof(tempbuf));
	
	key_value_info->keys_length = strlen(timeout_info.key);

	memcpy(key_value_info->keys, timeout_info.key, key_value_info->keys_length);
	
	sprintf(tempbuf, "%s\r\n\t", "<t>");
	sprintf(tempbuf, "%s%s%c%s\r\n\t", tempbuf, "<status>", timeout_info.status, "</status>");
	sprintf(tempbuf, "%s%s%ld%s\r\n\t", tempbuf, "<pid>", timeout_info.pid, "</pid>");
	sprintf(tempbuf, "%s%s\r\n", tempbuf, "</t>");
	
	temp_len = strlen(tempbuf);
	memcpy(key_value_info->value, tempbuf, temp_len);
	key_value_info->value_length = temp_len;
	#ifdef __TEST__
		dcs_log(0, 0, "key_value_info->keys=[%s]", key_value_info->keys);
		dcs_log(0, 0, "key_value_info->keys_length=[%d]", key_value_info->keys_length);
		dcs_log(0, 0, "key_value_info->value=[%s]", key_value_info->value);
		dcs_log(0, 0, "key_value_info->value_length=[%d]", key_value_info->value_length);
	#endif
	return 0;
}

/*删除*/
static int search_tminfo(struct TIMEOUT_INFO *timeout_info)
{
	
	//memcached存取数据
	char value[1024], tmpbuf[1024];
	int value_len = 0, datalen = 0;
	int rtn = 0;
	
	//保存到缓存数据memcached结构体
	KEY_VALUE_INFO key_value_info;
	
	memset(&key_value_info, 0x00, sizeof(KEY_VALUE_INFO));
	memset(value, 0x00, sizeof(value));
	
	memcpy(key_value_info.keys, timeout_info->key, strlen(timeout_info->key));
	rtn = Mem_GetKey(key_value_info.keys, value, &value_len, sizeof(value));
	if(rtn == -1)
	{
		dcs_log(0, 0, "get memcached value error");
		return -1;
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "status", tmpbuf);
	if(datalen < 0)
	{
		dcs_log(0, 0, "获取节点status失败");
		return -1;
	}
	tmpbuf[datalen] =0;
	timeout_info->status =tmpbuf[0];
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "pid", tmpbuf);
	if(datalen < 0)
	{
		dcs_log(0, 0, "获取节点pid失败");
		return -1;
	}
	tmpbuf[datalen] =0;
	timeout_info->pid = atol(tmpbuf);
	
	dcs_log(0, 0, "set or get uccess");
	return 0;
}


/*根据key获取到value的值*/
static int GetValueByKey(char *source, char *key, char *value, int value_len)
{
	char *delims = "&";
	char *result = NULL;
	char *saveptr= NULL;
	char *saveptr1= NULL;
	char *sub_delims = "=";
	char *temp_key =NULL;
	char *temp_value =NULL;
	int temp_len = 0, flag = 0;
	char buf[2048];
	memset(buf, 0x00, sizeof(buf));
	strcpy(buf, source);
	result = strtok_r(buf, delims, &saveptr);
	while(result != NULL) 
	{
		temp_key = strtok_r(result, sub_delims, &saveptr1);
		if(temp_key == NULL)
        {
			result = strtok_r(NULL, delims, &saveptr);
            continue;
        }
		temp_value = strtok_r(NULL, sub_delims, &saveptr1);
		if(temp_value == NULL)
        {
			result = strtok_r(NULL, delims, &saveptr);
            continue;
        }
		if(strlen(key)==strlen(temp_key) && memcmp(temp_key, key, strlen(temp_key))==0)
		{
			temp_len =  strlen(temp_value);
			if(temp_len > value_len)
				temp_len = value_len;
			if(value_len == 1)
				*value = *temp_value;
			else
			{
				memcpy(value, temp_value, temp_len);
				value[temp_len] =0;
			}
			flag = 1;
			break;
		}
		result = strtok_r(NULL, delims, &saveptr);
	} 
	if(flag == 0)
		return -1;
	return 0;	
}

//根据节点取数据
static int getnode(char *srvBuf, char *node, char *retBuf)
{
	char subbuf1[127], subbuf2[127];
	char *begin = NULL;
	char *end = NULL;
	char *teststr = NULL;
	int length = 0;
	
	sprintf(subbuf1, "%s%s%s", "<", node, ">");
	sprintf(subbuf2, "%s%s%s", "</", node, ">");
	begin = strstr(srvBuf, subbuf1);
	end = strstr(srvBuf, subbuf2);
	if(begin == NULL || end == NULL)
		return -1;
	length = end- begin;
	if(length < strlen(node)+2)
		return -1;
	teststr = (char *)malloc(1000 * sizeof(char));
	memset(teststr, 0x00, 1000);
	strncpy(teststr, begin, length);
	
	length = length-strlen(node)-2;
	memcpy(retBuf, teststr+strlen(node)+2, length);
	free(teststr);
	return length;
}

//获取系统时间，重新组7域，重新获取需要发送的data
static int busProc(ISO_data siso, struct TIMEOUT_INFO *timeout_info)
{
	//系统自带的时间结构体
	struct  tm *tm;   
	time_t  t;
	//当前日期
	char currentDate[14+1], tmpbuf[128], buffer[1024], order_info[40+1], trace[6+1];
	PEP_JNLS pep_jnls;
	int s = 0;
	
	memset(currentDate, 0x00, sizeof(currentDate));
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(buffer, 0x00, sizeof(buffer));
	memset(order_info, 0x00, sizeof(order_info));
	memset(trace, 0x00, sizeof(trace));
	memset(&pep_jnls, 0x00, sizeof(PEP_JNLS));
	
	time(&t);
    tm = localtime(&t);
    
    sprintf(currentDate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
    sprintf(currentDate+8, "%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	
	setbit(&siso, 7, (unsigned char *)currentDate+4, 10);
	
	s = iso_to_str((unsigned char *)buffer, &siso, 1);
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	bcd_to_asc((unsigned char *)tmpbuf, (unsigned char *)buffer, 20 ,0);
	
	memcpy(timeout_info->data, tmpbuf, 20);
	memcpy(timeout_info->data+20, buffer+10, s-10);
	s = s+10;

	//更新冲正标识，保存冲正信息
	memcpy(order_info, timeout_info->key+7, getstrlen(timeout_info->key)-13);
	memcpy(trace, timeout_info->key+7+getstrlen(order_info), 6);
	if(0>updateReversalFlag(order_info, trace))
	{
		dcs_log(0, 0, "updateReversalFlag error");
		return -1;
	}
	if(0>getPepJnlsInfo(order_info, trace, &pep_jnls))
	{
		dcs_log(0, 0, "getPepJnlsInfo error");
		return -1;
	}
	if(0>SaveRevInfo(siso, pep_jnls))
	{
		dcs_log(0, 0, "SaveRevInfo error");
		return -1;
	}
	return 0;
}

//拆包
static int MsgUnpack( char *srcBuf, ISO_data *iso, int iLen)
{
	char buffer[1024], tmpbuf[400], bcdbuf[400];
	int s;

	if( iLen < 20 )
	{
		dcs_log(0,0, "MsgUnpack error");
		return -1;
	}
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(bcdbuf, 0, sizeof(bcdbuf));
	memset(buffer, 0, sizeof(buffer));

	memcpy(tmpbuf, srcBuf, 20);
	tmpbuf[19]='0';

	asc_to_bcd((unsigned char *)bcdbuf, (unsigned char *)tmpbuf, 20, 0);
	memcpy(buffer, bcdbuf, 10);
	

	memcpy(buffer+10, srcBuf+20, iLen-20);
	
	pthread_setspecific(iso8583_conf_key, iso8583conf);
	pthread_setspecific(iso8583_key, iso8583conf);
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(1);
	
	if(str_to_iso((unsigned char *)buffer, iso, iLen-4-10+8)<0) 
	{
		dcs_debug(srcBuf, iLen, "cant not analyse got message(%d bytes) from ter-->", iLen);
		return -1;            
	}
	
	return 1;
}
