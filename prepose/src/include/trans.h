#define PUB_MAXSIZE	1024
#define PUB_MAXLEN	1024
#define PUB_MAX_SLEN    2048
#define BILL_MSGTYPE	"02"

/*
F0	前置系统		消费撤销或是退货时找不到原笔交易
F1	前置系统		解析接入系统发过来的报文数据失败
F2	前置系统		内部商户交易卡种限制，该商户不支持此银行卡
F3	前置系统		解密磁道错误
F4	前置系统		查询类：想要查询的交易不存在
F5	前置系统		组iso8583包失败
F6	前置系统		商户交易金额限制,无效的交易金额
F7	前置系统		转加密密码错误
F8	前置系统		取前置系统的交易流水错误
F9	前置系统		发送报文给核心系统失败
FA	前置系统		该交易不允许银行卡无密支付
FB	前置系统		查询msgchg_info失败,交易未开通
FC	前置系统 		查询samcard_info失败,psam卡未找到
FD	前置系统		该渠道该交易不存在
FE	前置系统		该渠道限制交易
FF	前置系统		该渠道该交易未开通
FG	前置系统		该终端属于黑名单，不允许交易
FH	前置系统		终端卡种限制，未知当前交易卡种
FI	前置系统		终端卡种限制,受限制的卡
FJ	前置系统		终端限制交易金额,不在最大最小金额的范围
FK	前置系统		终端卡种无法判断，又有交易日限额的控制
FL	前置系统		该终端当前的交易总额，超过总交易额的限制
FM	前置系统		操作数据库表商户黑名单失败
FN	前置系统		商户黑名单，该商户不允许该交易
FO	前置系统		保存pep_jnls数据失败
FP	前置系统		保存EWP_INFO失败
FQ	前置系统		终端正常，可以使用
FR	前置系统		终端审核中
FS	前置系统		终端撤机中
FT	前置系统		终端停机中
FU	前置系统		终端复机中
FV	前置系统		终端未使用
FW	前置系统		终端使用中
FX	前置系统		终端已停机
FY	前置系统		查询psam当前的状态信息失败
FZ	前置系统		数据库操作错误
H0	前置系统		订单支付状态未知，5分钟之内不允许再次支付
H1	前置系统		由于系统或者其他原因，交易暂时关闭
H2	前置系统		收款交易终端未开通
H3	前置系统		历史交易查询没有记录

H4	前置系统		取POS终端上送的41域错误
H5	前置系统		取POS终端上送的42域错误
H6	前置系统		取POS终端上送的60域错误
H7	前置系统		设置冲正字段失败
H8	前置系统		消费撤销是否需要磁密信息和后台不匹配
H9	前置系统		POS上送的报文格式不对

HA 	前置系统		POS机未入库
HB 	前置系统		POS签到失败
HC 	前置系统		不是刷卡交易暂时不受理
*/
#define RET_CODE_F0    "F0"
#define RET_CODE_F1    "F1"
#define RET_CODE_F2    "F2"
#define RET_CODE_F3    "F3"
#define RET_CODE_F4    "F4"
#define RET_CODE_F5    "F5"
#define RET_CODE_F6    "F6"
#define RET_CODE_F7    "F7"
#define RET_CODE_F8    "F8"
#define RET_CODE_F9    "F9"
#define RET_CODE_FA    "FA"
#define RET_CODE_FB    "FB"
#define RET_CODE_FC    "FC"
#define RET_CODE_FD    "FD"
#define RET_CODE_FE    "FE"
#define RET_CODE_FF    "FF"
#define RET_CODE_FG    "FG"
#define RET_CODE_FH    "FH"
#define RET_CODE_FI    "FI"
#define RET_CODE_FJ    "FJ"
#define RET_CODE_FK    "FK"
#define RET_CODE_FL    "FL"
#define RET_CODE_FM    "FM"
#define RET_CODE_FN    "FN"
#define RET_CODE_FO    "FO"
#define RET_CODE_FP    "FP"
#define RET_CODE_FY	   "FY"
#define RET_CODE_FZ	   "FZ"
#define RET_CODE_H0	   "H0"
#define RET_TRADE_CLOSED	"H1"
#define RET_CODE_H2	   "H2"
#define RET_CODE_H3	   "H3"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   	   "G3"
#define RET_CODE_H4	   "H4"
#define RET_CODE_H5    "H5"
#define RET_CODE_H6	   "H6"
#define RET_CODE_H7    "H7"
#define RET_CODE_H8    "H8"
#define RET_CODE_H9    "H9"
#define RET_CODE_HA    "HA"
#define RET_CODE_HB    "HB"
#define RET_CODE_HC    "HC"

#ifndef __TRANS_H__
#define __TRANS_H__
typedef struct{

	 int iId;
	 int iType;
	 int iLen;

}POSFLDDEF;


typedef struct {
     char  cverid[8];
	 char  cTransType[3];/*交易码*/
	 char  cAmount[12];/*交易金额*/
	 char  cPan[20];/*主账号也就是卡号*/
	 char  cSamID[20+1];/*psamid,ksn*/
	 char  cTermID[25];
	 char  cTrkFlg[5];
	/*磁道数据:第1位是否有数据，第2位是否加密;第3,4,5位长度*/
	 char  cTrack[150];	/*磁道信息*/
	 char  cPin[8];/*个人标识码*/
	 char  cMac[16];
	 int   iNum;
	 char  cOptCode[200];
	 char  cAddi[200];
	 /*用来标志该终端的参数升级请求是否切往新的管理系统*/
	 char   newParamSys[1+1];
	 char   e1code[64+1];
	 char   cdLimit[1+1];
}TERMMSGSTRUCT;

typedef struct {
	 char  bill_msgtype[2+1];
     char  bill_trancde[3+1];
     char  bill_remark[40+1];
	 char  bill_amount[12+1];
	 char  bill_simno[20+1];
	 char  bill_ctrkflg[5+1];
	/*磁道数据:第1位是否有数据，第2位是否加密;第3,4,5位长度*/
	 char  bill_track2[37+1];
	 char  bill_track3[104+1];
	 char  bill_cPin[16+1];
	 char  bill_stance[32+1];
	 char  bill_mercode[15+1];
	 char  bill_orgcode[11+1];
	 char  bill_currency[3+1];
	 char  bill_zone_code[6+1];
	 char  bill_callno[12+1];
	 char  bill_termid[25+1];
	 int   bill_hintlen;
	 char  bill_hintmsg[256+1];
	 char  bill_billvalid[1+1];
	 char  bill_retcode[2+1];
	 char  bill_rcvdate[8+1];
	 char  bill_rcvtime[6+1];
	 char  bill_expdt[14+1];
	 char  bill_pepstance[6+1];
	 char  bill_poitcde[3+1];
	 char  bill_modeflg[1+1];	/*短信标志 0-无  1-有*/
	 char  bill_modecde[24+1];	/*手机号*/
	 int   bill_fid;
	 int   bill_mid;
	 char  bill_field21[46+1];
	 char  bill_field2[20+1];
	 char  bill_field7[10+1];
	 char  bill_field12[6+1];
	 char  bill_field13[4+1];
	 char  bill_field25[2+1];
	 char  bill_termtrc[6+1];
	 char  bill_termmac[16+1];
	 char  bill_termpro[200+1];
	 char  bill_termsrcbuf[2048+1];
	 char  bill_field53[16+1];
	 char  bill_xzret[2+1];
	 char  time_stamp[14+1];
	 char  useraccount[15+1];
	 char  userdata[64+1];/*用户自定义信息*/
}BILLCKECKSTRUCT;


typedef struct {
	 char  bill_msgtype[2+1];
     char  bill_stance[32+1];
     char  bill_mercode[15+1];
	 char  bill_amount[12+1];
	 char  bill_retcode[2+1];
	 char  bill_pepstance[6+1];
	 char  bill_field2[20+1];
	 char  bill_field7[10+1];
	 char  time_stamp[14+1];
	 char  useraccount[15+1];
}RESENDDBLOGSTRUCT;


typedef struct {
	char  trancde[3] ;
	char  msgtype[4] ;
	char  process[6] ;
	char  service[2] ;
	char  amtcode[3] ;
	char  mercode[15];
	char  termcde[8] ;
	char  subtype[4] ;
}MSGCHGSTRU;

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
	char translaunchway[2+1];	/*交易发起方式*/
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
	char transfer_type[2]; /*转账汇款类型 0：普通/实时 1：次日到账*/
}PEP_JNLS;

struct  TERM_ORDER {              /* 终端域定义 */
        char    	 cTransType[3];  /* term_order */
        unsigned char    flag;
/* length field :1--1位HEX长 2--2位HEX长 T---7位压缩日期+1位HEX长 R--接收数据*/
        int    len;             /* data element max length */
        unsigned char    fmt;	/* 0-不压缩， 1-压缩  */
};

typedef struct {
   char	 trancde[3+1];
   char  msgtype[4+1] ;
   char  process[6+1] ;
   char  service[2+1];
   char  amtcode[3+1];
   char  mercode[15+1];
   char  termcde[8+1];
   char  subtype[4+1];
   char  revoid[1+1];
   char  bit_20[4+1];
   char  bit_25[2+1];
   char  frttran[40+1];
   char	 remark[40+1];
   char	 flag20[1+1];
   char	 flagpe[1+1];
   char	 cdlimit;
   char  c_amtlimit[12+1]; /*信用卡单笔交易限额 ：默认为null 不限制*/
   char  d_amtlimit[12+1]; /*借记卡单笔交易限额 ：默认为null 不限制 */
   char  c_sin_totalamt[12+1]; /*信用卡单卡交易日限额 ：默认为null 不限制*/
   char  d_sin_totalamt[12+1]; /*借记卡单卡交易日限额 ：默认为null 不限制*/
   int	 c_day_count; /*信用卡日交易次数限制：默认为null 不限制*/
   int   d_day_count; /*借记卡日交易次数限制：默认为null 不限制*/
   char  flagpwd[1+1];
   char	 flagfee[1+1]; /*便利金标志		 */
			/* 1	根据金额查找 */
}MSGCHGINFO;

typedef struct {
   char  transcde[3+1];
   char  minamt[12+1];
   char  maxamt[12+1];
   char  mercode[15+1];
   char  termcde[8+1];
   char  fee[12+1];
   char  rate[8+1];
   char  limit_upper[12+1];
   char	 flagfee[1+1];
}MSGCHGCOST;


typedef struct {
	char	simno[18+1];
        char	updflg[16+1];                   /*终端参数更新标示*/
					 	/*第1位--菜单参数*/
                                                /*第2位--终端参数*/
                                                /*第3位--SAM卡参数*/
                                                /*第4位--功能提示信息*/
                                                /*第5位--操作提示信息*/
                                                /*第6位--首页提示信息*/
        char	menuver[6+1];                   /*菜单索引号*/
        char	parmver[6+1];                   /*参数版本  */
	char	mumsgver[6+1];                  /*菜单操作提示信息版本*/
	char    gnmsgver[6+1];                  /*功能提示信息版本*/
	char	ermsgver[6+1];                  /*错误提示信息版本*/
	char	mamsgver[6+1];                  /*主提示信息版本  */
        char	prnver[6+1];               	/*打印信息版本     */
        char	lstdat[8+1];               	/*上次更新成功版本号*/
        char	curdat[8+1];               	/*当前更新版本号*/
}TERMVERINFO;

typedef struct {
        char	simno[18+1];
        char	code[20+1];       		/*终端标识码-一般是电话号*/
        char	brcode[6+1];                	/*终端分机号*/
        char	zone[6+1];                	/*终端地区号*/
        char	e1code[14+1];      		/*E1平台代码*/
        char	statu[1+1];	              	/*2---需更新参数; 1--正常*/
                                                /*0--注册 */
                                                /*3--注册成功，资料不全*/
                                                /*4--需上传交易日志*/
                                                /*5--需上传错误日志*/
        char	txtyp[1+1];	              	/*终端通讯类型 1-分机*/
	char	merid[15+1];               	/*商户编号*/
        char	mername[60+1];               	/*商户名称*/
    	char	menuver[6+1];          		/*菜单索引号*/

}TERMBASINFO;

typedef struct {
	char sam_card[16+1];/*psam卡号，16字节*/
	long channel_id; /*终端投资方Id*/
	char sam_model[1+1];/*psam卡内部加密机函数版本。1：音频刷卡设备，双分散密钥；2：实达一体机pos，双分散密钥*/
	char sam_status[2+1];/*psam是否正常，00正常，02菜单需要更新*/
	char sam_signinflag[1+1];/*签到是否成功标志*/
	int  sam_zmk_index;/*主密钥索引*/
	char sam_zmk[32+1];/*当前使用的主密钥，被加密机LMK主密钥加密*/
	char sam_zmk1[32+1];/*终端第1组主密钥*/
	char sam_zmk2[32+1];/*终端第2组主密钥*/
	char sam_zmk3[32+1];/*终端第3组主密钥*/
	int  sam_pinkey_idx;/*sam pik索引*/
	char sam_pinkey[32+1];/*pik密钥,双倍长*/
	int  sam_mackey_idx;/*sam mak索引*/
	char sam_mackey[32+1];/*mak密钥,双倍长*/
	int  sam_tdkey_idx;/*sam tdk索引*/
	char sam_tdkey[32+1];/*磁道密钥,双倍长*/
	int  pinkey_idx;/*zmk索引*/
	char pinkey[16+1];/*pik工作密钥，单倍长*/
	int  mackey_idx;/*zmk索引*/
	char mackey[16+1];/*mak工作密钥，单倍长*/
	int  pin_convflag;/*Pan与pin异或标志。1：Pan参与异或，2：Pan不参与异或*/
}SECUINFO;

typedef struct {
	char orgcode[12];
	char snddate[11];
	int stan;
	char ipadr[19];
	long mtype;
	char msgtype[3];
	char bill_type[5];
	char rcvdate[9];
	char sndtime[7];
	char zone_code[7];
	char callno[26];
	char expdt[15];
	char billstance[32+1];/*add by heqq*/
	char mercode[16];
	char currency[4];/*add by heqq*/
	char txamt[13];
	char rvcode[33];
	int hintlen;
	char hintmsg[256];
	char billvalid[2];/*add by heqq*/
	char e1code[4+1];/*add by heqq*/
	char simno[16+1];/*add by heqq*/
	char pepstance[6+1];/*add by heqq*/
	char time_stamp[14+1];/*add by heqq*/
	char  useraccount[15+1];/*add by heqq*/
	char retcode[3];
	int signlen;
	unsigned char signmsg[257];
} GSMS_BILLSTRUC;

typedef struct {
	char msgcode[3];
	char call_no[19];
	int sysstan;
	char endflag[2];
	char sndtime[11];
}SMS_DATAIDX;

typedef struct {
	char	in_cdno[20+1];
	char	in_date[8+1];
	char	in_time[6+1];
	char	in_telno[18+1];
	char	in_zcode[6+1];
	char	in_zbrcode[6+1];
	char	in_brtelno[8+1];
	char	in_samcdno[16+1];
	char	in_protime[10+1];
}PEP_CHECKIN;

typedef struct {
        char    mer_billcode[15+1];
        char    mer_paycode[15+1];
        char    mer_terno[8+1];
        char    mer_type[4+1];
        char    mer_name[200+1];
        char    mer_process[6+1];
        char    mer_bit20[2+1];
}MERINFO;

typedef struct {
        char transcde[3+1];
        char order_index[2+1];
        char term_order[3+1];
        char data_flag[1+1];
        char map_ord_code[200+1];
        char bitd_code[100+1];
        char map_bit_code[100+1];
        char fg_flag[4+1];
}ORD8583;

typedef struct {
	char tm_evkey[73];		/* key		*/
	int  tm_folder;			/* 发送数据的通讯目录 */
	int  tm_timeout;		/* 超时时间 */
	char tm_buffer[2000];		/* 原始数据 */
	int  tm_bufflen;		/* 原始数据长度 */
	char tm_create[10+1];		/* 建立的时间 */
} PEP_TIME;


typedef struct {
	int data_def;
	char data_formater[50];
}MEUNCOUNTROL;


/*消费类交易*/
typedef struct {
	char consumer_transtype[4+1];/*交易类型*/
	char consumer_transdealcode[6+1];/*交易处理码*/
	char consumer_sentdate[8+1];/*发送日期*/
	char consumer_senttime[6+1];/*发送时间*/
	char consumer_sentinstitu[4+1];/*发送机构号*/
	char consumer_transcode[3+1];/*交易码*/
	char consumer_psamno[16+1];/*psam编号*/
	char consumer_phonesnno[20+1];/*机具编号*/
	char consumer_username[11+1];/*用户名也就是手机号*/
	char consumer_orderno[20+1];/*订单号*/
	char consumer_merid[20+1];/*商户编号*/
	char consumer_termwaterno[6+1];/*终端流水*/
	char consumer_cardno[19+1];/*卡号指转出卡号*/
	char incardno[19+1];/*卡号指转入卡号*/
	char incdname[50+1];/*转入卡姓名,Gbk编码，右补空格*/
	char incdbkno[25+1];/*转入卡行号*/
	char consumer_money[12+1];/*金额*/
	char consumer_trackno[160+1];/*磁道*/
	char consumer_track_randnum[16+1];/*磁道密钥随机数*/
	char consumer_password[16+1];/*密码*/
	char consumer_pin_randnum[16+1];/*pin密钥随机数*/
	char consumer_responsecode[2+1];/*应答码*/
	char consumer_presyswaterno[6+1];/*前置系统流水*/
	char consumer_sysreferno[12+1];/*系统参考号*/
	char pretranstime[14+1];/*原交易发送日期和时间即：原笔交易的具体时间*/

	char retcode[3];/*解析数据是否成功的标记 */
	char authcode[6+1];/*授权码*/
	char discountamt[12+1];/*优惠金额*/
	char poscode[2+1];	/*它表示交易发生的终端状况，用它可以区分交易来自于什么终端。报文25域*/
	char translaunchway[2+1];	/*交易发起方式*/
	char selfdefine[20+1];/*自定义域*/
	char modeflg[1+1];/*是否需要发送短信的标记1：需要 0：不需要*/
	char modecde[11+1];/*需要发送短信的手机号*/
	char transfee[12+1];/*转账类交易和转账汇款的交易手续费*/
	char macinfo[8+1]; /*sk EWP上送的MAC信息*/
	char tpduheader[10+1];/*TPDU*/

	char netmanagecode[1+1];/*网络管理信息码*/
	char fallback_flag[1+1];/*降级标志*/
	char paraseq[2+1];/*当前公钥/参数序号*/
	char serinputmode[3+1];/*服务点输入方式码*/
	char filed55_length[3+1];/*IC卡数据域长度*/
	char filed55[512+1];/*IC卡数据域*/
	char originaldate[14+1];/*原交易日期*/
	char termtyp[1+1];/*终端类型*/
	char icflag[1+1];/*IC卡标识 IC挥卡：2   IC卡插卡：1      磁条卡：0*/
	char cardseq_flag[1+1];/*卡片序列号存在标识*/
	char cardseq[3+1];/*卡片序列号*/
	int  total_keypara;/*公钥/参数总个数*/
	char org_time[14+1];/*原交易日期时间*/
	char poitcde_flag[1+1];/*交易上送方式 0 存量终端，1 IC卡不降级，2 IC卡降级，3 纯磁条, 4 挥卡*/
	char download_pubkey_flag[1+1];/*公钥下载标识*/
	char download_para_flag[1+1];/*参数下载标识*/
	char revdate[10+1];
	char termcde[8+1];/*终端号*/
	char mercode[15+1];/*商户号*/
	char consumer_ksn[20+1];/*KSN编号*/
	char org_orderno[20+1];/*原笔交易订单号*/
	char ksn[20+1];/*just for i58 test*/
	char macblk[32+1];/*just for i58 test*/
	char mw_password[6+1];/*just for i58*/
	char trackno2[37+1]; /*二磁道信息*/
	char trackno3[104+1]; /*三磁道信息*/
	char transfer_type[2]; /*转账汇款类型 0：普通/实时 1：次日到账*/
}EWP_INFO;


typedef struct {
  char mgr_ins_id_cd[11+1];		/*机构号*/
  int  mgr_sys_index;			/*机构索引*/
  char mgr_term_id[8+1];		/*终端号*/
  char mgr_mer_id[15+1];		/*商户号*/
  char mgr_flag[1+1];			/*商户标识，签到是否成功标示*/
  char mgr_mer_des[40*3+1];		/*商户名称*/
  char mgr_batch_no[6+1];		/*批次号*/
  char mgr_folder[14+1];		/*链路名称*/
  char mgr_tpdu[22+1];			/*TPDU头*/
  char mgr_signout_flag[1+1];	/*签退标示，1—自动，0—不自动*/
  int  mgr_timeout;				/*超时时间，秒*/
  int  mgr_key_index;			/*当前使用主密钥索引值*/
  char mgr_kek[32+1];
  char mgr_des[3+1];			/*加密方法：DES：001 3DES：003  磁道密钥：004*/
  char mgr_pik[32+1]; 			/*pik工作密钥*/
  char mgr_mak[16+1]; 			/*mak工作密钥*/
  char mgr_tdk[32+1]; 			/*磁道工作密钥*/
  char mgr_signtime[14+1]; 			/*YYYYMMDD hhmmss*/
  char track_pwd_flag[2+1];			/*是否有磁道密码的标记 00: 无磁无密 01 :无磁有密  10 :有磁无密  11 :有磁有密*/
  char pos_machine_id[25+1]; 		/*pos机具编号*/
  int  pos_machine_type;			/*2:个人移动支付终端3:有线pos,4:无线gprspos,5:无线apn卡pos,6:sk项目POS*/
  char pos_apn[30+1];				/*apn卡号*/
  char pos_telno[16+1];				/*电话号码*/
  char pos_gpsno[35+1];				/*定位号码*/
  char batch_settle_flag[2+1];		/*批结算的标记00 不进行批结算 01 下载结束,强制批结算*/
  char pos_type[2+1]; 					/*pos终端应用类型*/
  int  cz_retry_count;				/*冲正重发次数*/
  char trans_telno1[14+1];  	/*三个交易电话号码之一,14位数字,右补空格*/
  char trans_telno2[14+1];   	/*三个交易电话号码之二,14位数字,右补空格*/
  char trans_telno3[14+1];   	/*三个交易电话号码之三,14位数字,右补空格*/
  char manager_telno[14+1];   	/*一个管理电话号码,14位数字*/
  int  hand_flag;      			/*是否支持手工输入卡号 1:支持,0:不支持(默认不支持)*/
  int  trans_retry_count;				/*交易重发次数*/
  char pos_main_key_index[2+1];			/*终端主密钥索引值 默认'00'*/
  char pos_ip_port1[30+1];          /*无线POS IP1 和端口号1*/
  char pos_ip_port2[30+1];          /*无线POS IP1 和端口号2*/
  char rtn_track_pwd_flag[2+1];		/*消费退货是否需要磁道和密码 10:有磁道无密码*/
  char input_order_flg[2+1];     /*01:表示终端存在输入订单号界面 00:表示终端存在输入订单号界面*/
  int  encrpy_flag; 				/*当前报文是否加密的标记 0：未加密 1：加密*/
  int  pos_update_flag;         /*POS程序更新标志 0：不更新 默认 1：更新*/
  char pos_update_add[30+1];    /*pos程序更新地址 如： 192.168.28.168:32092*/
  char remitfeeformula[25+1];/*POS转账汇款手续费公式*/
  char creditfeeformula[25+1];/*POS信用卡还款手续费公式*/
  int tag26; /*支持的交易类型*/
  int tag39; /*支持的电银增值业务类型*/
  char para_update_mode;/*TMS参数下载方式*/
  char para_update_flag;/*是否需要TMS参数下载标记*/
  char pos_update_mode;/*POS应用程序更新方式*/
  char cur_version_info[4+1];/*当前应用程序版本信息*/
  char new_version_info[4+1];/*最新的应用程序版本信息*/
  char pos_kek2[32+1];/*远程主密钥2*/
  char kek2_check_value[16+1];/*主密钥2校验值*/
  /*-----modify begin 140325-----*/
  char T0_flag[1+1];/*T+0商户标志*/
  char agents_code[6+1];/*代理商渠道号*/
  char download_pubkey_flag[1+1];/*公钥下载标识*/
  char download_para_flag[1+1];/*参数下载 标识*/
  char orig_time[14+1];/*原交易时间*/
  /*---modify end by hw---*/
}MER_INFO_T;

typedef struct {
  char mgr_term_id[8+1];		/*终端号*/
  char mgr_mer_id[15+1];		/*商户号*/
  char mgr_des[3+1];			/*加密方法：DES：001 3DES：003  磁道密钥：004*/
  char mgr_pik[32+1]; 			/*pik工作密钥*/
  char mgr_pik_checkvalue[8+1]; 			/*pik工作密钥校验值*/
  char mgr_mak[32+1]; 			/*mak工作密钥*/
  char mgr_mak_checkvalue[8+1]; 			/*mak工作密钥校验值*/
  char mgr_tdk[32+1]; 			/*磁道工作密钥*/
  char mgr_tdk_checkvalue[8+1]; 			/*tdk工作密钥校验值*/
}MER_SIGNIN_INFO;

/*商户交易控制表*/
typedef struct {
	char blk_mercode[15+1];/*商户的商户号*/
	char blk_termcde[8+1];/*商户的终端号默认为00000000*/
	char blk_adddate[8+1];/*商户的商户号添加日期*/
	char blk_enddate[8+1];/*商户的商户号的结束日期*/
	char trancdelist[127+1];/* 默认为*，表示什么交易都不能做。如果为*,103，那么就是说明除了103交易码之外的交易都不能做*/
}MER_TRADE_RESTRICT;

/*刷卡终端的交易限制*/
typedef struct {
	char psam_card[16+1];/*终端序列号即psam卡号  主键*/
	int  psam_restrict_flag; /*该终端是否需要风控,0：风控，1：不风控*/
  	char restrict_trade[27+1];/*终端需要限制的交易类型，默认"-"*/
  	char restrict_card;/*终端需要限制的卡种 默认为"-"*/
  	char passwd;/*终端交易是否需要密码的标记 1：需要密码 0 ：不需要密码 默认为"-"*/
  	char amtmin[12+1];/*限制终端单笔交易的最小金额,,默认为"-"不限*/
  	char amtmax[12+1];/*限制终端单笔交易的最大金额, 默认为"-"不限*/
  	char c_amttotal[12+1];/*信用卡限制终端日交易限额, 默认为"-"不限*/
  	char c_amtdate[8+1];/*信用卡终端日期和交易总额对应 表示是哪一天的交易总额*/
    char c_total_tranamt[12+1];/*信用卡当天当前终端的当前交易总额"0"*/
    char d_amttotal[12+1];/*借记卡限制终端日交易限额, 默认为"-"不限*/
  	char d_amtdate[8+1];/*借记卡终端日期和交易总额对应 表示是哪一天的交易总额*/
    char d_total_tranamt[12+1];/*借记卡当天当前终端的当前交易总额"0"*/
    int fail_count;/*限制终端日交易的失败次数*/
  	int success_count;/*限制终端日交易的成功次数*/
}SAMCARD_TRADE_RESTRICT;

/*psam卡状态信息*/
typedef struct {
	char status_code[2+1];/*终端序列号即psam卡的状态码*/
  	char status_desc[25+1];/*终端状态的详细信息*/
  	char xpep_ret_code[2+1];/*终端状态对应的应答码*/
}SAMCARD_STATUS_INFO;

/*终端商户风控表*/
typedef struct {
	char psam_card[16+1];/*终端序列号即psam卡号 */
	char mercode[15+1]; /*商户号*/
	int  psammer_restrict_flag; /*该终端该商户是否需要风控,1：风控，0：不风控*/
  	char c_amttotal[12+1];/*信用卡限制终端日交易限额, 默认为"-"不限*/
  	char c_amtdate[8+1];/*信用卡终端日期和交易总额对应 表示是哪一天的交易总额*/
    char c_total_tranamt[12+1];/*信用卡当天当前终端的当前交易总额默认"0"*/
}SAMCARD_MERTRADE_RESTRICT;

/*智能终端个人收单商户配置信息结构*/
typedef struct {
	char cust_id[20+1]; /*用户号码，比如手机号码等*/
	char cust_psam[16+1]; /*psam*/
	char cust_merid[15+1];/*内部商户号*/
	char cust_termid[8+1];/*内部终端号*/
	char incard[23+1];/*转入卡号,个人银行卡*/
	char cust_ebicode[20+1];/*电银帐号*/
	char merdescr[50*3+1];/*商户名称*/
	char merdescr_utf8[200+1];/*商户名称的UTF8格式表示*/
/*-----modify begin 140325-----*/
  char T0_flag[1+1];/*T+0商户标志*/
  char agents_code[6+1];/*代理商渠道号*/
  char download_pubkey_flag[1+1];
  char download_para_flag[1+1];
 /*---modify end by hw---*/
}POS_CUST_INFO;

/*存放查询到的交易数据信息*/
typedef struct {
	char xpep_jieji_amount[12+1]; /*pep_jnls中借记总金额*/
	char xpep_daiji_amount[12+1];/*pep_jnls中贷记总金额*/
	char xpep_daiji_cx_amount[12+1];/*pep_jnls中贷记（撤销）总金额*/
	char xpep_daiji_th_amount[12+1];/*pep_jnls中贷记（退货）总金额*/
	int xpep_jieji_count;/*pep_jnls中借记总笔数*/
	int xpep_daiji_count;/*pep_jnls中贷记总笔数*/
	int xpep_daiji_cx_count;/*pep_jnls中贷记（撤销）总笔数*/
	int xpep_daiji_th_count;/*pep_jnls中贷记（退货）总笔数*/
}XPEP_CD_INFO;
/*存放批上送交易明细的信息*/
typedef struct {
	char termtrc[6+1]; /*上送交易明细的流水*/
	char cardno[20+1];/*上送交易明细的卡号*/
	char transamt[12+1];/*上送交易明细的交易金额*/
	char posmercode[15+1];/*批上送的商户号*/
	char postermcde[8+1];/*批上送的终端号*/
}BATCH_SEND_INFO;

/*存放EWP上送历史交易信息查询请求报文*/
typedef struct {
	char transtype[4+1]; /*交易类型N4*/
	char process[6+1];/*交易处理码N6*/
	char sendcode[4+1];/*发送机构号N4*/
	char trancde[3+1];/*交易码N3*/
	char psam[16+1];/*PSAM卡编号ANS16*/
	char termidm[20+1];/*机具编号20*/
	char merid[20+1];/*商户编号ANS20*/
	char selectdate[8+1];/*查询日期N8*/
	char sendtime[6+1];/*发送时间N6*/
	char startdate[8+1];/*查询开始日期N8*/
	char enddate[8+1];/*查询结束日期N8*/
	char perpagenum[1+1];/*每页显示条数N1*/
	char curpage[3+1];/*当前页N3*/
	char username[11+1];/*用户名N11*/
	char aprespn[2+1];/*应答码ANS2*/
	char tatalnu[3+1];/*总条数N3*/
	char curnum[1+1];/*当前返回条数*/
	char selfewp[20+1];/*ANS20EWP自定义域*/
	char retcode[2+1];/*解析数据是否成功的标记*/
	char queryflag[1+1];/*查询条件*/
	char querycdno[19+1];/*查询卡号*/
	char billmsg[40+1];/*交易订单号*/
	char trace[6+1];/*交易流水号*/
}EWP_HIST_RQ;


typedef struct{
	char fee_flag[4];
    char fee_min[9];
    char fee_max[9];
    char fee_r[9];
    char fee[9];
}STR_FEE;

typedef struct{
	char iccard_no[8+1];
  	int  iccard_remain_num;
}ICCARD_INFO;

/* 55域信息*/
typedef struct _fd55
{
    char szTag[5];/*tag标签*/
    char szValue[512];/*value取值*/
    char szLen[10];/*len长度*/
    int iStatus;/*使用状态*/
    char szPoolFld[20];/*预留域*/
}fd55, *stfd55;

typedef struct {
	char pepdate[8+1];	/*系统日期,前置系统的日期 */
	char peptime[6+1];	/*系统时间 ,前置系统的时间*/
	char trancde[3+1];	/*交易码 */
	char outcdno[20+1];	/*转出卡号 */
	long trace;		/*流水号 */
	char sendcde[8+1];	/*发送机构代码 */
	char termcde[8+1];	/*终端号 */
	char mercode[15+1];	/*商户号 */
	char samid[25+1];	/*SAM卡号 */
    char card_sn[3+1];		/*卡片序列号*/
    char send_bit55[512+1]; /*终端上送55域*/
    char recv_bit55[512+1]; /*核心返回55域*/
}PEP_ICINFO;


typedef struct {
   char	 RID[10+1];
   char  CAPKI[2+1] ;
   char  CAPK_VALID[8+1] ;
   char  CAPK_HASH_FLAG[2+1];
   char  CAPK_ARITH_FLAG[2+1];
   char  CAPK_MODUL[496+1];
   char  CAPK_EXPONENT[6+1];
   char  CAPK_CHECKVALUE[40+1];
   char  CAPK_ADD_DATE[8+1];
}ICPUBKINFO;

typedef struct {
   char	 AID[32+1];
   char  ASI[2+1] ;
   char  APP_VERNUM[4+1] ;
   char  TAC_DEFAULT[10+1];
   char  TAC_ONLINE[10+1];
   char  TAC_DECLINE[10+1];
   char  TERM_MINLIMIT[8+1];
   char  THRESHOLD[8+1];
   char  MAXTARGET_PERCENT[2+1];
   char  TARGET_PERCENT[2+1];
   char  DDOL_DEFAULT[256+1];
   char  TERMPINCAP[2+1];
   char	 VLPTRANSLIMIT[12+1];
   char	 CLESSOFFLINELIMITAMT[12+1];
   char	 CLESSTRANSAMT[12+1];
   char	 TERMCVM_LIMIT[12+1];
}ICPARAINFO;

typedef struct {
   char	 pepdate[8+1];
   char  peptime[6+1] ;
   char  outcdno[20+1] ;
   char  transamt[12+1];
   char  termtrc[6+1];
   char  bit22[3+1];
   char  termcde[8+1];
   char  mercode[15+1];
   char  card_sn[3+1];
   char  bit55[255+1];
   char  tc_value[16+1];
   char  bit62[25+1];
}PEPTCINFO;

typedef struct {
	char ksn[20+1];/*ksn编号*/
	char initksn[20+1];/*数据表里实际保存的KSN编号*/
	int  bdk_index;
	int  bdk_xor_index;
	int  transfer_key_index;
	int  channel_id;
	char initkey[48+1];
	char dukptkey[48+1];
	char pinkey[16+1];/*核心转加密密钥,待定*/
	char tak[24+1];/*mac计算密钥*/
	char tpk[24+1];/*pin密钥*/
	char tdk[24+1];/*磁道密钥*/
	char final_info[60+1];
}KSN_INFO;

typedef struct {
	char cust_id[20+1];
	char cust_merid[15+1];
	char cust_termid[8+1];
	char cust_accountno[23+1];
	char cust_merdescr[50+1];
	char cust_merdescr_utf8[200+1];
	char download_pubkey_flag[1+1];
	char download_para_flag[1+1];
	char pubkey_update_time[14+1];
	char para_update_time[14+1];
}MPOS_CUST_INFO;

typedef struct {
	char ordid[40+1];
	char amt[12+1];
	char orddesc[90+1];
	char ordtime[17+1];
	char hashcode[64+1];
}TWO_CODE_INFO;


typedef struct {
	char trans_name[40+1];	/*交易名称 */
	char transfer_type[2]; /*转账汇款类型 0：普通/实时 1：次日到账*/
	char in_acc_bank_name[64+1]; /*入账银行名称*/
	char pay_amt[12+1];/*入账金额*/
	char fee[9+1];/*手续费*/
	char pay_respn[2+1];/*代付应答码*/
	char pay_respn_desc[120+1];/*代付应答码描述*/
	char sale_respn_desc[120+1];/*扣款应答码描述*/
	char retcode[2+1];/*解析数据是否成功的标记*/
	char accountId[30+1];/*账户号*/
	char bankNo[12+1];/*开户联行号*/
	char orderId[20+1];/*订单号*/
	char purpose[120+1];/*用途*/
	char rcvAcno[32+1];/*卡号/折号*/
	char rcvAcname[60+1];/*持卡人姓名/公司*/
	char bgRetUrl[120+1];/*异步结果通知地址*/
	char transAmt[12+1];/*交易金额*/
	char transType[2+1];/*交易类型C1*/
	char sysDate[8+1];/*交易发起日期*/
	char sysTime[6+1];/*交易发起时间*/
	char transStatus[1+1];/*交易状态*/
	char response_code[2+1];/*返回终端应答码*/
	char tseq[20+1];/*交易流水号*/
	char errorMsg[120+1];/*错误信息*/
	char merPriv[120+1];/*商家私有域*/
	char chkValue[120+1];/*数字签名*/
	char payResultcode[10+1];/*代付返回码*/
	char payResultcode_desc[30+1];/*返回码描述信息*/
	char pepdate[8+1];	/*系统日期,前置系统的日期 */
    char peptime[6+1];	/*系统时间 ,前置系统的时间*/
    long trace;		/*流水号 */
    char termcde[8+1];	/*终端号 */
    char mercode[15+1];	/*商户号 */
    char billmsg[40+1];	/*帐单号 */
    char modecde[11+1]; /*手机号*/
}TRANSFER_INFO;


#define FREE_STAT  0 //空闲状态
#define BUSINESS_STAT 1  //忙碌状态

/*测试 md5 key：EAAoGBAMkYfh7MeT
生产 md5 key：p65KCKvYmyYdWb5G*/

#define TEST_MD5_KEY	   "EAAoGBAMkYfh7MeT"
#define PROT_MD5_KEY	   "p65KCKvYmyYdWb5G"

#endif
