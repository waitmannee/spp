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

VARCHAR  vcaUserName;
VARCHAR  vcaPassWord;

// static MYSQL mysql,*sock;    //定义数据库连接的句柄，它被用于几乎所有的MySQL函数
MYSQL mysql, *sock; 

int TransferDBConect()
{ 
	mysql_init(&mysql);
	char value = 1;
	my_bool secure_auth =0;
	mysql_options(&mysql, MYSQL_OPT_RECONNECT, (char *)&value);
	mysql_options(&mysql, MYSQL_SECURE_AUTH, (my_bool *)&secure_auth);
	if(!(sock = mysql_real_connect(&mysql, (char *)getenv("SERVER"), (char *)getenv("DBUSR"), (char *)getenv("DBPWD"), (char *)getenv("DBNAME"), 0, NULL, 0))) {
		dcs_log(0, 0, "host%s,user:%s,ps:%s\n\n", (char *)getenv("SERVER"),(char *)getenv("DBUSR"),(char *)getenv("DBPWD"));
		dcs_log(0, 0, "Couldn't connect to DB!\n%s\n\n", mysql_error(&mysql));
		return 0 ;
	}
	
	if(mysql_set_character_set(&mysql, "utf8")){
		fprintf(stderr, "错误, %s\n", mysql_error(&mysql));
		dcs_log(0, 0, "错误, %s\n", mysql_error(& mysql));
	}

	if(sock){
		dcs_log(0, 0, "数据库连接成功,name:%s", getenv("DBNAME"));
		return 1;
	} else{
		dcs_log(0, 0, "数据库连接失败,name:%s", getenv("DBNAME"));
		return 0;
	}
	return 1;
}

int TransferDBConectEnd(int iDssOperate)
{
	switch (iDssOperate)
	{
	    case ROLLBACK:
            mysql_close(sock);
			return -1 ;
		case COMMIT:
            mysql_close(sock);
			break;
		default :
			dcs_log(0, 0, "<DSS>Invalid arguments: %d", iDssOperate );
			return -1 ;
	}
	return 0;
}

/*
 输入参数：
 cardno：银行卡号
 输出参数：
 bank_name：银行名称

 返回参数：
 'C':信用卡
 'D':借记卡
 '-'：不存在
 */
char GetCardInfo(char* cardno, char *bank_name)
{
	char ctype;
	char cdhd5[5+1];
	char cdhd6[6+1];
	char cdhd7[7+1];
	char cdhd8[8+1];
	char cdhd9[9+1];
	char cdhd10[10+1];
	char cdhd11[11+1];
	char cdhd12[12+1];
	char i_card_ins_desc[64+1];
	char i_card_desc[64+1];
	int  len ;
    
	memset(cdhd5, 0, sizeof(cdhd5));
    memset(cdhd6, 0, sizeof(cdhd6));
    memset(cdhd7, 0, sizeof(cdhd7));
    memset(cdhd8, 0, sizeof(cdhd8));
    memset(cdhd9, 0, sizeof(cdhd9));
    memset(cdhd10, 0, sizeof(cdhd10));
    memset(cdhd11, 0, sizeof(cdhd11));
    memset(cdhd12, 0, sizeof(cdhd12));
	memset(i_card_ins_desc, 0, sizeof(i_card_ins_desc));
	memset(i_card_desc, 0, sizeof(i_card_desc));
    
	memcpy(cdhd5, cardno, 5);
    memcpy(cdhd6, cardno, 6);
    memcpy(cdhd7, cardno, 7);
    memcpy(cdhd8, cardno, 8);
    memcpy(cdhd9, cardno, 9);
    memcpy(cdhd10, cardno, 10);
    memcpy(cdhd11, cardno, 11);
    memcpy(cdhd12, cardno, 12);
    
    len = getstrlen(cardno);
    
    char sql[512];
    memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "SELECT card_type, card_ins_desc, card_desc FROM card_transfer where card_head IN('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s') \
	and  card_no_length = %d order by card_length desc limit 1;", cdhd5, cdhd6, cdhd7, cdhd8, cdhd9, cdhd10, cdhd11, cdhd12, len);

    if(mysql_query(sock, sql)) {
      #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetCardInfo fail [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetCardInfo : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    
	num_rows = mysql_num_rows(res);
    if (num_rows == 1) {
#ifdef __LOG__
    	dcs_log(0, 0, "GetCardInfo ======= %s", row[0]);
#endif
        ctype = row[0][0];
		memcpy(i_card_ins_desc, row[1] ? row[1] : "\0", (int)sizeof(i_card_ins_desc));
		memcpy(i_card_desc, row[2] ? row[2] : "\0", (int)sizeof(i_card_desc));
    } 
	else if(num_rows == 0)
	{
		dcs_log(0, 0, "GetCardInfo：没有找到符合条件的记录");
		mysql_free_result(res);  
		return '-';
	}
	else if(num_rows >1)
	{
		dcs_log(0, 0, "GetCardInfo：查到多条符合条件的记录");
		mysql_free_result(res);  
		return '-';
	}
    mysql_free_result(res);
	sprintf(bank_name, "%s%s", i_card_ins_desc, i_card_desc);
  
    if(ctype == 'C')
		return 'C';
    else if(ctype == 'D')
		return 'D';
    else
    	return '-';
}

/*
 pepdate 同报文第7域,peptime 同报文第7域
	函数功能：保存转账汇款交易的扣款交易到pep_jnls中
 */
int SavePepjnls(ISO_data iso, EWP_INFO ewp_info, PEP_JNLS pep_jnls)
{
	char sql[2048];
	char tmpbuf[200], t_year[5];
	char escape_buf55[512+1];
	long  transfee = 0;
	int	s = 0;
	struct tm *tm;
	time_t t;
    
	memset(sql, 0x00, sizeof(sql));
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(t_year, 0x00, sizeof(t_year));
	memset(escape_buf55, 0x00, sizeof(escape_buf55));
	
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
	memcpy(pep_jnls.trancde, ewp_info.consumer_transcode, 3);
	
	if(getbit(&iso, 3, (unsigned char *)pep_jnls.process)<0)
	{
        dcs_log(0, 0, "can not get bit_3!");
        return -1;
	}
    
	getbit(&iso, 2, (unsigned char *)pep_jnls.outcdno);
	getbit(&iso, 4, (unsigned char *)pep_jnls.tranamt);
	
	/*termid 存手机号*/
	memcpy(pep_jnls.termid, ewp_info.consumer_username, 11);
	memcpy(pep_jnls.intcdno, ewp_info.incardno, getstrlen(ewp_info.incardno));
    
	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(getbit(&iso, 11, (unsigned char *)tmpbuf)<0)
	{
        dcs_log(0, 0, "can not get bit_3!");
        return -1;
	}
    
	sscanf(tmpbuf, "%06ld", &pep_jnls.trace);
	
	memcpy(pep_jnls.trntime, ewp_info.consumer_senttime, 6);
	memcpy(pep_jnls.trndate, ewp_info.consumer_sentdate, 8);
	
    
	if(getbit(&iso, 22, (unsigned char *)pep_jnls.poitcde)<0)
	{
        dcs_log(0, 0, "can not get bit_3!");
        return -1;
	}
	sscanf(ewp_info.transfee+1, "%ld", &transfee);
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, ewp_info.transfee, 1);
	sprintf(tmpbuf+1, "%08ld", transfee);
	memcpy(pep_jnls.filed28, tmpbuf, getstrlen(tmpbuf));
    
	/*发送机构代码 sendcde[8]*/
	memcpy(pep_jnls.sendcde, "08000000", 8);
    
	s = getbit(&iso, 39, (unsigned char *)pep_jnls.aprespn);
	//dcs_log(0, 0, "s = [%d]", s);
	if(s<= 0)
		dcs_log(0, 0, "bit_39 is null!");
	else
		memcpy(pep_jnls.revodid, "2", 1);
	
	if(getbit(&iso, 41, (unsigned char *)pep_jnls.termcde)<0)
	{
        dcs_log(0, 0, "can not get bit_41!");
        return -1;
	}
    
	if(getbit(&iso, 42, (unsigned char *)pep_jnls.mercode)<0)
	{
        dcs_log(0, 0, "can not get bit_42!");
        return -1;
	}
    
	memcpy(pep_jnls.modecde, ewp_info.modecde, getstrlen(ewp_info.modecde));
	memcpy(pep_jnls.modeflg, ewp_info.modeflg, 1);
	
	s = getstrlen(ewp_info.consumer_orderno);
	if(s == 0)
		sprintf(pep_jnls.billmsg, "%s%s%s%06ld", "DY", pep_jnls.pepdate+2, pep_jnls.peptime, pep_jnls.trace);
	else
		memcpy(pep_jnls.billmsg, ewp_info.consumer_orderno, s);
	
		
	memcpy(pep_jnls.samid, ewp_info.consumer_psamno, strlen(ewp_info.consumer_psamno));
	memcpy(pep_jnls.termidm, ewp_info.consumer_phonesnno, strlen(ewp_info.consumer_phonesnno));
	memcpy(pep_jnls.translaunchway, ewp_info.translaunchway, strlen(ewp_info.translaunchway));
	if(getbit(&iso, 25, (unsigned char *)pep_jnls.poscode)<0)
	{
        dcs_log(0, 0, "can not get poscode!");
        return -1;
	}
	
	memcpy(pep_jnls.intcdbank, ewp_info.incdbkno, getstrlen(ewp_info.incdbkno));
	memcpy(pep_jnls.intcdname, ewp_info.incdname, getstrlen(ewp_info.incdname));

	if(pep_jnls.outcdtype != 'C' && pep_jnls.outcdtype != 'D' && pep_jnls.outcdtype != '-' )
	{
		pep_jnls.outcdtype =' ';//赋值为空
	}
		
		
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
		dcs_log(0, 0, "pep_jnls.aprespn=[%s]", pep_jnls.aprespn, pep_jnls.merid);
		dcs_log(0, 0, "pep_jnls.translaunchway=[%s]", pep_jnls.translaunchway, pep_jnls.camtlimit);
		dcs_log(0, 0, "pep_jnls.damtlimit=[%d], pep_jnls.outcdtype=[%c]", pep_jnls.damtlimit, pep_jnls.outcdtype);
		dcs_log(0, 0, "pep_jnls.trnsndpid=[%d]", pep_jnls.trnsndpid, pep_jnls.termmeramtlimit);
		dcs_log(0, 0, "pep_jnls.posmercode=[%s], pep_jnls.postermcde=[%s]", pep_jnls.posmercode, pep_jnls.postermcde);
		dcs_log(0, 0, "ewp_info.filed55=[%s], ewp_info.cardseq=[%s]", ewp_info.filed55, ewp_info.cardseq);		
	#endif	    
	memset(escape_buf55, 0, sizeof(escape_buf55));
	s = atoi(ewp_info.filed55_length);
	if( s>0)
		mysql_real_escape_string(sock, escape_buf55, pep_jnls.filed55, s);

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "INSERT INTO pep_jnls(PEPDATE, PEPTIME, TRANCDE, MSGTYPE, OUTCDNO, INTCDNO, INTCDBANK,\
	INTCDNAME, PROCESS, TRANAMT, TRACE, TERMTRC, TRNTIME, TRNDATE, POITCDE, SENDCDE, TERMCDE, MERCODE, BILLMSG,\
	SAMID, TERMID, TERMIDM, SDRESPN, REVODID, BILLFLG, MMCHGID, MMCHGNM, APRESPN, FILED48, TRNSNDP, TRNMSGD, REVDATE,\
	MODEFLG, MODECDE, FILED28, MERID, DISCOUNTAMT, POSCODE, TRANSLAUNCHWAY, CAMTLIMIT, DAMTLIMIT, OUTCDTYPE, TRNSNDPID,\
	TERMMERAMTLIMIT, POSMERCODE, POSTERMCDE,FIELD55,CARD_SN)VALUES( '%s', '%s', '%s', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%s', %ld, '%s', '%s', '%s', '%s', '%s', '%s', '%s',\
	'%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s',\
	'%s', %d, %d, '%c', %d, %d,'%s', '%s', '%s', '%s');", pep_jnls.pepdate, pep_jnls.peptime, pep_jnls.trancde, pep_jnls.msgtype, pep_jnls.outcdno,\
	pep_jnls.intcdno, pep_jnls.intcdbank, pep_jnls.intcdname, pep_jnls.process, pep_jnls.tranamt, pep_jnls.trace,\
		pep_jnls.termtrc, pep_jnls.trntime, pep_jnls.trndate, pep_jnls.poitcde, pep_jnls.sendcde, pep_jnls.termcde,\
		pep_jnls.mercode, pep_jnls.billmsg, pep_jnls.samid, pep_jnls.termid,pep_jnls.termidm, pep_jnls.sdrespn, pep_jnls.revodid,\
		pep_jnls.billflg, pep_jnls.mmchgid, pep_jnls.mmchgnm, pep_jnls.aprespn, pep_jnls.filed48, pep_jnls.trnsndp, pep_jnls.trnmsgd,\
		pep_jnls.revdate, pep_jnls.modeflg, pep_jnls.modecde, pep_jnls.filed28, pep_jnls.merid, pep_jnls.discountamt,\
		pep_jnls.poscode, pep_jnls.translaunchway, pep_jnls.camtlimit, pep_jnls.damtlimit, pep_jnls.outcdtype, pep_jnls.trnsndpid,\
		pep_jnls.termmeramtlimit, pep_jnls.posmercode, pep_jnls.postermcde,escape_buf55, ewp_info.cardseq);
	    
	    if(mysql_query(sock, sql)) 
		{
		  #ifdef __LOG__
			dcs_log(0, 0, "%s", sql);
		  #endif
	        dcs_log(0, 0, "SavePepjnls error [%s]!", mysql_error(sock));
	        return -1;
	    }
	    dcs_log(0, 0, "SavePepjnls success");
		return 1;
}

/*
 pepdate 同报文第7域，peptime 同报文第7域和扣款表pep_jnls中保存的一致
函数功能，保存转账汇款信息到转账汇款表transfer_info中
 */
int SaveTransferInfo(ISO_data iso, EWP_INFO ewp_info, PEP_JNLS pep_jnls, TRANSFER_INFO transfer_info)
{
	char tmpbuf[200], t_year[5];
	char id[32+1];
	char sql[2048];
	int	s = 0, rtn = 0;
	struct tm *tm;
	time_t t;
    
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(t_year, 0, sizeof(t_year));
	memset(id, 0, sizeof(id));
	memset(sql, 0, sizeof(sql));

    time(&t);
    tm = localtime(&t);
    
    sprintf(t_year, "%04d", tm->tm_year+1900);
    
	if(getbit(&iso, 7, (unsigned char *)tmpbuf)<0)
	{
        dcs_log(0, 0, "can not get bit_7!");
        return -1;
	}
	sprintf(pep_jnls.pepdate, "%4.4s%4.4s", t_year, tmpbuf);
	memcpy(pep_jnls.peptime, tmpbuf+4, 6);
	memcpy(pep_jnls.trancde, ewp_info.consumer_transcode, 3);
	getbit(&iso, 2, (unsigned char *)pep_jnls.outcdno);
	getbit(&iso, 4, (unsigned char *)pep_jnls.tranamt);
	
	/*termid 存手机号*/
	memcpy(pep_jnls.termid, ewp_info.consumer_username, 11);
	memcpy(pep_jnls.intcdno, ewp_info.incardno, getstrlen(ewp_info.incardno));
    
	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(getbit(&iso, 11, (unsigned char *)tmpbuf)<0)
	{
        dcs_log(0, 0, "can not get bit_3!");
        return -1;
	}
    
	sscanf(tmpbuf, "%06ld", &pep_jnls.trace);
	
	memcpy(pep_jnls.trntime, ewp_info.consumer_senttime, 6);
	memcpy(pep_jnls.trndate, ewp_info.consumer_sentdate, 8);

	memcpy(transfer_info.fee, ewp_info.transfee, 1);
	memcpy(transfer_info.fee+1, ewp_info.transfee+4, 8);
    
	s = getbit(&iso, 39, (unsigned char *)pep_jnls.aprespn);
	//dcs_log(0, 0, "s = [%d]", s);
	if(s<= 0)
		dcs_log(0, 0, "bit_39 is null!");
	else
	{
		rtn = GetAprespnDesc(pep_jnls.aprespn, pep_jnls.trancde, transfer_info.sale_respn_desc);
		if(rtn < 0)
			dcs_log(0, 0, "获取应答码描述信息失败");
	}
	
	if(getbit(&iso, 41, (unsigned char *)pep_jnls.termcde)<0)
	{
        dcs_log(0, 0, "can not get bit_41!");
        return -1;
	}
    
	if(getbit(&iso, 42, (unsigned char *)pep_jnls.mercode)<0)
	{
        dcs_log(0, 0, "can not get bit_42!");
        return -1;
	}
    
	memcpy(pep_jnls.modecde, ewp_info.modecde, getstrlen(ewp_info.modecde));
	memcpy(pep_jnls.modeflg, ewp_info.modeflg, 1);
	
	s = getstrlen(ewp_info.consumer_orderno);
	if(s == 0)
		sprintf(pep_jnls.billmsg, "%s%s%s%06ld", "DY", pep_jnls.pepdate+2, pep_jnls.peptime, pep_jnls.trace);
	else
		memcpy(pep_jnls.billmsg, ewp_info.consumer_orderno, s);
	
		
	memcpy(pep_jnls.samid, ewp_info.consumer_psamno, getstrlen(ewp_info.consumer_psamno));
		
	memcpy(pep_jnls.termidm, ewp_info.consumer_phonesnno, strlen(ewp_info.consumer_phonesnno));
	memcpy(pep_jnls.translaunchway, ewp_info.translaunchway, strlen(ewp_info.translaunchway));
	
	memcpy(pep_jnls.intcdbank, ewp_info.incdbkno, getstrlen(ewp_info.incdbkno));
	memcpy(pep_jnls.intcdname, ewp_info.incdname, getstrlen(ewp_info.incdname));
	memcpy(transfer_info.transfer_type, ewp_info.transfer_type, getstrlen(ewp_info.transfer_type));
	memcpy(transfer_info.pay_amt, ewp_info.consumer_money, getstrlen(ewp_info.consumer_money));

	if(pep_jnls.outcdtype != 'C' && pep_jnls.outcdtype != 'D' && pep_jnls.outcdtype != '-' )
	{
		pep_jnls.outcdtype =' ';//赋值为空
	}
				
	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls.pepdate=[%s], pep_jnls.peptime=[%s]", pep_jnls.pepdate, pep_jnls.peptime);
		dcs_log(0, 0, "pep_jnls.trancde=[%s]", pep_jnls.trancde);
		//dcs_log(0, 0, "pep_jnls.outcdno=[%s], pep_jnls.intcdno=[%s]", pep_jnls.outcdno, pep_jnls.intcdno);
		//dcs_log(0, 0, "pep_jnls.intcdbank=[%s], pep_jnls.intcdname=[%s]", pep_jnls.intcdbank, pep_jnls.intcdname);
		dcs_log(0, 0, "pep_jnls.tranamt=[%s]", pep_jnls.tranamt);
		dcs_log(0, 0, "pep_jnls.trace=[%d]", pep_jnls.trace);
		dcs_log(0, 0, "pep_jnls.trntime=[%s], pep_jnls.trndate=[%s]", pep_jnls.trntime, pep_jnls.trndate);
		dcs_log(0, 0, "pep_jnls.termcde=[%s], pep_jnls.mercode=[%s]", pep_jnls.termcde, pep_jnls.mercode);
		dcs_log(0, 0, "pep_jnls.billmsg=[%s], pep_jnls.samid=[%s]", pep_jnls.billmsg, pep_jnls.samid);
		dcs_log(0, 0, "pep_jnls.trnsndp=[%s]", pep_jnls.trnsndp);
		dcs_log(0, 0, "pep_jnls.modeflg=[%s], pep_jnls.modecde=[%s]", pep_jnls.modeflg, pep_jnls.modecde);
		dcs_log(0, 0, "pep_jnls.aprespn=[%s]", pep_jnls.aprespn);
		dcs_log(0, 0, "pep_jnls.outcdtype=[%c]", pep_jnls.outcdtype);
		dcs_log(0, 0, "pep_jnls.trnsndpid=[%d]", pep_jnls.trnsndpid);
	#endif	    

	getUUID(id);
	sprintf(sql, "INSERT INTO transfer_info(ID, PEPDATE, PEPTIME, TRANCDE, TRANS_NAME, OUTCDNO, \
	INTCDNO, INTCDBANK, INTCDNAME, IN_ACC_BANK_NAME, TRANAMT, FEE, TRACE, \
	TERMCDE, MERCODE, BILLMSG, TERMID, SALE_RESPN, SALE_RESPN_DESC, \
	MODECDE, TRANSFER_TYPE, \
	PAY_AMT)VALUES('%s', '%s', '%s', '%s', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), \
	HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%s', '%s', %ld, \
	'%s', '%s', '%s', '%s', '%s', '%s', \
	'%s', '%s', '%s');", id, pep_jnls.pepdate, pep_jnls.peptime, pep_jnls.trancde, \
	transfer_info.trans_name, pep_jnls.outcdno, pep_jnls.intcdno, pep_jnls.intcdbank, \
	pep_jnls.intcdname, transfer_info.in_acc_bank_name, pep_jnls.tranamt, transfer_info.fee, \
	pep_jnls.trace, pep_jnls.termcde, pep_jnls.mercode, \
	pep_jnls.billmsg, pep_jnls.termid, pep_jnls.aprespn, \
	transfer_info.sale_respn_desc, pep_jnls.modecde, \
	transfer_info.transfer_type, transfer_info.pay_amt);
	    
	if(mysql_query(sock, sql)) 
	{
		#ifdef __LOG__
	    	dcs_log(0, 0, "%s", sql);
	  	#endif
	    dcs_log(0, 0, "SaveTransferInfo error [%s]!", mysql_error(sock));
	   	return -1;
	}
	//dcs_log(0, 0, "SaveTransferInfo insert transfer_info success");
	return 1;
}


//查询原笔消费交易信息
int getOrgSaleInfo(TRANSFER_INFO transfer_info, PEP_JNLS *pep_jnls)
{
	char sql[1024];

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "SELECT aprespn, samid, modeflg, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), tranamt, trndate, postermcde, posmercode, \
	trntime, billmsg, trnsndp, msgtype, termidm, modecde, trancde, translaunchway, AES_DECRYPT(UNHEX(intcdno), 'abcdefgh'), termid, \
	trnsndpid, intcdbank, AES_DECRYPT(UNHEX(intcdname), 'abcdefgh'), card_sn, poscode, poitcde, authcode, syseqno, trace, filed28 FROM pep_jnls \
	where pepdate ='%s' and	 peptime='%s' and trace='%ld' and termcde='%s' and mercode='%s' and billmsg ='%s';", transfer_info.pepdate,\
	transfer_info.peptime, transfer_info.trace, transfer_info.termcde, transfer_info.mercode, transfer_info.billmsg);
	if(mysql_query(sock, sql)) 
	{
	  #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "getOrgSaleInfo 查找原笔交易出错 [%s]!", mysql_error(sock));
	    return -1;
	}
	MYSQL_RES *res; 
	MYSQL_ROW row ; 
	int num_fields;
	int num_rows = 0;
    
	if(!(res = mysql_store_result(sock))) {
	    dcs_log(0, 0, "getOrgSaleInfo:Couldn't get result from %s\n", mysql_error(sock));
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
	}
	else if(num_rows == 0)
	{
	#ifdef __LOG__
		dcs_log(0, 0, "%s", sql);
	#endif
		dcs_log(0, 0, "getOrgSaleInfo没有找到符合条件的记录");
		mysql_free_result(res);  
		return -1;
	}
	else if(num_rows >1)
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
		dcs_log(0, 0, "getOrgSaleInfo查到多条符合条件的记录");
		mysql_free_result(res);
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
#endif
	return 0;
}

/*
次日代付查询，
查询beginDate和endDate之间的扣款成功待代付的转账汇款交易，
查询transfer_info
*/
int DealNextDayPay(char *beginDate, char *endDate)
{
	char sql[1024];
	char md5Buf[128];/*参与计算MD5的串*/
	char MD5key[16+1];/*存放MD5key*/
	char MD5Value[120+1];/*存放计算出的MD5串*/
	TRANSFER_INFO transfer_info, trans_info;
	char sendbuf[1024];
	char rcvbuf[2048];
	char local_ip[16];
	int len = 0, rtn = 0;
	int queue_id = -1;
	long msgtype = 0;
	
	MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	
	memset(md5Buf, 0x00, sizeof(md5Buf));
	memset(MD5key, 0x00, sizeof(MD5key));
	memset(MD5Value, 0x00, sizeof(MD5Value));
	memset(sendbuf, 0x00, sizeof(sendbuf));
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	memset(sql, 0x00, sizeof(sql));
	memset(&transfer_info, 0x00, sizeof(TRANSFER_INFO));
	memset(&trans_info, 0x00, sizeof(TRANSFER_INFO));
	memset(local_ip, 0x00, sizeof(local_ip));
    
	//获取本机IP地址
	getIP(local_ip);
	//选择MD5key
#ifdef __TEST__
	memcpy(MD5key, TEST_MD5_KEY, 16);
#else
	if(memcmp(local_ip, "192.168.30", 10)==0)/*UAT环境使用测试KEY*/
		memcpy(MD5key, TEST_MD5_KEY, 16);
	else
		memcpy(MD5key, PROT_MD5_KEY, 16);
#endif
	
	memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "select mercode, billmsg, pay_amt, AES_DECRYPT(UNHEX(intcdno), 'abcdefgh'), AES_DECRYPT(UNHEX(intcdname), 'abcdefgh'), intcdbank from transfer_info  where sale_respn = '00' \
	and transfer_type ='1' and pay_flag ='0' and cancel_pay_flag ='0' and reversalflag ='0' \
	and CONCAT(pepdate, peptime)>='%s' and  CONCAT(pepdate, peptime)< '%s'  ORDER BY CONCAT(pepdate,peptime);", beginDate, endDate); 
	if(mysql_query(sock,sql)) 
	{
        dcs_log(0, 0, "%s", sql);
        dcs_log(0,0, "getNextDayPayInfo fail [%s]!" ,mysql_error(sock));
        return -1;
    }
    if (!(res = mysql_store_result(sock))) 
	{
       dcs_log(0, 0, "getNextDayPayInfo Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
        
    num_fields = mysql_num_fields(res);
    while ((row = mysql_fetch_row(res)))
    {
		memset(md5Buf, 0x00, sizeof(md5Buf));
		memset(MD5Value, 0x00, sizeof(MD5Value));
		memset(sendbuf, 0x00, sizeof(sendbuf));
		len = 0;
		queue_id = -1;
		msgtype = 0;
		rtn = 0;
		memset(&transfer_info, 0x00, sizeof(TRANSFER_INFO));
		memset(&trans_info, 0x00, sizeof(TRANSFER_INFO));
		
		memcpy(transfer_info.accountId, row[0] ? row[0] : "\0", (int)sizeof(transfer_info.accountId)); 
		memcpy(transfer_info.orderId, row[1] ? row[1] : "\0", (int)sizeof(transfer_info.orderId));  
		memcpy(transfer_info.pay_amt, row[2] ? row[2] : "\0", (int)sizeof(transfer_info.pay_amt)); 
		memcpy(transfer_info.rcvAcno, row[3] ? row[3] : "\0", (int)sizeof(transfer_info.rcvAcno));  
		memcpy(transfer_info.rcvAcname, row[4] ? row[4] : "\0", (int)sizeof(transfer_info.rcvAcname)); 
		memcpy(transfer_info.bankNo, row[5] ? row[5] : "\0", (int)sizeof(transfer_info.bankNo)); 
		
		//更新代付标识以及代付请求日期
		rtn = updatePayReqDate(transfer_info);
		if(rtn < 0)
			continue;

		sprintf(md5Buf, "10%s%.02fC1%s%s", transfer_info.orderId, atof(transfer_info.pay_amt)/100, transfer_info.rcvAcno, MD5key);
		len = strlen(md5Buf);
		//计算签名值
		rtn = getUpMD5(md5Buf, len, MD5Value);
		if(rtn <0)
			continue;
	
		//组代付请求报文
		sprintf(sendbuf, "version=10&accountId=%s&orderId=%s&transAmt=%.02f&transType=C1&dfType=A&cardFlag=0&rcvAcno=%s&rcvAcname=%s&bankNo=%s&purpose=%s&merPriv= &bgRetUrl=http://%s:9915&dataSource=5&chkValue=%s", \
		transfer_info.accountId, transfer_info.orderId, atof(transfer_info.pay_amt)/100, transfer_info.rcvAcno, transfer_info.rcvAcname, \
		transfer_info.bankNo, "转账汇款代付", local_ip, MD5Value);
		len = strlen(sendbuf);
		//连接共享队列 发送发送报文到队列
		queue_id = queue_connect("DDFSC");
		if(queue_id < 0)
		{
			dcs_log(0, 0, "connect msg queue fail!");
			continue;
		}
		msgtype = getpid();
		rtn = queue_send_by_msgtype(queue_id, sendbuf, len, msgtype, 0);
		if(rtn< 0)
		{
			dcs_log(0, 0, "queue_send_by_msgtype err");
			continue;
		}
		//接收队列
		queue_id = queue_connect("R_DFSC");
		if (queue_id <0)
		{
			dcs_log(0, 0, "connect msg queue fail!");
			continue;
		}
		memset(rcvbuf, 0x00, sizeof(rcvbuf));
		rtn = queue_recv_by_msgtype(queue_id, rcvbuf, sizeof(rcvbuf), &msgtype, 0);
		if(rtn< 0)
		{
			dcs_log(0, 0, "queue_recv_by_msgtype err");
			continue;
		}
		if(memcmp(rcvbuf, "00", 2) == 0)//返回成功//处理返回 更新记录	
		{
			dcs_log(0, 0, "返回应答成功[%s]", rcvbuf+2);
			len = strlen(rcvbuf)-2;
			rtn = getResPonse(rcvbuf+2, len, &trans_info);
			if(rtn <0)
				continue;
			//解析之后验证签名 参与数字签名的字符串：orderId + transAmt + tseq + transStatus
			if(memcmp(trans_info.payResultcode+8, "00", 2)==0)
			{
				rtn = checkMD5value(trans_info);
				if(rtn <0)
					continue;
				//更新代付状态
				if(memcmp(trans_info.transStatus, "S", 1)==0)
				{
					//代付成功，更新记录并且发送短信
					memcpy(trans_info.pay_respn, "00", 2);
					memcpy(trans_info.pay_respn_desc, "代付成功", strlen("代付成功"));
					updatePayInfo(&trans_info);//更新代付记录
					//发送短信
					if(strlen(trans_info.modecde)==11)
					{
						dcs_log(0, 0, "需要发送短信");
						sendMes(trans_info);
					}
				}
				else if(memcmp(trans_info.transStatus, "F", 1)==0)
				{
					//受理成功，代付失败，更新记录
					memcpy(trans_info.pay_respn, "0F", 2);
					memcpy(trans_info.pay_respn_desc, trans_info.errorMsg, strlen(trans_info.errorMsg));
					updatePayInfo(&trans_info);//更新代付记录
				}
				else if(memcmp(trans_info.transStatus, "W", 1)==0)
				{
					//代付受理中，更新记录
					memcpy(trans_info.pay_respn, "A6", 2);
					memcpy(trans_info.pay_respn_desc, "受理中", strlen("受理中"));
					updatePayInfo(&trans_info);//更新代付记录
				}
			}
			else
			{
				//受理失败
				memcpy(transfer_info.pay_respn, trans_info.payResultcode+8, 2);
				memcpy(transfer_info.pay_respn_desc, trans_info.payResultcode_desc, strlen(trans_info.payResultcode_desc));
				updatePayInfo(&transfer_info);//更新代付记录
			}
			continue;
		}
		else
		{
			dcs_log(0, 0, "返回失败应答[%s]", rcvbuf);
			continue;
		}
    }      
    mysql_free_result(res);
	return 0;
}


int sendMes(TRANSFER_INFO transfer_info)
{
	PEP_JNLS pep_jnls;
	int rtn = 0;
	
	memset(&pep_jnls, 0x00, sizeof(pep_jnls));
	//查询原笔交易信息查询pep_jnls数据表
	rtn = getOrgSaleInfo(transfer_info, &pep_jnls);
	if(rtn<0)
	{
		dcs_log(0, 0, "获取原笔交易信息失败");
		return -1;
	}
	
	memcpy(pep_jnls.syseqno, transfer_info.tseq, 12);/*短信参考号为代付流水号*/
	strcpy(pep_jnls.filed28, transfer_info.fee);
	strcpy(pep_jnls.pepdate, transfer_info.sysDate);
	strcpy(pep_jnls.peptime, transfer_info.sysTime);
	SendMsg(pep_jnls);
	return 0;
}
/*
单张转出卡日交易金额不超过5万
*/
int getCurTotalAmt(char *curAmt, char *curDate, char * outcard)
{
	char sql[256];
	char i_curAmt[13];
	char i_curDate[8+1];
	
	memset(sql, 0x00, sizeof(sql));
	memset(i_curAmt, 0x00, sizeof(i_curAmt));
	memset(i_curDate, 0x00, sizeof(i_curDate));
	
	memcpy(i_curDate, curDate, 8);
	sprintf(sql, "SELECT lpad(ifnull(sum(pay_amt),'0'),12,'0') FROM transfer_info  WHERE sale_respn = '00' \
	and (transfer_type = '1' || (transfer_type = '0' and  pay_respn ='00')) \
    and  pepdate =%s and outcdno = HEX(AES_ENCRYPT('%s','abcdefgh'));", i_curDate,outcard);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getCurTotalAmt error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "getCurTotalAmt : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) 
	{
		memcpy(i_curAmt, row[0] ? row[0] : "\0", (int)sizeof(i_curAmt));
	}
	else if(num_rows == 0)
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
        dcs_log(0, 0, "getCurTotalAmt:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
        dcs_log(0, 0, "getCurTotalAmt:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	mysql_free_result(res);
	memcpy(curAmt, i_curAmt, strlen(i_curAmt));
	#ifdef __LOG__
		dcs_log(0, 0, "i_curAmt = [%s]", i_curAmt);
		dcs_log(0, 0, "curAmt = [%s]", curAmt);
	#endif
	
	return 0;
}

/*判断该笔订单是否已经发起过扣款*/
int JudgeOrderInfo(char *order_info)
{
	char sql[512];
	int num = 0;
    memset(sql, 0x00, sizeof(sql));
	
    sprintf(sql, "SELECT count(*) from pep_jnls where  aprespn ='00' and billmsg = '%s';", order_info);
    
    if(mysql_query(sock,sql)) 
	{
    #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
		dcs_log(0, 0, "JudgeOrderInfo info error [%s]!", mysql_error(sock));
        return -1;
    }
	 
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock)))
	{
        dcs_log(0, 0, "JudgeOrderInfo:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    
    if(mysql_num_rows(res) == 1) 
	{
        if (row[0]) 
			num = atoi(row[0]); 
		else 
			num = 0;
    } 
	else 
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
		dcs_log(0, 0, "JudgeOrderInfo:操作数据表失败");
		mysql_free_result(res);
		return -1;
	}
    
    mysql_free_result(res);
	return num;
}

/*根据流水号和订单号查询冲正对应的消费信息*/
int getSendInfo(char *billmsg, char *trace, PEP_JNLS *pep_jnls)
{
	char sql[1024];

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "SELECT aprespn, samid, modeflg, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), tranamt, trndate, postermcde, posmercode, \
	trntime, billmsg, trnsndp, msgtype, termidm, modecde, trancde, translaunchway, AES_DECRYPT(UNHEX(intcdno), 'abcdefgh'), termid, \
	trnsndpid, intcdbank, AES_DECRYPT(UNHEX(intcdname), 'abcdefgh'), card_sn, poscode, poitcde, authcode, syseqno, trace, filed28, mercode, termcde FROM pep_jnls \
	where trace='%ld' and billmsg ='%s';", atol(trace), billmsg);
	if(mysql_query(sock, sql)) 
	{
	  #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "getSendInfo 查找原笔交易出错 [%s]!", mysql_error(sock));
	    return -1;
	}
	MYSQL_RES *res; 
	MYSQL_ROW row ; 
	int num_fields;
	int num_rows = 0;
    
	if(!(res = mysql_store_result(sock))) {
	    dcs_log(0, 0, "getSendInfo:Couldn't get result from %s\n", mysql_error(sock));
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
	}
	else if(num_rows == 0)
	{
	#ifdef __LOG__
		dcs_log(0, 0, "%s", sql);
	#endif
		dcs_log(0, 0, "getSendInfo没有找到符合条件的记录");
		mysql_free_result(res);  
		return -1;
	}
	else if(num_rows >1)
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
		dcs_log(0, 0, "getSendInfo查到多条符合条件的记录");
		mysql_free_result(res);
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
#endif
	return 0;
}

/*mysql断开重连的处理*/
int GetTransferMysqlLink()
{	
	int i = 0;
	for(i = 0; i<7; i++)
	{
		#ifdef __TEST__
			dcs_log(0, 0, "%d ci GetTransferMysqlLink", i);
		#endif
		if(mysql_ping(sock)!=0)
			continue;
		else
			break;
	}
	#ifdef __TEST__
		dcs_log(0, 0, "test GetTransferMysqlLink");
	#endif
	return mysql_ping(sock);
}

