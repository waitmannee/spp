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
extern MYSQL mysql, *sock; 

int HTDBConect()
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

int HTDBConectEnd(int iDssOperate)
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
 查询指定时间段内某个用户的历史交易记录
 返回EWP信息有应答码ANS2，总条数N3，当前返回条数N1，
 转出卡号ANS19，转入卡号ANS19，交易金额N12，交易手续费N12，
 交易日期和时间N14，交易流水N6，交易参考号N12，应答码ANS2，分隔符S1';'
 */
int GetQueryResult(EWP_HIST_RQ ewp_hist_rq, char *buffer)
{
    char 	i_trancde[3+1];
    char 	i_termid[25+1];
    char 	i_startdate[8+1];
    char 	i_enddate[8+1];
    char 	i_pepdate[8+1];
    char 	i_peptime[6+1];
    char 	i_outcdno[20+1];
    char	i_intcdno[40+1];
    char 	i_tranamt[12+1];
    char 	i_transfee[12+1];
    long	i_trace;
    char 	i_syseqno[12+1];
    char    i_aprespn[2+1];
    char 	i_billmsg[40+1];
    char 	i_sendcde[8+1];
	char tmpbuf[27];
	int i_len = 0;
	int numflg = 0;
	
	memset(i_trancde, 0, sizeof(i_trancde));
	memset(i_termid, 0, sizeof(i_termid));
	memset(i_startdate, 0, sizeof(i_startdate));
	memset(i_enddate, 0, sizeof(i_enddate));
	memset(i_sendcde, 0, sizeof(i_sendcde));
	memset(i_pepdate, 0, sizeof(i_pepdate));
	memset(i_peptime, 0, sizeof(i_peptime));
	memset(i_outcdno, 0, sizeof(i_outcdno));
	memset(i_intcdno, 0, sizeof(i_intcdno));
	memset(i_tranamt, 0, sizeof(i_tranamt));
	memset(i_syseqno, 0, sizeof(i_syseqno));
	memset(i_aprespn, 0, sizeof(i_aprespn));
	memset(i_billmsg, 0, sizeof(i_billmsg));
	memset(i_transfee, 0, sizeof(i_transfee));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	
	int perpagenum, total_page, temp_page;
	int minnum, maxnum, curpage, len, total_num;
	int rtn;
	
	memcpy(i_termid, ewp_hist_rq.username, 11);
	i_termid[11] = '\0';
	memcpy(i_startdate, ewp_hist_rq.startdate, 8);
	memcpy(i_enddate, ewp_hist_rq.enddate, 8);
	
	memcpy(i_sendcde, ewp_hist_rq.sendcode, 4);
	memcpy(i_sendcde+4, "0000", 4);
	i_sendcde[8] = 0;
	sscanf(ewp_hist_rq.perpagenum, "%d", &perpagenum);
	sscanf(ewp_hist_rq.curpage, "%03d", &curpage);
	
	i_len = getstrlen(ewp_hist_rq.querycdno);
	if(i_len !=0)
		ewp_hist_rq.querycdno[i_len]=0;
    
   	char sql[1024], trancde_temp[128];
	
   	memset(sql, 0x00, sizeof(sql));
	memset(trancde_temp, 0x00, sizeof(trancde_temp));
	
	if(memcmp(ewp_hist_rq.process, "FF0001", 6) == 0)
	{
		dcs_log(0, 0, "查询收款类相关的交易");
		//收款，收款撤销，收款退货，脱机消费，脱机退货
		#ifdef __TEST__
			sprintf(trancde_temp, "(trancde = '200' or trancde = '209' or trancde = '210' or trancde = '211' or trancde = '212') and ");
		#else
			sprintf(trancde_temp, "(trancde = '815' or trancde = '209' or trancde = '210' or trancde = '211' or trancde = '212') and ");
		#endif
		dcs_log(0, 0, "trancde_temp=[%s]", trancde_temp);
	}
	else if(memcmp(ewp_hist_rq.process, "FF0002", 6) == 0)
	{
		dcs_log(0, 0, "查询所有类型的交易");
		sprintf(trancde_temp, " ");
		dcs_log(0, 0, "trancde_temp=[%s]", trancde_temp);
	}
	else if(memcmp(ewp_hist_rq.process, "FF0000", 6) == 0)
	{
		dcs_log(0, 0, "查询某种类型的交易trancde =[%s]", ewp_hist_rq.trancde);
		sprintf(trancde_temp, "trancde = '%s' and ", ewp_hist_rq.trancde);
		dcs_log(0, 0, "trancde_temp=[%s]", trancde_temp);
		
	}
    
	if(memcmp(ewp_hist_rq.queryflag, "0", 1)==0&&i_len!=0)
	{
		dcs_log(0, 0, "ewp_hist_rq.queryflag01 = [%s]", ewp_hist_rq.queryflag);
        sprintf(sql, "select sum(tmp_count) from ( select count(*) as tmp_count from pep_jnls  WHERE %s \
		pepdate >= '%s' and pepdate <= '%s' and rtrim(termid) = '%s' and sendcde = '%s' and rtrim(outcdno) = HEX(AES_ENCRYPT('%s','abcdefgh')) \
		UNION ALL select count(*) as tmp_count from pep_jnls_all  WHERE %s pepdate >= '%s' and pepdate <= '%s'\
		and rtrim(termid) = '%s' and sendcde = '%s' and rtrim(outcdno) = HEX(AES_ENCRYPT('%s','abcdefgh'))) as T", trancde_temp, i_startdate, i_enddate, i_termid,\
		i_sendcde, ewp_hist_rq.querycdno, trancde_temp, i_startdate, i_enddate, i_termid, i_sendcde, ewp_hist_rq.querycdno);
        
	}
	else if(memcmp(ewp_hist_rq.queryflag, "1", 1)==0&&i_len!=0)
	{
		dcs_log(0, 0, "ewp_hist_rq.queryflag11 = [%s]", ewp_hist_rq.queryflag);

        sprintf(sql, "select sum(tmp_count) from ( select count(*) as tmp_count from pep_jnls  WHERE %s pepdate >= '%s'\
		and pepdate <= '%s' and rtrim(termid) = '%s' and sendcde = '%s' and rtrim(outcdno) = HEX(AES_ENCRYPT('%s','abcdefgh')) and aprespn ='00' UNION ALL\
		select count(*) as tmp_count from pep_jnls_all  WHERE %s pepdate >= '%s' and pepdate <= '%s' \
		and rtrim(termid) = '%s' and sendcde = '%s' and rtrim(outcdno) = HEX(AES_ENCRYPT('%s','abcdefgh')) and aprespn ='00') as T", trancde_temp, i_startdate,\
		i_enddate, i_termid, i_sendcde, ewp_hist_rq.querycdno, trancde_temp, i_startdate, i_enddate, i_termid, i_sendcde, ewp_hist_rq.querycdno);
        
	}
	else if(memcmp(ewp_hist_rq.queryflag, "2", 1)==0&&i_len!=0)
	{
		dcs_log(0, 0, "ewp_hist_rq.queryflag21 = [%s]", ewp_hist_rq.queryflag);
        sprintf(sql, "select sum(tmp_count) from ( select count(*) as tmp_count from pep_jnls  WHERE %s pepdate >= '%s'\
		and pepdate <= '%s' and rtrim(termid) = '%s' and sendcde = '%s' and rtrim(outcdno) = HEX(AES_ENCRYPT('%s','abcdefgh')) and aprespn != '00' UNION ALL\
		select count(*) as tmp_count from pep_jnls_all  WHERE %s pepdate >= '%s' and pepdate <= '%s' and \
		rtrim(termid) = '%s' and sendcde = '%s' and rtrim(outcdno) = HEX(AES_ENCRYPT('%s','abcdefgh')) and aprespn != '00') as T", trancde_temp, i_startdate, \
		i_enddate, i_termid, i_sendcde, ewp_hist_rq.querycdno, trancde_temp, i_startdate, i_enddate, i_termid, i_sendcde, ewp_hist_rq.querycdno);
	}
	else if(memcmp(ewp_hist_rq.queryflag, "0", 1)==0&&i_len==0)
	{
		dcs_log(0, 0, "ewp_hist_rq.queryflag00 = [%s]", ewp_hist_rq.queryflag);  
        sprintf(sql, "select sum(tmp_count)  from ( select count(*) as tmp_count from pep_jnls  WHERE %s pepdate >= '%s'\
		and pepdate <= '%s' and rtrim(termid) = '%s' and sendcde = '%s' UNION ALL  select count(*) as tmp_count from pep_jnls_all \
		WHERE %s pepdate >= '%s' and pepdate <= '%s' and rtrim(termid) = '%s' and sendcde = '%s') as T",\
		trancde_temp, i_startdate, i_enddate, i_termid, i_sendcde, trancde_temp, i_startdate,  i_enddate, i_termid, i_sendcde);
        
	}
	else if(memcmp(ewp_hist_rq.queryflag, "1", 1)==0&&i_len==0)
	{
		dcs_log(0, 0, "ewp_hist_rq.queryflag10 = [%s]", ewp_hist_rq.queryflag);
      
        sprintf(sql, "select sum(tmp_count) from ( select count(*) as tmp_count from pep_jnls  WHERE %s pepdate >= '%s'\
		and pepdate <= '%s' and rtrim(termid) = '%s' and sendcde = '%s' and aprespn ='00' UNION ALL select count(*) as tmp_count\
		from pep_jnls_all  WHERE %s pepdate >= '%s' and pepdate <= '%s' and rtrim(termid) = '%s' \
		and sendcde = '%s' and aprespn ='00') as T", trancde_temp, i_startdate, i_enddate, i_termid, i_sendcde, trancde_temp, i_startdate,\
		i_enddate, i_termid, i_sendcde);

	}
	else if(memcmp(ewp_hist_rq.queryflag, "2", 1)==0&&i_len==0)
	{
		dcs_log(0, 0, "ewp_hist_rq.queryflag20 = [%s]", ewp_hist_rq.queryflag);
   
        sprintf(sql, "select sum(tmp_count) from ( select count(*) as tmp_count from pep_jnls  WHERE %s pepdate >= '%s'\
		and pepdate <= '%s' and rtrim(termid) = '%s' and sendcde = '%s' and aprespn !='00' UNION ALL select count(*) as tmp_count\
		from pep_jnls_all WHERE %s pepdate >= '%s' and pepdate <= '%s' and rtrim(termid) = '%s' and sendcde = '%s'\
		and aprespn !='00') as T", trancde_temp, i_startdate, i_enddate, i_termid, i_sendcde, trancde_temp, i_startdate, i_enddate, i_termid, i_sendcde);

	}
    
    if(mysql_query(sock,sql)) {
        dcs_log(0, 0, "%s", sql);
        dcs_log(0, 0, "EWPHT_mysql: GetQueryResult fail [%s]!", mysql_error(sock));
        return -1;
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "EWPHT_mysql..GetQueryResult : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    
    if (mysql_num_rows(res) == 1) {
        total_num = atoi(row[0]);
        
    } else total_num = 0;
    
    mysql_free_result(res);
	
	#ifdef __LOG__
		dcs_log(0, 0, "total_num = [%d]", total_num);
	#endif
	len = 0;
	memcpy(buffer, "0210", 4);
	len +=4;
	memcpy(buffer+len, ewp_hist_rq.process, 6);
	len +=6;
	memcpy(buffer+len, ewp_hist_rq.sendcode, 4);
	len +=4;
	memcpy(buffer+len, ewp_hist_rq.trancde, 3);
	len +=3;
	memcpy(buffer+len, ewp_hist_rq.startdate, 8);
	len +=8;
	memcpy(buffer+len, ewp_hist_rq.enddate, 8);
	len +=8;
	memcpy(buffer+len, ewp_hist_rq.perpagenum, 1);
	len +=1;
	memcpy(buffer+len, ewp_hist_rq.curpage, 3);
	len +=3;
	memcpy(buffer+len, ewp_hist_rq.username, 11);
	len +=11;
	memcpy(buffer+len, ewp_hist_rq.queryflag, 1);
	len +=1;
	memcpy(buffer+len, ewp_hist_rq.querycdno, 19);
	len +=19;
	if(total_num == 0)
	{
		memcpy(ewp_hist_rq.aprespn, "H3", 2);
		memcpy(buffer+len, ewp_hist_rq.aprespn, 2);
		len +=2;
		memcpy(buffer+len, ewp_hist_rq.selfewp, 20);
		len +=20;
		#ifdef __LOG__
			dcs_log(0, 0, "buffer = [%s], len = [%d]", buffer, len);
		#endif
	}
	else
	{
		if((curpage-1)*perpagenum >= total_num)
		{
			memcpy(ewp_hist_rq.aprespn, "H3", 2);
			memcpy(buffer+len, ewp_hist_rq.aprespn, 2);
			len +=2;
			memcpy(buffer+len, ewp_hist_rq.selfewp, 20);
			len +=20;
			#ifdef __LOG__
				dcs_log(0, 0, "buffer = [%s], len = [%d]", buffer, len);
			#endif
			return len;
		}
		memcpy(ewp_hist_rq.aprespn, "00", 2);
		sprintf(ewp_hist_rq.tatalnu, "%03d", total_num);
		temp_page = total_num%perpagenum;
		minnum = (curpage-1)*perpagenum+1;
		if(temp_page == 0)
		{
			total_page = total_num/perpagenum;
			maxnum = curpage*perpagenum;
		}
		else
		{
			total_page = total_num/perpagenum + 1;
			if(total_page == curpage)
				maxnum = total_num;
			else
				maxnum = curpage*perpagenum;
		}
		sprintf(ewp_hist_rq.curnum, "%d", maxnum-minnum+1);
		#ifdef __LOG__
			dcs_log(0, 0, "maxnum = [%d], minnum = [%d]", maxnum, minnum);
			dcs_log(0, 0, "ewp_hist_rq.curnum = [%s]", ewp_hist_rq.curnum);
			dcs_log(0, 0, "ewp_hist_rq.tatalnu = [%s]", ewp_hist_rq.tatalnu);
			dcs_log(0, 0, "ewp_hist_rq.aprespn = [%s]", ewp_hist_rq.aprespn);
		#endif
		memcpy(buffer+len, ewp_hist_rq.aprespn, 2);
		len +=2;
		memcpy(buffer+len, ewp_hist_rq.tatalnu, 3);
		len +=3;
		memcpy(buffer+len, ewp_hist_rq.curnum, 1);
		len +=1;        
        
        memset(sql, 0x00, sizeof(sql));
        sprintf(sql, "select pepdate, peptime, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), AES_DECRYPT(UNHEX(intcdno), 'abcdefgh'), lpad(coalesce(tranamt, 0), '12', '0'), trace, lpad(coalesce(syseqno, 0), '12', '0'),\
		aprespn, billmsg, rtrim(filed28), trancde from  pep_jnls_all  where %s rtrim(termid) = '%s' and sendcde = '%s' and pepdate>= '%s' \
		and pepdate <= '%s'  UNION ALL select pepdate, peptime, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), AES_DECRYPT(UNHEX(intcdno), 'abcdefgh'), lpad(coalesce(tranamt, 0), '12', '0'), trace, \
		lpad(coalesce(syseqno, 0), '12', '0'), aprespn, billmsg, rtrim(filed28), trancde  from  pep_jnls where %s rtrim(termid) = '%s' \
		and sendcde = '%s' and pepdate>= '%s' and pepdate <= '%s'  order by pepdate desc, peptime desc;", trancde_temp, i_termid, i_sendcde, i_startdate, \
		i_enddate, trancde_temp, i_termid, i_sendcde, i_startdate, i_enddate);
        
        if(mysql_query(sock,sql)) {
            dcs_log(0, 0, "%s", sql);
            dcs_log(0,0,"EWPHT_mysql: GetQueryResult fail [%s]!", mysql_error(sock));
            return -1;
        }

        MYSQL_RES *res2;
        if (!(res2 = mysql_store_result(sock))) {
            dcs_log(0, 0,"EWPHT_mysql..GetQueryResult 2 : Couldn't get result from %s\n", mysql_error(sock));
            return -1;
        }
        
        num_fields = mysql_num_fields(res2);

        while ((row = mysql_fetch_row(res2)))
        {
		memset(i_pepdate, 0, sizeof(i_pepdate));
		memset(i_peptime, 0, sizeof(i_peptime));
		memset(i_outcdno, 0, sizeof(i_outcdno));
		memset(i_intcdno, 0, sizeof(i_intcdno));
		memset(i_tranamt, 0, sizeof(i_tranamt));
		memset(i_syseqno, 0, sizeof(i_syseqno));
		memset(i_aprespn, 0, sizeof(i_aprespn));
		memset(i_billmsg, 0, sizeof(i_billmsg));
		memset(i_transfee, 0, sizeof(i_transfee));

		memcpy(i_pepdate, row[0] ? row[0] : "\0", (int)sizeof(i_pepdate));
		memcpy(i_peptime, row[1] ? row[1] : "\0", (int)sizeof(i_peptime));
		memcpy(i_outcdno, row[2] ? row[2] : "\0", (int)sizeof(i_outcdno));
		memcpy(i_intcdno, row[3] ? row[3] : "\0", (int)sizeof(i_intcdno));
		memcpy(i_tranamt, row[4] ? row[4] : "\0", (int)sizeof(i_tranamt));
		if(row[5])
			i_trace = atol(row[5]);
		memcpy(i_syseqno, row[6] ? row[6] : "\0", (int)sizeof(i_syseqno));
		memcpy(i_aprespn, row[7] ? row[7] : "\0", (int)sizeof(i_aprespn));
		memcpy(i_billmsg, row[8] ? row[8] : "\0", (int)sizeof(i_billmsg));
		memcpy(i_transfee, row[9] ? row[9] : "\0", (int)sizeof(i_transfee));
		memcpy(i_trancde, row[10] ? row[10] : "\0", (int)sizeof(i_trancde));

		if(i_len!=0&&i_len == getstrlen(i_outcdno))
		{
			if(memcmp(ewp_hist_rq.querycdno, i_outcdno, i_len)!=0)
				continue;
		}
		if(memcmp(ewp_hist_rq.queryflag, "1", 1)==0)
		{
			if(memcmp(i_aprespn, "00", 2)!=0)
				continue;
		}
		else if(memcmp(ewp_hist_rq.queryflag, "2", 1)==0)
		{
			if(memcmp(i_aprespn, "00", 2)==0)
				continue;
		}
		numflg++;
		dcs_log(0, 0, "numflg = [%d]", numflg);
		if(numflg<minnum)
			continue;
		if(numflg > maxnum)
			break;
#ifdef __LOG__
		dcs_log(0, 0, "i_pepdate = [%s]", i_pepdate);
		dcs_log(0, 0, "i_peptime = [%s]", i_peptime);
//		dcs_log(0, 0, "i_outcdno = [%s]", i_outcdno);
//		dcs_log(0, 0, "i_intcdno = [%s]", i_intcdno);
		dcs_log(0, 0, "i_tranamt = [%s]", i_tranamt);
		dcs_log(0, 0, "i_trace = [%ld]", i_trace);
		dcs_log(0, 0, "i_syseqno = [%s]", i_syseqno);
		dcs_log(0, 0, "i_aprespn = [%s]", i_aprespn);
		dcs_log(0, 0, "i_billmsg = [%s]", i_billmsg);
		dcs_log(0, 0, "i_transfee = [%s]", i_transfee);
#endif
		if(memcmp(ewp_hist_rq.process, "FF0001", 6) == 0||memcmp(ewp_hist_rq.process, "FF0002", 6) == 0)
		{
			sprintf(buffer+len, "%s", i_trancde);
			len +=3;
		}
		sprintf(buffer+len, "%-19s%-19s", i_outcdno, i_intcdno);
		len +=38;
		if(getstrlen(i_transfee)== 0)
		{
			sprintf(buffer+len, "%s%s", i_tranamt, "000000000000");
		}
		else
		{
			i_transfee[getstrlen(i_transfee)] = 0;
			memcpy(tmpbuf, i_transfee, 1);
			PubAddSymbolToStr(i_transfee+1,11, '0',0);
			sprintf(tmpbuf+1, "%s", i_transfee+1);
#ifdef __LOG__
				dcs_log(0, 0, "tmpbuf = [%s]", tmpbuf);
#endif
			sprintf(buffer+len, "%s%s", i_tranamt, tmpbuf);
		}
		len +=24;
		sprintf(buffer+len, "%s%s", i_pepdate, i_peptime);
		len +=14;
		sprintf(buffer+len, "%06ld%12s", i_trace, i_syseqno);
		len +=18;
		if(getstrlen(i_aprespn)==0||memcmp(i_aprespn, "N1", 2)==0)
		{
			memset(i_aprespn, 0x00, sizeof(i_aprespn));
			memcpy(i_aprespn, "98", 2);
		}
		sprintf(buffer+len, "%s%-40s%s", i_aprespn, i_billmsg, ";");
		len +=43;            
        }
        
        mysql_free_result(res2);
        
    	memcpy(buffer+len, ewp_hist_rq.selfewp, 20);
		len +=20;
	#ifdef __LOG__
        dcs_log(0, 0, "buffer = [%s], len = [%d]", buffer, len);
	#endif
	}	
	return len;
sql_error:
    dcs_log(0, 0, "EWPHT_mysql..GetQueryResult 3 : error [%s]\n", mysql_error(sock));
	return -1;
}


/*
 查询指定时间段内某个用户的可撤销交易记录
 返回EWP信息有应答码ANS2，总条数N3，当前返回条数N1，
 转出卡号ANS19，转入卡号ANS19，交易金额N12，交易手续费N12，
 交易日期和时间N14，交易流水N6，交易参考号N12，应答码ANS2，分隔符S1';'
 */
int GetMposQueryResult(EWP_HIST_RQ ewp_hist_rq, char *buffer)
{
    char 	i_trancde[3+1];
    char 	i_termid[25+1];
    char 	i_startdate[8+1];
    char 	i_enddate[8+1];
    char 	i_pepdate[8+1];
    char 	i_peptime[6+1];
    char 	i_outcdno[20+1];
    char	i_intcdno[40+1];
    char 	i_tranamt[12+1];
    char 	i_transfee[12+1];
    long	i_trace;
    char 	i_syseqno[12+1];
    char    i_aprespn[2+1];
    char 	i_billmsg[40+1];
    char 	i_sendcde[8+1];
	char tmpbuf[27];
	int i_len = 0;
	int numflg = 0;
	
	memset(i_trancde, 0, sizeof(i_trancde));
	memset(i_termid, 0, sizeof(i_termid));
	memset(i_startdate, 0, sizeof(i_startdate));
	memset(i_enddate, 0, sizeof(i_enddate));
	memset(i_sendcde, 0, sizeof(i_sendcde));
	memset(i_pepdate, 0, sizeof(i_pepdate));
	memset(i_peptime, 0, sizeof(i_peptime));
	memset(i_outcdno, 0, sizeof(i_outcdno));
	memset(i_intcdno, 0, sizeof(i_intcdno));
	memset(i_tranamt, 0, sizeof(i_tranamt));
	memset(i_syseqno, 0, sizeof(i_syseqno));
	memset(i_aprespn, 0, sizeof(i_aprespn));
	memset(i_billmsg, 0, sizeof(i_billmsg));
	memset(i_transfee, 0, sizeof(i_transfee));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	
	int perpagenum, total_page, temp_page;
	int minnum, maxnum, curpage, len, total_num;
	int rtn;
	
	memcpy(i_trancde, ewp_hist_rq.trancde, 3);
	memcpy(i_termid, ewp_hist_rq.username, 11);
	i_termid[11] = '\0';
	memcpy(i_startdate, ewp_hist_rq.startdate, 8);
	memcpy(i_enddate, ewp_hist_rq.enddate, 8);
	
	memcpy(i_sendcde, ewp_hist_rq.sendcode, 4);
	memcpy(i_sendcde+4, "0000", 4);
	i_sendcde[8] = 0;
	sscanf(ewp_hist_rq.perpagenum, "%d", &perpagenum);
	sscanf(ewp_hist_rq.curpage, "%03d", &curpage);
	
	i_len = getstrlen(ewp_hist_rq.querycdno);
	if(i_len !=0)
		ewp_hist_rq.querycdno[i_len]=0;
    
   	 char sql[1024];
   	 memset(sql, 0x0, sizeof(sql));
    
	if(memcmp(ewp_hist_rq.queryflag, "3", 1)==0&&i_len!=0)
	{
		dcs_log(0, 0, "ewp_hist_rq.queryflag31 = [%s]", ewp_hist_rq.queryflag);  		
  		sprintf(sql, "select count(*) as tmp_count from pep_jnls where outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and termid = '%s' and trndate = '%s' and trancde = '%s' \
        and aprespn = '00' and billmsg  not in(select billmsg from pep_jnls where outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and termid = '%s' \
        and trndate = '%s' and trancde = '208' and aprespn = '00');", ewp_hist_rq.querycdno, i_termid, i_startdate, i_trancde, \
		ewp_hist_rq.querycdno, i_termid, i_startdate);
  		
	}

    if(mysql_query(sock,sql)) {
        dcs_log(0, 0, "%s", sql);
        dcs_log(0, 0, "EWPHT_mysql: GetQueryResult fail [%s]!", mysql_error(sock));
        return -1;
    }
    dcs_log(0, 0, "%s", sql);
    
    MYSQL_RES *res;
    MYSQL_ROW row ;
    int num_fields;
    
    if (!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "EWPHT_mysql..GetQueryResult : Couldn't get result from %s\n", mysql_error(sock));
        return -1;
    }
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    
    if (mysql_num_rows(res) == 1) {
        total_num = atoi(row[0]);
        
    } else total_num = 0;
    
    mysql_free_result(res);
	
	#ifdef __LOG__
		dcs_log(0, 0, "total_num = [%d]", total_num);
	#endif
	len = 0;
	memcpy(buffer, "0210", 4);
	len +=4;
	memcpy(buffer+len, ewp_hist_rq.process, 6);
	len +=6;
	memcpy(buffer+len, ewp_hist_rq.sendcode, 4);
	len +=4;
	memcpy(buffer+len, ewp_hist_rq.trancde, 3);
	len +=3;
	memcpy(buffer+len, ewp_hist_rq.startdate, 8);
	len +=8;
	memcpy(buffer+len, ewp_hist_rq.enddate, 8);
	len +=8;
	memcpy(buffer+len, ewp_hist_rq.perpagenum, 1);
	len +=1;
	memcpy(buffer+len, ewp_hist_rq.curpage, 3);
	len +=3;
	memcpy(buffer+len, ewp_hist_rq.username, 11);
	len +=11;
	memcpy(buffer+len, ewp_hist_rq.queryflag, 1);
	len +=1;
	memcpy(buffer+len, ewp_hist_rq.querycdno, 19);
	len +=19;
	if(total_num == 0)
	{
		memcpy(ewp_hist_rq.aprespn, "H3", 2);
		memcpy(buffer+len, ewp_hist_rq.aprespn, 2);
		len +=2;
		memcpy(buffer+len, ewp_hist_rq.selfewp, 20);
		len +=20;
		#ifdef __LOG__
			dcs_log(0, 0, "buffer = [%s], len = [%d]", buffer, len);
		#endif
	}
	else
	{
		memcpy(ewp_hist_rq.aprespn, "00", 2);
		sprintf(ewp_hist_rq.tatalnu, "%03d", total_num);
		temp_page = total_num%perpagenum;
		minnum = (curpage-1)*perpagenum+1;
		if(temp_page == 0)
		{
			total_page = total_num/perpagenum;
			maxnum = curpage*perpagenum;
		}
		else
		{
			total_page = total_num/perpagenum + 1;
			if(total_page == curpage)
				maxnum = total_num;
			else
				maxnum = curpage*perpagenum;
		}
		sprintf(ewp_hist_rq.curnum, "%d", maxnum-minnum+1);
		#ifdef __LOG__
			dcs_log(0, 0, "maxnum = [%d], minnum = [%d]", maxnum, minnum);
			dcs_log(0, 0, "ewp_hist_rq.curnum = [%s]", ewp_hist_rq.curnum);
			dcs_log(0, 0, "ewp_hist_rq.tatalnu = [%s]", ewp_hist_rq.tatalnu);
			dcs_log(0, 0, "ewp_hist_rq.aprespn = [%s]", ewp_hist_rq.aprespn);
		#endif
		memcpy(buffer+len, ewp_hist_rq.aprespn, 2);
		len +=2;
		memcpy(buffer+len, ewp_hist_rq.tatalnu, 3);
		len +=3;
		memcpy(buffer+len, ewp_hist_rq.curnum, 1);
		len +=1;        
        
        memset(sql, 0x00, sizeof(sql));
        sprintf(sql, "select pepdate, peptime, AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), AES_DECRYPT(UNHEX(intcdno), 'abcdefgh'), lpad(coalesce(tranamt, 0), '12', '0'), trace, lpad(coalesce(syseqno, 0), '12', '0'), \
        aprespn, billmsg, rtrim(filed28) from pep_jnls where outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and termid = '%s'and trndate = '%s' and trancde = '%s' \
        and aprespn = '00' and billmsg  not in(select billmsg from pep_jnls where outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and termid = '%s' \
        and trndate = '%s' and trancde = '208' and aprespn = '00');", ewp_hist_rq.querycdno, i_termid, i_startdate, i_trancde, \
  	    ewp_hist_rq.querycdno, i_termid, i_startdate);
        
        if(mysql_query(sock,sql)) {
            dcs_log(0, 0, "%s", sql);
            dcs_log(0,0,"EWPHT_mysql: GetQueryResult fail [%s]!" ,mysql_error(sock));
            return -1;
        }
        dcs_log(0, 0, "%s", sql);

        MYSQL_RES *res2;
        if (!(res2 = mysql_store_result(sock))) {
            dcs_log(0, 0,"EWPHT_mysql..GetQueryResult 2 : Couldn't get result from %s\n", mysql_error(sock));
            return -1;
        }
        
        num_fields = mysql_num_fields(res2);

        while ((row = mysql_fetch_row(res2)))
        {
		memset(i_pepdate, 0, sizeof(i_pepdate));
		memset(i_peptime, 0, sizeof(i_peptime));
		memset(i_outcdno, 0, sizeof(i_outcdno));
		memset(i_intcdno, 0, sizeof(i_intcdno));
		memset(i_tranamt, 0, sizeof(i_tranamt));
		memset(i_syseqno, 0, sizeof(i_syseqno));
		memset(i_aprespn, 0, sizeof(i_aprespn));
		memset(i_billmsg, 0, sizeof(i_billmsg));
		memset(i_transfee, 0, sizeof(i_transfee));

		memcpy(i_pepdate, row[0] ? row[0] : "\0", (int)sizeof(i_pepdate));
		memcpy(i_peptime, row[1] ? row[1] : "\0", (int)sizeof(i_peptime));
		memcpy(i_outcdno, row[2] ? row[2] : "\0", (int)sizeof(i_outcdno));
		memcpy(i_intcdno, row[3] ? row[3] : "\0", (int)sizeof(i_intcdno));
		memcpy(i_tranamt, row[4] ? row[4] : "\0", (int)sizeof(i_tranamt));
		if(row[5])
			i_trace = atol(row[5]);
		memcpy(i_syseqno, row[6] ? row[6] : "\0", (int)sizeof(i_syseqno));
		memcpy(i_aprespn, row[7] ? row[7] : "\0", (int)sizeof(i_aprespn));
		memcpy(i_billmsg, row[8] ? row[8] : "\0", (int)sizeof(i_billmsg));
		memcpy(i_transfee, row[9] ? row[9] : "\0", (int)sizeof(i_transfee));

		numflg++;
		dcs_log(0, 0, "numflg = [%d]", numflg);
		if(numflg<minnum)
			continue;
		if(numflg > maxnum)
			break;
#ifdef __LOG__
		dcs_log(0, 0, "i_pepdate = [%s]", i_pepdate);
		dcs_log(0, 0, "i_peptime = [%s]", i_peptime);
//		dcs_log(0, 0, "i_outcdno = [%s]", i_outcdno);
//		dcs_log(0, 0, "i_intcdno = [%s]", i_intcdno);
		dcs_log(0, 0, "i_tranamt = [%s]", i_tranamt);
		dcs_log(0, 0, "i_trace = [%ld]", i_trace);
		dcs_log(0, 0, "i_syseqno = [%s]", i_syseqno);
		dcs_log(0, 0, "i_aprespn = [%s]", i_aprespn);
		dcs_log(0, 0, "i_billmsg = [%s]", i_billmsg);
		dcs_log(0, 0, "i_transfee = [%s]", i_transfee);
#endif
		sprintf(buffer+len, "%-19s%-19s", i_outcdno, i_intcdno);
		len +=38;
		if(getstrlen(i_transfee)== 0)
		{
			sprintf(buffer+len, "%s%s", i_tranamt, "000000000000");
		}
		else
		{
			i_transfee[getstrlen(i_transfee)] = 0;
			memcpy(tmpbuf, i_transfee, 1);
			PubAddSymbolToStr(i_transfee+1,11, '0',0);
			sprintf(tmpbuf+1, "%s", i_transfee+1);
#ifdef __LOG__
				dcs_log(0, 0, "tmpbuf = [%s]", tmpbuf);
#endif
			sprintf(buffer+len, "%s%s", i_tranamt, tmpbuf);
		}
		len +=24;
		sprintf(buffer+len, "%s%s", i_pepdate, i_peptime);
		len +=14;
		sprintf(buffer+len, "%06ld%12s", i_trace, i_syseqno);
		len +=18;
		if(getstrlen(i_aprespn)==0||memcmp(i_aprespn, "N1", 2)==0)
		{
			memset(i_aprespn, 0x00, sizeof(i_aprespn));
			memcpy(i_aprespn, "98", 2);
		}
		sprintf(buffer+len, "%s%-40s%s", i_aprespn, i_billmsg, ";");
		len +=43;            
        }
        
        mysql_free_result(res2);
        
    	memcpy(buffer+len, ewp_hist_rq.selfewp, 20);
		len +=20;
	#ifdef __LOG__
        dcs_log(0, 0, "buffer = [%s], len = [%d]", buffer, len);
	#endif
	}	
	return len;
sql_error:
    dcs_log(0, 0, "EWPHT_mysql..GetQueryResult 3 : error [%s]\n", mysql_error(sock));
	return -1;
}

/*
交易查询服务接口
 查询指定日期的某个用户的商户收款交易记录
 返回EWP信息有应答码N2,交易卡号N19,交易金额N12,交易日期和时间N14,
 交易流水N6,交易参考号N12,订单号ANS20,授权码ANS6;
 */
int GetRrEnQueryResult(EWP_HIST_RQ ewp_hist_rq, char *buffer)
{
    char 	i_pepdate[8+1];
    char 	i_peptime[6+1];
    char 	i_tranamt[12+1];
    long	i_trace = 0;
    char 	i_syseqno[12+1];
    char    i_aprespn[2+1];
    char 	i_billmsg[20+1];
	char 	i_authcode[6+1];
    char 	i_sendcde[8+1];
	char 	i_outcdno[19+1];
	char    i_trancde[3+1];
	char 	trancde_temp[127];
	char sql[1024];
	int len = 0, aprespn_len = 0, rtn = 0, merlen = 0;
	char aprespn_info[256];
	POS_CUST_INFO pos_cust_info;
	
   	memset(sql, 0x00, sizeof(sql));
	memset(i_sendcde, 0, sizeof(i_sendcde));
	memset(i_pepdate, 0, sizeof(i_pepdate));
	memset(i_peptime, 0, sizeof(i_peptime));
	memset(i_tranamt, 0, sizeof(i_tranamt));
	memset(i_syseqno, 0, sizeof(i_syseqno));
	memset(i_aprespn, 0, sizeof(i_aprespn));
	memset(i_billmsg, 0, sizeof(i_billmsg));
	memset(i_authcode, 0, sizeof(i_authcode));
	memset(aprespn_info, 0, sizeof(aprespn_info));
	memset(i_outcdno, 0, sizeof(i_outcdno));
	memset(i_trancde, 0, sizeof(i_trancde));
	memset(trancde_temp, 0, sizeof(trancde_temp));
	memset(&pos_cust_info, 0x00, sizeof(POS_CUST_INFO));
	
	memcpy(i_sendcde, ewp_hist_rq.sendcode, 4);
	memcpy(i_sendcde+4, "0000", 4);
	i_sendcde[8] = 0;
	i_trace = atoi(ewp_hist_rq.trace);

	memcpy(buffer, "0210", 4);
	len +=4;
	memcpy(buffer+len, ewp_hist_rq.process, 6);
	len +=6;
	memcpy(buffer+len, ewp_hist_rq.selectdate, 8);
	len +=8;
	memcpy(buffer+len, ewp_hist_rq.sendtime, 6);
	len +=6;
	memcpy(buffer+len, ewp_hist_rq.sendcode, 4);
	len +=4;
	memcpy(buffer+len, ewp_hist_rq.trancde, 3);
	len +=3;
	memcpy(buffer+len, ewp_hist_rq.psam, 16);
	len +=16;
	memcpy(buffer+len, ewp_hist_rq.termidm, 20);
	len +=20;
	memcpy(buffer+len, ewp_hist_rq.username, 11);
	len +=11;
	if(memcmp(ewp_hist_rq.process, "9F0000", 6)==0||memcmp(ewp_hist_rq.process, "9F0004", 6)==0)
	{
		dcs_log(0, 0, "交易状态查询服务,脱机退货订单号[%s]", ewp_hist_rq.billmsg);
		memcpy(buffer+len, ewp_hist_rq.billmsg, 20);
		len +=20;
	}
	memcpy(buffer+len, ewp_hist_rq.merid, 20);
	len +=20;
	
	if(memcmp(ewp_hist_rq.process, "9F0000", 6)==0)
	{
		dcs_log(0, 0, "交易状态查询服务");
		sprintf(sql, "select pepdate, peptime, lpad(coalesce(tranamt, 0), '12', '0'), lpad(coalesce(syseqno, 0), '12', '0'), billmsg, \
		AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), trace, lpad(coalesce(authcode, 0), '6', '0'), aprespn, trancde \
		FROM pep_jnls where  billmsg = '%s' and \
		date_format(CONCAT(pepdate, peptime), '%s') = (select max(date_format(CONCAT(pepdate, peptime), '%s')) \
		from pep_jnls where billmsg = '%s')", ewp_hist_rq.billmsg, "%Y%m%d%H%i%s", "%Y%m%d%H%i%s", ewp_hist_rq.billmsg);
	}	
	else if(memcmp(ewp_hist_rq.process, "9F0001", 6)==0)
	{
		dcs_log(0, 0, "商户收款撤销查原笔");
		sprintf(sql, "select pepdate, peptime, lpad(coalesce(tranamt, 0), '12', '0'), lpad(coalesce(syseqno, 0), '12', '0'), billmsg, \
		AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), trace, lpad(coalesce(authcode, 0), '6', '0'), aprespn, trancde \
		from pep_jnls where outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and termid = '%s' and trndate = '%s' and trancde = '%s' and aprespn = '00'  and trace = '%d';", \
		ewp_hist_rq.querycdno, ewp_hist_rq.username, ewp_hist_rq.selectdate, ewp_hist_rq.trancde, i_trace);
	}
	else if(memcmp(ewp_hist_rq.process, "9F0002", 6)==0)
	{
		dcs_log(0, 0, "商户收款退货查原笔");
		sprintf(sql, "select pepdate, peptime, lpad(coalesce(tranamt, 0), '12', '0'), lpad(coalesce(syseqno, 0), '12', '0'), billmsg, \
		AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), trace, lpad(coalesce(authcode, 0), '6', '0'), aprespn, trancde \
		from pep_jnls where outcdno =  HEX(AES_ENCRYPT('%s','abcdefgh')) and termid = '%s' and trndate = '%s' and trancde = '%s' and aprespn = '00'  and trace = '%d' UNION ALL \
		select pepdate, peptime, lpad(coalesce(tranamt, 0), '12', '0'), lpad(coalesce(syseqno, 0), '12', '0'), billmsg, \
		AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), trace, lpad(coalesce(authcode, 0), '6', '0'), aprespn, trancde \
		from pep_jnls_all where outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and termid = '%s' and trndate = '%s' and trancde = '%s' and aprespn = '00'  and trace = '%d';", \
		ewp_hist_rq.querycdno, ewp_hist_rq.username, ewp_hist_rq.selectdate, ewp_hist_rq.trancde, i_trace, ewp_hist_rq.querycdno, \
		ewp_hist_rq.username, ewp_hist_rq.selectdate, ewp_hist_rq.trancde, i_trace);
	}
	else if(memcmp(ewp_hist_rq.process, "9F0004", 6)==0)
	{
		dcs_log(0, 0, "商户收款脱机退货查原笔");
		sprintf(sql, "select pepdate, peptime, lpad(coalesce(tranamt, 0), '12', '0'), lpad(coalesce(syseqno, 0), '12', '0'), billmsg, \
		AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), trace, lpad(coalesce(authcode, 0), '6', '0'), aprespn, trancde \
		from pep_jnls where outcdno =  HEX(AES_ENCRYPT('%s','abcdefgh')) and termid = '%s' and trndate = '%s' and trancde = '%s' and aprespn = '00'  and billmsg = '%s' UNION ALL \
		select pepdate, peptime, lpad(coalesce(tranamt, 0), '12', '0'), lpad(coalesce(syseqno, 0), '12', '0'), billmsg, \
		AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), trace, lpad(coalesce(authcode, 0), '6', '0'), aprespn, trancde \
		from pep_jnls_all where outcdno = HEX(AES_ENCRYPT('%s','abcdefgh')) and termid = '%s' and trndate = '%s' and trancde = '%s' and aprespn = '00'  and billmsg = '%s';", \
		ewp_hist_rq.querycdno, ewp_hist_rq.username, ewp_hist_rq.selectdate, ewp_hist_rq.trancde, ewp_hist_rq.billmsg, ewp_hist_rq.querycdno, \
		ewp_hist_rq.username, ewp_hist_rq.selectdate, ewp_hist_rq.trancde, ewp_hist_rq.billmsg);
	}
    else
	{
		dcs_log(0, 0, "末笔成功交易信息查询");
		#ifdef __TEST__
			sprintf(trancde_temp, "(trancde = '200' or trancde = '209' or trancde = '210')");
		#else
			sprintf(trancde_temp, "(trancde = '815' or trancde = '209' or trancde = '210')");
		#endif
		sprintf(sql, "select pepdate, peptime, lpad(coalesce(tranamt, 0), '12', '0'), lpad(coalesce(syseqno, 0), '12', '0'), billmsg, \
		AES_DECRYPT(UNHEX(outcdno), 'abcdefgh'), trace, lpad(coalesce(authcode, 0), '6', '0'), aprespn, trancde \
		from pep_jnls where termid = '%s' and trndate = '%s' and aprespn = '00' and %s \
		order by peptime desc limit 1;", ewp_hist_rq.username, ewp_hist_rq.selectdate, trancde_temp);
        
	}
    if(mysql_query(sock, sql)) 
	{
        dcs_log(0, 0, "%s", sql);
        dcs_log(0, 0, "GetRrEnQueryResult 查找原笔交易出错 [%s]!", mysql_error(sock));
		memcpy(i_aprespn, "QE", 2);
		goto sql_error;
    }
	
    MYSQL_RES *res; 
	MYSQL_ROW row ; 
	int num_fields = 0;
	int num_rows = 0;
    
    if(!(res = mysql_store_result(sock))) 
	{
        dcs_log(0, 0, "GetRrEnQueryResult:Couldn't get result from %s\n", mysql_error(sock));
		memcpy(i_aprespn, "QE", 2);
		goto sql_error;
    }
    num_fields = mysql_num_fields(res);
    row = mysql_fetch_row(res); 
	num_rows = mysql_num_rows(res);
    if (num_rows == 1) 
	{
		memcpy(i_pepdate, row[0] ? row[0] : "\0", (int)sizeof(i_pepdate));
		memcpy(i_peptime, row[1] ? row[1] : "\0", (int)sizeof(i_peptime));
		memcpy(i_tranamt, row[2] ? row[2] : "\0", (int)sizeof(i_tranamt));
		memcpy(i_syseqno, row[3] ? row[3] : "\0", (int)sizeof(i_syseqno));
		memcpy(i_billmsg, row[4] ? row[4] : "\0", (int)sizeof(i_billmsg));
		memcpy(i_outcdno, row[5] ? row[5] : "\0", (int)sizeof(i_outcdno));
		if(row[6]) 
			i_trace = atoi(row[6]);
		memcpy(i_authcode, row[7] ? row[7] : "\0", (int)sizeof(i_authcode));
		memcpy(i_aprespn, row[8] ? row[8] : "\0", (int)sizeof(i_aprespn));
		memcpy(i_trancde, row[9] ? row[9] : "\0", (int)sizeof(i_trancde));
	}
	else if(num_rows == 0)
	{
		dcs_log(0, 0, "未查询到数据");
		dcs_log(0, 0, " [%s]!",sql);
		memcpy(i_aprespn, "QE", 2);
		goto sql_error;
	}
	#ifdef __LOG__
		dcs_log(0, 0, "i_pepdate = [%s]", i_pepdate);
		dcs_log(0, 0, "i_peptime = [%s]", i_peptime);
		dcs_log(0, 0, "i_tranamt = [%s]", i_tranamt);
		dcs_log(0, 0, "i_trace = [%ld]", i_trace);
		dcs_log(0, 0, "i_syseqno = [%s]", i_syseqno);
		dcs_log(0, 0, "i_aprespn = [%s]", i_aprespn);
		dcs_log(0, 0, "i_billmsg = [%s]", i_billmsg);
		dcs_log(0, 0, "i_authcode = [%s]", i_authcode);
		dcs_log(0, 0, "i_trancde = [%s]", i_trancde);
	#endif
sql_error:
	if(memcmp(i_aprespn, "00", 2)==0)
	{
		sprintf(buffer+len, "%-19s", i_outcdno);
		len +=19;
		sprintf(buffer+len, "%s", i_aprespn);	
		len +=2;
		sprintf(buffer+len, "%06ld", i_trace);
		len +=6;
		if(memcmp(ewp_hist_rq.process, "9F0000", 6)!=0)
		{
			sprintf(buffer+len, "%-19s", i_outcdno);
			len +=19;
			sprintf(buffer+len, "%s", i_tranamt);	
			len +=12;
			sprintf(buffer+len, "%s%s", i_pepdate, i_peptime);
			len +=14;
			sprintf(buffer+len, "%-20s", i_billmsg);
			len +=20; 
			sprintf(buffer+len, "%s", i_trancde);
			len +=3;
		}
		sprintf(buffer+len, "%s", i_syseqno);
		len +=12;
		sprintf(buffer+len, "%s", i_authcode);
		len +=6; 		
	}  
	else
	{
		sprintf(buffer+len, "%-19s", i_outcdno);
		len +=19;
		sprintf(buffer+len, "%s", i_aprespn);	
		len +=2;
		sprintf(buffer+len, "%06ld", i_trace);
		len +=6;
		sprintf(buffer+len, "%012d", 0);
		len +=12;
		sprintf(buffer+len, "%06d", 0);
		len +=6; 	
	}
    mysql_free_result(res);   
	aprespn_len = GetDetailInfo(i_aprespn, ewp_hist_rq.trancde, aprespn_info);
	if(aprespn_len == -1)
		aprespn_len = 0;
	
	dcs_log(0, 0, "取交易应答描述成功[%s]", aprespn_info);
	buffer[len] = aprespn_len;
	len +=1;
	memcpy(buffer+len, aprespn_info, aprespn_len);
	len +=aprespn_len;
	
	rtn = GetCustInfo(ewp_hist_rq.username, ewp_hist_rq.psam, &pos_cust_info);
	if(rtn < 0)
	{
		dcs_log(0, 0, "get pos_cust_info error");	
		sprintf(buffer+len, "%015d", 0);
		len += 15; 
		sprintf(buffer+len, "%08d", 0);
		len += 8; 
		buffer[len] = 0;
		len +=1;
	}
	else
	{
		memcpy(buffer+len, pos_cust_info.cust_merid, 15);
		len += 15; 
		memcpy(buffer+len, pos_cust_info.cust_termid, 8);
		len += 8; 
		merlen = getstrlen(pos_cust_info.merdescr_utf8);
		buffer[len] = merlen;
		len +=1;
		memcpy(buffer+len, pos_cust_info.merdescr_utf8, merlen);
		len += merlen; 	
	}	
				
	memcpy(buffer+len, ewp_hist_rq.selfewp, 20);
	len +=20;
	#ifdef __LOG__
        dcs_log(0, 0, "buffer = [%s], len = [%d]", buffer, len);
	#endif
	return len;
}

/*mysql断开重连的处理*/
int GetEwpMysqlLink()
{	
	int i =0;
	for(i = 0; i<7; i++)
	{
		#ifdef __TEST__
			dcs_log(0, 0, "%d ci GetEwpMysqlLink", i);
		#endif
		if(mysql_ping(sock)!=0)
			continue;
		else
			break;
	}
	#ifdef __TEST__
		dcs_log(0, 0, "test GetEwpMysqlLink");
	#endif
	return mysql_ping(sock);
}

