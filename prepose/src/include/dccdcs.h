#ifndef DCCDCS_H
#define DCCDCS_H

#include "folder.h"

// 定义常量
#define IS_TEST_OFF		0	// 测试模式关闭
#define DCS_BCDA_NAME		"BCDA"	// 共享内存BCDA的名字
#define DCS_HSTCOM_NAME  	"HstC"  // 通讯服务器的共享内存名

// 定义收件夹名称

#define DCS_CASHMEM_NAME       "CashMem"
//#define DCS_CASHREC_NAME       "ISAN"
#define DCS_CASHMAG_NAME       "CashMag"
#define DCS_POSSRV_FOLD_NAME   "PosSvr"
#define DCS_POSCOM_NAME        "POSCOM"

#define HSTSRV_FOLD_NAME       "HSTSRV"
#define DCS_HSTSRV_FOLD_NAME	HSTSRV_FOLD_NAME //defined in emcems.h("HSTC")
#define DCS_CASHREC_NAME        CASHREC_FOLD_NAME  //defined in emcems.h("cash")

#define DCS_COMMLNK_NAME	"isdcmlnk.conf"

// 定义跨行通讯连接的文件名
#define DCS_HSTSRV_LOG		"hst"

// 跨行通讯日志文件
#define DCS_NET_CONECT_ON	'1'	// 网络连接标志开启
#define DCS_NET_CONECT_OFF	'0'	// 网络连接标志关闭


// Add by xiezr 2001/08/29
#define MONITOR_CONNECT_WAIT_TIME	10
// add end by xiezr

//定义和成员行通信的每条链接的信息
struct linkinfo
{
	char   caFoldName[MAX_FOLDNAME_SIZE + 1]; /*该link对应文件夹的名字*/
	int    iFid;        /*该link对应文件夹的Id*/
	int    iApplFid;    /*该link对应的应用进程的文件夹的Id*/
	long   lFlags;      /*该link的标志字*/
	char   commType[4]; /*通道类型*/
	short  iRole;       /*该link在连接建立过程中的角色*/
	char   commNO[3];   /*通道顺序号*/
	short  iStatus;     /*该link当前 的状态*/
	char   caNotiFold[MAX_FOLDNAME_SIZE+1];   

	pid_t  pidComm;     /*该link对应的TCP进程的Id*/
	char   caLocalIp[20]; /*本地的IP地址*/
	char   caRemoteIp[20];/*期望的远端的IP地址*/
	int    sLocalPort;    /*本地的端口号*/
	int    sRemotePort;   /*期望的远端的端口号*/
	int    iTimeOut;     /* 网络侦测超时时间   // add for tang by 2001/03/26*/
	char    sActualIp[20];     /*实际的远端的IP地址*/
	int    nActualPort;   /*实际的远端的端口号*/
	int    iNotifyFlag;   /*链路状态通知标志*/
	char   caMsgType[4];   /*报文类型*/
	char heart_check_flag;  /*是否检查心跳包标识,'1'检查,'0' 不检查*/
  	int  heart_check_time;  /* 心跳检测时间间隔 */
};

//定义存放所有和成员行链接信息的共享内存结构
struct hstcom
{
    int    iLinkCnt;    //linkArray[]数组中的槽位计数
    int    iShmId;      //共享内存的Id
    struct linkinfo linkArray[1];
};

//
//HstSrv 接收的消息的结构
//
struct hstrequest     //通用格式
{
    char caMsgFlag[4];  // 命令标志:"ISEI"
    char caMsgType[4];  //消息类型
    char caMsgData[1];  //消息数据
};

struct commnotify     //Tcpip进程的通知格式
{
    char caMsgFlag[4];  // 命令标志:"ISEI"
    char caMsgType[4];  //消息类型
    char caFoldName[4]; //文件夹名称即链路名称
    int  iCommPid;
    int  iStatus;       //链路当前状态,0表示断开,1表示建立
};
struct trmcmd            //命令行接口来的请求格式
{
    char caMsgFlag[4];   // 命令标志:"ISEI"
    char caMsgType[4];   //消息类型
    char caFoldName[30]; //文件夹名称即链路名称
    int  iCommand;       //具体的命令
};


char   *g_pcBcdaShmPtr;             // 共享内存BCDA的首地址
struct hstcmd         //命令行接口来的请求格式
{
    char caMsgFlag[4];  // 命令标志:"ISEI"
    char caMsgType[4];  //消息类型
    char caFoldName[30]; //文件夹名称即链路名称
    int  iCommand;      //具体的命令
};

#define   DCS_FLAG_USED           0x00000001 //占用标志
#define   DCS_FLAG_DIRECT         0x00000002 //POS和中心直联
#define   DCS_FLAG_NACREADER      0x00000004
#define   DCS_FLAG_NACWRITER      0x00000008

#define   DCS_ROLE_PASSIVE        0 //被动（passive）角色
#define   DCS_ROLE_ACTIVE         1 //主动(active)角色
#define   DCS_ROLE_SIMPLEXSVR     2 //单工角色被动   simplexsvr单工单链路被动连接模式
#define   DCS_ROLE_SIMPLEXCLI     3 //单工角色主动   simplexcli单工单链路主动连接模式

#define   DCS_STAT_DISCONNECTED   0 //已经处于断开状态
#define   DCS_STAT_CALLING        1 //正在呼叫
#define   DCS_STAT_LISTENING      2 //正在监听
#define   DCS_STAT_CONNECTED      3 //已经处于连接状态
#define   DCS_STAT_STOPPED        4 //处于停止状态
#define DCS_STAT_RUNNING 15//进程处于运行状态 和stop相反

//for termcom
#define   DCS_STAT_READNET        5 //正在读网络
#define   DCS_STAT_WRITENET       6 //正在写网络
#define   DCS_STAT_READFOLD       7
#define   DCS_STAT_WRITEFOLD      8
#define   DCS_STAT_WAITREQ        9
#define   DCS_STAT_SENDREQ        10
#define   DCS_STAT_WAITRESP       11

//HstSrv接收的消息类型
#define	  DCS_LINKESTAB_NOTI      1001 //来自通信进程的链路建立通知
#define	  DCS_COMMAND_LINE        1002 //来自命令行接口的请求

//来自命令行接口的命令种类
#define   DCS_CMD_STOP            1   //停止一条处于连接态的链路
#define   DCS_CMD_START           2   //启动一条处于停止态的链路
#define   DCS_CMD_EXIT            3   //服务器退出运行
#define   DCS_CMD_ADD             4   //增加一条链路
#define   DCS_CMD_MODIFY          5   //修改一条链路
#define   DCS_CMD_DEL             6   //修改一条链路


//和NAC相连时的数据包头部之结构

#define  BYTE  unsigned char

// 前两个字节为接收包的长度，后5个字节为网控附加的TPDU头
// 在响应包返回时必须交换源地址和目的地址的值
struct nac_header
{
    BYTE  length[2];
    BYTE  dummy1[1];		//0x60
    BYTE  destaddr[2];		//源地址
    BYTE  srcaddr[2];		//目的地址
};
// 报文头类型
#define DCS_HEAD_TRM              1
#define DCS_HEAD_BANK             2
#define DCS_HEAD_ZFY              3
#define DCS_HEAD_AMP              4

struct trmgrpinfo
{
	char caFoldName[MAX_FOLDNAME_SIZE + 1];
	long lFlags;         //标志字
	int    iFid;
	pid_t pidComm;      //进程号
	short  iStatus;     //该terminal当前 的状态
    char    sRemoteIp[16];   //实际的远端的IP地址
    int    iRemotePort; //实际的远端的端口号
    int    iLocalPort;  //本地端口
};

struct trminfo
{
    char   caFoldName[MAX_FOLDNAME_SIZE + 1]; //该terminal对应文件夹的名字
    int    iGid;        //组号
    int    iFid;
    int    iApplFid;
    int    iRole;
    long   lFlags;      //该terminal的标志字
    short  iStatus;     //该terminal当前 的状态
    int    maxterm;
    int    minterm;
    int    iShmId;
    pid_t  pidComm;     //该terminal对应的TCP进程的Id
    int    iType ;      //通讯方式，主动还是被动
    char   sMsgType[5];  //报文类型
    int    iHeadType;    //报文头类型
    char   sRemoteIp[16];   //实际的远端的IP地址
    int    iRemotePort; //实际的远端的端口号
    int    iLocalPort;  //本地端口
    struct trmgrpinfo *trmArray;
};

struct trmcom
{
    int    iTrmCnt;  //elements# in array trmArray[]
    int    iShmId;
    struct trminfo linkArray[1];
};

//
//TrmSvr接收的消息类型
//
struct trmrequest     //通用格式
{
    char caMsgType[4];  //消息类型
    char caMsgData[1];  //消息数据
};
// comunication cortrol table(BCDA)
//struct BCDA
//{
//    char caBankCode[11];      // 成员行代码
//    int  iBankSeq;            // 成员行的顺序号，用作信号分量
//    char caBankName[15];      // 成员行名称
//    char cNetFlag;            // 网络连接数目
//    char cAppFlag;            // 应用连接标志
//    char cSafFlag;            // 存储转发标志
//    char cResetKeyFlag;       // 重置密钥标志
//    char cSecurityFlag;       // 安全检测标志
//    char cDateSwhBgnFlag;     // 日期切换标志
//    int  iDaySwhMaxTries;     // 重发日切开始请求的最大尝试次数
//    char caType[8];           // The Type of "sh p POSI ICSC"
//    
//};
//
//struct BCDAst
//{
//    int    iSemId;            //信号灯标识号,用于互斥访问
//    int    iBankNum;          // 成员行数目
//    struct BCDA stBcda[1];    // BCDA的首地址
//};

struct monilinkinfo
{
    long   lFlags;      //该link的标志字
    short  iRole;       //该link在连接建立过程中的角色
    short  iStatus;     //该link当前 的状态
    char   caLocalIp[20]; //本地的IP地址
    char   caRemoteIp[20];//期望的远端的IP地址
    int    sLocalPort;    //本地的端口号
    int    sRemotePort;   //期望的远端的端口号
    int    iTimeOut;      // 网络侦测超时时间   // add for tang by 2001/03/26
    int    iRemoteIp0;     //实际的远端的IP地址
    int    sRemotePort0;   //实际的远端的端口号
    int    timeOut;        //通信检测时间
    int    socket_id;
};
#endif

typedef  struct comm_link
{
	int num;
	int iRole;
	int iSocket[16];
	int flag[16];
}SOCKET_LINK_STATUS;
