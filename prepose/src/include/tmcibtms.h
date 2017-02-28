#ifndef IBTMS_H
#define IBTMS_H

#define ISDBANK         "isdbank.conf"  // 成员行名称代码对应表
#define ISO_DEFINE      "isdisodf.conf"

#define BCDA_SHM_NAME       "BCDA"      // 通讯控制表的名称
#define MAX_BANK_NUM        30

#define TMS_AFTERTOUT_FOLD_NAME   "AfterTmHandler"
#define  TMS_AUTOSAFH_FOLD_NAME   "AutoSAF"
#define  TMS_SYSTEMMONI_FOLD_NAME   "SysMoni"

//  成员通讯控制标志定义
#define TMS_APCONN_ON       1   // 应用连接打开
#define TMS_APCONN_OFF      0   // 应用连接关闭
#define TMS_SAFLAG_ON       1   // 存储转发标志打开
#define TMS_SAFLAG_OFF      0   // 存储转发标志关闭
#define TMS_DATESW_ON       1   // 日期切换标志打开
#define TMS_DATESW_OFF      0   // 日期切换标志关闭
#define TMS_RESETKEY_ON     1   // 重置密钥标志打开
#define TMS_RESETKEY_OFF    0   // 重置密钥标志关闭

//  交易超时标志定义
#define TMS_TIMEOUT_ON      1   // 交易超时标志打开
#define TMS_TIMEOUT_OFF     0   // 交易超时标志关闭

//  交易超时标志定义
#define TMS_TIMEOUT_FUN     1   // 超时控制代码
#define TMS_INFOSWITCH_FUN  0   // 信息交换功能代码


#define TMS_SHUTDOWN_CMD    0   // 系统关机命令
#define TMS_TIMEOUT_CMD     1   // 超时请求
#define TMS_DATESWBGN_CMD   2   // 日切开始请求命令
#define TMS_DATESWEND_CMD   3   // 日切结束请求命令
#define TMS_RSTKEY_CMD      4   // 重置密钥命令
#define TMS_SNDDTL_CMD      5   // 发送脱机交易明细命令
#define TMS_TIMERNOTI_CMD   6   // 定时器定时通知
#define TMS_SNDSAF_CMD      7   // 发送SAF命令
#define TMS_SNDBLKLIST_CMD  8   // 发送黑名单资料下发命令

// add by xiezr 2001-06-07
// #define TimeoutTriggerReversalFinancialTranscationCmd   TMS_TIMEOUT_FUN
// end add by xiezr 2001-06-07

//added by myl 2000-10-12 for实时冲正
#define TMS_REALTIMESAF_CMD  9   // 实时冲正

#define TMS_SNDCMP_CMD      10  // 发送对帐命令
#define TMS_ADJUST_CMD      11   // \267\242\313\315ADJUST\303\374\301\356


#define TMS_POSI	    2000 // pos上送包
#define TMS_BANK	    2001 // 银行标准包
#define TMS_ISEI	    2002 // ISE内部报文格式
#define TMS_CIBC	    2003 // 总中心报文
#define TMS_SHOP		2004 // 商场前置报文

#define TMS_MSGCLS_INNER    1000 //内部管理类命令
#define TMS_MSGCLS_8583REQ  1001 //外部请求类8583报文
#define TMS_MSGCLS_8583RESP 1002 //外部应答类8583报文

#define  TMS_MSGDEST_SWITCH   1000  //交换中心为报文的目的地
#define  TMS_MSGDEST_ISSUER   1001  //发卡行为报文的目的地
#define  TMS_MSGDEST_ACQUIRER 1002  //受理行为报文的目的地

// comunication cortrol table(BCDA)
struct BCDA
{
    char caBankCode[11];      // 成员行代码
    int  iBankSeq;            // 成员行的顺序号，用作信号分量
    char caBankName[15];       // 成员行名称
    char cNetFlag;            // 网络连接数目
    char cAppFlag;            // 应用连接标志
    char cSafFlag;            // 存储转发标志
    char cResetKeyFlag;       // 重置密钥标志
    char cSecurityFlag;       // 安全检测标志
    char cDateSwhBgnFlag;     // 日期切换标志
    int  iDaySwhMaxTries;     // 重发日切开始请求的最大尝试次数
    short iUseFlag;               //使用标志
    char caType[8];	    // The Type of "sh p POSI ICSC"
    
};

struct BCDAst
{
    int    iSemId;            //信号灯标识号,用于互斥访问
    int    iBankNum;          // 成员行数目
    struct BCDA stBcda[1];    // BCDA的首地址
};

#define  BCDA_NETFLAG            1  // 网络连接数目
#define  BCDA_APPFLAG            2  // 应用连接标志
#define  BCDA_SAFFLAG            3  // 存储转发标志
#define  BCDA_RESETKEYFLAG       4  // 重置密钥标志
#define  BCDA_SECCHECK		 5  // 安全检测标志
#define  BCDA_DATESWHBGNFLAG     6  // 日期切换标志

// 关机请求命令数据格式
struct  SHUTDOWNCMD
{
    char caMsgType[4];        // 关机请求命令的信息类型码：0000
};

// 定时器通知数据格式
struct TIMERNOTI
{
    char caMsgFlag[4];        // 命令标志:"ISEI"
    char caMsgType[4];        // 超时请求命令的信息类型码：0006
    int  iTimerId;
};

// 超时请求命令的数据格式
struct TIMEOUTCMD {
    //  char caMsgFlag[4];        // 命令标志:"ISEI"
    char caMsgType[4];        // 超时请求命令的信息类型码：0001
    char caIbTxnSeq[30];      // 跨行交易流水号
    char caAcqCode[11];       // 受理机构代码:成员行或POS终端
    char caCardCode[11];      // 发卡行代号
    char caTxnDate[8];        // 交易日期
    char caProcCode[6];       // 交易处理代码
    int  iOrgFid;             // 发送进程的文件夹Id
    long iOrgMTI;             // MTI of original message
};

// 日切开始请求命令的数据格式
struct DSWBGNCMD {
    //  char caMsgFlag[4];
    char caMsgType[4];		// Message Type: 0002
    char caProcessCode[6];	// Processing Code: 000000
    char caBankCode[13];  	// Len+BankCode && Flag+Len+BankCode
    char caFunctionType[3];     	// the 70 field function Code
    char cRespCode;
};

// 日切结束请求命令的数据格式
struct DSWENDCMD {
    char caMsgType[4];        // 日切开始请求信息代号：0003
};

typedef struct {
    // char caMsgFlag[4];
    char caMsgType[4]; 		// Message Type 0004
    char caProcessCode[6];	// Processing Code: 000000
    char caBankCode[13];		// Len+BankCode && Flag+Len+BankCode
    char cKeyType;     		// Key Type: '0': Mac Key , '1': Pin Key
    char cRespCode;
} RSTKEYCMD;


struct SNDDTLCMD
{
    //  char caMsgFlag[4];        // 命令标志:"ISEI"
    char caMsgType[4];        // 脱机交易明细上送命令代号：0005
    char caBankCode[8];	    // 成员行代号
    char cRespCode;	        // 响应代码：'0'->发送成功; '1'：超时; '2'->否定应答;
    // '3'->发送完成; '4'->ISE internal error
};


struct SNDSAFCMD
{
    //  char caMsgFlag[4];        // 命令标志:"ISEI"
    char caMsgType[4];        // 脱机交易明细上送命令代号：0007
    char caBankCode[8];	    // 成员行代号
    char caSeqNo[6];	        // 流水号
    char caTxnDateTime[10];       // 交易处理代码
    char cRespCode;	        // 响应代码：'0'->已经发送; '1'->暂时已没有要发送的SAF记录
};
struct ADJUSTTXNCMD
{
    //    char caMsgFlag[4];      // 命令标志:"ISEI"
    char caMsgType[4];      // \265\367\325\373\275\273\322\327\303\374\301\356\ 307\353\307\363"0009"
    char caBankCode[8];     // 成员行代号
    char caSeqNo[6];        // 流水号
    char cRespCode;         // 响应代码：'0'->已经发送; '1'->暂时已没有要发送的SAF记录
};


// 实时冲正命令，由ISA通知给tmxautosaf.x
struct RTIMESAFCMD {
    //  char caMsgFlag[4];        // 命令标志:"ISEI"
    char caMsgType[4];        // 日切开始请求信息代号：0009
    char caBankCode[16];      // 成员行代号
    char caSeqNo[8];	        // 流水号
    char caProcCode[6];       // 交易处理代码
};

// 管理及错误报文数据库
struct IBMSG
{
    char caTxnDate[8]; // 交易日期
    char caTxnTime[6];        // 信息发生时间
    char caMsgData[1500];     // 报文信息
};

// 跨行交易流水数据库
struct IBTXNMSG
{
    char caTxnDate[8]; // 交易日期
    char caCntCode[11];       // 受理中心代号
    char caCntSeqNo[6];       // 受理中心交易顺序号
    char cTxnOkFlag;          // 交易成功标志；0：失败；1：成功
    char caIbTxnMsgData[1500];// 跨行交易报文
};

// 存储转发数据库
struct SAF
{
    char caTxnDate[8]; // 交易日期
    char caCntCode[11];       // 本中心代码
    char caCntSeqNo[6];       // 原交易顺序号
    char cSafOkFlag;          // 存储转发完成标志；0：未完成；1：完成
    char caSafData[1500];     // 存储转发的报文资料
};

// modify by xiezr 2001-06-01
#define  TMS_BANK_CODE_LEN  8
// modify end  by xiezr 2001-06-01


// Add By Xiezr 2001/07/11
// Add Struct of TextSnd
typedef struct {
    //	char caMsgFlag[4];      // FLAG  "ISEI"
	char caMsgType[4];	// Message Type ::MT_TEXT_SND_CMD
	char caProcessCode[6];	// Processing Code:: PC_TEXT_SND
	char caBankCode[13];	// Bank Members
	char caText[400];	// Sending Text
} SND_MSG;
// For Test Line Status
typedef struct {
    //	char caMsgFlag[4];      // FLAG  "ISEI"
	char caMsgType[4];	// Message Type ::MT_TEST_BANK_CMD
	char caProcessCode[6];	// Processing Code:: PC_TEST_BANK
	char caBankCode[13];	// Bank Members
    // Success Flag: && Bank Code
} LINE_STATUS;

// For Open Bank
typedef struct {
    //	char caMsgFlag[4];      // FLAG  "ISEI"
	char caMsgType[4];	// Message Type ::MT_OPEN_BANK_CMD
	char caProcessCode[6];	// Processing Code:: PC_OPEN_BANK
	char caBankCode[13];	// Bank Members
} OPEN_BANK;

// For Close BanK
typedef struct {
    //	char caMsgFlag[4];      // FLAG  "ISEI"
	char caMsgType[4];	// Message Type ::MT_CLOSE_BANK_CMD
	char caProcessCode[6];	// Processing Code:: PC_CLOSE_BANK
	char caBankCode[13];	// Bank Members
} CLOSE_BANK;

// For Recocilication
typedef struct {
    //	char caMsgFlag[4];	// FLAG "ISEI"
	char caMsgType[4];	// Message Type ::MT__RECONCILE_AS_POS_ACQUIRER_CMD
	char caProcessCode[6];	// Processing Code:: PC_RECONCILE_AS_POS_ACQUIRER
	char caFlag[1];		// The Result:: '1': OK, '2': NOT OK, '3':Error
	char caBankCode[13];	// Bank Members
	char caSettleDate[8];	// settle date
	char caDebitNum[10];	// Debit Number
	char caRevDebitNum[10];
	char caCreditNum[10];
	char caRevCreditNum[10];
	char caTransNum[10];
	char caRevTransNum[10];
	char caBalanceNum[10];
	char caAuthorNum[10];
	char caDebitServiceFee[12];
	char caCreditServiceFee[12];
	char caDebitAmount[16];
	char caRevDebitAmount[16];
	char caCreditAmount[16];
	char caRevCreditAmount[16];
	char caPureAmount[17];
} RECONCILE;

// For Fee Collication
typedef struct {
    //	char caMsgFlag[4];      // FLAG  "ISEI"
	char caMsgType[4];	// Message Type ::MT_FEE_COLLICATION_CMD
	char caProcessCode[6];	// Processing Code:: PC_FEE_COLLICATION
	char caBankCode[13];	// Bank Members
	char caAccount[21];	// Len(2)+Account
	char caAmount[12];	// Amount
	char caFeeCause[4];	// 60 field
	char caCauseTxt[256];	// Len(3)+253
} FEE_COLLECTION;

// for Key Reset Apply Command
typedef struct {
    //	char caMsgFlag[4];      // FLAG  "ISEI"
	char caMsgType[4];	// Message Type ::MT_KEYRESET_APPLY_CMD,
	char caProcessCode[6];	// Processing Code:: PC_KEYRESET_APPLY
	char caBankCode[13];	// Bank Members
	char caKeyType[1];	// 1: pin 2: MAC
} KEYRESETAPPLY;


// for System Date Switch Begin
typedef struct {
	char caMsgType[4];	// Message Type ::MT_SYS_DSWBGN_CMD
	char caProcessCode[6];	// Processing Code:: PC_SYS_DSWBGN
	char caSettledate[8];
	char caFlag;	// '0':normal, '1':must ;resp '0',succ, '1':fail
} SYS_DSWBGN;

// for system date switch end
typedef struct {
    //	char caMsgFlag[4];      // FLAG  "ISEI"
	char caMsgType[4];	// Message Type ::MT_SYS_DSWEND_CMD
	char caProcessCode[6];	// Processing Code:: PC_SYS_DSWEND
	char caRetCode[1];	// Return Code, '0', succ, '1': fail
} SYS_DSWEND;

typedef struct {
    //	char caMsgFlag[4];      // FLAG  "ISEI"
	char caMsgType[4];	// Message Type ::MT_CTRLERR_MSG_CMD
	char caProcessCode[6];	// Processing Code:: PC_CTRLERR_MSG
	char caRetCode[1];	// Return Code, '0', succ, '1': fail
	char caRequest[2];	// Type:
    // "01" :  Issue Bank Credit
    // "02" :  Issue Bank Debit
    // "03" :  Acquir Bank Credit
    // "04" :  Acquir Bank Debit
	char caPointOfService[2]; // 25 field, Point of services
    // "00" :  Nomal ("00")
    // "13" :  again Request ("13")
    // "17" :  back sheet ("17")
    // "41" :  back sheet again("41");
    
	char caCardNumber[21];	// Card Number, LL+CardNumber(<19)
	char caAmount[12];	// Trans Amount  // Not Used here
	char caMarchineType[4];	// MarchineType:: (18 field)
	char caMerchCode[15];	// MerchCode
	char caTermNumber[8];	// Terminal Number
	char caSettleDate[8];	// SettleDate  // Not Used
	char caAdjAmount[12];	// Adjust Amount( field 4)
	char caOrg_MsgType[4];  // Trans Type ( for 90 field)
	char caOrg_TraceNum[6]; // Trace Number( for 90 field)
	char caOrg_TraceTime[10]; // Trans Time ( for 90 field)
	char caOrg_AcquirCode[13];// Acquiring Bank Code (for 90 field)
	char caOrg_FwdCode[13];   // forwarding institute  Code (for 90 field)
	char caAcquiringBank[13]; // Acquiring Bank Code ( 32 field) // not used
	char caIssueBankCode[13]; // Receiveing  Bank Code ( 100field)
	char caMessageCode[4];	// Message Cause Code (60 field)
} ADJUST_ACCOUNT;

// for Settlement manager control
typedef struct {
    //	char caMsgFlag[4];      // FLAG  "ISEI"
	char caMsgType[4];	// Message Type ::MT_TEXT_SND_CMD
	char caProcessCode[6];	// Processing Code:: PC_TEXT_SND
	char caBankCode[13];	// Bank Members
	char caSettledate[8];
	char caSettledata[300];	// SettleData
} SETTLEDATA;

#endif
