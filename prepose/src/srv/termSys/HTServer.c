#include "ibdcs.h"
#include "tmcibtms.h"

static int gs_myFid = -1;
static void HTServerExit(int signo);
static int OpenLog(char *IDENT);
static int CreateMyFolder();
static int DoLoop();

extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];
struct ISO_8583 iso8583conf[128];
struct ISO_8583 iso8583posconf[128];
/*
	HTServer is a server for trade query. The request maybe from ewp server or other trade accepted server.
*/
int main(int argc, char *argv[])
{
	catch_all_signals(HTServerExit);

	if(0 > OpenLog(argv[0]))
		exit(1);

	dcs_log(0,0, "HTServer is starting up...");

	u_daemonize(NULL);

	//初始化数据库连接
	if (!HTDBConect())
	{
		dcs_log(0,0,"Can not open mysql DB !");
		HTServerExit(0);
	}

	if( CreateMyFolder() < 0 )
		HTServerExit(0);

	dcs_log(0,0,"*************************************************\n"
	        "!!        HTServer startup completed.        !!\n"
	        "*************************************************\n");
	DoLoop();

	HTServerExit(0);
}

static void HTServerExit(int signo)
{
	dcs_log(0,0,"HTServerExit catch a signal %d",signo);

	if(signo !=0 && signo != SIGTERM && signo != SIGSEGV)
		return;

	dcs_log(0,0,"HTServer terminated.");

	HTDBConectEnd(0);

	exit(signo);
}

static int OpenLog(char *IDENT)
{
	char logfile[256];

	if(u_fabricatefile("log/HTServer.log", logfile, sizeof(logfile)) < 0)
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

	gs_myFid = fold_create_folder("HTSERVER");

	if(gs_myFid < 0)
		gs_myFid = fold_locate_folder("HTSERVER");    

	if(gs_myFid < 0)        
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", "HTSERVER", ise_strerror(errno) );
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
	int iRead, fromFid, transrcvFid;
	int fid;
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

		if( iRead < 30 )
		{
			dcs_debug(caBuffer, iRead, "package length less than 30, untrusted package.");
			continue;
		}

		if(GetEwpMysqlLink()!=0)
		{
			dcs_log(0, 0, "数据库连接失败");
			continue;
		}

		if(memcmp(caBuffer ,"EWHT",4) ==0)/*receive from ewp system, there is no tpdu in package.*/
			ewpHTProc(caBuffer+4, fromFid, gs_myFid, iRead-4);
		else if(memcmp(caBuffer ,"H3CP",4) ==0) /*receive from h3c with tpdu, deal in this selection*/
			h3cHTProc(caBuffer+4, fromFid, gs_myFid, iRead-4);
		else 
			dcs_debug(caBuffer,iRead, "Unkown Packet,discard !");
	}
    close(fid);
    return -1;
}
