/*  
 *文件名称：syn_http.h
 *功能描述：
 *支持http协议的客户端短链同步服务需要的头文件
*/
#include "dccdcs.h"
#include "dccsock.h"
#include "extern_function.h"
#include <curl/curl.h> 
/*
 * 短链客户端链路结构体,存放每条短链我方作为客户端链路的信息
*/
struct syn_httpc_struct{
	char recv_queue[20];    /*接收队列名*/
	char send_queue[20];    /*发送队列名*/
	char serverAddr[80];	/*存放服务器地址信息*/
	int timeout; 			/*超时时间*/
	int maxthreadnum; 		/*设置每条短链最大的线程数*/
	int iStatus; 			/*状态,是否在运行*/
	pid_t  pidComm;      	/*该link对应的进程的进程Id*/
};

//定义存放所有短链客户端链接信息的共享内存结构
struct httpc_Ast
{
	int i_conn_num;          // 短链配置的条数
	struct syn_httpc_struct httpc[1];  
};	

#define SYN_HTTPC_SHM_NAME "SYNHTTPC"

