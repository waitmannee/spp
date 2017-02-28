#include "ibdcs.h"
#include "tmcibtms.h"
#include "iso8583.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "extern_function.h"
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include "dccsock.h"
#ifdef __macos__
    #include </usr/local/mysql/include/mysql.h>
#elif defined(__linux__)
    #include </usr/include/mysql/mysql.h>
#elif defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include </usr/local/mysql/include/mysql.h>
#endif
//static char gIp[25]="";
static char g_date[8+1]="20160229";//日期
static char g_trace[6+1]="123456";//流水
#define MAX_PROCE 250

typedef struct child
{
	int pid[10];
	int num;
}child;


extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];

static int moniChongZheng();
static int SetDatePara();
static int SetTracePara();
typedef struct packBuf
{
	char buf[500];
	int trace;
}packBuf;


typedef struct {
	char pepdate[8+1];	/*系统日期,前置系统的日期 */
	char peptime[6+1];	/*系统时间 ,前置系统的时间*/
	char trancde[3+1];	/*交易类型 */
	char msgtype[4+1];	/*报文类型 */
	char outcdno[20+1];	/*转出卡号 */
	char outcdtype;	/*转出卡号卡种 */
	char intcdno[40+1];	/*转入卡号 */
	char intcdbank[30+1];/*转账汇款交易的转卡号银行编号*/
	char intcdname[30+1];/*转账汇款交易的转入卡姓名*/
	char process[6+1];	/*处理码 */
	char tranamt[12+1];	/*转出金额 */
	long trace;		/*流水号 */
	char termtrc[6+1];	/*终端上送流水号 */
	char trntime[6+1];	/*交易时间 , 交易报文中的发送日期*/
	char trndate[8+1];	/*交易日期,年月日. 交易报文中的发送日期*/
	char poitcde[3+1];	/*输入方式，报文22域 */
	char track2[37+1];	/*磁道2数据*/
	char track3[104+1];	/*磁道3数据*/
	char sendcde[8+1];	/*发送机构代码 */
	char syseqno[12+1];	/*系统参考号 */
	char termcde[8+1];	/*终端号 */
	char mercode[15+1];	/*商户号 */
	char billmsg[40+1];	/*帐单号 */
	char samid[25+1];	/*SAM卡号 */
	char termid[25+1];	/*终端号 */
	char termidm[25+1];	/*终端标示码 */
	char aprespn[2+1];	/*中心应答码 */
	char sdrespn[2+1];	/*上行系统应答码 */
	char revodid[1+1];	/*冲正标识 */
	char billflg[1+1];	/*帐单支付情况通知标识 */
	char mmchgid[1+1];	/*存储转发标识 */
	int  mmchgnm;	        /*存储转发次数 */
	char termmac[16+1];	/*终端上送MAC */
	char termpro[200+1];	/*终端上送流程码 */
	char trnsndp[15+1];	/*交易上送通道标识 */
	int  trnsndpid; /*用来表示folder的id*/
	char trnmsgd[4+1];	/*交易上送报文类型  --TPDU源地址 */
	char revdate[10+1];	/*交易回应*/
	char modeflg[1+1];	/*短信标志 0-无  1-有*/
	char modecde[24+1];	/*手机号*/
	char filed28[9+1];  	/* 28域 */
	char filed48[256+1];  	/* 48域 电银账户号码*/
	int	 merid;				/* 商户编号*/
	char authcode[6+1];		/*授权码*/
	char discountamt[12+1];	/*优惠金额*/
	char poscode[2+1];	/*它表示交易发生的终端状况，用它可以区分交易来自于什么终端。报文25域*/
	char translaunchway[1+1];	/*交易发起方式*/
	char batch_no[6+1];		/*批次号*/
	int camtlimit;  /*用于表示是否有信用卡日限额的限制，有就是1，没有就是0*/
	int damtlimit;  /*用于表示是否有借记卡日限额的限制，有就是1，没有就是0*/
	int termmeramtlimit; /*终端商户信用卡日限额标识，1：存在限制，0：没有限制*/
	char settlementdate[4+1];/*清算日期，报文的15域*/
	char ReversedFlag[1+1];/*终端冲正标识   1：终端发起冲正  0：未发起冲正*/
	char ReversedAns[2+1];/*冲正应答*/
	char SysReNo[12+1];/*终端冲正返回的系统参考号*/
	char posmercode[15+1];/*pos终端送上来的商户号*/
	char postermcde[8+1];/*pos终端送上的终端号*/
	char filed44[22+1];/*pos交易44域的信息*/
	char gpsid[35+1];/*无线POS基站编码*/
	char card_sn[3+1];/*卡片序列号*/
	char filed55[512+1];/*55域数据*/
	char tc_value[16+1]; /* TC value*/
	char netmanage_code[3+1]; /*网络管理信息码*/
}PEP_JNLS;

char iso8583name[129][30] = {
	"消息类型",			/*bit 0 */
	"bitmap",
	"主帐号",
	"处理代码",
	"交易金额",
	"自定义",
	"自定义",
	"交易日期和时间",
	"自定义",
	"自定义",
	"自定义",
	"系统流水号",
	"本地交易时间",
	"本地交易日期",
	"自定义",
	"结算日期",
	"自定义",
	"受理日期",
	"商户类型",
	"自定义",
	"自定义",
	"终端信息",
	"服各点进入方式",
	"自定义",
	"自定义",
	"服务点条件码",
	"自定义",
	"自定义",
	"交易费",
	"自定义",
	"交易处理费",
	"自定义",
	"代理方机构标识码",
	"发送机构标识码",
	"自定义",
	"第二磁道数据",
	"第三磁道数据",
	"检索参考号",
	"授权标识响应",
	"响应代码",					/* bit 39 */
	"服务限制代码",
	"受卡方终端标识",
	"受卡方标识代码",
	"受卡方名称/地点",
	"附加响应数据",
	"自定义",
	"自定义",
	"自定义",
	"自定义附加数据",
	"交易货币代号",
	"结算货币代号",
	"自定义",
	"个人识别号数据",
	"安全控制信息",
	"附加金额",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"MACkey使用标志",
	"自定义",
	"原因码",
	"结算批次号",
	"MAC值",						/* bit 64 */
	"自定义",
	"结算代号",
	"自定义",
	"自定义",
	"自定义",
	"网络管理功能码",
	"报文编号",
	"后报文编号",
	"动作日期",
	"贷记笔数",
	"撤消贷记笔数",
	"借记笔数",
	"撤消借记笔数",
	"转帐笔数",
	"撤消转帐笔数",
	"自定义",
	"授权笔数",
	"贷记手续费金额",
	"自定义",
	"借记手续费金额",
	"自定义",
	"贷记交易金额",
	"撤消贷记金额",
	"借记交易金额",
	"撤消借记金额",
	"原始数据",
	"文件更新代码",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"报文保密代码",
	"净清算额",
	"自定义",
	"自定义",
	"接收机构标识代码",
	"文件名称",
	"转出账户",						/* bit 102 */
	"转入账户",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"MAC值"						/* bit 128 */
};

MYSQL mysql, *sock;    //定义数据库连接的句柄，它被用于几乎所有的MySQL函数
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
static void TransRcvSvrExit(int signo)
{
    if(signo > 0)
        dcs_log(0,0,"catch a signal %d\n",signo);
    if(signo !=0 && signo != SIGTERM && signo != SIGSEGV)
        return;

    dcs_log(0,0,"AppServer terminated.\n");
    exit(signo);
}


int iso_to_str( unsigned char * dstr , ISO_data * iso, int flag )
{
	int flaghead, flaglength, flagmap;
	int len, slen, i, s, num ;;
	char buffer[2048], tmpbuf[999], bcdbuf[999], newbuf[999],tmp[300];
	char map[25];
	char dispinfo[8192];
	struct ISO_8583 iso8583_conf_local[128];
	memset(iso8583_conf_local, 0, sizeof(iso8583_conf_local));

		memcpy(iso8583_conf_local,iso8583_conf,sizeof(iso8583_conf));
	//iso8583=&iso8583_conf[0];注释掉
	flaghead = GetIsoHeardFlg();
	flaglength = GetFieldLeagthFlag();
	sprintf( dispinfo, "iso_to_str: flaghead=%d, flaglength=%d.\n", flaghead, flaglength );
	sprintf( dispinfo, "%s-------------------------------------------------------------------\n", dispinfo );
	flagmap = 0;
	memset( buffer, 0x0, sizeof(buffer) );
	memset( map, 0x0, sizeof( map ) );
	memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
	s = getbit( iso, 0, (unsigned char*) tmpbuf );
	//dcs_debug( tmpbuf, s, "Get bit[0  ] len=%d.", s );
	sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%s]\n", dispinfo, 0, iso8583name[0], s, tmpbuf );
	if ( s < 4 )
	{
		len = -1;
		goto isoend;
	}
	if ( flaghead == 1 )  ///不压缩
	{
		memcpy( dstr, tmpbuf, 4 );
		len = 4;
	} else	///压缩
	{
		memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
		asc_to_bcd( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, 4, 0 );  //暂时不考虑左边加0的情况
		memcpy( dstr, bcdbuf, 2 );
		len = 2;
	}
	slen = 0;
	for ( i = 1; i < 192; i++ )
	{
		num = 0;
		memset( newbuf, 0x0, sizeof( newbuf ) );
		s = getbit( iso, i + 1, (unsigned char *)tmpbuf );
		if ( s > 0 )
		{
			tmpbuf[ s] = 0x0;
			//为了显示方便不乱码，特转换为ascii码显示
			if ( i == 51 || i == 54 || i == 63 || i == 127 || i == 191 )
			{
				memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
				bcd_to_asc( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s  * 2, 0 );
				sprintf( newbuf, bcdbuf );
			}
			else if(i == 61)
			{
				memset(tmp, 0, sizeof(tmp));
				getbit( iso,20, (unsigned char *)tmp );
			}
			else
			{
				memcpy( newbuf, tmpbuf, s );
			}
			if(num == 0)
				num = s;
			#ifdef __TEST__
				sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%1.1d] [%1.1d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], num, iso8583_conf_local[i].flag, iso8583_conf_local[i].type, newbuf );
			#else
				if(i!=34 && i!=35 && i!=51)
					sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%1.1d] [%1.1d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], num, iso8583_conf_local[i].flag, iso8583_conf_local[i].type, newbuf );
			#endif
			//dcs_debug( tmpbuf, s, "Get bit[%3.3d] len=%d. flag=%d, type=%d.", i + 1, s, iso8583_conf[i].flag, iso8583_conf[i].type );
			map[ i / 8 ] = map[ i / 8 ] | (0x80 >> ( i % 8 ) );
			if ( i > 64 )
			{
				map[0] = map[0] | 0x80;
				flagmap = 1;
			}
			if ( i > 128 )
			{
				map[8] = map[8] | 0x80;
				flagmap = 2;
			}
			if ( iso8583_conf_local[i].flag == 0 )	////固定长
			{
				if ( iso8583_conf_local[i].type == 0 )  ///不压缩
				{
					memcpy( buffer + slen, tmpbuf, s );
					slen += s;
					if ( iso8583_conf_local[i].len != s )	//长度不够(多或少）
					{
						slen += iso8583_conf_local[i].len - s;
					}
				} else	///压缩
				{
					memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
					asc_to_bcd( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s, 0 );  //暂时不考虑左边加0的情况
					s = (s + 1) / 2;
					memcpy( buffer + slen, bcdbuf, s );
					slen += s;
				}
			} else		////可变长
			{
				if ( iso8583_conf_local[i].flag == 1 ) //2位可变长
				{
					if ( flaglength == 1 )
					{
						sprintf( buffer + slen, "%02d", s );
						slen += 2;
					} else
					{
						buffer[slen] = (unsigned char)( (s / 10) *16 + s % 10);
						slen += 1;
					}
				} else
				{
					if ( flaglength == 1 )
					{
						sprintf( buffer + slen, "%03d", s );
						slen += 3;
					} else
					{
						buffer[slen] = (unsigned char)(s / 100) ;
						slen += 1;
						buffer[slen] = (unsigned char)( ((s % 100) / 10) *16 + s % 10);
						slen += 1;
					}
				}
				if ( iso8583_conf_local[i].type == 0 )  ///不压缩
				{
					memcpy( buffer + slen, tmpbuf, s );
					slen += s;
				} else	///压缩
				{
					memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
					asc_to_bcd( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s, 0 );  //暂时不考虑左边加0的情况
					s = (s + 1) / 2;
					memcpy( buffer + slen, bcdbuf, s );
					slen += s;
				}
			}
		} else if ( s < 0 )  ////getbit()
		{
			len = -1;
			goto isoend;
		}
	} // end of for
	memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
	bcd_to_asc( (unsigned char *) tmpbuf, (unsigned char *)map, (flagmap + 1 ) * 8 * 2, 0 );
	sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%s]\n", dispinfo, 1, iso8583name[1], (flagmap + 1 ) * 8, tmpbuf );
	//dcs_debug( map, (flagmap + 1) * 8, "Get bitmap len=%d.", (flagmap + 1) * 8 );
	memcpy( dstr + len, map, (flagmap + 1) * 8 );
	len += (flagmap + 1) * 8;
	memcpy( dstr + len, buffer, slen );
	len += slen;
isoend:
	sprintf( dispinfo, "%s-------------------------------------------------------------------", dispinfo );
	if ( flag )
		dcs_log( 0, 0, "%s", dispinfo );

	return len;
}

static int OpenLog()
{
    char path[500];
    char file[520];
    memset(path,0,sizeof(path));
    memset(file,0,sizeof(file));
    getcwd(path,500);
    sprintf(file,"%s/moniSale.log",path);
    return dcs_log_open(file, "moniSale");
}

int main(int argc, char *argv[])
{
	catch_all_signals(TransRcvSvrExit);
    cmd_init("moniSalecmd>>",TransRcvSvrExit);
  	cmd_add("moniChongZheng",     moniChongZheng,   " moniChongZheng");
  	cmd_add("SetDatePara",     SetDatePara,   " SetDatePara");
  	cmd_add("SetTracePara",     SetTracePara,   " SetTracePara");
 	cmd_shell(argc,argv);

  	TransRcvSvrExit(0);
    return 0;
}//main()



/*
 根据交易流水查询pep_jnls得到原笔交易的前置系统流水
 */
int GetTrace(char *trace, char *date, PEP_JNLS *pep_jnls)
{
	char sql[1024];
    char tmpbuf[127], tmp_len[3];

	memset(sql, 0, sizeof(sql));

    dcs_log(0, 0, "原交易查询");

   	dcs_log(0, 0, "trace=[%s], date=[%s]", trace, date);

  	sprintf(sql, "SELECT trace, outcdno, tranamt, poitcde, poscode, termcde, mercode, sendcde,trnsndp,translaunchway,samid, \
  			postermcde,posmercode,termtrc,batchno,process,trancde FROM pep_jnls WHERE trace ='%s' and pepdate= '%s' and trancde not like 'C%';", trace, date);
	if(mysql_query(sock, sql)) {
        dcs_log(0, 0, "%s", sql);
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
		memcpy(pep_jnls->tranamt, row[2] ? row[2] : "\0", (int)sizeof(pep_jnls->tranamt));
		memcpy(pep_jnls->poitcde, row[3] ? row[3] : "\0", (int)sizeof(pep_jnls->poitcde));

		memcpy(pep_jnls->poscode, row[4] ? row[4] : "\0", (int)sizeof(pep_jnls->poscode));
		memcpy(pep_jnls->termcde, row[5] ? row[5] : "\0", (int)sizeof(pep_jnls->termcde));
		memcpy(pep_jnls->mercode, row[6] ? row[6] : "\0", (int)sizeof(pep_jnls->mercode));
		memcpy(pep_jnls->sendcde, row[7] ? row[7] : "\0", (int)sizeof(pep_jnls->sendcde));
		memcpy(pep_jnls->trnsndp, row[8] ? row[8] : "\0", (int)sizeof(pep_jnls->trnsndp));
		memcpy(pep_jnls->translaunchway, row[9] ? row[9] : "\0", (int)sizeof(pep_jnls->translaunchway));
		memcpy(pep_jnls->samid, row[10] ? row[10] : "\0", (int)sizeof(pep_jnls->samid));
		memcpy(pep_jnls->postermcde, row[11] ? row[11] : "\0", (int)sizeof(pep_jnls->postermcde));
		memcpy(pep_jnls->posmercode, row[12] ? row[12] : "\0", (int)sizeof(pep_jnls->posmercode));
		memcpy(pep_jnls->termtrc, row[13] ? row[13] : "\0", (int)sizeof(pep_jnls->termtrc));
		memcpy(pep_jnls->batch_no, row[14] ? row[14] : "\0", (int)sizeof(pep_jnls->batch_no));
		memcpy(pep_jnls->process, row[15] ? row[15] : "\0", (int)sizeof(pep_jnls->process));
		memcpy(pep_jnls->trancde, row[16] ? row[16] : "\0", (int)sizeof(pep_jnls->trancde));
	}

	else if(num_rows==0)
	{
		dcs_log(0, 0, "%s", sql);
		dcs_log(0, 0, "GetTrace 未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		dcs_log(0, 0, "%s", sql);
		dcs_log(0, 0, "GetTrace 找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	mysql_free_result(res);

  		dcs_log(0, 0, "pep_jnls->trace=[%ld], outcdno=[%s],trancde=[%s]", pep_jnls->trace, pep_jnls->outcdno,pep_jnls->trancde);

	return 0;
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
int GetChannelRestrict(char *i_mgr_termid, char * i_mgr_merid, char *trancde)
{
	int i_channel_id;
    int rtn ;
    char i_trancde[3+1];
	char sql[1024];

	memset(i_trancde, 0, sizeof(i_trancde));
	memset(sql, 0, sizeof(sql));

	memcpy(i_trancde, trancde, 3);

	sprintf(sql, "SELECT channel_id FROM pos_channel_info  WHERE mgr_term_id = '%s' and mgr_mer_id = '%s';",\
	i_mgr_termid, i_mgr_merid);

	if(mysql_query(sock, sql)) {
        dcs_log(0, 0, "%s", sql);
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
		dcs_log(0, 0, "%s", sql);
        dcs_log(0, 0, "GetChannelRestrict:未找到符合条件的记录");
		mysql_free_result(res);
        return -1;
	}
	else if(num_rows > 1)
	{
		dcs_log(0, 0, "%s", sql);
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
        dcs_log(0, 0, "%s", sql);
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
		dcs_log(0, 0, "%s", sql);
		dcs_log(0, 0, "ChannelTradeCharge:未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows >1)
	{
		dcs_log(0, 0, "%s", sql);
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
 功能：通过psam卡号得到该psam卡的密钥信息
 输入参数：psamNo卡号，16字节长度
 输出参数：SECUINFO
 */
int GetSecuInfo(char *samNo)
{
	char sql[1024];
	int channel_id;

    memset(sql, 0x00, sizeof(sql));

    sprintf(sql, "SELECT channel_id FROM samcard_info WHERE sam_card = '%s';", samNo);

    if(mysql_query(sock, sql)) {
        dcs_log(0, 0, "%s", sql);
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
        if(row[0])
			channel_id = atoi(row[0]);
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
	return channel_id;
}



/*
 输入参数：pos机终端号和商户号,以及机具编号
 输出参数：主密钥索引
 成功返回0
 失败返回-1
 */
int GetPosInfo(char *i_term_id, char * i_mer_id,char *T0_flag ,char *agents_code)
{
	char sql[1048];
	memset(sql, 0, sizeof(sql));

	sprintf(sql, "SELECT t0_flag,agents_code  FROM pos_conf_info WHERE mgr_term_id = '%s' and mgr_mer_id ='%s'", i_term_id, i_mer_id);


	if(mysql_query(sock,sql)) {
        dcs_log(0, 0, "%s", sql);
        dcs_log(0, 0, "GetPosInfo error=[%s]!", mysql_error(sock));
        return -1;
    }

    MYSQL_RES *res;
    MYSQL_ROW row ;
    memset(&row, 0, sizeof(MYSQL_ROW));
    int num_fields;
	int num_rows = 0;

    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetPosInfo : Couldn't get result from %s\n", mysql_error(sock));
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
		dcs_log(0, 0, "GetPosInfo 未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		dcs_log(0, 0, "%s", sql);
		dcs_log(0, 0, "GetPosInfo 找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);
	dcs_log(0, 0, "mer_info_t.T0_flag=[%s], mer_info_t.agents_code=[%s]", T0_flag,agents_code);
	return 0;
}


/*根据用户名查询数据表pos_cust_info得到商户号和终端号*/
int GetCustInfo(char *username, char *psam,char *T0_flag ,char *agents_code)
{
	char sql[1048];
	memset(sql, 0, sizeof(sql));
	char i_username[10];
	char i_psam[17];

	memset(i_username, 0, sizeof(i_username));
	memset(i_psam, 0, sizeof(i_psam));

	if(getstrlen(username)<=2)
		return -1;
	memcpy(i_username, username, getstrlen(username)-2);
	memcpy(i_psam, psam, getstrlen(psam));

	 sprintf(sql, "SELECT t0_flag,agents_code FROM pos_cust_info  \
		WHERE substr(cust_id,1,9) = '%s' and cust_psam = '%s';",i_username, i_psam);

	if(mysql_query(sock,sql)) {
        dcs_log(0, 0, "%s", sql);
        dcs_log(0, 0, "GetPosInfo error=[%s]!", mysql_error(sock));
        return -1;
    }

    MYSQL_RES *res;
    MYSQL_ROW row ;
    memset(&row, 0, sizeof(MYSQL_ROW));
    int num_fields;
	int num_rows = 0;

    if(!(res = mysql_store_result(sock))) {
        dcs_log(0, 0, "GetPosInfo : Couldn't get result from %s\n", mysql_error(sock));
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
		dcs_log(0, 0, "GetPosInfo 未找到符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
	else if(num_rows > 1)
	{
		dcs_log(0, 0, "%s", sql);
		dcs_log(0, 0, "GetPosInfo 找到多条符合条件的记录");
		mysql_free_result(res);
		return -1;
	}
    mysql_free_result(res);
	dcs_log(0, 0, "T0_flag=[%s],agents_code=[%s]", T0_flag,agents_code);
	return 0;
}


/*
模拟消费冲正交易
*/
int moniChongZheng()
{
 	ISO_data siso;
       struct  tm *tm;
	time_t  t;
	char    *ltime;
	char tmpbuf[40];
	int s = 0, ret=0;
    char	buffer[1000],buffer2[1000], newbuf[1000];
    char T0_flag[2], angentcode[6+1];
	int pid,s_myFid;
	char currentDate[14+1];
	PEP_JNLS pep_jnls;

	memset(&pep_jnls, 0, sizeof(PEP_JNLS));

	s = OpenLog();
	if(s < 0)
        return -1;
	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		 return 0;
	}

	struct packBuf *pack;
	if(IsoLoad8583config(&iso8583_conf[0],"iso8583.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));
	        return -1;
    	}


	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);

	dcs_log(0,0," 模拟POS消费冲正交易发送给核心 start");//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
	time(&t);
	tm = localtime(&t);
	memset(currentDate, 0, sizeof(currentDate));

	sprintf(currentDate, "%04d%02d%02d%02d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	s_myFid = fold_create_folder("PEX_COM");

	if(s_myFid < 0)
		s_myFid = fold_locate_folder("PEX_COM");

	if(s_myFid < 0)
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", "PEX_COM", ise_strerror(errno) );
		return -1;
	}
	DssConect();
	GetTrace(g_trace, g_date, &pep_jnls);
	dcs_log(0,0,"currentDate=[%s]\n", currentDate);
	//组报文
	memset(&siso,0,sizeof(ISO_data));
	setbit(&siso,0,(unsigned char *)"0400", 4);
	setbit(&siso,2,(unsigned char *)pep_jnls.outcdno, strlen(pep_jnls.outcdno));
	setbit(&siso,3,(unsigned char *)"000000", 6);
	setbit(&siso,4,(unsigned char *)pep_jnls.tranamt, strlen(pep_jnls.tranamt));
	setbit(&siso,7,(unsigned char *)currentDate+4, 10);
	setbit(&siso,11,(unsigned char *)g_trace, 6);//流水

	//组21域报文
	memset(buffer, 0, sizeof(buffer));
	if(memcmp(pep_jnls.trnsndp, "TPOS", 4) ==0 )//传统POS
	{
		int rtn = GetChannelRestrict(pep_jnls.postermcde, pep_jnls.posmercode, pep_jnls.trancde);
		if(rtn < 0)
				return -1;
		memset(T0_flag, 0, sizeof(T0_flag));
		memset(angentcode, 0, sizeof(angentcode));
		GetPosInfo(pep_jnls.postermcde, pep_jnls.posmercode,T0_flag, angentcode);
		sprintf(buffer, "%s%s303%s%s%s%s%04d%s%s", pep_jnls.posmercode,pep_jnls.postermcde, pep_jnls.trancde, pep_jnls.termtrc, pep_jnls.batch_no, "000000", rtn,T0_flag, angentcode);
	}
	else
	{
		int rtn = GetSecuInfo(pep_jnls.samid);
		if(rtn < 0)
			return -1;
		memset(T0_flag, 0, sizeof(T0_flag));
		memset(angentcode, 0, sizeof(angentcode));
		GetCustInfo(pep_jnls.termid,pep_jnls.samid,T0_flag, angentcode);
		sprintf(buffer, "%s%-15s%s01%s%s%04d%s%s",pep_jnls.samid, pep_jnls.termid, pep_jnls.translaunchway, pep_jnls.trancde, "000000", rtn,T0_flag, angentcode);
	}
	setbit(&siso,21,(unsigned char *)buffer, strlen(buffer));

	if(memcmp(pep_jnls.poitcde, "05", 2) == 0)
		memcpy(pep_jnls.poitcde, "021", 3);

	setbit(&siso,22,(unsigned char *)pep_jnls.poitcde, 3);
	setbit(&siso,25,(unsigned char *)pep_jnls.poscode, 2);
	setbit(&siso,39,(unsigned char *)"98", 2);
	setbit(&siso,41,(unsigned char *)pep_jnls.termcde,8);
	setbit(&siso,42,(unsigned char *)pep_jnls.mercode,15);
	setbit(&siso,49,(unsigned char *)"156",3);

	//组60域
	memset(buffer, 0, sizeof(buffer));
	if(memcmp(pep_jnls.poitcde, "02", 2) == 0)
		memcpy(buffer, "20", 2);
	else if(memcmp(pep_jnls.poitcde, "05", 2) == 0)
		memcpy(buffer, "50", 2);
	else if(memcmp(pep_jnls.poitcde, "9", 1) == 0)
		memcpy(buffer, "60", 2);

	if(memcmp(pep_jnls.trnsndp, "TPOS", 4) ==0 )//传统POS
		memcpy(buffer+2, "03", 2);
	else if(memcmp(pep_jnls.sendcde, "0100", 4)==0
	|| memcmp(pep_jnls.sendcde, "0600", 4)==0
	||memcmp(pep_jnls.sendcde, "0900", 4)==0)
		memcpy(buffer+2, "22", 2);
	else if(memcmp(pep_jnls.sendcde, "0400", 4)==0
	|| memcmp(pep_jnls.sendcde, "0500", 4)==0
	|| memcmp(pep_jnls.sendcde, "0800", 4)==0)
		memcpy(tmpbuf+2, "11", 2);


	setbit(&siso,60,(unsigned char *)buffer , 4);

	iso8583 =&iso8583_conf[0];
	SetIsoHeardFlag(0);
		SetIsoFieldLengthFlag(1);

	memset(buffer, 0, sizeof(buffer));
	s = iso_to_str( (unsigned char *)buffer, &siso, 1);
	if ( s < 0 )
	{
		dcs_log(0, 0, "iso_to_str error.");
		return -1;
	}
	memset(tmpbuf, 0, sizeof(tmpbuf));
	bcd_to_asc((unsigned char *)tmpbuf, (unsigned char *)buffer, 20 ,0);

	memset(buffer2, 0, sizeof(buffer));

	memcpy(buffer2, tmpbuf, 20);
	memcpy(buffer2+20, buffer+10, s-10);

	s = s+10;

	dcs_log(0,0, "folder myFid=%d\n", s_myFid);

	fold_write(s_myFid, 0, buffer2, s);
	dcs_debug(buffer2, s,"buf");
	dcs_log(0,0,"发送报文结束end");
	exit(1);

}

int SetDatePara()
{
	printf("当前设置:日期%s.\n",g_date);
	printf("请输入日期:");
	scanf("%s",g_date);
	return 0;
}

int SetTracePara()
{
	printf("当前设置:流水号%s.\n",g_trace);
	printf("请输入流水:");
	scanf("%s",g_trace);
	return 0;
}






