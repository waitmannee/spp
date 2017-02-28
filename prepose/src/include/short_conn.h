#include "dccdcs.h"
#include "dccsock.h"
#include "extern_function.h"
/*
 * 短链链路结构体,存放每条短链的信息
 */
struct conn_s{
	char tpdu[10+1]; //tpdu
	int tpduFlag; //tpdu 是否可用,1可用,-1不可用
	char name[20];               //该短链路名字
	int iFid;//该link对应文件夹的Id
	char fold_name[20];          //该链路对应业务的Folder名称
	int    iApplFid;    //该link对应的应用进程的文件夹的Id
	char msgtype[4+1];  //报文类型
	char listen_ip[128];
	int  listen_port;
	int iStatus;//链路状态
	pid_t  pidComm;     //该link对应的TCP进程的进程Id
};

//定义存放所有短链链接信息的共享内存结构
struct conn_s_Ast
{
	int    i_conn_num;          // 短链配置的条数
	struct conn_s conns[1];  
};

#define SHORT_CONN_SHM_NAME "SC_SERVER"


