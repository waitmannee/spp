#define PUB_MAX_LEN           1024
#ifndef _PUB_ERROR
#define _PUB_ERROR

#define PUB_SUCC			0
#define PUB_FAIL			-1

#define PUB_NETTEST			-2	/* Socket test */
#define PUB_NETFAIL			-3	/* Net connect failed */
#define PUB_SOCKERR			-4	/* Create socket error */
#define PUB_RMSGERR			-5	/* Read message from MSGQUEUE error */
#define PUB_WMSGERR			-6	/* Write message to MSGQUEUE error */
#define PUB_OPENERR			-7	/* Open file error */
#define PUB_COMMERR			-8	/* Serial port operate error */
#define PUB_RCVOT			-9	/* Receive data overtime */
#define PUB_DATAERR			-10	/* Data illegal */
#define PUB_NOPARM			-11	/* No the parameter */
#define PUB_CMSGERR			-12	/* Create MSGQUEUE error */
#define PUB_PKGLENERR		-13	/* Create MSGQUEUE error */

#define PUB_LENOV			-14
#define PUB_MSGLIM			-15 /* message record overflow */
#define PUB_RECOV			-16 /* 记录超限 */
#define PUB_SYSRUN			-17

#define PUB_CSHMERR			-20
#define PUB_GETSHM_ERR		-21
#define PUB_NODEF_SHM		-22
#define PUB_STANTYPE_ERR	-23

#define SHMDATA_NOREC		-24
#define SHMBIT_NOREC		-25
#define PUB_NOREC			-26

#define PUB_LEN_ILLEGAL		-50
#define PUB_DECRYPT_ERR		-51
#define PUB_TIMEOUT			-52

#define TRACK2_FMTERR		-61
#define TRACK3_FMTERR		-62

/* Error code for database */
#define DB_ERR				-401
#define NO_DBASE			-402
#define DB_PINERR			-403
#define DB_NORECORD			-404
#define DB_NOUSE			-405
#define NO_VALID_REC		-406
#define NO_USER				-407
#define NO_PASSWD			-408
#define NO_DBNAME			-409
#define NO_TABLE			-410
#define NO_SERVER			-411

#define NO_STANPARAM		-412

/* For analyse 8583 */
#define DB_COMMIT_ERR		-450
#define DB_NODATA			-451
#define DB_CURERR			-452
#define DATA_LEN_ERR		-453
#define DB_UPDATE_ERR		-454
#define CALPIN_ERR			-481
#define CALMAC_ERR			-482
#define CHKMAC_ERR			-483

#define DATA_FMT_ERR		-484
#define ORDER_FMT_ERR		-485
#define DATA_DEF_ERR		-486

#define WORK_DAY_ERR		-522

#define DBSAM_NOUSE			1001
#define DBSAM_USED			1002

#define NO_TRADE			"A1"
#define PKG_ERR				"A5"
#define DBA_ERR				"A6"		/* 数据库操作错误*/
#define SYS_ERR				"A7"		/* 系统异常 */

#define MAC_ERR				"A8"		/* 校验MAC错 */
#define TER_ERR				"A9"		/* 未进行注册 */
#define GET_STAN_ERR		"AA"		/* 取得系统流水号错误 */
#define SND_QERR			"AB"		/* 发送消息队列错误 */

#define GET_BILLERR			"91"		/* 取得帐单信息错误 */

#define BILL_EXP			"D1"		/* 帐单已过支付期限*/
#define NOBILL				"D2"		/* 不存在的帐单 */
#define NORET				"D3"		/* 已发出支付请求 */
#define BILL_PAID			"D4"		/* 帐单已支付 */
#define BILL_CANCEL			"D5"		/* 帐单已作撤销处理 */

#define PCSYS_ERR			"H0"		/* 其他组包错误 */
#define PCSJL_ERR			"H1"		/* 连接加密机错误 */
#define PCTRC2_ERR			"H2"		/* 二轨资料非法(>37) */
#define PCTRC3_ERR			"H3"		/* 三轨资料非法(>104) */

#define NO_DLBILL			"G0"		/* 无待下载的帐单 */
#define NO_DLPARM			"G1"		/* 无待下载参数 */

#endif	/* end of _PUB_ERROR */


#define MAX_MENUNUM		6
#define NO_ORGCODE		50
#define NO_CALLNO		51
#define NO_ZONE			52
#define NO_PARM			53
#define NO_BILLTYPE		54
#define SYSERR			55
#define BILL_LENERR		56
#define BILL_EXIST		57
#define	RCV_TIMEOUT		58
#define BILL_SNDERR		59
#define MERFMT_ERR		60
#define	AMTFMT_ERR		61
#define NO_MERCODE		62
#define BILL_BPAID		63
#define BILL_SENT		64
#define CALLNO_ERR		65
#define DATA_FMTERR		66
#define NOT_SEND		67
#define NET_BUSY		68
#define NO_BILL			69
