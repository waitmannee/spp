#ifndef EMCGLOBAL_H
#define EMCGLOBAL_H

#include "extern_function.h"

//#ifndef _MainProg
//#define EXTERN extern
//#else
//#define EXTERN
//#endif

#define EXTERN extern

EXTERN char   g_caSwaShmName[];	// 系统工作区的共享内存名字
EXTERN char   g_caBtaShmName[];	// 业务表格区的共享内存名字
EXTERN char   g_caTcaShmName[];	// 超时控制表的共享内存名字
EXTERN char   g_caMonFoldName[];	// 监视器共用文件夹名字
extern char   g_caIsaFoldName[];	// 跨行交易系统共用文件夹名字
EXTERN int    g_iTestMode;		// 测试模式；0:生产  1：测试
extern int    g_iMaxPacketSize;		// 网络传输中的最大包字节数
extern int    g_iIbTimer2;		// 联机跨行交易的最大超时时间
EXTERN int    g_iIbTimer3;		// 开始请求的超时时间
EXTERN int    g_iIbTimer4;		// 结束请求的超时时间
EXTERN int    g_iIbTimer5;		// 日切开始请求的超时时间
EXTERN int    g_iIbTimer6;		// 日切结束请求的超时时间
EXTERN char   g_caHostName[];		// 主机名称
EXTERN short  g_iListenPort;		// 侦听端口

extern int    g_iIsaFoldId;		// 存放跨行交易管理系统共用文件夹标识
EXTERN int    g_iMonFoldId;		// 存放环境管理系统共用文件夹标识

EXTERN int    g_iBcdaShmId;		// 共享内存BCDA的标识
//EXTERN char   *g_pcBcdaShmPtr;		// 共享内存BCDA的首地址
 char   *g_pcBcdaShmPtr;		// 共享内存BCDA的首地址

EXTERN int    g_iShutDownFlag;		// 通讯服务器进程关机通知。1：关机
EXTERN int    g_iMaxTxnPerTime;		// 同一时刻允许的最大交易量
EXTERN char   g_cTermFlag;		// 进程终止标志; 'y'表示终止

/* Mary add begin, 2001-6-5 */
EXTERN char   g_caCenterCode[12];	// 中心机构代码
EXTERN int    g_iCenterCodeLen;		// 中心机构代码长度
/* Mary add end, 2001-6-5 */

#endif
