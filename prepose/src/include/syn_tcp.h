/*  
 *文件名称：syn_tcp.h
 *功能描述：
 *支持TCP协议的客户端短链同步服务需要的头文件
*/
#include "dccdcs.h"
#include "dccsock.h"
#include "extern_function.h"
/*
 * 短链客户端链路结构体,存放每条短链我方作为客户端链路的信息
 */
struct syn_tcp_struct{
	char recv_queue[20];               /*接收队列名*/
	char send_queue[20];           /*发送队列名*/
	char  commType[4];   /*通讯模式,用来标识报文类型的，主要是报文长度类型*/
	char server_ip[67]; /*存放ip或者是域名*/
	int  server_port;
	int timeout; /*超时时间*/
	int maxthreadnum; /*设置每条短链最大的线程数*/
	int iStatus; /*状态,是否在运行*/
	pid_t  pidComm;      /*该link对应的TCP进程的进程Id*/
};

//定义存放所有短链客户端链接信息的共享内存结构
struct tcp_Ast
{
	int    i_conn_num;          // 短链配置的条数
	struct syn_tcp_struct tcpc[1];  
};

#define SYN_TCP_SHM_NAME "SYN_TCP"

