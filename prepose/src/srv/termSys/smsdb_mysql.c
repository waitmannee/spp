#include <errno.h>
#include <time.h>
#include <unistd.h>
#include "trans.h" 
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

struct  _varchar {
	unsigned short len;
	unsigned char  arr[1];
};

typedef struct _varchar VARCHAR;

//==============================//
static MYSQL mysql, *sock;    //定义数据库连接的句柄，它被用于几乎所有的MySQL函数
//==============================//

int SMSDBConect()
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
		fprintf (stderr , "错误, %s\n" , mysql_error(& mysql));
		dcs_log(0, 0, "错误, %s\n", mysql_error(& mysql));
	}

	if (sock) {
		dcs_log(0, 0, "数据库连接成功,name:%s", getenv("DBNAME"));
		return 1;
	} else {
		dcs_log(0, 0 ,"数据库连接失败,name:%s", getenv("DBNAME"));
		return 0;
	}
	return 0;
}

int SMSDBConectEnd(int iDssOperate)
{
	switch(iDssOperate)
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
    mysql_close(sock);
	return 0;
}

/*
trancde|outcard|incard|tradeamt|tradefee|tradedate|traderesp|tradetrace|traderefer|tradebillmsg|userphone
*/
int sms_save(char *smsdata)
{
	char 	trancde[3+1];
	char 	outcard[19+1];
	char 	incard[19+1];
	char 	tradeamt[12+1];
	char 	tradefee[12+1];
	char 	tradedate[14+1];
	char 	traderesp[2+1];
	char	tradetrace[6+1];
	char 	traderefer[12+1];
	char    tradebillmsg[40+1];
	char 	userphone[11+1];
	char    sql[1024];
	
	memset(trancde, 0, sizeof(trancde));
	memset(outcard, 0, sizeof(outcard));
	memset(incard, 0, sizeof(incard));
	memset(tradeamt, 0, sizeof(tradeamt));
	memset(tradefee, 0, sizeof(tradefee));
	memset(tradedate, 0, sizeof(tradedate));
	memset(traderesp, 0, sizeof(traderesp));
	memset(tradetrace, 0, sizeof(tradetrace));
	memset(traderefer, 0, sizeof(traderefer));
	memset(tradebillmsg, 0, sizeof(tradebillmsg));
	memset(userphone, 0, sizeof(userphone));
	memset(sql, 0, sizeof(sql));
	
	dcs_log(0, 0, "sms_save smsdata:[%s]", smsdata);
	sscanf(smsdata, "%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]", trancde, outcard, incard, tradeamt, 
				tradefee, tradedate, traderesp, tradetrace, traderefer, tradebillmsg, userphone); 
	
	dcs_log(0, 0, "trancde:[%s]", trancde);
	#ifdef __TEST__
		dcs_log(0, 0, "outcard:[%s]", outcard);
	#endif 
//	dcs_log(0, 0, "incard:[%s]", incard);
	dcs_log(0, 0, "tradeamt:[%s]", tradeamt);
	dcs_log(0, 0, "tradefee:[%s]", tradefee);
	dcs_log(0, 0, "tradedate:[%s]", tradedate);
	dcs_log(0, 0, "traderesp:[%s]", traderesp);
	dcs_log(0, 0, "tradetrace:[%s]", tradetrace);
	dcs_log(0, 0, "traderefer:[%s]", traderefer);
	dcs_log(0, 0, "tradebillmsg:[%s]", tradebillmsg);
	dcs_log(0, 0, "userphone:[%s]", userphone);
	
	sprintf(sql, "INSERT INTO sms_jnls(trancde, outcard, incard, tradeamt, tradefee, tradedate,\
								   traderesp, tradetrace, traderefer, tradebillmsg, userphone, allsmsdata)\
    	 VALUES ('%s', HEX(AES_ENCRYPT('%s','abcdefgh')), HEX(AES_ENCRYPT('%s','abcdefgh')), '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')", trancde, outcard, incard, tradeamt, tradefee, tradedate, \
		 traderesp, tradetrace, traderefer, tradebillmsg, userphone, smsdata);
	if(mysql_query(sock,sql)) 
	{
        dcs_log(0, 0, "%s", sql);
        dcs_log(0, 0, "INSERT INTO sms_jnlsDB  error [%s]!", mysql_error(sock));
        return -1;
    }
	
	return 1;
}

int updateSmsResult(char *smsdata)
{
	char 	smsresult[10];
	char* p;
	char sql[512];
	
	memset(smsresult, 0, sizeof(smsresult));
	memset(sql, 0, sizeof(sql));
	
	if(smsdata == NULL)
	{
		dcs_log(0, 0, "smsdata is null, return -2");
		return -2;
	}
	
	dcs_log(0, 0, "updateSmsResult smsdata:[%s]", smsdata);
	
	p = strstr(smsdata, "|");
	if(p == NULL)
	{
		dcs_log(0, 0, "smsdata error, return -2");
		return -2;
	}
	
	if( (p-smsdata == 0) || (p-smsdata > 10) )
	{
		dcs_log(0, 0, "smsdata result length error.");
		return -3;
	}
	
	memcpy(smsresult, smsdata, p-smsdata);
	p += 1;
	
	dcs_log(0, 0, "updateSmsResult smsresult=[%s]", smsresult);
	dcs_log(0, 0, "updateSmsResult original data=[%s]", p);
	
	sprintf(sql, "UPDATE sms_jnls set smsresult ='%s' where allsmsdata ='%s'", smsresult, p);
	if(mysql_query(sock,sql)) 
	{
        dcs_log(0, 0, "%s", sql);
        dcs_log(0, 0, "update sms_jnls DB  error [%s]!", mysql_error(sock));
        return -1;
    }
	p = NULL;
	return 1;
	
}

/*mysql断开重连的处理*/
int GetSmsMysqlLink()
{	
	int i =0;
	for(i = 0; i<7; i++)
	{
		#ifdef __TEST__
			dcs_log(0, 0, "%d ci GetSmsMysqlLink", i);
		#endif
		if(mysql_ping(sock)!=0)
			continue;
		else
			break;
	}
	#ifdef __TEST__
		dcs_log(0, 0, "test GetSmsMysqlLink");
	#endif
	return mysql_ping(sock);
}

