#include "ibdcs.h"
#include "tmcibtms.h"
#include "sms.h"

static int gs_myFid = -1;
static void SMSRcvExit(int signo);
static int OpenLog(char *IDENT);
static int CreateMyFolder();
static int DoLoop();


int main(int argc, char *argv[])
{
	catch_all_signals(SMSRcvExit);

	if(0 > OpenLog(argv[0]))
		exit(1);

	dcs_log(0,0, "SMSRcv is starting up...");

	u_daemonize(NULL);
		
	//初始化数据库连接
	if (!SMSDBConect())
	{
		dcs_log(0,0,"Can not open mysql DB !");
		SMSRcvExit(0);
	}

	if( CreateMyFolder() < 0 )
		SMSRcvExit(0);

	dcs_log(0,0,"*************************************************\n"
	        "!!        SMSRcv startup completed.        !!\n"
	        "*************************************************\n");
	DoLoop();

	SMSRcvExit(0);
}

static void SMSRcvExit(int signo)
{
    dcs_log(0,0,"SMSRcvExit catch a signal %d",signo);
        
    if(signo !=0 && signo != SIGTERM && signo != SIGSEGV)
        return;

    dcs_log(0,0,"SMSRcvExit terminated.");
    
    SMSDBConectEnd(0);
    
    exit(signo);
}

static int OpenLog(char *IDENT)
{
	char logfile[256];
	memset(logfile, 0, sizeof(logfile));
	if(u_fabricatefile("log/SMSRcv.log", logfile, sizeof(logfile)) < 0)
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

	gs_myFid = fold_create_folder("SMSRCV");

	if(gs_myFid < 0)
		gs_myFid = fold_locate_folder("SMSRCV");    

	if(gs_myFid < 0)        
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", "SMSRCV", ise_strerror(errno) );
		return -1;
	} 

	dcs_log(0,0, "folder myFid=%d\n", gs_myFid);

	if(fold_get_maxmsg(gs_myFid) <2)
		fold_set_maxmsg(gs_myFid, 50) ;

	return 0;
}

/*
 caBuffer formatter:trancde|outcard|incard|tradeamt|tradefee|tradedate|traderesp|tradetrace|traderefer|tradebillmsg|userphone
*/
static int DoLoop()
{
	char caBuffer[256];
	int iRead, fromFid, slen, result;
	int fid;
	fid=fold_open(gs_myFid);
	while(1)
	{
		memset(caBuffer, 0, sizeof(caBuffer));

		iRead = fold_read( gs_myFid,fid, &fromFid, caBuffer, sizeof(caBuffer), 1);
		if(iRead < 0)
		{
			dcs_log(0,0,"fold_read() failed:%s\n",ise_strerror(errno));
			break;
		}

		if( iRead < 10 )
		{
			dcs_debug(caBuffer, iRead, "package length less than 10, untrusted package.");
			continue;
		}
		#ifdef __LOG__
		dcs_debug(caBuffer,iRead, "recvice from my folder myid=%d, fromid=%d, len =[%d]",gs_myFid, fromFid, iRead);
		#else
			dcs_log(0, 0, "recvice from my folder myid=%d, fromid=%d, len =[%d]",gs_myFid, fromFid, iRead);
		#endif

		if(GetSmsMysqlLink()!=0)
		{
			dcs_log(0, 0, "数据库连接失败");
			continue;
		}
		if(memcmp(caBuffer ,"SMSC",4) ==0)
		{
			//receive sms send result from sms system,update db 
			dcs_log(caBuffer+4, iRead-4, "send sms result: ");
			result = updateSmsResult(caBuffer+4);
			if(result == 1)
				dcs_log(0, 0, "更新sms_jnls成功。");
			else
				dcs_log(0, 0, "更新sms_jnls失败。");
		}
		else
		{
			fromFid = fold_locate_folder("TMSS");
			if(fromFid < 0)
			{
				//TODO ... Save trade to db for resend next time
				dcs_log(0, 0, "fold_locate_folder TMS Error.");
			}else
			{
				result = sms_save(caBuffer);
				if(result == 1)
				{
					dcs_log(0, 0, "保存sms_jnls成功。");
					slen = fold_write(fromFid, -1, caBuffer, iRead);
					if(slen < 0 )
					{
						//TODO ... Save trade to db for resend next time
						dcs_log(0, 0, "写入fold error");
					}
					else
						dcs_log(0, 0, "write folder success");
				}
				else
				{
					dcs_log(0, 0, "保存sms_jnls失败。");
				}
			}
		}
	}
	close(fid);
	return -1;
}

