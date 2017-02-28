#include <errno.h>
#include <time.h>
#include <unistd.h>
#include "trans.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "unixhdrs.h"
#include "ibdcs.h"
#ifdef __macos__
    #include </usr/local/mysql/include/mysql.h>
#elif defined(__linux__)
    #include </usr/include/mysql/mysql.h>
#elif defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include </usr/local/mysql/include/mysql.h>
#endif

#define  ROLLBACK	0
#define  COMMIT		1
#define MAX_QUHAO_LEN 6


struct  _varchar {
	unsigned short len;
	unsigned char  arr[1];
};

typedef struct _varchar VARCHAR;

extern MYSQL *GetFreeMysqlLink();

VARCHAR  vcaUserName;
VARCHAR  vcaPassWord;

// static MYSQL mysql,*sock;    //定义数据库连接的句柄，它被用于几乎所有的MySQL函数
MYSQL mysql, *sock; 


//获取到连接MYSQL数据库的sock
MYSQL *GetMysqlLinkSock()
{
	//多线程时需要从连接池中获取空闲mysql链接
	if(GetIsoMultiThreadFlag())
	{
		return GetFreeMysqlLink();
	}
	else
	{
		//多进程时直接使用全局变量
		return sock;
	}
}


//更新冲正标识，更新pepJnls和tannsfer_info中的冲正标识字段
int updateReversalFlag(char *billmsg, char *trace)
{
	char  sql[1024];
	char i_curdate[8+1];
	char o_curdate[8+1];
	
	struct tm *tm;   
	time_t lastTime;
	
	memset(sql, 0x00, sizeof(sql));
	memset(i_curdate, 0x00, sizeof(i_curdate));
	memset(o_curdate, 0x00, sizeof(o_curdate));
	
	/*昨天*/
    lastTime = time(NULL) - 86400;
	tm = localtime(&lastTime);
	sprintf(o_curdate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	/*今天*/
	time(&lastTime);
	tm = localtime(&lastTime);
	sprintf(i_curdate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	
	//需要从连接池获取连接
  	MYSQL *sock  = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "UPDATE pep_jnls set reversedflag ='1'  where (pepdate= '%s' or pepdate = '%s') and trace=%ld \
	and billmsg ='%s' and msgtype ='0200';",\
	i_curdate, o_curdate, atol(trace), billmsg);
	
	if(mysql_query(sock, sql)) 
	{
        #ifdef __LOG__
	    	dcs_log(0, 0, "%s", sql);
	  	#endif
        dcs_log(0, 0, "updateReversalFlag error [%s]!", mysql_error(sock));
		 //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }	
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "UPDATE transfer_info set reversalflag ='1' where (pepdate= '%s' or pepdate = '%s') and trace=%ld \
	and billmsg ='%s';",\
	i_curdate, o_curdate, atol(trace), billmsg);
	
	if(mysql_query(sock, sql)) 
	{
        #ifdef __LOG__
	    	dcs_log(0, 0, "%s", sql);
	  	#endif
        dcs_log(0, 0, "updateReversalFlag error [%s]!", mysql_error(sock));
		 //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
	//从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
	return 0;
}

/*根据流水号和订单号查询冲正对应的消费信息*/
int getPepJnlsInfo(char *billmsg, char *trace, PEP_JNLS *pep_jnls)
{
	char  sql[1024];
	char i_curdate[8+1];
	char o_curdate[8+1];
	
	struct tm *tm;   
	time_t lastTime;
	
	memset(sql, 0x00, sizeof(sql));
	memset(i_curdate, 0x00, sizeof(i_curdate));
	memset(o_curdate, 0x00, sizeof(o_curdate));
	
	/*昨天*/
    lastTime = time(NULL) - 86400;
	tm = localtime(&lastTime);
	sprintf(o_curdate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	/*今天*/
	time(&lastTime);
	tm = localtime(&lastTime);
	sprintf(i_curdate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);

	//需要从连接池获取连接
  	MYSQL *sock  = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "SELECT aprespn, samid, modeflg, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), tranamt, trndate, postermcde, posmercode, \
	trntime, billmsg, trnsndp, msgtype, termidm, modecde, trancde, translaunchway, AES_DECRYPT(UNHEX(intcdno), 'abcdefgh'), termid, \
	trnsndpid, intcdbank, AES_DECRYPT(UNHEX(intcdname), 'abcdefgh'), card_sn, poscode, poitcde, authcode, syseqno, trace, filed28, \
	mercode, termcde, outcdtype FROM pep_jnls \
	where (pepdate= '%s' or pepdate = '%s') and trace='%ld' and billmsg ='%s' and msgtype ='0200';", i_curdate, o_curdate, atol(trace), billmsg);
	if(mysql_query(sock, sql)) 
	{
	  #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "getPepJnlsInfo 查找原笔交易出错 [%s]!", mysql_error(sock));
		 //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
	    return -1;
	}
	MYSQL_RES *res; 
	MYSQL_ROW row ; 
	int num_fields;
	int num_rows = 0;
    
	if(!(res = mysql_store_result(sock))) {
	    dcs_log(0, 0, "getPepJnlsInfo:Couldn't get result from %s\n", mysql_error(sock));
		 //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
	    return -1;
	}
	num_fields = mysql_num_fields(res);
	row = mysql_fetch_row(res); 
	num_rows = mysql_num_rows(res);
	if (num_rows == 1) 
	{
		memcpy(pep_jnls->aprespn, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls->aprespn));
		memcpy(pep_jnls->samid, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->samid));
		memcpy(pep_jnls->modeflg, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls->modeflg));
		memcpy(pep_jnls->outcdno, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->outcdno));
		memcpy(pep_jnls->tranamt, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls->tranamt));
		memcpy(pep_jnls->trndate, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls->trndate));
		memcpy(pep_jnls->postermcde, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls->postermcde));
		memcpy(pep_jnls->posmercode, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls->posmercode));
		memcpy(pep_jnls->trntime, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls->trntime));
		memcpy(pep_jnls->billmsg, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls->billmsg));
		memcpy(pep_jnls->trnsndp, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls->trnsndp));
		memcpy(pep_jnls->msgtype, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls->msgtype));
		memcpy(pep_jnls->termidm, row[12] ? row[12] : "\0", (int)sizeof(pep_jnls->termidm));
		memcpy(pep_jnls->modecde, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls->modecde));
		memcpy(pep_jnls->trancde, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls->trancde));
		memcpy(pep_jnls->translaunchway, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls->translaunchway));
		memcpy(pep_jnls->intcdno, row[16] ? row[16] : "\0", (int)sizeof(pep_jnls->intcdno));
		memcpy(pep_jnls->termid, row[17] ? row[17] : "\0", (int)sizeof(pep_jnls->termid));
		if(row[18])
			pep_jnls->trnsndpid = atoi(row[18]);
		memcpy(pep_jnls->intcdbank, row[19] ? row[19] : "\0", (int)sizeof(pep_jnls->intcdbank));
		memcpy(pep_jnls->intcdname, row[20] ? row[20] : "\0", (int)sizeof(pep_jnls->intcdname));
		memcpy(pep_jnls->card_sn, row[21] ? row[21] : "\0", (int)sizeof(pep_jnls->card_sn));
		memcpy(pep_jnls->poscode, row[22] ? row[22] : "\0", (int)sizeof(pep_jnls->poscode));
		memcpy(pep_jnls->poitcde, row[23] ? row[23] : "\0", (int)sizeof(pep_jnls->poitcde));
		memcpy(pep_jnls->authcode, row[24] ? row[24] : "\0", (int)sizeof(pep_jnls->authcode));
		memcpy(pep_jnls->syseqno, row[25] ? row[25] : "\0", (int)sizeof(pep_jnls->syseqno));
		if(row[26])
			pep_jnls->trace = atol(row[26]);
		memcpy(pep_jnls->filed28, row[27] ? row[27] : "\0", (int)sizeof(pep_jnls->filed28));
		memcpy(pep_jnls->mercode, row[28] ? row[28] : "\0", (int)sizeof(pep_jnls->mercode));
		memcpy(pep_jnls->termcde, row[29] ? row[29] : "\0", (int)sizeof(pep_jnls->termcde));
		if(row[30])
			pep_jnls->outcdtype = row[30][0];
	}
	else if(num_rows == 0)
	{
	#ifdef __LOG__
		dcs_log(0, 0, "%s", sql);
	#endif
		dcs_log(0, 0, "getPepJnlsInfo没有找到符合条件的记录");
		mysql_free_result(res); 
		 //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return -1;
	}
	else if(num_rows >1)
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
		dcs_log(0, 0, "getPepJnlsInfo查到多条符合条件的记录");
		mysql_free_result(res);
		 //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return -1;
	}
	mysql_free_result(res);
	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls->aprespn =[%s]", pep_jnls->aprespn);
		dcs_log(0, 0, "pep_jnls->samid=[%s]", pep_jnls->samid);
		dcs_log(0, 0, "pep_jnls->modeflg=[%s]", pep_jnls->modeflg);
		dcs_log(0, 0, "pep_jnls->outcdno=[%s]", pep_jnls->outcdno);
		dcs_log(0, 0, "pep_jnls->tranamt=[%s]", pep_jnls->tranamt);
		dcs_log(0, 0, "pep_jnls->trndate=[%s]", pep_jnls->trndate);
		dcs_log(0, 0, "pep_jnls->postermcde=[%s]", pep_jnls->postermcde);
		dcs_log(0, 0, "pep_jnls->posmercode=[%s]", pep_jnls->posmercode);
		dcs_log(0, 0, "pep_jnls->trntime=[%s]", pep_jnls->trntime);
		dcs_log(0, 0, "pep_jnls->billmsg=[%s]", pep_jnls->billmsg);
		dcs_log(0, 0, "pep_jnls->trnsndp=[%s]", pep_jnls->trnsndp);
		dcs_log(0, 0, "pep_jnls->msgtype=[%s]", pep_jnls->msgtype);
		dcs_log(0, 0, "pep_jnls->termidm=[%s]", pep_jnls->termidm);
		dcs_log(0, 0, "pep_jnls->modecde=[%s]", pep_jnls->modecde);
		dcs_log(0, 0, "pep_jnls->trancde=[%s]", pep_jnls->trancde);
		dcs_log(0, 0, "pep_jnls->translaunchway=[%s]", pep_jnls->translaunchway);
		dcs_log(0, 0, "pep_jnls->intcdno=[%s]", pep_jnls->intcdno);
		dcs_log(0, 0, "pep_jnls->termid=[%s]", pep_jnls->termid);
		dcs_log(0, 0, "pep_jnls->trnsndpid=[%d]", pep_jnls->trnsndpid);
		dcs_log(0, 0, "pep_jnls->intcdbank=[%s]", pep_jnls->intcdbank);
		dcs_log(0, 0, "pep_jnls->intcdname=[%s]", pep_jnls->intcdname);
		dcs_log(0, 0, "pep_jnls->card_sn=[%s]", pep_jnls->card_sn);
		dcs_log(0, 0, "pep_jnls->poscode=[%s]", pep_jnls->poscode);
		dcs_log(0, 0, "pep_jnls->poitcde=[%s]", pep_jnls->poitcde);
		dcs_log(0, 0, "pep_jnls->authcode=[%s]", pep_jnls->authcode);
		dcs_log(0, 0, "pep_jnls->syseqno=[%s]", pep_jnls->syseqno);
		dcs_log(0, 0, "pep_jnls->trace=[%ld]", pep_jnls->trace);
		dcs_log(0, 0, "pep_jnls->filed28=[%s]", pep_jnls->filed28);
		dcs_log(0, 0, "pep_jnls->mercode=[%s]", pep_jnls->mercode);
		dcs_log(0, 0, "pep_jnls->termcde=[%s]", pep_jnls->termcde);
		dcs_log(0, 0, "pep_jnls->outcdtype=[%c]", pep_jnls->outcdtype);
#endif
	
	//从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
	return 0;
}

/*保存冲正信息*/
int SaveRevInfo(ISO_data iso, PEP_JNLS pep_jnls)
{
	char sql[2048];
	char tmpbuf[200], t_year[5];
	int	s = 0;
	struct tm *tm;
	time_t t;
    
	memset(sql, 0x00, sizeof(sql));
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(t_year, 0x00, sizeof(t_year));
	
    time(&t);
    tm = localtime(&t);
    
    sprintf(t_year, "%04d", tm->tm_year+1900);
    
	memcpy(pep_jnls.mmchgid, "0", 1);
	memcpy(pep_jnls.revodid, "1", 1);
	pep_jnls.mmchgnm = 0;
    
	if(getbit(&iso, 0, (unsigned char *)pep_jnls.msgtype)<0)
	{
        	dcs_log(0, 0, "can not get bit_0!");
        	return -1;
	}
    
	if(getbit(&iso, 7, (unsigned char *)tmpbuf)<0)
	{
        	dcs_log(0, 0, "can not get bit_7!");
        	return -1;
	}
	sprintf(pep_jnls.pepdate, "%4.4s%4.4s", t_year, tmpbuf);
	memcpy(pep_jnls.peptime, tmpbuf+4, 6);
    
	/*发送机构代码 sendcde[8]*/
	memcpy(pep_jnls.sendcde, "08000000", 8);
    
	s = getbit(&iso, 39, (unsigned char *)pep_jnls.aprespn);
	if(s<= 0)
		dcs_log(0, 0, "bit_39 is null!");
	else
		memcpy(pep_jnls.revodid, "2", 1); 
	memcpy(pep_jnls.process, "000000", 6);

	//todo .. 冲正交易码暂定为C00
	memcpy(pep_jnls.trancde,"C00",3);
		
#ifdef __LOG__
	dcs_log(0, 0, "pep_jnls.pepdate=[%s], pep_jnls.peptime=[%s]", pep_jnls.pepdate, pep_jnls.peptime);
	dcs_log(0, 0, "pep_jnls.trancde=[%s], pep_jnls.msgtype=[%s]", pep_jnls.trancde, pep_jnls.msgtype);
	//dcs_log(0, 0, "pep_jnls.outcdno=[%s], pep_jnls.intcdno=[%s]", pep_jnls.outcdno, pep_jnls.intcdno);
	//dcs_log(0, 0, "pep_jnls.intcdbank=[%s], pep_jnls.intcdname=[%s]", pep_jnls.intcdbank, pep_jnls.intcdname);
	dcs_log(0, 0, "pep_jnls.process=[%s], pep_jnls.tranamt=[%s]", pep_jnls.process, pep_jnls.tranamt);
	dcs_log(0, 0, "pep_jnls.trace=[%d], pep_jnls.termtrc=[%s]", pep_jnls.trace, pep_jnls.termtrc);
	dcs_log(0, 0, "pep_jnls.trntime=[%s], pep_jnls.trndate=[%s]", pep_jnls.trntime, pep_jnls.trndate);
	dcs_log(0, 0, "pep_jnls.poitcde=[%s], pep_jnls.sendcde=[%s]", pep_jnls.poitcde, pep_jnls.sendcde);
	dcs_log(0, 0, "pep_jnls.termcde=[%s], pep_jnls.mercode=[%s]", pep_jnls.termcde, pep_jnls.mercode);
	dcs_log(0, 0, "pep_jnls.billmsg=[%s], pep_jnls.samid=[%s]", pep_jnls.billmsg, pep_jnls.samid);
	dcs_log(0, 0, "pep_jnls.sdrespn=[%s], pep_jnls.revodid=[%s]", pep_jnls.sdrespn, pep_jnls.revodid);
	dcs_log(0, 0, "pep_jnls.billflg=[%s], pep_jnls.mmchgid=[%s]", pep_jnls.billflg, pep_jnls.mmchgid);
	dcs_log(0, 0, "pep_jnls.mmchgnm=[%d], pep_jnls.trnsndp=[%s]", pep_jnls.mmchgnm, pep_jnls.trnsndp);
	dcs_log(0, 0, "pep_jnls.trnmsgd=[%s], pep_jnls.revdate=[%s]", pep_jnls.trnmsgd, pep_jnls.revdate);
	dcs_log(0, 0, "pep_jnls.modeflg=[%s], pep_jnls.modecde=[%s]", pep_jnls.modeflg, pep_jnls.modecde);
	dcs_log(0, 0, "pep_jnls.filed28=[%s], pep_jnls.filed48=[%s]", pep_jnls.filed28, pep_jnls.filed48);
	dcs_log(0, 0, "pep_jnls.aprespn=[%s]", pep_jnls.aprespn);
	dcs_log(0, 0, "pep_jnls.translaunchway=[%s]", pep_jnls.translaunchway);
	dcs_log(0, 0, "pep_jnls.outcdtype=[%c]", pep_jnls.outcdtype);
	dcs_log(0, 0, "pep_jnls.trnsndpid=[%d]", pep_jnls.trnsndpid);
	dcs_log(0, 0, "pep_jnls.posmercode=[%s], pep_jnls.postermcde=[%s]", pep_jnls.posmercode, pep_jnls.postermcde);
#endif
	
	//需要从连接池获取连接
  	MYSQL *sock  = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "INSERT INTO pep_jnls(PEPDATE, PEPTIME, TRANCDE, MSGTYPE, OUTCDNO, INTCDNO, INTCDBANK,\
	INTCDNAME, PROCESS, TRANAMT, TRACE, TERMTRC, TRNTIME, TRNDATE, POITCDE, SENDCDE, TERMCDE, MERCODE, BILLMSG,\
	SAMID, TERMID, TERMIDM, SDRESPN, REVODID, BILLFLG, MMCHGID, MMCHGNM, APRESPN, FILED48, TRNSNDP, TRNMSGD, REVDATE,\
	MODEFLG, MODECDE, FILED28, MERID, DISCOUNTAMT, POSCODE, TRANSLAUNCHWAY, CAMTLIMIT, DAMTLIMIT, OUTCDTYPE, TRNSNDPID,\
	TERMMERAMTLIMIT, POSMERCODE, POSTERMCDE,CARD_SN)VALUES('%s', '%s', '%s', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', \
	HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%s', %ld, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', \
	'%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', '%s', '%s', \
	'%s', '%s', '%s', %d, '%s', '%s', '%s', %d, %d, '%c', %d, \
	%d,'%s', '%s', '%s');", pep_jnls.pepdate, pep_jnls.peptime, pep_jnls.trancde, pep_jnls.msgtype, pep_jnls.outcdno,\
	pep_jnls.intcdno, pep_jnls.intcdbank, pep_jnls.intcdname, pep_jnls.process, pep_jnls.tranamt, pep_jnls.trace,\
		pep_jnls.termtrc, pep_jnls.trntime, pep_jnls.trndate, pep_jnls.poitcde, pep_jnls.sendcde, pep_jnls.termcde,\
		pep_jnls.mercode, pep_jnls.billmsg, pep_jnls.samid, pep_jnls.termid,pep_jnls.termidm, pep_jnls.sdrespn, pep_jnls.revodid,\
		pep_jnls.billflg, pep_jnls.mmchgid, pep_jnls.mmchgnm, pep_jnls.aprespn, pep_jnls.filed48, pep_jnls.trnsndp, pep_jnls.trnmsgd,\
		pep_jnls.revdate, pep_jnls.modeflg, pep_jnls.modecde, pep_jnls.filed28, pep_jnls.merid, pep_jnls.discountamt,\
		pep_jnls.poscode, pep_jnls.translaunchway, pep_jnls.camtlimit, pep_jnls.damtlimit, pep_jnls.outcdtype, pep_jnls.trnsndpid,\
		pep_jnls.termmeramtlimit, pep_jnls.posmercode, pep_jnls.postermcde, pep_jnls.card_sn);
	  
	    if(mysql_query(sock, sql)) 
		{
	  #ifdef __LOG__
			dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "SaveRevInfo error [%s]!", mysql_error(sock));
			if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
	        return -1;
	    }
	    dcs_log(0, 0, "SaveRevInfo success");
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return 1;
	
}

/*mysql断开重连的处理*/
int GetTmOutMysqlLink()
{	
	int i = 0;
	for(i = 0; i<7; i++)
	{
		#ifdef __TEST__
			dcs_log(0, 0, "%d ci GetTmOutMysqlLink", i);
		#endif
		if(mysql_ping(sock)!=0)
			continue;
		else
			break;
	}
	#ifdef __TEST__
		dcs_log(0, 0, "test GetTmOutMysqlLink");
	#endif
	return mysql_ping(sock);
}

