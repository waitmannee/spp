#include "ibdcs.h"
#include "tmcibtms.h"
#include "iso8583.h"

static int gs_myFid    = -1;
static char *g_pcBcdaShmPtr;
static void MoniPospExit(int signo);
static int OpenLog(char *IDENT);
static int CreateMyFolder();
static int DoLoop();


extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];
struct ISO_8583 iso8583conf[128];
struct ISO_8583 iso8583posconf[128];
int main(int argc, char *argv[])
{
	char szTmp[20+1];
	catch_all_signals(MoniPospExit);

	if(0 > OpenLog(argv[0]))
		exit(1);

	dcs_log(0,0, "MoniPosp Servers is starting up...");

	u_daemonize(NULL);

	if(IsoLoad8583config(&iso8583conf[0],"iso8583.conf") < 0)
	{
		dcs_log(0,0,"IsoLoad8583config(iso8583.conf) failed:%s", strerror(errno));
		MoniPospExit(0);
	}
	if(IsoLoad8583config(&iso8583posconf[0],"iso8583_pos.conf") < 0)
	{
		dcs_log(0,0,"IsoLoad8583config(iso8583_pos.conf) failed:%s", strerror(errno));
		MoniPospExit(0);
	}

	if( ICSLoadTerconfig() < 0)
	{
		dcs_log(0,0,"ICSLoadTerconfig() failed:%s",strerror(errno));
		MoniPospExit(0);
	}

	if( CreateMyFolder() < 0 )
		MoniPospExit(0);

	dcs_log(0,0,"*************************************************\n"
	"!!        MoniPos servers startup completed.        !!\n"
	"*************************************************\n");

	//create 自己的接收队列
	memset(szTmp,0,sizeof(szTmp));	
	sprintf(szTmp,"%d",getpid());
	if(queue_create(szTmp)<0)
		dcs_log(0,0,"create queue %s err,errno=%d",szTmp,errno);

	DoLoop();

	MoniPospExit(0);
}

static void MoniPospExit(int signo)
{
	dcs_log(0,0,"catch a signal %d",signo);
	    
	if(signo !=0 && signo != SIGTERM && signo != SIGSEGV)
	    return;

	dcs_log(0,0,"MoniPosp terminated.");

	exit(signo);
}

static int OpenLog(char *IDENT)
{
	char logfile[256];

	if(u_fabricatefile("log/MoniPosp.log",logfile,sizeof(logfile)) < 0)
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

	gs_myFid = fold_create_folder("MONIPOSP");

	if(gs_myFid < 0)
		gs_myFid = fold_locate_folder("MONIPOSP");    

	if(gs_myFid < 0)        
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", "MONIPOSP", ise_strerror(errno) );
		return -1;
	} 

	dcs_log(0,0, "folder myFid=%d\n", gs_myFid);

	if(fold_get_maxmsg(gs_myFid) <2)
		fold_set_maxmsg(gs_myFid, 20) ;

	return 0;
}

static int DoLoop()
{
	char caBuffer[1024];
	int iRead, fromFid;
	int fid;
	fid = fold_open(gs_myFid);
	while(1)
	{
		memset(caBuffer, 0, sizeof(caBuffer));

		iRead = fold_read( gs_myFid, fid, &fromFid, caBuffer, sizeof(caBuffer), 1);
		if(iRead < 0)
		{
			dcs_log(0,0,"fold_read() failed:%s\n",ise_strerror(errno));
			break;
		}

		if(( iRead < 33 )&& ( memcmp( caBuffer,"TIME",4) != 0))
			continue;

		dcs_debug(caBuffer,iRead,"recvice from my folder myid=%d, fromid=%d, len =[%d]",gs_myFid, fromFid, iRead);


		if(memcmp(caBuffer ,"POSP",4) ==0) /*模拟posp*/
			moni_posp(caBuffer+4, fromFid, gs_myFid, iRead-4);
		else  if(memcmp(caBuffer ,"CUPS",4) ==0) /*模拟cups*/
			moni_cups(caBuffer+4, fromFid, gs_myFid, iRead-4);
		else
			dcs_debug(caBuffer,iRead,"Unkown Packet,discard !");
	}
	close(fid);
	return -1;
}

int ICSLoadTerconfig()
{
	return 1;
}
