/*****************************************************
*  dssdclfn_mysql.c
*  iPrepose
*
*  Created by Freax on 13-4-8.
*  Copyright (c) 2013年 Chinaebi. All rights reserved.
********************************************************/
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include "iso8583.h"
#include "trans.h"
#include "modi.h"
#include "pub_error.h"
#include <stdlib.h>
#include <string.h>
#include "extern_function.h"

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

extern struct MSGCHGSTRU *termbuf;
extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583_conf[128];	
extern MYSQL *GetFreeMysqlLink();
extern char card_sn[3+1];
static fd55 stFd55[43]={{"FF00"},{"FF01"},{"FF02"},{"FF03"},{"FF04"},{"FF05"},{"FF06"},{"FF07"},{"FF08"},{"FF09"},{"FF20"},{"FF21"},{"FF22"},{"FF23"},{"FF24"},{"FF25"},{"FF26"},{"FF27"},{"FF28"},{"FF29"},{"FF2A"},{"FF2B"},
  	               {"FF40"},{"FF41"},{"FF42"},{"FF43"},{"FF44"},{"FF45"},{"FF46"},{"FF47"},{"FF48"},{"FF49"},{"FF4A"},{"FF4B"},{"FF60"},{"FF61"},{"FF62"},{"FF63"},{"FF64"},{"FF80"},{"FF81"},{"FF82"}};
	
static fd55 stFd55_IC[35]={{"9F26"},{"9F27"},{"9F10"},{"9F37"},{"9F36"},{"95"},{"9A"},{"9C"},{"9F02"},{"5F2A"},{"82"},{"9F1A"},{"9F03"},{"9F33"},{"9F74"},
							{"9F34"},{"9F35"},{"9F1E"},{"84"},{"9F09"},{"9F41"},{"91"},{"71"},{"72"},{"DF31"},{"9F63"},{"8A"},{"DF32"},{"DF33"},{"DF34"}, 
							{"9B"}, {"4F"}, {"5F34"}, {"50"}};


//static int getstrlen(char * StrBuf);//去掉字符串后的空格

struct  _varchar {
    unsigned short len;
    unsigned char  arr[1];
};

typedef struct _varchar VARCHAR;

//==============================//
 MYSQL mysql, *sock;    //定义数据库连接的句柄，它被用于几乎所有的MySQL函数
//==============================//
int DssConect()
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

	if(mysql_set_character_set(&mysql, "utf8")) {
		fprintf (stderr, "错误, %s\n", mysql_error(&mysql));
		dcs_log(0, 0, "错误, %s\n", mysql_error(&mysql));
		return 0;
	}

	if(sock){
		dcs_log(0, 0, "数据库连接成功,name:%s", (char *)getenv("DBNAME"));
		return 1;
	}else{
		dcs_log(0, 0, "数据库连接失败,name:%s", (char *)getenv("DBNAME"));
		return 0;
	}
	return 1;
}

int DssEnd(int iDssOperate)
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
			dcs_log(0, 0, "<DSS>Invalid arguments: %d", iDssOperate);
			return -1 ;
	}
    mysql_close(sock);
	return 0;
}

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

/*
 根据3位的交易码查询数据库表msgchg_info，得到MSGCHGINFO对象
 */
int Readmsgchg(char *trncde, MSGCHGINFO *o_msginfo)
{
    char i_transcde[3+1];
	char sql[1024];

    MSGCHGINFO	msginfo;
	  
	memset(&msginfo, 0, sizeof(MSGCHGINFO));
	memset(i_transcde, 0, sizeof(i_transcde));
	memset(sql, 0x00, sizeof(sql));
	
	memcpy(i_transcde, trncde, 3);
    
    sprintf(sql, "select trancde, msgtype, process, service, amtcode, mercode, termcde, subtype,\
	revoid, bit_20, bit_25, fsttran, remark, flag20, flagpe, cdlimit, ifnull(c_amtlimit, '0'),\
	ifnull(d_amtlimit, '0'), ifnull(c_sin_totalamt, '0'), ifnull(d_sin_totalamt, '0'), \
	ifnull(c_day_count, 0), ifnull(d_day_count, 0), flagpwd, flagfee from msgchg_info where trancde = '%s';", i_transcde);
    
    if(mysql_query(sock, sql)){
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "Read msgchg_info DB error [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "Readmsgchg:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
	
    if(num_rows == 1) {
        memcpy(msginfo.trancde, row[0] ? row[0] : "\0", (int)sizeof(msginfo.trancde));
        memcpy(msginfo.msgtype, row[1] ? row[1] : "\0", (int)sizeof(msginfo.msgtype));
        memcpy(msginfo.process, row[2] ? row[2] : "\0", (int)sizeof(msginfo.process));
        memcpy(msginfo.service, row[3] ? row[3] : "\0", (int)sizeof(msginfo.service));
        memcpy(msginfo.amtcode, row[4] ? row[4] : "\0", (int)sizeof(msginfo.amtcode));
        memcpy(msginfo.mercode, row[5] ? row[5] : "\0", (int)sizeof(msginfo.mercode));
        memcpy(msginfo.termcde, row[6] ? row[6] : "\0", (int)sizeof(msginfo.termcde));
        memcpy(msginfo.subtype, row[7] ? row[7] : "\0", (int)sizeof(msginfo.subtype));
        memcpy(msginfo.revoid, row[8] ? row[8] : "\0", (int)sizeof(msginfo.revoid));
        memcpy(msginfo.bit_20, row[9] ? row[9] : "\0", (int)sizeof(msginfo.bit_20));
        memcpy(msginfo.bit_25, row[10] ? row[10] : "\0", (int)sizeof(msginfo.bit_25));
        memcpy(msginfo.frttran, row[11] ? row[11] : "\0", (int)sizeof(msginfo.frttran));
        memcpy(msginfo.remark, row[12] ? row[12] : "\0", (int)sizeof(msginfo.remark));
        memcpy(msginfo.flag20, row[13] ? row[13] : "\0", (int)sizeof(msginfo.flag20));
        memcpy(msginfo.flagpe, row[14] ? row[14] : "\0", (int)sizeof(msginfo.flagpe));
        if(row[15]) 
			msginfo.cdlimit = row[15][0];
        memcpy(msginfo.c_amtlimit, row[16] ? row[16] : "\0", (int)sizeof(msginfo.c_amtlimit));
		memcpy(msginfo.d_amtlimit, row[17] ? row[17] : "\0", (int)sizeof(msginfo.d_amtlimit));
		memcpy(msginfo.c_sin_totalamt, row[18] ? row[18] : "\0", (int)sizeof(msginfo.c_sin_totalamt));
		memcpy(msginfo.d_sin_totalamt, row[19] ? row[19] : "\0", (int)sizeof(msginfo.d_sin_totalamt));
		msginfo.c_day_count = atoi(row[20]);
		msginfo.d_day_count = atoi(row[21]);
        memcpy(msginfo.flagpwd, row[22] ? row[22] : "\0", (int)sizeof(msginfo.flagpwd));
        memcpy(msginfo.flagfee, row[23] ? row[23] : "\0", (int)sizeof(msginfo.flagfee));
    }
    else if(num_rows == 0)
	{
		dcs_log(0, 0, "Readmsgchg：没有找到符合条件的记录");
		mysql_free_result(res);  
		return -1;
	}
	else if(num_rows >1)
	{
		dcs_log(0, 0, "Readmsgchg：查到多条符合条件的记录");
		mysql_free_result(res);  
		return -1;
	}
	
    mysql_free_result(res);
    
	#ifdef __LOG__
		dcs_log(0, 0, "i_transcde=%s", i_transcde);
	#endif
   	
	#ifdef __LOG__
		dcs_log(0, 0, "msginfo:tcde=%s, o_msgtype=%s, o_process=%s, o_flagpe=[%s].",\
            msginfo.trancde, msginfo.msgtype, msginfo.process, msginfo.flagpe);
	#endif
	
	memcpy(o_msginfo, &msginfo, sizeof(MSGCHGINFO));
	
	return 0;
}

/*
 获得前置系统的交易流水
 */
int pub_get_trace_from_seq(char *ctrace)
{
    long new_trace = 0;
    char sql[512];
	
    memset(sql, 0x00, sizeof(sql));
    strcpy(sql, "select nextval('trace_seq');");
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "select nextval('trace_seq'); error [%s]!", mysql_error(sock));
        return -1;
    }
      
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "pub_get_trace_from_seq : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    num_rows = mysql_num_rows(res);
	
    if(num_rows == 1) {
        row = mysql_fetch_row(res);
        new_trace = atol(row[0]);
    }
    else if(num_rows ==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "pub_get_trace_from_seq：未找到符合条件的记录");
		mysql_free_result(res);  
		return  -1;
	}
	else 
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "pub_get_trace_from_seq：查找到多条符合条件的记录");
		mysql_free_result(res);  
		return  -1;
	}
    mysql_free_result(res);   
	sprintf(ctrace, "%06ld", new_trace);  
	
	return 0 ;
}

/*
 功能：通过psam卡号得到该psam卡的密钥信息
 输入参数：psamNo卡号，16字节长度
 输出参数：SECUINFO
 */
int GetSecuInfo(char *samNo, SECUINFO *sec_info)
{
    SECUINFO i_sec_info;
	char sql[1024];
	
	memset(&i_sec_info, 0, sizeof(SECUINFO)); 
    memset(sql, 0x00, sizeof(sql));
			
    sprintf(sql, "SELECT sam_card, sam_model, sam_status, sam_signinflag, sam_zmk_index, sam_zmk, sam_zmk1,\
	sam_zmk2, sam_zmk3, sam_pinkey_idx, sam_pinkey, sam_mackey_idx, sam_mackey, sam_tdkey_idx, sam_tdkey,\
	pinkey_idx, pinkey, mackey_idx, mackey, pin_convflag, channel_id FROM samcard_info WHERE sam_card = '%s';", samNo);
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetSecuInfoTable samcard_info DB error [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "samcard_info : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
	
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
        memcpy(i_sec_info.sam_card, row[0] ? row[0] : "\0", (int)sizeof(i_sec_info.sam_card));
        memcpy(i_sec_info.sam_model, row[1] ? row[1] : "\0", (int)sizeof(i_sec_info.sam_model));
        memcpy(i_sec_info.sam_status, row[2] ? row[2] : "\0", (int)sizeof(i_sec_info.sam_status));
        memcpy(i_sec_info.sam_signinflag, row[3] ? row[3] : "\0", (int)sizeof(i_sec_info.sam_signinflag));
        if(row[4]) 
			i_sec_info.sam_zmk_index = atoi(row[4]);
        memcpy(i_sec_info.sam_zmk, row[5] ? row[5] : "\0", (int)sizeof(i_sec_info.sam_zmk));
        memcpy(i_sec_info.sam_zmk1, row[6] ? row[6] : "\0", (int)sizeof(i_sec_info.sam_zmk1));
        memcpy(i_sec_info.sam_zmk2, row[7] ? row[7] : "\0", (int)sizeof(i_sec_info.sam_zmk2));
        memcpy(i_sec_info.sam_zmk3, row[8] ? row[8] : "\0", (int)sizeof(i_sec_info.sam_zmk3));
        if(row[9]) 
			i_sec_info.sam_pinkey_idx = atoi(row[9]);
        memcpy(i_sec_info.sam_pinkey, row[10] ? row[10] : "\0", (int)sizeof(i_sec_info.sam_pinkey));
        if(row[11]) 
			i_sec_info.sam_mackey_idx = atoi(row[11]);
        memcpy(i_sec_info.sam_mackey, row[12] ? row[12] : "\0", (int)sizeof(i_sec_info.sam_mackey));
        if(row[13]) 
			i_sec_info.sam_tdkey_idx = atoi(row[13]);
        memcpy(i_sec_info.sam_tdkey, row[14] ? row[14] : "\0", (int)sizeof(i_sec_info.sam_tdkey));
        if(row[15]) 
			i_sec_info.pinkey_idx = atoi(row[15]);
        memcpy(i_sec_info.pinkey, row[16] ? row[16] : "\0", (int)sizeof(i_sec_info.pinkey));
        if(row[17])
			i_sec_info.mackey_idx = atoi(row[17]);
        memcpy(i_sec_info.mackey, row[18] ? row[18] : "\0", (int)sizeof(i_sec_info.mackey));
        if(row[19]) 
			i_sec_info.pin_convflag = atoi(row[19]);
        if(row[20]) 
			i_sec_info.channel_id = atol(row[20]);
    }
	 else if(num_rows == 0)
	{
		dcs_log(0, 0, "GetSecuInfo：没有找到符合条件的记录");
		mysql_free_result(res);  
		return -1;
	}
	else if(num_rows >1)
	{
		dcs_log(0, 0, "GetSecuInfo：查到多条符合条件的记录");
		mysql_free_result(res);  
		return -1;
	}
    mysql_free_result(res);
	memcpy(sec_info, &i_sec_info, sizeof(SECUINFO));	
	return 1;
}

/*
 信用卡日限额金额更新
 输入参数：
 samNo，psam卡号
 amt，交易金额，比如100元，就是10000，精确到分

 输出参数：
 -1：失败
 1：成功
 */
int UpdateTerminalCreditCardTradeAmtLimit(char *samNo, char *amt)
{
	char i_psamno[16+1];
	char sql[512];
	
  	memset(i_psamno, 0, sizeof(i_psamno));
	memset(sql, 0x00, sizeof(sql));
	
  	memcpy(i_psamno, samNo, getstrlen(samNo));
	//需要从连接池获取连接
  	MYSQL *sock  = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
    sprintf(sql, "update samcard_trade_restrict set c_total_tranamt = c_total_tranamt + '%s' where psam_card = '%s';", amt, i_psamno);
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "UpdateTerminalCreditCardTradeAmtLimit update samcard_trade_restrict psam_card DB  error [%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
    //从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
	return 1;
}

/*
 借记卡日限额金额更新
 输入参数：
 samNo，psam卡号
 amt，交易金额，比如100元，就是10000，精确到分

 输出参数：
 -1：失败
 1：成功
 */
int UpdateTerminalDebitCardTradeAmtLimit(char *samNo, char * amt)
{
	char i_psamno[16+1];
	char sql[512];
	
  	memset(i_psamno,0,sizeof(i_psamno));
	memset(sql, 0x00, sizeof(sql));
	
  	memcpy(i_psamno, samNo, getstrlen(samNo));
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
    sprintf(sql, "update samcard_trade_restrict set d_total_tranamt = d_total_tranamt+ '%s' where psam_card = '%s';", amt, i_psamno);
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "UpdateTerminalDebitCardTradeAmtLimit update samcard_trade_restrict psam_card DB  error [%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
    //从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
	return 1;
}

int pub_rtrim(inbuf, inlen, outbuf)
char *inbuf;
int inlen;
char *outbuf;
{
	int i;
    char *buf;
    
    buf = (char *)malloc(inlen+1);
    if(buf == NULL)
    {
		dcs_log(0, 0, "pub_rtrim:malloc error.");
		outbuf[0] = 0x0;
        return (-1);
    }
    memcpy(buf, inbuf, inlen);
    
	for (i = inlen - 1; i > 0; i--)
	{
		if ((buf[i] == ' ') || (buf[i] == 0x0))
			buf[i] = 0;
		else break;
	}
    if(i >= 0)
    {
        memcpy(outbuf, buf, i + 1);
        outbuf[i + 1] = 0;
    } 
	else
    {
        outbuf[0] = 0x0;
    }
    
	free(buf);
	return(0);
}

int TrimRightSpace(char* pString)
{
    int iStringLen;
    int iCurPos;
    
    if (pString == NULL )
        return 0;
    
    iStringLen= strlen(pString);
    
    for(iCurPos= iStringLen-1; iCurPos>=0; iCurPos--)
    {
        if(pString[iCurPos] == ' ')
        {
            pString[iCurPos]= '\0';
            continue;
        }
        else
            break;
    }
    return 1;
}

/*
 pepdate 同报文第7域
 peptime 同报文第7域
 */
int WriteXepDb( ISO_data iso, EWP_INFO ewp_info, TERMMSGSTRUCT termbuf, PEP_JNLS pep_jnls)
{
	char tmpbuf[200], t_year[5];
	char escape_buf55[512+1];
	int	s, i;
	struct tm *tm;
	time_t t;
    
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
    
	if(getbit(&iso, 2, (unsigned char *)pep_jnls.outcdno)<1)
		memcpy(pep_jnls.outcdno, termbuf.cPan, 20);
	
	getbit(&iso, 4, (unsigned char *)pep_jnls.tranamt);
	/*优惠金额*/
	sprintf(pep_jnls.discountamt, "%012s", ewp_info.discountamt);
	
	/*termid 存手机号*/
	memcpy(pep_jnls.termid, ewp_info.consumer_username, 11);
	
	getbit(&iso, 20, (unsigned char *)pep_jnls.intcdno);
    
	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(getbit(&iso, 11, (unsigned char *)tmpbuf)<0)
	{
        dcs_log(0, 0, "can not get bit_3!");
        return -1;
	}
    
	sscanf(tmpbuf, "%06ld", &pep_jnls.trace);
	
	sprintf(pep_jnls.termtrc, "%06ld", termbuf.iNum);
	
	//ewp 脱机消费上送时 该字段的取值不用取 发送日期
	if(strlen(pep_jnls.trndate) == 0)
	{
		memcpy(pep_jnls.trntime, ewp_info.consumer_senttime, 6);
		memcpy(pep_jnls.trndate, ewp_info.consumer_sentdate, 8);
	}
    
	if(getbit(&iso, 22, (unsigned char *)pep_jnls.poitcde)<0)
	{
        dcs_log(0, 0, "can not get bit_3!");
        return -1;
	}
	getbit(&iso, 28, (unsigned char *)pep_jnls.filed28);
    
	/*发送机构代码 sendcde[8]*/
	memcpy(pep_jnls.sendcde, ewp_info.consumer_sentinstitu, 4);
	memcpy(pep_jnls.sendcde+4, "0000", 4);
    
	s = getbit(&iso, 39, (unsigned char *)pep_jnls.aprespn);
	dcs_log(0, 0, "s = [%d]", s);
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
	
		
	memcpy(pep_jnls.samid, termbuf.cSamID, strlen(termbuf.cSamID));
	/*
     memcpy(pep_jnls.termid, termbuf.cTermID, 25);
     memcpy(pep_jnls.termmac, termbuf.cMac, 16);
     memcpy(pep_jnls.termpro, termbuf.cOptCode, strlen(termbuf.cOptCode));
     */
	memcpy(pep_jnls.trnsndp, termbuf.e1code, strlen(termbuf.e1code));
	
	pep_jnls.merid=atoi(ewp_info.consumer_merid);
	
	memcpy(pep_jnls.termidm, ewp_info.consumer_phonesnno, strlen(ewp_info.consumer_phonesnno));
	memcpy(pep_jnls.translaunchway, ewp_info.translaunchway,strlen(ewp_info.translaunchway));
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
		
		
//	dcs_log(0, 0, "__LOG___begin");
	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls.pepdate=[%s], pep_jnls.peptime=[%s]", pep_jnls.pepdate, pep_jnls.peptime);
		dcs_log(0, 0, "pep_jnls.trancde=[%s], pep_jnls.msgtype=[%s]", pep_jnls.trancde, pep_jnls.msgtype);
		//dcs_log(0, 0, "pep_jnls.outcdno=[%s], pep_jnls.intcdno=[%s]", pep_jnls.outcdno, pep_jnls.intcdno);
		dcs_log(0, 0, "pep_jnls.intcdbank=[%s], pep_jnls.intcdname=[%s]", pep_jnls.intcdbank, pep_jnls.intcdname);
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
		dcs_log(0, 0, "pep_jnls.aprespn=[%s], pep_jnls.merid=[%d]", pep_jnls.aprespn, pep_jnls.merid);
		dcs_log(0, 0, "pep_jnls.translaunchway=[%s], pep_jnls.camtlimit=[%d]", pep_jnls.translaunchway, pep_jnls.camtlimit);
		dcs_log(0, 0, "pep_jnls.damtlimit=[%d], pep_jnls.outcdtype=[%c]", pep_jnls.damtlimit, pep_jnls.outcdtype);
		dcs_log(0, 0, "pep_jnls.trnsndpid=[%d], pep_jnls.termmeramtlimit=[%d]", pep_jnls.trnsndpid, pep_jnls.termmeramtlimit);
		dcs_log(0, 0, "pep_jnls.posmercode=[%s], pep_jnls.postermcde=[%s]", pep_jnls.posmercode, pep_jnls.postermcde);
		dcs_log(0, 0, "ewp_info.filed55=[%s], ewp_info.cardseq=[%s]", ewp_info.filed55, ewp_info.cardseq);		
	#endif	    
//	dcs_log(0, 0, "__LOG___end!");
	memset(escape_buf55, 0, sizeof(escape_buf55));
	s =atoi(ewp_info.filed55_length);
	if( s>0)
		mysql_real_escape_string(sock, escape_buf55, pep_jnls.filed55, s);

	char sql[2048];
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
	    
	    if(mysql_query(sock, sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "WriteXepDb error [%s]!", mysql_error(sock));
	        return -1;
	    }
	    dcs_log(0, 0, "WriteXepDb insert pep_jnls success");
		return 1;
}

int saveOrUpdatePepjnls(ISO_data iso, PEP_JNLS *opep_jnls)
{    
	int	s = 0, rtn = 0;
	char tmpbuf[256], trace[7], currentDate[15];
	struct  tm *tm;
	time_t  t;
	char sql[1024];

	PEP_JNLS pep_jnls;
	TWO_CODE_INFO two_code_info;

	char oldaprespn[2+1];

	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(trace, 0, sizeof(trace));
	memset(currentDate, 0, sizeof(currentDate));
	memset(&two_code_info, 0, sizeof(TWO_CODE_INFO));
	memset(&pep_jnls, 0, sizeof(PEP_JNLS));
	memset(oldaprespn, 0, sizeof(oldaprespn));
	time(&t);
	tm = localtime(&t);
	
	/*i_revdate从核心返回的信息中12域和13域中取得*/
	s=getbit(&iso, 13, (unsigned char *)pep_jnls.revdate);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_13 error,ERROR应答");
		return -1;
	}
	
	s=getbit(&iso, 15, (unsigned char *)pep_jnls.settlementdate);
	if(s<0)
	{
		dcs_log(0, 0, "bit_15 is null");
	}
	
	s=getbit(&iso, 12, (unsigned char *)pep_jnls.revdate+4);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_12 error,ERROR应答");
		return -1;
	}
	
    sprintf(currentDate, "%04d%s", tm->tm_year+1900, pep_jnls.revdate);
	
	s=getbit(&iso, 3, (unsigned char *)pep_jnls.process);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_3 error,ERROR应答");
		return -1;
	}
    
	s=getbit(&iso, 7, (unsigned char *)tmpbuf);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_7 error,ERROR应答");
		return -1;
	}
	else
	{
		sprintf(pep_jnls.pepdate, "%04d%4.4s", tm->tm_year+1900, tmpbuf);
		memcpy(pep_jnls.peptime, tmpbuf+4, 6);
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s=getbit(&iso, 11, (unsigned char *)tmpbuf);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_11 error,ERROR应答");
		return -1;
	}
	memcpy(trace, tmpbuf, s);
	sscanf(tmpbuf, "%06ld", &pep_jnls.trace);
	
	s = getbit(&iso, 37, (unsigned char *)pep_jnls.syseqno);
	if(s<0)
		dcs_log(0, 0, "get bit_37 error");
    
	s = getbit(&iso, 38, (unsigned char *)pep_jnls.authcode);
	if(s<0)
		dcs_log(0, 0, "get bit_38 error");
	
	s=getbit(&iso, 39, (unsigned char *)pep_jnls.aprespn);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_39 error,ERROR应答");
		return -1;
	}
	
	s=getbit(&iso, 41, (unsigned char *)pep_jnls.termcde);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_41 error,ERROR应答");
		return -1;
	}
	
	s=getbit(&iso, 42, (unsigned char *)pep_jnls.mercode);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_42 error");
		return -1;
	}
	memcpy(pep_jnls.revodid, "2", 1);
	
	s=getbit(&iso, 44, (unsigned char *)pep_jnls.filed44);
	if(s<0)
		dcs_log(0, 0, "get bit_44 error"); 

	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "SELECT aprespn, samid, merid, termtrc, modeflg, \
	sendcde, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), tranamt, trndate, batchno, postermcde, posmercode, trnmsgd, trntime, billmsg, trnsndp,\
	msgtype, termidm, modecde, trancde, translaunchway, AES_DECRYPT(UNHEX(intcdno), 'abcdefgh'), termid, outcdtype, camtlimit, damtlimit, trnsndpid, \
	termmeramtlimit, filed28, intcdbank, AES_DECRYPT(UNHEX(intcdname), 'abcdefgh'), card_sn, poscode, poitcde FROM pep_jnls where pepdate ='%s' and	 \
	peptime='%s' and process='%s' and trace='%ld' and termcde='%s' and mercode='%s';", pep_jnls.pepdate,\
	pep_jnls.peptime, pep_jnls.process, pep_jnls.trace, pep_jnls.termcde, pep_jnls.mercode);
	if(mysql_query(sock, sql)) {
	    #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "saveOrUpdatePepjnls 查找原笔交易出错 [%s]!", mysql_error(sock));
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
	    dcs_log(0, 0, "saveOrUpdatePepjnls:Couldn't get result from %s\n", mysql_error(sock));
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
		memcpy(oldaprespn, row[0] ? row[0] : "\0", (int)sizeof(oldaprespn));
		memcpy(pep_jnls.samid, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls.samid));
		if(row[2])
			pep_jnls.merid = atoi(row[2]);
		memcpy(pep_jnls.termtrc, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls.termtrc));
		memcpy(pep_jnls.modeflg, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls.modeflg));
		memcpy(pep_jnls.sendcde, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls.sendcde));
		memcpy(pep_jnls.outcdno, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls.outcdno));
		memcpy(pep_jnls.tranamt, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls.tranamt));
		memcpy(pep_jnls.trndate, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls.trndate));
		memcpy(pep_jnls.batch_no, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls.batch_no));
		memcpy(pep_jnls.postermcde, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls.postermcde));
		memcpy(pep_jnls.posmercode, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls.posmercode));
		memcpy(pep_jnls.trnmsgd, row[12] ? row[12] : "\0", (int)sizeof(pep_jnls.trnmsgd));
		memcpy(pep_jnls.trntime, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls.trntime));
		memcpy(pep_jnls.billmsg, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls.billmsg));
		memcpy(pep_jnls.trnsndp, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls.trnsndp));
		memcpy(pep_jnls.msgtype, row[16] ? row[16] : "\0", (int)sizeof(pep_jnls.msgtype));
		memcpy(pep_jnls.termidm, row[17] ? row[17] : "\0", (int)sizeof(pep_jnls.termidm));
		memcpy(pep_jnls.modecde, row[18] ? row[18] : "\0", (int)sizeof(pep_jnls.modecde));
		memcpy(pep_jnls.trancde, row[19] ? row[19] : "\0", (int)sizeof(pep_jnls.trancde));
		memcpy(pep_jnls.translaunchway, row[20] ? row[20] : "\0", (int)sizeof(pep_jnls.translaunchway));
		memcpy(pep_jnls.intcdno, row[21] ? row[21] : "\0", (int)sizeof(pep_jnls.intcdno));
		memcpy(pep_jnls.termid, row[22] ? row[22] : "\0", (int)sizeof(pep_jnls.termid));
		if(row[23])
			pep_jnls.outcdtype = row[23][0];
		if(row[24])
			pep_jnls.camtlimit = atoi(row[24]);
		if(row[25])
			pep_jnls.damtlimit = atoi(row[25]);
		if(row[26])
			pep_jnls.trnsndpid = atoi(row[26]);
		if(row[27])
			pep_jnls.termmeramtlimit = atoi(row[27]);
		memcpy(pep_jnls.filed28, row[28] ? row[28] : "\0", (int)sizeof(pep_jnls.filed28));
		memcpy(pep_jnls.intcdbank, row[29] ? row[29] : "\0", (int)sizeof(pep_jnls.intcdbank));
		memcpy(pep_jnls.intcdname, row[30] ? row[30] : "\0", (int)sizeof(pep_jnls.intcdname));
		memcpy(pep_jnls.card_sn, row[31] ? row[31] : "\0", (int)sizeof(pep_jnls.card_sn));
		memcpy(pep_jnls.poscode, row[32] ? row[32] : "\0", (int)sizeof(pep_jnls.poscode));
		memcpy(pep_jnls.poitcde, row[33] ? row[33] : "\0", (int)sizeof(pep_jnls.poitcde));
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "saveOrUpdatePepjnls：没有找到符合条件的记录");
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
		dcs_log(0, 0, "saveOrUpdatePepjnls：查到多条符合条件的记录");
		mysql_free_result(res);
		//从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return -1;
	}
	mysql_free_result(res);

	pep_jnls.termid[getstrlen(pep_jnls.termid)] = 0;
	pep_jnls.samid[getstrlen(pep_jnls.samid)] = 0;
    
#ifdef __LOG__
	dcs_log(0, 0, "pep_jnls.samid=[%s], pep_jnls.termid=[%s]", pep_jnls.samid, pep_jnls.termid);
	dcs_log(0, 0, "pep_jnls.merid=[%d], pep_jnls.billmsg=[%s]", pep_jnls.merid, pep_jnls.billmsg);
	dcs_log(0, 0, "pep_jnls.trnsndp=[%s], pep_jnls.outcdtype=[%c]", pep_jnls.trnsndp, pep_jnls.outcdtype);
	dcs_log(0, 0, "pep_jnls.camtlimit=[%d], pep_jnls.damtlimit=[%d]", pep_jnls.camtlimit, pep_jnls.damtlimit);
	dcs_log(0, 0, "pep_jnls.trnsndpid=[%d], pep_jnls.termmeramtlimit=[%d]", pep_jnls.trnsndpid, pep_jnls.termmeramtlimit);
	dcs_log(0, 0, "Update pep_jnls aprespn=[%s], pep_jnls authcode=[%s]", pep_jnls.aprespn, pep_jnls.authcode);
	dcs_log(0, 0, "Update pep_jnls revodid=[%s], pep_jnls syseqno=[%s]", pep_jnls.revodid, pep_jnls.syseqno);
	dcs_log(0, 0, "Update pep_jnls revdate = [%s], pep_jnls pepdate = [%s]", pep_jnls.revdate, pep_jnls.pepdate);
	dcs_log(0, 0, "Update pep_jnls peptime = [%s], pep_jnls process = [%s]", pep_jnls.peptime, pep_jnls.process);
	dcs_log(0, 0, "Update pep_jnls termcde = [%s], pep_jnls trace = [%ld]", pep_jnls.termcde, pep_jnls.trace);
	dcs_log(0, 0, "Update pep_jnls mercode = [%s], pep_jnls settlementdate= [%s]", pep_jnls.mercode, pep_jnls.settlementdate);
	dcs_log(0, 0, "Update pep_jnls filed44 = [%s]", pep_jnls.filed44);
	dcs_log(0, 0, "Update pep_jnls.intcdname = [%s]", pep_jnls.intcdname);
#endif
#ifdef __TEST__
	//模拟测试test
	if( memcmp(pep_jnls.trancde, "050",3) == 0)
	{
		memcpy(oldaprespn, "N1", 2);
	}
#endif	
	/*如果是脱机消费上送交易,需要判断是否将交易结果通知其他系统*/
	if( (memcmp(pep_jnls.trancde, "050",3) == 0 ||memcmp(pep_jnls.trancde, "211",3) == 0)&& memcmp(pep_jnls.aprespn, "00", 2) == 0
			&& strlen(oldaprespn) != 0 &&memcmp(pep_jnls.aprespn,oldaprespn, 2)!= 0 )
	{
		DealOfflineSaleResultNotice(pep_jnls);
	}
	
	/*二维码支付,在支付成功之后通知到商城支付成功*/
	if(memcmp(pep_jnls.trancde, "M01", 3) == 0 && memcmp(pep_jnls.aprespn, "00", 2) == 0)
	{
		GetTwoInfo(&two_code_info, pep_jnls.billmsg);
		SendPayResult(two_code_info, trace, currentDate);
	}
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "UPDATE pep_jnls set aprespn='%s', authcode = '%s', revodid='%s', syseqno='%s',\
	revdate='%s', settlementdate='%s', filed44='%s' where pepdate ='%s' and peptime='%s' \
	and process='%s' and trace='%ld' and termcde='%s' and mercode='%s';", pep_jnls.aprespn, pep_jnls.authcode, pep_jnls.revodid, \
	pep_jnls.syseqno, pep_jnls.revdate, pep_jnls.settlementdate, pep_jnls.filed44, pep_jnls.pepdate, pep_jnls.peptime, pep_jnls.process, pep_jnls.trace, pep_jnls.termcde, pep_jnls.mercode);

	if(mysql_query(sock, sql)) {
	    #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "Update pep_jnls DB  error [%s]!", mysql_error(sock));
	    //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
	    return -1;
	}
	//从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
	memcpy(opep_jnls, &pep_jnls, sizeof(PEP_JNLS));
	return 0;
}

/*
 输入参数：
 cardno：银行卡号

 返回参数：
 'C':信用卡
 'D':借记卡
 '-'：不存在
 */
char ReadCardType(char* cardno)
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
	int  len ;
    
	memset(cdhd5, 0, sizeof(cdhd5));
    memset(cdhd6, 0, sizeof(cdhd6));
    memset(cdhd7, 0, sizeof(cdhd7));
    memset(cdhd8, 0, sizeof(cdhd8));
    memset(cdhd9, 0, sizeof(cdhd9));
    memset(cdhd10, 0, sizeof(cdhd10));
    memset(cdhd11, 0, sizeof(cdhd11));
    memset(cdhd12, 0, sizeof(cdhd12));
    
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
    sprintf(sql, "SELECT card_type FROM card_transfer where card_head IN('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s') \
	and  card_no_length = %d order by card_length desc limit 1;", cdhd5, cdhd6, cdhd7, cdhd8, cdhd9, cdhd10, cdhd11, cdhd12, len);

    if(mysql_query(sock, sql)) {
      #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "icheck card_transfer fail [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "icheck card_transfer : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    
	num_rows = mysql_num_rows(res);
    if (num_rows == 1) {
#ifdef __LOG__
    	dcs_log(0, 0, "icheck card_transfer ======= %s", row[0]);
#endif
        ctype = row[0][0];
    } 
	else if(num_rows == 0)
	{
		dcs_log(0, 0, "ReadCardType：没有找到符合条件的记录");
		mysql_free_result(res);  
		return '-';
	}
	else if(num_rows >1)
	{
		dcs_log(0, 0, "ReadCardType：查到多条符合条件的记录");
		mysql_free_result(res);  
		return '-';
	}
    mysql_free_result(res);
  
    if(ctype == 'C')
		return 'C';
    else if(ctype == 'D')
		return 'D';
    else
    	return '-';
}

/*
 每条从EWP过来的交易数据，都需要保存在ewp_jnls数据库表中。返回EWP的时候，需要取其中的数据。
 插入成功：return 0
 插入失败：return -1
 */
int WriteDbEwpInfo(EWP_INFO ewp_info)
{
    char orderno[20+1];
    memset(orderno, 0, sizeof(orderno));
	
	if(memcmp(ewp_info.consumer_sentinstitu, "0700", 4)==0)
    	 memcpy(ewp_info.consumer_psamno, "0700000000000000", 16);
    
	#ifdef __LOG__
		dcs_log(0, 0, "ewp_info.consumer_orderno=[%s]", ewp_info.consumer_orderno);
		dcs_log(0, 0, "ewp_info.consumer_psamno=[%s]", ewp_info.consumer_psamno);
	#endif
	
    memcpy(orderno, ewp_info.consumer_orderno, strlen(ewp_info.consumer_orderno));
     
    char sql[1024];
    memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "INSERT INTO ewp_info (TRANSTYPE, TRANSDEALCODE, SENTDATE, SENTTIME, SENTINSTITU, \
	TRANSCODE, ORDERNO, PSAMID, TRANSLAUNCHWAY, SELFDEFINE, INCDNAME, INCDBKNO, TRANSFEE, tpduheader, poitcde_flag) VALUES ( '%s',\
	'%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s');", ewp_info.consumer_transtype, \
	ewp_info.consumer_transdealcode, ewp_info.consumer_sentdate, ewp_info.consumer_senttime, \
	ewp_info.consumer_sentinstitu, ewp_info.consumer_transcode, orderno, ewp_info.consumer_psamno, \
	ewp_info.translaunchway, ewp_info.selfdefine, ewp_info.incdname, ewp_info.incdbkno, ewp_info.transfee, ewp_info.tpduheader, \
	ewp_info.poitcde_flag);
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "insert ewp_info DB error [%s]!", mysql_error(sock));
        return -1;
    }	
	return 0;
}

/*从数据表ewp_info根据订单号等字段查找某笔交易的交易处理码*/
int RdDealcodebyOrderNo(PEP_JNLS pep_jnls, EWP_INFO *ewp_info )
{
    EWP_INFO ewp_tempinfo;
	char sql[1024];
	int len = 0;
	
	memset(&ewp_tempinfo, 0, sizeof(EWP_INFO));
    memset(sql, 0x00, sizeof(sql));
	
	pub_rtrim_lp(pep_jnls.billmsg, strlen(pep_jnls.billmsg), pep_jnls.billmsg, 1);
	if(memcmp(pep_jnls.sendcde, "0700", 4)==0)
	{
		memcpy(pep_jnls.samid, "0700000000000000", 16);
		pep_jnls.samid[16]=0;
	}
		
	len = getstrlen(pep_jnls.samid);
	pep_jnls.samid[len]=0;
	
	#ifdef __LOG__
		dcs_log(0,0,"pep_jnls.trndate=[%s]",pep_jnls.trndate);
		dcs_log(0,0,"pep_jnls.trntime=[%s]",pep_jnls.trntime);
		dcs_log(0,0,"pep_jnls.billmsg=[%s]",pep_jnls.billmsg);
		dcs_log(0,0,"pep_jnls.samid=[%s]",pep_jnls.samid);
	#endif
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
    sprintf(sql, "SELECT transdealcode, translaunchway, selfdefine, incdname, incdbkno, transfee, psamid, tpduheader, sentinstitu, poitcde_flag, transtype \
	FROM ewp_info where sentdate = '%s'  and senttime = '%s'  and (orderno = '%s' or orderno is null or orderno ='') and psamid = '%s';", \
	pep_jnls.trndate, pep_jnls.trntime, pep_jnls.billmsg, pep_jnls.samid);
	
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "select ewp_info error = [%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
    
    MYSQL_RES *res; 
	MYSQL_ROW row ; 
	int num_fields;
	int num_rows = 0;
	
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "RdDealcodebyOrderNo:Couldn't get result from %s\n", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
    //从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
        memcpy(ewp_tempinfo.consumer_transdealcode, row[0] ? row[0] : "\0", (int)sizeof(ewp_tempinfo.consumer_transdealcode));
        memcpy(ewp_tempinfo.translaunchway, row[1] ? row[1] : "\0", (int)sizeof(ewp_tempinfo.translaunchway));
        memcpy(ewp_tempinfo.selfdefine, row[2] ? row[2] : "\0", (int)sizeof(ewp_tempinfo.selfdefine));
        memcpy(ewp_tempinfo.incdname, row[3] ? row[3] : "\0", (int)sizeof(ewp_tempinfo.incdname));
        memcpy(ewp_tempinfo.incdbkno, row[4] ? row[4] : "\0", (int)sizeof(ewp_tempinfo.incdbkno));
        memcpy(ewp_tempinfo.transfee, row[5] ? row[5] : "\0", (int)sizeof(ewp_tempinfo.transfee));
        memcpy(ewp_tempinfo.consumer_psamno, row[6] ? row[6] : "\0", (int)sizeof(ewp_tempinfo.consumer_psamno));
		memcpy(ewp_tempinfo.tpduheader, row[7] ? row[7] : "\0", (int)sizeof(ewp_tempinfo.tpduheader));
        memcpy(ewp_tempinfo.consumer_sentinstitu, row[8] ? row[8] : "\0", (int)sizeof(ewp_tempinfo.consumer_sentinstitu));
		memcpy(ewp_tempinfo.poitcde_flag, row[9] ? row[9] : "\0", (int)sizeof(ewp_tempinfo.poitcde_flag));
	 	memcpy(ewp_tempinfo.consumer_transtype, row[10] ? row[10] : "\0", (int)sizeof(ewp_tempinfo.consumer_transtype));
    }
	else if(num_rows==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "RdDealcodebyOrderNo：未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows >1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "RdDealcodebyOrderNo：找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);
	memcpy(ewp_info, &ewp_tempinfo, sizeof(EWP_INFO));
	
	dcs_log(0, 0, "ewp_info->consumer_transdealcode=%s\n ", ewp_info->consumer_transdealcode);
	return 0;
}

/*
 通过原始交易日期和时间、原始交易卡号、原始交易流水、原始交易系统参考号、
 原始交易授权码、原始交易订单号等(而且要是当日，当台设备 并且是消费成功的交易)字段查询pep_jnls数据库表
 */
int GetOriginalTrans(EWP_INFO ewp_info, PEP_JNLS *opep_jnls)
{	
	PEP_JNLS pep_jnls;
	char i_carno[19+1];
	long i_tranwater;
	char i_orderno[40+1];
	char i_sysreferno[12+1];
	char sql[1024];
	
	memset(&pep_jnls, 0, sizeof(PEP_JNLS));
	memset(i_carno, 0, sizeof(i_carno));
	memset(i_orderno, 0, sizeof(i_orderno));
	memset(i_sysreferno, 0, sizeof(i_sysreferno));
	memset(sql, 0x00, sizeof(sql));
	
	memcpy(i_carno, ewp_info.consumer_cardno, getstrlen(ewp_info.consumer_cardno));
	i_tranwater = atol(ewp_info.consumer_presyswaterno);
	memcpy(i_orderno, ewp_info.consumer_orderno, getstrlen(ewp_info.consumer_orderno));
	memcpy(i_sysreferno, ewp_info.consumer_sysreferno, getstrlen(ewp_info.consumer_sysreferno));
	
	#ifdef __LOG__
		dcs_log(0, 0, "原笔交易查询，流水=[%ld]", i_tranwater);
		dcs_log(0, 0, "原笔交易查询，订单号=[%s]", i_orderno);
		dcs_log(0, 0, "原笔交易查询，参考号=[%s]", i_sysreferno);
	#endif
	
	sprintf(sql, "SELECT trim(samid), trim(authcode), syseqno, trim(revdate), trim(translaunchway),AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), \
		outcdtype, tranamt, termtrc, postermcde, posmercode, billmsg, trim(modecde), \
		modeflg, termcde, trim(mercode), termidm, trim(intcdbank), trim(AES_DECRYPT(UNHEX(intcdname), 'abcdefgh')), trim(AES_DECRYPT(UNHEX(intcdno), 'abcdefgh')), sendcde FROM pep_jnls \
		where outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and trace = %ld and billmsg = '%s' and (syseqno = '%s' or syseqno is null or syseqno ='') and aprespn != 'A3';",\
		i_carno, i_tranwater, i_orderno, i_sysreferno); 
	
	 if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "select ewp_info error = [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res; 
	MYSQL_ROW row ; 
	int num_fields;
	int num_rows = 0;
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "msgchg_info:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
        memcpy(pep_jnls.samid, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls.samid));
        memcpy(pep_jnls.authcode, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls.authcode));
        memcpy(pep_jnls.syseqno, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls.syseqno));
        memcpy(pep_jnls.revdate, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls.revdate));
        memcpy(pep_jnls.translaunchway, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls.translaunchway));
        memcpy(pep_jnls.outcdno, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls.outcdno));
		if(row[6])
			pep_jnls.outcdtype = row[6][0];
		memcpy(pep_jnls.tranamt, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls.tranamt));
		memcpy(pep_jnls.termtrc, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls.termtrc));
		memcpy(pep_jnls.postermcde, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls.postermcde));
		memcpy(pep_jnls.posmercode, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls.posmercode));

		memcpy(pep_jnls.billmsg, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls.billmsg));
		memcpy(pep_jnls.modecde, row[12] ? row[12] : "\0", (int)sizeof(pep_jnls.modecde));
		memcpy(pep_jnls.modeflg, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls.modeflg));
		memcpy(pep_jnls.termcde, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls.termcde));
		memcpy(pep_jnls.mercode, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls.mercode));
		memcpy(pep_jnls.termidm, row[16] ? row[16] : "\0", (int)sizeof(pep_jnls.termidm));
		memcpy(pep_jnls.intcdbank, row[17] ? row[17] : "\0", (int)sizeof(pep_jnls.intcdbank));
		memcpy(pep_jnls.intcdname, row[18] ? row[18] : "\0", (int)sizeof(pep_jnls.intcdname));
		memcpy(pep_jnls.intcdno, row[19] ? row[19] : "\0", (int)sizeof(pep_jnls.intcdno));
		memcpy(pep_jnls.sendcde, row[20] ? row[20] : "\0", (int)sizeof(pep_jnls.sendcde));
    }
	else if(num_rows==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOriginalTrans：未找到原笔记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows >1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOriginalTrans：找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);
	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls.tranamt=[%s], pep_jnls.syseqno=[%s]", pep_jnls.tranamt, pep_jnls.syseqno);
		dcs_log(0, 0, "ewp_info.consumer_sysreferno=[%s], pep_jnls.sendcde=[%s]", ewp_info.consumer_sysreferno, pep_jnls.sendcde);
	#endif
	memcpy(opep_jnls, &pep_jnls, sizeof(PEP_JNLS));
	dcs_log(0, 0, "opep_jnls->tranamt=[%s]", opep_jnls->tranamt);
	return 0;
}

/*根据订单号查询pep_jnls*/
int GetInquiryInfo(EWP_INFO ewp_info, PEP_JNLS *opep_jnls)
{
    PEP_JNLS pep_jnls;
    char i_orderno[20+1];
	char sql[1024];
	
	memset(&pep_jnls, 0, sizeof(PEP_JNLS));
	memset(i_orderno, 0, sizeof(i_orderno));
	memset(sql, 0x00, sizeof(sql));   
	
	memcpy(i_orderno, ewp_info.consumer_orderno, strlen(ewp_info.consumer_orderno));
	
	#ifdef __LOG__
		dcs_log(0, 0, "ewp_info.consumer_orderno=[%s]", ewp_info.consumer_orderno);
		dcs_log(0, 0, "i_orderno=[%s]", i_orderno);
	#endif 
    
    sprintf(sql, "SELECT  aprespn, samid, merid, authcode, syseqno, sendcde, process, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), tranamt, trndate, trace,\
	trntime, billmsg, trnsndp, msgtype ,termidm, modecde,trancde FROM pep_jnls where  billmsg = '%s' and \
	date_format(CONCAT(pepdate, peptime), '%s') = (select max(date_format(CONCAT(pepdate, peptime), '%s')) \
	from pep_jnls where billmsg = '%s')", i_orderno, "%Y%m%d%H%i%s", "%Y%m%d%H%i%s", i_orderno);
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "Inquiry the transaction error [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetInquiryInfo:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
    
    if(num_rows == 1) {
        memcpy(pep_jnls.aprespn, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls.aprespn));
        memcpy(pep_jnls.samid, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls.samid));
        if (row[2]) 
			pep_jnls.merid = atoi(row[2]);
        memcpy(pep_jnls.authcode, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls.authcode));
        memcpy(pep_jnls.syseqno, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls.syseqno));
        memcpy(pep_jnls.sendcde, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls.sendcde));
        memcpy(pep_jnls.process, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls.process));
        memcpy(pep_jnls.outcdno, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls.outcdno));
        memcpy(pep_jnls.tranamt, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls.tranamt));
        memcpy(pep_jnls.trndate, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls.trndate));
        if (row[10]) 
			pep_jnls.trace = atol(row[10]);
        memcpy(pep_jnls.trntime, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls.trntime));
        memcpy(pep_jnls.billmsg, row[12] ? row[12] : "\0", (int)sizeof(pep_jnls.billmsg));
        memcpy(pep_jnls.trnsndp, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls.trnsndp));
        memcpy(pep_jnls.msgtype, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls.msgtype));
        memcpy(pep_jnls.termidm, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls.termidm));
        memcpy(pep_jnls.modecde, row[16] ? row[16] : "\0", (int)sizeof(pep_jnls.modecde));
        memcpy(pep_jnls.trancde, row[17] ? row[17] : "\0", (int)sizeof(pep_jnls.trancde));
    }
    else if(num_rows==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetInquiryInfo :未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows >1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetInquiryInfo :找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	
    mysql_free_result(res); 
	memcpy(opep_jnls, &pep_jnls, sizeof(PEP_JNLS));
	return 0;
}

/*把交易错误的信息保存到数据表xpep_errtrade_jnls中*/
int errorTradeLog(EWP_INFO ewp_info, char *folderName, char *errdetail)
{
    char i_e1code[16];
    char i_error[64];
	char sql[1024];
	
    memset(i_e1code, 0, sizeof(i_e1code));
    memset(i_error, 0, sizeof(i_error));
    memset(sql, 0x00, sizeof(sql));
	
	memcpy(i_e1code, folderName, strlen(folderName));
    memcpy(i_error, errdetail, strlen(errdetail));
    
    sprintf(sql, "INSERT INTO xpep_errtrade_jnls (TERM_DATE, TERM_TEL, TERM_PSAM, TERM_E1CODE, TERM_ERROR, \
	TERM_TIME, TERM_TRACE, TERM_RESP, TERM_TRANCDE) VALUES('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s'); ",\
	ewp_info.consumer_sentdate, ewp_info.consumer_username, ewp_info.consumer_psamno, i_e1code, i_error, \
	ewp_info.consumer_senttime, ewp_info.consumer_termwaterno, ewp_info.consumer_responsecode, ewp_info.consumer_transcode);
    
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "errorTradeLog:Query failed (%s)\n", mysql_error(sock));
        return -1;
    }
	return 0;
}

/*
 风控：根据商户号查询  商户黑名单数据表
 输入参数：
 mercode：商户号
 trancde: 交易码

 返回参数：
 -1：数据库操作失败
 1：商户和交易是黑名单
 0：商户或者交易不是黑名单
 */
int  checkMerBlank(char *mercode, char *trancde)
{
    int count; 
    char sql[512];
    memset(sql, 0x00, sizeof(sql));
	
    sprintf(sql, "SELECT count(*) FROM blackmer_info where mercode= '%s' and (trancdelist = '*' or \
	instr(trancdelist, '%s') <= 0) and date_format(enddate, '%s') >= date_format(sysdate(), '%s') \
	 and date_format(adddate, '%s') <= date_format(sysdate(), '%s');",\
	mercode, trancde, "%Y%m%d", "%Y%m%d", "%Y%m%d", "%Y%m%d");
        
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "Inquiry the blackmer_info error [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
    
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "checkMerBlank:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    
    if(mysql_num_rows(res) == 1) {
        count = atoi(row[0]);
    }
	else
	{
		dcs_log(0, 0, "checkMerBlank，操作失败");
		mysql_free_result(res);
		return -1;
	}
    
    mysql_free_result(res);
    
	if(count > 0)
		return 1;
	else
		return 0;
}

/*
 输入参数：
 trancde：交易码
 channel_id：终端投资方Id
 
 返回参数：
 0：成功
 -1：该渠道该交易不存在
 -2：该渠道限制交易
 -3：该渠道该交易未开通
 */
int ChannelTradeCharge(char *trancde, long channel_id)
{
    int branch_restrict_trade;
    int trade_restrict_trade;
    char i_trancde[3+1];
	char sql[512];
	 
    branch_restrict_trade = -1;
    trade_restrict_trade = -1;
    
	memset(sql, 0x00, sizeof(sql));
    memset(i_trancde, 0, sizeof(i_trancde));
    memcpy(i_trancde, trancde, 3);
   
    sprintf(sql, "SELECT t1.restrict_trade, t2.restrict_trade FROM samcard_channel_info t1, \
	samcardchannel_to_msgchginfo t2 where t1.channel_id= '%ld' and t1.channel_id = t2.channel_id \
	and t2.trancde = '%s';", channel_id, i_trancde);
    
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "ChannelTradeCharge error [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "ChannelTradeCharge : Couldn't get result from %s\n", mysql_error(sock));   
        return -1;
    }
    
    num_fields = mysql_num_fields(res);  
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
        branch_restrict_trade = atoi(row[0]);
        trade_restrict_trade = atoi(row[1]);     
    }
	else if(num_rows ==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "ChannelTradeCharge:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows >1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "ChannelTradeCharge:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	
    mysql_free_result(res);
    
	#ifdef __LOG__
		dcs_log(0, 0, "branch_restrict_trade=[%d]", branch_restrict_trade);
		dcs_log(0, 0, "trade_restrict_trade=[%d]", trade_restrict_trade);
	#endif
	
	if(branch_restrict_trade == 1 || branch_restrict_trade == -1)
		return -2;
	else if(trade_restrict_trade == 1 || trade_restrict_trade == -1)
		return -3;
	else
		return 0;
}
/*
 输入参数：
 sam_status：psam卡的状态
 输出参数：
 ret_psam_code：当前的psam卡状态的详细信息对应的返回给EWP的应答码
 
 */
int GetPsamStatusInfo(char *sam_status, SAMCARD_STATUS_INFO *osamcard_status_info)
{
    SAMCARD_STATUS_INFO samcard_status_info;
	char sql[512];
	
	memset(&samcard_status_info, 0, sizeof(SAMCARD_STATUS_INFO));
	memset(sql, 0x00, sizeof(sql));
    
	#ifdef __LOG__
		dcs_log(0,0,"sam_status =[%s]" ,sam_status);
	#endif

    sprintf(sql, "SELECT samcard_status_code, samcard_status_desc, xpep_ret_code FROM samcard_status \
	where samcard_status_code = '%s';",  sam_status);
    
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetPsamStatusInfo error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetPsamStatusInfo : Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
    
    if(num_rows == 1) {
        memcpy(samcard_status_info.status_code, row[0] ? row[0] : "\0", (int)sizeof(samcard_status_info.status_code));
        memcpy(samcard_status_info.status_desc, row[1] ? row[1] : "\0", (int)sizeof(samcard_status_info.status_desc));
        memcpy(samcard_status_info.xpep_ret_code, row[2] ? row[2] : "\0", (int)sizeof(samcard_status_info.xpep_ret_code));
    }
    
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPsamStatusInfo:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows >1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPsamStatusInfo:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);
    
	#ifdef __LOG__
		dcs_log(0, 0, "samcard_status_info.status_code=[%s]", samcard_status_info.status_code);
		dcs_log(0, 0, "samcard_status_info.status_desc=[%s]", samcard_status_info.status_desc);
		dcs_log(0, 0, "samcard_status_info.xpep_ret_code=[%s]", samcard_status_info.xpep_ret_code);
	#endif
	
	memcpy(osamcard_status_info, &samcard_status_info, sizeof(SAMCARD_STATUS_INFO));
	
	return 0;
}

/*
 函数功能：通过samNo表查询该终端是否需要风险控制

 输入参数：
 currentDate：当前日期
 samNo：psam卡号
 
 返回结果：
 samcard_trade_restric对象
 */
int GetSamcardRestrictInfo(char *currentDate, char *samNo, SAMCARD_TRADE_RESTRICT *samcard_trade_restric)
{
    SAMCARD_TRADE_RESTRICT i_samcard_trade_restric;
    char samcno[16+1];
	char sql[512];
    int count;
	count = 0;
	
	memset(&i_samcard_trade_restric, 0, sizeof(SAMCARD_TRADE_RESTRICT));
	memset(samcno, 0, sizeof(samcno));
	memset(sql, 0x00, sizeof(sql));
    
	memcpy(samcno, samNo, 16);
    
	#ifdef __LOG__
		dcs_log(0, 0, "GetSecuInfoTable samNo=%s", samNo);
	#endif
    
    sprintf(sql, "SELECT count(*) FROM samcard_trade_restrict WHERE psam_card = '%s';", samcno);
    
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "samcard_trade_restrict DB error [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "samcard_trade_restrict DB : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
	
    if(num_rows == 1) {
        count = atoi(row[0]);
    }
    else if(num_rows ==0)
	{
		dcs_log(0, 0, "samcard_trade_restrict，操作失败");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);
    
	if(count == 0){
		samcard_trade_restric->psam_restrict_flag = 0;
		return 0;
	}
	else{
		i_samcard_trade_restric.psam_restrict_flag = 1;
	}
    
    memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "SELECT psam_card, restrict_trade, restrict_card, passwd, amtmin, amtmax, c_amttotal, \
	c_amtdate, lpad(rtrim(c_total_tranamt), 12, '0'), d_amttotal, d_amtdate , lpad(rtrim(d_total_tranamt), 12, '0') \
	FROM samcard_trade_restrict WHERE psam_card = '%s';", samcno);
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "samcard_trade_restrict DB error [%s]!", mysql_error(sock));
        return -1;
    }
    MYSQL_RES *res2;
    if (!(res2 = mysql_store_result(sock))) {
        dcs_log(0, 0, "samcard_trade_restrict : Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res2);
    row = mysql_fetch_row(res2);
    num_rows = mysql_num_rows(res2);
    if(num_rows == 1) {
        memcpy(i_samcard_trade_restric.psam_card, row[0] ? row[0] : "\0", (int)sizeof(i_samcard_trade_restric.psam_card));
        memcpy(i_samcard_trade_restric.restrict_trade, row[1] ? row[1] : "\0", (int)sizeof(i_samcard_trade_restric.restrict_trade));     
        i_samcard_trade_restric.restrict_card =row[2][0];
	 i_samcard_trade_restric.passwd = row[3][0];
        memcpy(i_samcard_trade_restric.amtmin, row[4] ? row[4] : "\0", (int)sizeof(i_samcard_trade_restric.amtmin));
        memcpy(i_samcard_trade_restric.amtmax, row[5] ? row[5] : "\0", (int)sizeof(i_samcard_trade_restric.amtmax));
        memcpy(i_samcard_trade_restric.c_amttotal, row[6] ? row[6] : "\0", (int)sizeof(i_samcard_trade_restric.c_amttotal));
        memcpy(i_samcard_trade_restric.c_amtdate, row[7] ? row[7] : "\0", (int)sizeof(i_samcard_trade_restric.c_amtdate));
        memcpy(i_samcard_trade_restric.c_total_tranamt, row[8] ? row[8] : "\0", (int)sizeof(i_samcard_trade_restric.c_total_tranamt)); 
        memcpy(i_samcard_trade_restric.d_amttotal, row[9] ? row[9] : "\0", (int)sizeof(i_samcard_trade_restric.d_amttotal));
        memcpy(i_samcard_trade_restric.d_amtdate, row[10] ? row[10] : "\0", (int)sizeof(i_samcard_trade_restric.d_amtdate));
        memcpy(i_samcard_trade_restric.d_total_tranamt, row[11] ? row[11] : "\0", (int)sizeof(i_samcard_trade_restric.d_total_tranamt));
    }
	else if(num_rows ==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetSamcardRestrictInfo：未找到符合条件的记录");
		mysql_free_result(res2);
		return -1;
	}
	else if(num_rows >1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetSamcardRestrictInfo ：找到多条符合条件的记录");
		mysql_free_result(res2);
		return -1;
	}
    mysql_free_result(res2);
	if(memcmp(i_samcard_trade_restric.c_amttotal, "-", 1)!=0 && memcmp(i_samcard_trade_restric.c_amtdate, currentDate, 8) != 0)
	{
        memset(sql, 0x00, sizeof(sql));
        sprintf(sql, "update samcard_trade_restrict set c_amtdate = '%s', c_total_tranamt = '000000000000' \
		where psam_card = '%s';", currentDate, samcno);
        if(mysql_query(sock, sql)) {
            #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
            dcs_log(0, 0, "update samcard_trade_restrict DB error [%s]!", mysql_error(sock));
            return -1;
        }
		memcpy(i_samcard_trade_restric.c_total_tranamt, "000000000000", 12);
		memcpy(i_samcard_trade_restric.c_amtdate, currentDate, 8);
	}
    
	if( memcmp(i_samcard_trade_restric.d_amttotal, "-", 1)!=0 && memcmp(i_samcard_trade_restric.d_amtdate, currentDate, 8) != 0)
	{   
        memset(sql, 0x00, sizeof(sql));
        sprintf(sql, "update samcard_trade_restrict set d_amtdate = '%s', d_total_tranamt = '000000000000'\
		where psam_card = '%s';", currentDate, samcno);
        
        if(mysql_query(sock, sql)) {
            #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
            dcs_log(0, 0, "update samcard_trade_restrict DB error 3[%s]!", mysql_error(sock));
            return -1;
        }
        
		memcpy(i_samcard_trade_restric.d_total_tranamt, "000000000000", 12);
		memcpy(i_samcard_trade_restric.d_amtdate, currentDate, 8);
	}
	#ifdef __LOG__
		dcs_log(0, 0, "i_samcard_trade_restric.restrict_trade = %s", i_samcard_trade_restric.restrict_trade);
		dcs_log(0, 0, "i_samcard_trade_restric.restrict_card = %c", i_samcard_trade_restric.restrict_card);
		dcs_log(0, 0, "i_samcard_trade_restric.passwd = %c", i_samcard_trade_restric.passwd);
		dcs_log(0, 0, "i_samcard_trade_restric.amtmin = [%s]", i_samcard_trade_restric.amtmin);
		dcs_log(0, 0, "i_samcard_trade_restric.amtmax = [%s]", i_samcard_trade_restric.amtmax);
		dcs_log(0, 0, "i_samcard_trade_restric.c_amttotal = [%s]", i_samcard_trade_restric.c_amttotal);
		dcs_log(0, 0, "i_samcard_trade_restric.c_total_tranamt = [%s]", i_samcard_trade_restric.c_total_tranamt);
		dcs_log(0, 0, "i_samcard_trade_restric.c_amtdate = [%s]", i_samcard_trade_restric.c_amtdate);
		dcs_log(0, 0, "i_samcard_trade_restric.d_amttotal = [%s]", i_samcard_trade_restric.d_amttotal);
		dcs_log(0, 0, "i_samcard_trade_restric.d_total_tranamt = [%s]", i_samcard_trade_restric.d_total_tranamt);
		dcs_log(0, 0, "i_samcard_trade_restric.d_amtdate = [%s]", i_samcard_trade_restric.d_amtdate);
	#endif
	memcpy(samcard_trade_restric, &i_samcard_trade_restric, sizeof(SAMCARD_TRADE_RESTRICT));
	
	return 1;
}


/*
 函数功能：通过psam,mercode表查询该终端该商户是否需要风险控制

 输入参数：
 currentDate：当前日期
 psam：psam卡号
 mercode:商户号

 返回结果：
 SAMCARD_MERTRADE_RESTRICT对象
 */
int GetSamcardMerRestrictInfo(char *currentDate, char *psam, char *mercode, SAMCARD_MERTRADE_RESTRICT *samcard_mertrade_restric)
{
    SAMCARD_MERTRADE_RESTRICT i_samcard_mertrade_restric;
    char i_psam[16+1];
    char i_mercode[15+1];
    int count;
	char sql[512];
	
	count = 0;
	memset(&i_samcard_mertrade_restric, 0, sizeof(SAMCARD_MERTRADE_RESTRICT));
	memset(i_psam, 0, sizeof(i_psam));
	memset(i_mercode, 0, sizeof(i_mercode));
	memset(sql, 0x00, sizeof(sql));
	
	memcpy(i_psam, psam, 16);
	memcpy(i_mercode, mercode, 15);
    
	#ifdef __LOG__
		dcs_log(0, 0, "GetSamcardRestrictInfo i_psam=[%s], i_mercode=[%s]", i_psam, i_mercode);
	#endif
 
    sprintf(sql, "SELECT count(*) FROM samcard_mertrade_restrict WHERE psam_card = '%s' and mercode = '%s';", i_psam, i_mercode);
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "samcard_mertrade_restrict DB error [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "samcard_mertrade_restrict :Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);  
    row = mysql_fetch_row(res);
    
    if (mysql_num_rows(res) == 1) {
        count = atoi(row[0]);
    }
    else 
	{
		dcs_log(0, 0, "GetSamcardMerRestrictInfo:操作数据库失败");
		mysql_free_result(res);
		return -1;
		
	}
    mysql_free_result(res);
    
	if(count == 0){
		samcard_mertrade_restric->psammer_restrict_flag = 0;
		return 0;
	}else{
		i_samcard_mertrade_restric.psammer_restrict_flag = 1;
	}
    
    memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "SELECT psam_card, mercode, c_amttotal, c_amtdate, lpad(rtrim(c_total_tranamt), 12, '0')\
	FROM samcard_mertrade_restrict WHERE psam_card = '%s' and mercode = '%s';", i_psam, i_mercode);
    
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "samcard_mertrade_restrict DB error2 [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res2;
    
    if (!(res2 = mysql_store_result(sock))) {
        dcs_log(0, 0, "samcard_mertrade_restrict :Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res2); 
    row = mysql_fetch_row(res2);
    
	num_rows = mysql_num_rows(res2);
    if (num_rows == 1) {
        memcpy(i_samcard_mertrade_restric.psam_card, row[0] ? row[0] : "\0", (int)sizeof(i_samcard_mertrade_restric.psam_card));
        memcpy(i_samcard_mertrade_restric.mercode, row[1] ? row[1] : "\0", (int)sizeof(i_samcard_mertrade_restric.mercode));
        memcpy(i_samcard_mertrade_restric.c_amttotal, row[2] ? row[2] : "\0", (int)sizeof(i_samcard_mertrade_restric.c_amttotal));
        memcpy(i_samcard_mertrade_restric.c_amtdate, row[3] ? row[3] : "\0", (int)sizeof(i_samcard_mertrade_restric.c_amtdate));
        memcpy(i_samcard_mertrade_restric.c_total_tranamt, row[4] ? row[4] : "\0", (int)sizeof(i_samcard_mertrade_restric.c_total_tranamt));
    }
    else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetSamcardMerRestrictInfo:未找到符合条件的记录");
		mysql_free_result(res2); 
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetSamcardMerRestrictInfo:找到多条符合条件的记录");
		mysql_free_result(res2); 
		return -1;
	}
    mysql_free_result(res2); 
	if(memcmp(i_samcard_mertrade_restric.c_amttotal, "-", 1)!=0 && memcmp(i_samcard_mertrade_restric.c_amtdate, currentDate, 8) != 0)
	{
        memset(sql, 0x00, sizeof(sql));
        sprintf(sql, "update samcard_mertrade_restrict set c_amtdate = '%s', c_total_tranamt = '000000000000'\
		WHERE psam_card = '%s' and mercode = '%s';", currentDate, i_psam, i_mercode);
        
        if(mysql_query(sock, sql)) {
            #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
            dcs_log(0, 0, "update samcard_mertrade_restrict DB error3[%s]!", mysql_error(sock));
            return -1;
        }
		
		memcpy(i_samcard_mertrade_restric.c_total_tranamt, "000000000000", 12);
		memcpy(i_samcard_mertrade_restric.c_amtdate, currentDate, 8);
	}
    
	#ifdef __LOG__
		dcs_log(0, 0, "i_samcard_mertrade_restric.c_amttotal = [%s]", i_samcard_mertrade_restric.c_amttotal);
		dcs_log(0, 0, "i_samcard_mertrade_restric.c_total_tranamt = [%s]", i_samcard_mertrade_restric.c_total_tranamt);
		dcs_log(0, 0, "i_samcard_mertrade_restric.c_amtdate = [%s]", i_samcard_mertrade_restric.c_amtdate);
		dcs_log(0, 0, "i_samcard_mertrade_restric.psammer_restrict_flag = [%d]", i_samcard_mertrade_restric.psammer_restrict_flag);
	#endif
	
	memcpy(samcard_mertrade_restric, &i_samcard_mertrade_restric, sizeof(SAMCARD_MERTRADE_RESTRICT));
	
	return 1;
}

/*
 终端商户日限额
 输入参数：samid，psam卡号
 输入参数：mercode，商户号
 输入参数：transamt,交易金额
 
 输出参数：
 1：成功。
 -1：失败。
 */
int UpdateTermMerCreditCardTradeAmtLimit(char *samid, char *mercode, char * transamt)
{
    char i_psamno[16+1];
    char i_mercode[15+1];
	char sql[512];
  	
  	memset(i_psamno,0,sizeof(i_psamno));
  	memset(i_mercode,0,sizeof(i_mercode));
	memset(sql, 0x00, sizeof(sql));
	
  	memcpy(i_psamno, samid, 16);
  	memcpy(i_mercode, mercode, 15);
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
    sprintf(sql, "update samcard_mertrade_restrict set c_total_tranamt = c_total_tranamt+'%s' where psam_card = '%s'\
	and mercode = '%s';", transamt, i_psamno, i_mercode);
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "update samcard_mertrade_restrict DB error [%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
    //从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
	return 1;  
}
/*
 订单支付状态未知，5分钟之内不允许再次支付 对消费类，转账和信用卡还款做风控
 输入参数：orderId，订单号
 输入参数：currentDate，当前日期
 输入参数：currentTime,当前时间
 
 输出参数：
 1：成功。
 -1：失败。
 */
int ewpOrderRepeatPayCharge(char *orderId, char *currentDate, char *currentTime)
{
    char i_billmsg[40+1];
    char currentDateTime[14+1];
    int count = 0;
  	char sql[1024];
	 
  	memset(currentDateTime, 0, sizeof(currentDateTime));
  	memset(i_billmsg, 0, sizeof(i_billmsg));
	memset(sql, 0x00, sizeof(sql));
	
  	memcpy(currentDateTime, currentDate, 8);
  	memcpy(currentDateTime+8, currentTime, 6);
  	memcpy(i_billmsg, orderId, getstrlen(orderId));
  
    sprintf(sql, "select count(*) from pep_jnls where trim(billmsg) = '%s' and \
	(((aprespn is null or aprespn ='' or aprespn = 'N1' or aprespn = '98') and TIMESTAMPDIFF( MINUTE, date_format(CONCAT(pepdate, peptime), '%s'),\
	date_format('%s', '%s')) < 5) or aprespn = '00');", i_billmsg, "%Y%m%d%H%i%s", currentDateTime,  "%Y%m%d%H%i%s");
    
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "select pep_jnls DB error [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "ewpOrderRepeatPayCharge:Couldn't get result from %s\n", mysql_error(sock)); 
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    
    row = mysql_fetch_row(res); 
    if (mysql_num_rows(res) == 1) {
        if (row[0]) 
			count = atoi(row[0]); 
		else 
			count = 0;
    }
	else
	{
		dcs_log(0, 0, "ewpOrderRepeatPayCharge :数据库操作失败");
		mysql_free_result(res);
		return -1;
	}
	
    mysql_free_result(res);
	if(count > 0)
		return -1;
	else
		return 1;
}

/*
 函数功能：
 前置系统发送数据到核心之前如果存pep_ewp数据库错误或者发送错误，更新已经存在pep_jnls中的数据

 输入参数：pep_jnls
 输入参数：ewp_info
 输入参数：trace
 输入参数：retCode
 
 输出参数：1，成功
 输出参数：-1，失败
 */
int updateXpepErrorTradeInfo(EWP_INFO ewp_info, char *trace, char *retCode)
{
    long i_trace;
    char i_aprespn[2+1];
    char i_samid[16+1];
    char i_carno[20+1];
    char i_rtndate[8+1];
    char i_rtntime[6+1];
  	char sql[512];
	
  	memset(i_aprespn, 0, sizeof(i_aprespn));
  	memset(i_samid, 0, sizeof(i_samid));
  	memset(i_carno, 0, sizeof(i_carno));
  	memset(i_rtndate, 0, sizeof(i_rtndate));
  	memset(i_rtntime, 0, sizeof(i_rtntime));
	memset(sql, 0x00, sizeof(sql));
	
	memcpy(i_aprespn, retCode, 2);
    
    //Freax Modify: 数组越界
    /*memcpy(i_rtndate, ewp_info.consumer_sentdate, getstrlen(ewp_info.consumer_sentdate));
    memcpy(i_rtntime, ewp_info.consumer_senttime, getstrlen(ewp_info.consumer_senttime));
    memcpy(i_samid, pep_jnls.samid, getstrlen(pep_jnls.samid));
    memcpy(i_carno, pep_jnls.outcdno, getstrlen(pep_jnls.outcdno));*/
    
	/*
    memcpy(i_rtndate, ewp_info.consumer_sentdate, sizeof(ewp_info.consumer_sentdate));
    memcpy(i_rtntime, ewp_info.consumer_senttime, sizeof(i_rtntime));
    memcpy(i_samid, pep_jnls.samid, sizeof(i_samid));
    memcpy(i_carno, pep_jnls.outcdno, sizeof(i_carno));*/
	
	memcpy(i_rtndate, ewp_info.consumer_sentdate, getstrlen(ewp_info.consumer_sentdate));
  	memcpy(i_rtntime, ewp_info.consumer_senttime, getstrlen(ewp_info.consumer_senttime));
  	
  	memcpy(i_samid, ewp_info.consumer_psamno, getstrlen(ewp_info.consumer_psamno));
  	memcpy(i_carno, ewp_info.consumer_cardno, getstrlen(ewp_info.consumer_cardno));
  
  	
  	sscanf(trace, "%06ld", &i_trace);
	#ifdef __LOG__
		dcs_log(0, 0, "i_aprespn=[%s]", i_aprespn);
		dcs_log(0, 0, "i_rtndate=[%s]", i_rtndate);
		dcs_log(0, 0, "i_rtntime=[%s]", i_rtntime);
		dcs_log(0, 0, "i_samid=[%s]", i_samid);
		dcs_log(0, 0, "i_trace=[%d]", i_trace);
	#endif
 
    sprintf(sql, "update pep_jnls set aprespn = '%s', revodid = '2' where trndate = '%s' and trntime = '%s' \
	and trace = %ld and samid = '%s' and outcdno = HEX(AES_ENCRYPT('%s','abcdefgh'));", i_aprespn, i_rtndate, i_rtntime, i_trace,i_samid, i_carno);
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "updateXpepErrorTradeInfo error [%s]!", mysql_error(sock));
        return -1;
    }
	#ifdef __LOG__
		dcs_log(0, 0, "updateXpepErrorTradeInfo success.");
	#endif
    
	return 1;
}


int updateMerWorkey(MER_INFO_T *mer_info)
{
    char i_mgr_term_id[8+1];
    char i_mgr_mer_id[15+1];
    char i_mgr_pik[32+1];
    char i_mgr_mak[16+1];
    char i_mgr_tdk[32+1];
    char i_mgr_signtime[14+1];
    char i_mgr_batch_no[6+1];
	char sql[512];
	
  	memset(i_mgr_term_id, 0, sizeof(i_mgr_term_id));
  	memset(i_mgr_mer_id, 0, sizeof(i_mgr_mer_id));
  	memset(i_mgr_pik, 0, sizeof(i_mgr_pik));
  	memset(i_mgr_mak, 0, sizeof(i_mgr_mak));
  	memset(i_mgr_tdk, 0, sizeof(i_mgr_tdk));
  	memset(i_mgr_signtime, 0, sizeof(i_mgr_signtime));
  	memset(i_mgr_batch_no, 0, sizeof(i_mgr_batch_no));
	memset(sql, 0x00, sizeof(sql));
  	
  	memcpy(i_mgr_term_id, mer_info->mgr_term_id, getstrlen(mer_info->mgr_term_id));
  	memcpy(i_mgr_mer_id, mer_info->mgr_mer_id, getstrlen(mer_info->mgr_mer_id));
  	memcpy(i_mgr_pik, mer_info->mgr_pik, getstrlen(mer_info->mgr_pik));
  	memcpy(i_mgr_mak, mer_info->mgr_mak, getstrlen(mer_info->mgr_mak));
  	memcpy(i_mgr_tdk, mer_info->mgr_tdk, getstrlen(mer_info->mgr_tdk));
  	memcpy(i_mgr_signtime, mer_info->mgr_signtime, getstrlen(mer_info->mgr_signtime));
  	memcpy(i_mgr_batch_no, mer_info->mgr_batch_no, getstrlen(mer_info->mgr_batch_no));
    
    sprintf(sql, "update pos_conf_info set mgr_pik = '%s', mgr_mak = '%s', mgr_tdk = '%s', mgr_signtime = '%s',\
	mgr_flag = 'Y', mgr_batch_no = '%s'  where mgr_mer_id = '%s' and mgr_term_id = '%s';", i_mgr_pik, i_mgr_mak,i_mgr_tdk, \
	i_mgr_signtime, i_mgr_batch_no, i_mgr_mer_id, i_mgr_term_id);
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "pos_conf_info errorr [%s]!", mysql_error(sock));
        return -1;
    }
		
	#ifdef __LOG__
		dcs_log(0, 0, "update pos_conf_info success.");
	#endif 	
	return 1;
}

/*函数名：WritePosXpep
 函数功能：保存POS终端上送的消费交易信息到数据表pep_jnls中
 输入参数：rcvDate.终端上送交易时间
 返回值：-1表示操作数据表pep_jnls失败
 0表示操作数据表pep_jnls成功
 
 
 修改保存冲正交易的原因请求码到reversedans字段中，以前保存在aprespn字段中
    modify by lp 20140820
*/
 
int WritePosXpep(ISO_data siso, PEP_JNLS pep_jnls, char *rcvDate)
{    
	char tmpbuf[200], t_year[5];
	char i_intcdname[30+1];
	int	s;
	char escape_buf55[255+1];
	char sql[2048];

	struct  tm *tm;   
	time_t  t;

    time(&t);
    tm = localtime(&t);
    
    memset(i_intcdname, 0x00, sizeof(i_intcdname));
	memset(sql, 0x00, sizeof(sql));
    
    sprintf(t_year, "%04d", tm->tm_year+1900);

	memcpy(pep_jnls.mmchgid, "0", 1);
	memcpy(pep_jnls.revodid, "1", 1);
	pep_jnls.mmchgnm = 0;

	if(getbit(&siso, 0, (unsigned char *)pep_jnls.msgtype)<0)
	{
		 dcs_log(0, 0, "can not get bit_0!");	
		 return -1;
	}

	if(getbit(&siso, 7, (unsigned char *)tmpbuf)<0)
	{
		 dcs_log(0, 0, "can not get bit_7!");	
		 return -1;
	}
	sprintf(pep_jnls.pepdate, "%4.4s%4.4s", t_year, tmpbuf);
	memcpy(pep_jnls.peptime, tmpbuf+4, 6);	
	if(getbit(&siso, 3, (unsigned char *)pep_jnls.process)<0)
	{
		dcs_log(0, 0, "can not get bit_3!");	
		return -1;
	}

	getbit(&siso, 2, (unsigned char *)pep_jnls.outcdno);
	if(memcmp(pep_jnls.process, "190000", 4)!=0 && memcmp(pep_jnls.process, "480000", 4)!=0)
		getbit(&siso, 4, (unsigned char *)pep_jnls.tranamt);

	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(getbit(&siso, 11, (unsigned char *)tmpbuf)<0)
	{
		 dcs_log(0, 0, "can not get bit_11!");	
		 return -1;
	}
	sscanf(tmpbuf, "%06d", &pep_jnls.trace);
	
	if(memcmp(pep_jnls.trancde, "050", 3)!=0)
	{
		memcpy(pep_jnls.trntime, rcvDate+4, 6);	
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(pep_jnls.trndate, "%4.4s%4.4s", t_year, rcvDate);
	}

	if(getbit(&siso, 22, (unsigned char *)pep_jnls.poitcde)<0)
	{
		 dcs_log(0, 0, "can not get bit_3!");	
		 return -1;
	}
	if(memcmp(pep_jnls.process, "310000", 6) == 0 || memcmp(pep_jnls.process, "190000", 6) == 0 
	||memcmp(pep_jnls.process, "480000", 6) == 0 || memcmp(pep_jnls.trancde, "D01", 3)==0
	|| memcmp(pep_jnls.trancde, "D02", 3)==0 ||memcmp(pep_jnls.trancde, "D03", 3)==0
	|| memcmp(pep_jnls.trancde, "D04", 3)==0)
	{	 
		if(getbit(&siso, 41, (unsigned char *)pep_jnls.termcde)<0)
		{
		 	dcs_log(0, 0, "bit_41 is null!");
		 	return -1;	
		}

		if(getbit(&siso, 42,(unsigned char *)pep_jnls.mercode)<0)
		{
		 	dcs_log(0, 0, "bit_42 is null!");
			return -1;	
		}
	}
	/*这样汇总会有问题，所以做出修改，把冲正请求原因码放在冲正应答码的字段ReversedAns中 by lp 20140820*/
	/*s = getbit(&siso,39,(unsigned char *)pep_jnls.aprespn);*/ 
	if(memcmp(pep_jnls.msgtype, "0400", 4) == 0)
	{
		dcs_log(0, 0, "冲正交易");
		s = getbit(&siso,39,(unsigned char *)pep_jnls.ReversedAns);
	}
	else
	{
#ifdef __LOG__
		dcs_log(0, 0, "非冲正交易");
#endif
		s = getbit(&siso,39,(unsigned char *)pep_jnls.aprespn);
	}
	if(s <= 0)
	{
#ifdef __LOG__
		 dcs_log(0, 0, "bit_39 is null!");
#endif
	}
	else
		memcpy(pep_jnls.revodid, "2", 1);
		
	if(getbit(&siso, 25, (unsigned char *)pep_jnls.poscode)<0)
	{
		 dcs_log(0, 0, "can not get poscode!");	
		 return -1;
	}
	
	if(strlen(pep_jnls.billmsg) == 0)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s%s%06ld", "DY", pep_jnls.pepdate+2, pep_jnls.peptime, pep_jnls.trace);
		memcpy(pep_jnls.billmsg, tmpbuf, 22);	
	}
	
	s = getbit(&siso, 44, (unsigned char *)pep_jnls.filed44);
	if(s <= 0)
	{
#ifdef __LOG__
		dcs_log(0, 0, "bit_44 is null!");
#endif
	}
	
	memcpy(pep_jnls.sendcde, "03000000", 8);
	memcpy(pep_jnls.samid, pep_jnls.termidm, getstrlen(pep_jnls.termidm));

	memset(escape_buf55,0,sizeof(escape_buf55));
	s = getbit(&siso, 55, (unsigned char *)pep_jnls.filed55);
	if(s <= 0)
	{
	#ifdef __LOG__
		 dcs_log(0, 0, "bit_55 is null!");
	#endif
	}

	else if(memcmp(pep_jnls.poitcde,"05",2) ==0||memcmp(pep_jnls.poitcde,"07",2) ==0)//是IC卡交易
	{
	#ifdef __TEST__
		dcs_debug(pep_jnls.filed55, s,"IC卡交易55域,长度len=%d",s);
	#endif
		mysql_real_escape_string(sock,escape_buf55,pep_jnls.filed55,s);
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 59,(unsigned char *)tmpbuf);
	if(s <= 0)
	{
#ifdef __LOG__
		dcs_log(0, 0, "bit59 is null!");
#endif
	}
	else
	{
		sscanf(tmpbuf, "%[^|]|%s", pep_jnls.intcdname, pep_jnls.intcdbank);
		memcpy(i_intcdname, pep_jnls.intcdname, strlen(pep_jnls.intcdname));
	}

	//预授权撤销，预授权完成撤销，预授权完成请求 交易需要保存授权码,用于发生冲正时能够找到原笔---add by wu at 2016/7/5
	if(memcmp(pep_jnls.trancde, "031", 3)==0|| memcmp(pep_jnls.trancde, "032", 3)==0 ||memcmp(pep_jnls.trancde, "033", 3)==0)
	{
		s = getbit(&siso, 38, (unsigned char *)pep_jnls.authcode);
		if(s<0)
			dcs_log(0, 0, "get bit_38 error");
	}

	if(pep_jnls.outcdtype == 0)
		pep_jnls.outcdtype =' ';
	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls.pepdate=[%s], pep_jnls.peptime=[%s]", pep_jnls.pepdate, pep_jnls.peptime);
		dcs_log(0, 0, "pep_jnls.trancde=[%s], pep_jnls.msgtype=[%s]", pep_jnls.trancde, pep_jnls.msgtype);
		//dcs_log(0, 0, "pep_jnls.outcdno=[%s], pep_jnls.outcdtype=[%c]", pep_jnls.outcdno, pep_jnls.outcdtype);
		dcs_log(0, 0, " pep_jnls.process=[%s],pep_jnls.authcode", pep_jnls.process,pep_jnls.authcode);
		dcs_log(0, 0, "pep_jnls.tranamt=[%s], pep_jnls.trace=[%ld]", pep_jnls.tranamt, pep_jnls.trace);
		dcs_log(0, 0, "pep_jnls.termtrc=[%s], pep_jnls.trntime=[%s]", pep_jnls.termtrc, pep_jnls.trntime);
		dcs_log(0, 0, "pep_jnls.trndate=[%s], pep_jnls.poitcde=[%s]", pep_jnls.trndate, pep_jnls.poitcde);
		dcs_log(0, 0, "pep_jnls.termcde=[%s], pep_jnls.mercode=[%s]", pep_jnls.termcde, pep_jnls.mercode);
		dcs_log(0, 0, "pep_jnls.samid=[%s], pep_jnls.revodid=[%s]", pep_jnls.samid, pep_jnls.revodid);
		dcs_log(0, 0, "pep_jnls.mmchgid=[%s], pep_jnls.mmchgnm=[%d]", pep_jnls.mmchgid, pep_jnls.mmchgnm);
		dcs_log(0, 0, "pep_jnls.trnsndp=[%s], pep_jnls.trnmsgd=[%s]", pep_jnls.trnsndp, pep_jnls.trnmsgd);
		dcs_log(0, 0, "pep_jnls.revdate=[%s], pep_jnls.trnsndpid=[%d]", pep_jnls.revdate, pep_jnls.trnsndpid);
		dcs_log(0, 0, "pep_jnls.poscode=[%s], pep_jnls.batch_no=[%s]", pep_jnls.poscode, pep_jnls.batch_no);
		dcs_log(0, 0, "pep_jnls.aprespn=[%s], pep_jnls.ReversedAns=[%s]", pep_jnls.aprespn, pep_jnls.ReversedAns);	
		dcs_log(0, 0, "pep_jnls.billmsg=[%s], pep_jnls.postermcde=[%s]", pep_jnls.billmsg, pep_jnls.postermcde);
		dcs_log(0, 0, "pep_jnls.posmercode=[%s], pep_jnls.sendcde=[%s]", pep_jnls.posmercode, pep_jnls.sendcde);
		dcs_log(0, 0, "pep_jnls.termid=[%s], pep_jnls.termidm=[%s]", pep_jnls.termid, pep_jnls.termidm);
		dcs_log(0, 0, "pep_jnls.gpsid=[%s], pep_jnls.modecde=[%s]", pep_jnls.gpsid, pep_jnls.modecde);
		dcs_log(0, 0, "pep_jnls.modeflg=[%s], pep_jnls.filed28=[%s]", pep_jnls.modeflg, pep_jnls.filed28);	
		dcs_log(0, 0, "pep_jnls.tc_value=[%s], i_intcdname=[%s], pep_jnls.intcdbank=[%s]", pep_jnls.tc_value, i_intcdname, pep_jnls.intcdbank);
	#endif
	sprintf(sql, "INSERT INTO pep_jnls (PEPDATE, PEPTIME, TRANCDE, MSGTYPE, OUTCDNO, OUTCDTYPE, INTCDNO, PROCESS, BILLMSG, TERMID, \
			TRANAMT, TRACE, TERMTRC, TRNTIME, TRNDATE, POITCDE, TERMCDE, MERCODE, REVERSEDANS, SENDCDE, TERMIDM, GPSID, MODECDE, MODEFLG, FILED28,\
			SAMID, REVODID, MMCHGID, MMCHGNM, TRNSNDP, TRNMSGD, REVDATE, POSCODE, TRNSNDPID, BATCHNO, MERID, POSTERMCDE, POSMERCODE, INTCDNAME, INTCDBANK, FIELD55, CARD_SN, APRESPN, TC_VALUE,AUTHCODE)\
    	 VALUES ('%s', '%s', '%s', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')),'%c', HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%s', '%s', '%s', %ld, '%s', '%s', '%s', '%s', '%s', '%s', '%s',\
		 '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', '%s', %d, '%s', %d, '%s', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%s', '%s', '%s', '%s','%s');",\
		 pep_jnls.pepdate, pep_jnls.peptime, pep_jnls.trancde, pep_jnls.msgtype, pep_jnls.outcdno, \
		 pep_jnls.outcdtype,  pep_jnls.intcdno, pep_jnls.process, pep_jnls.billmsg, pep_jnls.termid, \
		 pep_jnls.tranamt, pep_jnls.trace, pep_jnls.termtrc, pep_jnls.trntime, pep_jnls.trndate, pep_jnls.poitcde,\
		 pep_jnls.termcde, pep_jnls.mercode, pep_jnls.ReversedAns, pep_jnls.sendcde, pep_jnls.termidm, pep_jnls.gpsid, \
		 pep_jnls.modecde, pep_jnls.modeflg, pep_jnls.filed28, pep_jnls.samid, pep_jnls.revodid, pep_jnls.mmchgid, pep_jnls.mmchgnm, \
		 pep_jnls.trnsndp, pep_jnls.trnmsgd, pep_jnls.revdate, pep_jnls.poscode, pep_jnls.trnsndpid, pep_jnls.batch_no, pep_jnls.merid, \
		 pep_jnls.postermcde, pep_jnls.posmercode, i_intcdname, pep_jnls.intcdbank, escape_buf55, card_sn, pep_jnls.aprespn, pep_jnls.tc_value,pep_jnls.authcode);
	if(mysql_query(sock, sql)) {
	  #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "WritePosXpep INSERT INTO pep_jnls DB error [%s]!", mysql_error(sock));
	        return -1;
    }
	
	return 0;
}

/*
 输入参数：pos机终端号和商户号,以及机具编号
 输出参数：主密钥索引
 成功返回0
 失败返回-1
 */
int GetPosInfo(MER_INFO_T *omer_info_t, char *termidm)
{	
	char i_term_id[8+1];
	char i_mer_id[15+1];
	char i_termidm[25+1];
	char sql[1048];
	MER_INFO_T mer_info_t;
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
  	memset(i_term_id, 0, sizeof(i_term_id));
  	memset(i_mer_id, 0, sizeof(i_mer_id));
  	memset(i_termidm, 0, sizeof(i_termidm));
	memset(sql, 0, sizeof(sql));
  	memset(&mer_info_t, 0, sizeof(MER_INFO_T));
	
  	memcpy(i_term_id, omer_info_t->mgr_term_id, getstrlen(omer_info_t->mgr_term_id));
  	memcpy(i_mer_id, omer_info_t->mgr_mer_id, getstrlen(omer_info_t->mgr_mer_id));
  	memcpy(i_termidm, termidm, getstrlen(termidm));
  	
  	mer_info_t.encrpy_flag = omer_info_t->encrpy_flag ;
  	
  	#ifdef __LOG___
  		dcs_log(0, 0, "i_term_id=[%s]", i_term_id);
  		dcs_log(0, 0, "i_mer_id=[%s]", i_mer_id);
  		dcs_log(0, 0, "i_termidm=[%s]", i_termidm);
  	#endif
  	
  	if(strlen(omer_info_t->mgr_tpdu) == 10)
  	{
  		sprintf(sql, "SELECT mgr_key_index ,mgr_term_id, mgr_mer_id, mgr_flag, mgr_mer_des, mgr_batch_no, mgr_folder,\
		pos_machine_type, pos_telno, pos_gpsno, mgr_signout_flag, mgr_kek, mgr_desflg, mgr_pik, mgr_mak, mgr_tdk, \
		mgr_signtime, track_pwd_flag, pos_machine_id, pos_apn, remitfeeformula, creditfeeformula, para_update_flag, \
		para_update_mode, pos_update_mode, cur_version_info, new_version_info, pos_update_flag, tag26, tag39,t0_flag,agents_code, download_pubkey_flag, download_para_flag FROM pos_conf_info \
		WHERE mgr_term_id = '%s' and mgr_mer_id = '%s' and pos_machine_id = '%s';", i_term_id, i_mer_id, i_termidm);
		memcpy(mer_info_t.mgr_tpdu, omer_info_t->mgr_tpdu, 10);
  	}
  	else
  	{
	  	sprintf(sql, "SELECT mgr_key_index ,mgr_term_id, mgr_mer_id, mgr_flag, mgr_mer_des, mgr_batch_no, mgr_folder,\
		pos_machine_type, pos_telno, pos_gpsno, mgr_signout_flag, mgr_kek, mgr_desflg, mgr_pik, mgr_mak, mgr_tdk,\
		mgr_signtime, track_pwd_flag, pos_machine_id, pos_apn, remitfeeformula, creditfeeformula, para_update_flag, \
		para_update_mode, pos_update_mode, cur_version_info, new_version_info, pos_update_flag, tag26, tag39,t0_flag,agents_code, download_pubkey_flag, download_para_flag, mgr_tpdu  FROM pos_conf_info\
		WHERE mgr_term_id = '%s' and mgr_mer_id ='%s' and pos_machine_id = '%s';", i_term_id, i_mer_id, i_termidm);
	}
	if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetPosInfo error=[%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    memset(&row, 0, sizeof(MYSQL_ROW));
    int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetPosInfo : Couldn't get result from %s\n", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
    //从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res); 
    if(num_rows == 1) {
		if(row[0])
			mer_info_t.mgr_key_index = atoi(row[0]);
        memcpy(mer_info_t.mgr_term_id, row[1] ? row[1] : "\0", (int)sizeof(mer_info_t.mgr_term_id));
        memcpy(mer_info_t.mgr_mer_id, row[2] ? row[2] : "\0", (int)sizeof(mer_info_t.mgr_mer_id));
		memcpy(mer_info_t.mgr_flag, row[3] ? row[3] : "\0", (int)sizeof(mer_info_t.mgr_flag));
		memcpy(mer_info_t.mgr_mer_des, row[4] ? row[4] : "\0", (int)sizeof(mer_info_t.mgr_mer_des));
		memcpy(mer_info_t.mgr_batch_no, row[5] ? row[5] : "\0", (int)sizeof(mer_info_t.mgr_batch_no));
		memcpy(mer_info_t.mgr_folder, row[6] ? row[6] : "\0", (int)sizeof(mer_info_t.mgr_folder));
		if(row[7])
			mer_info_t.pos_machine_type = atoi(row[7]);
		memcpy(mer_info_t.pos_telno, row[8] ? row[8] : "\0", (int)sizeof(mer_info_t.pos_telno));
		memcpy(mer_info_t.pos_gpsno, row[9] ? row[9] : "\0", (int)sizeof(mer_info_t.pos_gpsno));
		memcpy(mer_info_t.mgr_signout_flag, row[10] ? row[10] : "\0", (int)sizeof(mer_info_t.mgr_signout_flag));
		memcpy(mer_info_t.mgr_kek, row[11] ? row[11] : "\0", (int)sizeof(mer_info_t.mgr_kek));
		memcpy(mer_info_t.mgr_des, row[12] ? row[12] : "\0", (int)sizeof(mer_info_t.mgr_des));
		memcpy(mer_info_t.mgr_pik, row[13] ? row[13] : "\0", (int)sizeof(mer_info_t.mgr_pik));
		memcpy(mer_info_t.mgr_mak, row[14] ? row[14] : "\0", (int)sizeof(mer_info_t.mgr_mak));
		memcpy(mer_info_t.mgr_tdk, row[15] ? row[15] : "\0", (int)sizeof(mer_info_t.mgr_tdk));
		memcpy(mer_info_t.mgr_signtime, row[16] ? row[16] : "\0", (int)sizeof(mer_info_t.mgr_signtime));
		memcpy(mer_info_t.track_pwd_flag, row[17] ? row[17] : "\0", (int)sizeof(mer_info_t.track_pwd_flag));
		memcpy(mer_info_t.pos_machine_id, row[18] ? row[18] : "\0", (int)sizeof(mer_info_t.pos_machine_id));
		memcpy(mer_info_t.pos_apn, row[19] ? row[19] : "\0", (int)sizeof(mer_info_t.pos_apn));
		memcpy(mer_info_t.remitfeeformula, row[20] ? row[20] : "\0", (int)sizeof(mer_info_t.remitfeeformula));
		memcpy(mer_info_t.creditfeeformula, row[21] ? row[21] : "\0", (int)sizeof(mer_info_t.creditfeeformula));
		if(row[22])
			mer_info_t.para_update_flag = row[22][0];
		if(row[23])
			mer_info_t.para_update_mode = row[23][0];
		if(row[24])
			mer_info_t.pos_update_mode = row[24][0];
		memcpy(mer_info_t.cur_version_info, row[25] ? row[25] : "\0", (int)sizeof(mer_info_t.cur_version_info));
		memcpy(mer_info_t.new_version_info, row[26] ? row[26] : "\0", (int)sizeof(mer_info_t.new_version_info));
		if(row[27])
			mer_info_t.pos_update_flag = atoi(row[27]);
		if(row[28])
			mer_info_t.tag26 = atoi(row[28]);
		if(row[29])
			mer_info_t.tag39 = atoi(row[29]);
		if(row[30])
			mer_info_t.T0_flag[0] = row[30][0];
		memcpy(mer_info_t.agents_code, row[31] ? row[31] : "\0", (int)sizeof(mer_info_t.agents_code));
		memcpy(mer_info_t.download_pubkey_flag, row[32] ? row[32] : "\0", (int)sizeof(mer_info_t.download_pubkey_flag));
		memcpy(mer_info_t.download_para_flag, row[33] ? row[33] : "\0", (int)sizeof(mer_info_t.download_para_flag));
		if(strlen(omer_info_t->mgr_tpdu) != 10)
			memcpy(mer_info_t.mgr_tpdu, row[34] ? row[34] : "\0", (int)sizeof(mer_info_t.mgr_tpdu));
    }
    else if(num_rows==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPosInfo 未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPosInfo 找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);
    
	#ifdef __LOG__
		dcs_log(0, 0, "mer_info_t.pos_machine_type=[%d], mer_info_t.remitfeeformula=[%s]", mer_info_t.pos_machine_type, mer_info_t.remitfeeformula);
		dcs_log(0, 0, "mer_info_t.creditfeeformula=[%s], mer_info_t.mgr_key_index=[%d]", mer_info_t.creditfeeformula, mer_info_t.mgr_key_index);
  		dcs_log(0, 0, "mer_info_t.pos_update_flag=[%d], mer_info_t.para_update_flag=[%c]", mer_info_t.pos_update_flag, mer_info_t.para_update_flag);
		dcs_log(0, 0, "mer_info_t.para_update_mode=[%c], mer_info_t.pos_update_mode=[%c]", mer_info_t.para_update_mode, mer_info_t.pos_update_mode);
  		dcs_log(0, 0, "mer_info_t.cur_version_info=[%s], mer_info_t.new_version_info=[%s]", mer_info_t.cur_version_info, mer_info_t.new_version_info);
  		dcs_log(0, 0, "mer_info_t.tag26=[%d], mer_info_t.tag39=[%d]", mer_info_t.tag26, mer_info_t.tag39);
		dcs_log(0, 0, "mer_info_t.T0_flag=[%s], mer_info_t.agents_code=[%s]", mer_info_t.T0_flag, mer_info_t.agents_code);
  	#endif
  	mer_info_t.remitfeeformula[getstrlen(mer_info_t.remitfeeformula)] = 0;
  	mer_info_t.creditfeeformula[getstrlen(mer_info_t.creditfeeformula)] = 0;
  	
  	memcpy(omer_info_t, &mer_info_t, sizeof(MER_INFO_T));
	return 0;
}

/*
 保存终端上送的错误的交易信息到数据表xpep_errtrade_jnls中
 */
int SaveErTradeInfo(char *retCode, char *CurDate, char *errorDetail, MER_INFO_T mer_info_t,PEP_JNLS pep_jnls,char *trace)
{
	char i_ter_date[8+1];
	char i_ter_time[6+1];
	char i_ter_psam[23+1];
	char i_error[64];
	char sql[1024];
    
    memset(i_ter_date, 0, sizeof(i_ter_date));
    memset(i_ter_time, 0, sizeof(i_ter_time));
    memset(i_ter_psam, 0, sizeof(i_ter_psam));
    memset(i_error, 0, sizeof(i_error));
	memset(sql, 0, sizeof(sql));
    
    memcpy(i_ter_date, CurDate, 8);
    memcpy(i_ter_time, CurDate+8, 6);
	
    if(getstrlen(mer_info_t.mgr_mer_id)!=0)
    	memcpy(i_ter_psam,mer_info_t.mgr_mer_id,15);
   	if(getstrlen(mer_info_t.mgr_term_id)!=0 && getstrlen(i_ter_psam)!=0)
    	memcpy(i_ter_psam+15, mer_info_t.mgr_term_id, 8);
    if(getstrlen(i_ter_psam)==0)
    	memcpy(i_ter_psam, CurDate, 14);
    memcpy(i_error, errorDetail, strlen(errorDetail));
    
    #ifdef __LOG__
   		dcs_log(0, 0, "i_ter_date=[%s], mer_info_t.mgr_mer_id=[%s], i_ter_psam=[%s]", i_ter_date, mer_info_t.mgr_mer_id, i_ter_psam);
  		dcs_log(0, 0, "pep_jnls.trnsndp=[%s], i_error=[%s], i_ter_time=[%s]", pep_jnls.trnsndp, i_error, i_ter_time);
  		dcs_log(0, 0, "trace=[%s], retCode=[%s], pep_jnls.trancde=[%s]", trace, retCode, pep_jnls.trancde);
  	#endif
    
	sprintf(sql, "INSERT INTO xpep_errtrade_jnls (TERM_DATE, TERM_TEL, TERM_PSAM, TERM_E1CODE, TERM_ERROR, TERM_TIME,\
	TERM_TRACE, TERM_RESP, TERM_TRANCDE) VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s');", \
	i_ter_date, mer_info_t.mgr_mer_id, i_ter_psam, pep_jnls.trnsndp, i_error, i_ter_time, trace, retCode, pep_jnls.trancde);
	
	if(mysql_query(sock, sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "SaveErTradeInfo INSERT INTO xpep_errtrade_jnls DB error [%s]!", mysql_error(sock));
	        return -1;
    }
	return 0;
}

/*
 根据终端上送的原笔交易流水查询pep_jnls得到原笔交易的前置系统流水
 */
int GetTrace(ISO_data siso, char *trace, char *currentDate, PEP_JNLS *pep_jnls)
{
	char i_ter_trace[6+1];
	char i_date[8+1];
	char o_date[8+1];
	char i_mercode[15+1];
	char i_termcde[8+1];
	char i_batchno[6+1];
	char i_termidm[25+1];
	char sql[1024];
   
    int s, len;
    char tmpbuf[127], tmp_len[3];
    
    struct  tm *tm;   
	time_t  lastTime;
	 
    memset(i_ter_trace, 0, sizeof(i_ter_trace));
    memset(i_mercode, 0, sizeof(i_mercode));
    memset(i_termcde, 0, sizeof(i_termcde));
    memset(i_batchno, 0, sizeof(i_batchno));
    memset(tmpbuf, 0, sizeof(tmpbuf));
    memset(i_termidm, 0, sizeof(i_termidm));
    memset(tmp_len, 0, sizeof(tmp_len));  
    memset(i_date, 0, sizeof(i_date)); 
    memset(o_date, 0, sizeof(o_date)); 
	memset(sql, 0, sizeof(sql)); 
 
    dcs_log(0, 0, "原交易查询");
    
     /*昨天*/
    lastTime = time(NULL) - 86400;
	tm = localtime(&lastTime);
	sprintf(o_date, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	/*今天*/
   	memcpy(i_date, currentDate, 8);
    
    s = getbit(&siso, 11, (unsigned char *)tmpbuf);
    if(s < 0)
    {
    	dcs_log(0, 0,"冲正取终端上送流水失败");
		return -1;
    }
   	memcpy(i_ter_trace, tmpbuf, 6);
   	
   	memset(tmpbuf, 0, sizeof(tmpbuf));
   	s = getbit(&siso, 41, (unsigned char *)tmpbuf);
    if(s < 0)
    {
    	dcs_log(0, 0,"get bit_41 error");
		return -1;
    }
    memcpy(i_termcde, tmpbuf, 8);
   
   	memset(tmpbuf, 0, sizeof(tmpbuf));
   	s = getbit(&siso, 42, (unsigned char *)tmpbuf);
    if(s < 0)
    {
    	dcs_log(0, 0,"get bit_42 error");
		return -1;
    }
    memcpy(i_mercode, tmpbuf, 15);
    
    memset(tmpbuf, 0, sizeof(tmpbuf));
   	s = getbit(&siso, 60, (unsigned char *)tmpbuf);
    if(s < 0)
    {
    	dcs_log(0,0,"get bit_60 error");
		return -1;
    }
   	memcpy(i_batchno, tmpbuf+2, 6);
   	
   	memset(tmpbuf, 0, sizeof(tmpbuf));
   	s = getbit(&siso, 21, (unsigned char *)tmpbuf);
    if(s < 0)
    {
    	dcs_log(0, 0,"get bit_21 error");
		return -1;
    }
   	memcpy(tmp_len, tmpbuf, 2);
   	sscanf(tmp_len, "%d", &len);
   	memcpy(i_termidm, tmpbuf+2, len);
   	i_termidm[len]='\0';
   	
   	#ifdef __LOG__
   		dcs_log(0, 0, "i_date=[%s], o_date=[%s], i_ter_trace=[%s], i_termcde=[%s]", i_date, o_date, i_ter_trace, i_termcde);
  		dcs_log(0, 0, "i_mercode=[%s], i_batchno=[%s], i_termidm=[%s]", i_mercode, i_batchno, i_termidm);
  	#endif
    
  	sprintf(sql, "SELECT trace, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), outcdtype, billmsg, authcode FROM pep_jnls \
	WHERE termtrc ='%s' and (pepdate= '%s' or pepdate= '%s') and termcde= '%s' and mercode= '%s' and batchno= '%s'\
	and termidm= '%s' and msgtype in ('0200', '0100');", i_ter_trace, i_date, o_date, i_termcde, i_mercode, i_batchno, i_termidm);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetTrace error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetTrace : Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		if(row[0])
			pep_jnls->trace = atol(row[0]);
		memcpy(pep_jnls->outcdno, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->outcdno));
		if(row[2])
			pep_jnls->outcdtype = row[2][0];
		memcpy(pep_jnls->billmsg, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->billmsg));
		memcpy(pep_jnls->authcode, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls->authcode));
	}
	
	else if(num_rows==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetTrace 未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetTrace 找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);
	
	#ifdef __LOG__
  		//dcs_log(0, 0, "pep_jnls->trace=[%ld], outcdno=[%s], outcdtype=[%c]", pep_jnls->trace, pep_jnls->outcdno, pep_jnls->outcdtype);
  		dcs_log(0, 0, "pep_jnls->billmsg=[%s], pep_jnls->authcode=[%s]", pep_jnls->billmsg, pep_jnls->authcode);
  	#endif
  
 	sprintf(trace, "%06ld", pep_jnls->trace);
 	if(memcmp(pep_jnls->trancde, "C32", 3)==0)
   	{
   		dcs_log(0,0,"预授权完成请求冲正处理");
   		memset(tmpbuf, 0, sizeof(tmpbuf));
   		s = getbit(&siso, 38, (unsigned char *)tmpbuf);
   	 	if(s < 0)
	    	{
	    		dcs_log(0,0,"get bit_38 error");
				return -1;
	    	}
	    	if(memcmp(tmpbuf, pep_jnls->authcode, 6)!=0)
	    	{
	    		dcs_log(0,0,"预授权完成请求冲正处理未找到原笔");
	    		return -1;
	    	}
   	}
	return 0;
}

/*消费冲正更新数据表pep_jnls*/
int Updatepep_jnls(ISO_data iso, PEP_JNLS *pep_jnls)
{
	char i_authcode[6+1];
	char i_revdate[10+1];
	char i_SettleDate[4+1];
	char i_filed44[22+1];
	char curdate[8+1], tmpbuf[127], sql[1024];
	struct tm *tm;   
	time_t lastTime;
	int s = 0;
	char o_curdate[8+1];
	
    memset(curdate, 0, sizeof(curdate));
	memset(o_curdate, 0, sizeof(o_curdate));
    memset(tmpbuf, 0, sizeof(tmpbuf));
    memset(i_authcode, 0, sizeof(i_authcode));
    memset(i_revdate, 0, sizeof(i_revdate));
    memset(i_SettleDate, 0, sizeof(i_SettleDate));
    memset(i_filed44, 0, sizeof(i_filed44));
	memset(sql, 0x00, sizeof(sql));
	
	/*昨天*/
	lastTime = time(NULL) - 86400;
	tm = localtime(&lastTime);
	sprintf(o_curdate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	
	/*今天*/
	time(&lastTime);
	tm = localtime(&lastTime);
	sprintf(curdate, "%04d", tm->tm_year+1900);
    
    /*i_revdate从核心返回的信息中12域和13域中取得*/
	s=getbit(&iso, 13, (unsigned char *)i_revdate);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_13 error,ERROR应答");
		return -1;
	}
	
	s=getbit(&iso, 15, (unsigned char *)i_SettleDate);
	if(s<0)
	{
#ifdef __LOG__
		dcs_log(0, 0, "bit_15 is null");
#endif
	}
	
	s=getbit(&iso, 12, (unsigned char *)i_revdate+4);
	if(s<0)
	{
#ifdef __LOG__
		dcs_log(0, 0, "get bit_12 error,ERROR应答");
#endif
		return -1;
	}
			
	s=getbit(&iso, 7, (unsigned char *)tmpbuf);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_7 error,ERROR应答");
		return -1;
	}
	else
	{
		memcpy(pep_jnls->peptime, tmpbuf+4, 6);
		memcpy(curdate+4, tmpbuf, 4);
	}
		
	s = getbit(&iso, 38, (unsigned char *)i_authcode);
	if(s<0)
		dcs_log(0, 0, "get bit_38 error"); 

	s=getbit(&iso, 44, (unsigned char *)i_filed44);
	if(s<0)
		dcs_log(0, 0, "get bit_44 error"); 
	
    getbit(&iso, 41, (unsigned char *)pep_jnls->termcde);
	getbit(&iso, 42, (unsigned char *)pep_jnls->mercode);
	getbit(&iso, 37, (unsigned char *)pep_jnls->SysReNo);
	getbit(&iso, 39, (unsigned char *)pep_jnls->aprespn);
	getbit(&iso, 4, (unsigned char *)pep_jnls->tranamt);
    
    #ifdef __LOG__
  		dcs_log(0, 0, "i_revdate=[%s", i_revdate);
  		dcs_log(0, 0, "i_filed44=[%s]", i_filed44);
  		dcs_log(0, 0, "i_authcode=[%s]", i_authcode);
  		dcs_log(0, 0, "i_SettleDate=[%s]", i_SettleDate);
  	#endif
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}

	sprintf(sql, "UPDATE pep_jnls set reversedans='%s', reversedflag = '1', sysreno='%s' where trace=%ld\
	and termcde= '%s' and mercode= '%s' and (pepdate= '%s' or pepdate='%s') and tranamt= '%s' and msgtype in ('0200', '0100');",\
		pep_jnls->aprespn, pep_jnls->SysReNo, pep_jnls->trace, pep_jnls->termcde, pep_jnls->mercode, curdate, o_curdate, pep_jnls->tranamt); 
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "UPDATE pep_jnls DB1 error [%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }	
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "UPDATE pep_jnls set aprespn ='%s', reversedflag = '1', sysreno= '%s', revdate = '%s',\
	filed44 ='%s', authcode ='%s', settlementdate ='%s', syseqno ='%s' where trace=%ld and termcde= '%s' \
	and mercode= '%s' and pepdate= '%s' and tranamt= '%s' and msgtype = '0400' and peptime = '%s';",\
	pep_jnls->aprespn,  pep_jnls->SysReNo, i_revdate, i_filed44, i_authcode, i_SettleDate,\
	pep_jnls->SysReNo, pep_jnls->trace, pep_jnls->termcde, pep_jnls->mercode, curdate,\
	pep_jnls->tranamt, pep_jnls->peptime);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "UPDATE pep_jnls DB2 error [%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }	
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "SELECT  mercode, termcde, termtrc, process, trace, batchno, billmsg, trnmsgd, trnsndp, msgtype, trancde,\
	trnsndpid, settlementdate, termidm, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), card_sn FROM pep_jnls where trace=%ld and termcde= '%s' \
	and mercode= '%s' and pepdate= '%s' and tranamt= '%s' and msgtype = '0400' and peptime = '%s';", pep_jnls->trace, \
	pep_jnls->termcde, pep_jnls->mercode, curdate, pep_jnls->tranamt, pep_jnls->peptime);
	
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "Updatepep_jnls error=[%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "Updatepep_jnls : Couldn't get result from %s\n", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
    //从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		memcpy(pep_jnls->mercode, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls->mercode));
		memcpy(pep_jnls->termcde, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->termcde));
        memcpy(pep_jnls->termtrc, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls->termtrc));
		memcpy(pep_jnls->process, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->process));
		if(row[4])
			pep_jnls->trace = atoi(row[4]);
		memcpy(pep_jnls->batch_no, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls->batch_no));
		memcpy(pep_jnls->billmsg, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls->billmsg));
		memcpy(pep_jnls->trnmsgd, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls->trnmsgd));
		memcpy(pep_jnls->trnsndp, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls->trnsndp));
		memcpy(pep_jnls->msgtype, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls->msgtype));
        memcpy(pep_jnls->trancde, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls->trancde));
		if(row[11])
			pep_jnls->trnsndpid = atoi(row[11]);
		memcpy(pep_jnls->settlementdate, row[12] ? row[12] : "\0", (int)sizeof(pep_jnls->settlementdate));
		memcpy(pep_jnls->termidm, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls->termidm));
		memcpy(pep_jnls->outcdno, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls->outcdno));
		memcpy(pep_jnls->card_sn, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls->card_sn));
	}
	else if(num_rows==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "Updatepep_jnls 未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "Updatepep_jnls 找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);
#ifdef __LOG__
  	dcs_log(0, 0, "pep_jnls->trancde =[%s], pep_jnls->mercode=[%s], pep_jnls->process=[%s]", pep_jnls->trancde, pep_jnls->mercode, pep_jnls->process);
#endif
	return 0;
}

/*
1：查询终端消费撤销时原笔交易根据37域61域41,42域还有4域
2：查询终端预授权完成撤销时原笔交易根据37域 61域41,42,4域还有38域
*/
int GetOriTrans(ISO_data siso, PEP_JNLS *opep_jnls, MER_INFO_T mer_info_t, char *curdate)
{
	PEP_JNLS i_pep_jnls;
	char i_termtrc[6+1];
	char i_sysrefno[12+1];
	char i_aprespn[2+1];
	char i_batchno[6+1];
	char i_outcard[20+1];
	char i_termcde[8+1];
	char i_mercode[15+1];
	char i_transamt[12+1];	
	char o_curdate[8+1];
	char sql[1024];
	
	int s, n;
	char tmpbuf[127], bit_22[3], authcode[6+1];
	struct  tm *tm;   
	time_t  lastTime;
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(bit_22, 0, sizeof(bit_22));
	memset(&i_pep_jnls, 0, sizeof(PEP_JNLS));
	memset(i_termtrc, 0, sizeof(i_termtrc));
	memset(i_sysrefno, 0, sizeof(i_sysrefno));
	memset(i_aprespn, 0, sizeof(i_aprespn));
	memset(i_batchno, 0, sizeof(i_batchno));
	memset(i_outcard, 0, sizeof(i_outcard));
	memset(i_termcde, 0, sizeof(i_termcde));
	memset(i_mercode, 0, sizeof(i_mercode));
	memset(i_transamt, 0, sizeof(i_transamt));
	memset(o_curdate, 0, sizeof(o_curdate));
	memset(sql, 0, sizeof(sql));
	memset(authcode, 0, sizeof(authcode));
	
	curdate[8]='\0';

	dcs_log(0, 0, "消费撤销原笔交易查询。");
	
	memcpy(i_mercode, mer_info_t.mgr_mer_id, 15);
	memcpy(i_termcde, mer_info_t.mgr_term_id, 8);
	memcpy(i_pep_jnls.trnmsgd, opep_jnls->trnmsgd, 4);
	memcpy(i_pep_jnls.termid, opep_jnls->termid, getstrlen(opep_jnls->termid));
	memcpy(i_pep_jnls.termidm, opep_jnls->termidm, getstrlen(opep_jnls->termidm));
	memcpy(i_pep_jnls.trancde, opep_jnls->trancde, sizeof(opep_jnls->trancde));
	memcpy(i_pep_jnls.trnsndp, opep_jnls->trnsndp, sizeof(opep_jnls->trnsndp));
	i_pep_jnls.trnsndpid = opep_jnls->trnsndpid;
	/*中心应答码为00 表示这是一笔消费成功的交易*/
	memcpy(i_aprespn, "00", 2);
	
	/*昨天*/
    lastTime = time(NULL) - 86400;
	tm = localtime(&lastTime);
	sprintf(o_curdate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	
	s = getbit(&siso, 37, (unsigned char *)i_sysrefno);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 37 error");
		return -1;
	}
	
	s = getbit(&siso, 61, (unsigned char *)tmpbuf);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 61 error");
		return -1;
	}
	memcpy(i_batchno, tmpbuf, 6);
	memcpy(i_termtrc, tmpbuf+6, 6);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 22, (unsigned char *)tmpbuf);
	if(s > 0)
	{
		memcpy(bit_22, tmpbuf, 2);
		if(memcmp(bit_22, "01", 2)==0 || memcmp(bit_22, "05", 2)==0 || memcmp(bit_22, "95", 2)==0|| memcmp(bit_22, "07", 2)==0 || memcmp(bit_22, "96", 2)==0)
		{
			if(getbit(&siso, 2, (unsigned char *)i_outcard) < 0)
			{
				dcs_log(0, 0, "取卡号失败");
				return -2;
			}
		}
		else if(memcmp(bit_22, "02", 2)==0)
		{
			memset(tmpbuf, 0, sizeof(tmpbuf));
			s = getbit(&siso, 35, (unsigned char *)tmpbuf);
			if(s > 0)
			{
				for(n=0; n<20; n++)
				{
					if(tmpbuf[n] == '=' )
						break;
				}
				if(n == 20)
				{
					dcs_log(0,0, "二磁数据错误。");
					return -2;
				}
				else
				{
					memcpy(i_outcard, tmpbuf, n);		        	
//					dcs_log(0, 0, "i_outcard = [%s]", i_outcard);
				}				
			}
			else
			{
				dcs_log(0, 0, "二磁不存在");
				return -2;
			}
		}
	}
#ifdef __LOG__
	dcs_log(0, 0, "bit_22=[%s], i_sysrefno=[%s], i_aprespn=[%s]", bit_22, i_sysrefno, i_aprespn);
	dcs_log(0, 0, "i_mercode=[%s], i_termcde=[%s]", i_mercode, i_termcde);
	dcs_log(0, 0, "i_batchno=[%s], i_termtrc=[%s], curdate=[%s], o_curdate=[%s]", i_batchno, i_termtrc, curdate, o_curdate);
#endif
	
	s = getbit(&siso, 4, (unsigned char *)i_transamt);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 4 error");
		return -1;
	}
	#ifdef __LOG__
		dcs_log(0, 0, "i_transamt=[%s]", i_transamt);
	  #endif


	sprintf(sql, "SELECT revdate, trim(AES_DECRYPT(UNHEX(outcdno), 'abcdefgh')), syseqno, termcde, mercode, postermcde, posmercode,\
	billmsg, authcode FROM pep_jnls  where syseqno = '%s' and aprespn = '%s'  and outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and mercode = '%s'\
	and termcde = '%s' and tranamt = '%s' and batchno = '%s' and termtrc = '%s' and (pepdate = '%s' or pepdate = '%s');", \
	i_sysrefno, i_aprespn, i_outcard, i_mercode, i_termcde, i_transamt, i_batchno, i_termtrc, curdate,  o_curdate);
	
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetOriTrans error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetOriTrans : Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows== 1) {
		memcpy(i_pep_jnls.revdate, row[0] ? row[0] : "\0", (int)sizeof(i_pep_jnls.revdate));
		memcpy(i_pep_jnls.outcdno, row[1] ? row[1] : "\0", (int)sizeof(i_pep_jnls.outcdno));
		memcpy(i_pep_jnls.syseqno, row[2] ? row[2] : "\0", (int)sizeof(i_pep_jnls.syseqno));
		memcpy(i_pep_jnls.termcde, row[3] ? row[3] : "\0", (int)sizeof(i_pep_jnls.termcde));
		memcpy(i_pep_jnls.mercode, row[4] ? row[4] : "\0", (int)sizeof(i_pep_jnls.mercode));
		memcpy(i_pep_jnls.postermcde, row[5] ? row[5] : "\0", (int)sizeof(i_pep_jnls.postermcde));
		memcpy(i_pep_jnls.posmercode, row[6] ? row[6] : "\0", (int)sizeof(i_pep_jnls.posmercode));
		memcpy(i_pep_jnls.billmsg, row[7] ? row[7] : "\0", (int)sizeof(i_pep_jnls.billmsg));
		memcpy(i_pep_jnls.authcode, row[8] ? row[8] : "\0", (int)sizeof(i_pep_jnls.authcode));
	}
	else if(num_rows==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOriTrans 未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOriTrans 找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);
	
	s = strlen(opep_jnls->billmsg);
	if(s!=0&&s!=getstrlen(i_pep_jnls.billmsg))
	{
		dcs_log(0, 0, "订单号不一致");
		return -1;
	}
	if(s != 0 && memcmp(opep_jnls->billmsg, i_pep_jnls.billmsg, s)!=0)
	{
		dcs_log(0, 0, "订单号不一致");
		return -1;
		
	}
	if(memcmp(opep_jnls->trancde, "033", 3) ==0 )
	{
		dcs_log(0, 0, "预授权完成撤销时取授权号");
		s = getbit(&siso, 38, (unsigned char *)authcode);
		if(s>0 && memcmp(authcode, i_pep_jnls.authcode, 6)!=0)
		{
			dcs_log(0, 0, "预授权撤销未找到原笔");
			return -1;
		}
	}
	memcpy(opep_jnls, &i_pep_jnls, sizeof(PEP_JNLS));
	return 0;
}

/*消费退货时查询原笔交易61域*/
int GetOriTransRtn(ISO_data siso, PEP_JNLS *opep_jnls)
{
	PEP_JNLS i_pep_jnls;
	char i_termtrc[6+1];
	char i_sysrefno[12+1];
	char i_aprespn[2+1];
	char i_batchno[6+1];
	char i_outcard[20+1];
	char i_date[12+1];
	char i_curtranamt[12+1];	
	char i_mercode[15+1];	
	char i_pepdate[8+1];
	char i_termcode[8+1];
	
	int s, n;
	char tmpbuf[127];
	char sql[1024];
 
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(i_pepdate, 0, sizeof(i_pepdate));
	memset(&i_pep_jnls, 0, sizeof(PEP_JNLS));
	memset(i_termtrc, 0, sizeof(i_termtrc));
	memset(i_sysrefno, 0, sizeof(i_sysrefno));
	memset(i_aprespn, 0, sizeof(i_aprespn));
	memset(i_batchno, 0, sizeof(i_batchno));
	memset(i_outcard, 0, sizeof(i_outcard));
	memset(i_date, 0, sizeof(i_date));
	memset(i_curtranamt, 0, sizeof(i_curtranamt));
	memset(i_mercode, 0, sizeof(i_mercode));
	memset(i_termcode, 0, sizeof(i_termcode));
	memset(sql, 0, sizeof(sql));
	
	/*中心应答码为00 表示这是一笔消费成功的交易*/
	
	memcpy(i_aprespn, "00", 2);
	memcpy(i_pep_jnls.trnmsgd, opep_jnls->trnmsgd, 4);
	memcpy(i_pep_jnls.termid, opep_jnls->termid, getstrlen(opep_jnls->termid));
	memcpy(i_pep_jnls.termidm, opep_jnls->termidm, getstrlen(opep_jnls->termidm));
	memcpy(i_pep_jnls.trancde, opep_jnls->trancde, getstrlen(opep_jnls->trancde));
	memcpy(i_pep_jnls.trnsndp, opep_jnls->trnsndp, getstrlen(opep_jnls->trnsndp));
	i_pep_jnls.trnsndpid = opep_jnls->trnsndpid;
	s = getbit(&siso, 37, (unsigned char *)i_sysrefno);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 37 error");
		return -1;
	}

	if(memcmp(i_pep_jnls.trancde, "051", 3) == 0)//脱机退货需要获取原交易终端号
	{
		s = getbit(&siso, 62, (unsigned char *)i_termcode);
		if(s < 0)
		{
			dcs_log(0, 0, "get_bit 62 error");
			return -1;
		}
	}
	s = getbit(&siso, 61, (unsigned char *)tmpbuf);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 61 error");
		return -1;
	}
	else
		dcs_log(0, 0, "bit61=[%s], len=%d", tmpbuf, s);
	
	memcpy(i_batchno, tmpbuf, 6);
	memcpy(i_termtrc, tmpbuf+6, 6);
	memcpy(i_date, tmpbuf+12, 4);
	//模糊查询
	memcpy(i_pepdate, "%", 1);
	memcpy(i_pepdate+1, i_date, 4);
#ifdef __LOG__
	dcs_log(0, 0, "i_termtrc=[%s]", i_termtrc);
	dcs_log(0, 0, "i_date=[%s]", i_date);
#endif
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 35, (unsigned char *)tmpbuf);
	if(s > 0)
	{
		for(n=0; n<20; n++)
		{
			if(tmpbuf[n] == '=' )
				break;
		}
		if(n == 20)
		{
			dcs_log(0, 0, "二磁数据错误。");
			return -2;
		}
		else
		{
			memcpy(i_outcard, tmpbuf, n);
//			dcs_log(0, 0, "card = [%s]", i_outcard);
		}
	}
	else
	{
		dcs_log(0, 0, "二磁不存在");
		return -2;
	}
	
	s = getbit(&siso, 42, (unsigned char *)i_mercode);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 42 error");
		return -1;
	}
#ifdef __LOG__
	dcs_log(0, 0, "i_aprespn=[%s]", i_aprespn);
	dcs_log(0, 0, "i_sysrefno=[%s]", i_sysrefno);
	dcs_log(0, 0, "i_batchno=[%s]", i_batchno);
	dcs_log(0, 0, "i_mercode=[%s]", i_mercode);
	dcs_log(0, 0, "i_termcode=[%s]", i_termcode);
#endif

	if(memcmp(i_pep_jnls.trancde, "051", 3) == 0) 
	{
//		 dcs_log(0, 0, "脱机退货查询原笔");
		 sprintf(sql, "SELECT pepdate, revdate, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), syseqno, mercode,\
	tranamt, termtrc, batchno, posmercode, billmsg, authcode FROM  pep_jnls t1 where t1.aprespn ='%s' and t1.termtrc ='%s' and t1.pepdate like '%s' \
	and t1.outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and t1.mercode = '%s' and t1.trancde = '050' and t1.termcde ='%s'\
	UNION\
		SELECT pepdate, revdate, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), syseqno, mercode, tranamt, termtrc, batchno, posmercode,\
		billmsg, authcode FROM  pep_jnls_all t2 where t2.aprespn = '%s' and t2.termtrc ='%s' and t2.pepdate  like '%s' and t2.outcdno = HEX(AES_ENCRYPT('%s','abcdefgh'))\
		and t2.mercode = '%s' and t2.trancde = '050' and t2.termcde ='%s';", i_aprespn, i_termtrc, i_pepdate, i_outcard, i_mercode, i_termcode, \
		i_aprespn, i_termtrc, i_pepdate, i_outcard, i_mercode, i_termcode);
	}
	else
	{
//		dcs_log(0, 0, "联机退货查询原笔");
		sprintf(sql, "SELECT pepdate, revdate, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), syseqno, mercode,\
	tranamt, termtrc, batchno, posmercode, billmsg, authcode FROM  pep_jnls t1 where t1.syseqno = '%s' and t1.aprespn ='%s'\
	and t1.outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and t1.mercode = '%s' and t1.trancde = '000'\
	UNION\
		SELECT pepdate, revdate, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), syseqno, mercode, tranamt, termtrc, batchno, posmercode,\
		billmsg, authcode FROM  pep_jnls_all t2 where t2.syseqno = '%s' and t2.aprespn = '%s' and t2.outcdno = HEX(AES_ENCRYPT('%s','abcdefgh'))\
		and t2.mercode = '%s' and t2.trancde = '000';", i_sysrefno, i_aprespn, i_outcard, i_mercode, i_sysrefno,\
		i_aprespn, i_outcard, i_mercode);
	}
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetOriTransRtn error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetOriTransRtn : Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows== 1) {
		memcpy(i_pep_jnls.pepdate, row[0] ? row[0] : "\0", (int)sizeof(i_pep_jnls.pepdate));
		memcpy(i_pep_jnls.revdate, row[1] ? row[1] : "\0", (int)sizeof(i_pep_jnls.revdate));
		memcpy(i_pep_jnls.outcdno, row[2] ? row[2] : "\0", (int)sizeof(i_pep_jnls.outcdno));
		memcpy(i_pep_jnls.syseqno, row[3] ? row[3] : "\0", (int)sizeof(i_pep_jnls.syseqno));
		memcpy(i_pep_jnls.mercode, row[4] ? row[4] : "\0", (int)sizeof(i_pep_jnls.mercode));
		memcpy(i_pep_jnls.tranamt, row[5] ? row[5] : "\0", (int)sizeof(i_pep_jnls.tranamt));
		memcpy(i_pep_jnls.termtrc, row[6] ? row[6] : "\0", (int)sizeof(i_pep_jnls.termtrc));
		memcpy(i_pep_jnls.batch_no, row[7] ? row[7] : "\0", (int)sizeof(i_pep_jnls.batch_no));
		memcpy(i_pep_jnls.posmercode, row[8] ? row[8] : "\0", (int)sizeof(i_pep_jnls.posmercode));
		memcpy(i_pep_jnls.billmsg, row[9] ? row[9] : "\0", (int)sizeof(i_pep_jnls.billmsg));
		memcpy(i_pep_jnls.authcode, row[10] ? row[10] : "\0", (int)sizeof(i_pep_jnls.authcode));
		
	}
	else if(num_rows==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOriTransRtn 未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOriTransRtn 找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);
#ifdef __LOG__
	dcs_log(0, 0, "i_pep_jnls.tranamt=[%s]", i_pep_jnls.tranamt);
#endif

	s = getbit(&siso, 4, (unsigned char *)i_curtranamt);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 4 error");
		return -1;
	}

#ifdef __LOG__
	dcs_log(0, 0, "i_curtranamt=[%s]", i_curtranamt);
#endif
	if(memcmp(i_pep_jnls.tranamt, i_curtranamt, 12)<0)
	{
		dcs_log(0, 0, "退货金额不能大于原笔交易金额");
		return -3;
	}
	
	if(memcmp(i_pep_jnls.pepdate+4, i_date, 4)!= 0)
	{
		dcs_log(0, 0, "根据日期查找原笔交易失败");
		return -1;
	}
	
	if(memcmp(i_batchno, "000000", 6) != 0 && memcmp(i_termtrc, "000000", 6) != 0)
	{
		if(memcmp(i_batchno, i_pep_jnls.batch_no, 6) != 0&& memcmp(i_termtrc, i_pep_jnls.termtrc, 6) != 0)
			return -1;	
	}
	s = strlen(opep_jnls->billmsg);
	if(s==0 && memcmp(i_pep_jnls.billmsg, "DY", 2)!=0)
	{
		dcs_log(0, 0, "订单号不一致");
		return -1;
	}
	if(s!=0&&s!=getstrlen(i_pep_jnls.billmsg))
	{
		dcs_log(0, 0, "订单号不一致");
		return -1;
	}
	if(s != 0 && memcmp(opep_jnls->billmsg, i_pep_jnls.billmsg, s)!=0)
	{
		dcs_log(0, 0, "订单号不一致");
		return -1;	
	}
	
	memcpy(opep_jnls, &i_pep_jnls, sizeof(PEP_JNLS));
	return 0;
}

/*
 从pos_settlement_jnls_sequence中
 获得批结算表pos_settlement_jnls自增长的主键ID
 */
long get_id_fpos_settlement_seq()
{
	long new_trace = 0;
    char sql[512];
    memset(sql, 0x00, sizeof(sql));
    strcpy(sql, "select nextval('settlement_seq');");
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "Read get_id_fpos_settlement_seq error [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "get_id_fpos_settlement_seq : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    num_rows =mysql_num_rows(res);
    if(num_rows == 1) {
        row = mysql_fetch_row(res);
        new_trace = atol(row[0]);
    }
    else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "get_id_fpos_settlement_seq:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "get_id_fpos_settlement_seq:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);
    return new_trace;
}


/*消费撤销冲正时查询原笔交易信息*/
int  GetOrignTrans(ISO_data siso, char *currentDate, PEP_JNLS *pep_jnls, char *trace)
{
	char i_ter_trace[6+1];
	char i_batchno[6+1];
	char i_tranamt[12+1];
	char i_date[8+1];
	char o_date[8+1];
	char i_termidm[25+1];
	char sql[1024];
    
    int s, len;
    char tmpbuf[127], tmp_len[3];
    
    struct  tm *tm;   
	time_t  lastTime;
    
    memset(tmpbuf, 0, sizeof(tmpbuf));
    memset(i_ter_trace, 0, sizeof(i_ter_trace));
    memset(i_batchno, 0, sizeof(i_batchno));
    memset(i_tranamt, 0, sizeof(i_tranamt));
    memset(i_date, 0, sizeof(i_date));
    memset(o_date, 0, sizeof(o_date));
    memset(i_termidm, 0, sizeof(i_termidm));
    memset(tmp_len, 0, sizeof(tmp_len));
	memset(sql, 0, sizeof(sql));
    
    dcs_log(0, 0, "撤销冲正原笔交易查询。");
    
    /*昨天*/
    lastTime = time(NULL) - 86400;
	tm = localtime(&lastTime);
	sprintf(o_date, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	/*今天*/
    memcpy(i_date, currentDate, 8);
    
   	s = getbit(&siso, 61, (unsigned char *)tmpbuf);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 61 error");
		return -1;
	}
	memcpy(i_batchno, tmpbuf, 6);
	memcpy(i_ter_trace, tmpbuf+6, 6);
	s = getbit(&siso, 4, (unsigned char *)i_tranamt);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 4 error");
		return -1;
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 21, (unsigned char *)tmpbuf);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 21 error");
		return -1;
	}
	memcpy(tmp_len, tmpbuf, 2);
	sscanf(tmp_len, "%d", &len);
	memcpy(i_termidm, tmpbuf+2, len);
	i_termidm[len] = '\0';
	
	#ifdef __LOG__
    	dcs_log(0, 0, "i_ter_trace=[%s], i_batchno=[%s], i_tranamt=[%s]", i_ter_trace, i_batchno, i_tranamt);
    	dcs_log(0, 0, "pepdate=[%s] or [%s], pep_jnls->termcde=[%s]", i_date, o_date, pep_jnls->termcde);
  		dcs_log(0, 0, "pep_jnls->mercode=[%s], i_termidm=[%s]", pep_jnls->mercode, i_termidm);
  	#endif
    
	sprintf(sql, "SELECT trace, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), outcdtype, billmsg, authcode FROM pep_jnls \
	WHERE termtrc = '%s' and (pepdate= '%s' or pepdate = '%s') and termcde= '%s' and mercode= '%s' and batchno= '%s'\
	and tranamt= '%s' and termidm= '%s' and msgtype in('0200', '0100');", i_ter_trace, i_date, o_date,\
	pep_jnls->termcde, pep_jnls->mercode, i_batchno, i_tranamt, i_termidm);
  		
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetOrignTrans error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetOrignTrans : Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		if(row[0])
			pep_jnls->trace = atol(row[0]);
		memcpy(pep_jnls->outcdno, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->outcdno));
		if(row[2])
			pep_jnls->outcdtype = row[2][0];
		memcpy(pep_jnls->billmsg, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->billmsg));
		memcpy(pep_jnls->authcode, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls->authcode));
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOrignTrans ：未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOrignTrans:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);
	
#ifdef __LOG__
  	dcs_log(0, 0, "pep_jnls->trace=[%ld]", pep_jnls->trace);
	dcs_log(0, 0, "pep_jnls->billmsg=[%s], pep_jnls->authcode=[%s]", pep_jnls->billmsg, pep_jnls->authcode);   
#endif
	if(memcmp(pep_jnls->trancde, "C31", 3)==0)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 38, (unsigned char *)tmpbuf);
		if(s < 0)
		{
			dcs_log(0, 0, "get_bit 38 error");
			dcs_log(0, 0, "预授权撤销冲正");
			return -1;
		}
		if(memcmp(tmpbuf, pep_jnls->authcode, 6)!=0)
		{
			dcs_log(0, 0, "预授权撤销冲正未找到原笔");
			return -1;
		}
	}
 	sprintf(trace, "%06ld", pep_jnls->trace);
 	
	return 0;
}

/*
 根据应答码和交易码查询数据表xpep_retcode得到详细信息的描述
 返回描述信息的长度len
 */
int GetDetailInfo(char *rtnCode, char  *trancde, char *ewp_info)
{
   	char i_ewp_info[255+1];
	int len;
	memset(i_ewp_info, 0, sizeof(i_ewp_info));
    
    char sql[512];
   	memset(sql, 0x00, sizeof(sql));
#ifdef __LOG__
   		dcs_log(0, 0, "rtnCode = [%s]", rtnCode);
   		dcs_log(0, 0, "trancde = [%s]", trancde);
#endif
	
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}

    /*sprintf(sql, "SELECT xpep_content_ewp FROM xpep_retcode WHERE xpep_retcode = '%s' and xpep_trancde = '%s';", rtnCode, trancde);*/
	sprintf(sql, "SELECT xpep_content FROM xpep_retcode WHERE xpep_retcode = '%s' and xpep_trancde = '%s';", rtnCode, trancde);
    if(mysql_query(sock,sql)) {
	    #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "get xpep_retcode info error1 [%s], trancde = %s!", mysql_error(sock), trancde);
	    //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
	    return -1;
    }
    
    MYSQL_RES *res;
   	MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
	    dcs_log(0, 0, "get xpep_retcode info  :1 Couldn't get result from %s\n", mysql_error(sock));
	    //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
	    return -1;
    }
   
   	num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) 
    {	
       	memcpy(i_ewp_info, row[0] ? row[0] : "\0", (int)sizeof(i_ewp_info));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
    }
    else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetDetailInfo1:未找到符合条件的记录,再查找交易码是FFF");
		memset(sql, 0x00, sizeof(sql));
	    /*sprintf(sql, "SELECT xpep_content_ewp FROM xpep_retcode  WHERE xpep_retcode = '%s' and xpep_trancde = 'FFF';", rtnCode);*/
	    sprintf(sql, "SELECT xpep_content FROM xpep_retcode  WHERE xpep_retcode = '%s' and xpep_trancde = 'FFF';", rtnCode);
		
	    if(mysql_query(sock, sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "get xpep_retcode info error2 [%s], trancde = FFF!", mysql_error(sock));
	        //从连接池获取的连接需要释放掉
			if(GetIsoMultiThreadFlag())
				SetFreeLink(sock);
	        return -1;
	    }
	    
	    MYSQL_RES *res2 = NULL;
	    num_fields = 0;
	 	num_rows = 0;

		if (!(res2 = mysql_store_result(sock))) {
		    dcs_log(0, 0, "get xpep_retcode info  : Couldn't get result from %s\n", mysql_error(sock));
		    //从连接池获取的连接需要释放掉
			if(GetIsoMultiThreadFlag())
				SetFreeLink(sock);
		    return -1;
		}
		 //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
	    num_fields = mysql_num_fields(res2);
	    row = mysql_fetch_row(res2);
	    num_rows = mysql_num_rows(res2);
	    if(num_rows == 1) 
		{
	        memcpy(i_ewp_info, row[0] ? row[0] : "\0", (int)sizeof(i_ewp_info));
	     	mysql_free_result(res2);
	    }
	    else if(num_rows == 0)
		{
			#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
			dcs_log(0, 0, "GetDetailInfo2:FFF未找到符合条件的记录");
			mysql_free_result(res2);
			return -1;
		}
		else if(num_rows > 1)
		{
			#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
			dcs_log(0, 0, "GetDetailInfo2:FFF找到多条符合条件的记录");
			mysql_free_result(res2);
			return -1;
		}
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetDetailInfo2:找到多条符合条件的记录");
		 //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		mysql_free_result(res);
		return -1;
	}
  	mysql_free_result(res);
 
	memcpy(ewp_info, i_ewp_info, getstrlen(i_ewp_info));
	len = getstrlen(ewp_info);
	
	#ifdef __LOG__
		dcs_log(0, 0, "ewp_info = [%s], len = [%d]", ewp_info, len);
	#endif
	
	return len;
}

/*根据用户名查询数据表pos_cust_info得到商户号和终端号*/
int GetCustInfo(char *username, char *psam, POS_CUST_INFO *opos_cust_info)
{
    POS_CUST_INFO pos_cust_info;
    int num;
    char i_username[10];
    char i_psam[17];
	
	memset(i_username, 0, sizeof(i_username));
	memset(i_psam, 0, sizeof(i_psam));
	memset(&pos_cust_info, 0, sizeof(POS_CUST_INFO));
	
	if(getstrlen(username)<=2)
		return -1;
	memcpy(i_username, username, getstrlen(username)-2);
	memcpy(i_psam, psam, getstrlen(psam));
    
	#ifdef __LOG__
		dcs_log(0, 0, "i_username = [%s]", i_username);
		dcs_log(0, 0, "i_psam = [%s]", i_psam);
	#endif
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
    char sql[512];
    memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "SELECT count(*) from pos_cust_info where substr(cust_id,1,9) = '%s' and cust_psam = '%s';", i_username, i_psam);
    
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetCustInfo info error [%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
	 
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetCustInfo:Couldn't get result from %s\n", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    
    if(mysql_num_rows(res) == 1) {
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
		dcs_log(0, 0, "GetCustInfo:操作数据表失败");
		//从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		mysql_free_result(res);
		return -1;
	}
    
    if(num != 1)
	{
		dcs_log(0, 0, "get pos_cust_info count error");
		//从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return -2;
	}
    mysql_free_result(res);
    
    memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "SELECT cust_id, cust_psam, cust_merid, cust_termid, cust_incard, cust_ebicode, cust_merdescr, cust_merdescr_utf8,t0_flag,agents_code FROM pos_cust_info  \
	WHERE substr(cust_id,1,9) = '%s' and cust_psam = '%s';",i_username, i_psam);
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetCustInfo info error [%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
        if(GetIsoMultiThreadFlag())
        	SetFreeLink(sock);
        return -1;
    }
    
    MYSQL_RES *res2;
    if(!(res2 = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetCustInfo:Couldn't get result from %s\n", mysql_error(sock));
        //从连接池获取的连接需要释放掉
        if(GetIsoMultiThreadFlag())
        	SetFreeLink(sock);
        return -1;
    }
    //从连接池获取的连接需要释放掉
    if(GetIsoMultiThreadFlag())
    	SetFreeLink(sock);
    num_fields = mysql_num_fields(res2);
    row = mysql_fetch_row(res2);
    num_rows = mysql_num_rows(res2);
    if(num_rows == 1) {
        memcpy(pos_cust_info.cust_id, row[0] ? row[0] : "\0", (int)sizeof(pos_cust_info.cust_id));
        memcpy(pos_cust_info.cust_psam, row[1] ? row[1] : "\0", (int)sizeof(pos_cust_info.cust_psam));
        memcpy(pos_cust_info.cust_merid, row[2] ? row[2] : "\0", (int)sizeof(pos_cust_info.cust_merid));
        memcpy(pos_cust_info.cust_termid, row[3] ? row[3] : "\0", (int)sizeof(pos_cust_info.cust_termid));
        memcpy(pos_cust_info.incard, row[4] ? row[4] : "\0", (int)sizeof(pos_cust_info.incard));
        memcpy(pos_cust_info.cust_ebicode, row[5] ? row[5] : "\0", (int)sizeof(pos_cust_info.cust_ebicode));
        memcpy(pos_cust_info.merdescr, row[6] ? row[6] : "\0", (int)sizeof(pos_cust_info.merdescr));
        memcpy(pos_cust_info.merdescr_utf8, row[7] ? row[7] : "\0", (int)sizeof(pos_cust_info.merdescr_utf8));
		if(row[8])
			pos_cust_info.T0_flag[0] = row[8][0];
        memcpy(pos_cust_info.agents_code, row[9] ? row[9] : "\0", (int)sizeof(pos_cust_info.agents_code));
    }
 
    mysql_free_result(res2);
    
#ifdef __LOG__
	dcs_log(0, 0, "pos_cust_info.cust_merid = [%s], pos_cust_info.cust_termid = [%s]", pos_cust_info.cust_merid, pos_cust_info.cust_termid);
	dcs_log(0, 0, "pos_cust_info.cust_ebicode = [%s]", pos_cust_info.cust_ebicode);
	dcs_log(0, 0, "pos_cust_info.merdescr = [%s], pos_cust_info.merdescr_utf8 = [%s]", pos_cust_info.merdescr, pos_cust_info.merdescr_utf8);
	dcs_log(0, 0, "pos_cust_info.T0_flag = [%s], pos_cust_info.agents_code = [%s]", pos_cust_info.T0_flag, pos_cust_info.agents_code);
#endif
	memcpy(opos_cust_info, &pos_cust_info, sizeof(POS_CUST_INFO));
	
	return 1;
}

/*
 根据pos终端送上的商户号和终端号查询其属于哪个渠道，
 并且判断该渠道是否允许该交易码指定的交易
 输入参数：mer_info_t用来保存POS上送的商户号和终端号的结构体
 trancde：交易码，区分交易
 返回值：0:渠道允许交易正常进行
 -1：该渠道没有配置该交易，交易不受理
 -2：该渠道限制所有交易
 -3：该渠道限制该交易(该交易未开通)
 -4：查询pos_channel_info数据表失败
 */
int GetChannelRestrict(MER_INFO_T mer_info_t, char *trancde)
{
	char i_mgr_termid[8+1];
	char i_mgr_merid[15+1];
	int i_channel_id;
    int rtn ;
    char i_trancde[3+1];
	char sql[1024];
	
	memset(i_mgr_termid, 0, sizeof(i_mgr_termid));
	memset(i_mgr_merid, 0, sizeof(i_mgr_merid));
	memset(i_trancde, 0, sizeof(i_trancde));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_mgr_termid, mer_info_t.mgr_term_id, 8);
	memcpy(i_mgr_merid, mer_info_t.mgr_mer_id, 15);
	memcpy(i_trancde, trancde, 3);
	
	sprintf(sql, "SELECT channel_id FROM pos_channel_info  WHERE mgr_term_id = '%s' and mgr_mer_id = '%s';",\
	i_mgr_termid, i_mgr_merid);
	
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetChannelRestrict error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetChannelRestrict: Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		if(row[0])
			i_channel_id = atoi(row[0]);
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetChannelRestrict:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetChannelRestrict:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	mysql_free_result(res);
	
	#ifdef __LOG__
		dcs_log(0, 0, "i_channel_id = [%d]", i_channel_id);
	#endif
	rtn = ChannelTradeCharge(i_trancde, i_channel_id);
	if(rtn == 0)
		rtn = i_channel_id;
	return rtn;
}

/*冲正时更新数据表pep_jnls字段reversedflag为1*/
int Update_reverflag(ISO_data iso, char *trace, char *termidm)
{
	char i_mercode[15+1];
	char i_termcde[8+1];
	char i_batchno[6+1];
	long i_trace;
	char i_termidm[25+1];
	char i_curdate[8+1];
	char o_curdate[9];
	char tmpbuf[27];
	char sql[1024];
	int s;
	
	struct tm *tm;   
	time_t lastTime;
	
    dcs_log(0, 0, "冲正，更新冲正标识字段。");
    
    memset(o_curdate, 0, sizeof(o_curdate));
    memset(i_curdate, 0, sizeof(i_curdate));
	memset(i_termidm, 0, sizeof(i_termidm));
    memset(i_mercode, 0, sizeof(i_mercode));
    memset(i_termcde, 0, sizeof(i_termcde));
    memset(i_batchno, 0, sizeof(i_batchno));
    memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(sql, 0, sizeof(sql));
 
    memcpy(i_termidm, termidm, getstrlen(termidm));
    
    /*昨天*/
    lastTime = time(NULL) - 86400;
	tm = localtime(&lastTime);
	sprintf(o_curdate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	/*今天*/
	time(&lastTime);
	tm = localtime(&lastTime);
	sprintf(i_curdate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);

	s = getbit(&iso, 60,(unsigned char *)tmpbuf);
	if(s < 0)
	{
	dcs_log(0, 0, "get 60_error");
	return -1;
	}
	memcpy(i_batchno, tmpbuf+2, 6);

	getbit(&iso, 41, (unsigned char *)i_termcde);
	getbit(&iso, 42, (unsigned char *)i_mercode);
	sscanf(trace, "%06ld", &i_trace);
    
    #ifdef __LOG__
		dcs_log(0, 0, "i_termcde = [%s]", i_termcde);
		dcs_log(0, 0, "i_mercode = [%s]", i_mercode);
		dcs_log(0, 0, "i_trace = [%ld]", i_trace);
		dcs_log(0, 0, "i_curdate = [%s]", i_curdate);	
		dcs_log(0, 0, "o_curdate = [%s]", o_curdate);	
		dcs_log(0, 0, "i_termidm = [%s]", i_termidm);	
	#endif
    
	sprintf(sql, "UPDATE pep_jnls set reversedflag = '1' where trace= %ld and termcde= '%s' and mercode= '%s' \
	and batchno= '%s' and (pepdate= '%s' or pepdate = '%s') and termidm= '%s' and msgtype in ('0200', '0100');", i_trace,\
	i_termcde, i_mercode, i_batchno, i_curdate, o_curdate, i_termidm);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "POS_CONF_INFO errorr [%s]!", mysql_error(sock));
        return -1;
    }
	return 0;
}

/*
 函数功能：根据批次号和商户号终端号得到交易的信息
 输入参数：ISO_data siso,pos上送的报文得到商户号和终端号以及结算的批次号
 输出参数：xpep_cd_info结构体
 xpep_jieji_amount借记总金额
 xpep_daiji_cx_amount贷记(撤销)总金额
 xpep_daiji_th_amount贷记(退货)总金额
 xpep_daiji_amount贷记总金额
 xpep_jieji_count借记总次数
 xpep_daiji_cx_count贷记(撤销)总次数
 xpep_daiji_th_count贷记(退货)总次数
 xpep_daiji_count贷记总次数
 返回值： 0 ：成功  -1 查询数据失败
 */
int getjnlsdata(ISO_data siso, XPEP_CD_INFO *xpep_cd_info, char *termidm)
{
	char i_pos_mercode[15+1];
	char i_pos_termcode[8+1];
	char i_batch_no[6+1];
	char i_termidm[25+1];
	XPEP_CD_INFO i_xpep_cd_info;
    char tmpbuf[27];
	char sql[2048];
    
	memset(i_pos_mercode, 0, sizeof(i_pos_mercode));
	memset(i_pos_termcode, 0, sizeof(i_pos_termcode));
	memset(i_batch_no, 0, sizeof(i_batch_no));
	memset(i_termidm, 0, sizeof(i_termidm));
	memset(&i_xpep_cd_info, 0, sizeof(XPEP_CD_INFO));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_termidm, termidm, getstrlen(termidm));
	getbit(&siso, 41, (unsigned char *)i_pos_termcode);
	getbit(&siso, 42, (unsigned char *)i_pos_mercode);
	getbit(&siso, 60, (unsigned char *)tmpbuf);
	memcpy(i_batch_no, tmpbuf+2, 6);
	
	#ifdef __LOG__
		dcs_log(0, 0, "i_pos_mercode = [%s]", i_pos_mercode);
		dcs_log(0, 0, "i_pos_termcode = [%s]", i_pos_termcode);
		dcs_log(0, 0, "i_batch_no = [%s]", i_batch_no);
		dcs_log(0, 0, "i_termidm = [%s]", i_termidm);
	#endif
	
	sprintf(sql, "SELECT lpad(ifnull(sum(c1),'0'),12,'0'), sum(c2)  from(\
	SELECT sum(t1.tranamt) as c1, count(*) as c2 \
	FROM pep_jnls t1  WHERE t1.posmercode = '%s' \
			  and t1.postermcde = '%s'\
			  and t1.batchno = '%s'\
			  and (t1.aprespn = '00' or t1.aprespn ='10' or t1.aprespn ='11')\
			  and (t1.trancde = '000' or t1.trancde = '032'  or t1.trancde ='050')\
			  and t1.termidm = '%s'\
			  and (t1.reversedflag is null or t1.reversedflag = '0' or t1.reversedflag ='' or\
			  (t1.reversedflag = '1' and (t1.reversedans != '00' or t1.reversedans is null or t1.reversedans ='')))\
		UNION ALL\
	 SELECT sum(t2.tranamt) as c1,count(*) as c2\
		FROM pep_jnls_all t2  \
		WHERE t2.posmercode = '%s' \
			  and t2.postermcde = '%s'\
			  and t2.batchno = '%s'\
			  and (t2.aprespn = '00' or t2.aprespn = '10' or t2.aprespn = '11')\
			  and (t2.trancde = '000' or t2.trancde = '032'  or t2.trancde ='050')\
			  and t2.termidm = '%s'\
			  and (t2.reversedflag is null or t2.reversedflag = '0' or t2.reversedflag ='' or\
			  (t2.reversedflag = '1' and (t2.reversedans != '00' or t2.reversedans is null or t2.reversedans ='')))) as a;", i_pos_mercode, \
			  i_pos_termcode, i_batch_no, i_termidm, i_pos_mercode, i_pos_termcode, i_batch_no, i_termidm);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata error1=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "getjnlsdata : Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		memcpy(i_xpep_cd_info.xpep_jieji_amount, row[0] ? row[0] : "\0", (int)sizeof(i_xpep_cd_info.xpep_jieji_amount));
		if(row[1])
			i_xpep_cd_info.xpep_jieji_count = atoi(row[1]);
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata1:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata1:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	mysql_free_result(res);

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "SELECT lpad(ifnull(sum(c3),'0'),12,'0'), sum(c4) from(\
	SELECT sum(t1.tranamt) as c3 , count(*) as c4\
		FROM pep_jnls t1\
		WHERE t1.posmercode = '%s' \
			  and t1.postermcde = '%s'\
			  and t1.batchno = '%s'\
			  and (t1.aprespn = '00' or t1.aprespn = '11')\
			  and (t1.trancde = '001' or t1.trancde = '033')\
			  and t1.termidm = '%s'\
			  and (t1.reversedflag is null or t1.reversedflag = '0' or t1.reversedflag = '' or \
			  (t1.reversedflag = '1' and (t1.reversedans != '00' or t1.reversedans is null or t1.reversedans = '')))\
		UNION ALL\
	SELECT sum(t2.tranamt) as c3, count(*) as c4 \
		FROM pep_jnls_all t2\
		WHERE t2.posmercode = '%s' \
			  and t2.postermcde = '%s'\
			  and t2.batchno = '%s'\
			  and (t2.aprespn = '00' or t2.aprespn = '11')\
			  and (t2.trancde = '001'  or t2.trancde = '033')\
			  and t2.termidm = '%s'\
			  and (t2.reversedflag is null or t2.reversedflag ='' or t2.reversedflag = '0' or \
			  (t2.reversedflag = '1' and (t2.reversedans != '00' or t2.reversedans is null or t2.reversedflag ='')))) as a;", \
			  i_pos_mercode, i_pos_termcode, i_batch_no, i_termidm, i_pos_mercode, i_pos_termcode, i_batch_no, i_termidm);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata error2=[%s]!", mysql_error(sock));
        return -1;
    }
    
	num_fields = 0;
	num_rows = 0;

	
	if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "getjnlsdata : 2 Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		memcpy(i_xpep_cd_info.xpep_daiji_cx_amount, row[0] ? row[0] : "\0", (int)sizeof(i_xpep_cd_info.xpep_daiji_cx_amount));
		if(row[1])
			i_xpep_cd_info.xpep_daiji_cx_count =atoi(row[1]);
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata2:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata2:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	mysql_free_result(res);
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "SELECT lpad(ifnull(sum(c5),'0'),12,'0'), sum(c6) from(\
	SELECT sum(t1.tranamt) as c5, count(*) as c6\
	FROM pep_jnls t1\
		WHERE t1.posmercode = '%s'\
			  and t1.postermcde = '%s'\
			  and t1.batchno = '%s'\
			  and (t1.aprespn = '00' or t1.aprespn = '11')\
			  and (t1.trancde = '002' or t1.trancde = '051')\
			  and t1.termidm = '%s'\
		UNION ALL\
	SELECT sum(t2.tranamt) as c5, count(*) as c6\
	FROM pep_jnls_all t2\
		WHERE t2.posmercode = '%s' \
			  and t2.postermcde = '%s'\
			  and t2.batchno = '%s'\
			  and (t2.aprespn = '00' or t2.aprespn = '11')\
			  and (t2.trancde = '002' or t2.trancde = '051')\
			  and t2.termidm = '%s') as a;", i_pos_mercode, i_pos_termcode, i_batch_no, i_termidm, \
			  i_pos_mercode, i_pos_termcode, i_batch_no, i_termidm);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata error3=[%s]!", mysql_error(sock));
        return -1;
    }
    
	num_fields = 0;
	num_rows = 0;

	

	if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "getjnlsdata :3 Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		memcpy(i_xpep_cd_info.xpep_daiji_th_amount, row[0] ? row[0] : "\0", (int)sizeof(i_xpep_cd_info.xpep_daiji_th_amount));
		if(row[1])
			i_xpep_cd_info.xpep_daiji_th_count = atoi(row[1]);
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata3:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata3:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	mysql_free_result(res);
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "SELECT lpad(ifnull(sum(c7),'0'),12,'0'), sum(c8) from(\
	SELECT sum(t1.tranamt) as c7, count(*) as c8\
		FROM pep_jnls t1\
		WHERE t1.posmercode = '%s' \
			  and t1.postermcde = '%s'\
			  and t1.batchno = '%s'\
			  and (t1.aprespn = '00' or t1.aprespn ='11')\
			  and t1.termidm = '%s'\
			  and (t1.trancde = '002' or t1.trancde ='051' or (t1.trancde in('001', '033')  and (t1.reversedflag is null or t1.reversedflag = '0' or t1.reversedflag =''\
			  or (t1.reversedflag = '1' and (t1.reversedans != '00' or t1.reversedans is null or t1.reversedans ='')))))\
		UNION ALL\
	SELECT sum(t2.tranamt) as c7, count(*) as c8\
			FROM pep_jnls_all t2\
		WHERE t2.posmercode = '%s' \
			  and t2.postermcde = '%s'\
			  and t2.batchno = '%s'\
			  and (t2.aprespn = '00' or t2.aprespn = '11')\
			  and t2.termidm = '%s'\
			  and (t2.trancde = '002' or t2.trancde ='051' or (t2.trancde in('001', '033') and (t2.reversedflag is null or t2.reversedflag = '0' or t2.reversedflag ='' \
			  or (t2.reversedflag = '1' and (t2.reversedans != '00' or t2.reversedans is null or t2.reversedans ='')))))) as a;", i_pos_mercode,\
			  i_pos_termcode, i_batch_no, i_termidm, i_pos_mercode, i_pos_termcode, i_batch_no, i_termidm);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata error4=[%s]!", mysql_error(sock));
        return -1;
    }
    
	num_fields = 0;
	num_rows = 0;

	if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "getjnlsdata : 4 Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		memcpy(i_xpep_cd_info.xpep_daiji_amount, row[0] ? row[0] : "\0", (int)sizeof(i_xpep_cd_info.xpep_daiji_amount));
		if(row[1])
			i_xpep_cd_info.xpep_daiji_count = atoi(row[1]);
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata4:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getjnlsdata4:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	mysql_free_result(res);
	
	#ifdef __LOG__
		dcs_log(0, 0, "i_xpep_cd_info.xpep_jieji_amount = [%s]", i_xpep_cd_info.xpep_jieji_amount);
		dcs_log(0, 0, "i_xpep_cd_info.xpep_daiji_cx_amount = [%s]", i_xpep_cd_info.xpep_daiji_cx_amount);
		dcs_log(0, 0, "i_xpep_cd_info.xpep_daiji_th_amount = [%s]", i_xpep_cd_info.xpep_daiji_th_amount);
		dcs_log(0, 0, "i_xpep_cd_info.xpep_daiji_amount = [%s]", i_xpep_cd_info.xpep_daiji_amount);
		dcs_log(0, 0, "i_xpep_cd_info.xpep_jieji_count = [%d]", i_xpep_cd_info.xpep_jieji_count);
		dcs_log(0, 0, "i_xpep_cd_info.xpep_daiji_cx_count = [%d]", i_xpep_cd_info.xpep_daiji_cx_count);
		dcs_log(0, 0, "i_xpep_cd_info.xpep_daiji_th_count = [%d]", i_xpep_cd_info.xpep_daiji_th_count);
		dcs_log(0, 0, "i_xpep_cd_info.xpep_daiji_count = [%d]", i_xpep_cd_info.xpep_daiji_count);
	#endif
	memcpy(xpep_cd_info, &i_xpep_cd_info, sizeof(XPEP_CD_INFO));
	return 0;
}

/*
 函数功能：保存结算信息
 */
int saveSettlemtntJnls(ISO_data siso, char *currentDate, char *pos_damount, int pos_dcount,char *pos_camount, int pos_ccount, XPEP_CD_INFO xpep_cd_info, char *termidm)
{
	char xpepdate[8+1];
	char xpeptime[6+1];
	long trace;
	char  reference[12+1];
	char  pos_mercode[15+1];
	char  pos_termcode[8+1];
	char settlement_date[4+1];
	long batch_no;
	char operator_no[3+1];
	int settlement_result;
	char i_termidm[25+1];
	char sql[2048];
    char tmpbuf[27], i_batch_no[6+1], i_trace[6+1];
    long id;
    
    memset(xpepdate, 0, sizeof(xpepdate));
	memset(xpeptime, 0, sizeof(xpeptime));
	memset(reference, 0, sizeof(reference));
	memset(pos_mercode, 0, sizeof(pos_mercode));
	memset(pos_termcode, 0, sizeof(pos_termcode));
	memset(settlement_date, 0, sizeof(settlement_date));
	memset(operator_no, 0, sizeof(operator_no));
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(i_batch_no, 0, sizeof(i_batch_no));
	memset(i_trace, 0, sizeof(i_trace));
	memset(i_termidm, 0, sizeof(i_termidm));
	memset(sql, 0, sizeof(sql));
	
	memcpy(xpepdate, currentDate, 8);
	memcpy(xpeptime, currentDate+8, 6);
	
	getbit(&siso, 11, (unsigned char *)i_trace);
	sscanf(i_trace, "%06ld", &trace);
	getbit(&siso, 15, (unsigned char *)settlement_date);
	settlement_date[4]='\0';
	getbit(&siso, 37, (unsigned char *)reference);
	
	getbit(&siso, 41, (unsigned char *)pos_termcode);
	getbit(&siso, 42, (unsigned char *)pos_mercode);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	getbit(&siso, 60, (unsigned char *)tmpbuf);
	memcpy(i_batch_no, tmpbuf+2, 6);
	sscanf(i_batch_no, "%06ld", &batch_no);
	getbit(&siso, 63, (unsigned char *)operator_no);

	if(memcmp(pos_damount, xpep_cd_info.xpep_jieji_amount, 12)==0&&memcmp(pos_camount, xpep_cd_info.xpep_daiji_amount, 12)==0\
	&&(xpep_cd_info.xpep_jieji_count==pos_dcount)&&(xpep_cd_info.xpep_daiji_count==pos_ccount))
		settlement_result = 1;
	else
		settlement_result = 2;
	
	id = get_id_fpos_settlement_seq();
	if(id < 0 )
	{
		dcs_log(0, 0, "取得数据表pos_settlement_jnls的主键id失败!");
		return -1;
	}
		
	memcpy(i_termidm, termidm, getstrlen(termidm));
	
	#ifdef __LOG__
		dcs_log(0, 0, "xpepdate = [%s]", xpepdate);
		dcs_log(0, 0, "xpeptime = [%s]", xpeptime);
		dcs_log(0, 0, "trace = [%ld]", trace);
		dcs_log(0, 0, "reference = [%s]", reference);
		dcs_log(0, 0, "pos_mercode = [%s]", pos_mercode);
		dcs_log(0, 0, "pos_termcode = [%s]", pos_termcode);
		dcs_log(0, 0, "settlement_date = [%s]", settlement_date);
		dcs_log(0, 0, "batch_no = [%ld]", batch_no);
		dcs_log(0, 0, "operator_no = [%s]", operator_no);
		dcs_log(0, 0, "pos_damount = [%s]", pos_damount);
		dcs_log(0, 0, "pos_dcount = [%d]", pos_dcount);
		dcs_log(0, 0, "pos_camount = [%s]", pos_camount);
		dcs_log(0, 0, "pos_ccount = [%d]", pos_ccount);
		dcs_log(0, 0, "xpep_cd_info.xpep_jieji_amount = [%s]", xpep_cd_info.xpep_jieji_amount);
		dcs_log(0, 0, "xpep_cd_info.xpep_jieji_count = [%d]", xpep_cd_info.xpep_jieji_count);
		dcs_log(0, 0, "xpep_cd_info.xpep_daiji_cx_amount = [%s]", xpep_cd_info.xpep_daiji_cx_amount);
		dcs_log(0, 0, "xpep_cd_info.xpep_daiji_cx_count = [%d]", xpep_cd_info.xpep_daiji_cx_count);
		dcs_log(0, 0, "xpep_cd_info.xpep_daiji_th_amount = [%s]", xpep_cd_info.xpep_daiji_th_amount);
		dcs_log(0, 0, "xpep_cd_info.xpep_daiji_th_count = [%d]", xpep_cd_info.xpep_daiji_th_count);
		dcs_log(0, 0, "settlement_result = [%d]", settlement_result);
		dcs_log(0, 0, "id = [%ld]", id); 
		dcs_log(0, 0, "i_termidm = [%s]", i_termidm); 
	#endif
	
	sprintf(sql, "INSERT INTO pos_settlement_jnls(ID, XPEPDATE, XPEPTIME, TRACE, REFERENCE, POS_MERCODE, POS_TERMCODE,\
	SETTLEMENT_DATE, BATCH_NO, OPERATOR_NO, POS_JIEJI_AMOUNT, POS_JIEJI_COUNT, POS_DAIJI_AMOUNT, POS_DAIJI_COUNT, \
	XPEP_JIEJI_AMOUNT, XPEP_JIEJI_COUNT, XPEP_DAIJI_CX_AMOUNT, XPEP_DAIJI_CX_COUNT, XPEP_DAIJI_TH_AMOUNT, XPEP_DAIJI_TH_COUNT, \
	SETTLEMENT_RESULT, TERMIDM) VALUES (%ld, '%s', '%s', %ld, '%s', '%s', '%s', '%s', %ld, '%s', '%s', %d, '%s', %d, \
	'%s', %d, '%s', %d, '%s', %d, %d, '%s');", \
	id, xpepdate, xpeptime, trace, reference, pos_mercode, pos_termcode, settlement_date, batch_no, operator_no, pos_damount, \
	pos_dcount, pos_camount, pos_ccount, xpep_cd_info.xpep_jieji_amount, xpep_cd_info.xpep_jieji_count, \
	xpep_cd_info.xpep_daiji_cx_amount, xpep_cd_info.xpep_daiji_cx_count, xpep_cd_info.xpep_daiji_th_amount, \
	xpep_cd_info.xpep_daiji_th_count, settlement_result, i_termidm);
	
	if(mysql_query(sock, sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "saveSettlemtntJnls INSERT INTO pos_settlement_jnls DB error [%s]!", mysql_error(sock));
	        return -1;
    }
	return 0;
}


/*
 从pos_batch_send_sequence中
 获得批上送数据表pos_settlement_detail_jnls当前的主键ID
 */
long get_currid_fpos_batchsend_seq()
{
	long current_id = 0;
    char sql[512];
    memset(sql, 0x00, sizeof(sql));
    strcpy(sql, "select currval('settle_send_seq');");
    
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0,0,"Read get_id_fpos_batchsend_seq error [%s]!" ,mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows =0 ;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0,"get_id_fpos_batchsend_seq : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
        row = mysql_fetch_row(res);
        current_id = atol(row[0]);
    } 
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "get_currid_fpos_batchsend_seq:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "get_currid_fpos_batchsend_seq:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
    mysql_free_result(res);
	return current_id;
}


/*
 根据商户号、终端号、批次号查询结算表得到pid
 */
long getPid(char *posmercode, char *postermcde, long batchno)
{
	long new_pid;
	char sql[512];
	
	memset(sql, 0x00, sizeof(sql));
	#ifdef __LOG__
		dcs_log(0, 0, "posmercode = [%s]", posmercode);
		dcs_log(0, 0, "postermcde = [%s]", postermcde);
		dcs_log(0, 0, "batchno = [%ld]", batchno);
	#endif
	
	sprintf(sql, "select id from pos_settlement_jnls\
	where pos_mercode = '%s'\
	and pos_termcode = '%s'\
	and batch_no = %ld;", posmercode, postermcde, batchno);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getPid error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "getPid : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows =mysql_num_rows(res);
    if(num_rows == 1) {
		if(row[0])
			new_pid = atol(row[0]);
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getPid:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "getPid:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	mysql_free_result(res);
	return new_pid ;
}

/*
 从pos_batch_send_sequence中
 获得批上送数据表pos_settlement_detail_jnls自增长的主键ID
 */
long get_id_fpos_batchsend_seq()
{
	long new_trace = 0;
    char sql[512];
    memset(sql, 0x00, sizeof(sql));
    strcpy(sql, "select nextval('settle_send_seq');");
    
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0,0,"Read get_id_fpos_batchsend_seq error [%s]!" ,mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0,"get_id_fpos_batchsend_seq : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
        row = mysql_fetch_row(res);
        new_trace = atol(row[0]);
    }
    else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "get_id_fpos_batchsend_seq:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "get_id_fpos_batchsend_seq:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
    mysql_free_result(res);
    return new_trace;
}

/*
 保存批上送信息
 */
int savebatchsend_jnls(ISO_data siso, char *currentdate, char *termidm)
{
	int i_pep_jnls_count;
	int i_success_count;
	int i_failed_count;
	int i_package_count;
	long id;
	long i_pid;
	long i_trace;
	long i_batch_no;
	char i_bit48[322+1];
	char i_result[2+1];
	char i_reference[12+1];
	char i_netmanager_code[3+1];
	char i_senddate[8+1];
	char i_sendtime[6+1];
	char i_termidm[25+1];
	char sql[1024];
    
    BATCH_SEND_INFO batch_send_info;
    
    char tembuf[127], batchno[6+1], reference[12+1], aprespn[2+1];
    int s, i;
	i_pep_jnls_count = i_success_count = i_failed_count = i_package_count = 0;

	memset(i_bit48, 0, sizeof(i_bit48));
	memset(i_result, 0, sizeof(i_result));
	memset(i_reference, 0, sizeof(i_reference));
	memset(tembuf, 0, sizeof(tembuf));
	memset(batchno, 0, sizeof(batchno));
	memset(i_netmanager_code, 0, sizeof(i_netmanager_code));
	memset(reference, 0, sizeof(reference));
	memset(aprespn, 0, sizeof(aprespn));
	memset(i_senddate, 0, sizeof(i_senddate));
	memset(i_sendtime, 0, sizeof(i_sendtime));
	memset(&batch_send_info, 0, sizeof(BATCH_SEND_INFO));
	memset(i_termidm, 0, sizeof(i_termidm));
	memset(sql, 0x00, sizeof(sql));
	
	memcpy(i_termidm, termidm, getstrlen(termidm));
	memcpy(i_result, "00", 2);
	
	getbit(&siso, 41, (unsigned char *)batch_send_info.postermcde);
	getbit(&siso, 42, (unsigned char *)batch_send_info.posmercode);
	getbit(&siso, 37, (unsigned char *)i_reference);
		
	s = getbit(&siso, 11, (unsigned char *)tembuf);
	if(s < 0)
	{
		dcs_log(0, 0, "get bit_11 error");
		return -1;
	}
	sscanf(tembuf, "%06ld", &i_trace);
	
	memset(tembuf, 0, sizeof(tembuf));
	s = getbit(&siso, 60, (unsigned char *)tembuf);
	if(s < 0)
	{
		dcs_log(0, 0, "get bit_60 error");
		return -1;
	}
	memcpy(batchno, tembuf+2, 6);	
	sscanf(batchno, "%06ld", &i_batch_no);
	memcpy(i_netmanager_code, tembuf+8, 3);
	
	memset(tembuf, 0, sizeof(tembuf));
	s = getbit(&siso, 3, (unsigned char *)tembuf);
	if(s > 0)
	{
		sprintf(batch_send_info.termtrc, "%06ld", i_trace);
		batch_send_info.termtrc[6] = '\0';
		s = getbit(&siso, 4, (unsigned char *)batch_send_info.transamt);
		
		dcs_log(0, 0, "batch_send_info.posmercode=%s", batch_send_info.posmercode);
		
		memset(tembuf, 0, sizeof(tembuf));
		s = getbit(&siso, 35, (unsigned char *)tembuf);
		if(s > 0)
		{
			for(i=0; i<20; i++)
			{
				if(tembuf[i] == '=' )
					break;
			}
			if(i == 20)
				dcs_log(0, 0, "二磁数据错误。");
			else
			{
				memcpy(batch_send_info.cardno, tembuf, i);	 
				batch_send_info.cardno[i] = '\0';       	
//				dcs_log(0, 0, "batch_send_info.cardno=[%s]", batch_send_info.cardno);
			}			
		}
		else
		{
			dcs_log(0, 0, "二磁不存在");
		}
		memcpy(reference, i_reference, 12);
		memcpy(aprespn, "00", 2);
		id = get_currid_fpos_batchsend_seq();
		/*getbit(&siso,42,(unsigned char *)batch_send_info.posmercode);*/
		savebatchsend_detail(id, &batch_send_info, i_batch_no, reference, aprespn);
		return 0;
	}
	id = get_id_fpos_batchsend_seq();
	if(id < 0 )
	{
		dcs_log(0, 0, "取得数据表pos_settlement_detail_jnls的主键id失败!");
		return -1;
	}
	
	i_pid = getPid(batch_send_info.posmercode, batch_send_info.postermcde, i_batch_no);
	
	s = getbit(&siso, 48, (unsigned char *)i_bit48);
	if(s < 0)
	{
		dcs_log(0, 0, "get bit_48 error");
		return -1;
	}
	
	if(memcmp(i_netmanager_code, "202", 3) == 0 || memcmp(i_netmanager_code, "207", 3) == 0)/*批上送结束*/
	{
		memset(tembuf, 0, sizeof(tembuf));
		memcpy(tembuf, i_bit48, 4);
		sscanf(tembuf, "%d", &i_package_count);
	}
	else
	{
		memset(tembuf, 0, sizeof(tembuf));
		memcpy(tembuf, i_bit48, 2);
		sscanf(tembuf, "%d", &i_package_count);
	}
	memcpy(i_senddate, currentdate, 8);
	memcpy(i_sendtime, currentdate+8, 6);
	
	#ifdef __LOG__
		dcs_log(0, 0, "id = [%ld]", id);
		dcs_log(0, 0, "i_pid = [%ld]", i_pid);
		dcs_log(0, 0, "i_trace = [%ld]", i_trace);
		dcs_log(0, 0, "i_reference = [%s]", i_reference);
		dcs_log(0, 0, "batch_send_info.posmercode = [%s]", batch_send_info.posmercode);
		dcs_log(0, 0, "batch_send_info.postermcde = [%s]", batch_send_info.postermcde);
		dcs_log(0, 0, "i_batch_no = [%ld]", i_batch_no);
		dcs_log(0, 0, "i_result = [%s]", i_result);
		dcs_log(0, 0, "i_package_count = [%d]", i_package_count);
		dcs_log(0, 0, "i_pep_jnls_count = [%d]", i_pep_jnls_count);
		dcs_log(0, 0, "i_success_count = [%d]", i_success_count);
		dcs_log(0, 0, "i_failed_count = [%d]", i_failed_count);
		dcs_log(0, 0, "i_bit48 = [%s]", i_bit48);
		dcs_log(0, 0, "i_senddate = [%s]", i_senddate);
		dcs_log(0, 0, "i_sendtime = [%s]", i_sendtime);
	#endif
	
	sprintf(sql, "INSERT INTO pos_settlement_detail_jnls(ID, PID, TRACE, REFERENCE, POS_MERCODE, POS_TERMCODE, BATCH_NO, RESLUT,\
			NETMANAGER_CODE, PACKAGE_COUNT, PEP_JNLS_COUNT, SUCCESS_COUNT, FAILED_COUNT, BIT48, SENDDATE, SENDTIME) \
			VALUES (%ld, %ld, %ld, '%s', '%s', '%s', %ld, '%s', '%s', %d, %d, %d, %d, '%s', '%s', '%s');",\
			id, i_pid, i_trace, i_reference, batch_send_info.posmercode, batch_send_info.postermcde, i_batch_no, \
		i_result, i_netmanager_code, i_package_count, i_pep_jnls_count, i_success_count, i_failed_count, i_bit48, i_senddate, \
		i_sendtime);
	if(mysql_query(sock, sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "INSERT INTO pos_settlement_detail_jnls DB error [%s]!", mysql_error(sock));
	        return -1;
    }
 	
	if(memcmp(i_netmanager_code, "202", 3) != 0 && memcmp(i_netmanager_code, "207", 3) != 0)/*进行批上送未结束*/
	{
		for(i = 0; i < i_package_count; i++)
		{
			memcpy(batch_send_info.termtrc, i_bit48+2+i*40+2, 6);
			memset(tembuf, 0, sizeof(tembuf));
			memcpy(tembuf, i_bit48+2+i*40+8, 20);
			pub_rtrim_lp(tembuf, 20, batch_send_info.cardno, 0);
//			dcs_log(0, 0, "batch_send_info.cardno=[%s]", batch_send_info.cardno);
			memcpy(batch_send_info.transamt, i_bit48+2+i*40+28, 12);
			s = gettranscount(batch_send_info, batchno, reference, aprespn, i_termidm);
			if(s == 0)
				i_success_count++;
			else if(s == -1)
				i_failed_count++;
			else 
				dcs_log(0,0,"未找到或是出错了" );
			savebatchsend_detail(id, &batch_send_info, i_batch_no, reference, aprespn);
			
			memset(batch_send_info.termtrc, 0, sizeof(batch_send_info.termtrc));
			memset(batch_send_info.cardno, 0, sizeof(batch_send_info.cardno));
			memset(batch_send_info.transamt, 0, sizeof(batch_send_info.transamt));
		}
		i_pep_jnls_count = i_success_count + i_failed_count;
	}
 	#ifdef __LOG__	
		dcs_log(0, 0, "i_pep_jnls_count = [%d]", i_pep_jnls_count);
		dcs_log(0, 0, "i_success_count = [%d]", i_success_count);
		dcs_log(0, 0, "i_failed_count = [%d]", i_failed_count);
	#endif
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "update pos_settlement_detail_jnls set PEP_JNLS_COUNT = %d, SUCCESS_COUNT = %d, \
	FAILED_COUNT = %d where id = %ld and pid = %ld;", i_pep_jnls_count, i_success_count, i_failed_count, id, i_pid);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "update pos_settlement_detail_jnls DB error [%s]!", mysql_error(sock));
        return -1;
    }
	return 0;
}

/*
 从pos_batchsend_detail_sequence中
 获得批上送信息明细数据表pos_batch_send_detail_jnls自增长的主键ID
 */
long get_id_fbatchsend_detail_seq()
{
	long new_trace = 0;
    char sql[512];
    memset(sql, 0x00, sizeof(sql));
    strcpy(sql, "select nextval('settle_detail_seq');");
    
    if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "Read xpep_jnls_trace_sequence error [%s]!" ,mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "get_id_fbatchsend_detail_seq : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
        row = mysql_fetch_row(res);
        new_trace = atol(row[0]);
    }
    else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "get_id_fbatchsend_detail_seq:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "get_id_fbatchsend_detail_seq:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
    mysql_free_result(res);
    return  new_trace;
}



/*
 保存批上送信息明细表到pos_batch_send_detail_jnls中
 */
int savebatchsend_detail(long pid, BATCH_SEND_INFO *batch_send_info, long batch_no, char *o_reference, char *o_aprespn)
{
	long i_id;
	long i_trace;
	char i_cdno[20];
	char i_result[2+1];
	char i_reference[12+1];
	char i_termcde[8+1];
	char i_mercode[15+1];
	char i_transamt[12+1];
	char sql[1024];
    
    memset(i_cdno, 0, sizeof(i_cdno));
    memset(i_result, 0, sizeof(i_result));
    memset(i_reference, 0, sizeof(i_reference));
    memset(i_termcde, 0, sizeof(i_termcde));
    memset(i_mercode, 0, sizeof(i_mercode));
    memset(i_transamt, 0, sizeof(i_transamt));
	memset(sql, 0, sizeof(sql));
    
    memcpy(i_cdno, batch_send_info->cardno, getstrlen(batch_send_info->cardno));
    memcpy(i_result, o_aprespn, 2);
    memcpy(i_reference, o_reference, 12);
    memcpy(i_termcde, batch_send_info->postermcde, 8);
    memcpy(i_mercode, batch_send_info->posmercode, 15);
    memcpy(i_transamt, batch_send_info->transamt, 12);
 
    sscanf(batch_send_info->termtrc, "%06ld", &i_trace);
 	i_id = get_id_fbatchsend_detail_seq();
	if(i_id < 0)
	{
		dcs_log(0, 0, "get 批上送交易明细 表信息的主键Id失败");
		return -1;
	}
	
	#ifdef __LOG__
		dcs_log(0, 0, "i_id = [%ld]", i_id);
		dcs_log(0, 0, "pid = [%ld]", pid);
		dcs_log(0, 0, "i_trace = [%ld]", i_trace);
		dcs_log(0, 0, "i_transamt = [%s]", i_transamt);
		dcs_log(0, 0, "i_reference = [%s]", i_reference);
		dcs_log(0, 0, "i_termcde = [%s]", i_termcde);
		dcs_log(0, 0, "i_mercode = [%s]", i_mercode);
		dcs_log(0, 0, "batch_no = [%ld]", batch_no);
		dcs_log(0, 0, "i_result = [%s]", i_result);
	#endif 
	
	sprintf(sql, "INSERT INTO pos_batch_send_detail_jnls(ID, PID, TRACE, CARDNO, AMT, REFERENCE, POS_MERCODE, POS_TERMCODE, \
	BATCH_NO, RESLUT) VALUES(%ld, %ld, %ld, HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%s', '%s', '%s', %ld, '%s');", i_id, pid, i_trace, i_cdno, i_transamt,\
	i_reference, i_mercode, i_termcde, batch_no, i_result);

	if(mysql_query(sock, sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "INSERT INTO pos_batch_send_detail_jnls DB error [%s]!", mysql_error(sock));
	        return -1;
    }	
	return 0;
}


/*
 根据上送的批次流水号、批次号、终端号和商户号卡号 金额等查询原笔交易
 原笔交易成功的返回 0 ，原笔交易失败的 返回-1 未找到原笔交易返回-2
 */
int gettranscount(BATCH_SEND_INFO ibatch_send_info, char *batchno, char *o_reference, char *o_aprespn, char *termidm)
{	
	char i_aprespn[2+1];
	char i_postermcde[8+1];
	char i_posmercode[15+1];
	char i_reference[12+1];
	char i_cdno[20];
	char i_termidm[25+1];
	char sql[1024];
	int s = 0;
	
	s = getstrlen(ibatch_send_info.cardno);
	memset(i_aprespn, 0, sizeof(i_aprespn));
	memset(i_postermcde, 0, sizeof(i_postermcde));
	memset(i_posmercode, 0, sizeof(i_posmercode));
	memset(i_cdno, 0, sizeof(i_cdno));
	memset(i_reference, 0, sizeof(i_reference));
	memset(i_termidm, 0, sizeof(i_termidm));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_termidm, termidm, getstrlen(termidm));
	memcpy(i_postermcde, ibatch_send_info.postermcde, 8);
	memcpy(i_posmercode, ibatch_send_info.posmercode, 15);
	memcpy(i_cdno, ibatch_send_info.cardno, s);
	i_cdno[s]='\0';
	
	#ifdef __LOG__
		dcs_log(0, 0, "i_postermcde = [%s]", i_postermcde);
		dcs_log(0, 0, "i_posmercode = [%s]", i_posmercode);
		dcs_log(0, 0, "batchno = [%s]", batchno);
		dcs_log(0, 0, "ibatch_send_info.termtrc = [%s]", ibatch_send_info.termtrc);
//		dcs_log(0, 0, "i_cdno = [%s]", i_cdno);
		dcs_log(0, 0, "ibatch_send_info.transamt = [%s]", ibatch_send_info.transamt);
		dcs_log(0, 0, "i_termidm = [%s]", i_termidm);
	#endif
	
	sprintf(sql, "select aprespn, syseqno from pep_jnls \
	where  postermcde = '%s'\
	and posmercode = '%s'\
	and batchno = '%s'\
	and termtrc = '%s'\
	and trim(AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'))= '%s'\
	and tranamt = '%s'\
	and termidm = '%s'\
	UNION\
	select aprespn, syseqno from pep_jnls_all \
	where  postermcde = '%s'\
	and posmercode = '%s'\
	and batchno = '%s'\
	and termtrc = '%s'\
	and trim(AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'))= '%s'\
	and tranamt = '%s'\
	and termidm = '%s';", \
	i_postermcde, i_posmercode, batchno, ibatch_send_info.termtrc, i_cdno, ibatch_send_info.transamt, i_termidm, \
	i_postermcde, i_posmercode, batchno, ibatch_send_info.termtrc, i_cdno, ibatch_send_info.transamt, i_termidm);
	
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "gettranscount error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0 ;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "gettranscount : Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		memcpy(i_aprespn, row[0] ? row[0] : "\0", (int)sizeof(i_aprespn));
		memcpy(i_reference, row[1] ? row[1] : "\0", (int)sizeof(i_reference));
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "gettranscount:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "gettranscount:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	mysql_free_result(res);

	memcpy(o_aprespn, i_aprespn, 2);
	memcpy(o_reference, i_reference, 12);
	if(memcmp(i_aprespn, "00", 2)== 0)
		return 0;
	else
		return -1;
}

/*
 根据商户号和终端号还有批次号更新数据表pos_conf_info中的批次号
 */
int updatepos_conf_info(ISO_data siso, MER_INFO_T mer_info_t, char *termidm)
{
	char i_termcde[8+1];
	char i_mercode[15+1];
	char i_batchno[6+1];
	char i_termidm[25+1];
	char sql[1024];
	
	char tembuf[27], temp_batchno[6+1];
	long batchno;
	int s;
	
	memset(tembuf, 0, sizeof(tembuf));
	memset(i_termcde, 0, sizeof(i_termcde));
	memset(i_mercode, 0, sizeof(i_mercode));
	memset(i_batchno, 0, sizeof(i_batchno));
	memset(temp_batchno, 0, sizeof(temp_batchno));
	memset(i_termidm, 0, sizeof(i_termidm));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_termcde, mer_info_t.mgr_term_id, 8);
	memcpy(i_mercode, mer_info_t.mgr_mer_id, 15);
	memcpy(i_termidm, termidm, getstrlen(termidm));

	s = getbit(&siso, 60,(unsigned char *)tembuf);
	if(s < 0)
	{
		dcs_log(0, 0, "更新批次号时取60域失败");
		return -1;
	}
	memcpy(i_batchno, tembuf+2, 6);
	sscanf(i_batchno, "%06ld", &batchno);
	if(batchno!=999999)
		batchno += 1;
	else
		batchno = 1;
	sprintf(temp_batchno, "%06ld", batchno);
	
	sprintf(sql, "update pos_conf_info set mgr_batch_no = '%s'\
	where  mgr_term_id = '%s'\
	and mgr_mer_id = '%s'\
	and mgr_batch_no = '%s'\
	and pos_machine_id = '%s';", temp_batchno, i_termcde, i_mercode, i_batchno, i_termidm);
	if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "update pos_conf_info  errorr [%s]!", mysql_error(sock));
        return -1;
    }
		
	return 0 ;
}

/*根据机具编号查询数据表pos_conf_info*/
int GetParameter(char *mobileid, MER_INFO_T *omer_info_t)
{
	char i_mobile_id[25+1];
	MER_INFO_T mer_info_t;
	int i_signout_flag;
	char sql[1024];
	
	memset(i_mobile_id, 0, sizeof(i_mobile_id));
	memset(&mer_info_t, 0, sizeof(MER_INFO_T));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_mobile_id, mobileid, strlen(mobileid));
	
	#ifdef __LOG__
		dcs_log(0, 0, "mobileid = [%s], len =%d", mobileid, strlen(mobileid));
		dcs_log(0, 0, "i_mobile_id = [%s]", i_mobile_id);
	#endif
	mer_info_t.encrpy_flag = omer_info_t->encrpy_flag;
	
	sprintf(sql, "select mgr_term_id, mgr_mer_id, mgr_batch_no, mgr_mer_des, mgr_desflg, mgr_signout_flag, mgr_tpdu, \
	track_pwd_flag, pos_type, cz_retry_count, trans_telno1, trans_telno2, trans_telno3, manager_telno, hand_flag, \
	trans_retry_count, pos_main_key_index, new_version_info, pos_ip_port1, pos_ip_port2, rtn_track_pwd_flag, mgr_timeout,\
	input_order_flg, pos_machine_type, pos_update_flag, pos_update_add, pos_machine_id, remitfeeformula, creditfeeformula,\
	tag26, tag39, pos_kek2, kek2_check_value, para_update_flag, para_update_mode, pos_update_mode from pos_conf_info\
	where pos_machine_id = '%s';", i_mobile_id);
	
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetPosInfo error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetPosInfo : Couldn't get result from %s\n", mysql_error(sock));
        
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		memcpy(mer_info_t.mgr_term_id, row[0] ? row[0] : "\0", (int)sizeof(mer_info_t.mgr_term_id));
		memcpy(mer_info_t.mgr_mer_id, row[1] ? row[1] : "\0", (int)sizeof(mer_info_t.mgr_mer_id));
		memcpy(mer_info_t.mgr_batch_no, row[2] ? row[2] : "\0", (int)sizeof(mer_info_t.mgr_batch_no));
		memcpy(mer_info_t.mgr_mer_des, row[3] ? row[3] : "\0", (int)sizeof(mer_info_t.mgr_mer_des));
		memcpy(mer_info_t.mgr_des, row[4] ? row[4] : "\0", (int)sizeof(mer_info_t.mgr_des));
		if(row[5])
			i_signout_flag = atoi(row[5]);
		memcpy(mer_info_t.mgr_tpdu, row[6] ? row[6] : "\0", (int)sizeof(mer_info_t.mgr_tpdu));
		memcpy(mer_info_t.track_pwd_flag, row[7] ? row[7] : "\0", (int)sizeof(mer_info_t.track_pwd_flag));
		memcpy(mer_info_t.pos_type, row[8] ? row[8] : "\0", (int)sizeof(mer_info_t.pos_type));
		if(row[9])
			mer_info_t.cz_retry_count = atoi(row[9]);
		memcpy(mer_info_t.trans_telno1, row[10] ? row[10] : "\0", (int)sizeof(mer_info_t.trans_telno1));
		memcpy(mer_info_t.trans_telno2, row[11] ? row[11] : "\0", (int)sizeof(mer_info_t.trans_telno2));
		memcpy(mer_info_t.trans_telno3, row[12] ? row[12] : "\0", (int)sizeof(mer_info_t.trans_telno3));
		memcpy(mer_info_t.manager_telno, row[13] ? row[13] : "\0", (int)sizeof(mer_info_t.manager_telno));
		if(row[14])
			mer_info_t.hand_flag = atoi(row[14]);
		if(row[15])
			mer_info_t.trans_retry_count = atoi(row[15]);
		memcpy(mer_info_t.pos_main_key_index, row[16] ? row[16] : "\0", (int)sizeof(mer_info_t.pos_main_key_index));
		memcpy(mer_info_t.new_version_info, row[17] ? row[17] : "\0", (int)sizeof(mer_info_t.new_version_info));
		memcpy(mer_info_t.pos_ip_port1, row[18] ? row[18] : "\0", (int)sizeof(mer_info_t.pos_ip_port1));
		memcpy(mer_info_t.pos_ip_port2, row[19] ? row[19] : "\0", (int)sizeof(mer_info_t.pos_ip_port2));
		memcpy(mer_info_t.rtn_track_pwd_flag, row[20] ? row[20] : "\0", (int)sizeof(mer_info_t.rtn_track_pwd_flag));
		if(row[21])
			mer_info_t.mgr_timeout = atoi(row[21]);
		memcpy(mer_info_t.input_order_flg, row[22] ? row[22] : "\0", (int)sizeof(mer_info_t.input_order_flg));
		if(row[23])
			mer_info_t.pos_machine_type = atoi(row[23]);
		if(row[24])
			mer_info_t.pos_update_flag = atoi(row[24]);
		memcpy(mer_info_t.pos_update_add, row[25] ? row[25] : "\0", (int)sizeof(mer_info_t.pos_update_add));
		memcpy(mer_info_t.pos_machine_id, row[26] ? row[26] : "\0", (int)sizeof(mer_info_t.pos_machine_id));
		memcpy(mer_info_t.remitfeeformula, row[27] ? row[27] : "\0", (int)sizeof(mer_info_t.remitfeeformula));
		memcpy(mer_info_t.creditfeeformula, row[28] ? row[28] : "\0", (int)sizeof(mer_info_t.creditfeeformula));
		if(row[29])
			mer_info_t.tag26 = atoi(row[29]);
		if(row[30])
			mer_info_t.tag39 = atoi(row[30]);
		memcpy(mer_info_t.pos_kek2, row[31] ? row[31] : "\0", (int)sizeof(mer_info_t.pos_kek2));
		memcpy(mer_info_t.kek2_check_value, row[32] ? row[32] : "\0", (int)sizeof(mer_info_t.kek2_check_value));
		
		if(row[33])
			mer_info_t.para_update_flag = row[33][0];
		if(row[34])
			mer_info_t.para_update_mode = row[34][0];
		if(row[35])
			mer_info_t.pos_update_mode = row[35][0];
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetParameter:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetParameter:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	mysql_free_result(res);
	
	sprintf(mer_info_t.mgr_signout_flag, "%d", i_signout_flag);
	#ifdef __LOG__
		dcs_log(0, 0, "mer_info_t.mgr_term_id = [%s]", mer_info_t.mgr_term_id);
		dcs_log(0, 0, "mer_info_t.mgr_mer_id = [%s]", mer_info_t.mgr_mer_id);
		dcs_log(0, 0, "mer_info_t.mgr_batch_no = [%s]", mer_info_t.mgr_batch_no);
		dcs_log(0, 0, "mer_info_t.mgr_mer_des = [%s]", mer_info_t.mgr_mer_des);
		dcs_log(0, 0, "mer_info_t.mgr_des = [%s]", mer_info_t.mgr_des);
		dcs_log(0, 0, "mer_info_t.mgr_signout_flag = [%s]", mer_info_t.mgr_signout_flag);
		dcs_log(0, 0, "mer_info_t.track_pwd_flag = [%s]", mer_info_t.track_pwd_flag);
		dcs_log(0, 0, "mer_info_t.pos_type = [%s]", mer_info_t.pos_type);
		dcs_log(0, 0, "mer_info_t.cz_retry_count = [%d]", mer_info_t.cz_retry_count);
		dcs_log(0, 0, "mer_info_t.trans_telno1 = [%s]", mer_info_t.trans_telno1);
		dcs_log(0, 0, "mer_info_t.trans_telno2 = [%s]", mer_info_t.trans_telno2);
		dcs_log(0, 0, "mer_info_t.trans_telno3 = [%s]", mer_info_t.trans_telno3);
		dcs_log(0, 0, "mer_info_t.manager_telno = [%s]", mer_info_t.manager_telno);
		dcs_log(0, 0, "mer_info_t.hand_flag = [%d]", mer_info_t.hand_flag);
		dcs_log(0, 0, "mer_info_t.trans_retry_count = [%d]", mer_info_t.trans_retry_count);
		dcs_log(0, 0, "mer_info_t.pos_main_key_index = [%s]", mer_info_t.pos_main_key_index);
		dcs_log(0, 0, "mer_info_t.pos_ip_port1 = [%s]", mer_info_t.pos_ip_port1);
		dcs_log(0, 0, "mer_info_t.pos_ip_port2 = [%s]", mer_info_t.pos_ip_port2);
		dcs_log(0, 0, "mer_info_t.rtn_track_pwd_flag = [%s]", mer_info_t.rtn_track_pwd_flag);
		dcs_log(0, 0, "mer_info_t.mgr_timeout = [%d]", mer_info_t.mgr_timeout);
		dcs_log(0, 0, "mer_info_t.input_order_flg = [%s]", mer_info_t.input_order_flg);
		dcs_log(0, 0, "mer_info_t.pos_machine_id = [%s]", mer_info_t.pos_machine_id);
		dcs_log(0, 0, "mer_info_t.remitfeeformula = [%s]", mer_info_t.remitfeeformula);
		dcs_log(0, 0, "mer_info_t.creditfeeformula = [%s]", mer_info_t.creditfeeformula);
		dcs_log(0, 0, "mer_info_t.tag26 = [%d]", mer_info_t.tag26);
		dcs_log(0, 0, "mer_info_t.tag39 = [%d]", mer_info_t.tag39);
		dcs_log(0, 0, "mer_info_t.para_update_flag  = [%c]", mer_info_t.para_update_flag);
		dcs_log(0, 0, "mer_info_t.para_update_mode  = [%c]", mer_info_t.para_update_mode);
		dcs_log(0, 0, "mer_info_t.pos_update_flag  = [%d]", mer_info_t.pos_update_flag);
		dcs_log(0, 0, "mer_info_t.pos_update_mode  = [%c]", mer_info_t.pos_update_mode);
		dcs_log(0, 0, "mer_info_t.new_version_info  = [%s]", mer_info_t.new_version_info);
		dcs_log(0, 0, "mer_info_t.pos_kek2 = [%s]", mer_info_t.pos_kek2);
		dcs_log(0, 0, "mer_info_t.kek2_check_value = [%s]", mer_info_t.kek2_check_value);	
	#endif
	
	memcpy(omer_info_t, &mer_info_t, sizeof(MER_INFO_T));
	return 0 ;
}
/*
 根据当前的批次号查询交易表看是否有有交易
 */
int GetJnlsFlg(char *batch_no, char *termidm, char *mercode, char *termcde)
{
	int i_count;
	char i_termidm[25+1];
	char i_mercode[15+1];
	char i_termcde[8+1];
	char sql[2048];
	
	memset(i_termidm, 0, sizeof(i_termidm));
	memset(i_mercode, 0, sizeof(i_mercode));
	memset(i_termcde, 0, sizeof(i_termcde));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_termidm, termidm, getstrlen(termidm));
	memcpy(i_mercode, mercode, getstrlen(mercode));
	memcpy(i_termcde, termcde, getstrlen(termcde));
	
	#ifdef __LOG__
		dcs_log(0, 0, "batch_no = [%s]", batch_no);
		dcs_log(0, 0, "i_termidm = [%s]", i_termidm);
		dcs_log(0, 0, "i_mercode = [%s]", i_mercode);
		dcs_log(0, 0, "i_termcde = [%s]", i_termcde);
	#endif
	
	sprintf(sql, "select sum(tmp_count) from (\
	select count(*) as tmp_count from pep_jnls t1\
	where t1.batchno = '%s'  \
		and t1.process !='310000'\
		and (t1.aprespn= '00' or t1.aprespn = '10' or t1.aprespn = '11')\
		and t1.termidm ='%s'\
		and t1.posmercode ='%s'\
		and t1.postermcde ='%s'\
		and (t1.reversedflag ='0' or (t1.reversedflag = '1' and (t1.reversedans !='00' or t1.reversedans is null or t1.reversedans='')))\
	UNION ALL\
	select count(*) as tmp_count from pep_jnls_all t2\
	where t2.batchno = '%s'\
		and t2.process !='310000'\
		and (t2.aprespn= '00' or t2.aprespn = '10' or t2.aprespn ='11')\
		and t2.termidm = '%s'\
		and t2.posmercode ='%s'\
		and t2.postermcde ='%s'\
		and (t2.reversedflag ='0' or (t2.reversedflag = '1' and (t2.reversedans !='00' or t2.reversedans is null or t2.reversedans='')))) as a;", \
		batch_no, i_termidm, i_mercode, i_termcde, batch_no, i_termidm, i_mercode, i_termcde);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetJnlsFlg error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetJnlsFlg : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		if(row[0])
			i_count = atoi(row[0]);
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetJnlsFlg:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetJnlsFlg:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	mysql_free_result(res);

	#ifdef __LOG__
		dcs_log(0, 0, "i_count00 = [%d]", i_count);
	#endif
	
	return i_count;
}
/*
 有交易的话再查询批结算表
 判断当前中心的批次号是否进行过批结算
 返回值：大于0 表示进行过批结算  否则未进行过批结算
 */
int GetSettleFlg(char *batch_no, char *termidm)
{
	long batchno;
	int i_count;
	char i_termidm[25+1];
	char sql[1024];
	
	memset(i_termidm, 0, sizeof(i_termidm));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_termidm, termidm, getstrlen(termidm));
	
	sscanf(batch_no ,"%06ld", &batchno);
	
	#ifdef __LOG__
		dcs_log(0, 0, "batch_no = [%s]", batch_no);
		dcs_log(0, 0, "batchno = [%ld]", batchno);
		dcs_log(0, 0, "i_termidm = [%ld]", i_termidm);
	#endif
	
	sprintf(sql, "select count(*) from pos_settlement_jnls\
			where batch_no = %ld and termidm = '%s';", batchno, i_termidm);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetSettleFlg error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetSettleFlg : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    
    if(mysql_num_rows(res) == 1) {
		if(row[0])
			i_count  = atoi(row[0]);
	}
	else 
	{
		dcs_log(0, 0, "GetSettleFlg:操作数据表失败");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);

	#ifdef __LOG__
		dcs_log(0, 0, "i_count11 = [%d]", i_count);
	#endif
	
	return i_count;
}

/*
 输入参数：pos机终端号和商户号
 输出参数：磁道主密钥
 成功返回0
 失败返回-1
 */
int GetPosTdk(MER_INFO_T *mer_info_t)
{
    char i_term_id[8+1];
    char i_mer_id[15+1];
  	
  	memset(i_term_id, 0, sizeof(i_term_id));
  	memset(i_mer_id, 0, sizeof(i_mer_id));
	
  	memcpy(i_term_id, mer_info_t->mgr_term_id, getstrlen(mer_info_t->mgr_term_id));
  	memcpy(i_mer_id, mer_info_t->mgr_mer_id, getstrlen(mer_info_t->mgr_mer_id));
	#ifdef __LOG__
		dcs_log(0, 0, "i_term_id=[%s]", i_term_id);
		dcs_log(0, 0, "i_mer_id=[%s]", i_mer_id);
	#endif    
    char sql[512];
    memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "SELECT  mgr_tdk  FROM pos_conf_info WHERE mgr_term_id = '%s' and mgr_mer_id = '%s'", \
	mer_info_t->mgr_term_id, mer_info_t->mgr_mer_id);
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetSettleFlg error=[%s]!", mysql_error(sock));
        return -1;
    }
	
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetPosTdk : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		memcpy(mer_info_t->mgr_tdk, row[0] ? row[0] : "\0", (int)sizeof(mer_info_t->mgr_tdk));
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetPosTdk:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetPosTdk:找到多条符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	mysql_free_result(res);
    
	#ifdef __LOG__
		dcs_log(0, 0, "mer_info_t->mgr_tdk=[%s]", mer_info_t->mgr_tdk);
	#endif
	return 0;
}

/*
flag = 1:保存软件版本信息
flag = 2:当是GPRS方式接入时保存GPRS信息
flag = 3：更新应用程序更新标记为false
flag = 4：更新参数更新标记为false
*/
int UpdatePos_info(MER_INFO_T mer_info_t, char *termidm, int flag)
{
	char i_term_id[8+1];
	char i_mer_id[15+1];
	char i_termidm[25+1];
	char i_temp[35+1];
	char sql[512];
  	
  	memset(i_term_id, 0, sizeof(i_term_id));
  	memset(i_mer_id, 0, sizeof(i_mer_id));
  	memset(i_termidm, 0, sizeof(i_termidm));
  	memset(i_temp, 0, sizeof(i_temp));
	
  	memcpy(i_term_id, mer_info_t.mgr_term_id, getstrlen(mer_info_t.mgr_term_id));
  	memcpy(i_mer_id, mer_info_t.mgr_mer_id, getstrlen(mer_info_t.mgr_mer_id));
  	memcpy(i_termidm, termidm, getstrlen(termidm));
  	
  	#ifdef __LOG__
  		dcs_log(0, 0, "i_term_id=[%s]", i_term_id);
  		dcs_log(0, 0, "i_mer_id=[%s]", i_mer_id);
  		dcs_log(0, 0, "i_termidm=[%s]", i_termidm);
  	#endif
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
  	if(flag == 1)
  	{
  		memcpy(i_temp, mer_info_t.cur_version_info, getstrlen(mer_info_t.cur_version_info));
		memset(sql, 0x00, sizeof(sql));
#ifdef __LOG__
		dcs_log(0, 0, "i_temp=[%s]", i_temp);
#endif
		sprintf(sql, "update pos_conf_info set cur_version_info = '%s' where pos_machine_id = '%s';", i_temp, i_termidm);

#ifdef __LOG__
		dcs_log(0, 0, "UpdatePos_info sql=[%s]", sql);
#endif
	}
  	else if(flag == 2)
  	{
  		memcpy(i_temp, mer_info_t.pos_gpsno, getstrlen(mer_info_t.pos_gpsno));
		memset(sql, 0x00, sizeof(sql));
#ifdef __LOG__
		dcs_log(0, 0, "mer_info_t.pos_gpsno=[%s]", mer_info_t.pos_gpsno);
		dcs_log(0, 0, "i_temp=[%s]", i_temp);
#endif
		sprintf(sql, "update pos_conf_info set pos_gpsno = '%s' where mgr_mer_id = '%s' and mgr_term_id = '%s' \
		and pos_machine_id = '%s';", i_temp, i_mer_id, i_term_id, i_termidm);
	}
	else if(flag == 3)
  	{
		memset(sql, 0x00, sizeof(sql));
		sprintf(sql, "update pos_conf_info set pos_update_flag = 0 where mgr_mer_id = '%s' and mgr_term_id = '%s' \
		and pos_machine_id = '%s';", i_mer_id, i_term_id, i_termidm);
	}
	else if(flag == 4)
  	{
		memset(sql, 0x00, sizeof(sql));
		sprintf(sql, "update pos_conf_info set para_update_flag = '0' where mgr_mer_id = '%s' and mgr_term_id = '%s' \
		and pos_machine_id = '%s';", i_mer_id, i_term_id, i_termidm);
	}
	if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "POS_CONF_INFO errorr [%s]!", mysql_error(sock));
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

/*
保存在pos_iccard_jnls数据库表中。
 插入成功：return 0
 插入失败：return -1
 */
int Save_Iccard_Info(ICCARD_INFO *iccard_info, char *termidm, char *termcde, char *mercode)
{
	char sql[512];
	memset(sql,0x00,sizeof(sql));
	#ifdef __TEST__
		dcs_log(0, 0, "termidm=[%s]", termidm);
		dcs_log(0, 0, "termcde=[%s]", termcde);
		dcs_log(0, 0, "mercode=[%s]", mercode);
		dcs_log(0, 0, "i_iccard_no=[%s]", iccard_info->iccard_no);
		dcs_log(0, 0, "i_left_num=[%d]",  iccard_info->iccard_remain_num);
	#endif
	sprintf(sql, "INSERT INTO pos_iccard_jnls(iccard_no, left_num, pos_no, pos_mercode, pos_termid) VALUES('%s', %d, '%s', '%s', '%s')",\
	iccard_info->iccard_no, iccard_info->iccard_remain_num, termidm, mercode, termcde);
	
	if(mysql_query(sock,sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "insert pos_iccard_jnls DB error [%s]!", mysql_error(sock));
	        return -1;
    }
	return 0;	
}
int updateCustKeyInfo(char *sendCde, char *sendDate, char *sendTime, char *sendRandom, char *responseCode, char *xpepRandom, char *randKey, char *sendUserData)
{
	char sql[512];
	memset(sql, 0x00, sizeof(sql));
	#ifdef __LOG__
		dcs_log(0, 0, "update custkey_info sendcde=[%s]", sendCde);
		dcs_log(0, 0, "update custkey_info sentdate=[%s]", sendDate);
		dcs_log(0, 0, "update custkey_info senttime=[%s]", sendTime);
		dcs_log(0, 0, "update custkey_info cderandom=[%s]", sendRandom);
		dcs_log(0, 0, "update custkey_info respcode=[%s]", responseCode);
		dcs_log(0, 0, "update custkey_info localrandom=[%s]", xpepRandom);
		dcs_log(0, 0, "update custkey_info secukey=[%s]", randKey);
		dcs_log(0, 0, "update custkey_info selfdata=[%s]", sendUserData);
	#endif
		
	sprintf(sql,"update custkey_info set sentdate ='%s', senttime = '%s', cderandom = '%s', respcode = '%s',localrandom = '%s', \
	secukey = '%s', selfdata = '%s' where  sendcde = '%s'",\
	sendDate, sendTime, sendRandom, responseCode, xpepRandom, randKey, sendUserData,sendCde);
	
	if(mysql_query(sock,sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "update custkey_info DB error [%s]!" ,mysql_error(sock));
        return -1;
    }
	return 0;	
}


int getCustKeyInfo(char *sendCde, char *sendRandom, char *xpepRandom)
{
	char sql[512];
	memset(sql, 0x00, sizeof(sql));

	#ifdef __LOG__
		dcs_log(0, 0, "getCustKeyInfo sendCde=[%s]", sendCde);
	#endif
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	sprintf(sql, "select cderandom, localrandom  from custkey_info where sendcde = '%s'", sendCde);
	if(mysql_query(sock, sql))
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "select custkey_info DB error [%s]!", mysql_error(sock));
		//从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return -1;
	}
	MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "getCustKeyInfo:Couldn't get result from %s\n", mysql_error(sock));
        //从连接池获取的连接需要释放掉
        if(GetIsoMultiThreadFlag())
        	SetFreeLink(sock);
        return -1;
    }
    //从连接池获取的连接需要释放掉
    if(GetIsoMultiThreadFlag())
    	SetFreeLink(sock);
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(mysql_num_rows(res) == 1) {
		// memcpy(sendRandom, row[0] ? row[0] : "\0", (int)sizeof(sendRandom));
		//memcpy(xpepRandom, row[1] ? row[1] : "\0", (int)sizeof(xpepRandom));
		memcpy(sendRandom, row[0] ? row[0] : "\0",strlen(row[0]));
		memcpy(xpepRandom, row[1] ? row[1] : "\0",strlen(row[1]));
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "getCustKeyInfo:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "getCustKeyInfo:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);
	#ifdef __LOG__
		dcs_log(0, 0, "getCustKeyInfo rtn sendRandom=[%s]", sendRandom);
		dcs_log(0, 0, "getCustKeyInfo rtn xpepRandom=[%s]", xpepRandom);
	#endif
	
	return 1;
}
/*获得当前卡的交易金额和成功交易的次数*/
int getamt_count(char *currentDate, EWP_INFO ewp_info, char *totalamt)
{
	char i_sendcde[8+1], sql[512], i_totalamt[12+1];
	int total_count = 0;
	
	memset(sql, 0x00, sizeof(sql));
	memset(i_sendcde, 0x00, sizeof(i_sendcde));
	memset(i_totalamt, 0x00, sizeof(i_totalamt));
	
	memcpy(i_sendcde, ewp_info.consumer_sentinstitu, 4);
	memcpy(i_sendcde+4, "0000", 4);
	
	sprintf(sql,"select lpad(IFNULL(sum(tranamt),'0'), 12,'0') ,count(*) from  pep_jnls where pepdate = '%s' \
	and trancde ='%s' and rtrim(AES_DECRYPT(UNHEX(outcdno), 'abcdefgh')) ='%s' and sendcde = '%s' and aprespn ='00'", currentDate, ewp_info.consumer_transcode, \
	ewp_info.consumer_cardno, i_sendcde);
	if(mysql_query(sock, sql))
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "select pep_jnls DB error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "getamt_count:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;
    if(num_rows == 1) {
       memcpy(i_totalamt, row[0] ? row[0] : "\0", (int)sizeof(i_totalamt));
       	if( row[1] )  total_count =atoi(row[1]);
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "getamt_count:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "getamt_count:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);
	memcpy(totalamt, i_totalamt, 12);
	#ifdef __LOG__
		dcs_log(0, 0, "i_totalamt=[%s]", i_totalamt);
		dcs_log(0, 0, "totalamt=[%s]", totalamt);
		dcs_log(0, 0, "total_count=[%d]", total_count);
	#endif
	return total_count;
}
/*
	产生16字节长度的密钥
*/
int GenrateKey(char *key)
{
    int i, k, s;
    struct tm *tm;
    time_t t;
    unsigned char buf[20], *p;
   
	if (key == NULL)
   		return 0;
   
	time(&t);
	tm = localtime(&t);
	
	memset(buf, 0, sizeof(buf));
	
	for(i=0, k=0; i<16; i++,k++)
	{
		srand(((tm->tm_sec*tm->tm_min)<<k)+k);
		s=rand();
		for( ; ; )
		{
			if(s != 0)
				break;
			else
			{
				srand(((tm->tm_sec*tm->tm_min)<<k)+k);
				k++;
				s = rand();
			}
		}
		p= (unsigned char*)&s;
		*(key) = *p;
		key++;
		*(key) = *(p+1);
		key++;
	}	
	return 1;
}

/*
	产生8字节长度的密钥,非ascii码
*/
int GenrateKey8(char *key)
{
    int i, k, s;
    struct tm *tm;
    time_t t;
    unsigned char buf[20], *p;
   
	if (key == NULL)
   		return 0;
   
	time(&t);
	tm = localtime(&t);
	
	memset(buf, 0, sizeof(buf));
	
	for(i=0 ,k=0; i<8; i++,k++)
	{
		srand(((tm->tm_sec*tm->tm_min)<<k)+k);
		s=rand();
		for( ; ; )
		{
			if(s != 0)
				break;
			else
			{
				srand(((tm->tm_sec*tm->tm_min)<<k)+k);
				k++;
				s = rand();
			}
		}
		p= (unsigned char*)&s;
		*(key) = *p;
		key++;
		*(key) = *(p+1);
		key++;
	}	
	return 1;
}

int CustWriteXepDb(ISO_data iso, EWP_INFO ewp_info, PEP_JNLS pep_jnls)
{	
	char i_sendcde[8+1];
	char tmpbuf[200], t_year[5];
	struct  tm *tm;   
	time_t  t;
	char sql[2048];
	
	memset(i_sendcde, 0, sizeof(i_sendcde));
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(t_year, 0x00, sizeof(t_year));
	memset(sql, 0x00, sizeof(sql));

    time(&t);
    tm = localtime(&t);
    
    sprintf(t_year,"%04d", tm->tm_year+1900);

	memcpy(pep_jnls.mmchgid, "0", 1);
	memcpy(pep_jnls.revodid, "1", 1);
	pep_jnls.mmchgnm = 0;

	if(getbit(&iso, 0,(unsigned char *)pep_jnls.msgtype)<0)
	{
		 dcs_log(0, 0, "can not get bit_0!");	
		 return -1;
	}

	if(getbit(&iso, 7,(unsigned char *)tmpbuf)<0)
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
	
	if(getstrlen(pep_jnls.tranamt) > 0)
		dcs_log(0, 0, "amt=[%s]", pep_jnls.tranamt);
	else
	{	
		getbit(&iso, 4, (unsigned char *)pep_jnls.tranamt);
		dcs_log(0, 0, "bit4 amt=[%s]", pep_jnls.tranamt);
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(getbit(&iso, 11, (unsigned char *)tmpbuf)<0)
	{
		 dcs_log(0, 0, "can not get bit_3!");	
		 return -1;
	}
	
	sscanf(tmpbuf, "%06ld", &pep_jnls.trace);
	
	memcpy(pep_jnls.trntime, ewp_info.consumer_senttime, 6);	
	memcpy(pep_jnls.trndate, ewp_info.consumer_sentdate, 8);
	memcpy(pep_jnls.termid, ewp_info.consumer_username, 11);

	if(getbit(&iso, 22, (unsigned char *)pep_jnls.poitcde)<0)
	{
		 dcs_log(0, 0, "can not get bit_3!");	
		 return -1;
	}
	getbit(&iso, 28, (unsigned char *)pep_jnls.filed28);

	/*发送机构代码 sendcde[8]*/
	memcpy(i_sendcde, ewp_info.consumer_sentinstitu, 4);
	memcpy(i_sendcde+4, "0000", 4);
	pep_jnls.merid=atoi(ewp_info.consumer_merid);
	memcpy(pep_jnls.translaunchway, ewp_info.translaunchway,strlen(ewp_info.translaunchway));
	
	pep_jnls.outcdno[getstrlen(pep_jnls.outcdno)] =0;
	pep_jnls.intcdno[getstrlen(pep_jnls.intcdno)] =0;
	pep_jnls.intcdbank[getstrlen(pep_jnls.intcdbank)] =0;
	pep_jnls.intcdname[getstrlen(pep_jnls.intcdname)] =0;
	pep_jnls.billmsg[getstrlen(pep_jnls.billmsg)] =0;
	pep_jnls.samid[getstrlen(pep_jnls.samid)] =0;
	pep_jnls.modecde[getstrlen(pep_jnls.modecde)] =0;
	
	#ifdef __LOG__
		dcs_log(0, 0, "cust pep_jnls.pepdate=[%s]", pep_jnls.pepdate);
		dcs_log(0, 0, "cust pep_jnls.peptime=[%s]", pep_jnls.peptime);
		dcs_log(0, 0, "cust pep_jnls.posmercode=[%s]", pep_jnls.posmercode);
		dcs_log(0, 0, "cust pep_jnls.postermcde=[%s]", pep_jnls.postermcde);
		dcs_log(0, 0, "cust pep_jnls.trancde=[%s]", pep_jnls.trancde);
		dcs_log(0, 0, "cust pep_jnls.msgtype=[%s]", pep_jnls.msgtype);
//		dcs_log(0, 0, "cust pep_jnls.outcdno=[%s]", pep_jnls.outcdno);
//		dcs_log(0, 0, "cust pep_jnls.intcdno=[%s]", pep_jnls.intcdno);
		dcs_log(0, 0, "cust pep_jnls.intcdbank=[%s]", pep_jnls.intcdbank);
		dcs_log(0, 0, "cust pep_jnls.intcdname=[%s]", pep_jnls.intcdname);
		dcs_log(0, 0, "cust pep_jnls.process=[%s]", pep_jnls.process);
		dcs_log(0, 0, "cust pep_jnls.tranamt=[%s]", pep_jnls.tranamt);
		dcs_log(0, 0, "cust pep_jnls.trace=[%d]", pep_jnls.trace);
		dcs_log(0, 0, "cust pep_jnls.termtrc=[%s]", pep_jnls.termtrc);
		dcs_log(0, 0, "cust pep_jnls.trntime=[%s]", pep_jnls.trntime);
		dcs_log(0, 0, "cust pep_jnls.trndate=[%s]", pep_jnls.trndate);
		dcs_log(0, 0, "cust pep_jnls.poitcde=[%s]", pep_jnls.poitcde);
		dcs_log(0, 0, "cust i_sendcde=[%s]", i_sendcde);
		dcs_log(0, 0, "cust pep_jnls.termcde=[%s]", pep_jnls.termcde);
		dcs_log(0, 0, "cust pep_jnls.mercode=[%s]", pep_jnls.mercode);
		dcs_log(0, 0, "cust pep_jnls.billmsg=[%s]", pep_jnls.billmsg);
		dcs_log(0, 0, "cust pep_jnls.samid=[%s]", pep_jnls.samid);
		dcs_log(0, 0, "cust pep_jnls.sdrespn=[%s]", pep_jnls.sdrespn);
		dcs_log(0, 0, "cust pep_jnls.revodid=[%s]", pep_jnls.revodid);
		dcs_log(0, 0, "cust pep_jnls.billflg=[%s]", pep_jnls.billflg);
		dcs_log(0, 0, "cust pep_jnls.mmchgid=[%s]", pep_jnls.mmchgid);
		dcs_log(0, 0, "cust pep_jnls.mmchgnm=[%s]", pep_jnls.mmchgnm);
		dcs_log(0, 0, "cust pep_jnls.trnsndp=[%s]", pep_jnls.trnsndp);
		dcs_log(0, 0, "cust pep_jnls.trnmsgd=[%s]", pep_jnls.trnmsgd);
		dcs_log(0, 0, "cust pep_jnls.revdate=[%s]", pep_jnls.revdate);
		dcs_log(0, 0, "cust pep_jnls.modeflg=[%s]", pep_jnls.modeflg);
		dcs_log(0, 0, "cust pep_jnls.modecde=[%s]", pep_jnls.modecde);
		dcs_log(0, 0, "cust pep_jnls.filed28=[%s]", pep_jnls.filed28);
		dcs_log(0, 0, "cust pep_jnls.filed48=[%s]", pep_jnls.filed48);
		dcs_log(0, 0, "cust pep_jnls.aprespn=[%s]", pep_jnls.aprespn);
		dcs_log(0, 0, "cust pep_jnls.merid=[%d]", pep_jnls.merid);
		dcs_log(0, 0, "cust pep_jnls.translaunchway=[%s]", pep_jnls.translaunchway);
		dcs_log(0, 0, "cust pep_jnls.camtlimit=[%d]", pep_jnls.camtlimit);
		dcs_log(0, 0, "cust pep_jnls.damtlimit=[%d]", pep_jnls.damtlimit);
		dcs_log(0, 0, "cust pep_jnls.outcdtype=[%c]", pep_jnls.outcdtype);
		dcs_log(0, 0, "cust pep_jnls.trnsndpid=[%d]", pep_jnls.trnsndpid);
		dcs_log(0, 0, "cust pep_jnls.termmeramtlimit=[%d]", pep_jnls.termmeramtlimit);
	#endif
	
	sprintf(sql, "INSERT INTO pep_jnls (PEPDATE, PEPTIME,trancde,msgtype, OUTCDNO, INTCDNO,\
	 INTCDBANK, INTCDNAME, PROCESS, TRANAMT, TRACE, TERMTRC, TRNTIME, TRNDATE, POITCDE, SENDCDE, TERMCDE, MERCODE, BILLMSG, SAMID, TERMID, \
	TERMIDM, SDRESPN, REVODID, BILLFLG, MMCHGID, MMCHGNM, APRESPN, FILED48, POSMERCODE, POSTERMCDE, TRNSNDP, TRNMSGD, REVDATE,\
	MODEFLG, MODECDE, FILED28, MERID, DISCOUNTAMT, POSCODE, TRANSLAUNCHWAY, CAMTLIMIT, DAMTLIMIT, OUTCDTYPE, TRNSNDPID, \
	TERMMERAMTLIMIT) VALUES('%s','%s','%s','%s',HEX(AES_ENCRYPT('%s','abcdefgh')),HEX(AES_ENCRYPT('%s','abcdefgh')),'%s',HEX(AES_ENCRYPT('%s','abcdefgh')),'%s','%s',%ld,'%s','%s','%s','%s','%s','%s','%s','%s', '%s','%s','%s','%s','%s','%s','%s',%d,'%s','%s',\
	'%s','%s','%s','%s','%s','%s','%s','%s',%d,'%s','%s','%s',%d,%d,'%c',%d,%d)",\
		pep_jnls.pepdate, pep_jnls.peptime,pep_jnls.trancde, pep_jnls.msgtype, pep_jnls.outcdno, pep_jnls.intcdno,\
		 pep_jnls.intcdbank, pep_jnls.intcdname, pep_jnls.process, pep_jnls.tranamt,\
		pep_jnls.trace, pep_jnls.termtrc, pep_jnls.trntime, pep_jnls.trndate, pep_jnls.poitcde,\
		i_sendcde, pep_jnls.termcde, pep_jnls.mercode, pep_jnls.billmsg, pep_jnls.samid, \
		pep_jnls.termid, pep_jnls.termidm, pep_jnls.sdrespn, pep_jnls.revodid, pep_jnls.billflg, pep_jnls.mmchgid, \
		pep_jnls.mmchgnm, pep_jnls.aprespn, pep_jnls.filed48, pep_jnls.posmercode, pep_jnls.postermcde, pep_jnls.trnsndp, \
		pep_jnls.trnmsgd, pep_jnls.revdate, pep_jnls.modeflg, pep_jnls.modecde, \
		pep_jnls.filed28, pep_jnls.merid, pep_jnls.discountamt, pep_jnls.poscode,pep_jnls.translaunchway,\
		pep_jnls.camtlimit, pep_jnls.damtlimit, pep_jnls.outcdtype, pep_jnls.trnsndpid, pep_jnls.termmeramtlimit);
	if(mysql_query(sock,sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "insert pos_iccard_jnls DB error [%s]!", mysql_error(sock));
	        return -1;
    }
	return 1;
}

/*
预授权完成请求 查询终端预授权原笔交易根据38域 61域41,42, ---032
预授权撤销 4域 38域 61域 41,42 ---031
这两种类型的交易可以跨终端执行，所以只要求是同一商户下就可以
*/
int GetPreAuthOriTrans(ISO_data siso, PEP_JNLS *opep_jnls, MER_INFO_T mer_info_t)
{
	PEP_JNLS i_pep_jnls;
	char i_termtrc[6+1];
	char i_aprespn[2+1];
	char i_batchno[6+1];
	char i_outcard[20+1];
	char i_mercode[15+1];
	char o_curdate[8+1];
	char i_authcode[6+1];
	char sql[1024];
	
	int s, n;
	char tmpbuf[127], bit_22[3], transamt[12+1], desamt[12+1];
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(bit_22, 0, sizeof(bit_22));
	memset(&i_pep_jnls, 0, sizeof(PEP_JNLS));
	memset(i_termtrc, 0, sizeof(i_termtrc));
	memset(i_aprespn, 0, sizeof(i_aprespn));
	memset(i_batchno, 0, sizeof(i_batchno));
	memset(i_outcard, 0, sizeof(i_outcard));
	memset(i_mercode, 0, sizeof(i_mercode));
	memset(o_curdate, 0, sizeof(o_curdate));
	memset(i_authcode, 0, sizeof(i_authcode));
	memset(transamt, 0, sizeof(transamt));
	memset(desamt, 0, sizeof(desamt));
	memset(sql, 0, sizeof(sql));
	
	dcs_log(0,0,"预授权完成时原笔预授权交易查询");
	
	memcpy(i_mercode, mer_info_t.mgr_mer_id,15);
	memcpy(i_pep_jnls.trnmsgd, opep_jnls->trnmsgd, 4);
	memcpy(i_pep_jnls.termid, opep_jnls->termid, getstrlen(opep_jnls->termid));
	memcpy(i_pep_jnls.trnsndp, opep_jnls->trnsndp, getstrlen(opep_jnls->trnsndp));
	i_pep_jnls.trnsndpid = opep_jnls->trnsndpid;
	/*中心应答码为00 表示这是一笔预授权成功的交易*/
	memcpy(i_aprespn, "00", 2);
	
	s = getbit(&siso, 61, (unsigned char *)tmpbuf);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 61 error");
		return -1;
	}
	memcpy(i_batchno, tmpbuf, 6);
	memcpy(i_termtrc, tmpbuf+6, 6);
	//模糊查询日期
	memcpy(o_curdate, "%", 1);
	memcpy(o_curdate+1, tmpbuf+12, 4);

	s = getbit(&siso, 38, (unsigned char *)i_authcode);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 38 error");
		return -1;
	}
	
	dcs_log(0, 0, "预授权撤销，取原笔交易金额");
	s = getbit(&siso, 4, (unsigned char *)transamt);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 4 error");
		return -1;
	}
	
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 22, (unsigned char *)tmpbuf);
	if(s > 0)
	{
		memcpy(bit_22, tmpbuf, 2);
		if(memcmp(bit_22, "01", 2)==0)
		{
			if(getbit(&siso, 2, (unsigned char *)i_outcard) < 0)
			{
				dcs_log(0, 0, "取卡号失败");
				return -2;
			}
		}
		else
		{
			memset(tmpbuf, 0, sizeof(tmpbuf));
			s = getbit(&siso, 35, (unsigned char *)tmpbuf);
			if(s > 0)
			{
				for(n=0; n<20; n++)
				{
					if(tmpbuf[n] == '=' )
						break;
				}
				if(n == 20)
				{
					dcs_log(0, 0, "二磁数据错误。");
					return -2;
				}
				else
				{
					memcpy(i_outcard, tmpbuf, n);		        	
//					dcs_log(0, 0, "i_outcard = [%s]", i_outcard);
				}				
			}
			else
			{
#ifdef __LOG__
				dcs_log(0, 0, "二磁不存在");
#endif
				return -2;
			}
		}
	}
	#ifdef __LOG__
		dcs_log(0, 0, "bit_22=[%s]", bit_22);
		dcs_log(0, 0, "i_aprespn=[%s]", i_aprespn);
//		dcs_log(0, 0, "i_outcard =[%s]", i_outcard);
		dcs_log(0, 0, "i_mercode=[%s]", i_mercode);
		dcs_log(0, 0, "i_batchno=[%s]", i_batchno);
		dcs_log(0, 0, "i_termtrc=[%s]", i_termtrc);
		dcs_log(0, 0, "o_curdate=[%s]", o_curdate+1);
		dcs_log(0, 0, "i_authcode=[%s]", i_authcode);
	#endif
	
	sprintf(sql, "SELECT revdate, trim(AES_DECRYPT(UNHEX(outcdno), 'abcdefgh')), syseqno, mercode, tranamt, posmercode, billmsg, authcode\
	FROM pep_jnls t1 where t1.aprespn = '%s'\
		and t1.outcdno = HEX(AES_ENCRYPT('%s','abcdefgh'))\
		and t1.mercode = '%s'\
		and t1.batchno = '%s'\
		and t1.termtrc = '%s'\
		and t1.pepdate like '%s'\
		and t1.authcode = '%s'\
	UNION \
		SELECT revdate, trim(AES_DECRYPT(UNHEX(outcdno), 'abcdefgh')), syseqno, mercode, tranamt, posmercode, billmsg, authcode\
	FROM  pep_jnls_thirty t2 where t2.aprespn = '%s'\
		and t2.outcdno = HEX(AES_ENCRYPT('%s','abcdefgh'))\
		and t2.mercode = '%s'\
		and t2.batchno = '%s'\
		and t2.termtrc = '%s'\
		and t2.pepdate like '%s'\
		and t2.authcode = '%s';", i_aprespn, i_outcard, i_mercode, i_batchno, i_termtrc, o_curdate, i_authcode, \
		i_aprespn, i_outcard, i_mercode, i_batchno, i_termtrc, o_curdate, i_authcode);
	if(mysql_query(sock, sql))
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "select pep_jnls DB error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetPreAuthOriTrans:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
    if(num_rows == 1) {
       memcpy(i_pep_jnls.revdate, row[0] ? row[0] : "\0", (int)sizeof(i_pep_jnls.revdate));
	   memcpy(i_pep_jnls.outcdno, row[1] ? row[1] : "\0", (int)sizeof(i_pep_jnls.outcdno));
	   memcpy(i_pep_jnls.syseqno, row[2] ? row[2] : "\0", (int)sizeof(i_pep_jnls.syseqno));
	   memcpy(i_pep_jnls.mercode, row[3] ? row[3] : "\0", (int)sizeof(i_pep_jnls.mercode));
	   memcpy(i_pep_jnls.tranamt, row[4] ? row[4] : "\0", (int)sizeof(i_pep_jnls.tranamt));
	   memcpy(i_pep_jnls.posmercode, row[5] ? row[5] : "\0", (int)sizeof(i_pep_jnls.posmercode));
       memcpy(i_pep_jnls.billmsg, row[6] ? row[6] : "\0", (int)sizeof(i_pep_jnls.billmsg));
	   memcpy(i_pep_jnls.authcode, row[7] ? row[7] : "\0", (int)sizeof(i_pep_jnls.authcode));
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPreAuthOriTrans:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPreAuthOriTrans:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);	
	#ifdef __LOG__
		dcs_log(0, 0, "i_pep_jnls.revdate=[%s]", i_pep_jnls.revdate);
//		dcs_log(0, 0, "i_pep_jnls.outcdno=[%s]", i_pep_jnls.outcdno);
		dcs_log(0, 0, "i_pep_jnls.syseqno=[%s]", i_pep_jnls.syseqno);
		dcs_log(0, 0, "i_pep_jnls.mercode=[%s]", i_pep_jnls.mercode);
		dcs_log(0, 0, "i_pep_jnls.tranamt=[%s]", i_pep_jnls.tranamt);
		dcs_log(0, 0, "i_pep_jnls.trnsndp=[%s]", i_pep_jnls.trnsndp);
		dcs_log(0, 0, "i_pep_jnls.trnsndpid=[%d]", i_pep_jnls.trnsndpid);
		dcs_log(0, 0, "i_pep_jnls.posmercode=[%s]", i_pep_jnls.posmercode);
		dcs_log(0, 0, "i_pep_jnls.billmsg=[%s]", i_pep_jnls.billmsg);
		dcs_log(0, 0, "i_pep_jnls.authcode=[%s]", i_pep_jnls.authcode);
	#endif
		
	s = strlen(opep_jnls->billmsg);
	if(s!=0&&s!=getstrlen(i_pep_jnls.billmsg))
	{
		dcs_log(0,0,"订单号不一致");
		return -1;
	}
	if(s != 0 && memcmp(opep_jnls->billmsg, i_pep_jnls.billmsg, s)!=0)
	{
		dcs_log(0,0,"订单号不一致");
		return -1;
		
	}
	if(memcmp(opep_jnls->trancde, "031", 3) ==0)
	{
		if(memcmp(i_pep_jnls.tranamt, transamt, 12)!=0)
		{
			dcs_log(0, 0, "预授权撤销金额不符");
			return -1;
		}
	}
	else if(memcmp(opep_jnls->trancde, "032", 3) ==0)
	{	
		memset(tmpbuf, '0', sizeof(tmpbuf));
		tmpbuf[12]= 0;
		dcs_log(0, 0, "预授权完成金额限制");
		multiply("11500", i_pep_jnls.tranamt, tmpbuf);
		memcpy(desamt, tmpbuf, strlen(tmpbuf));
		PubAddSymbolToStr(desamt, 12, '0', 0);
		//sprintf(desamt, "%012s", tmpbuf);
#ifdef __LOG__
		dcs_log(0, 0, "desamt=[%s]", desamt);
		dcs_log(0, 0, "i_pep_jnls.tranamt=[%s]", i_pep_jnls.tranamt);
		dcs_log(0, 0, "transamt=[%s]", transamt);
#endif
		if(memcmp(desamt, transamt, 12) < 0)
		{
			dcs_log(0, 0, "预授权完成金额不能大于预授权金额的115%");
			return -4;
		}
	}
	memcpy(opep_jnls, &i_pep_jnls, sizeof(PEP_JNLS));
	return 0;
}

/*预授权完成撤销冲正处理*/
int GetPreTrans(ISO_data siso, char *currentDate, PEP_JNLS *pep_jnls, char *trace)
{
	char i_ter_trace[6+1];
	char i_batchno[6+1];
	char i_tranamt[12+1];
	char i_date[8+1];
	char o_date[8+1];
	char i_termidm[25+1];
    
    int s, len;
    char tmpbuf[127], tmp_len[3], sql[1024];
    
    struct  tm *tm;   
	time_t  lastTime;
    
    memset(tmpbuf,0,sizeof(tmpbuf));
    memset(i_ter_trace,0,sizeof(i_ter_trace));
    memset(i_batchno,0,sizeof(i_batchno));
    memset(i_tranamt,0,sizeof(i_tranamt));
    memset(i_date,0,sizeof(i_date));
    memset(o_date,0,sizeof(o_date));
    memset(i_termidm, 0, sizeof(i_termidm));
    memset(tmp_len, 0, sizeof(tmp_len));
	memset(sql, 0, sizeof(sql));
    
    dcs_log(0, 0, "预授权完成撤销冲正原笔交易查询。");
    
    /*昨天*/
    lastTime = time(NULL) - 86400;
	tm = localtime(&lastTime);
	sprintf(o_date, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	/*今天*/
    memcpy(i_date, currentDate, 8);
    
   	s = getbit(&siso, 61, (unsigned char *)tmpbuf);
	if(s < 0)
	{
		dcs_log(0,0,"get_bit 61 error");
		return -1;
	}
	memcpy(i_batchno, tmpbuf, 6);
	memcpy(i_ter_trace, tmpbuf+6, 6);
	
	s = getbit(&siso, 4, (unsigned char *)i_tranamt);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 4 error");
		return -1;
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 21, (unsigned char *)tmpbuf);
	if(s < 0)
	{
		dcs_log(0,0,"get_bit 21 error");
		return -1;
	}
	memcpy(tmp_len, tmpbuf, 2);
	sscanf(tmp_len, "%d", &len);
	memcpy(i_termidm, tmpbuf+2, len);
	i_termidm[len] = '\0';
	
	#ifdef __LOG__
    	dcs_log(0, 0, "i_ter_trace=[%s]", i_ter_trace);
   	 	dcs_log(0, 0, "i_batchno=[%s]", i_batchno);
   	 	dcs_log(0, 0, "i_tranamt=[%s]", i_tranamt);  
    	dcs_log(0, 0, "pepdate=[%s]", i_date);
  		dcs_log(0, 0, "pep_jnls->termcde=[%s]", pep_jnls->termcde);
  		dcs_log(0, 0, "pep_jnls->mercode=[%s]", pep_jnls->mercode);
  		dcs_log(0, 0, "i_termidm=[%s]", i_termidm);
  	#endif
    
	sprintf(sql, "SELECT trace, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), outcdtype, billmsg, authcode  FROM pep_jnls \
	WHERE termtrc ='%s'\
	and (pepdate= '%s' or pepdate = '%s') \
	and termcde='%s' and mercode='%s' \
	and batchno='%s' and tranamt='%s' \
	and termidm= '%s' and msgtype in ('0200', '0100');",\
	i_ter_trace, i_date, o_date, pep_jnls->termcde, pep_jnls->mercode, i_batchno, i_tranamt, i_termidm);
	
	if(mysql_query(sock, sql))
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " GetPreTrans elect pep_jnls DB error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetPreTrans:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
    if(num_rows == 1) {
		if(row[0])
			pep_jnls->trace =atol(row[0]);
	   memcpy(pep_jnls->outcdno, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->outcdno));
	   if(row[2])
			pep_jnls->outcdtype = row[2][0];
	   memcpy(pep_jnls->billmsg, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->billmsg));
	   memcpy(pep_jnls->authcode, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls->authcode));
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPreTrans:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPreTrans:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);	
	#ifdef __LOG__
  		dcs_log(0, 0, "pep_jnls->trace=[%ld]", pep_jnls->trace);
//		dcs_log(0, 0, "pep_jnls->outcdno=[%s]", pep_jnls->outcdno);
		dcs_log(0, 0, "pep_jnls->outcdtype=[%c]", pep_jnls->outcdtype); 
		dcs_log(0, 0, "pep_jnls->billmsg=[%s]", pep_jnls->billmsg);  
		dcs_log(0, 0, "pep_jnls->authcode=[%s]", pep_jnls->authcode); 
	#endif
	
	if(memcmp(pep_jnls->trancde, "C33", 3)==0)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));
		s = getbit(&siso, 38, (unsigned char *)tmpbuf);
		if(s>0 && memcmp(tmpbuf, pep_jnls->authcode, 6)!=0)
		{
			dcs_log(0, 0, "预授权完成撤销冲正未找到原笔");
			return -1;
		}
	}
 	sprintf(trace, "%06ld", pep_jnls->trace);
	return 0;
}

/*获取某些特殊商户的新的*/
int GetNewMerInfo(char *mercode, char *termcde, char *termidm)
{
	char i_termcde[8+1];
	char i_mercode[15+1];
	char i_termidm[25+1];
	char sql[512];
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	memset(i_termcde, 0, sizeof(i_termcde));
	memset(i_mercode, 0, sizeof(i_mercode));
	memset(i_termidm, 0, sizeof(i_termidm));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_termidm , termidm, getstrlen(termidm));
	
	sprintf(sql, "SELECT  new_termcde, new_mercode FROM merterm_info where machine_no = '%s';", i_termidm);
	
	if(mysql_query(sock, sql))
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " GetNewMerInfo error [%s]!", mysql_error(sock));
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
        dcs_log(0, 0, "GetNewMerInfo:Couldn't get result from %s\n", mysql_error(sock));
        //从连接池获取的连接需要释放掉
        if(GetIsoMultiThreadFlag())
        	SetFreeLink(sock);
        return -1;
    }
    //从连接池获取的连接需要释放掉
    if(GetIsoMultiThreadFlag())
    	SetFreeLink(sock);
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
    if(num_rows == 1) {
	   memcpy(i_termcde, row[0] ? row[0] : "\0", (int)sizeof(i_termcde));
	   memcpy(i_mercode, row[1] ? row[1] : "\0", (int)sizeof(i_mercode));
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	    dcs_log(0, 0, "GetNewMerInfo:未找到符合条件的记录");
	  #endif
		mysql_free_result(res);
		return 1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetNewMerInfo:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);	
	
	memcpy(mercode, i_mercode, 15);
	memcpy(termcde, i_termcde, 8);
	#ifdef __LOG__
		dcs_log(0, 0, "i_mercode=[%s]", i_mercode);
		dcs_log(0, 0, "i_termcde=[%s]", i_termcde);
		dcs_log(0, 0, "mercode =[%s]", mercode);		
		dcs_log(0, 0, "termcde=[%s]", termcde);
	#endif
	return 0;
}

/*mysql断开重连的处理*/
int GetMysqlLink()
{	
	int i =0;
	for(i = 0; i<7; i++)
	{
		#ifdef __TEST__
			dcs_log(0, 0, "%d ci GetMysqlLink", i);
		#endif
		if(mysql_ping(sock)!=0)
			continue;
		else
			break;
	}
	#ifdef __TEST__
		dcs_log(0, 0, "test GetMysqlLink");
	#endif
	return mysql_ping(sock);
}

/*根据机具编号，获取当前数据表中的软件版本号*/
int getCurVersion(char *termidm, char *cur_version)
{
	char i_termidm[25+1];
	char i_cur_version[5];
	char sql[512];
	
	memset(i_termidm, 0, sizeof(i_termidm));
	memset(i_cur_version, 0, sizeof(i_cur_version));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_termidm , termidm, getstrlen(termidm));
	
	sprintf(sql, "SELECT  cur_version_info FROM pos_conf_info where pos_machine_id = '%s';", i_termidm);
	
	if(mysql_query(sock, sql))
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " getCurVersion error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
    	MYSQL_ROW row ;
    	int num_fields;
	int num_rows = 0;
    
    	if(!(res = mysql_store_result(sock))) {
       	dcs_log(0, 0, "getCurVersion:Couldn't get result from %s\n", mysql_error(sock));
        	return -1;
    	}
    
    	num_fields = mysql_num_fields(res);
    	row = mysql_fetch_row(res);
    	num_rows = mysql_num_rows(res) ;	
    	if(num_rows == 1) {
	   	memcpy(i_cur_version, row[0] ? row[0] : "\0", (int)sizeof(i_cur_version));
    	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "getCurVersion:未找到符合条件的记录");
		mysql_free_result(res);
		return 1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "getCurVersion:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    	mysql_free_result(res);	
	
	memcpy(cur_version, i_cur_version, 4);
	#ifdef __LOG__
		dcs_log(0, 0, "i_cur_version=[%s]", i_cur_version);
		dcs_log(0, 0, "cur_version=[%s]", cur_version);
	#endif
	return 0;
}


/*根据电子签名上送的报文信息，查找到原笔交易*/
int getConfirmated(ISO_data siso, MER_INFO_T mer_info_t, PEP_JNLS *pep_jnls, char *elecsign, char *bit_55, int *bit_55_len)
{
	char i_termtrc[6+1];
	char i_sysrefno[12+1];
	char i_aprespn[2+1];
	char i_batchno[6+1];
	char i_postermcde[8+1];
	char i_posmercode[15+1];
	char i_transamt[12+1];	
	char i_pepdate[8+1];
	char i_peptime[6+1];
	
	int s, len;
	char tmpbuf[2048];
	char sql[1024];
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(i_termtrc, 0, sizeof(i_termtrc));
	memset(i_sysrefno, 0, sizeof(i_sysrefno));
	memset(i_aprespn, 0, sizeof(i_aprespn));
	memset(i_batchno, 0, sizeof(i_batchno));
	memset(i_postermcde, 0, sizeof(i_postermcde));
	memset(i_posmercode, 0, sizeof(i_posmercode));
	memset(i_transamt, 0, sizeof(i_transamt));
	memset(i_pepdate, 0, sizeof(i_pepdate));
	memset(i_peptime, 0, sizeof(i_peptime));
	memset(sql, 0, sizeof(sql));

	dcs_log(0, 0, "电子签名的报文请求");
	
	memcpy(i_posmercode, mer_info_t.mgr_mer_id, 15);
	memcpy(i_postermcde, mer_info_t.mgr_term_id, 8);
	
	/*中心应答码为00 表示这是一笔成功的交易*/
	memcpy(i_aprespn, "00", 2);
	
	s = getbit(&siso, 37, (unsigned char *)i_sysrefno);
	if(s < 0)
	{
		dcs_log(0,0,"get_bit 37 error");
		return -1;
	}
	
	s = getbit(&siso, 60, (unsigned char *)tmpbuf);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 60 error");
		return -1;
	}
	memcpy(i_batchno, tmpbuf+2, 6);
	memcpy(i_termtrc, pep_jnls->termtrc, 6);
	
	s = getbit(&siso, 4, (unsigned char *)i_transamt);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 4 error");
		return -1;
	}
	
	s = getbit(&siso, 62, (unsigned char *)elecsign);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 62 error");
		return -1;
	}
#ifdef __LOG__
	dcs_debug(elecsign, s, "取数据elecsign :");
#endif

	len = s ;
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 55, (unsigned char *)tmpbuf);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 55 error");
		return -1;
	}
#ifdef __LOG__
	dcs_debug(tmpbuf, s, "tmpbuf : s=[%d]", s);
#endif

	*bit_55_len = s;
	memcpy(bit_55, tmpbuf, s);
	analyse55(tmpbuf, s, stFd55, 42);	/* 如果为电子签名，实参4固定填42，如果为IC卡数据，实参4固定填30 */
	
#ifdef __LOG__
	dcs_log(0, 0, "stFd55[6].szValue=[%s]", stFd55[6].szValue);
#endif
	
	/*TODO :get pepdate peptime from filed 55*/
	memcpy(i_pepdate, stFd55[6].szValue, 8);
	memcpy(i_peptime, stFd55[6].szValue+8, 6);
	
	#ifdef __LOG__
		dcs_log(0, 0, "i_sysrefno=[%s]", i_sysrefno);
		dcs_log(0, 0, "i_aprespn=[%s]", i_aprespn);		
		dcs_log(0, 0, "i_posmercode=[%s]", i_posmercode);
		dcs_log(0, 0, "i_postermcde=[%s]", i_postermcde);
		dcs_log(0, 0, "i_batchno=[%s]", i_batchno);
		dcs_log(0, 0, "i_termtrc=[%s]", i_termtrc);
		dcs_log(0, 0, "i_pepdate=[%s]", i_pepdate);
		dcs_log(0, 0, "i_peptime=[%s]", i_peptime);
		dcs_log(0, 0, "i_transamt=[%s]", i_transamt);
	#endif
	
	sprintf(sql, "SELECT  pepdate, peptime, msgtype, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), AES_DECRYPT(UNHEX(intcdno), 'abcdefgh'), process, tranamt, trace, syseqno, sendcde,  termcde, mercode, billmsg,\
	batchno, intcdbank, AES_DECRYPT(UNHEX(intcdname), 'abcdefgh') FROM pep_jnls where syseqno = '%s' and  aprespn ='%s' and  posmercode ='%s' and postermcde ='%s' and tranamt ='%s'\
	and batchno='%s' and  termtrc ='%s' and pepdate ='%s'  and  peptime ='%s';", i_sysrefno, i_aprespn, i_posmercode, i_postermcde, i_transamt, i_batchno,\
	i_termtrc, i_pepdate, i_peptime);
	
	if(mysql_query(sock, sql))
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " getConfirmated error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "getConfirmated:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
    if(num_rows == 1) {
	   memcpy(pep_jnls->pepdate, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls->pepdate));
	   memcpy(pep_jnls->peptime, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->peptime));
	   memcpy(pep_jnls->msgtype, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls->msgtype));
	   memcpy(pep_jnls->outcdno, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->outcdno));
	   memcpy(pep_jnls->intcdno, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls->intcdno));
	   memcpy(pep_jnls->process, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls->process));
	   memcpy(pep_jnls->tranamt, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls->tranamt));
	   if(row[7])
			pep_jnls->trace =atol(row[7]);
	   memcpy(pep_jnls->syseqno, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls->syseqno));
	   memcpy(pep_jnls->sendcde, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls->sendcde));
	   memcpy(pep_jnls->termcde, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls->termcde));
	   memcpy(pep_jnls->mercode, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls->mercode));
	   memcpy(pep_jnls->billmsg, row[12] ? row[12] : "\0", (int)sizeof(pep_jnls->billmsg));
	   memcpy(pep_jnls->batch_no, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls->batch_no));
	   memcpy(pep_jnls->intcdbank, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls->intcdbank));
	   memcpy(pep_jnls->intcdname, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls->intcdname));
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "getConfirmated:pep_jnls未找到符合条件的记录");
		mysql_free_result(res);
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "SELECT  pepdate, peptime, msgtype, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), AES_DECRYPT(UNHEX(intcdno), 'abcdefgh'), process, tranamt, trace, syseqno, sendcde,  termcde, mercode, billmsg,\
		batchno, intcdbank, AES_DECRYPT(UNHEX(intcdname), 'abcdefgh') FROM pep_jnls_all where syseqno = '%s' and  aprespn ='%s' and  posmercode ='%s' and postermcde ='%s' and tranamt ='%s'\
		and batchno='%s' and  termtrc ='%s' and pepdate ='%s'  and  peptime ='%s';", i_sysrefno, i_aprespn, i_posmercode, i_postermcde, i_transamt, i_batchno,\
		i_termtrc, i_pepdate, i_peptime);
		if(mysql_query(sock, sql))
		{
			#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
			dcs_log(0, 0, " getConfirmated pep_jnls_all error [%s]!", mysql_error(sock));
			return -1;
		}
		if(!(res = mysql_store_result(sock)))
		{
			dcs_log(0, 0, "getConfirmated:Couldn't get result from %s\n", mysql_error(sock));
			return -1;
		}
		int num_rows2 = 0;
		num_fields = mysql_num_fields(res);
		row = mysql_fetch_row(res);
		num_rows2 = mysql_num_rows(res) ;	
		if(num_rows2 == 1) 
		{
			memcpy(pep_jnls->pepdate, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls->pepdate));
			memcpy(pep_jnls->peptime, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->peptime));
			memcpy(pep_jnls->msgtype, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls->msgtype));
			memcpy(pep_jnls->outcdno, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->outcdno));
			memcpy(pep_jnls->intcdno, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls->intcdno));
			memcpy(pep_jnls->process, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls->process));
			memcpy(pep_jnls->tranamt, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls->tranamt));
			if(row[7])
				pep_jnls->trace =atol(row[7]);
			memcpy(pep_jnls->syseqno, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls->syseqno));
			memcpy(pep_jnls->sendcde, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls->sendcde));
			memcpy(pep_jnls->termcde, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls->termcde));
			memcpy(pep_jnls->mercode, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls->mercode));
			memcpy(pep_jnls->billmsg, row[12] ? row[12] : "\0", (int)sizeof(pep_jnls->billmsg));
			memcpy(pep_jnls->batch_no, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls->batch_no));
			memcpy(pep_jnls->intcdbank, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls->intcdbank));
			memcpy(pep_jnls->intcdname, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls->intcdname));
		}
		else if(num_rows2 == 0)
		{
			#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
			dcs_log(0, 0, "getConfirmated:pep_jnls_all未找到符合条件的记录");
			mysql_free_result(res);	
			return -1;
		}
		else if(num_rows2 > 1)
		{
			#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
			dcs_log(0, 0, "getConfirmated:pep_jnls_all找到多条符合条件的记录");
			mysql_free_result(res);	
			return -1;
		}
		mysql_free_result(res);	
		return len;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "getConfirmated:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);	
	return len;
}

/*保存电子签名的信息*/
int saveElecSign(PEP_JNLS pep_jnls, char *elecsign, int len, char *bit_55, int bit_55_len)
{
	char i_elecsign[2048];
	char i_bit_55[2048];
	char i_merchant_name[41];
	char i_operatorno[4];
	char i_acquirer[12];
	char i_issuer[12];
	char i_cardvalid[5];
	char i_author[7];
	char i_transtype[26];
	
	int iLen = 0,len_dcm = 0;
	/*fd55 stFd55[43]={{"FF00"},{"FF01"},{"FF02"},{"FF03"},{"FF04"},{"FF05"},{"FF06"},{"FF07"},{"FF08"},{"FF09"},{"FF20"},{"FF21"},{"FF22"},{"FF23"},{"FF24"},{"FF25"},{"FF26"},{"FF27"},{"FF28"},{"FF29"},{"FF2A"},{"FF2B"},
  	               {"FF40"},{"FF41"},{"FF42"},{"FF43"},{"FF44"},{"FF45"},{"FF46"},{"FF47"},{"FF48"},{"FF49"},{"FF4A"},{"FF4B"},{"FF60"},{"FF61"},{"FF62"},{"FF63"},{"FF64"},{"FF80"},{"FF81"},{"FF82"}};
	*/
	char tembuf[127];
	char sql[4096];
	
	memset(i_elecsign, 0, sizeof(i_elecsign));
	memset(i_bit_55, 0, sizeof(i_bit_55));
	memset(i_merchant_name, 0, sizeof(i_merchant_name));
	memset(i_operatorno, 0, sizeof(i_operatorno));
	memset(i_acquirer, 0, sizeof(i_acquirer));
	memset(i_issuer, 0, sizeof(i_issuer));
	memset(i_cardvalid, 0, sizeof(i_cardvalid));
	memset(i_author, 0, sizeof(i_author));
	memset(i_transtype, 0, sizeof(i_transtype));
	memset(tembuf, 0, sizeof(tembuf));
	memset(sql, 0, sizeof(sql));
	
	/*analyse55(bit_55, bit_55_len, stFd55);*/
	
	iLen = strtol(stFd55[0].szLen, NULL, 16);
	memcpy(tembuf, stFd55[0].szValue, 2*iLen);
	asc_to_bcd((unsigned char *)i_merchant_name,(unsigned char *)tembuf, 2*iLen,0);
	
	iLen = strtol(stFd55[2].szLen, NULL, 16);
	memcpy(i_operatorno, stFd55[2].szValue, 2*iLen);
	
	iLen = strtol(stFd55[3].szLen, NULL, 16);
	if(iLen!=0)
	{
		memcpy(tembuf, stFd55[3].szValue, 2*iLen);
		asc_to_bcd((unsigned char *)i_acquirer,(unsigned char *)tembuf, 2*iLen,0);
	}
	
	iLen = strtol(stFd55[4].szLen, NULL, 16);
	if(iLen!=0)
	{
		memcpy(tembuf, stFd55[4].szValue, 2*iLen);
		asc_to_bcd((unsigned char *)i_issuer,(unsigned char *)tembuf, 2*iLen,0);	
	}
	
	iLen = strtol(stFd55[5].szLen, NULL, 16);
	if(iLen!=0)
		memcpy(i_cardvalid, stFd55[5].szValue, 2*iLen);
	
	iLen = strtol(stFd55[7].szLen, NULL, 16);
	if(iLen!=0)
	{
	 	memcpy(tembuf, stFd55[7].szValue, 2*iLen);
	 	asc_to_bcd((unsigned char *)i_author,(unsigned char *)tembuf, 2*iLen,0);	
	 }
	
	iLen = strtol(stFd55[1].szLen, NULL, 16);
	memcpy(tembuf, stFd55[1].szValue, 2*iLen);
	asc_to_bcd((unsigned char *)i_transtype,(unsigned char *)tembuf, 2*iLen,0);
	
	#ifdef __LOG__
		dcs_debug(elecsign, len , "存数据62域elecsign:");
		dcs_debug(bit_55, bit_55_len , "存数据55域bit_55:");
		dcs_log(0,0,"i_merchant_name=[%s]",i_merchant_name);
		dcs_log(0,0,"pep_jnls.postermcde=[%s]",pep_jnls.postermcde);
		dcs_log(0,0,"pep_jnls.posmercode=[%s]",pep_jnls.posmercode);
		dcs_log(0,0,"i_operatorno=[%s]",i_operatorno);
		dcs_log(0,0,"i_acquirer=[%s]",i_acquirer);
		dcs_log(0,0,"i_issuer=[%s]",i_issuer);
//		dcs_log(0,0,"pep_jnls.outcdno=[%s]",pep_jnls.outcdno);
		dcs_log(0,0,"pep_jnls.pepdate=[%s]",pep_jnls.pepdate);
		dcs_log(0,0,"pep_jnls.peptime=[%s]",pep_jnls.peptime);
		dcs_log(0,0,"i_cardvalid=[%s]",i_cardvalid);
		dcs_log(0,0,"pep_jnls.syseqno=[%s]",pep_jnls.syseqno);
		dcs_log(0,0,"pep_jnls.batch_no=[%s]",pep_jnls.batch_no);
		dcs_log(0,0,"pep_jnls.termtrc=[%s]",pep_jnls.termtrc);
		dcs_log(0,0,"i_author=[%s]",i_author);
		dcs_log(0,0,"i_transtype=[%s]",i_transtype);
//		dcs_log(0,0,"pep_jnls.intcdno=[%s]",pep_jnls.intcdno);
		dcs_log(0,0,"pep_jnls.tranamt=[%s]",pep_jnls.tranamt);
		dcs_log(0,0,"pep_jnls.trace=[%ld]",pep_jnls.trace);
		dcs_log(0,0,"pep_jnls.aprespn=[%s]",pep_jnls.aprespn);	
		dcs_log(0,0,"pep_jnls.intcdname=[%s]", pep_jnls.intcdname);
		dcs_log(0,0,"pep_jnls.intcdbank=[%s]", pep_jnls.intcdbank);
	#endif

	len_dcm = mysql_real_escape_string(sock, i_bit_55,bit_55,bit_55_len);
//	dcs_debug(i_bit_55, len_dcm , "存数据i_bit_55:");
	len_dcm= mysql_real_escape_string(sock, i_elecsign,elecsign,len);
#ifdef __LOG__
	dcs_debug(i_elecsign, len_dcm , "存数据62域elecsign:");
#endif

	sprintf(sql, "INSERT INTO elecsignature (MERCHANT_NAME, POSMERCODE, POSTERMCDE, OPERATORNO, ACQUIRER, ISSUER, OUTCDNO, TRNDATE, TRNTIME,\
	CARDVALID, SYSEQNO, BATCHNO, TERMTRC, AUTHOR, TRANSTYPE, INTCDNO, TRANAMT, TRACE, APRESPN, INTCDNAME, INTCDBANK, ELECSIGNATURE, BIT_55)\
	VALUES ('%s', '%s', '%s', '%s', '%s', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%ld', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%s', '%s');",\
	i_merchant_name, pep_jnls.posmercode, pep_jnls.postermcde, i_operatorno, i_acquirer, i_issuer, pep_jnls.outcdno, pep_jnls.pepdate, pep_jnls.peptime,\
	i_cardvalid, pep_jnls.syseqno, pep_jnls.batch_no, pep_jnls.termtrc, i_author, i_transtype, pep_jnls.intcdno, pep_jnls.tranamt, pep_jnls.trace,\
	pep_jnls.aprespn, pep_jnls.intcdname, pep_jnls.intcdbank, i_elecsign, i_bit_55);
  
//	#ifdef __LOG__
//		dcs_log(0,0,"here sucessful 6647");
//	#endif
//
	if(mysql_query(sock, sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "insert elecsignature DB error [%s]!", mysql_error(sock));
	        return -1;
    }
//
//    #ifdef __LOG__
//		dcs_log(0,0,"here sucessful 6657");
//	#endif
	
	return 0;
}

/*T+0 POS 交易回执查询原笔，以及T+0 交易数据的保存*/
int  ConfirmAndsave(ISO_data siso, PEP_JNLS *pep_jnls, char *currdate, char *flag, char *agents_code)
{
	char i_sysrefno[12+1];
	char i_aprespn[2+1];
	char i_batchno[6+1];
	char i_transamt[12+1];	
	char i_pepdate[8+1];
	char i_peptime[6+1];
	char i_agents_code[6+1];
	
	int s, len;
	char tmpbuf[30];
	int i_count = 0;
	char sql[1024];
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(i_sysrefno, 0, sizeof(i_sysrefno));
	memset(i_aprespn, 0, sizeof(i_aprespn));
	memset(i_batchno, 0, sizeof(i_batchno));
	memset(i_transamt, 0, sizeof(i_transamt));
	memset(i_pepdate, 0, sizeof(i_pepdate));
	memset(i_peptime, 0, sizeof(i_peptime));
	memset(i_agents_code, 0, sizeof(i_agents_code));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_pepdate, currdate, 4);
	if(agents_code!=NULL)
		memcpy(i_agents_code, agents_code, getstrlen(agents_code));
	/*中心应答码为00 表示这是一笔成功的交易*/
	memcpy(i_aprespn, "00", 2);
	
	s = getbit(&siso, 37, (unsigned char *)i_sysrefno);
	if(s <= 0)
	{
		dcs_log(0,0,"get_bit 37 error");
		return -1;
	}
	
	s = getbit(&siso, 60, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "get_bit 60 error");
		return -1;
	}
	memcpy(i_batchno, tmpbuf+2, 6);
	
	s = getbit(&siso, 4, (unsigned char *)i_transamt);
	if(s < 0)
	{
		dcs_log(0, 0, "get_bit 4 error");
		return -1;
	}
	
	s = getbit(&siso, 12, (unsigned char *)i_peptime);
	if(s <= 0)
	{
		dcs_log(0, 0, "get_bit 12 error");
		return -1;
	}
	
	s = getbit(&siso, 13, (unsigned char *)i_pepdate+4);
	if(s <= 0)
	{
		dcs_log(0, 0, "get_bit 13 error");
		return -1;
	}
	
	#ifdef __LOG__
		dcs_log(0, 0, "i_sysrefno=[%s]", i_sysrefno);
		dcs_log(0, 0, "i_aprespn=[%s]", i_aprespn);		
		dcs_log(0, 0, "pep_jnls->posmercode=[%s]", pep_jnls->posmercode);
		dcs_log(0, 0, "pep_jnls->postermcde=[%s]", pep_jnls->postermcde);
		dcs_log(0, 0, "i_batchno=[%s]", i_batchno);
		dcs_log(0, 0, "pep_jnls->termtrc=[%s]", pep_jnls->termtrc);
		dcs_log(0, 0, "i_pepdate=[%s]", i_pepdate);
		dcs_log(0, 0, "i_peptime=[%s]", i_peptime);
		dcs_log(0, 0, "i_transamt=[%s]", i_transamt);
	#endif

	sprintf(sql, "SELECT  trace, syseqno, filed44 FROM pep_jnls  where syseqno = '%s' and aprespn = '%s' and posmercode = '%s'\
	and postermcde ='%s' and tranamt = '%s' and batchno = '%s' and termtrc = '%s' and pepdate = '%s' and peptime = '%s';", i_sysrefno, i_aprespn,\
	pep_jnls->posmercode, pep_jnls->postermcde, i_transamt, i_batchno, pep_jnls->termtrc, i_pepdate, i_peptime);
	
	if(mysql_query(sock, sql))
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " ConfirmAndsave select error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "ConfirmAndsave select:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
    if(num_rows == 1) {
	   if(row[0])
		  pep_jnls->trace =atol(row[0]);
	   memcpy(pep_jnls->syseqno, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->syseqno));
	   memcpy(pep_jnls->filed44, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls->filed44));
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "ConfirmAndsave 查询原笔交易信息失败:未找到符合条件的记录");
		mysql_free_result(res);
		return 1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "ConfirmAndsave 查询原笔交易信息失败:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);	
	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls->trace=[%ld]", pep_jnls->trace);
		dcs_log(0, 0, "pep_jnls->syseqno=[%s]", pep_jnls->syseqno);		
		dcs_log(0, 0, "pep_jnls->filed44=[%s]", pep_jnls->filed44);
	#endif
	if(memcmp(flag, "1", 1)!=0 )/*不是T+0商户,不用保存数据到t0_pos_jnls*/
	{
		dcs_log(0, 0, "不是T+0商户,不用保存数据到t0_pos_jnls");
		return 0;
	}
	
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "SELECT count(*) from t0_pos_jnls where syseqno = '%s' and aprespn = '%s' and posmercode = '%s'\
	and postermcde ='%s' and tranamt = '%s' and batchno = '%s' and termtrc = '%s' and pepdate = '%s' and peptime = '%s';", i_sysrefno, i_aprespn,\
	pep_jnls->posmercode, pep_jnls->postermcde, i_transamt, i_batchno, pep_jnls->termtrc, i_pepdate, i_peptime);
	
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "ConfirmAndsave  select count error=[%s]!", mysql_error(sock));
        return -1;
    }
	
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "查询 t0_pos_jnls  error  %s\n", mysql_error(sock));
        return -1;
    }
    row = mysql_fetch_row(res);
	if(row[0])
		i_count = atoi(row[0]);
	mysql_free_result(res);
	
	if(i_count > 0)
	{
		dcs_log(0, 0, "T+0 数据已经保存,无需再保存");
   		return 0;
	}
	
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "INSERT INTO t0_pos_jnls SELECT * from pep_jnls where syseqno = '%s' and aprespn = '%s' and posmercode = '%s'\
	and postermcde ='%s' and tranamt = '%s' and batchno = '%s' and termtrc = '%s' and pepdate = '%s' and peptime = '%s';", i_sysrefno, i_aprespn,\
	pep_jnls->posmercode, pep_jnls->postermcde, i_transamt, i_batchno, pep_jnls->termtrc, i_pepdate, i_peptime);
	
	if(mysql_query(sock, sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "ConfirmAndsave 保存T+0交易信息失败  error [%s]!", mysql_error(sock));
	        return -1;
    }
	
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "UPDATE t0_pos_jnls set agents_code ='%s' where syseqno = '%s' and aprespn = '%s' and posmercode = '%s' \
	and postermcde ='%s' and tranamt = '%s' and batchno = '%s' and termtrc = '%s' and pepdate = '%s' and peptime = '%s';", i_agents_code,\
	i_sysrefno,i_aprespn, pep_jnls->posmercode, pep_jnls->postermcde, i_transamt, i_batchno, pep_jnls->termtrc, i_pepdate, i_peptime);
	
	if(mysql_query(sock, sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "ConfirmAndsave 保存代理商编号  error [%s]!", mysql_error(sock));
	        return -1;
    }
	return 0;
}

/*末笔交易信息查询，查询末笔消费交易的状态*/
int GetJnlsStatus(ISO_data siso, PEP_JNLS *pep_jnls, char *currentDate)
{
	char i_batchno[6+1];
	char i_transamt[12+1];	
	char i_pepdate[8+1];
	char o_pepdate[8+1];
	char i_termtrc[6+1];
	
	int s, len;
	char tmpbuf[30];
	char sql[1024];
	
	struct  tm *tm;   
	time_t  lastTime;
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(i_batchno, 0, sizeof(i_batchno));
	memset(i_transamt, 0, sizeof(i_transamt));
	memset(i_pepdate, 0, sizeof(i_pepdate));
	memset(o_pepdate, 0, sizeof(o_pepdate));
	memset(i_termtrc, 0, sizeof(i_termtrc)); 
	memset(sql, 0, sizeof(sql)); 

     /*昨天*/
    lastTime = time(NULL) - 86400;
	tm = localtime(&lastTime);
	sprintf(o_pepdate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	/*今天*/
   	memcpy(i_pepdate, currentDate, 8);
	
	dcs_log(0, 0, "末笔交易信息查询");
	
	s = getbit(&siso, 61, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "get_bit 60 error");
		return -1;
	}
	memcpy(i_batchno, tmpbuf, 6);
	memcpy(i_termtrc, tmpbuf+6, 6);
	
	s = getbit(&siso, 4, (unsigned char *)i_transamt);
	if(s <= 0)
	{
		dcs_log(0, 0, "get_bit 4 error");
		return -1;
	}
	
	#ifdef __LOG__	
		dcs_log(0, 0, "pep_jnls->posmercode=[%s]", pep_jnls->posmercode);
		dcs_log(0, 0, "pep_jnls->postermcde=[%s]", pep_jnls->postermcde);
		dcs_log(0, 0, "i_batchno=[%s]", i_batchno);
		dcs_log(0, 0, "i_termtrc=[%s]", i_termtrc);
		dcs_log(0, 0, "i_pepdate=[%s]", i_pepdate);
		dcs_log(0, 0, "o_pepdate=[%s]", o_pepdate);
		dcs_log(0, 0, "i_transamt=[%s]", i_transamt);
	#endif

	sprintf(sql, "SELECT  pepdate, peptime, syseqno, aprespn, filed44, trace  FROM pep_jnls  where posmercode = '%s' and postermcde = '%s'  and tranamt = '%s'\
	and batchno = '%s' and termtrc = '%s' and (pepdate = '%s' or pepdate = '%s');", pep_jnls->posmercode, pep_jnls->postermcde, i_transamt, i_batchno,\
	i_termtrc, i_pepdate, o_pepdate);
	if(mysql_query(sock, sql))
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " GetJnlsStatus error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
    	int num_fields;
	int num_rows = 0;
    
	if(!(res = mysql_store_result(sock))) {
	    dcs_log(0, 0, "GetJnlsStatus:Couldn't get result from %s\n", mysql_error(sock));
	    return -1;
	}

	num_fields = mysql_num_fields(res);
	row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res) ;	
	if(num_rows == 1) {
	   memcpy(pep_jnls->pepdate, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls->pepdate));
	   memcpy(pep_jnls->peptime, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->peptime));
	   memcpy(pep_jnls->syseqno, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls->syseqno));
	   memcpy(pep_jnls->aprespn, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->aprespn));
	   memcpy(pep_jnls->filed44, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls->filed44));
	   if(row[5])
			pep_jnls->trace = atol(row[5]);
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetJnlsStatus:未找到符合条件的记录");
		mysql_free_result(res);
		return 1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetJnlsStatus:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    	mysql_free_result(res);		
	#ifdef __LOG__	
		dcs_log(0, 0, "pep_jnls->pepdate=[%s]", pep_jnls->pepdate);
		dcs_log(0, 0, "pep_jnls->peptime=[%s]", pep_jnls->peptime);
		dcs_log(0, 0, "pep_jnls->syseqno=[%s]", pep_jnls->syseqno);
		dcs_log(0, 0, "pep_jnls->aprespn=[%s]", pep_jnls->aprespn);
		dcs_log(0, 0, "pep_jnls->filed44=[%s]", pep_jnls->filed44);
		dcs_log(0, 0, "pep_jnls->trace=[%ld]", pep_jnls->trace);
	#endif
	if(getstrlen(pep_jnls->aprespn)==0)
	{
		sprintf(pep_jnls->syseqno, "%06ld%s", pep_jnls->trace, pep_jnls->pepdate);
		sprintf(pep_jnls->filed44, "%022d", 0);
		memcpy(pep_jnls->aprespn, "96", 2);
	}
	return 0;
}

/*
	输入参数：username
	输出参数：samcard
	根据EWP上送的用户名查询pos_cust_info获取PSAM卡号
	并把机具编号存放到pos_cust_info数据表中
	获取PSAM卡号以及更新pos_cust_info
*/
int GetPsamUpdate(char *username, char *termidm, char *samcard)
{
	char i_cust_id[20+1];
	char i_samcard[16+1];
	char i_termidm[20+1];
	char sql[512];
	
	memset(i_cust_id, 0, sizeof(i_cust_id));
	memset(i_samcard, 0, sizeof(i_samcard));
	memset(i_termidm, 0, sizeof(i_termidm));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_cust_id, username, getstrlen(username));
	memcpy(i_termidm, termidm, getstrlen(termidm));
	
	#ifdef __TEST__	
		dcs_log(0, 0, "username=[%s]", username);
		dcs_log(0, 0, "termidm=[%s]", termidm);
		dcs_log(0, 0, "i_cust_id=[%s]", i_cust_id);
		dcs_log(0, 0, "i_termidm=[%s]", i_termidm);
	#endif

	sprintf(sql, "SELECT  cust_psam FROM pos_cust_info where cust_id = '%s';", i_cust_id);
	if(mysql_query(sock, sql))
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " GetPsamUpdate select error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetPsamUpdate:Couldn't get samid from %s\n", mysql_error(sock));
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
    if(num_rows == 1) {
	   memcpy(i_samcard, row[0] ? row[0] : "\0", (int)sizeof(i_samcard));
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPsamUpdate:未找到符合条件的记录");
		mysql_free_result(res);
		return 1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPsamUpdate:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);	
	
	memcpy(samcard, i_samcard, getstrlen(i_samcard));
	#ifdef __TEST__	
		dcs_log(0, 0, "i_samcard=[%s]", i_samcard);
		dcs_log(0, 0, "samcard=[%s]", samcard);
	#endif
	
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "UPDATE pos_cust_info set terminal_code ='%s' where  cust_id = '%s';", i_termidm, i_cust_id);
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetPsamUpdate update  error [%s]!", mysql_error(sock));
        return -1;
    }
 	return 0;
}

/*
输入参数：samid
输出参数：pik_index，mak_index，tdk_index
函数功能：根据samid从samcard_info中获取密钥索引
*/
int GetKeyindex(char *samid, int *pik_index, int *mak_index, int *tdk_index, int *zmk_index)
{
	char i_samcard[16+1];
	int  i_pik_idx;
	int  i_mak_idx;
	int  i_tdk_idx;
	int  i_zmk_idx;
	char sql[512];
	
	memset(i_samcard, 0, sizeof(i_samcard));
	memset(sql, 0, sizeof(sql));
	
	memcpy(i_samcard, samid, getstrlen(samid));
	
	#ifdef __TEST__	
		dcs_log(0, 0, "samid=[%s]", samid);
		dcs_log(0, 0, "i_samcard=[%s]", i_samcard);
	#endif
	sprintf(sql, "SELECT   sam_pinkey_idx, sam_mackey_idx, sam_tdkey_idx, sam_zmk_index FROM samcard_info where sam_card = '%s';",i_samcard);
	if(mysql_query(sock, sql))
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " GetKeyindex select error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
    	int num_fields;
	int num_rows = 0;
    
    	if(!(res = mysql_store_result(sock))) {
        	dcs_log(0, 0, "GetKeyindex:Couldn't get samid from %s\n", mysql_error(sock));
       	 return -1;
   	 }
    
   	num_fields = mysql_num_fields(res);
    	row = mysql_fetch_row(res);
    	num_rows = mysql_num_rows(res) ;	
    	if(num_rows == 1) {
		if(row[0])
			i_pik_idx =atoi(row[0]);
		if(row[1])
			i_mak_idx =atoi(row[1]);
		if(row[2])
			i_tdk_idx =atoi(row[2]);
		if(row[3])
			i_zmk_idx =atoi(row[3]);
    	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetKeyindex:未找到符合条件的记录");
		mysql_free_result(res);
		return 1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPsamUpdate:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    	mysql_free_result(res);	
	
	*pik_index = i_pik_idx;
	*mak_index = i_mak_idx;
	*tdk_index = i_tdk_idx;
	*zmk_index = i_zmk_idx;
	#ifdef __TEST__	
		dcs_log(0, 0, "i_pik_idx=[%d]", i_pik_idx);
		dcs_log(0, 0, "i_mak_idx=[%d]", i_mak_idx);
		dcs_log(0, 0, "i_tdk_idx=[%d]", i_tdk_idx);
		dcs_log(0, 0, "i_zmk_idx=[%d]", i_zmk_idx);
	#endif
	return 0;
}

/*
函数名：WriteIcinfo
函数功能：保存POS终端上送IC卡的交易信息到数据表pep_icinfo中
输入参数：rcvDate.终端上送交易时间
返回值：-1表示操作数据表pep_icinfo失败
		0表示操作数据表pep_icinfo成功
*/
int WriteIcinfo(ISO_data siso, PEP_JNLS pep_jnls, char *rcvDate)
{
	char tmpbuf[200], t_year[5], szTmp[512+1];
	int	s;

	PEP_ICINFO pep_icinfo;
	char sql[1024];
	
	struct  tm *tm;   
	time_t  t;	
	time(&t);
	tm = localtime(&t);

	char i_intcdname[30+1];

	memset(i_intcdname, 0, sizeof(i_intcdname));
	memset(&pep_icinfo, 0, sizeof(PEP_ICINFO));
	memset(szTmp, 0, sizeof(szTmp));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(t_year, 0, sizeof(t_year));
	memset(sql,0x00,sizeof(sql));
	
	sprintf(t_year,"%04d", tm->tm_year+1900);

	if(getbit(&siso, 3, (unsigned char *)pep_jnls.process)<0)
	{
		 dcs_log(0, 0, "can not get bit_3!");	
		 return -1;
	}
	dcs_log(0, 0, "pep_jnls.process[%s]", pep_jnls.process);

	if (getbit(&siso, 7, (unsigned char *)tmpbuf)<0)
	{
		 dcs_log(0,0,"can not get bit_7!");	
		 return -1;
	}
	sprintf(pep_icinfo.pepdate, "%4.4s%4.4s", t_year, tmpbuf);
	memcpy(pep_icinfo.peptime, tmpbuf+4, 6);	

	getbit(&siso, 2, (unsigned char *)pep_icinfo.outcdno);

	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(getbit(&siso,11,(unsigned char *)tmpbuf)<0)
	{
		 dcs_log(0, 0, "can not get bit_11!");	
		 return -1;
	}
	sscanf(tmpbuf, "%06ld", &pep_icinfo.trace);

	if(memcmp(pep_jnls.process, "310000", 6) == 0 || memcmp(pep_jnls.process, "190000", 6) == 0 
	||memcmp(pep_jnls.process, "480000", 6) == 0 || memcmp(pep_jnls.trancde, "D01", 3)==0
	|| memcmp(pep_jnls.trancde, "D02", 3)==0 ||memcmp(pep_jnls.trancde, "D03", 3)==0
	|| memcmp(pep_jnls.trancde, "D04", 3)==0||memcmp(pep_jnls.trancde, "04", 2)==0)
	{	 
		if(getbit(&siso, 41, (unsigned char *)pep_icinfo.termcde)<0)
		{
		 	dcs_log(0,0,"bit_41 is null!");
		 	return -1;	
		}

		if(getbit(&siso, 42, (unsigned char *)pep_icinfo.mercode)<0)
		{
		 	dcs_log(0, 0, "bit_42 is null!");
			 return -1;	
		}
	}
	else
	{
		memcpy(pep_icinfo.termcde, pep_jnls.termcde, sizeof(pep_jnls.termcde));
		memcpy(pep_icinfo.mercode, pep_jnls.mercode, sizeof(pep_jnls.mercode));
	}

	memcpy(pep_icinfo.trancde, pep_jnls.trancde, sizeof(pep_jnls.trancde));
	s = getbit(&siso, 23, (unsigned char *)pep_icinfo.card_sn);
	if(s <= 0)
		 dcs_log(0, 0, "bit23 is null!");	
		 
	s = getbit(&siso, 55,(unsigned char *)pep_icinfo.send_bit55);
	if(s <= 0)
		 dcs_log(0, 0, "bit55 is null!");	
	else
	{
		bcd_to_asc((unsigned char *)szTmp, (unsigned char *)pep_icinfo.send_bit55, 2*s, 0);
	}

	memset(pep_icinfo.samid, 0, sizeof(pep_icinfo.samid));
	if( strlen(pep_jnls.samid) > 0 )
	{
		memcpy(pep_icinfo.samid, pep_jnls.samid, sizeof(pep_jnls.samid));
	}

#ifdef __LOG__
	dcs_log(0, 0, "pep_icinfo.pepdate=[%s]", pep_icinfo.pepdate);
	dcs_log(0, 0, "pep_icinfo.peptime=[%s]", pep_icinfo.peptime);
	dcs_log(0, 0, "pep_icinfo.trancde=[%s]", pep_icinfo.trancde);
//	dcs_log(0, 0, "pep_icinfo.outcdno=[%s]", pep_icinfo.outcdno);
	dcs_log(0, 0, "pep_icinfo.trace=[%ld]", pep_icinfo.trace);
	dcs_log(0, 0, "pep_icinfo.termcde=[%s]", pep_icinfo.termcde);
	dcs_log(0, 0, "pep_icinfo.mercode=[%s]", pep_icinfo.mercode);
	dcs_log(0, 0, "pep_icinfo.samid=[%s]", pep_icinfo.samid);
	dcs_log(0, 0, "pep_icinfo.card_sn=[%s]", pep_icinfo.card_sn);
	dcs_log(0, 0, "pep_icinfo.send_bit55=[%s]", szTmp);
#endif
	sprintf(sql,"INSERT INTO pep_icinfo(pepdate, peptime, trancde, trace, outcdno, termcde, mercode, samid, card_sn, send_bit55) VALUES('%s', '%s', '%s', %ld, HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%s', '%s',  '%s', '%s')",
			  pep_icinfo.pepdate, pep_icinfo.peptime, pep_icinfo.trancde, pep_icinfo.trace, pep_icinfo.outcdno,
		 		  pep_icinfo.termcde, pep_icinfo.mercode, pep_icinfo.samid, pep_icinfo.card_sn, szTmp );

	if(mysql_query(sock, sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "INSERT INTO pep_icinfo error [%s]!", mysql_error(sock));
		return -1;
	}
	
	return 0;
}


/*
函数名：GetIcinfo
函数功能：查询IC卡返回55域内容，返回终端
返回值：-1表示操作数据表pep_icinfo失败
		0表示操作数据表pep_icinfo成功
*/
int GetIcinfo( PEP_JNLS pep_jnls, PEP_ICINFO *pep_icinfo )
{
	char sql[1024];
    	memset(pep_icinfo->recv_bit55 , 0x00, sizeof(pep_icinfo->recv_bit55 ));
	memset(sql,0x00,sizeof(sql));
#ifdef __LOG__
	dcs_log( 0, 0, "pep_jnls.pepdate[%s]",pep_jnls.pepdate );
	dcs_log( 0, 0, "pep_jnls.peptime[%s]",pep_jnls.peptime );
	dcs_log( 0, 0, "pep_jnls.trace[%ld]" ,pep_jnls.trace );
#endif

	sprintf(sql,"SELECT recv_bit55 from pep_icinfo WHERE pepdate='%s' and peptime='%s' and trace=%d", pep_jnls.pepdate, pep_jnls.peptime, pep_jnls.trace);

	if(mysql_query(sock, sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " SELECT pep_icinfo error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
    	int num_fields;
	int num_rows = 0;
    
    	if(!(res = mysql_store_result(sock))) {
        	dcs_log(0, 0, "GetIcinfo:Couldn't get result from %s\n", mysql_error(sock));
        	return -1;
    	}
    
    	row = mysql_fetch_row(res);
    	num_rows = mysql_num_rows(res) ;	
   	if(num_rows == 1) {
		memcpy(pep_icinfo->recv_bit55, row[0] ? row[0] : "\0", (int)sizeof(pep_icinfo->recv_bit55));
    	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetIcinfo:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetIcinfo:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    	mysql_free_result(res);	
	
	return 0;
}

int GetOriTransRtnIC(ISO_data siso, PEP_JNLS *opep_jnls)
{
	PEP_JNLS i_pep_jnls;
	char i_termtrc[6+1];
	char i_sysrefno[12+1];
	char i_batchno[6+1];
	char i_date[12+1];
	char i_mercode[15+1];	
	char sql[1024];
	int s;
	char tmpbuf[127];

	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(&i_pep_jnls,0,sizeof(PEP_JNLS));
	memset(i_termtrc,0,sizeof(i_termtrc));
	memset(i_sysrefno,0,sizeof(i_sysrefno));
	memset(i_batchno,0,sizeof(i_batchno));
	memset(i_date,0,sizeof(i_date));
	memset(i_mercode, 0, sizeof(i_mercode));
	memset(sql,0x00,sizeof(sql));
	dcs_log(0,0,"脚本通知原笔交易查询。");

	memcpy(i_pep_jnls.trnmsgd, opep_jnls->trnmsgd, 4);
	memcpy(i_pep_jnls.termid, opep_jnls->termid, getstrlen(opep_jnls->termid));
	memcpy(i_pep_jnls.termidm, opep_jnls->termidm, getstrlen(opep_jnls->termidm));
	memcpy(i_pep_jnls.trancde, opep_jnls->trancde, sizeof(opep_jnls->trancde));
	memcpy(i_pep_jnls.pepdate, opep_jnls->pepdate, sizeof(opep_jnls->pepdate));
	memcpy(i_pep_jnls.termcde, opep_jnls->termcde, sizeof(opep_jnls->termcde));
	memcpy(i_pep_jnls.process, opep_jnls->process, sizeof(opep_jnls->process));
	memcpy(i_pep_jnls.netmanage_code, opep_jnls->netmanage_code, sizeof(opep_jnls->netmanage_code));
	memcpy(i_pep_jnls.trnsndp, opep_jnls->trnsndp, sizeof(opep_jnls->trnsndp));
	i_pep_jnls.trnsndpid = opep_jnls->trnsndpid;
	s = getbit(&siso, 37, (unsigned char *)i_sysrefno);
	if(s < 0)
	{
		dcs_log(0,0,"get_bit 37 error");
		return -1;
	}
	s = getbit(&siso, 61, (unsigned char *)tmpbuf);
	if(s < 0)
	{
		dcs_log(0,0,"get_bit 61 error");
		return -1;
	}
	else
		dcs_log(0,0,"bit61=[%s],len=%d",tmpbuf,s);

	memcpy(i_batchno, tmpbuf, 6);
	memcpy(i_termtrc, tmpbuf+6, 6);
	memcpy(i_date, tmpbuf+12, 4);
#ifdef __LOG__
	dcs_log(0,0,"i_termtrc=[%s]",i_termtrc);
	dcs_log(0,0,"i_date=[%s]",i_date);
#endif

	memset(tmpbuf, 0, sizeof(tmpbuf));

	s = getbit(&siso, 42, (unsigned char *)i_mercode);
	if(s < 0)
	{
		dcs_log(0,0,"get_bit 42 error");
		return -1;
	}
#ifdef __LOG__
	dcs_log(0,0,"i_sysrefno=[%s]",i_sysrefno);
	dcs_log(0,0,"i_batchno=[%s]",i_batchno);
	dcs_log(0,0,"i_mercode=[%s]",i_mercode);
#endif

	sprintf(sql,"SELECT pepdate, revdate,  trim(AES_DECRYPT(UNHEX(outcdno), 'abcdefgh')), syseqno, mercode, \
	  			tranamt, termtrc, batchno, posmercode, billmsg, authcode, postermcde, termcde, trace, msgtype \
			FROM  pep_jnls t1 \
			where t1.syseqno = '%s' and t1.posmercode = '%s'  and t1.msgtype != '0400' \
	    UNION \
		       SELECT pepdate, revdate,  trim(AES_DECRYPT(UNHEX(outcdno), 'abcdefgh')), syseqno, mercode, \
				tranamt, termtrc, batchno, posmercode, billmsg, authcode, postermcde, termcde, trace, msgtype \
			FROM  pep_jnls_all t2 \
			where t2.syseqno = '%s'  and t2.posmercode = '%s'  and t2.msgtype != '0400';",
			i_sysrefno, i_mercode, i_sysrefno, i_mercode);

	if(mysql_query(sock,sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " GetOriTransRtnIC select error [%s]!", mysql_error(sock));
		return -1;
	}

	MYSQL_RES *res;
	MYSQL_ROW row ;
	int num_rows = 0;

	if(!(res = mysql_store_result(sock))) {
   	 dcs_log(0, 0, "GetOriTransRtnIC:Couldn't get result from %s\n", mysql_error(sock));
   	 return -1;
	}

	row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res) ;	
	if(num_rows == 1)
	{
		memcpy(i_pep_jnls.pepdate, row[0] ? row[0] : "\0", (int)sizeof(i_pep_jnls.pepdate));
		memcpy(i_pep_jnls.revdate, row[1] ? row[1] : "\0", (int)sizeof(i_pep_jnls.revdate));
		memcpy(i_pep_jnls.outcdno, row[2] ? row[2] : "\0", (int)sizeof(i_pep_jnls.outcdno));
		memcpy(i_pep_jnls.syseqno, row[3] ? row[3] : "\0", (int)sizeof(i_pep_jnls.syseqno));
		memcpy(i_pep_jnls.mercode, row[4] ? row[4] : "\0", (int)sizeof(i_pep_jnls.mercode));	
		memcpy(i_pep_jnls.tranamt, row[5] ? row[5] : "\0", (int)sizeof(i_pep_jnls.tranamt));
		memcpy(i_pep_jnls.termtrc, row[6] ? row[6] : "\0", (int)sizeof(i_pep_jnls.termtrc));
		memcpy(i_pep_jnls.batch_no, row[7] ? row[7] : "\0", (int)sizeof(i_pep_jnls.batch_no));
		memcpy(i_pep_jnls.posmercode, row[8] ? row[8] : "\0", (int)sizeof(i_pep_jnls.posmercode));
		memcpy(i_pep_jnls.billmsg, row[9] ? row[9] : "\0", (int)sizeof(i_pep_jnls.billmsg));
		memcpy(i_pep_jnls.authcode, row[10] ? row[10] : "\0", (int)sizeof(i_pep_jnls.authcode));	
		memcpy(i_pep_jnls.postermcde, row[11] ? row[11] : "\0", (int)sizeof(i_pep_jnls.postermcde));
		memcpy(i_pep_jnls.termcde, row[12] ? row[12] : "\0", (int)sizeof(i_pep_jnls.termcde));
		if(row[13])
			i_pep_jnls.trace=atol(row[13]);
		memcpy(i_pep_jnls.msgtype, row[14] ? row[14] : "\0", (int)sizeof(i_pep_jnls.msgtype));
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOriTransRtnIC:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOriTransRtnIC:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);	
	if(memcmp(i_pep_jnls.pepdate+4, i_date, 4)!= 0)
	{
		dcs_log(0,0,"根据日期查找原笔交易失败");
		return -1;
	}

	if(memcmp(i_batchno, "000000", 6) != 0 && memcmp(i_termtrc, "000000", 6) != 0)
	{
		if(memcmp(i_batchno, i_pep_jnls.batch_no, 6) != 0&& memcmp(i_termtrc, i_pep_jnls.termtrc, 6) != 0)
			return -1;	
	}

	memcpy(opep_jnls, &i_pep_jnls, sizeof(PEP_JNLS));
	return 0;
}

int GeticPubkey(char *pubkeybuf)
{
	ICPUBKINFO icpubkinfo_out;
		
	int len=0, NUM=0;
	char pubkeybuf_tmp[2048];
	char sql[1024];
	memset(&icpubkinfo_out, 0, sizeof(ICPUBKINFO));
	memset(pubkeybuf_tmp, 0, sizeof(pubkeybuf_tmp));
	memset(sql,0x00,sizeof(sql));
	
	sprintf(sql, "SELECT rid, capki ,capk_valid FROM icpubk_info");

	if(mysql_query(sock,sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GeticPubkey SELECT icpubk_info err[%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
	int num_rows = 0;
    
    	if(!(res = mysql_store_result(sock))) {
        	dcs_log(0, 0, "GeticPubkey:Couldn't get result from %s\n", mysql_error(sock));
       	 return -1;
   	 }
    
    	
    	num_rows = mysql_num_rows(res) ;
	if(num_rows == 0)//无公钥
	{
		memcpy(pubkeybuf, "00", 2);
	}
	else 
	{
		while(row = mysql_fetch_row(res))
		{
			sprintf(pubkeybuf_tmp+len,"9F0605%s9F2201%sDF0504%s",row[0], row[1], row[2]);
			len = strlen(pubkeybuf_tmp);
			/*当公钥条数达到26条是数据长度988为本域可容纳最大条数*/
			NUM++;
			if(NUM>=26)
				break;
		}
		sprintf(pubkeybuf, "01%s", pubkeybuf_tmp);
	}
	
    	mysql_free_result(res);	

	return 0;
}

int DownLoadPublicKey(char *pubkeybuf, char *v_rid, char *v_capki)
{
	ICPUBKINFO icpubkinfo_out;
	char v_rid_tmp[10+1];
	char v_capki_tmp[2+1];
	
	int len=0,NUM=0;
	char pubkeybuf_tmp[2048];
	char modullen[2+1], exponentlen[2+1], checkvaluelen[2+1], CAPK_VALID_TMP[16+1];
	char sql[1024];
	memset(v_rid_tmp, 0x00, sizeof(v_rid_tmp));
	memset(v_capki_tmp, 0x00, sizeof(v_capki_tmp));
	memcpy(v_rid_tmp, v_rid, strlen(v_rid));
	memcpy(v_capki_tmp, v_capki, strlen(v_capki));
	memset(pubkeybuf_tmp, 0x00, sizeof(pubkeybuf_tmp));
	memset(&icpubkinfo_out, 0x00, sizeof(ICPUBKINFO));
	memset(modullen, 0x00, sizeof(modullen));
	memset(exponentlen, 0x00, sizeof(exponentlen));
	memset(checkvaluelen, 0x00, sizeof(checkvaluelen));
	memset(CAPK_VALID_TMP, 0x00, sizeof(CAPK_VALID_TMP));
	memset(sql,0x00,sizeof(sql));
	sprintf(sql, "SELECT RID, CAPKI, CAPK_VALID , CAPK_HASH_FLAG, CAPK_ARITH_FLAG, CAPK_MODUL, CAPK_EXPONENT, CAPK_CHECKVALUE \
			FROM icpubk_info \
			WHERE RID = '%s' and CAPKI = '%s'",v_rid_tmp, v_capki_tmp);
	if(mysql_query(sock,sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "DownLoadPublicKey SELECT icpubk_info err[%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
	int num_rows = 0;
    	if(!(res = mysql_store_result(sock))) {
        	dcs_log(0, 0, "DownLoadPublicKey:Couldn't get result from %s\n", mysql_error(sock));
       	 return -1;
   	 }
    
    	num_rows = mysql_num_rows(res) ;
	row = mysql_fetch_row(res);
	if(num_rows == 0)//无此公钥
	{
		memcpy(pubkeybuf, "30", 2);
	}
	else if(num_rows == 1)
	{
		memcpy(icpubkinfo_out.RID, row[0] ? row[0] : "\0", (int)sizeof(icpubkinfo_out.RID));
		memcpy(icpubkinfo_out.CAPKI, row[1] ? row[1] : "\0", (int)sizeof(icpubkinfo_out.CAPKI));
		memcpy(icpubkinfo_out.CAPK_VALID, row[2] ? row[2] : "\0", (int)sizeof(icpubkinfo_out.CAPK_VALID));
		memcpy(icpubkinfo_out.CAPK_HASH_FLAG, row[3] ? row[3] : "\0", (int)sizeof(icpubkinfo_out.CAPK_HASH_FLAG));
		memcpy(icpubkinfo_out.CAPK_ARITH_FLAG, row[4] ? row[4] : "\0", (int)sizeof(icpubkinfo_out.CAPK_ARITH_FLAG));
		memcpy(icpubkinfo_out.CAPK_MODUL, row[5] ? row[5] : "\0", (int)sizeof(icpubkinfo_out.CAPK_MODUL));
		memcpy(icpubkinfo_out.CAPK_EXPONENT, row[6] ? row[6] : "\0", (int)sizeof(icpubkinfo_out.CAPK_EXPONENT));
		memcpy(icpubkinfo_out.CAPK_CHECKVALUE, row[7] ? row[7] : "\0", (int)sizeof(icpubkinfo_out.CAPK_CHECKVALUE));

		r_trim(icpubkinfo_out.CAPK_MODUL);
		r_trim(icpubkinfo_out.CAPK_EXPONENT);
		r_trim(icpubkinfo_out.CAPK_CHECKVALUE);
		sprintf(modullen, "%02X", strlen(icpubkinfo_out.CAPK_MODUL)/2);
		sprintf(exponentlen, "%02X", strlen(icpubkinfo_out.CAPK_EXPONENT)/2);
		sprintf(checkvaluelen, "%02X", strlen(icpubkinfo_out.CAPK_CHECKVALUE)/2);

		bcd_to_asc((unsigned char *)CAPK_VALID_TMP,(unsigned char *)icpubkinfo_out.CAPK_VALID,16,0);
		if(strcmp(modullen,"7F")>0)
		{
			sprintf(pubkeybuf_tmp,"9F0605%s9F2201%sDF0508%sDF0601%sDF0701%sDF0281%s%sDF04%s%sDF03%s%s",
				icpubkinfo_out.RID, icpubkinfo_out.CAPKI, CAPK_VALID_TMP, icpubkinfo_out.CAPK_HASH_FLAG,
				icpubkinfo_out.CAPK_ARITH_FLAG, modullen, icpubkinfo_out.CAPK_MODUL, exponentlen, icpubkinfo_out.CAPK_EXPONENT, checkvaluelen, icpubkinfo_out.CAPK_CHECKVALUE);
		}
		else 
		{
			sprintf(pubkeybuf_tmp,"9F0605%s9F2201%sDF0508%sDF0601%sDF0701%sDF02%s%sDF04%s%sDF03%s%s",
				icpubkinfo_out.RID, icpubkinfo_out.CAPKI, CAPK_VALID_TMP, icpubkinfo_out.CAPK_HASH_FLAG,
				icpubkinfo_out.CAPK_ARITH_FLAG, modullen, icpubkinfo_out.CAPK_MODUL, exponentlen, icpubkinfo_out.CAPK_EXPONENT, checkvaluelen, icpubkinfo_out.CAPK_CHECKVALUE);
		}
		sprintf(pubkeybuf, "31%s", pubkeybuf_tmp);
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "DownLoadPublicKey:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    	mysql_free_result(res);	
	
	return 0;
}

int GeticPara(char *pubkeybuf)
{
	ICPARAINFO icparainfo_out;
		
	int len=0,NUM=0;
	char pubkeybuf_tmp[2048],aidlen[2+1],pubkeybuf_temp[2048];
	char sql[1024];

	memset(sql,0x00,sizeof(sql));
	sprintf(sql,"SELECT aid FROM   icpara_info ");

	if(mysql_query(sock, sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " SELECT icpara_info error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
	int num_rows = 0;
    
    	if(!(res = mysql_store_result(sock))) {
        	dcs_log(0, 0, "GeticPara:Couldn't get result from %s\n", mysql_error(sock));
       	 return -1;
   	 }
    
    	
    	num_rows = mysql_num_rows(res) ;
	if(num_rows == 0)//无参数
	{
		memcpy(pubkeybuf_temp, "30", 2);
	}
	else 
	{
		while(row = mysql_fetch_row(res))
		{
			memcpy(icparainfo_out.AID, row[0] ? row[0] : "\0", (int)sizeof(icparainfo_out.AID));
			dcs_log(0,0," NUM==%d",NUM);	
			sprintf(aidlen, "%02X", getstrlen(icparainfo_out.AID)/2);
			sprintf(pubkeybuf_tmp+len,"9F06%s%s", aidlen, icparainfo_out.AID);
			len = getstrlen(pubkeybuf_tmp);
			NUM++;
		}
		sprintf(pubkeybuf_temp, "31%s", pubkeybuf_tmp);
	}
	
    	mysql_free_result(res);	
	
	memcpy(pubkeybuf, pubkeybuf_temp, getstrlen(pubkeybuf_temp));
	return 0;
}

int DownLoadPara(char *pubkeybuf, char *v_aid)
{
	ICPARAINFO icparainfo_out;
	char v_aid_tmp[32+1];
	int len=0,NUM=0;
	char pubkeybuf_tmp[2048];
	char aidlen[2+1], ddollen[2+1];
	char sql[1024];
	memset(v_aid_tmp, 0x00, sizeof(v_aid_tmp));
	memcpy(v_aid_tmp, v_aid, strlen(v_aid));
	memset(sql, 0x00 ,sizeof(sql));

	sprintf(sql, "SELECT RTrim(AID), asi, app_vernum, tac_default, tac_online, threshold, tac_decline, \
						term_minlimit, maxtarget_percent, target_percent, ddol_default, termpincap, \
						vlptranslimit, clessofflinelimitamt, clesstransamt, termcvm_limit \
			FROM icpara_info \
			WHERE aid = '%s'", v_aid_tmp);
			
	if(mysql_query(sock,sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "DownLoadPara SELECT icpara_info err[%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
	int num_rows = 0;
    
    	if(!(res = mysql_store_result(sock))) {
        	dcs_log(0, 0, "DownLoadPara:Couldn't get result from %s\n", mysql_error(sock));
       	 return -1;
   	 }
    
    	num_rows = mysql_num_rows(res) ;
	row = mysql_fetch_row(res);
	if(num_rows == 0)//无此参数
	{
		memcpy(pubkeybuf, "30", 2);
	}
	else if(num_rows == 1)
	{
		memcpy(icparainfo_out.AID, row[0] ? row[0] : "\0", (int)sizeof(icparainfo_out.AID));
		memcpy(icparainfo_out.ASI, row[1] ? row[1] : "\0", (int)sizeof(icparainfo_out.ASI));
		memcpy(icparainfo_out.APP_VERNUM, row[2] ? row[2] : "\0", (int)sizeof(icparainfo_out.APP_VERNUM));
		memcpy(icparainfo_out.TAC_DEFAULT, row[3] ? row[3] : "\0", (int)sizeof(icparainfo_out.TAC_DEFAULT));
		memcpy(icparainfo_out.TAC_ONLINE, row[4] ? row[4] : "\0", (int)sizeof(icparainfo_out.TAC_ONLINE));
		memcpy(icparainfo_out.THRESHOLD, row[5] ? row[5] : "\0", (int)sizeof(icparainfo_out.THRESHOLD));
		memcpy(icparainfo_out.TAC_DECLINE, row[6] ? row[6] : "\0", (int)sizeof(icparainfo_out.TAC_DECLINE));
		memcpy(icparainfo_out.TERM_MINLIMIT, row[7] ? row[7] : "\0", (int)sizeof(icparainfo_out.TERM_MINLIMIT));
		memcpy(icparainfo_out.MAXTARGET_PERCENT, row[8] ? row[8] : "\0", (int)sizeof(icparainfo_out.MAXTARGET_PERCENT));
		memcpy(icparainfo_out.TARGET_PERCENT, row[9] ? row[9] : "\0", (int)sizeof(icparainfo_out.TARGET_PERCENT));
		memcpy(icparainfo_out.DDOL_DEFAULT, row[10] ? row[10] : "\0", (int)sizeof(icparainfo_out.DDOL_DEFAULT));
		memcpy(icparainfo_out.TERMPINCAP, row[11] ? row[11] : "\0", (int)sizeof(icparainfo_out.TERMPINCAP));
		memcpy(icparainfo_out.VLPTRANSLIMIT, row[12] ? row[12] : "\0", (int)sizeof(icparainfo_out.VLPTRANSLIMIT));
		memcpy(icparainfo_out.CLESSOFFLINELIMITAMT, row[13] ? row[13] : "\0", (int)sizeof(icparainfo_out.CLESSOFFLINELIMITAMT));
		memcpy(icparainfo_out.CLESSTRANSAMT, row[14] ? row[14] : "\0", (int)sizeof(icparainfo_out.CLESSTRANSAMT));
		memcpy(icparainfo_out.TERMCVM_LIMIT, row[15] ? row[15] : "\0", (int)sizeof(icparainfo_out.TERMCVM_LIMIT));

		r_trim(icparainfo_out.AID);
		r_trim(icparainfo_out.DDOL_DEFAULT);
		sprintf(aidlen, "%02X", strlen(icparainfo_out.AID)/2);
		sprintf(ddollen, "%02X", strlen(icparainfo_out.DDOL_DEFAULT)/2);
		sprintf(pubkeybuf_tmp,"9F06%s%sDF0101%s9F0902%sDF1105%sDF1205%sDF1504%sDF1305%s9F1B04%sDF1601%sDF1701%sDF14%s%sDF1801%s9F7B06%sDF1906%sDF2006%sDF2106%s",
				aidlen, icparainfo_out.AID, icparainfo_out.ASI, icparainfo_out.APP_VERNUM, icparainfo_out.TAC_DEFAULT, icparainfo_out.TAC_ONLINE, icparainfo_out.THRESHOLD, 
				icparainfo_out.TAC_DECLINE, icparainfo_out.TERM_MINLIMIT, icparainfo_out.MAXTARGET_PERCENT, icparainfo_out.TARGET_PERCENT, ddollen, icparainfo_out.DDOL_DEFAULT,
				icparainfo_out.TERMPINCAP, icparainfo_out.VLPTRANSLIMIT, icparainfo_out.CLESSOFFLINELIMITAMT, icparainfo_out.CLESSTRANSAMT, icparainfo_out.TERMCVM_LIMIT);

		sprintf(pubkeybuf, "31%s", pubkeybuf_tmp);
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "DownLoadPara:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	
	return 0;
}

/*
	保存TC值到manage_info中的55域，以blob的数据类型,需要先查询原笔交易信息查询pep_jnls
*/
int saveTCvalue(ISO_data siso, char *currentDate, PEP_JNLS *pep_jnls)
{
	char bit_55[512], netmanage_code[3+1];
	char batchno[6+1];
	PEP_JNLS pep_jnls_tc;
	
	char tmpbuf[512];
	char sql[1024];
	int s = -1, rtn = 0;
	memset(bit_55, 0, sizeof(bit_55));
	memset(netmanage_code, 0, sizeof(netmanage_code));
	memset(batchno, 0, sizeof(batchno));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(&pep_jnls_tc, 0, sizeof(PEP_JNLS));
	memset(sql, 0x00, sizeof(sql));
	
	memcpy(pep_jnls_tc.pepdate, currentDate, 8);
	memcpy(pep_jnls_tc.peptime, currentDate+8, 6);
	memcpy(pep_jnls_tc.trndate, currentDate, 8);
	memcpy(pep_jnls_tc.trntime, currentDate+8, 6);
	
	s = getbit(&siso, 60, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0,0,"get_bit 60 error");
		return -1;
	}
	memcpy(batchno, tmpbuf+2, 6);
	memcpy(netmanage_code, tmpbuf+8, 3);
		memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 55, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "get_bit 55 error");
		return -1;
	}
#ifdef __LOG__
	dcs_debug(tmpbuf, s, "转换前bit55");
#endif
	
	mysql_real_escape_string(sock, bit_55, tmpbuf, s);
#ifdef __LOG__
	dcs_debug(bit_55, s, "转换后bit55");
#endif

	sprintf(sql, "SELECT  AES_DECRYPT(UNHEX(outcdno),'abcdefgh'), tranamt, trace, termcde, mercode, process, sendcde, billmsg, termidm, authcode, revdate, syseqno \
			from pep_jnls \
			WHERE batchno = '%s' and termtrc = '%s' and postermcde ='%s' and posmercode ='%s'  and msgtype != '0400' \
		UNION \
			SELECT AES_DECRYPT(UNHEX(outcdno),'abcdefgh'), tranamt, trace, termcde, mercode, process, sendcde, billmsg, termidm, authcode, revdate, syseqno \
			from pep_jnls_all \
			WHERE batchno = '%s' and termtrc = '%s' and postermcde ='%s' and posmercode ='%s'and msgtype != '0400' ",
			batchno, pep_jnls->termtrc, pep_jnls->postermcde, pep_jnls->posmercode, batchno, pep_jnls->termtrc, pep_jnls->postermcde, pep_jnls->posmercode);

	if(mysql_query(sock, sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " saveTCvalue:SELECT pep_jnls error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock)))
	{
        dcs_log(0, 0, "saveTCvalue:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
   	if(num_rows == 1) 
	{
		memcpy(pep_jnls_tc.outcdno, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls_tc.outcdno));
		memcpy(pep_jnls_tc.tranamt, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls_tc.tranamt));
		if(row[2])
			pep_jnls_tc.trace=atol(row[2]);	
		memcpy(pep_jnls_tc.termcde, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls_tc.termcde));
		memcpy(pep_jnls_tc.mercode, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls_tc.mercode));
		memcpy(pep_jnls_tc.process, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls_tc.process));
		memcpy(pep_jnls_tc.sendcde, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls_tc.sendcde));
		memcpy(pep_jnls_tc.billmsg, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls_tc.billmsg));
		memcpy(pep_jnls_tc.termidm, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls_tc.termidm));
		memcpy(pep_jnls_tc.authcode, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls_tc.authcode));
		memcpy(pep_jnls_tc.revdate, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls_tc.revdate));
		memcpy(pep_jnls_tc.syseqno, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls_tc.syseqno));
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "saveTCvalue:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "saveTCvalue:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);
	
	/* TC值上送核心时需要重组61域，需要authcoder,revdate,trace,syseqno,此处赋值到pep_jnls中方便后续重组使用*/
	memcpy(pep_jnls->authcode, pep_jnls_tc.authcode, strlen(pep_jnls_tc.authcode));
	memcpy(pep_jnls->revdate, pep_jnls_tc.revdate, strlen(pep_jnls_tc.revdate));
	pep_jnls->trace = pep_jnls_tc.trace;
	memcpy(pep_jnls->syseqno, pep_jnls_tc.syseqno, strlen(pep_jnls_tc.syseqno));
	memcpy(pep_jnls->outcdno, pep_jnls_tc.outcdno, strlen(pep_jnls_tc.outcdno));

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "INSERT INTO manage_info(pepdate, peptime, trancde, outcdno, tranamt, trace, termtrc, sendcde, syseqno, termcde, mercode, billmsg, \
	termidm, aprespn, postermcde, posmercode, field55, card_sn, tc_value, netmanage_code, msgtype, process, batchno, trnsndpid, trnsndp, authcode, trnmsgd) \
	VALUES('%s', '%s', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%d', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%s', '%s', '%s')", \
	pep_jnls_tc.pepdate, pep_jnls_tc.peptime, pep_jnls->trancde, pep_jnls_tc.outcdno, pep_jnls_tc.tranamt, pep_jnls_tc.trace, pep_jnls->termtrc, pep_jnls_tc.sendcde, pep_jnls_tc.syseqno, \
	pep_jnls_tc.termcde, pep_jnls_tc.mercode, pep_jnls_tc.billmsg, pep_jnls_tc.termidm, pep_jnls->aprespn, pep_jnls->postermcde, pep_jnls->posmercode, bit_55, card_sn, \
	pep_jnls->tc_value, netmanage_code, pep_jnls->msgtype, pep_jnls_tc.process, batchno, pep_jnls->trnsndpid, pep_jnls->trnsndp, pep_jnls_tc.authcode, pep_jnls->trnmsgd);
	if(mysql_query(sock,sql)) 
	{
	    #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "insert manage_info DB error [%s]!", mysql_error(sock));
	    return -1;
    }
	return 0;
}

int Updatedownloadflag(ISO_data iso, int flag, char *CurDate)
{
	
	dcs_log(0,0,"flag [%d] ",flag);
	
	char termcde[8+1],mercode[15+1],sql[1024+1];
	
	memset(termcde, 0x00, sizeof(termcde));
	memset(mercode, 0x00, sizeof(mercode));
	memset(sql, 0x00, sizeof(sql));
	
	if ( getbit(&iso,41,(unsigned char *)termcde)<0)
	{
		 dcs_log(0,0,"can not get bit_41!");	
		 return -1;
	}
	
	if ( getbit(&iso,42,(unsigned char *)mercode)<0)
	{
		 dcs_log(0,0,"can not get bit_41!");	
		 return -1;
	}
#ifdef __LOG__
	dcs_log(0,0,"postermcde [%s] ",termcde);
	dcs_log(0,0,"posmercode [%s] ",mercode);
#endif

	memset(sql, 0x00, sizeof(sql));
	if( flag == 1 )
	{
		sprintf(sql, "UPDATE pos_conf_info set download_pubkey_flag = '1',pubkey_update_time='%s' where mgr_term_id='%s' and mgr_mer_id='%s'", CurDate, termcde, mercode);
	}
	else if( flag == 2 )
	{
		sprintf(sql, "UPDATE pos_conf_info set download_para_flag = '1',para_update_time='%s' where mgr_term_id='%s' and mgr_mer_id='%s'", CurDate, termcde, mercode);
	}
	else 
	{
		return -1;
	}
	
	if (mysql_query(sock, sql) != 0)
	{
   		dcs_log(0,0,"update pep_conf_info DB error [%s][%s]! ", sql, mysql_error(sock));
   		return -1;
	}
	
	dcs_log(0,0,"update pep_conf_info DB SUCCESS");
	return 0;
}


int EwpGeticKey(EWP_INFO *ewp_info, char *pubkeybuf)
{
	dcs_log(0, 0, "当前下载公钥序号[%s]", ewp_info->paraseq);

	char modullen[2+1], exponentlen[2+1], checkvaluelen[2+1], CAPK_VALID_TMP[16+1];
	char sql[1024];
	
	memset(CAPK_VALID_TMP,0x00,sizeof(CAPK_VALID_TMP));
	memset(checkvaluelen,0x00,sizeof(checkvaluelen));
	memset(exponentlen,0x00,sizeof(exponentlen));
	memset(modullen,0x00,sizeof(modullen));

	ICPUBKINFO icpubkinfo_out;
	int paraseq=0;


	paraseq = atoi(ewp_info->paraseq) - 1;

	memset(sql,0x00,sizeof(sql));
	sprintf(sql,"select RID, CAPKI, CAPK_VALID , CAPK_HASH_FLAG, CAPK_ARITH_FLAG, CAPK_MODUL, CAPK_EXPONENT, CAPK_CHECKVALUE \
		FROM icpubk_info limit %d,1",paraseq);

	if(mysql_query(sock,sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "EwpGeticKey SELECT icpubk_info err[%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
	int num_rows = 0;
    
    	if(!(res = mysql_store_result(sock))) {
        	dcs_log(0, 0, "EwpGeticKey:Couldn't get result from %s\n", mysql_error(sock));
       	 return -1;
   	 }
    
    	num_rows = mysql_num_rows(res) ;
	row = mysql_fetch_row(res);
	if(num_rows == 0)//无此参数
	{
		dcs_log(0,0,"无数据");
	}
	else if(num_rows == 1)
	{
		memcpy(icpubkinfo_out.RID, row[0] ? row[0] : "\0", (int)sizeof(icpubkinfo_out.RID));
		memcpy(icpubkinfo_out.CAPKI, row[1] ? row[1] : "\0", (int)sizeof(icpubkinfo_out.CAPKI));
		memcpy(icpubkinfo_out.CAPK_VALID, row[2] ? row[2] : "\0", (int)sizeof(icpubkinfo_out.CAPK_VALID));
		memcpy(icpubkinfo_out.CAPK_HASH_FLAG, row[3] ? row[3] : "\0", (int)sizeof(icpubkinfo_out.CAPK_HASH_FLAG));
		memcpy(icpubkinfo_out.CAPK_ARITH_FLAG, row[4] ? row[4] : "\0", (int)sizeof(icpubkinfo_out.CAPK_ARITH_FLAG));
		memcpy(icpubkinfo_out.CAPK_MODUL, row[5] ? row[5] : "\0", (int)sizeof(icpubkinfo_out.CAPK_MODUL));
		memcpy(icpubkinfo_out.CAPK_EXPONENT, row[6] ? row[6] : "\0", (int)sizeof(icpubkinfo_out.CAPK_EXPONENT));
		memcpy(icpubkinfo_out.CAPK_CHECKVALUE, row[7] ? row[7] : "\0", (int)sizeof(icpubkinfo_out.CAPK_CHECKVALUE));

		r_trim(icpubkinfo_out.CAPK_MODUL);
		r_trim(icpubkinfo_out.CAPK_EXPONENT);
		r_trim(icpubkinfo_out.CAPK_CHECKVALUE);
		sprintf(modullen, "%02X", strlen(icpubkinfo_out.CAPK_MODUL)/2);
		sprintf(exponentlen, "%02X", strlen(icpubkinfo_out.CAPK_EXPONENT)/2);
		sprintf(checkvaluelen, "%02X", strlen(icpubkinfo_out.CAPK_CHECKVALUE)/2);
#ifdef __LOG__
		dcs_log(0,0,"[%s][%s][%s]", modullen, exponentlen, checkvaluelen );
#endif
	
		bcd_to_asc((unsigned char *)CAPK_VALID_TMP,(unsigned char *)icpubkinfo_out.CAPK_VALID,16,0);
		if(strcmp(modullen,"7F")>0)
		{
			sprintf(pubkeybuf,"9F0605%s9F2201%sDF0508%sDF0601%sDF0701%sDF0281%s%sDF04%s%sDF03%s%s",
			icpubkinfo_out.RID, icpubkinfo_out.CAPKI, CAPK_VALID_TMP, icpubkinfo_out.CAPK_HASH_FLAG,
			icpubkinfo_out.CAPK_ARITH_FLAG, modullen, icpubkinfo_out.CAPK_MODUL, exponentlen, icpubkinfo_out.CAPK_EXPONENT, checkvaluelen, icpubkinfo_out.CAPK_CHECKVALUE);
		}
		else 
		{
			sprintf(pubkeybuf,"9F0605%s9F2201%sDF0508%sDF0601%sDF0701%sDF02%s%sDF04%s%sDF03%s%s",
			icpubkinfo_out.RID, icpubkinfo_out.CAPKI, CAPK_VALID_TMP, icpubkinfo_out.CAPK_HASH_FLAG,
			icpubkinfo_out.CAPK_ARITH_FLAG, modullen, icpubkinfo_out.CAPK_MODUL, exponentlen, icpubkinfo_out.CAPK_EXPONENT, checkvaluelen, icpubkinfo_out.CAPK_CHECKVALUE);
		}
		
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "EwpGeticKey:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}

#ifdef __LOG__
	dcs_log(0,0,"下发公钥值[%s][%s]", CAPK_VALID_TMP, pubkeybuf);
#endif
	mysql_free_result(res);

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, " select count(*) from icpubk_info");

	if(mysql_query(sock,sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "EwpGeticKey  查询总数据出错[%s]!", mysql_error(sock));
		return -1;
	}
	num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) 
    {
		dcs_log(0, 0, "EwpGeticKey:Couldn't get result from %s\n", mysql_error(sock));
		return -1;
   	 }
    
    num_rows = mysql_num_rows(res) ;
	row = mysql_fetch_row(res);

	ewp_info->total_keypara = atoi(row[0]);
	dcs_log(0,0,"公钥总条数[%d]", ewp_info->total_keypara );
	mysql_free_result(res);
	return 0;	
}

int EwpGeticPara(EWP_INFO *ewp_info, char *pubkeybuf)
{
	dcs_log(0, 0, "当前参数下载序号[%s]", ewp_info->paraseq);
	char sql[1024];
	ICPARAINFO icparainfo_out;
	int paraseq = 0;
	int len=0,NUM=0;
	char aidlen[2+1], ddollen[2+1];
	paraseq = atoi(ewp_info->paraseq) -1;

	memset(sql,0x00,sizeof(sql));
	sprintf(sql, "select RTrim(AID), ASI, TAC_DEFAULT, TAC_ONLINE, THRESHOLD, TAC_DECLINE, \
				TERM_MINLIMIT, MAXTARGET_PERCENT, TARGET_PERCENT, DDOL_DEFAULT, TERMPINCAP, \
				APP_VERNUM, VLPTRANSLIMIT, CLESSOFFLINELIMITAMT,CLESSTRANSAMT,TERMCVM_LIMIT \
		FROM icpara_info limit %d,1",paraseq);

	if(mysql_query(sock,sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "EwpGeticPara SELECT icpara_info err[%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock)))
    {
        dcs_log(0, 0, "EwpGeticPara:Couldn't get result from %s\n", mysql_error(sock));
       	return -1;
   	}
    
    num_rows = mysql_num_rows(res) ;
	row = mysql_fetch_row(res);
	if(num_rows == 0)//无此参数
	{
		dcs_log(0,0,"无数据");
	}
	else if(num_rows == 1)
	{
		memcpy(icparainfo_out.AID, row[0] ? row[0] : "\0", (int)sizeof(icparainfo_out.AID));
		memcpy(icparainfo_out.ASI, row[1] ? row[1] : "\0", (int)sizeof(icparainfo_out.ASI));
		memcpy(icparainfo_out.TAC_DEFAULT, row[2] ? row[2] : "\0", (int)sizeof(icparainfo_out.TAC_DEFAULT));
		memcpy(icparainfo_out.TAC_ONLINE, row[3] ? row[3] : "\0", (int)sizeof(icparainfo_out.TAC_ONLINE));
		memcpy(icparainfo_out.THRESHOLD, row[4] ? row[4] : "\0", (int)sizeof(icparainfo_out.THRESHOLD));
		memcpy(icparainfo_out.TAC_DECLINE, row[5] ? row[5] : "\0", (int)sizeof(icparainfo_out.TAC_DECLINE));
		memcpy(icparainfo_out.TERM_MINLIMIT, row[6] ? row[6] : "\0", (int)sizeof(icparainfo_out.TERM_MINLIMIT));
		memcpy(icparainfo_out.MAXTARGET_PERCENT, row[7] ? row[7] : "\0", (int)sizeof(icparainfo_out.MAXTARGET_PERCENT));
		memcpy(icparainfo_out.TARGET_PERCENT, row[8] ? row[8] : "\0", (int)sizeof(icparainfo_out.TARGET_PERCENT));
		memcpy(icparainfo_out.DDOL_DEFAULT, row[9] ? row[9] : "\0", (int)sizeof(icparainfo_out.DDOL_DEFAULT));
		memcpy(icparainfo_out.TERMPINCAP, row[10] ? row[10] : "\0", (int)sizeof(icparainfo_out.TERMPINCAP));
		memcpy(icparainfo_out.APP_VERNUM, row[11] ? row[11] : "\0", (int)sizeof(icparainfo_out.APP_VERNUM));
		memcpy(icparainfo_out.VLPTRANSLIMIT, row[12] ? row[12] : "\0", (int)sizeof(icparainfo_out.VLPTRANSLIMIT));
		memcpy(icparainfo_out.CLESSOFFLINELIMITAMT, row[13] ? row[13] : "\0", (int)sizeof(icparainfo_out.CLESSOFFLINELIMITAMT));
		memcpy(icparainfo_out.CLESSTRANSAMT, row[14] ? row[14] : "\0", (int)sizeof(icparainfo_out.CLESSTRANSAMT));
		memcpy(icparainfo_out.TERMCVM_LIMIT, row[15] ? row[15] : "\0", (int)sizeof(icparainfo_out.TERMCVM_LIMIT));

		r_trim(icparainfo_out.AID);
		r_trim(icparainfo_out.DDOL_DEFAULT);
		sprintf(aidlen, "%02X", strlen(icparainfo_out.AID)/2);
		sprintf(ddollen, "%02X", strlen(icparainfo_out.DDOL_DEFAULT)/2);
		sprintf(pubkeybuf,"9F06%s%sDF0101%s9F0902%sDF1105%sDF1205%sDF1504%sDF1305%s9F1B04%sDF1601%sDF1701%sDF14%s%sDF1801%s9F7B06%sDF1906%sDF2006%sDF2106%s",
				aidlen, icparainfo_out.AID, icparainfo_out.ASI, icparainfo_out.APP_VERNUM, icparainfo_out.TAC_DEFAULT, icparainfo_out.TAC_ONLINE, icparainfo_out.THRESHOLD, 
				icparainfo_out.TAC_DECLINE, icparainfo_out.TERM_MINLIMIT, icparainfo_out.MAXTARGET_PERCENT, icparainfo_out.TARGET_PERCENT, ddollen, icparainfo_out.DDOL_DEFAULT, 
				icparainfo_out.TERMPINCAP, icparainfo_out.VLPTRANSLIMIT, icparainfo_out.CLESSOFFLINELIMITAMT, icparainfo_out.CLESSTRANSAMT, icparainfo_out.TERMCVM_LIMIT);
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "EwpGeticPara:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}

	#ifdef __TEST__
		dcs_log(0,0,"下发参数值[%s]", pubkeybuf);
	#endif

	mysql_free_result(res);

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, " select count(*) from icpara_info");

	if(mysql_query(sock,sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "EwpGeticPara  查询总数据出错[%s]!", mysql_error(sock));
		return -1;
	}
	num_rows = 0;
    
    if(!(res = mysql_store_result(sock)))
    {
        dcs_log(0, 0, "EwpGeticPara:Couldn't get result from %s\n", mysql_error(sock));
       	return -1;
   	}
    
    	num_rows = mysql_num_rows(res) ;
	row = mysql_fetch_row(res);

	ewp_info->total_keypara = atoi(row[0]);
	dcs_log(0,0,"参数总条数[%d]", ewp_info->total_keypara );
	mysql_free_result(res);
	return 0;	
}

//保存EWP上送的TC值到manage_info中，首先需要查询pep_jnls交易信息表查询原笔交易信息
int EwpsaveTCvalue(EWP_INFO ewp_info, PEP_JNLS *pep_jnls)
{
	char bit_55[512];
	char batchno[6+1];
	PEP_JNLS pep_jnls_tc;
	
	char tmpbuf[512];
	int s = -1, len = 0, rtn =0;
	char sql[1024];
	//当前日期和时间
	char currentDate[8+1];
	char currentTime[6+1];
	struct  tm *tm;   
	time_t  t;
	
	memset(bit_55, 0, sizeof(bit_55));
	memset(batchno, 0, sizeof(batchno));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(&pep_jnls_tc, 0, sizeof(PEP_JNLS));
	memset(currentDate, 0, sizeof(currentDate));
	memset(currentTime, 0, sizeof(currentTime));

	time(&t);
	tm = localtime(&t);

	sprintf(currentDate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	sprintf(currentTime, "%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

	
	if(memcmp(ewp_info.filed55_length, "000", 3)!=0)
	{
		len= atoi(ewp_info.filed55_length);
		asc_to_bcd((unsigned char *)tmpbuf, (unsigned char * )ewp_info.filed55,len, 0);
		s=(len+1)/2;
	    //添加TC值解析
		//-----------------------------begin------------------------------
		#ifdef __TEST__
			dcs_debug(tmpbuf, s, "解析TC值得55域内容:");
		#endif
		rtn = analyse55(tmpbuf, s, stFd55_IC, 34);
		if(rtn < 0 )
		{
			dcs_log(0, 0, "解析55域失败,解析结果[%d]", rtn);
			memcpy(pep_jnls->aprespn, "ER", 2);	
		}
		else 
		{
			memcpy(pep_jnls->tc_value, stFd55_IC[0].szValue, 16);
#ifdef __LOG__
			dcs_log(0, 0, "TC值:[%s]", pep_jnls->tc_value);
#endif
			memcpy(pep_jnls->aprespn, "00", 2);
		}
		//-------------------------------end-------------------------------
		len = mysql_real_escape_string(sock, bit_55, tmpbuf, s);
	}
#ifdef __LOG__
	dcs_log(0,0,"ewp_info.consumer_sysreferno[%s]", ewp_info.consumer_sysreferno);
	dcs_log(0,0,"ewp_info.originaldate[%s]", ewp_info.originaldate);
#endif

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "SELECT  AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), tranamt, trace, poitcde, termcde, mercode, samid, process, sendcde, billmsg, termidm, aprespn, syseqno, \
	authcode, card_sn, revdate, termid \
			from pep_jnls \
			WHERE syseqno = '%s' and trndate = '%s' \
		UNION \
			SELECT AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), tranamt, trace, poitcde, termcde, mercode, samid, process, sendcde, billmsg, termidm, aprespn, syseqno, \
	authcode, card_sn, revdate, termid \
			from pep_jnls_all \
			WHERE syseqno = '%s' and trndate = '%s'",
			ewp_info.consumer_sysreferno, ewp_info.originaldate, ewp_info.consumer_sysreferno, ewp_info.originaldate);

	if(mysql_query(sock, sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " EwpsaveTCvalue:SELECT pep_jnls error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
	if(!(res = mysql_store_result(sock))) {
	    dcs_log(0, 0, "EwpsaveTCvalue:Couldn't get result from %s\n", mysql_error(sock));
	    return -1;
	}
    
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
   	if(num_rows == 1) {
		memcpy(pep_jnls_tc.outcdno, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls_tc.outcdno));
		memcpy(pep_jnls_tc.tranamt, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls_tc.tranamt));
		if(row[2])
			pep_jnls_tc.trace=atol(row[2]);
		memcpy(pep_jnls_tc.poitcde, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls_tc.poitcde));
		memcpy(pep_jnls_tc.termcde, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls_tc.termcde));
		memcpy(pep_jnls_tc.mercode, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls_tc.mercode));
		memcpy(pep_jnls_tc.samid, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls_tc.samid));
		memcpy(pep_jnls_tc.process, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls_tc.process));
		memcpy(pep_jnls_tc.sendcde, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls_tc.sendcde));
		memcpy(pep_jnls_tc.billmsg, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls_tc.billmsg));
		memcpy(pep_jnls_tc.termidm, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls_tc.termidm));
		memcpy(pep_jnls_tc.aprespn, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls_tc.aprespn));
		memcpy(pep_jnls_tc.syseqno, row[12] ? row[12] : "\0", (int)sizeof(pep_jnls_tc.syseqno));
		memcpy(pep_jnls_tc.authcode, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls_tc.authcode));
		memcpy(pep_jnls_tc.card_sn, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls_tc.card_sn));
		memcpy(pep_jnls_tc.revdate, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls_tc.revdate));
		memcpy(pep_jnls_tc.termid, row[16] ? row[16] : "\0", (int)sizeof(pep_jnls_tc.termid));
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "EwpsaveTCvalue:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "EwpsaveTCvalue:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "INSERT INTO manage_info(pepdate, peptime, trancde, outcdno, trace, sendcde, syseqno, termcde, mercode, \
	termidm, aprespn, field55, card_sn, tc_value, netmanage_code, msgtype, process, trnsndpid, trnsndp, trndate, trntime) \
	VALUES('%s', '%s', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), '%d', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%s', '%s', '%s')", \
	currentDate, currentTime, ewp_info.consumer_transcode, pep_jnls_tc.outcdno, pep_jnls_tc.trace, pep_jnls_tc.sendcde, \
	ewp_info.consumer_sysreferno, pep_jnls_tc.termcde, pep_jnls_tc.mercode, pep_jnls_tc.termidm, pep_jnls->aprespn, bit_55, \
	pep_jnls_tc.card_sn, pep_jnls->tc_value, ewp_info.netmanagecode, ewp_info.consumer_transtype, pep_jnls_tc.process, pep_jnls->trnsndpid, \
	pep_jnls->trnsndp, ewp_info.consumer_sentdate, ewp_info.consumer_senttime);
	if(mysql_query(sock,sql)) 
	{
	    #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "insert manage_info DB error [%s]!", mysql_error(sock));
	    return -1;
    }
	
	/*金额,转出卡号,卡片序列号   保存到pep_jnls中，方便TC值上送核心取值*/
	memcpy(pep_jnls->tranamt, pep_jnls_tc.tranamt, sizeof(pep_jnls_tc.tranamt));
	memcpy(pep_jnls->outcdno, pep_jnls_tc.outcdno, sizeof(pep_jnls_tc.outcdno));
	memcpy(pep_jnls->card_sn, pep_jnls_tc.card_sn, sizeof(pep_jnls_tc.card_sn));
	memcpy(pep_jnls->authcode, pep_jnls_tc.authcode, sizeof(pep_jnls_tc.authcode));
	memcpy(pep_jnls->syseqno, pep_jnls_tc.syseqno, sizeof(pep_jnls_tc.syseqno));
	memcpy(pep_jnls->revdate, pep_jnls_tc.revdate, sizeof(pep_jnls_tc.revdate));
	memcpy(pep_jnls->poitcde, pep_jnls_tc.poitcde, sizeof(pep_jnls_tc.poitcde));
	memcpy(pep_jnls->termcde, pep_jnls_tc.termcde, sizeof(pep_jnls_tc.termcde));
	memcpy(pep_jnls->mercode, pep_jnls_tc.mercode, sizeof(pep_jnls_tc.mercode));
	memcpy(pep_jnls->process, pep_jnls_tc.process, sizeof(pep_jnls_tc.process));
	pep_jnls->trace = pep_jnls_tc.trace;
	
	return 0;
}


int GetEwpTrace(EWP_INFO *ewp_info, char *trace)
{
	char sql[1024];
	int  temp = 0;
	
	dcs_log(0,0,"ewp_info.consumer_sysreferno [%s] ",ewp_info->consumer_sysreferno);
	dcs_log(0,0,"ewp_info.originaldate [%s] ",ewp_info->originaldate);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, " select trace, card_sn, poitcde, authcode, revdate, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), termcde, mercode, tranamt, billmsg \
		from pep_jnls \
		WHERE syseqno = '%s' and   trndate = '%s' and trancde != '046'", ewp_info->consumer_sysreferno, ewp_info->originaldate);

	if(mysql_query(sock, sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " GetPosTrace:SELECT pep_jnls error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
    	int num_fields;
	int num_rows = 0;
    
	if(!(res = mysql_store_result(sock))) {
	    	dcs_log(0, 0, "GetPosTrace:Couldn't get result from %s\n", mysql_error(sock));
	    	return -1;
	}
    
    	row = mysql_fetch_row(res);
    	num_rows = mysql_num_rows(res) ;	
   	if(num_rows == 1) {
		temp=atoi(row[0]);
		memcpy(ewp_info->cardseq, row[1] ? row[1] : "\0", (int)sizeof(ewp_info->cardseq));
		memcpy(ewp_info->serinputmode, row[2] ? row[2] : "\0", (int)sizeof(ewp_info->serinputmode));
		memcpy(ewp_info->authcode, row[3] ? row[3] : "\0", (int)sizeof(ewp_info->authcode));
		memcpy(ewp_info->revdate, row[4] ? row[4] : "\0", (int)sizeof(ewp_info->revdate));
		memcpy(ewp_info->consumer_cardno, row[5] ? row[5] : "\0", (int)sizeof(ewp_info->consumer_cardno));
		memcpy(ewp_info->termcde, row[6] ? row[6] : "\0", (int)sizeof(ewp_info->termcde));
		memcpy(ewp_info->mercode, row[7] ? row[7] : "\0", (int)sizeof(ewp_info->mercode));
		memcpy(ewp_info->consumer_money, row[8] ? row[8] : "\0", (int)sizeof(ewp_info->consumer_money));
		memcpy(ewp_info->consumer_orderno, row[9] ? row[9] : "\0", (int)sizeof(ewp_info->consumer_orderno));
    	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPosTrace:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPosTrace:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}	
	
	sprintf(trace,"%06d",temp);
#ifdef __LOG__
	dcs_log(0,0,"trace [%s][%s][%s][%s]",trace,ewp_info->termcde,ewp_info->mercode,ewp_info->consumer_money);
#endif

	mysql_free_result(res);
	return 0;
}

int EwpUpdatedownloadflag(EWP_INFO ewp_info, char *CurDate, char *CurTime)
{
#ifdef __LOG__
	dcs_log(0,0,"come in Updatedownloadflag :ewp_info.netmanagecode [%s] ",ewp_info.netmanagecode);
	dcs_log(0,0,"ewp_info.consumer_psamno [%s] ",ewp_info.consumer_psamno);
#endif
	char update_time[14+1];
	char sql[1024+1];
	
	sprintf(update_time,"%s%s", CurDate, CurTime);
	dcs_log(0,0,"update_time [%s] ", update_time);

	memset(sql, 0x00, sizeof(sql));
	if( memcmp(ewp_info.netmanagecode, "1", 1)==0 )
	{
		sprintf(sql, "UPDATE pos_cust_info set download_pubkey_flag = '1',pubkey_update_time='%s' where cust_psam='%s'", CurDate, ewp_info.consumer_psamno);
	}
	else if( memcmp(ewp_info.netmanagecode, "0", 1)==0 )
	{
		sprintf(sql, "UPDATE pos_cust_info set download_para_flag = '1',para_update_time='%s' where cust_psam='%s' ", CurDate, ewp_info.consumer_psamno);
	}
	else 
	{
		return -1;
	}
	
	if (mysql_query(sock, sql) != 0)
	{
   		dcs_log(0,0,"update pos_cust_info DB error [%s][%s]! ", sql, mysql_error(sock));
   		return -1;
	}
	
	dcs_log(0,0,"update pos_cust_info DB SUCCESS");
	
	return 0;
}

/*消费冲正根据订单号，交易时间查询pep_jnls原交易信息*/
int GetOrgInfo(EWP_INFO ewp_info, PEP_JNLS *opep_jnls)
{
	PEP_JNLS pep_jnls;
	char i_orderno[20+1];
	char sql[1024+1];
	
	memset(&pep_jnls, 0, sizeof(PEP_JNLS));
	memset(i_orderno, 0, sizeof(i_orderno));
	memcpy(i_orderno, ewp_info.consumer_orderno, strlen(ewp_info.consumer_orderno));
	pep_jnls.trnsndpid = opep_jnls->trnsndpid;

	#ifdef __LOG__
		dcs_log(0,0,"原笔交易订单号[%s]ewp_info.consumer_psamno[%s]" ,i_orderno,ewp_info.consumer_psamno);
	#endif
	memset(sql, 0, sizeof(0));
	sprintf(sql, "SELECT  aprespn, samid, merid, authcode, syseqno, sendcde, process, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), tranamt, trndate, trace, \
	trntime, billmsg, trnsndp, msgtype ,termidm, modecde, trancde, poitcde, card_sn, field55, termcde, mercode \
	FROM pep_jnls \
	where  billmsg = '%s' and samid ='%s' and msgtype != '0400' ; ", i_orderno, ewp_info.consumer_psamno);

	if(mysql_query(sock, sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " GetOrgInfo:SELECT pep_jnls error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
	if(!(res = mysql_store_result(sock))) {
	    dcs_log(0, 0, "GetOrgInfo:Couldn't get result from %s\n", mysql_error(sock));
	    return -1;
	}

    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
   	if(num_rows == 1) {
		memcpy(pep_jnls.aprespn, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls.aprespn));
		memcpy(pep_jnls.samid, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls.samid));
		if(row[2])
			pep_jnls.merid = atoi(row[2]);
		memcpy(pep_jnls.authcode, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls.authcode));
		memcpy(pep_jnls.syseqno, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls.syseqno));
		memcpy(pep_jnls.sendcde, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls.sendcde));
		memcpy(pep_jnls.process, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls.process));
		memcpy(pep_jnls.outcdno, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls.outcdno));
		memcpy(pep_jnls.tranamt, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls.tranamt));
		memcpy(pep_jnls.trndate, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls.trndate));
		if(row[10])
			pep_jnls.trace = atoi(row[10]);
		memcpy(pep_jnls.trntime, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls.trntime));
		memcpy(pep_jnls.billmsg, row[12] ? row[12] : "\0", (int)sizeof(pep_jnls.billmsg));
		memcpy(pep_jnls.trnsndp, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls.trnsndp));
		memcpy(pep_jnls.msgtype, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls.msgtype));
		memcpy(pep_jnls.termidm, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls.termidm));
		memcpy(pep_jnls.modecde, row[16] ? row[16] : "\0", (int)sizeof(pep_jnls.modecde));
		memcpy(pep_jnls.trancde, row[17] ? row[17] : "\0", (int)sizeof(pep_jnls.trancde));
		memcpy(pep_jnls.poitcde, row[18] ? row[18] : "\0", (int)sizeof(pep_jnls.poitcde));
		memcpy(pep_jnls.card_sn, row[19] ? row[19] : "\0", (int)sizeof(pep_jnls.card_sn));
		memcpy(pep_jnls.filed55, row[20] ? row[20] : "\0", (int)sizeof(pep_jnls.filed55));		
		memcpy(pep_jnls.termcde, row[21] ? row[21] : "\0", (int)sizeof(pep_jnls.termcde));
		memcpy(pep_jnls.mercode, row[22] ? row[22] : "\0", (int)sizeof(pep_jnls.mercode));		
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOrgInfo:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOrgInfo:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}	
	
	memcpy(opep_jnls, &pep_jnls, sizeof(PEP_JNLS));
	mysql_free_result(res);
	return 0;
}

int key_para_update(EWP_INFO *ewp_info)
{
	char sql[1024+1];
	memset(sql , 0, sizeof(sql));
	MYSQL *sock  = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	sprintf(sql, "SELECT download_pubkey_flag, download_para_flag FROM pos_cust_info where cust_psam= '%s'", ewp_info->consumer_psamno);

	if(mysql_query(sock, sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " key_para_update:SELECT pos_cust_info error [%s]!", mysql_error(sock));
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
	    dcs_log(0, 0, "key_para_update:Couldn't get result from %s\n", mysql_error(sock));
	    //从连接池获取的连接需要释放掉
	    if(GetIsoMultiThreadFlag())
	    	SetFreeLink(sock);
	    return -1;
	}
	//从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
   	if(num_rows == 1) {
		memcpy(ewp_info->download_pubkey_flag, row[0] ? row[0] : "\0",  (int)sizeof(ewp_info->download_pubkey_flag));
		memcpy(ewp_info->download_para_flag, row[1] ? row[1] : "\0", (int)sizeof(ewp_info->download_para_flag));	
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "key_para_update:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "key_para_update:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}	
#ifdef __LOG__
   	dcs_log(0,0,"公钥下载标识	0:需要  1:不需要		本次取值[%s]" ,ewp_info->download_pubkey_flag);
   	dcs_log(0,0,"参数下载标识	0:需要  1:不需要		本次取值[%s]",ewp_info->download_para_flag);
  #endif
	mysql_free_result(res);

	return 0;
}


/*根据ksn号获取该终端 bdk、bdkxor索引，核心pin转加密密钥*/
int Getksninfo(KSN_INFO *ksn_info, char *ksnbuf)
{
	char sql[512];
	int channel_id = -1 ,transfer_key_index = -1,bdk_index = -1,bdk_xor_index = -1;
	memset(sql, 0x00, sizeof(sql));

	dcs_log(0, 0, "ksnbuf=[%s]", ksnbuf);

	sprintf(sql, "SELECT transfer_key_index, bdk_index, bdk_xor_index,channel_id, initkey, final_info  FROM ksn_info WHERE ksn = '%s' ;", ksnbuf);

	if(mysql_query(sock,sql)) {
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "select from ksn_info error [%s]!", mysql_error(sock));
		return -1;
	}
	
	MYSQL_RES *res;
	MYSQL_ROW row ;
	int num_fields;
	int num_rows = 0;

	if (!(res = mysql_store_result(sock))) 
	{
		dcs_log(0, 0, "select from ksn_info error: Couldn't get result from %s\n", mysql_error(sock));
		return -1;
	}

	num_fields = mysql_num_fields(res);
	row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
	if(num_rows == 1) 
	{
		if(row[0])
			transfer_key_index = atoi(row[0]);
		if(row[1])
			bdk_index = atoi(row[1]);
		if(row[2])
			bdk_xor_index = atoi(row[2]);
		if(row[3])
			channel_id = atoi(row[3]);
		memcpy(ksn_info->initkey, row[4] ? row[4] : "\0", 32);
		memcpy(ksn_info->final_info, row[5] ? row[5] : "\0", 58);
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "Getksninfo:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "Getksninfo:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
#ifdef __LOG__
	dcs_log(0,0,"transfer_key_index=[%d]",transfer_key_index);
	dcs_log(0,0,"bdk_index=[%d]",bdk_index);
	dcs_log(0,0,"bdk_xor_index=[%d]",bdk_xor_index);
	dcs_log(0,0,"channel_id=[%d]",channel_id);
	dcs_log(0,0,"initkey=[%s]",ksn_info->initkey);
	dcs_log(0,0,"final_info=[%s]",ksn_info->final_info);
#endif

	mysql_free_result(res);
	//判断是否需要删除ksn_info 表
	if(ksn_info->transfer_key_index !=transfer_key_index || ksn_info->bdk_index !=bdk_index
		|| ksn_info->bdk_xor_index != bdk_xor_index )
	{
		dcs_log(0,0,"索引不同,需要删除原记录,重新计算");
		memset(sql, 0x00, sizeof(sql));
		sprintf(sql, "delete from ksn_info WHERE ksn = '%s' ;" ,ksnbuf);

		if(mysql_query(sock,sql)) {
			#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
			dcs_log(0, 0, "update from ksn_info error [%s]!", mysql_error(sock));
			return -1;
		}
		//返回-1,重新计算initkey
		return -1;
	}

	//判断是否需要update ksn_info 表
	if(ksn_info->channel_id != channel_id )
	{
		dcs_log(0,0,"渠道号不同,需要更新ksn_info 表");
		memset(sql, 0x00, sizeof(sql));
		sprintf(sql, "update ksn_info set channel_id ='%04d' WHERE ksn = '%s' ;", ksn_info->channel_id ,ksnbuf);

		if(mysql_query(sock,sql)) {
			#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
			dcs_log(0, 0, "update from ksn_info error [%s]!", mysql_error(sock));
			return -1;
		}
	}
	return 0;	
}

/*根据ksn号更新该终端的initkey 密钥*/
int Saveinitkey(KSN_INFO ksn_info)
{
	char sql[512];
	
	memset(sql, 0x00, sizeof(sql));
	
	dcs_log(0, 0, "Saveinitkey ksn_info.ksn=[%s]", ksn_info.ksn);
	dcs_log(0, 0, "Saveinitkey ksn_info.initkey=[%s]", ksn_info.initkey);

	sprintf(sql, "UPDATE ksn_info set initkey = '%s' where ksn='%s'", ksn_info.initkey, ksn_info.initksn);	
	
	if (mysql_query(sock, sql) != 0)
	{
   		dcs_log(0,0,"update ksn_info DB error [%s][%s]! ", sql, mysql_error(sock));
   		return -1;
	}
	
	dcs_log(0,0,"update ksn_info DB SUCCESS");
	
	return 0;
}

/*根据ksn号更新该终端的dukptkey*/
int Savedukptkey(KSN_INFO ksn_info)
{
	char sql[512];
	
	memset(sql, 0x00, sizeof(sql));

	dcs_log(0, 0, "ksn_info.dukptkey=[%s]", ksn_info.dukptkey);
	dcs_log(0, 0, "ksn_info.initksn=[%s]", ksn_info.initksn);
	
	sprintf(sql, "UPDATE ksn_info set dukptkey = '%s' where ksn='%s'", ksn_info.dukptkey, ksn_info.initksn);	
	
	if (mysql_query(sock, sql) != 0)
	{
   		dcs_log(0,0,"update ksn_info DB error [%s][%s]! ", sql, mysql_error(sock));
   		return -1;
	}
	
	dcs_log(0,0,"update ksn_info DB SUCCESS");
	
	return 0;
}

int mpos_key_para_update(EWP_INFO *ewp_info)
{
	char sql[1024+1];
	memset(sql , 0, sizeof(sql));

	//根据 用户名 查找
	sprintf(sql, "SELECT download_pubkey_flag, download_para_flag FROM mpos_cust_info where cust_id= '%s'", ewp_info->consumer_username);

	if(mysql_query(sock, sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " key_para_update:SELECT pos_cust_info error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
	if(!(res = mysql_store_result(sock))) {
	    dcs_log(0, 0, "key_para_update:Couldn't get result from %s\n", mysql_error(sock));
	    return -1;
	}

    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
   	if(num_rows == 1) 
	{
		memcpy(ewp_info->download_pubkey_flag, row[0] ? row[0] : "\0",  (int)sizeof(ewp_info->download_pubkey_flag));
		memcpy(ewp_info->download_para_flag, row[1] ? row[1] : "\0", (int)sizeof(ewp_info->download_para_flag));	
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "key_para_update:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "key_para_update:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}	
	mysql_free_result(res);
	
	dcs_log(0,0,"公钥下载标识	0:需要  1:不需要		本次取值[%s]" ,ewp_info->download_pubkey_flag);
	dcs_log(0,0,"参数下载标识	0:需要  1:不需要		本次取值[%s]",ewp_info->download_para_flag);
	return 0;
}

/*Mpos消费撤销根据订单号，交易时间查询pep_jnls成功的原交易信息*/
int MposGetOrgInfo(EWP_INFO ewp_info, PEP_JNLS *opep_jnls)
{
	PEP_JNLS pep_jnls;
	char i_orderno[20+1];
	char sql[1024+1];
	
	memset(&pep_jnls, 0, sizeof(PEP_JNLS));
	memset(i_orderno, 0, sizeof(i_orderno));
	memcpy(i_orderno, ewp_info.consumer_orderno, strlen(ewp_info.consumer_orderno));
	
	#ifdef __LOG__
		dcs_log(0,0,"原笔交易订单号[%s]ewp_info.consumer_psamno[%s]" ,i_orderno,ewp_info.consumer_psamno);
	#endif
	memset(sql, 0, sizeof(0));
	sprintf(sql, "SELECT  aprespn, samid, merid, authcode, syseqno, sendcde, process, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), tranamt, trndate, trace, \
	trntime, billmsg, trnsndp, msgtype ,termidm, modecde, trancde, poitcde, card_sn, field55, termcde, mercode, batchno, revdate \
	FROM pep_jnls \
	where  billmsg = '%s' and samid ='%s' and msgtype != '0400' and aprespn = '00' ; ", i_orderno, ewp_info.consumer_psamno);

	if(mysql_query(sock, sql) != 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, " MposGetOrgInfo:SELECT pep_jnls error [%s]!", mysql_error(sock));
		return -1;
	}
	MYSQL_RES *res;
   	MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
	if(!(res = mysql_store_result(sock))) 
	{
	    dcs_log(0, 0, "MposGetOrgInfo:Couldn't get result from %s\n", mysql_error(sock));
	    return -1;
	}

    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res) ;	
   	if(num_rows == 1) 
	{
		memcpy(pep_jnls.aprespn, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls.aprespn));
		memcpy(pep_jnls.samid, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls.samid));
		if(row[2])
			pep_jnls.merid = atoi(row[2]);
		memcpy(pep_jnls.authcode, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls.authcode));
		memcpy(pep_jnls.syseqno, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls.syseqno));
		memcpy(pep_jnls.sendcde, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls.sendcde));
		memcpy(pep_jnls.process, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls.process));
		memcpy(pep_jnls.outcdno, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls.outcdno));
		memcpy(pep_jnls.tranamt, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls.tranamt));
		memcpy(pep_jnls.trndate, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls.trndate));
		if(row[10])
			pep_jnls.trace = atoi(row[10]);
		memcpy(pep_jnls.trntime, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls.trntime));
		memcpy(pep_jnls.billmsg, row[12] ? row[12] : "\0", (int)sizeof(pep_jnls.billmsg));
		memcpy(pep_jnls.trnsndp, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls.trnsndp));
		memcpy(pep_jnls.msgtype, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls.msgtype));
		memcpy(pep_jnls.termidm, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls.termidm));
		memcpy(pep_jnls.modecde, row[16] ? row[16] : "\0", (int)sizeof(pep_jnls.modecde));
		memcpy(pep_jnls.trancde, row[17] ? row[17] : "\0", (int)sizeof(pep_jnls.trancde));
		memcpy(pep_jnls.poitcde, row[18] ? row[18] : "\0", (int)sizeof(pep_jnls.poitcde));
		memcpy(pep_jnls.card_sn, row[19] ? row[19] : "\0", (int)sizeof(pep_jnls.card_sn));
		memcpy(pep_jnls.filed55, row[20] ? row[20] : "\0", (int)sizeof(pep_jnls.filed55));		
		memcpy(pep_jnls.termcde, row[21] ? row[21] : "\0", (int)sizeof(pep_jnls.termcde));
		memcpy(pep_jnls.mercode, row[22] ? row[22] : "\0", (int)sizeof(pep_jnls.mercode));
		memcpy(pep_jnls.batch_no, row[23] ? row[23] : "\0", (int)sizeof(pep_jnls.batch_no));
		memcpy(pep_jnls.revdate, row[24] ? row[24] : "\0", (int)sizeof(pep_jnls.revdate));
    }
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "MposGetOrgInfo:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "MposGetOrgInfo:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}	
	
	memcpy(opep_jnls, &pep_jnls, sizeof(PEP_JNLS));
	mysql_free_result(res);
	return 0;
}
/*
获取POS渠道ID
*/
int GetPosChannelId(char *termcode, char *mercode)
{
	char sql[1024];
	int i_channel_id = 0;

	memset(sql, 0, sizeof(sql));

	dcs_log(0, 0, "查询POS渠道ID");
	
	#ifdef __LOG__
		dcs_log(0, 0, "termcode =[%s]", termcode);
		dcs_log(0, 0, "mercode =[%s]", mercode);
	#endif
	
	sprintf(sql, "SELECT channel_id FROM pos_channel_info  where  mgr_term_id = '%s' and mgr_mer_id = '%s';", termcode, mercode);
	
	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetPosChannelId error=[%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetPosChannelId : Couldn't get result from %s\n", mysql_error(sock)); 
        return -1;
    }
    
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows== 1) 
	{
		if(row[0])
			i_channel_id = atoi(row[0]);
	
	}
	else if(num_rows==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPosChannelId 未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetPosChannelId 找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);
	return i_channel_id;
}

/*
函数功能：批上送POS终端成功的脱机消费交易，查询交易状态
返回值：-1： 处理失败   返回96
      3： 找到已发请求的记录
      0：未找到记录
*/
int GetOriTuoInfo(PEP_JNLS *pep_jnls)
{
	char sql[1024];
	memset(sql, 0, sizeof(sql));

	dcs_log(0, 0, "脱机消费查询是否发起原笔");

	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls->posmercode = [%s]", pep_jnls->posmercode);
		dcs_log(0, 0, "pep_jnls->postermcde = [%s]", pep_jnls->postermcde);
		dcs_log(0, 0, "pep_jnls->termidm = [%s]", pep_jnls->termidm);
		dcs_log(0, 0, "pep_jnls->termtrc =[%s]", pep_jnls->termtrc);
		dcs_log(0, 0, "pep_jnls->trndate = [%s]", pep_jnls->trndate);
	#endif

	sprintf(sql, "SELECT aprespn, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), syseqno, termcde, mercode, trace, process, batchno, poscode, pepdate, peptime\
	FROM pep_jnls  where  posmercode = '%s' and postermcde = '%s' and termidm ='%s' \
	and termtrc = '%s' and trndate = '%s' and  trancde ='050';", \
	pep_jnls->posmercode, pep_jnls->postermcde, pep_jnls->termidm, pep_jnls->termtrc,  pep_jnls->trndate);

	if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetOriTuoInfo error=[%s]!", mysql_error(sock));
        return -1;
    }

    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;

    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetOriTuoInfo : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }

    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows== 1) {
		memcpy(pep_jnls->aprespn, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls->aprespn));
		memcpy(pep_jnls->outcdno, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->outcdno));
		memcpy(pep_jnls->syseqno, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls->syseqno));
		memcpy(pep_jnls->termcde, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->termcde));
		memcpy(pep_jnls->mercode, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls->mercode));
		if(row[5])
			pep_jnls->trace =atoi(row[5]);
		memcpy(pep_jnls->process, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls->process));
		memcpy(pep_jnls->batch_no, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls->batch_no));
		memcpy(pep_jnls->poscode, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls->poscode));
		memcpy(pep_jnls->pepdate, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls->pepdate));
		memcpy(pep_jnls->peptime, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls->peptime));
	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls->aprespn = [%s]", pep_jnls->aprespn);
//		dcs_log(0, 0, "pep_jnls->outcdno = [%s]", pep_jnls->outcdno);
		dcs_log(0, 0, "pep_jnls->syseqno = [%s]", pep_jnls->syseqno);
		dcs_log(0, 0, "pep_jnls->termcde =[%s]", pep_jnls->termcde);
		dcs_log(0, 0, "pep_jnls->mercode = [%s]", pep_jnls->mercode);
		dcs_log(0, 0, "pep_jnls->trace = [%d]", pep_jnls->trace);
		dcs_log(0, 0, "pep_jnls->process = [%s]", pep_jnls->process);
		dcs_log(0, 0, "pep_jnls->batch_no = [%s]", pep_jnls->batch_no);
		dcs_log(0, 0, "pep_jnls->poscode = [%s]", pep_jnls->poscode);
		dcs_log(0, 0, "pep_jnls->pepdate = [%s]", pep_jnls->pepdate);
		dcs_log(0, 0, "pep_jnls->peptime = [%s]", pep_jnls->peptime);
	#endif

	}
	else if(num_rows==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOriTuoInfo 未找到符合条件的记录");
		mysql_free_result(res);
		return 0;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	   #endif
		dcs_log(0, 0, "GetOriTuoInfo 找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);

	return 3;
}

/*保存 KSN  明文密钥  密钥文件值 到 ksn_info 数据表*/
int Savefinalinfo(KSN_INFO ksn_info, char *ksnbuf)
{
	char sql[512];
	memset(sql,0x00,sizeof(sql));
	#ifdef __TEST__
		dcs_log(0, 0, "ksnbuf=[%s]", ksnbuf);
		dcs_log(0, 0, "ksn_info->transfer_key_index=[%d]", ksn_info.transfer_key_index);
		dcs_log(0, 0, "ksn_info->bdkindex=[%d]", ksn_info.bdk_index);
		dcs_log(0, 0, "ksn_info->bdk_xor_index=[%d]", ksn_info.bdk_xor_index);
		dcs_log(0, 0, "ksn_info->channel_id=[%d]", ksn_info.channel_id);
		dcs_log(0, 0, "ksn_info->initkey=[%s]", ksn_info.initkey);
		dcs_log(0, 0, "ksn_info->final_info=[%s]", ksn_info.final_info);
	#endif
	sprintf(sql, "INSERT INTO ksn_info(ksn, transfer_key_index, bdk_index, bdk_xor_index, channel_id, initkey, final_info) VALUES('%s', '%d', '%d', '%d', '%d', '%s', '%s')",\
	ksnbuf, ksn_info.transfer_key_index, ksn_info.bdk_index, ksn_info.bdk_xor_index, ksn_info.channel_id, ksn_info.initkey, ksn_info.final_info);

	if(mysql_query(sock,sql)) {
	        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	        dcs_log(0, 0, "insert ksn_info DB error [%s]!", mysql_error(sock));
	        return -1;
    }
	return 0;
}

/*根据ksn号获取该终端 bdk、bdkxor索引，核心pin转加密密钥,initkey*/
int GetMposInfoByKsn(KSN_INFO *ksn_info)
{
	char sql[512];
	int channel_id = -1 ,transfer_key_index = -1,bdk_index = -1,bdk_xor_index = -1;
	memset(sql, 0x00, sizeof(sql));
#ifdef __TEST__
	dcs_log(0, 0, "initksn=[%s]", ksn_info->initksn);
#endif
	sprintf(sql, "SELECT transfer_key_index, bdk_index, bdk_xor_index,channel_id, pinkey,initkey  FROM ksn_info WHERE ksn = '%s' ;",  ksn_info->initksn);

	if(mysql_query(sock,sql)) {
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "select from ksn_info error [%s]!", mysql_error(sock));
		return -1;
	}
	
	MYSQL_RES *res;
	MYSQL_ROW row ;
	int num_fields;
	int num_rows = 0;

	if (!(res = mysql_store_result(sock))) 
	{
		dcs_log(0, 0, "select from ksn_info error: Couldn't get result from %s\n", mysql_error(sock));
		return -1;
	}

	num_fields = mysql_num_fields(res);
	row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
	if(num_rows == 1) 
	{
		if(row[0])
			ksn_info->transfer_key_index = atoi(row[0]);
		if(row[1])
			ksn_info->bdk_index = atoi(row[1]);
		if(row[2])
			ksn_info->bdk_xor_index = atoi(row[2]);
		if(row[3])
			ksn_info->channel_id = atoi(row[3]);
		memcpy(ksn_info->pinkey, row[4] ? row[4] : "\0", 16);
		memcpy(ksn_info->initkey, row[5] ? row[5] : "\0", 32);
		
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "Getksninfo:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "Getksninfo:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
#ifdef __TEST__
	dcs_log(0,0,"transfer_key_index=[%d]",ksn_info->transfer_key_index);
	dcs_log(0,0,"bdk_index=[%d]",ksn_info->bdk_index);
	dcs_log(0,0,"bdk_xor_index=[%d]",ksn_info->bdk_xor_index);
	dcs_log(0,0,"channel_id=[%d]",ksn_info->channel_id);
	dcs_log(0,0,"initkey=[%s]",ksn_info->initkey);
#endif
	mysql_free_result(res);
	return 0;	
}

/*查找sha_info表
*  参数:sha1 报文计算的哈希值,maxNum 表示报文允许出现最大次数
* 返回值:-1 表示数据库操作出现错误,
			 0 表示合法报文,
			 1 表示报文出现次数已达最大值,为非法报文应丢弃
*/
int GetShaInfo(char * sha1, int maxNum)
{
	char sql[512];
	int num =0;

	//一般交易直接insert into
	if(maxNum == 1)
	{
		memset(sql , 0, sizeof(sql));
		sprintf(sql, "INSERT INTO sha_info(sha1) VALUES('%s')", sha1);
		if(mysql_query(sock, sql))
		{
			if(mysql_errno(sock) == 1062)// 主键约束。重复的sha1
			{
				#ifdef __TEST__
					dcs_log(0, 0, "该sha1信息重复出现");
				#endif
				return 1;
			}
			dcs_log(0, 0, "sql[%s]", sql);
			dcs_log(0, 0, "INSERT INTO sha_info DB error [%s][%d]", mysql_error(sock), mysql_errno(sock));
			return -1;
		}
		#ifdef __TEST__
			dcs_log(0, 0, "首次出现的sha,插入表格");
		#endif
		return 0;
	}
	else 
	{
		memset(sql, 0, sizeof(sql));
		sprintf(sql , "select num from sha_info where sha1 = '%s'",sha1);
		if(mysql_query(sock,sql))
		{
			dcs_log(0, 0, "sql[%s]", sql);
			dcs_log(0, 0, "select sha_info DB error [%s]",mysql_error(sock));
			return -1;
		}

		MYSQL_RES *res;
		MYSQL_ROW row ;
		int num_fields;
		int num_rows = 0;

		if (!(res = mysql_store_result(sock))) 
		{
		    dcs_log(0, 0, "GetShaInfo : Couldn't get result from %s\n", mysql_error(sock)); 
		    return -1;
		}

		num_fields = mysql_num_fields(res);
		row = mysql_fetch_row(res);
		num_rows = mysql_num_rows(res);
		if(num_rows== 1) 
		{
			if(row[0])
				num  = atoi(row[0]);
		}
		else if(num_rows==0)
		{
			#ifdef __TEST__
					dcs_log(0, 0, "首次出现的sha,插入表格");
			#endif
			mysql_free_result(res);
			memset(sql , 0, sizeof(sql));
			sprintf(sql, "INSERT INTO sha_info(sha1) VALUES('%s')", sha1);
			if(mysql_query(sock, sql))
			{	
				dcs_log(0, 0, "sql[%s]", sql);
				dcs_log(0, 0, "INSERT INTO sha_info DB error [%s][%d]", mysql_error(sock), mysql_errno(sock));
				return -1;
			}
			return 0;
		}
		mysql_free_result(res);

		if(num >=maxNum)
		{
			#ifdef __TEST__
				dcs_log(0, 0, "该sha1信息出现次数已达最大次数num = %d",num);
			#endif
			return 1;
		}
		else
		{
			dcs_log(0, 0, "该sha1信息出现次数未达最大次数maxNum=%d,当前次数为num = %d,更新记录",maxNum,num);
			memset(sql , 0, sizeof(sql));
			sprintf(sql, "UPDATE  sha_info SET num = %c WHERE sha1 = '%s'", (num+1+'0'), sha1);
			if(mysql_query(sock, sql))
			{
				dcs_log(0, 0, "sql[%s]", sql);
				dcs_log(0, 0, "UPDATE sha_info DB error [%s]", mysql_error(sock));
				return -1;
			}
			return 0;
		}
	}
	return 0;
}


//保存POS终端上送的管理类信息
int Pos_SavaManageInfo(ISO_data siso, PEP_JNLS pep_jnls, char *currentdate)
{
	char sql[2048], tmpbuf[27], netmanage_code[3+1];
	char escape_buf55[255+1];
	int s = 0;
	
	memset(sql, 0x00, sizeof(sql));
	memset(escape_buf55, 0x00, sizeof(escape_buf55));
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(netmanage_code, 0x00, sizeof(netmanage_code));
	
	memcpy(pep_jnls.pepdate, currentdate, 8);
	memcpy(pep_jnls.peptime, currentdate+8, 6);
	memcpy(pep_jnls.sendcde, "03000000", 8);
	
	memset(escape_buf55, 0, sizeof(escape_buf55));
	s = getbit(&siso, 55, (unsigned char *)pep_jnls.filed55);
	if(s <= 0)
	{
#ifdef __LOG__
		 dcs_log(0, 0, "bit_55 is null!");
#endif
	}
	else 
	{
		mysql_real_escape_string(sock, escape_buf55, pep_jnls.filed55, s);
	}
	//脚本不需要重新取数据
	if(memcmp(pep_jnls.trancde, "04", 2)!=0 || memcmp(pep_jnls.trancde, "046", 3)==0)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 60, (unsigned char *)tmpbuf);
		if( s <= 0)
		{
			dcs_log(0, 0, "报文60域数据信息错误");
		}
		memcpy(pep_jnls.batch_no, tmpbuf+2, 6);
		memcpy(netmanage_code, tmpbuf+8, 3);
	}
	else
		memcpy(netmanage_code, pep_jnls.netmanage_code, 3);
	if(getbit(&siso, 41, (unsigned char *)pep_jnls.termcde)<0)
	{
		dcs_log(0, 0, "bit_41 is null!");
	}

	if(getbit(&siso, 42,(unsigned char *)pep_jnls.mercode)<0)
	{
		dcs_log(0, 0, "bit_42 is null!");	
	}
	
	if(getbit(&siso, 3, (unsigned char *)pep_jnls.process)<0)
	{
		 dcs_log(0, 0, "can not get bit_3!");	
		 return -1;
	}

	getbit(&siso, 2, (unsigned char *)pep_jnls.outcdno);
	
	s = getbit(&siso, 38, (unsigned char *)pep_jnls.authcode);
	if(s<0)
		dcs_log(0, 0, "get bit_38 error");
	
	#ifdef __TEST__
		dcs_log(0, 0, "pep_jnls.pepdate=[%s]", pep_jnls.pepdate);
		dcs_log(0, 0, "pep_jnls.peptime=[%s]", pep_jnls.peptime);
		dcs_log(0, 0, "pep_jnls.trancde=[%s]", pep_jnls.trancde);
//		dcs_log(0, 0, "pep_jnls.outcdno=[%s]", pep_jnls.outcdno);
		dcs_log(0, 0, "pep_jnls.tranamt=[%s]", pep_jnls.tranamt);
		dcs_log(0, 0, "pep_jnls.trace=[%d]", pep_jnls.trace);
		dcs_log(0, 0, "pep_jnls.termtrc=[%s]", pep_jnls.termtrc);
		dcs_log(0, 0, "pep_jnls.sendcde=[%s]", pep_jnls.sendcde);
		dcs_log(0, 0, "pep_jnls.syseqno=[%s]", pep_jnls.syseqno);
		dcs_log(0, 0, "pep_jnls.termcde=[%s]", pep_jnls.termcde);
		dcs_log(0, 0, "pep_jnls.mercode=[%s]", pep_jnls.mercode);
		dcs_log(0, 0, "pep_jnls.termidm=[%s]", pep_jnls.termidm);
		dcs_log(0, 0, "pep_jnls.aprespn=[%s]", pep_jnls.aprespn);
		dcs_log(0, 0, "pep_jnls.postermcde=[%s]", pep_jnls.postermcde);
		dcs_log(0, 0, "pep_jnls.posmercode=[%s]", pep_jnls.posmercode);
		dcs_log(0, 0, "pep_jnls.filed55=[%s]", pep_jnls.filed55);
		dcs_log(0, 0, "pep_jnls.card_sn=[%s]", pep_jnls.card_sn);
		dcs_log(0, 0, "pep_jnls.tc_value=[%s]", pep_jnls.tc_value);
		dcs_log(0, 0, "netmanage_code=[%s]", netmanage_code);
		dcs_log(0, 0, "pep_jnls.msgtype=[%s]", pep_jnls.msgtype);
		dcs_log(0, 0, "pep_jnls.process=[%s]", pep_jnls.process);
		dcs_log(0, 0, "pep_jnls.batch_no=[%s]", pep_jnls.batch_no);
		dcs_log(0, 0, "pep_jnls.trnsndpid=[%d]", pep_jnls.trnsndpid);
		dcs_log(0, 0, "pep_jnls.trnsndp=[%s]", pep_jnls.trnsndp);
		dcs_log(0, 0, "pep_jnls.authcode=[%s]", pep_jnls.authcode);
		dcs_log(0, 0, "pep_jnls.trnmsgd=[%s]", pep_jnls.trnmsgd); 
	#endif
	sprintf(sql, "INSERT INTO manage_info(pepdate, peptime, trancde, outcdno, tranamt, trace, termtrc, sendcde, syseqno, termcde, mercode, billmsg, \
	termidm, aprespn, postermcde, posmercode, field55, card_sn, tc_value, netmanage_code, msgtype, process, batchno, trnsndpid, trnsndp, authcode, trnmsgd) \
	VALUES('%s', '%s', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%d', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%s', '%s', '%s')", \
	pep_jnls.pepdate, pep_jnls.peptime, pep_jnls.trancde, pep_jnls.outcdno, pep_jnls.tranamt, pep_jnls.trace, pep_jnls.termtrc, pep_jnls.sendcde, pep_jnls.syseqno, \
	pep_jnls.termcde, pep_jnls.mercode, pep_jnls.billmsg, pep_jnls.termidm, pep_jnls.aprespn, pep_jnls.postermcde, pep_jnls.posmercode, escape_buf55, pep_jnls.card_sn, \
	pep_jnls.tc_value, netmanage_code, pep_jnls.msgtype, pep_jnls.process, pep_jnls.batch_no, pep_jnls.trnsndpid, pep_jnls.trnsndp, pep_jnls.authcode, pep_jnls.trnmsgd);
	if(mysql_query(sock,sql)) 
	{
	    #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "insert manage_info DB error [%s]!", mysql_error(sock));
	    return -1;
    }
	return 0;
}

//脚本上送通知交易更新交易应答状态
int saveOrUpdateManageInfo(ISO_data iso, PEP_JNLS *opep_jnls)
{    
	int	s = 0;
	char tmpbuf[256];
	struct  tm *tm;
	time_t  t;
       
    PEP_JNLS pep_jnls;
	
    char i_pepdate[9];
    char i_peptime[7];
    char i_process[7];
    char i_termcde[9];
    char i_mercode[16];
    char i_response[3];;
    long i_trace = 0;
	char i_authcode[6+1];
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(&pep_jnls, 0, sizeof(PEP_JNLS));
	memset(i_response, 0, sizeof(i_response));
	memset(i_pepdate, 0, sizeof(i_pepdate));
	memset(i_peptime, 0, sizeof(i_peptime));
	memset(i_process, 0, sizeof(i_process));
	memset(i_termcde, 0, sizeof(i_termcde));
	memset(i_mercode, 0, sizeof(i_mercode));
	memset(i_authcode, 0, sizeof(i_authcode));
	
	time(&t);
	tm = localtime(&t);
	 
	s=getbit(&iso, 3, (unsigned char *)i_process);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_3 error,ERROR应答");
		return -1;
	}
    
	s=getbit(&iso, 7, (unsigned char *)tmpbuf);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_7 error,ERROR应答");
		return -1;
	}
	else
	{
		sprintf(i_pepdate, "%04d%4.4s", tm->tm_year+1900, tmpbuf);
		memcpy(i_peptime, tmpbuf+4, 6);
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s=getbit(&iso, 11, (unsigned char *)tmpbuf);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_11 error,ERROR应答");
		return -1;
	}
	
	sscanf(tmpbuf, "%06ld", &i_trace);
	
	s = getbit(&iso, 38, (unsigned char *)i_authcode);
	if(s<0)
		dcs_log(0, 0, "get bit_38 error");
	
	s=getbit(&iso, 39, (unsigned char *)i_response);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_39 error,ERROR应答");
		return -1;
	}
	
	s=getbit(&iso, 41, (unsigned char *)i_termcde);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_41 error,ERROR应答");
		return -1;
	}
	
	s=getbit(&iso, 42, (unsigned char *)i_mercode);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_42 error");
		return -1;
	}
	
#ifdef __TEST__
    dcs_log(0, 0, "Update manage_info i_response = [%s]", i_response);
    dcs_log(0, 0, "Update manage_info i_pepdate = [%s]", i_pepdate);
    dcs_log(0, 0, "Update manage_info i_peptime = [%s]", i_peptime);
    dcs_log(0, 0, "Update manage_info i_process = [%s]", i_process);
    dcs_log(0, 0, "Update manage_info i_termcde = [%s]", i_termcde);
    dcs_log(0, 0, "Update manage_info i_trace = [%ld]", i_trace);
    dcs_log(0, 0, "Update manage_info i_mercode = [%s]", i_mercode);
	dcs_log(0, 0, "Update manage_info i_authcode = [%s]", i_authcode);
#endif
	MYSQL *sock  = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
    char sql[1024];
    memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "UPDATE manage_info set aprespn='%s', authcode ='%s' where pepdate ='%s' and peptime='%s' \
	and process='%s' and trace='%ld' and termcde='%s' and mercode='%s';", i_response, i_authcode, i_pepdate, \
	i_peptime, i_process, i_trace, i_termcde, i_mercode);
    
    if(mysql_query(sock, sql)) {
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "Update manage_info DB  error [%s]!", mysql_error(sock));

        //从连接池获取的连接需要释放掉
        if(GetIsoMultiThreadFlag())
        	SetFreeLink(sock);
        return -1;
    }
    	
    memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "SELECT pepdate, peptime, aprespn, samid, authcode, syseqno, mercode, termcde, termtrc, \
	sendcde, process, AES_DECRYPT(UNHEX(outcdno),'abcdefgh'), trace, batchno, postermcde, posmercode, billmsg, trnsndp, msgtype ,termidm, \
	trancde, trnsndpid, card_sn, trnmsgd, trndate, trntime FROM manage_info where pepdate ='%s' and	 \
	peptime='%s' and process='%s' and trace='%ld' and termcde='%s' and mercode='%s';", i_pepdate,\
	i_peptime, i_process, i_trace, i_termcde, i_mercode);
    
    if(mysql_query(sock, sql)) 
	{
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "saveOrUpdateManageInfo 查找原笔交易出错 [%s]!", mysql_error(sock));

        //从连接池获取的连接需要释放掉
        if(GetIsoMultiThreadFlag())
        	SetFreeLink(sock);
        return -1;
    }
	
    MYSQL_RES *res; 
	MYSQL_ROW row ; 
	int num_fields;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) 
	{
        dcs_log(0, 0, "saveOrUpdateManageInfo:Couldn't get result from %s\n", mysql_error(sock));

        //从连接池获取的连接需要释放掉
        if(GetIsoMultiThreadFlag())
        	SetFreeLink(sock);
        return -1;
    }

    //从连接池获取的连接需要释放掉
    if(GetIsoMultiThreadFlag())
    	SetFreeLink(sock);
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res); 
	num_rows = mysql_num_rows(res);
    if (num_rows == 1) {
        memcpy(pep_jnls.pepdate, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls.pepdate));
        memcpy(pep_jnls.peptime, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls.peptime));
        memcpy(pep_jnls.aprespn, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls.aprespn));
        memcpy(pep_jnls.samid, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls.samid));
        memcpy(pep_jnls.authcode, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls.authcode));
        memcpy(pep_jnls.syseqno, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls.syseqno));
        memcpy(pep_jnls.mercode, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls.mercode));
        memcpy(pep_jnls.termcde, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls.termcde));
        memcpy(pep_jnls.termtrc, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls.termtrc));
        memcpy(pep_jnls.sendcde, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls.sendcde));
        memcpy(pep_jnls.process, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls.process));
        memcpy(pep_jnls.outcdno, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls.outcdno));
		if(row[12])
			pep_jnls.trace = atol(row[12]);
        memcpy(pep_jnls.batch_no, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls.batch_no));
        memcpy(pep_jnls.postermcde, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls.postermcde));
        memcpy(pep_jnls.posmercode, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls.posmercode));
        memcpy(pep_jnls.billmsg, row[16] ? row[16] : "\0", (int)sizeof(pep_jnls.billmsg));
        memcpy(pep_jnls.trnsndp, row[17] ? row[17] : "\0", (int)sizeof(pep_jnls.trnsndp));
        memcpy(pep_jnls.msgtype, row[18] ? row[18] : "\0", (int)sizeof(pep_jnls.msgtype));
        memcpy(pep_jnls.termidm, row[19] ? row[19] : "\0", (int)sizeof(pep_jnls.termidm));
        memcpy(pep_jnls.trancde, row[20] ? row[20] : "\0", (int)sizeof(pep_jnls.trancde));
		if(row[21])
			pep_jnls.trnsndpid = atoi(row[21]);
		memcpy(pep_jnls.card_sn, row[22] ? row[22] : "\0", (int)sizeof(pep_jnls.card_sn));
		memcpy(pep_jnls.trnmsgd, row[23] ? row[23] : "\0", (int)sizeof(pep_jnls.trnmsgd));
		memcpy(pep_jnls.trndate, row[24] ? row[24] : "\0", (int)sizeof(pep_jnls.trndate));
		memcpy(pep_jnls.trntime, row[25] ? row[25] : "\0", (int)sizeof(pep_jnls.trntime));
    }
    
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "saveOrUpdateManageInfo：没有找到符合条件的记录");
		mysql_free_result(res);  
		return -1;
	}
	else if(num_rows >1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "saveOrUpdateManageInfo：查到多条符合条件的记录");
		mysql_free_result(res);  
		return -1;
	}
    mysql_free_result(res);

	pep_jnls.samid[getstrlen(pep_jnls.samid)] = 0;
    
	#ifdef __TEST__
		dcs_log(0, 0, "pep_jnls.aprespn=[%s]", pep_jnls.aprespn);
		dcs_log(0, 0, "pep_jnls.samid=[%s]", pep_jnls.samid);
		dcs_log(0, 0, "pep_jnls.billmsg=[%s]", pep_jnls.billmsg);
		dcs_log(0, 0, "pep_jnls.trnsndp=[%s]", pep_jnls.trnsndp);
		dcs_log(0, 0, "pep_jnls.trnsndpid=[%d]", pep_jnls.trnsndpid);
		dcs_log(0, 0, "pep_jnls.trnmsgd=[%s]", pep_jnls.trnmsgd);
		dcs_log(0, 0, "pep_jnls.trndate=[%s]", pep_jnls.trndate);
		dcs_log(0, 0, "pep_jnls.trntime=[%s]", pep_jnls.trntime);
	#endif
	
	memcpy(opep_jnls, &pep_jnls, sizeof(PEP_JNLS));
	return 0;
}

//保存EWP上送的管理类信息
int Ewp_SavaManageInfo(ISO_data siso, PEP_JNLS pep_jnls, char *currentdate, char *currentTime, EWP_INFO ewp_info)
{
	char sql[2048], tmpbuf[27];
	
	memset(sql, 0x00, sizeof(sql));
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	
	memcpy(pep_jnls.pepdate, currentdate, 8);
	memcpy(pep_jnls.peptime, currentTime, 6);
	memcpy(pep_jnls.sendcde, ewp_info.consumer_sentinstitu, 4);
	memcpy(pep_jnls.sendcde+4, "0000", 4);
	
	if(getbit(&siso, 11, (unsigned char *)tmpbuf)<0)
	{
		 dcs_log(0, 0, "can not get bit_11!");	
	}
	sscanf(tmpbuf, "%06d", &pep_jnls.trace);
	
	if(getbit(&siso, 41, (unsigned char *)pep_jnls.termcde)<0)
	{
		 dcs_log(0, 0, "can not get bit_41!");	
	}
	
	if(getbit(&siso, 42, (unsigned char *)pep_jnls.mercode)<0)
	{
		 dcs_log(0, 0, "can not get bit_42!");	
	}
	#ifdef __TEST__
		dcs_log(0, 0, "pep_jnls.pepdate=[%s]", pep_jnls.pepdate);
		dcs_log(0, 0, "pep_jnls.peptime=[%s]", pep_jnls.peptime);
		dcs_log(0, 0, "pep_jnls.trnsndpid=[%d]", pep_jnls.trnsndpid);
		dcs_log(0, 0, "pep_jnls.sendcde=[%s]", pep_jnls.sendcde);
		dcs_log(0, 0, "pep_jnls.aprespn=[%s]", pep_jnls.aprespn);
		dcs_log(0, 0, "pep_jnls.trnsndp=[%s]", pep_jnls.trnsndp);
		dcs_log(0, 0, "pep_jnls.trace=[%d]", pep_jnls.trace);
		dcs_log(0, 0, "pep_jnls.mercode=[%s]", pep_jnls.mercode);
		dcs_log(0, 0, "pep_jnls.termcde=[%s]", pep_jnls.termcde);
		dcs_log(0, 0, "pep_jnls.tc_value=[%s]", pep_jnls.tc_value);
		dcs_log(0, 0, "ewp_info.consumer_transtype=[%s]", ewp_info.consumer_transtype);
		dcs_log(0, 0, "ewp_info.consumer_phonesnno=[%s]", ewp_info.consumer_phonesnno);
		dcs_log(0, 0, "ewp_info.consumer_username=[%s]", ewp_info.consumer_username);
		dcs_log(0, 0, "ewp_info.consumer_sentdate=[%s]", ewp_info.consumer_sentdate);
		dcs_log(0, 0, "ewp_info.consumer_senttime=[%s]", ewp_info.consumer_senttime);
		dcs_log(0, 0, "ewp_info.filed55=[%s]", ewp_info.filed55);
		dcs_log(0, 0, "ewp_info.cardseq=[%s]", ewp_info.cardseq);
		dcs_log(0, 0, "ewp_info.netmanagecode=[%s]", ewp_info.netmanagecode);
		dcs_log(0, 0, "ewp_info.consumer_transdealcode=[%s]", ewp_info.consumer_transdealcode);
		dcs_log(0, 0, "ewp_info.consumer_transcode=[%s]", ewp_info.consumer_transcode);
		dcs_log(0, 0, "ewp_info.consumer_transcode=[%s]", ewp_info.consumer_transcode);
		dcs_log(0, 0, "ewp_info.consumer_sysreferno=[%s]", ewp_info.consumer_sysreferno);
		
	#endif
	sprintf(sql, "INSERT INTO manage_info(pepdate, peptime, trancde, outcdno, trace, sendcde, syseqno, termcde, mercode, \
	termidm, aprespn, field55, card_sn, tc_value, netmanage_code, msgtype, process, trnsndpid, trnsndp, trndate, trntime, billmsg, samid) \
	VALUES('%s', '%s', '%s', HEX(AES_ENCRYPT('%s','abcdefgh')), '%d', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%s', '%s', '%s', '%s', '%s')", \
	pep_jnls.pepdate, pep_jnls.peptime, ewp_info.consumer_transcode, ewp_info.consumer_cardno, pep_jnls.trace, pep_jnls.sendcde, \
	ewp_info.consumer_sysreferno, pep_jnls.termcde, pep_jnls.mercode, ewp_info.consumer_phonesnno, pep_jnls.aprespn, ewp_info.filed55, \
	ewp_info.cardseq, pep_jnls.tc_value, ewp_info.netmanagecode, ewp_info.consumer_transtype, ewp_info.consumer_transdealcode, pep_jnls.trnsndpid, \
	pep_jnls.trnsndp, ewp_info.consumer_sentdate, ewp_info.consumer_senttime, ewp_info.consumer_orderno, ewp_info.consumer_psamno);
	if(mysql_query(sock,sql)) 
	{
	    #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "insert manage_info DB error [%s]!", mysql_error(sock));
	    return -1;
    }
	return 0;
}

/*
受理一体机上送的消费撤销、退货交易
查询原笔交易信息
*/
int GetOrginMerTransInfo(EWP_INFO ewp_info, PEP_JNLS *pep_jnls, char *currentdate)
{
	long i_trace = 0;
	char sql[1024];
	int tmp_len1 = 0, tmp_len2 = 0;
	
	memset(sql, 0x00, sizeof(sql));
	
	i_trace = atol(ewp_info.consumer_presyswaterno);
	
	#ifdef __LOG__
		dcs_log(0, 0, "查询流水=[%ld]", i_trace);
		dcs_log(0, 0, "查询订单号=[%s]", ewp_info.consumer_orderno);
		dcs_log(0, 0, "查询参考号=[%s]", ewp_info.consumer_sysreferno);
		dcs_log(0, 0, "查询原交易日期=[%s]", ewp_info.pretranstime);
		dcs_log(0, 0, "查询授权码=[%s]", ewp_info.authcode);
	#endif
	
	if(memcmp(ewp_info.consumer_transdealcode, "200000", 6) == 0)
	{
		dcs_log(0, 0, "撤销查询原笔");
		sprintf(sql, "SELECT tranamt, billmsg, termcde, mercode, sendcde, revdate, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), trace, syseqno, lpad(coalesce(authcode, 0), '6', '0'), \
		pepdate, peptime  FROM pep_jnls \
		where  trace = %ld and billmsg = '%s' and syseqno = '%s' and CONCAT(pepdate,peptime) ='%s' and aprespn = '00';", \
		i_trace, ewp_info.consumer_orderno, ewp_info.consumer_sysreferno, ewp_info.pretranstime); 
	}
	else
	{
		dcs_log(0, 0, "退货查询原笔");
		sprintf(sql, "SELECT tranamt, billmsg, termcde, mercode, sendcde, revdate, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), trace, syseqno, lpad(coalesce(authcode, 0), '6', '0'), \
		pepdate, peptime  FROM pep_jnls  where  trace = %ld and billmsg = '%s' and syseqno = '%s' and CONCAT(pepdate,peptime) ='%s' and aprespn = '00' \
		UNION ALL  SELECT tranamt, billmsg, termcde, mercode, sendcde, revdate, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), trace, syseqno, lpad(coalesce(authcode, 0), '6', '0'), \
		pepdate, peptime  FROM pep_jnls_all  where  trace = %ld and billmsg = '%s' and syseqno = '%s' and CONCAT(pepdate,peptime) ='%s' and aprespn = '00';", \
		i_trace, ewp_info.consumer_orderno, ewp_info.consumer_sysreferno, ewp_info.pretranstime, i_trace, \
		ewp_info.consumer_orderno, ewp_info.consumer_sysreferno, ewp_info.pretranstime); 
		
	}
	if(mysql_query(sock, sql)) 
	{
        #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "GetOrginMerTransInfo error = [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res; 
	MYSQL_ROW row ; 
	int num_fields;
	int num_rows = 0;
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetOrginMerTransInfo:Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
    if(num_rows == 1) 
	{
		memcpy(pep_jnls->tranamt, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls->tranamt));
		memcpy(pep_jnls->billmsg, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->billmsg));
		memcpy(pep_jnls->termcde, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls->termcde));
		memcpy(pep_jnls->mercode, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->mercode));
		memcpy(pep_jnls->sendcde, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls->sendcde));
		memcpy(pep_jnls->revdate, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls->revdate));
		memcpy(pep_jnls->outcdno, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls->outcdno));
		if(row[7])
			pep_jnls->trace = atol(row[7]);
		memcpy(pep_jnls->syseqno, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls->syseqno));
        memcpy(pep_jnls->authcode, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls->authcode));
		memcpy(pep_jnls->pepdate, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls->pepdate));
        memcpy(pep_jnls->peptime, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls->peptime));
    }
	else if(num_rows==0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOrginMerTransInfo：未找到原笔记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows >1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetOrginMerTransInfo：找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);
	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls->tranamt=[%s]", pep_jnls->tranamt);
		dcs_log(0, 0, "pep_jnls->billmsg=[%s]", pep_jnls->billmsg);
		dcs_log(0, 0, "pep_jnls->termcde=[%s]", pep_jnls->termcde);
		dcs_log(0, 0, "pep_jnls->mercode=[%s]", pep_jnls->mercode);
		dcs_log(0, 0, "pep_jnls->sendcde=[%s]", pep_jnls->sendcde);
		dcs_log(0, 0, "pep_jnls->revdate=[%s]", pep_jnls->revdate);
//		dcs_log(0, 0, "pep_jnls->outcdno=[%s]", pep_jnls->outcdno);
		dcs_log(0, 0, "pep_jnls->trace=[%ld]", pep_jnls->trace);
		dcs_log(0, 0, "pep_jnls->syseqno=[%s]", pep_jnls->syseqno);
		dcs_log(0, 0, "pep_jnls->authcode=[%s]", pep_jnls->authcode);
		dcs_log(0, 0, "pep_jnls->pepdate=[%s]", pep_jnls->pepdate);
		dcs_log(0, 0, "pep_jnls->peptime=[%s]", pep_jnls->peptime);
	#endif
	/*卡号判断*/
	tmp_len1 = getstrlen(ewp_info.consumer_cardno);
	tmp_len2 = getstrlen(pep_jnls->outcdno);
	if(tmp_len1!=tmp_len2 || (tmp_len1==tmp_len2 && memcmp(ewp_info.consumer_cardno, pep_jnls->outcdno, tmp_len2)!=0))
	{
		dcs_log(0, 0, "卡号不匹配");
		return -1;
	}
		
	/*授权码判断*/
	if(memcmp(ewp_info.authcode, "000000", 6)!= 0 && memcmp(ewp_info.authcode, pep_jnls->authcode, 6)!= 0)
	{
		dcs_log(0, 0, "授权码不匹配");
		return -1;
	}
	if(memcmp(ewp_info.consumer_transdealcode, "200000", 6) == 0)
	{
		if(memcmp(currentdate, ewp_info.pretranstime, 8)!=0)
		{
			dcs_log(0, 0, "撤销查询原笔时间不匹配");
			return -1;
		}
	}
	else
	{
		if(memcmp(pep_jnls->tranamt, ewp_info.consumer_money, 12) <0)
		{
			dcs_log(0, 0, "退货金额不合法");
			return -1;
		}
	}
	return 0;
}

//保存二维码信息
int SaveTwoInfo(TWO_CODE_INFO two_code_info)
{
	char sql[1024];
	memset(sql, 0x00, sizeof(sql));
	
	#ifdef __TEST__
		dcs_log(0, 0, "two_code_info.ordid=[%s]", two_code_info.ordid);
		dcs_log(0, 0, "two_code_info.amt=[%s]", two_code_info.amt);
		dcs_log(0, 0, "two_code_info.orddesc=[%s]", two_code_info.orddesc);
		dcs_log(0, 0, "two_code_info.ordtime=[%s]", two_code_info.ordtime);
		dcs_log(0, 0, "two_code_info.hashcode=[%s]", two_code_info.hashcode);	
	#endif
	sprintf(sql, "INSERT INTO two_code_info(ordid, amt, orddesc, ordtime, hashcode) VALUES('%s', '%s', '%s', '%s', '%s')", \
	two_code_info.ordid, two_code_info.amt, two_code_info.orddesc, two_code_info.ordtime, two_code_info.hashcode);
	if(mysql_query(sock,sql)) 
	{
	    #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "insert two_code_info DB error [%s]!", mysql_error(sock));
	    return -1;
    }
	return 0;	
}

//获取二维码信息
int GetTwoInfo(TWO_CODE_INFO *two_code_info, char *billmsg)
{
	char sql[512];
	
	memset(sql, 0x00, sizeof(sql));
	
	dcs_log(0, 0, "查询订单号=[%s]", billmsg);
	
	//sprintf(sql, "SELECT ordid, amt, ordtime FROM two_code_info WHERE ordid = '%s' ;",  "000000000141");
	sprintf(sql, "SELECT ordid, amt, ordtime FROM two_code_info WHERE ordid = '%s' ;",  billmsg);

	if(mysql_query(sock,sql)) {
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "select from two_code_info error [%s]!", mysql_error(sock));
		return -1;
	}
	
	MYSQL_RES *res;
	MYSQL_ROW row ;
	int num_fields;
	int num_rows = 0;

	if (!(res = mysql_store_result(sock))) 
	{
		dcs_log(0, 0, "select from two_code_info error: Couldn't get result from %s\n", mysql_error(sock));
		return -1;
	}

	num_fields = mysql_num_fields(res);
	row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
	if(num_rows == 1) 
	{
		memcpy(two_code_info->ordid, row[0] ? row[0] : "\0", sizeof(two_code_info->ordid));
		memcpy(two_code_info->amt, row[1] ? row[1] : "\0", sizeof(two_code_info->amt));
		memcpy(two_code_info->ordtime, row[2] ? row[2] : "\0", sizeof(two_code_info->ordtime));
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetTwoInfo:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "GetTwoInfo:找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
#ifdef __TEST__
	dcs_log(0,0,"ordid=[%s]", two_code_info->ordid);
	dcs_log(0,0,"amt=[%s]", two_code_info->amt);
	dcs_log(0,0,"ordtime=[%s]", two_code_info->ordtime);
#endif

	mysql_free_result(res);
	return 0;	
	
}

/*
 参数：
输入  cardno：银行卡号
返回  bankNo:银行机构号
*/
int ReadCardBankNo(char* cardno,char * bankNo)
{
	char cdhd5[5+1];
	char cdhd6[6+1];
	char cdhd7[7+1];
	char cdhd8[8+1];
	char cdhd9[9+1];
	char cdhd10[10+1];
	char cdhd11[11+1];
	char cdhd12[12+1];
	int  len ;

	memset(cdhd5, 0, sizeof(cdhd5));
    memset(cdhd6, 0, sizeof(cdhd6));
    memset(cdhd7, 0, sizeof(cdhd7));
    memset(cdhd8, 0, sizeof(cdhd8));
    memset(cdhd9, 0, sizeof(cdhd9));
    memset(cdhd10, 0, sizeof(cdhd10));
    memset(cdhd11, 0, sizeof(cdhd11));
    memset(cdhd12, 0, sizeof(cdhd12));

	memcpy(cdhd5, cardno, 5);
    memcpy(cdhd6, cardno, 6);
    memcpy(cdhd7, cardno, 7);
    memcpy(cdhd8, cardno, 8);
    memcpy(cdhd9, cardno, 9);
    memcpy(cdhd10, cardno, 10);
    memcpy(cdhd11, cardno, 11);
    memcpy(cdhd12, cardno, 12);

    len = getstrlen(cardno);
    MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
    char sql[512];
    memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "SELECT card_ins_code FROM card_transfer where card_head IN('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s') \
	and  card_no_length = %d order by card_length desc limit 1;", cdhd5, cdhd6, cdhd7, cdhd8, cdhd9, cdhd10, cdhd11, cdhd12, len);

    if(mysql_query(sock, sql)) {
      #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
        dcs_log(0, 0, "icheck card_transfer fail [%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
	    if(GetIsoMultiThreadFlag())
	    	SetFreeLink(sock);
        return -1;
    }
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;

    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "icheck card_transfer : Couldn't get result from %s\n", mysql_error(sock));
        //从连接池获取的连接需要释放掉
        if(GetIsoMultiThreadFlag())
        	SetFreeLink(sock);
        return -1;
    }
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
	//从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
    if (num_rows == 1) {
#ifdef __LOG__
    	dcs_log(0, 0, "bankNo ======= %s", row[0]);
#endif
        memcpy(bankNo, row[0] ? row[0] : "\0", strlen(row[0]));
        mysql_free_result(res);
        return 0;
    }
	else if(num_rows == 0)
	{
		dcs_log(0, 0, "ReadCardBankNo：没有找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows >1)
	{
		dcs_log(0, 0, "ReadCardBankNo：查到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}

    return 0;
}

/*
函数功能：查询EWP终端成功的脱机消费交易信息
返回值：-1： 处理失败   返回96
      3： 找到已发请求的记录
      0：未找到记录
*/
int getEwpOriTuoInfo(PEP_JNLS *pep_jnls,EWP_INFO ewp_info)
{
	char sql[1024];
	memset(sql, 0, sizeof(sql));

	dcs_log(0, 0, "脱机消费查询是否发起原笔");

	sprintf(sql, "SELECT aprespn, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), syseqno, termcde, mercode, trace, process, batchno, poscode, pepdate, peptime, poitcde\
	FROM pep_jnls  where outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and termid = '%s' and trndate = '%s' and trancde = '211' and billmsg = '%s';",
	ewp_info.consumer_cardno, ewp_info.consumer_username, pep_jnls->trndate, ewp_info.consumer_orderno);
	if(mysql_query(sock, sql)) {
        dcs_log(0, 0, "getEwpOriTuoInfo error=[%s]!", mysql_error(sock));
        return -1;
    }

    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;

    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "getEwpOriTuoInfo : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }

    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows== 1) {
		memcpy(pep_jnls->aprespn, row[0] ? row[0] : "\0", (int)sizeof(pep_jnls->aprespn));
		memcpy(pep_jnls->outcdno, row[1] ? row[1] : "\0", (int)sizeof(pep_jnls->outcdno));
		memcpy(pep_jnls->syseqno, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls->syseqno));
		memcpy(pep_jnls->termcde, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->termcde));
		memcpy(pep_jnls->mercode, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls->mercode));
		if(row[5])
			pep_jnls->trace =atoi(row[5]);
		memcpy(pep_jnls->process, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls->process));
		memcpy(pep_jnls->batch_no, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls->batch_no));
		memcpy(pep_jnls->poscode, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls->poscode));
		memcpy(pep_jnls->pepdate, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls->pepdate));
		memcpy(pep_jnls->peptime, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls->peptime));
		memcpy(pep_jnls->poitcde, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls->poitcde));
	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls->aprespn = [%s]", pep_jnls->aprespn);
//		dcs_log(0, 0, "pep_jnls->outcdno = [%s]", pep_jnls->outcdno);
		dcs_log(0, 0, "pep_jnls->syseqno = [%s]", pep_jnls->syseqno);
		dcs_log(0, 0, "pep_jnls->termcde =[%s]", pep_jnls->termcde);
		dcs_log(0, 0, "pep_jnls->mercode = [%s]", pep_jnls->mercode);
		dcs_log(0, 0, "pep_jnls->trace = [%d]", pep_jnls->trace);
		dcs_log(0, 0, "pep_jnls->process = [%s]", pep_jnls->process);
		dcs_log(0, 0, "pep_jnls->batch_no = [%s]", pep_jnls->batch_no);
		dcs_log(0, 0, "pep_jnls->poscode = [%s]", pep_jnls->poscode);
		dcs_log(0, 0, "pep_jnls->pepdate = [%s]", pep_jnls->pepdate);
		dcs_log(0, 0, "pep_jnls->peptime = [%s]", pep_jnls->peptime);
	#endif

	}
	else if(num_rows==0)
	{
		#ifdef __TEST__
	    dcs_log(0, 0, "%s", sql);
	  #endif
		dcs_log(0, 0, "getEwpOriTuoInfo 未找到符合条件的记录");
		mysql_free_result(res);
		return 0;
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	   #endif
		dcs_log(0, 0, "getEwpOriTuoInfo 找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);

	return 3;
}

/*
 根据应答码和交易码查询数据表xpep_retcode得到应答码的详细描述信息
 返回描述信息的长度len
 */
int GetAprespnDesc(char *rtnCode, char  *trancde, char *content)
{
   	char i_content[120+1];
	char sql[512];
	int len = 0;

	memset(i_content, 0, sizeof(i_content));
   	memset(sql, 0x00, sizeof(sql));
#ifdef __LOG__
   	dcs_log(0, 0, "rtnCode = [%s]", rtnCode);
   	dcs_log(0, 0, "trancde = [%s]", trancde);
#endif
	
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	sprintf(sql, "SELECT xpep_content FROM xpep_retcode WHERE xpep_retcode = '%s' and xpep_trancde = '%s';", rtnCode, trancde);
    if(mysql_query(sock,sql)) 
	{
	  #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	  dcs_log(0, 0, "GetAprespnDesc error1 [%s], trancde = %s!", mysql_error(sock), trancde);
	    //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
	    return -1;
    }
    
    MYSQL_RES *res;
   	MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    
    if (!(res = mysql_store_result(sock))) {
	    dcs_log(0, 0, "GetAprespnDesc:1 Couldn't get result from %s\n", mysql_error(sock));
	    //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
	    return -1;
    }
   
   	num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) 
    {	
       	memcpy(i_content, row[0] ? row[0] : "\0", (int)sizeof(i_content));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
    }
    else if(num_rows == 0)
	{
		#ifdef __LOG__
	    	dcs_log(0, 0, "%s", sql);
	  	#endif
		dcs_log(0, 0, "GetAprespnDesc:未找到符合条件的记录,再查找交易码是FFF");
		memset(sql, 0x00, sizeof(sql));
	   	sprintf(sql, "SELECT xpep_content FROM xpep_retcode  WHERE xpep_retcode = '%s' and xpep_trancde = 'FFF';", rtnCode);
		
	    if(mysql_query(sock, sql)) 
		{
		   	#ifdef __LOG__
		    	dcs_log(0, 0, "%s", sql);
		 	#endif
		    dcs_log(0, 0, "GetAprespnDesc error2 [%s], trancde = FFF!", mysql_error(sock));
		   //从连接池获取的连接需要释放掉
			if(GetIsoMultiThreadFlag())
				SetFreeLink(sock);
		    return -1;
	 	}
	    
		MYSQL_RES *res2 = NULL;
		num_fields = 0;
		num_rows = 0;
		if (!(res2 = mysql_store_result(sock))) 
		{
			dcs_log(0, 0, "GetAprespnDesc: Couldn't get result from %s\n", mysql_error(sock));
			//从连接池获取的连接需要释放掉
			if(GetIsoMultiThreadFlag())
				SetFreeLink(sock);
			return -1;
		}
		//从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		num_fields = mysql_num_fields(res2);
		row = mysql_fetch_row(res2);
		num_rows = mysql_num_rows(res2);
		if(num_rows == 1) 
		{
		    memcpy(i_content, row[0] ? row[0] : "\0", (int)sizeof(i_content));
		    mysql_free_result(res2);
		}
		else if(num_rows == 0)
		{
		#ifdef __LOG__
		   	dcs_log(0, 0, "%s", sql);
		#endif
			dcs_log(0, 0, "GetAprespnDesc:FFF未找到符合条件的记录");
			mysql_free_result(res2);
			return -1;
		}
		else if(num_rows > 1)
		{
			#ifdef __LOG__
		    	dcs_log(0, 0, "%s", sql);
		  	#endif
			dcs_log(0, 0, "GetAprespnDesc:FFF找到多条符合条件的记录");
			mysql_free_result(res2);
			return -1;
		}
	}
	else if(num_rows > 1)
	{
		#ifdef __LOG__
	    	dcs_log(0, 0, "%s", sql);
	  	#endif
		dcs_log(0, 0, "GetAprespnDesc:找到多条符合条件的记录");
		 //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		mysql_free_result(res);
		return -1;
	}
  	mysql_free_result(res);
 
	memcpy(content, i_content, getstrlen(i_content));
	len = getstrlen(content);
	
	#ifdef __LOG__
		dcs_log(0, 0, "content = [%s], len = [%d]", content, len);
	#endif
	//从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
	return len;
}

//更新扣款相关信息
int updateSaleAprespn(PEP_JNLS pep_jnls, TRANSFER_INFO *transfer_info)
{    
	int	rtn = 0;
	char sql[1024];
	char aprespn_desc[120+1];
	
	memset(sql, 0x00, sizeof(sql));
	memset(aprespn_desc, 0x00, sizeof(aprespn_desc));

	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "SELECT mercode, billmsg, pay_amt, transfer_type FROM transfer_info where pepdate ='%s' and	 \
	peptime='%s' and trace='%ld' and termcde='%s' and mercode='%s' and billmsg ='%s';", pep_jnls.pepdate,\
	pep_jnls.peptime, pep_jnls.trace, pep_jnls.termcde, pep_jnls.mercode, pep_jnls.billmsg);
	if(mysql_query(sock, sql)) 
	{
	  #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "updateSaleAprespn 查找原笔交易出错 [%s]!", mysql_error(sock));
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
	    dcs_log(0, 0, "updateSaleAprespn:Couldn't get result from %s\n", mysql_error(sock));
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
		memcpy(transfer_info->accountId, row[0] ? row[0] : "\0", (int)sizeof(transfer_info->accountId));
		memcpy(transfer_info->orderId, row[1] ? row[1] : "\0", (int)sizeof(transfer_info->orderId));
		memcpy(transfer_info->transAmt, row[2] ? row[2] : "\0", (int)sizeof(transfer_info->transAmt));
		memcpy(transfer_info->transfer_type, row[3] ? row[3] : "\0", (int)sizeof(transfer_info->transfer_type));
	}
	else if(num_rows == 0)
	{
		#ifdef __LOG__
	    	dcs_log(0, 0, "%s", sql);
	 	#endif
		dcs_log(0, 0, "updateSaleAprespn：没有找到符合条件的记录");
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
		dcs_log(0, 0, "updateSaleAprespn：查到多条符合条件的记录");
		mysql_free_result(res);
		//从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return -1;
	}
	mysql_free_result(res);
    memcpy(transfer_info->purpose, "转账汇款代付请求", strlen("转账汇款代付请求"));
	memcpy(transfer_info->bgRetUrl, "192.168.20.176:9915", strlen("192.168.20.176:9915"));
#ifdef __LOG__
	dcs_log(0, 0, "transfer_info->accountId=[%s]", transfer_info->accountId);
	dcs_log(0, 0, "transfer_info->orderId=[%s]", transfer_info->orderId);
	dcs_log(0, 0, "transfer_info->transAmt=[%s]", transfer_info->transAmt);
	dcs_log(0, 0, "transfer_info->purpose=[%s]", transfer_info->purpose);
	dcs_log(0, 0, "transfer_info->bgRetUrl=[%s]", transfer_info->bgRetUrl);
	dcs_log(0, 0, "transfer_info->transfer_type=[%s]", transfer_info->transfer_type);	
#endif
	
	rtn = GetAprespnDesc(pep_jnls.aprespn, pep_jnls.trancde, aprespn_desc);
	if(rtn < 0)
		dcs_log(0, 0, "UPDATE transfer_info 获取应答码描述信息失败");
	dcs_log(0, 0, "aprespn_desc =[%s]", aprespn_desc);

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "UPDATE transfer_info set sale_respn='%s', sale_respn_desc = '%s', authcode='%s', \
	sale_rcvdate ='%s', syseqno ='%s' where pepdate ='%s' and peptime='%s' and billmsg ='%s' \
	and trace='%ld' and termcde='%s' and mercode='%s';", pep_jnls.aprespn, aprespn_desc, pep_jnls.authcode, \
	pep_jnls.revdate, pep_jnls.syseqno, pep_jnls.pepdate, pep_jnls.peptime, pep_jnls.billmsg, pep_jnls.trace, pep_jnls.termcde, pep_jnls.mercode);
	if(mysql_query(sock, sql)) 
	{
	  #ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	  #endif
	    dcs_log(0, 0, "UPDATE transfer_info error [%s]!", mysql_error(sock));
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
//更新冲正标识，更新pepJnls和tannsfer_info中的冲正标识字段
int updateReverFlg(ISO_data iso, PEP_JNLS pep_jnls)
{
	char  sql[1024];
	char i_curdate[8+1];
	char o_curdate[8+1];
	char i_aprespn_desc[128];
	int rtn = 0;
	
	struct tm *tm;   
	time_t lastTime;
	
	memset(sql, 0x00, sizeof(sql));
	memset(i_curdate, 0x00, sizeof(i_curdate));
	memset(o_curdate, 0x00, sizeof(o_curdate));
	memset(i_aprespn_desc, 0x00, sizeof(i_aprespn_desc));
	
	  /*昨天*/
    lastTime = time(NULL) - 86400;
	tm = localtime(&lastTime);
	sprintf(o_curdate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	/*今天*/
	time(&lastTime);
	tm = localtime(&lastTime);
	sprintf(i_curdate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	
	rtn = GetAprespnDesc(pep_jnls.aprespn, pep_jnls.trancde, i_aprespn_desc);
	if(rtn < 0)
		dcs_log(0, 0, "updateReverFlg 获取应答码描述信息失败");

	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "UPDATE pep_jnls set reversedflag ='1', reversedans ='%s' where (pepdate= '%s' or pepdate = '%s') and trace=%ld and termcde= '%s' \
	and mercode= '%s' and tranamt= '%s' and billmsg ='%s' and msgtype ='0200';",\
	pep_jnls.aprespn, i_curdate, o_curdate, pep_jnls.trace, pep_jnls.termcde, pep_jnls.mercode, pep_jnls.tranamt, pep_jnls.billmsg);
	
	if(mysql_query(sock, sql)) 
	{
        #ifdef __LOG__
	    	dcs_log(0, 0, "%s", sql);
	  	#endif
        dcs_log(0, 0, "updateReverFlg error [%s]!", mysql_error(sock));
        //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }	
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "UPDATE transfer_info set reversalflag ='1', reversal_respn ='%s', reversal_respn_desc='%s' where (pepdate= '%s' or pepdate = '%s') and trace=%ld and termcde= '%s' \
	and mercode= '%s' and tranamt= '%s' and billmsg ='%s';",\
	pep_jnls.aprespn, i_aprespn_desc, i_curdate, o_curdate, pep_jnls.trace, pep_jnls.termcde, pep_jnls.mercode, pep_jnls.tranamt, pep_jnls.billmsg);
	
	if(mysql_query(sock, sql)) 
	{
        #ifdef __LOG__
	    	dcs_log(0, 0, "%s", sql);
	  	#endif
        dcs_log(0, 0, "updateReverFlg error [%s]!", mysql_error(sock));
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

/*
	转账汇款扣款成功之后，代付失败，发起冲正，更新转账汇款表冲正有关字段
*/
int updateReversal(ISO_data iso, PEP_JNLS pep_jnls)
{
	char i_revdate[10+1];
	char i_aprespn[2+1];
	char i_syseqno[12+1];
	char i_transamt[12+1];
	char i_aprespn_desc[120+1];
	char tmpbuf[127], sql[1024];

	int s = 0, rtn = 0;
	
	memset(i_aprespn_desc, 0, sizeof(i_aprespn_desc));
	memset(i_aprespn, 0, sizeof(i_aprespn));
	memset(i_syseqno, 0, sizeof(i_syseqno));
	memset(i_transamt, 0, sizeof(i_transamt));
    memset(tmpbuf, 0, sizeof(tmpbuf));
    memset(i_revdate, 0, sizeof(i_revdate));
	memset(sql, 0x00, sizeof(sql));
	
    /*i_revdate从核心返回的信息中12域和13域中取得*/
	s = getbit(&iso, 13, (unsigned char *)i_revdate);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_13 error,ERROR应答");
		return -1;
	}
	
	s = getbit(&iso, 12, (unsigned char *)i_revdate+4);
	if(s<0)
	{
#ifdef __LOG__
		dcs_log(0, 0, "get bit_12 error,ERROR应答");
#endif
		return -1;
	}
	
	getbit(&iso, 37, (unsigned char *)i_syseqno);
	getbit(&iso, 39, (unsigned char *)i_aprespn);
	getbit(&iso, 4, (unsigned char *)i_transamt);
    
	rtn = GetAprespnDesc(i_aprespn, pep_jnls.trancde, i_aprespn_desc);
	if(rtn < 0)
		dcs_log(0, 0, "updateReversal 获取应答码描述信息失败");
	
    #ifdef __LOG__
  		dcs_log(0, 0, "i_revdate=[%s]", i_revdate);
		dcs_log(0, 0, "i_syseqno=[%s]", i_syseqno);
		dcs_log(0, 0, "i_aprespn=[%s]", i_aprespn);
		dcs_log(0, 0, "i_transamt=[%s]", i_transamt);
		dcs_log(0, 0, "i_aprespn_desc =[%s]", i_aprespn_desc);
  	#endif

	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "UPDATE transfer_info set reversalflag ='1', reversal_respn ='%s', reversal_respn_desc = '%s', reversal_syseqno= '%s', reversal_respn_date = '%s' \
	where trace=%ld and termcde= '%s' and mercode= '%s' and tranamt= '%s' and billmsg ='%s';",\
	i_aprespn, i_aprespn_desc, i_syseqno, i_revdate,\
	pep_jnls.trace, pep_jnls.termcde, pep_jnls.mercode, i_transamt, pep_jnls.billmsg);
	if(mysql_query(sock, sql)) 
	{
        #ifdef __LOG__
	    	dcs_log(0, 0, "%s", sql);
	  	#endif
        dcs_log(0, 0, "updateReversal error [%s]!", mysql_error(sock));
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
//更代付请求日期
int updatePayReqDate(TRANSFER_INFO transfer_info)
{    
	char sql[1024];
	char currentDate[15];
	struct tm *tm;
	time_t t;

    time(&t);
    tm = localtime(&t);
	
	memset(sql, 0x00, sizeof(sql));
	memset(currentDate, 0x00, sizeof(currentDate));
	
	sprintf(currentDate, "%04d%02d%02d%02d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	//更新记录
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	
	//更新转账汇款记录
    memset(sql, 0x00, sizeof(sql));
	if(transfer_info.transfer_type[0]=='0')
		sprintf(sql, "UPDATE transfer_info set pay_request_date='%s', pay_flag ='1', response_code='A6' where billmsg='%s';", currentDate, transfer_info.orderId);
	else
		sprintf(sql, "UPDATE transfer_info set pay_request_date='%s', pay_flag ='1' where billmsg='%s';", currentDate, transfer_info.orderId);

	if(mysql_query(sock, sql))
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
	    dcs_log(0, 0, "updatePayReqDate error [%s]!", mysql_error(sock));
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


//更新代付状态
int updatePayInfo(TRANSFER_INFO *transfer_info)
{
	char sql[1024];
	
	memset(sql, 0x00, sizeof(sql));

	//更新记录
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	//更新转账汇款记录
    memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "UPDATE transfer_info set pay_respn='%s', pay_respn_desc = '%s', pay_tseq='%s',\
	pay_respn_date ='%s%s' where billmsg='%s';", transfer_info->pay_respn, transfer_info->pay_respn_desc, \
	transfer_info->tseq, transfer_info->sysDate, transfer_info->sysTime, transfer_info->orderId);

	if(mysql_query(sock, sql))
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
	    dcs_log(0, 0, "updatePayInfo error [%s]!", mysql_error(sock));
	    //从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
	    return -1;
	}
	
	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "SELECT transfer_type, pepdate, peptime, trace, termcde, mercode, billmsg, fee, modecde from transfer_info where billmsg='%s';", transfer_info->orderId);
	if(mysql_query(sock, sql)) {
	#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
	    dcs_log(0, 0, "updatePayInfo 查找原笔交易出错 [%s]!", mysql_error(sock));
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
	    dcs_log(0, 0, "updatePayInfo:Couldn't get result from %s\n", mysql_error(sock));
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
		memcpy(transfer_info->transfer_type, row[0] ? row[0] : "\0", (int)sizeof(transfer_info->transfer_type));
		memcpy(transfer_info->pepdate, row[1] ? row[1] : "\0", (int)sizeof(transfer_info->pepdate));
		memcpy(transfer_info->peptime, row[2] ? row[2] : "\0", (int)sizeof(transfer_info->peptime));
		if(row[3])
			transfer_info->trace = atol(row[3]);
		memcpy(transfer_info->termcde, row[4] ? row[4] : "\0", (int)sizeof(transfer_info->termcde));
		memcpy(transfer_info->mercode, row[5] ? row[5] : "\0", (int)sizeof(transfer_info->mercode));
		memcpy(transfer_info->billmsg, row[6] ? row[6] : "\0", (int)sizeof(transfer_info->billmsg));
		memcpy(transfer_info->fee, row[7] ? row[7] : "\0", (int)sizeof(transfer_info->fee));
		memcpy(transfer_info->modecde, row[8] ? row[8] : "\0", (int)sizeof(transfer_info->modecde));
	}
	else if(num_rows == 0)
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
		dcs_log(0, 0, "updatePayInfo:没有找到符合条件的记录");
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
		dcs_log(0, 0, "updatePayInfo：查到多条符合条件的记录");
		mysql_free_result(res);
		//从连接池获取的连接需要释放掉
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return -1;
	}
	mysql_free_result(res);
#ifdef __LOG__
	dcs_log(0, 0, "transfer_info->transfer_type=[%s]", transfer_info->transfer_type);
	dcs_log(0, 0, "transfer_info->pepdate=[%s]", transfer_info->pepdate);
	dcs_log(0, 0, "transfer_info->peptime=[%s]", transfer_info->peptime);
	dcs_log(0, 0, "transfer_info->trace=[%ld]", transfer_info->trace);
	dcs_log(0, 0, "transfer_info->termcde=[%s]", transfer_info->termcde);
	dcs_log(0, 0, "transfer_info->mercode=[%s]", transfer_info->mercode);
	dcs_log(0, 0, "transfer_info->billmsg=[%s]", transfer_info->billmsg);
	dcs_log(0, 0, "transfer_info->fee=[%s]", transfer_info->fee);
	dcs_log(0, 0, "transfer_info->modecde=[%s]", transfer_info->modecde);
#endif

	//从连接池获取的连接需要释放掉
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
	return 0;
}

/*
 功能：通过psam卡号得到该渠道id
 输入参数：psamNo卡号，16字节长度
 输出参数：channel_id
 */
int GetChannelId(char *samNo)
{
	char sql[1024];
	int channel_id = 0;

    memset(sql, 0x00, sizeof(sql));
	#ifdef __LOG__
		dcs_log(0, 0, "samNo=[%s]", samNo);
	#endif
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
    sprintf(sql, "SELECT channel_id FROM samcard_info WHERE sam_card = '%s';", samNo);
	
    if(mysql_query(sock, sql)) {
        dcs_log(0, 0, "%s", sql);
        dcs_log(0, 0, "GetChannelId samcard_info DB error [%s]!", mysql_error(sock));
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }

    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
	int num_rows = 0;
    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetChannelId samcard_info : Couldn't get result from %s\n", mysql_error(sock));
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }

    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
	num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
        if(row[0])
			channel_id = atoi(row[0]);
    }
	 else if(num_rows == 0)
	{
		dcs_log(0, 0, "GetChannelId没有找到符合条件的记录");
		mysql_free_result(res);
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return -1;
	}
	else if(num_rows >1)
	{
		dcs_log(0, 0, "GetChannelId：查到多条符合条件的记录");
		mysql_free_result(res);
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return -1;
	}
    mysql_free_result(res);
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
	dcs_log(0, 0, "channel_id = [%d]", channel_id);
	return channel_id;
}


/*根据用户名查询数据表pos_cust_info得到代理机构编号等信息*/
int GetAgentInfo(char *username, char *psam, char *T0_flag, char *agents_code)
{
	char sql[1048];
	char i_username[10];
	char i_psam[17];

	memset(sql, 0x00, sizeof(sql));
	memset(i_username, 0x00, sizeof(i_username));
	memset(i_psam, 0x00, sizeof(i_psam));

	if(getstrlen(username)<=2)
		return -1;
	memcpy(i_username, username, getstrlen(username)-2);
	memcpy(i_psam, psam, getstrlen(psam));
	
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	sprintf(sql, "SELECT t0_flag, agents_code FROM pos_cust_info  \
		WHERE substr(cust_id,1,9) = '%s' and cust_psam = '%s';",i_username, i_psam);

	if(mysql_query(sock,sql)) {
        dcs_log(0, 0, "%s", sql);
        dcs_log(0, 0, "GetAgentInfo error=[%s]!", mysql_error(sock));
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }

    MYSQL_RES *res;
    MYSQL_ROW row ;
    memset(&row, 0, sizeof(MYSQL_ROW));
    int num_fields;
	int num_rows = 0;

    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetAgentInfo : Couldn't get result from %s\n", mysql_error(sock));
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
        return -1;
    }
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    num_rows = mysql_num_rows(res);
    if(num_rows == 1) {
		memcpy(T0_flag, row[0] ? row[0] : "\0", (int)sizeof(T0_flag));
		memcpy(agents_code, row[1] ? row[1] : "\0", (int)sizeof(agents_code));
    }
    else if(num_rows==0)
	{
		dcs_log(0, 0, "%s", sql);
		dcs_log(0, 0, "GetAgentInfo 未找到符合条件的记录");
		mysql_free_result(res);
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return -1;
	}
	else if(num_rows > 1)
	{
		dcs_log(0, 0, "%s", sql);
		dcs_log(0, 0, "GetAgentInfo找到多条符合条件的记录");
		mysql_free_result(res);
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return -1;
	}
 	mysql_free_result(res);
	if(GetIsoMultiThreadFlag())
		SetFreeLink(sock);
	#ifdef __LOG__
		dcs_log(0, 0, "T0_flag=[%s], agents_code=[%s]", T0_flag, agents_code);
	#endif
	return 0;
}

//保存冲正信息
int SaveReversalInfo(ISO_data iso, PEP_JNLS pep_jnls)
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
	
	//todo ..冲正交易码暂定为C00
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
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
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
	        dcs_log(0, 0, "SaveReversalInfo error [%s]!", mysql_error(sock));
			if(GetIsoMultiThreadFlag())
				SetFreeLink(sock);
	        return -1;
	    }
	    dcs_log(0, 0, "SaveReversalInfo success");
		if(GetIsoMultiThreadFlag())
			SetFreeLink(sock);
		return 1;
}


//更新返回终端返回码
int updateResponseCode(TRANSFER_INFO transfer_info)
{
	char sql[1024];
	
	memset(sql, 0x00, sizeof(sql));

	//更新记录
	MYSQL *sock = NULL;
	sock = GetMysqlLinkSock();
	if(sock ==  NULL)
	{
		dcs_log(0, 0, "获取MYSQL连接失败");
		return -1;
	}
	//更新转账汇款记录
    memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "UPDATE transfer_info set response_code='%s' where billmsg='%s';", transfer_info.pay_respn, transfer_info.orderId);
	if(mysql_query(sock, sql))
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "%s", sql);
	#endif
	    dcs_log(0, 0, "updateResponseCode error [%s]!", mysql_error(sock));
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
