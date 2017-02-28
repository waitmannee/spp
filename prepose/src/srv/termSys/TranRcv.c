#include "ibdcs.h"
#include "tmcibtms.h"
#include "iso8583.h"
#include "pub_function.h"
static int gs_myFid    = -1;
static void TransRcvSvrExit(int signo);

extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];
struct ISO_8583 iso8583conf[128];
struct ISO_8583 iso8583posconf[128];

static int OpenLog(char *IDENT);
static int CreateMyFolder();
static int DoLoop();

int main(int argc, char *argv[])
{
	int qid;
	char szTmp[20];
	catch_all_signals(TransRcvSvrExit);

	if(0 > OpenLog(argv[0]))
		exit(1);

	dcs_log(0,0, "TranRcv Servers is starting up...");

	u_daemonize(NULL);

	//初始化数据库连接
	if (!DssConect())
	{
		dcs_log(0,0,"Can not open mysql DB !");
		TransRcvSvrExit(0);
	}
	
	//加载前置与终端的8583配置文件
	if(IsoLoad8583config(&iso8583posconf[0], "iso8583_pos.conf") < 0)
	{
		dcs_log(0,0,"Load iso8583_pos.conf failed:%s",strerror(errno));
		TransRcvSvrExit(0);
	}

	//加载前置与核心的8583配置文件
	if(IsoLoad8583config(&iso8583conf[0],"iso8583.conf") < 0)
	{
		dcs_log(0,0,"Load iso8583.conf failed:%s",strerror(errno));
		TransRcvSvrExit(0);
	}

	if( CreateMyFolder() < 0 )
		TransRcvSvrExit(0);
	
	sem_get("SHA_TRANRCV");
	

	dcs_log(0,0,"*************************************************\n"
				"!!        TranRcv servers startup completed.        !!\n"
				"*************************************************\n");
	//create 自己的接收队列
	 memset(szTmp,0,sizeof(szTmp));	
	 sprintf(szTmp,"%d",getpid());
	if(queue_create(szTmp)<0)
		dcs_log(0,0,"create queue %s err,errno=%d",szTmp,errno);
	
	DoLoop();

	TransRcvSvrExit(0);
}

static void TransRcvSvrExit(int signo)
{
	dcs_log(0,0,"catch a signal %d",signo);
	    
	if(signo  == 15)
	    dcs_log(0,0,"脚本停服务");
	else if(signo == 11)
	{
		dcs_log(0,0,"内存溢出");
	}

	dcs_log(0,0,"TransRcv exit.");

	DssEnd(0);
	    
	exit(signo);
}

static int OpenLog(char *IDENT)
{
	char logfile[256];
	memset(logfile, 0, sizeof(logfile));
	
	if(u_fabricatefile("log/TransRcv.log",logfile,sizeof(logfile)) < 0)
		return -1;

	return dcs_log_open(logfile, IDENT);
}

static int CreateMyFolder()
{
	if(fold_initsys() < 0)
	{
		dcs_log(0,0, "cannot attach to folder system!");
		return -1;
	}

	gs_myFid = fold_create_folder("TRANSRCV");

	if(gs_myFid < 0)
		gs_myFid = fold_locate_folder("TRANSRCV");    

	if(gs_myFid < 0)        
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", "TRANSRCV", ise_strerror(errno) );
		return -1;
	} 

	dcs_log(0,0, "folder myFid=%d\n", gs_myFid);
#ifdef __TEST__
	if(fold_get_maxmsg(gs_myFid) < 100)
		fold_set_maxmsg(gs_myFid, 500) ;
#else
	if(fold_get_maxmsg(gs_myFid) <2)
		fold_set_maxmsg(gs_myFid, 20) ;
#endif
	return 0;
}

static int DoLoop()
{
    char caBuffer[PUB_MAX_SLEN+4+1];
    int iRead =0, fromFid =0;
    int fid;
    fid =fold_open(gs_myFid);
    while(1)
    { 
        memset(caBuffer, 0, sizeof(caBuffer));
        iRead = fold_read(gs_myFid,fid, &fromFid, caBuffer, sizeof(caBuffer), 1);
        if(iRead < 0) 
        {
            dcs_log(0,0,"fold_read() failed:%s\n",ise_strerror(errno));
            break;
        }

        
        if(( iRead < 33 ) && ( memcmp( caBuffer,"TIME",4) != 0))
			continue;

        //dcs_debug(caBuffer, iRead, "recvice from my folder myid=%d, fromid=%d, len =[%d]", gs_myFid, fromFid, iRead);
		if(GetMysqlLink()!=0)
		{
			dcs_log(0, 0, "数据库连接失败");
			continue;
		}
	
       	if(memcmp(caBuffer, "TPOS", 4) ==0) /*终端发起的pos交易*/
      		posProc(caBuffer, fromFid, gs_myFid, iRead);
      	else if(memcmp( caBuffer, "H3CP",4) ==0)  /*ewp交易数据*/
      		ewpProc(caBuffer, fromFid, gs_myFid, iRead);
      	else if(memcmp( caBuffer, "CUST",4) ==0) /*支付管理平台过来的交易报文*/
      		custProc(caBuffer, fromFid, gs_myFid, iRead);
      	else if(memcmp(caBuffer, "GLPT", 4) ==0)  /*管理平台交易数据*/
      		glptProc(caBuffer+4, fromFid, gs_myFid, iRead-4);
        //add by he.qingqi@chinaebi.com,用于memcached测试
        //else if(memcmp(caBuffer, "MEMC", 4) ==0)  /*测试memcached服务*/
        //    memcProc(caBuffer+4, fromFid, gs_myFid, iRead-4);
        else if(memcmp( caBuffer, "MPOS",4) ==0)/*mpos交易数据*/
            mposProc(caBuffer, fromFid, gs_myFid, iRead);
      	#ifdef __TEST__
      	else 
       		ewpProc(caBuffer, fromFid, gs_myFid, iRead);/*消费、查询、余额查询*/
		#else
		else
			dcs_debug(caBuffer, iRead, "Unkown Packet,discard !");
      	#endif
    }
    close(fid);
    return -1;
}

/*
	根据某个商户号生产对应的流水
	成功返回0，失败返回-1

int createMerCodeTrace(char* mercode, char* ctrace)
{
	if(semid < 0)
	{
		semid = sem_connect("TRANSEM", 2);
	    if ( semid < 0 )
	  	{
			dcs_log(0,0, "获取TRANSEM信号量错误,创建信号量TRANSEM...");
			semid = sem_create("TRANSEM", 2);
			if(semid <0)
			{
				dcs_log(0,0, "Can not create Sem name=TRANSEM" );
				return -1;
			}
		}
	}
	
	sem_lock(semid, 1, 1);
	pub_get_trace(mercode, ctrace);
	sem_unlock(semid, 1, 1);
	return 0;
}
*/


