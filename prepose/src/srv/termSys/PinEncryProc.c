/*
模块说明：
		此模块主要处理接收ewp发送过来的基于DUKPT算法加密的密码进行转加密操作
*     add by  wuzf  at 2015/5/9
*/
#include "ibdcs.h"
#include "pub_function.h"
static int gs_myFid    = -1;

extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];
struct ISO_8583 iso8583conf[128];
struct ISO_8583 iso8583posconf[128];

static int OpenLog(char *IDENT);
static int CreateMyFolder();
static int DoLoop();
static void SvrExit(int signo);

int main(int argc, char *argv[])
{
	catch_all_signals(SvrExit);

	if(0 > OpenLog(argv[0]))
		exit(1);

	dcs_log(0, 0, "PinEncrySrv  is starting up...");

	//初始化数据库连接
	if (!DssConect())
	{
		dcs_log(0, 0, "Can not open mysql DB !");
		SvrExit(0);
	}
	
	if( CreateMyFolder() < 0 )
		SvrExit(0);

	dcs_log(0, 0, "*************************************************\n"
				  "!!PinEncrySrv 启动成功 !!\n"
				  "*************************************************\n");	
	DoLoop();

	SvrExit(0);
}

static void SvrExit(int signo)
{
	dcs_log(0, 0, "catch a signal %d", signo);
	    
	if(signo  == 15)
	    dcs_log(0, 0, "脚本停服务");
	else if(signo == 11)
	{
		dcs_log(0, 0, "内存溢出");
	}

	dcs_log(0, 0, "PinEncrySrv exit.");

	DssEnd(0);
	    
	exit(signo);
}

static int OpenLog(char *IDENT)
{
	char logfile[256];
	memset(logfile, 0, sizeof(logfile));
	
	if(u_fabricatefile("log/PinEncrySrv.log", logfile, sizeof(logfile)) < 0)
		return -1;

	return dcs_log_open(logfile, IDENT);
}

static int CreateMyFolder()
{
	if(fold_initsys() < 0)
	{
		dcs_log(0, 0, "cannot attach to folder system!");
		return -1;
	}

	gs_myFid = fold_create_folder("PINENC");

	if(gs_myFid < 0)
		gs_myFid = fold_locate_folder("PINENC");    

	if(gs_myFid < 0)        
	{
		dcs_log(0, 0, "cannot create folder '%s':%s\n", "PINENC", ise_strerror(errno) );
		return -1;
	} 

	dcs_log(0, 0, "folder myFid=%d\n", gs_myFid);
#ifdef __TEST__
	if(fold_get_maxmsg(gs_myFid) < 100)
		fold_set_maxmsg(gs_myFid, 500) ;
#else
	if(fold_get_maxmsg(gs_myFid) < 2)
		fold_set_maxmsg(gs_myFid, 20) ;
#endif
	return 0;
}

void PinEncryProc(char *srcBuf, int iFid, int iMid, int iLen)
{
#ifdef __LOG__
	dcs_debug(srcBuf, iLen, "mposPinConver Proc, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
#else
	dcs_log(0, 0,"mposPinConver Proc, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
#endif
	//变量定义
	int rtn = 0;
		
	char data[16+1];
		
	//KSN结构体
	KSN_INFO ksn_info;

	//数据长度
	int len=0; 

	char tpdu[5+1];
	char cardno[19+1];
	char tmpbuf[10+1]; 
	char pinbuf[16+1];
	//返回码
	char code[2+1];
	
	//初始化变量
	memset(data, 0, sizeof(data));
	memset(&ksn_info, 0x00, sizeof(KSN_INFO));
	memset(tpdu, 0, sizeof(tpdu));
	memset(cardno, 0, sizeof(cardno));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(pinbuf, 0, sizeof(pinbuf));
	memset(code, 0, sizeof(code));
	
	memcpy(tpdu, srcBuf, 5 );
	len= len + 5;
	
	dcs_log(0, 0, "解析ewp 上送的pin转加密报文");

	//首先判断报文长度是否正确
	// 5字节TPDU(前置底层加的),20 字节ksn,19字节卡号(不足右补空格),16字节密码密文
	if(iLen  != 5 + 20 + 19 +16)
	{
		dcs_log(0, 0, "ewp 报文长度错误len=%d", iLen -5);
		memcpy(code, RET_CODE_F1,2);
		ReturnEwp(tpdu, iFid, data, code);
		return;
	}
	//ksn
	memcpy(ksn_info.ksn, srcBuf+len, 20);
	len= len + 20;
	dcs_log(0, 0, "ksn 为:%s", ksn_info.ksn);

	//卡号
	memset(cardno, 0x00, sizeof(cardno));
	memcpy(cardno, srcBuf+len, 19);
	r_trim(cardno);
	len= len + 19;
//	dcs_log(0, 0, "卡号:%s", cardno);
	
	//密码密文
	memset(pinbuf, 0x00, sizeof(pinbuf));
	memcpy(pinbuf, srcBuf+len,16);
	len= len + 16;
//	dcs_log(0, 0, "密码密文:%s", pinbuf);
	
	 
	//将送上来的20字节ksn号转换为15字节的ksn初始值,去除counter值
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	asc_to_bcd(tmpbuf, ksn_info.ksn, 20, 0);
	tmpbuf[7] &= 0xE0;
	tmpbuf[8] = 0x00;
	tmpbuf[9] = 0x00;
	
	memcpy(ksn_info.initksn,tmpbuf,5);
	bcd_to_asc(ksn_info.initksn+5, tmpbuf+5, 10, 0);
	
	rtn= GetMposInfoByKsn(&ksn_info);
	if(rtn < 0)
	{
		dcs_log(0, 0, "查询ksn_info失败,ksn未找到, ksn=%s", ksn_info.initksn);
		memcpy(code, RET_CODE_FC,2);
		ReturnEwp(tpdu, iFid, data, code);
		return;
	}
	rtn= MakeDukptKey(&ksn_info);
	if(rtn < 0)
	{
		dcs_log(0, 0, "MakeDukptKey失败");
		memcpy(code, RET_CODE_F7,2);
		ReturnEwp(tpdu, iFid, data, code);
		return;
	}

	//转加密密码
	memset(tmpbuf, 0, sizeof(tmpbuf));
	rtn =  MposPinConver(ksn_info.tpk, pinbuf, cardno, tmpbuf);
	if(rtn < 0)
	{
		dcs_log(0, 0,"转加密密码错误, error code=%d", rtn);
		memcpy(code, RET_CODE_F7,2);
		ReturnEwp(tpdu, iFid, data, code);
		return; 
	}
	memcpy(code, "00",2);
	bcd_to_asc(data, tmpbuf,16, 0);
	
	//发送报文
	rtn=ReturnEwp(tpdu, iFid, data, code);
	if(rtn == -1)
	{
		dcs_log(0, 0, "返回信息给ewp系统失败");
		return;
	}
	dcs_log(0, 0, "返回信息给ewp系统成功");
	return;	
}


/*
组返回ewp的报文
code = 0 返回成功处理
code !=0 返回失败应答
*/
int ReturnEwp(char *tpdu, int trnsndpid, char *buffer, char *code)
{
	int gs_comfid = -1, s = 0;
	char buffer_send[100];
	
	memset(buffer_send, 0, sizeof(buffer_send));

	s = strlen(buffer);
	gs_comfid = trnsndpid;
	
	#ifdef __TEST__
    if(fold_get_maxmsg(gs_comfid) < 100)
		fold_set_maxmsg(gs_comfid, 500) ;
 	#else
	if(fold_get_maxmsg(gs_comfid) < 2)
		fold_set_maxmsg(gs_comfid, 20) ;
    #endif

	memcpy(buffer_send, tpdu, 1);
	memcpy(buffer_send+1, tpdu+3, 2);
	memcpy(buffer_send+3, tpdu+1, 2);
	
	if(memcmp(code , "00", 2) == 0)
	{
		memcpy(buffer_send+5, code, 2);
		memcpy(buffer_send+7, buffer, s);
		#ifdef __LOG__
			dcs_debug(buffer_send, s+7, "data_buf len=%d, foldId=%d", s+7, gs_comfid);
		#endif

		fold_write(gs_comfid, -1, buffer_send, s+7);
	}
	else 
	{
		memcpy(buffer_send+5, code, 2);
		#ifdef __LOG__
		dcs_debug(buffer_send, s+7, "data_buf len=%d, foldId=%d", s+7, gs_comfid);
		#endif

		fold_write(gs_comfid, -1, buffer_send, 7);
	}
	return 0;
}


static int DoLoop()
{
	char caBuffer[PUB_MAX_SLEN+4+1];
	int iRead = 0, fromFid = 0;
	int fid = 0;
	fid = fold_open(gs_myFid);
	while(1)
	{ 
		memset(caBuffer, 0, sizeof(caBuffer));
		iRead = fold_read(gs_myFid, fid, &fromFid, caBuffer, sizeof(caBuffer), 1);
		if(iRead < 0) 
		{
			dcs_log(0, 0, "fold_read()failed:%s\n", ise_strerror(errno));
			break;
		}
		
		if(GetMysqlLink()!=0)
		{
			dcs_log(0, 0, "数据库连接失败");
			continue;
		}
		
		if(memcmp(caBuffer, "PINE",4) ==0)
			PinEncryProc(caBuffer+4, fromFid, gs_myFid, iRead-4);
		else
			dcs_debug(caBuffer, iRead, "Unkown Packet,discard !");
	}
	close(fid);
	return -1;
}

