/*  
 *文件名称：http_server.h
 *功能描述：
 *支持http协议的服务器端短链服务需要的头文件
*/
#include "dccdcs.h"
#include "dccsock.h"
#include "extern_function.h"

/*支持HTTP协议
 * 短链服务器链路结构体,存放每条http服务器链路的信息
 */
struct http_s{
	char name[20];               /*该短链路名字*/
	int iFid;/*该link对应文件夹的Id*/
	char fold_name[20];          /*该链路对应业务的Folder名称*/
	int    iApplFid;    /*该link对应的应用进程的文件夹的Id*/
	char msgtype[4+1];  /*报文类型*/
	int port;/*监听端口*/
	int iStatus;/*状态,是否在运行*/
	int timeout; 			/*超时时间*/
	int maxthreadnum; 		/*设置每条短链最大的线程数,即最大并发数*/
	pid_t  pidComm;    /*该link对应的TCP进程的进程Id*/
};

//定义存放所有http服务器端链接信息的共享内存结构
struct http_s_Ast
{
	int    i_http_num;          /* 短链配置的条数*/
	struct http_s https[1];  
};

#define HTTP_SERVER_SHM_NAME "HTTP_SERVER"

