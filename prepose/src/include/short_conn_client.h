#include "dccdcs.h"
#include "dccsock.h"
#include "extern_function.h"
/*
 * 短链客户端链路结构体,存放每条短链我方作为客户端链路的信息
 */
struct conn_c{
	char name[20];               //该短链路名字
	int iFid;//该link对应文件夹的Id
	char fold_name[20];          //该链路对应业务的Folder名称
	int    iApplFid;    //该link对应的应用进程的文件夹的Id
	char msgtype[4+1];  //报文类型
	char  commType[4];  //通讯模式,用来标识报文类型的，主要是报文长度类型
	char server_ip[67];//存放ip或者是域名
	int  server_port;
	int timeout;//超时时间
	int maxthreadnum;//设置每条短链最大的线程数
	int iStatus;//状态,是否在运行
	pid_t  pidComm;     //该link对应的TCP进程的进程Id
};

//定义存放所有短链客户端链接信息的共享内存结构
struct conn_c_Ast
{
	int    i_conn_num;          // 短链配置的条数
	struct conn_c connc[1];  
};
struct messBuf
{
	char buf[PACKAGE_LENGTH+1];
	int len;
};


#define SHORT_CONN_CLIENT_SHM_NAME "SC_CLIENT"

