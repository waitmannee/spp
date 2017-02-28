#ifndef __IBDCS__H__
#define __IBDCS__H__

#include "folder.h"
#include "timer.h"
#include "iso8583.h"

#define IBDCS_SHM_NAME      "IbDC"

#define MONITOR_FOLD_NAME   "Monitor"
#define APPSRV1_FOLD_NAME   "AppServer1"
#define APPSRV2_FOLD_NAME   "AppServer2"
#define DCSCOM_FOLD_NAME    "DcsComm"
#define TIMERSVR_FOLD_NAME  "TimerServer"
#define APPSRV_FOLD_NAME    "ISAN"


#define SEC_KEY_TYPE_TPK 0
#define SEC_KEY_TYPE_TAK 1
#define SEC_KEY_TYPE_TDK 2

/*IBDCS系统配置参数*/

#define DCS_KEY_SIZE  8
typedef struct tagIbdcsCfg
{
    /*IP和端口号*/
    char cSwIP[32]; /*交换中心的IP地址*/
    int iSwPort;    /*交换中心发送数据的端口*/
    int iLocPort;
    
    /*共享内存中超时表的记录数*/
    int iTbTRows;                  /* 超时表中的记录条数*/
    
    /*超时时间*/
    int iT1;  /*建链请求的超时*/
    int iT2;  /*结束请求的超时*/
    int iT3;  /*交易超时的请求*/
    
    int    iMacKUse; /* 标志是否使用MacKey*/
    int    iRunMode; /*运行模式:0--调试模式;1--生产模式*/
    int    iHeartBeating;
    
    char   szSendInsCode[16]; /*发信机构代码*/
    char   szRecvInsCode[16]; /*收信机构代码*/
} IBDCS_CFG;

/*IBDCS系统的状态*/
typedef struct tagIBDCS_status
{
    int ComProc_Status;   /*通信进程的状态*/
    int Tcp_Connections;  /*与中心的TCP连接数目*/
    int Appl_Ready;       /*与中心的应用连接是否就绪*/
    int CYH_B_Req;        /*成员行发开始请求报文状态字*/
} IBDCS_STAT;

#define SYSSTAT_LOW      0
#define COMPROC_STATUS   1  /*通信进程的状态*/
#define TCP_CONNECTIONS  2  /*与中心的TCP连接是否就绪*/
#define APPL_READY       3  /*与中心的应用连接是否就绪*/
#define CYH_B_REQ        4  /*成员行发开始请求报文状态字*/
#define SYSSTAT_UPPER    5

#define TIMEOUT1        20
#define TIMEOUT2        21
#define TIMEOUT3        22
#define MACKEY_USED     23
#define RUN_MODE        24

#define BANK_CODE_LENGTH  7  /*联行行号长度*/
#define INST_SWITCH       1
#define INST_BANK         0

extern void pre_dcs_log(char * filename,long line,void *ptrbytes, int nbytes,const char * message,...);
extern void pre_dcs_debug(char * filename,long line,void *ptrbytes, int nbytes,const char * message,...);

#define dcs_log(ptrbytes, nbytes,message,...)   pre_dcs_log(__FILE__,__LINE__,(ptrbytes),(nbytes),(message),##__VA_ARGS__)
#define dcs_debug(ptrbytes, nbytes,message,...)  pre_dcs_debug(__FILE__,__LINE__,(ptrbytes),(nbytes),(message),##__VA_ARGS__)



/*超时表的记录格式*/
typedef struct eventqueue TBT_AREA;
typedef struct eventitem  TBT_BUF;

#define  TBT_STAT_USED          0x0001

#define  MAX_CHILDREN_NUM       10

/*layout of shared memory in IBDCS*/
typedef struct tagIbdcsShm
{
    int           is_semid;     /*含3个分量的信号量;*/
    /*0号分量用来互斥访问共享内存中的*/
    /*is_stat区; 1,2号分量用来访问超时表部分*/
    int           is_MoniPid;   /*monitor进程的pid*/
    int           is_nchld;     /*子进程的个数*/
    int           is_children[MAX_CHILDREN_NUM]; /*各子进程的pid*/
    
    IBDCS_CFG     is_config;    /*系统配置参数区*/
    IBDCS_STAT    is_stat;      /*IBDCS系统的状态*/
    TBT_AREA      is_tmTbl;     /*超时表*/
}IBDCSSHM;

#define MAX_IBDCS_MSG_SIZE 2048

struct IBDCSPacket
{
    short  pkt_cmd;     /*请求包命令,定义见下*/
    short  pkt_val;     /*请求的数据,含义依赖于不同的命令*/
    long   pkt_bytes;   /*请求数据区中的字节数*/
    char   pkt_buf[1];  /*请求的数据区,含义依赖于不同的命令*/
};
/*用在超时控制中*/
struct savedtxn              /*保存原始请求保文的数据格式*/
{
    int fromfid;   /* 该报文从哪个文件夹发送来的*/
    int bytes ;    /*报文的字节长度*/
    int databuf[1];   /* 报文内容*/
};


#define ISDSERVER_SHM_NAME "ISDSRVER"

/*
 * isdsrver.conf 中服务结构体
 */
struct serverconf {
    char subsystem[10];
    char command[20];
    int  max;
    int  min;
    char para_area[40];
};

struct ServerAst
{
    int    serverNum;           // 成员行数目
    struct serverconf server[1];
};


#define PKT_CMD_TCPCONN         1000   /*tcp链路状态发生改变*/
#define PKT_CMD_DATAFROMSWITCH  1002   /*来自交换中心的交易数据*/
#define PKT_CMD_CYHREQ          1003   /*发送成员行开始请求*/
#define PKT_CMD_BUSIDOWN        1004   /*业务系统结束交易*/
#define PKT_CMD_DATATOSWITCH    1005   /*发往交换中心的交易数据*/
#define PKT_CMD_TIMER           1006

extern IBDCSSHM     *g_pIbdcsShm;
extern IBDCS_CFG    *g_pIbdcsCfg;
extern IBDCS_STAT   *g_pIbdcsStat;
extern TBT_AREA     *g_pTimeoutTbl;
extern unsigned long g_seqNo;

/*dcs_log.c*/
int  dcs_log_open(const char * logfile, char *ident);
int  dcs_set_logfd(int fd);
void dcs_log_close();
//void dcs_log(void *pbytes, int nbytes,const char * message, ...);
//void dcs_debug(void *pbytes, int nbytes,const char * message, ...);

void dcs_dump(void *pbytes, int nbytes,const char * message,...);

/*int dcs_chkmsg.c*/

/*dcs_shminit.c*/
int dcs_delete_shm();
int dcs_create_shm(int tblrows);
int dcs_connect_shm();

/*dcs_sysconf.c*/
int dcs_load_config(IBDCS_CFG *pIbdcsCfg);
int dcs_set_sysconf(int which, int val);
int dcs_dump_sysconf(FILE *fpOut);

/*dcs_sysstat.c*/
int dcs_get_sysstat(int which, int* val);
int dcs_set_sysstat(int which, int val);
int dcs_dump_sysstat(FILE *fpOut);

/*dcs_seqno.c*/
int dcs_save_seqNo();
unsigned long dcs_make_seqNo();

/*from dcs_sock.c*/
extern int tcp_open_server(const char *listen_addr, int listen_port) ;
extern int tcp_accept_client(int listensock,int *cliaddr, int *cliport);
extern int tcp_connet_server(char *servaddr, int servport, int cliport);
extern int tcp_close_socket(int sockfd);
extern int tcp_check_readable(int conn_sockfd,int ntimeout);
extern int tcp_check_writable(int conn_sockfd,int ntimeout);
extern int tcp_write_nbytes(int conn_sock, const void *buffer, int nbytes);
extern int read_msg_from_net(int connfd,void *msgbuf,int nbufsize,int ntimeout);
extern int write_msg_to_net(int connfd,void *msgbuf, int nbytes,int ntimeout);
extern int tcp_pton(char *strAddr);

#endif /*__IBDCS__H__*/
