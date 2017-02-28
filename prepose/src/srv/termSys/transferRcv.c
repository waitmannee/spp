#include "ibdcs.h"
#include "tmcibtms.h"

static int gs_myFid = -1;
static void TransferSrvExit(int signo);
static int OpenLog(char *IDENT);
static int CreateMyFolder();
static int DoLoop();

extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];
struct ISO_8583 iso8583conf[128];
struct ISO_8583 iso8583posconf[128];
/*
	处理转账汇款的交易
*/
int main(int argc, char *argv[])
{
	catch_all_signals(TransferSrvExit);

	if(0 > OpenLog(argv[0]))
		exit(1);

	dcs_log(0,0, "TransferSrv is starting up...");


	//初始化数据库连接
	if (!TransferDBConect())
	{
		dcs_log(0,0,"Can not open mysql DB !");
		TransferSrvExit(0);
	}

	//加载前置与核心的8583配置文件
	if(IsoLoad8583config(&iso8583conf[0], "iso8583.conf") < 0)
	{
		dcs_log(0, 0, "Load iso8583.conf failed:%s",strerror(errno));
		TransferSrvExit(0);
	}
	
		
	if( CreateMyFolder() < 0 )
		TransferSrvExit(0);

	sem_get("SHA_TRANFERRCV");
	
	dcs_log(0,0,"*************************************************\n"
	        "!!        TransferSrv startup completed.        !!\n"
	        "*************************************************\n");
	DoLoop();

	TransferSrvExit(0);
}

static void TransferSrvExit(int signo)
{
	dcs_log(0,0,"TransferSrvExit catch a signal %d",signo);

	if(signo !=0 && signo != SIGTERM && signo != SIGSEGV)
		return;

	dcs_log(0,0,"TransferSrv terminated.");

	TransferDBConectEnd(0);

	exit(signo);
}

static int OpenLog(char *IDENT)
{
	char logfile[256];

	if(u_fabricatefile("log/TransferSrv.log", logfile, sizeof(logfile)) < 0)
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

	gs_myFid = fold_create_folder("TRANSFERRCV");

	if(gs_myFid < 0)
		gs_myFid = fold_locate_folder("TRANSFERRCV");    

	if(gs_myFid < 0)        
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", "TRANSFERRCV", ise_strerror(errno) );
		return -1;
	} 

	dcs_log(0,0, "folder myFid=%d\n", gs_myFid);

	if(fold_get_maxmsg(gs_myFid) <2)
		fold_set_maxmsg(gs_myFid, 50) ;

	return 0;
}

static int DoLoop()
{
	unsigned char caBuffer[1024];
	int iRead = 0, fromFid = -1;
	int fid = -1;
	fid= fold_open(gs_myFid);
	while(1)
	{
		memset(caBuffer, 0, sizeof(caBuffer));

		iRead = fold_read( gs_myFid, fid, &fromFid, caBuffer, sizeof(caBuffer), 1);
		if(iRead < 0)
		{
			dcs_log(0,0,"fold_read() failed:%s\n",ise_strerror(errno));
			break;
		}

		if(GetTransferMysqlLink()!=0)
		{
			dcs_log(0, 0, "数据库连接失败");
			continue;
		}

		if(memcmp(caBuffer ,"TRNF",4) ==0)
			transferProc(caBuffer, fromFid, gs_myFid, iRead);
		else if(memcmp(caBuffer ,"TFDF",4) ==0)
			dealAsynReturnProc(caBuffer+4, fromFid, gs_myFid, iRead-4);//代付异步处理通知返回受理
		else if(memcmp(caBuffer ,"NDTP",4) ==0)
			nextDayPayProc(caBuffer+4, fromFid, gs_myFid, iRead-4);//次日代付请求受理
		else 
			dcs_debug(caBuffer, iRead, "Unkown Packet,discard !");
	}
    close(fid);
    return -1;
}
