#include "dccdcs.h"
#include "dccsock.h"
#include "extern_function.h"
/*支持HTTP协议
 * 短链客户端链路结构体,存放每条http客户端链路的信息
 */
struct http_c{
	char name[20];               //该短链路名字
	int iFid;//该link对应文件夹的Id
	char fold_name[20];          //该链路对应业务的Folder名称
	int    iApplFid;    //该link对应的应用进程的文件夹的Id
	char msgtype[4+1];  //报文类型
	char serverAddr[80];//存放服务器地址信息
	char server_ip[60];//存放ip或者是域名
	int  server_port;
	int timeout;//超时时间
	int maxthreadnum;//设置每条短链最大的线程数
	int iStatus;//状态,是否在运行
	pid_t  pidComm;     //该link对应的TCP进程的进程Id
};

//定义存放所有http客户端链接信息的共享内存结构
struct http_c_Ast
{
	int    i_http_num;          // 短链配置的条数
	struct http_c httpc[1];  
};

struct httpMessBuf
{
	char buf[PACKAGE_LENGTH+1];
	int len;
};


#define HTTP_CLIENT_SHM_NAME "HTTP_CLIENT"


